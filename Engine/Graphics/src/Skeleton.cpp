#include <kimia/Skeleton.h>

#include <cmath>

namespace kimia {

namespace {

constexpr f64 kEpsilon = 1e-12;

Vec3 lerpVec3(const Vec3& a, const Vec3& b, f64 t) { return a + (b - a) * t; }

// Shortest-arc quaternion interpolation. Falls back to a normalized lerp
// for nearly-identical rotations, where slerp's sine term goes to zero.
Quat slerp(const Quat& a, const Quat& b, f64 t) {
  f64 ax = a.x, ay = a.y, az = a.z, aw = a.w;
  f64 bx = b.x, by = b.y, bz = b.z, bw = b.w;
  f64 dot = ax * bx + ay * by + az * bz + aw * bw;
  // Take the short way round: q and -q are the same rotation.
  if (dot < 0.0) {
    bx = -bx;
    by = -by;
    bz = -bz;
    bw = -bw;
    dot = -dot;
  }
  if (dot > 0.9995) {
    Quat out;
    out.x = ax + (bx - ax) * t;
    out.y = ay + (by - ay) * t;
    out.z = az + (bz - az) * t;
    out.w = aw + (bw - aw) * t;
    return out.normalized();
  }
  const f64 theta = std::acos(dot);
  const f64 sinTheta = std::sin(theta);
  if (std::abs(sinTheta) < kEpsilon) return a;
  const f64 scaleA = std::sin((1.0 - t) * theta) / sinTheta;
  const f64 scaleB = std::sin(t * theta) / sinTheta;
  Quat out;
  out.x = ax * scaleA + bx * scaleB;
  out.y = ay * scaleA + by * scaleB;
  out.z = az * scaleA + bz * scaleB;
  out.w = aw * scaleA + bw * scaleB;
  return out.normalized();
}

Transform3D blend(const Transform3D& a, const Transform3D& b, f64 t) {
  Transform3D out;
  out.position = lerpVec3(a.position, b.position, t);
  out.scale = lerpVec3(a.scale, b.scale, t);
  out.rotation = slerp(a.rotation, b.rotation, t);
  return out;
}

}  // namespace

i32 Skeleton::findBone(const std::string& name) const {
  for (usize i = 0; i < bones.size(); ++i) {
    if (bones[i].name == name) return static_cast<i32>(i);
  }
  return kNoParentBone;
}

bool Skeleton::isValid() const {
  for (usize i = 0; i < bones.size(); ++i) {
    const i32 parent = bones[i].parent;
    if (parent == kNoParentBone) continue;
    if (parent < 0 || static_cast<usize>(parent) >= bones.size()) return false;
    // A parent must come first, which is what lets one forward pass work.
    if (static_cast<usize>(parent) >= i) return false;
  }
  return true;
}

f64 VertexSkin::totalWeight() const {
  f64 total = 0.0;
  for (u32 i = 0; i < kMaxBoneInfluences; ++i) total += weights[i];
  return total;
}

void VertexSkin::normalize() {
  const f64 total = totalWeight();
  if (total <= kEpsilon) return;  // no influences: leave it rigid
  for (u32 i = 0; i < kMaxBoneInfluences; ++i) weights[i] /= total;
}

bool SkinnedMesh::isValid() const {
  if (!bindMesh.isValid()) return false;
  if (!skeleton.isValid() || skeleton.isEmpty()) return false;
  if (skins.size() != bindMesh.positions.size()) return false;
  for (const VertexSkin& skin : skins) {
    for (u32 i = 0; i < kMaxBoneInfluences; ++i) {
      if (skin.weights[i] == 0.0) continue;
      if (static_cast<usize>(skin.bones[i]) >= skeleton.bones.size()) return false;
    }
  }
  return true;
}

Transform3D sampleTrack(const BoneTrack& track, f64 time) {
  if (track.keys.empty()) return Transform3D{};
  if (track.keys.size() == 1U) return track.keys.front().pose;
  // Hold the ends rather than extrapolating.
  if (time <= track.keys.front().time) return track.keys.front().pose;
  if (time >= track.keys.back().time) return track.keys.back().pose;
  for (usize i = 1; i < track.keys.size(); ++i) {
    const BoneKey& next = track.keys[i];
    if (time > next.time) continue;
    const BoneKey& previous = track.keys[i - 1U];
    const f64 span = next.time - previous.time;
    if (span <= kEpsilon) return next.pose;
    return blend(previous.pose, next.pose, (time - previous.time) / span);
  }
  return track.keys.back().pose;
}

f64 clipTime(const AnimationClip& clip, f64 time) {
  if (clip.duration <= kEpsilon) return 0.0;
  if (!clip.loop) {
    if (time <= 0.0) return 0.0;
    return time >= clip.duration ? clip.duration : time;
  }
  f64 wrapped = std::fmod(time, clip.duration);
  if (wrapped < 0.0) wrapped += clip.duration;  // running the clip backwards
  return wrapped;
}

void samplePose(const Skeleton& skeleton, const AnimationClip& clip, f64 time, std::vector<Transform3D>& out) {
  out.assign(skeleton.bones.size(), Transform3D{});
  // Start from the rest pose, so bones the clip says nothing about simply
  // stay where the artist left them.
  for (usize i = 0; i < skeleton.bones.size(); ++i) out[i] = skeleton.bones[i].restPose;
  const f64 local = clipTime(clip, time);
  for (const BoneTrack& track : clip.tracks) {
    if (track.bone < 0 || static_cast<usize>(track.bone) >= out.size()) continue;
    if (track.keys.empty()) continue;
    out[static_cast<usize>(track.bone)] = sampleTrack(track, local);
  }
}

void computeWorldMatrices(const Skeleton& skeleton, const std::vector<Transform3D>& localPoses,
                          std::vector<Mat4>& out) {
  const usize count = skeleton.bones.size();
  out.assign(count, Mat4{});
  for (usize i = 0; i < count; ++i) {
    const Mat4 local = i < localPoses.size() ? localPoses[i].toMat4() : skeleton.bones[i].restPose.toMat4();
    const i32 parent = skeleton.bones[i].parent;
    // The parent is always earlier in the array (Skeleton::isValid), so its
    // world matrix is already finished.
    if (parent == kNoParentBone || static_cast<usize>(parent) >= i) {
      out[i] = local;
    } else {
      out[i] = out[static_cast<usize>(parent)] * local;
    }
  }
}

void computeSkinMatrices(const Skeleton& skeleton, const std::vector<Transform3D>& localPoses,
                         std::vector<Mat4>& out) {
  std::vector<Mat4> world;
  computeWorldMatrices(skeleton, localPoses, world);
  out.assign(world.size(), Mat4{});
  for (usize i = 0; i < world.size(); ++i) {
    out[i] = world[i] * skeleton.bones[i].inverseBindPose;
  }
}

bool skinMesh(const SkinnedMesh& mesh, const std::vector<Mat4>& skinMatrices, MeshData& out) {
  if (!mesh.isValid()) return false;
  if (skinMatrices.size() != mesh.skeleton.bones.size()) return false;

  out = mesh.bindMesh;  // topology, uvs and names carry over untouched
  const bool hasNormals = mesh.bindMesh.normals.size() == mesh.bindMesh.positions.size();
  for (usize v = 0; v < mesh.bindMesh.positions.size(); ++v) {
    const VertexSkin& skin = mesh.skins[v];
    const f64 total = skin.totalWeight();
    if (total <= kEpsilon) continue;  // unweighted vertex: stays rigid

    const Vec3& bindPosition = mesh.bindMesh.positions[v];
    Vec3 position{0.0, 0.0, 0.0};
    Vec3 normal{0.0, 0.0, 0.0};
    for (u32 i = 0; i < kMaxBoneInfluences; ++i) {
      const f64 weight = skin.weights[i];
      if (weight == 0.0) continue;
      const usize bone = static_cast<usize>(skin.bones[i]);
      if (bone >= skinMatrices.size()) continue;
      // Weights are normalized on the fly, so an asset whose weights do not
      // quite sum to one still skins correctly instead of shrinking.
      const f64 share = weight / total;
      position += (skinMatrices[bone] * bindPosition) * share;
      if (hasNormals) normal += skinMatrices[bone].transformDirection(mesh.bindMesh.normals[v]) * share;
    }
    out.positions[v] = position;
    if (hasNormals) {
      const f64 length = normal.length();
      out.normals[v] = length > kEpsilon ? normal * (1.0 / length) : mesh.bindMesh.normals[v];
    }
  }
  return true;
}

bool poseMesh(const SkinnedMesh& mesh, const AnimationClip& clip, f64 time, MeshData& out) {
  if (!mesh.isValid()) return false;
  std::vector<Transform3D> pose;
  samplePose(mesh.skeleton, clip, time, pose);
  std::vector<Mat4> matrices;
  computeSkinMatrices(mesh.skeleton, pose, matrices);
  return skinMesh(mesh, matrices, out);
}

// --- Posing a figure without a model (stage 33) ---
//
// A rig, not a character. The engine supplies the joints so a player is
// visibly a figure with limbs; the ART is always the user's to bring.

namespace {

// Proportions as fractions of total height, near enough to a real body
// that the walk reads correctly.
constexpr f64 kHipHeight = 0.53;
constexpr f64 kChestRise = 0.24;
constexpr f64 kHeadRise = 0.16;
constexpr f64 kShoulderHalf = 0.11;
constexpr f64 kHipHalf = 0.055;
constexpr f64 kUpperArm = 0.16;
constexpr f64 kForearm = 0.15;
constexpr f64 kThigh = 0.26;
constexpr f64 kShin = 0.25;

Bone makeBone(const char* name, i32 parent, const Vec3& offset) {
  Bone bone;
  bone.name = name;
  bone.parent = parent;
  bone.restPose.position = offset;
  return bone;
}

}  // namespace

Skeleton makeFigureRig(f64 height) {
  if (height <= 0.0) height = 1.7;
  Skeleton rig;
  rig.bones.reserve(static_cast<usize>(FigureBone::Count));

  // Hips are the root: everything else hangs off them, so moving the root
  // moves the whole figure.
  rig.bones.push_back(makeBone("Hips", kNoParentBone, Vec3{0.0, height * kHipHeight, 0.0}));
  rig.bones.push_back(makeBone("Chest", 0, Vec3{0.0, height * kChestRise, 0.0}));
  rig.bones.push_back(makeBone("Head", 1, Vec3{0.0, height * kHeadRise, 0.0}));

  // Arms hang from the chest, legs from the hips. Each limb bone must
  // carry the LENGTH of its segment, not just the offset to one side:
  // rotating a zero-length bone moves nothing, which is exactly what went
  // wrong the first time — the legs refused to swing at all.
  //
  // LeftArm sits at the shoulder and reaches down the upper arm; LeftHand
  // continues down the forearm. Same idea for the legs, so a knee is a
  // real hinge at the bottom of a real thigh.
  rig.bones.push_back(makeBone("LeftArm", 1, Vec3{height * kShoulderHalf, -height * kUpperArm, 0.0}));
  rig.bones.push_back(makeBone("LeftHand", 3, Vec3{0.0, -height * kForearm, 0.0}));
  rig.bones.push_back(makeBone("RightArm", 1, Vec3{-height * kShoulderHalf, -height * kUpperArm, 0.0}));
  rig.bones.push_back(makeBone("RightHand", 5, Vec3{0.0, -height * kForearm, 0.0}));
  rig.bones.push_back(makeBone("LeftLeg", 0, Vec3{height * kHipHalf, -height * kThigh, 0.0}));
  rig.bones.push_back(makeBone("LeftFoot", 7, Vec3{0.0, -height * kShin, 0.0}));
  rig.bones.push_back(makeBone("RightLeg", 0, Vec3{-height * kHipHalf, -height * kThigh, 0.0}));
  rig.bones.push_back(makeBone("RightFoot", 9, Vec3{0.0, -height * kShin, 0.0}));

  // The bind pose is the rest pose, so a figure posed at rest is exactly
  // the shape built here.
  std::vector<Transform3D> rest(rig.bones.size());
  for (usize i = 0; i < rig.bones.size(); ++i) rest[i] = rig.bones[i].restPose;
  std::vector<Mat4> world;
  computeWorldMatrices(rig, rest, world);
  for (usize i = 0; i < rig.bones.size(); ++i) rig.bones[i].inverseBindPose = world[i].inverse();
  return rig;
}

void poseFigure(const Skeleton& rig, const FigureMotion& motion, std::vector<Transform3D>& out) {
  out.assign(rig.bones.size(), Transform3D{});
  for (usize i = 0; i < rig.bones.size(); ++i) out[i] = rig.bones[i].restPose;
  if (rig.bones.size() < static_cast<usize>(FigureBone::Count)) return;

  const auto index = [](FigureBone bone) { return static_cast<usize>(bone); };
  const auto swing = [](f64 radians) { return Quat::fromAxisAngle(Vec3{1.0, 0.0, 0.0}, radians); };

  // Knocked out: fold forward and drop. Nothing else matters.
  if (motion.downed) {
    out[index(FigureBone::Hips)].rotation = Quat::fromAxisAngle(Vec3{1.0, 0.0, 0.0}, -1.4);
    out[index(FigureBone::Hips)].position.y *= 0.25;
    return;
  }

  // In the air: tuck the legs and lift the arms, so a jump reads as a jump.
  if (motion.airborne) {
    // The knees come up too, or the feet trail on the floor and the figure
    // looks like it is standing rather than jumping.
    out[index(FigureBone::LeftFoot)].rotation = swing(1.1);
    out[index(FigureBone::RightFoot)].rotation = swing(0.9);
    out[index(FigureBone::LeftLeg)].rotation = swing(-0.7);
    out[index(FigureBone::RightLeg)].rotation = swing(-0.5);
    out[index(FigureBone::LeftArm)].rotation = swing(-1.2);
    out[index(FigureBone::RightArm)].rotation = swing(-1.2);
    return;
  }

  // Walking: opposite arm to opposite leg, the way people actually move.
  // Both the stride length and the cadence grow with speed, so a sprint
  // does not look like a stroll played fast.
  const f64 pace = motion.speed < 0.0 ? 0.0 : motion.speed;
  if (pace < 0.05) return;  // standing still: the rest pose is correct
  const f64 reach = std::min(0.85, pace * 0.16);
  const f64 cadence = 2.2 + std::min(pace, 8.0) * 0.55;
  const f64 phase = std::sin(motion.time * cadence);

  out[index(FigureBone::LeftLeg)].rotation = swing(phase * reach);
  out[index(FigureBone::RightLeg)].rotation = swing(-phase * reach);
  // Knees only bend forwards, so the trailing leg bends and the leading
  // one stays straight — a leg that hinges backwards looks broken.
  out[index(FigureBone::LeftFoot)].rotation = swing(std::max(0.0, -phase) * reach * 0.9);
  out[index(FigureBone::RightFoot)].rotation = swing(std::max(0.0, phase) * reach * 0.9);
  out[index(FigureBone::LeftArm)].rotation = swing(-phase * reach * 0.8);
  out[index(FigureBone::RightArm)].rotation = swing(phase * reach * 0.8);
  // A slight bob: the body rises on each step.
  out[index(FigureBone::Hips)].position.y += std::abs(phase) * reach * 0.035;
}

void figureLimbs(const Skeleton& rig, const std::vector<Transform3D>& pose, const Vec3& position, f64 yaw,
                 std::vector<FigureLimb>& out) {
  out.clear();
  if (rig.bones.size() < static_cast<usize>(FigureBone::Count)) return;

  std::vector<Mat4> world;
  computeWorldMatrices(rig, pose, world);

  // Turn the figure to face its heading, then stand it where it belongs.
  const f64 sinYaw = std::sin(yaw);
  const f64 cosYaw = std::cos(yaw);
  const auto place = [&](const Vec3& local) {
    return Vec3{position.x + local.x * cosYaw + local.z * sinYaw, position.y + local.y,
                position.z - local.x * sinYaw + local.z * cosYaw};
  };
  const auto jointAt = [&](FigureBone bone) {
    return place(world[static_cast<usize>(bone)] * Vec3{0.0, 0.0, 0.0});
  };

  const auto limb = [&](FigureBone a, FigureBone b, f64 thickness) {
    FigureLimb segment;
    segment.from = jointAt(a);
    segment.to = jointAt(b);
    segment.thickness = thickness;
    out.push_back(segment);
  };

  limb(FigureBone::Hips, FigureBone::Chest, 0.13);   // torso
  limb(FigureBone::Chest, FigureBone::Head, 0.09);   // neck
  limb(FigureBone::Chest, FigureBone::LeftArm, 0.06);
  limb(FigureBone::LeftArm, FigureBone::LeftHand, 0.055);
  limb(FigureBone::Chest, FigureBone::RightArm, 0.06);
  limb(FigureBone::RightArm, FigureBone::RightHand, 0.055);
  limb(FigureBone::Hips, FigureBone::LeftLeg, 0.075);
  limb(FigureBone::LeftLeg, FigureBone::LeftFoot, 0.07);
  limb(FigureBone::Hips, FigureBone::RightLeg, 0.075);
  limb(FigureBone::RightLeg, FigureBone::RightFoot, 0.07);

  // The head itself, as a short stub above the neck joint.
  FigureLimb head;
  head.from = jointAt(FigureBone::Head);
  head.to = head.from + Vec3{0.0, 0.13, 0.0};
  head.thickness = 0.11;
  out.push_back(head);
}

// --- Hand-authored rigs (stage 35) ---

void customFigureLimbs(const std::vector<CustomBone>& bones, const FigureMotion& motion, const Vec3& position,
                       f64 yaw, std::vector<FigureLimb>& out) {
  out.clear();
  if (bones.empty()) return;

  // How far this frame is through the walk cycle. Identical maths to the
  // built-in figure, so a hand-made rig moves in the same rhythm.
  f64 phase = 0.0;
  f64 reach = 0.0;
  if (!motion.downed && !motion.airborne && motion.speed >= 0.05) {
    reach = std::min(0.85, motion.speed * 0.16);
    phase = std::sin(motion.time * (2.2 + std::min(motion.speed, 8.0) * 0.55));
  }

  // Each bone's own rotation about its start point.
  const usize count = bones.size();
  std::vector<Mat4> local(count);
  for (usize i = 0; i < count; ++i) {
    f64 angle = phase * reach * bones[i].swing;
    if (motion.airborne) angle = -0.7 * std::abs(bones[i].swing);  // tuck
    if (motion.downed) angle = 0.0;
    local[i] = Mat4::translation(bones[i].from) * Mat4::rotationX(angle) *
               Mat4::translation(Vec3{-bones[i].from.x, -bones[i].from.y, -bones[i].from.z});
  }

  // Resolve each bone's parent chain. Bones may be listed in any order —
  // the user is dragging them around, not maintaining a sorted array — so
  // walk up by name each time and stop if the chain loops.
  std::vector<Mat4> world(count);
  for (usize i = 0; i < count; ++i) {
    Mat4 accumulated = local[i];
    std::string parent = bones[i].parent;
    for (usize guard = 0; guard < count && !parent.empty(); ++guard) {
      bool found = false;
      for (usize j = 0; j < count; ++j) {
        if (bones[j].name != parent) continue;
        accumulated = local[j] * accumulated;
        parent = bones[j].parent;
        found = true;
        break;
      }
      if (!found) break;  // dangling parent: draw it where it was authored
    }
    world[i] = accumulated;
  }

  // Turn to face the heading, then stand the character where it belongs.
  const f64 sinYaw = std::sin(yaw);
  const f64 cosYaw = std::cos(yaw);
  const auto place = [&](const Vec3& p) {
    return Vec3{position.x + p.x * cosYaw + p.z * sinYaw, position.y + p.y,
                position.z - p.x * sinYaw + p.z * cosYaw};
  };

  // A downed character folds forward onto the ground.
  const Mat4 fall = motion.downed ? Mat4::rotationX(-1.4) : Mat4{};

  out.reserve(count);
  for (usize i = 0; i < count; ++i) {
    FigureLimb limb;
    limb.from = place(fall * (world[i] * bones[i].from));
    limb.to = place(fall * (world[i] * bones[i].to));
    limb.thickness = bones[i].thickness;
    out.push_back(limb);
  }
}

}  // namespace kimia
