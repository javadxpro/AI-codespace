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

}  // namespace kimia
