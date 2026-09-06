#include <kimia/AssetPipeline.h>
#include <kimia/Skeleton.h>
#include <kimia_test.h>

#include <cmath>
#include <string>
#include <vector>

namespace {

using kimia::AnimationClip;
using kimia::Bone;
using kimia::BoneKey;
using kimia::BoneTrack;
using kimia::Mat4;
using kimia::MeshData;
using kimia::Quat;
using kimia::Skeleton;
using kimia::SkinnedMesh;
using kimia::Transform3D;
using kimia::Vec3;
using kimia::VertexSkin;
using kimia::f64;
using kimia::i32;
using kimia::u32;
using kimia::usize;

bool near(f64 a, f64 b, f64 tolerance = 1e-9) { return std::abs(a - b) <= tolerance; }

bool near3(const Vec3& a, const Vec3& b, f64 tolerance = 1e-9) {
  return near(a.x, b.x, tolerance) && near(a.y, b.y, tolerance) && near(a.z, b.z, tolerance);
}

constexpr f64 kPi = 3.14159265358979323846;

// A two bone chain: root at the origin, tip one unit up. This is the same
// shape as the shipped skinned_bar.fbx, so the numbers line up.
Skeleton twoBoneChain() {
  Skeleton skeleton;
  Bone root;
  root.name = "root";
  root.parent = kimia::kNoParentBone;
  root.inverseBindPose = Mat4{};  // identity: the root is at the origin
  skeleton.bones.push_back(root);

  Bone tip;
  tip.name = "tip";
  tip.parent = 0;
  tip.restPose.position = Vec3{0.0, 1.0, 0.0};
  // The tip's bind pose is one unit up, so its inverse moves the world down.
  tip.inverseBindPose = Mat4::translation(Vec3{0.0, -1.0, 0.0});
  skeleton.bones.push_back(tip);
  return skeleton;
}

}  // namespace

// --- Stage 25: skeletons ---

KIMIA_TEST(skeleton_validity_requires_parents_before_children) {
  Skeleton good = twoBoneChain();
  KIMIA_REQUIRE(good.isValid());
  KIMIA_REQUIRE(good.boneCount() == 2U);
  KIMIA_REQUIRE(good.findBone("root") == 0);
  KIMIA_REQUIRE(good.findBone("tip") == 1);
  KIMIA_REQUIRE(good.findBone("nope") == kimia::kNoParentBone);

  // A child listed before its parent breaks the single forward pass.
  Skeleton backwards;
  Bone child;
  child.name = "child";
  child.parent = 1;  // points at a bone that comes later
  backwards.bones.push_back(child);
  Bone parent;
  parent.name = "parent";
  backwards.bones.push_back(parent);
  KIMIA_REQUIRE(!backwards.isValid());

  // An out-of-range parent is invalid too.
  Skeleton dangling;
  Bone lost;
  lost.name = "lost";
  lost.parent = 7;
  dangling.bones.push_back(lost);
  KIMIA_REQUIRE(!dangling.isValid());
}

KIMIA_TEST(skeleton_world_matrices_chain_through_parents) {
  const Skeleton skeleton = twoBoneChain();
  std::vector<Transform3D> pose(2);
  pose[0].position = Vec3{5.0, 0.0, 0.0};  // move the root sideways
  pose[1].position = Vec3{0.0, 1.0, 0.0};  // tip stays one up from the root

  std::vector<Mat4> world;
  kimia::computeWorldMatrices(skeleton, pose, world);
  KIMIA_REQUIRE(world.size() == 2U);
  // The root lands where it was put.
  KIMIA_REQUIRE(near3(world[0] * Vec3{0.0, 0.0, 0.0}, Vec3{5.0, 0.0, 0.0}));
  // The tip inherits the root's move AND keeps its own offset: 5 across,
  // 1 up. That is the whole point of a hierarchy.
  KIMIA_REQUIRE(near3(world[1] * Vec3{0.0, 0.0, 0.0}, Vec3{5.0, 1.0, 0.0}));
}

KIMIA_TEST(skeleton_skin_matrix_is_identity_in_the_bind_pose) {
  // A skeleton posed exactly at its bind pose must not move a single
  // vertex: worldMatrix * inverseBind has to come out as the identity.
  const Skeleton skeleton = twoBoneChain();
  std::vector<Transform3D> bindPose(2);
  bindPose[0] = skeleton.bones[0].restPose;
  bindPose[1] = skeleton.bones[1].restPose;

  std::vector<Mat4> skinMatrices;
  kimia::computeSkinMatrices(skeleton, bindPose, skinMatrices);
  KIMIA_REQUIRE(skinMatrices.size() == 2U);
  const Vec3 probe{0.3, 1.7, -0.4};
  for (const Mat4& matrix : skinMatrices) {
    KIMIA_REQUIRE(near3(matrix * probe, probe, 1e-12));
  }
}

// --- Animation sampling ---

KIMIA_TEST(animation_track_interpolates_between_keys) {
  BoneTrack track;
  track.bone = 0;
  BoneKey first;
  first.time = 0.0;
  first.pose.position = Vec3{0.0, 0.0, 0.0};
  track.keys.push_back(first);
  BoneKey last;
  last.time = 2.0;
  last.pose.position = Vec3{10.0, 0.0, 0.0};
  track.keys.push_back(last);

  // Exactly on the keys.
  KIMIA_REQUIRE(near3(kimia::sampleTrack(track, 0.0).position, Vec3{0.0, 0.0, 0.0}));
  KIMIA_REQUIRE(near3(kimia::sampleTrack(track, 2.0).position, Vec3{10.0, 0.0, 0.0}));
  // Half way is exactly half way.
  KIMIA_REQUIRE(near3(kimia::sampleTrack(track, 1.0).position, Vec3{5.0, 0.0, 0.0}));
  KIMIA_REQUIRE(near3(kimia::sampleTrack(track, 0.5).position, Vec3{2.5, 0.0, 0.0}));
  // Outside the range it holds the end keys rather than flying off.
  KIMIA_REQUIRE(near3(kimia::sampleTrack(track, -5.0).position, Vec3{0.0, 0.0, 0.0}));
  KIMIA_REQUIRE(near3(kimia::sampleTrack(track, 99.0).position, Vec3{10.0, 0.0, 0.0}));
}

KIMIA_TEST(animation_rotation_uses_the_short_way_round) {
  BoneTrack track;
  track.bone = 0;
  BoneKey first;
  first.time = 0.0;
  first.pose.rotation = Quat::fromAxisAngle(Vec3{0.0, 0.0, 1.0}, 0.0);
  track.keys.push_back(first);
  BoneKey last;
  last.time = 1.0;
  last.pose.rotation = Quat::fromAxisAngle(Vec3{0.0, 0.0, 1.0}, kPi * 0.5);
  track.keys.push_back(last);

  // Half way through a 90 degree turn is 45 degrees: a point on +X ends up
  // on the 45 degree diagonal.
  const Transform3D middle = kimia::sampleTrack(track, 0.5);
  const Vec3 turned = middle.rotation.toMat4() * Vec3{1.0, 0.0, 0.0};
  const f64 root2 = std::sqrt(0.5);
  KIMIA_REQUIRE(near3(turned, Vec3{root2, root2, 0.0}, 1e-9));
  // And the quaternion stays a unit one, which is what keeps meshes from
  // shearing as they animate.
  const f64 length = std::sqrt(middle.rotation.x * middle.rotation.x + middle.rotation.y * middle.rotation.y +
                               middle.rotation.z * middle.rotation.z + middle.rotation.w * middle.rotation.w);
  KIMIA_REQUIRE(near(length, 1.0, 1e-12));
}

KIMIA_TEST(animation_clip_loops_or_stops_at_the_end) {
  AnimationClip clip;
  clip.duration = 2.0;
  clip.loop = true;
  KIMIA_REQUIRE(near(kimia::clipTime(clip, 0.5), 0.5));
  KIMIA_REQUIRE(near(kimia::clipTime(clip, 2.0), 0.0));   // wrapped
  KIMIA_REQUIRE(near(kimia::clipTime(clip, 2.5), 0.5));
  KIMIA_REQUIRE(near(kimia::clipTime(clip, 6.5), 0.5));   // many loops later
  KIMIA_REQUIRE(near(kimia::clipTime(clip, -0.5), 1.5));  // played backwards

  clip.loop = false;
  KIMIA_REQUIRE(near(kimia::clipTime(clip, 0.5), 0.5));
  KIMIA_REQUIRE(near(kimia::clipTime(clip, 9.0), 2.0));   // held at the end
  KIMIA_REQUIRE(near(kimia::clipTime(clip, -9.0), 0.0));

  // A clip with no duration is always at zero, never a divide by zero.
  AnimationClip still;
  KIMIA_REQUIRE(near(kimia::clipTime(still, 3.0), 0.0));
}

KIMIA_TEST(animation_untracked_bones_keep_their_rest_pose) {
  const Skeleton skeleton = twoBoneChain();
  AnimationClip clip;
  clip.duration = 1.0;
  BoneTrack track;
  track.bone = 1;  // only the tip is animated
  BoneKey key;
  key.time = 0.0;
  key.pose.position = Vec3{0.0, 3.0, 0.0};
  track.keys.push_back(key);
  clip.tracks.push_back(track);

  std::vector<Transform3D> pose;
  kimia::samplePose(skeleton, clip, 0.0, pose);
  KIMIA_REQUIRE(pose.size() == 2U);
  // The root was never mentioned, so it stands exactly where it rests.
  KIMIA_REQUIRE(near3(pose[0].position, skeleton.bones[0].restPose.position));
  // The tip follows its track.
  KIMIA_REQUIRE(near3(pose[1].position, Vec3{0.0, 3.0, 0.0}));
}

// --- Skinning ---

KIMIA_TEST(skin_weights_normalize_and_reject_bad_input) {
  VertexSkin skin;
  skin.bones[0] = 0U;
  skin.weights[0] = 3.0;
  skin.bones[1] = 1U;
  skin.weights[1] = 1.0;
  KIMIA_REQUIRE(near(skin.totalWeight(), 4.0));
  skin.normalize();
  KIMIA_REQUIRE(near(skin.weights[0], 0.75));
  KIMIA_REQUIRE(near(skin.weights[1], 0.25));
  KIMIA_REQUIRE(near(skin.totalWeight(), 1.0));

  // A vertex with no influences is left alone rather than dividing by zero.
  VertexSkin rigid;
  rigid.normalize();
  KIMIA_REQUIRE(rigid.totalWeight() == 0.0);

  // A mesh whose skin list does not match its vertices is invalid.
  SkinnedMesh mismatched;
  mismatched.skeleton = twoBoneChain();
  mismatched.bindMesh.positions.push_back(Vec3{0.0, 0.0, 0.0});
  mismatched.bindMesh.normals.push_back(Vec3{0.0, 1.0, 0.0});
  mismatched.bindMesh.indices = {0U, 0U, 0U};
  KIMIA_REQUIRE(!mismatched.isValid());
}

KIMIA_TEST(skin_moves_the_bound_half_and_leaves_the_rest_still) {
  // Two vertices: one owned by the root, one by the tip. Bending the tip
  // must move its vertex and leave the root's exactly where it was.
  SkinnedMesh mesh;
  mesh.skeleton = twoBoneChain();
  mesh.bindMesh.positions = {Vec3{0.0, 0.0, 0.0}, Vec3{0.0, 2.0, 0.0}};
  mesh.bindMesh.normals = {Vec3{0.0, 1.0, 0.0}, Vec3{0.0, 1.0, 0.0}};
  mesh.bindMesh.indices = {0U, 1U, 0U};
  VertexSkin bottom;
  bottom.bones[0] = 0U;
  bottom.weights[0] = 1.0;
  VertexSkin top;
  top.bones[0] = 1U;
  top.weights[0] = 1.0;
  mesh.skins = {bottom, top};
  KIMIA_REQUIRE(mesh.isValid());

  // Bend the tip 90 degrees about Z.
  std::vector<Transform3D> pose(2);
  pose[1] = mesh.skeleton.bones[1].restPose;
  pose[1].rotation = Quat::fromAxisAngle(Vec3{0.0, 0.0, 1.0}, kPi * 0.5);

  std::vector<Mat4> matrices;
  kimia::computeSkinMatrices(mesh.skeleton, pose, matrices);
  MeshData posed;
  KIMIA_REQUIRE(kimia::skinMesh(mesh, matrices, posed));

  // The root's vertex never moves.
  KIMIA_REQUIRE(near3(posed.positions[0], Vec3{0.0, 0.0, 0.0}, 1e-12));
  // The tip's vertex was one unit above the joint; rotating 90 degrees
  // about Z swings it out to -X at the joint's height.
  KIMIA_REQUIRE(near3(posed.positions[1], Vec3{-1.0, 1.0, 0.0}, 1e-9));
  // The normal turned with it.
  KIMIA_REQUIRE(near3(posed.normals[1], Vec3{-1.0, 0.0, 0.0}, 1e-9));
  KIMIA_REQUIRE(near3(posed.normals[0], Vec3{0.0, 1.0, 0.0}, 1e-12));
  // Topology is untouched.
  KIMIA_REQUIRE(posed.indices == mesh.bindMesh.indices);
}

KIMIA_TEST(skin_at_the_bind_pose_reproduces_the_mesh_exactly) {
  SkinnedMesh mesh;
  mesh.skeleton = twoBoneChain();
  mesh.bindMesh.positions = {Vec3{0.2, 0.0, -0.3}, Vec3{0.7, 2.0, 0.4}};
  mesh.bindMesh.normals = {Vec3{0.0, 1.0, 0.0}, Vec3{1.0, 0.0, 0.0}};
  mesh.bindMesh.indices = {0U, 1U, 0U};
  VertexSkin bottom;
  bottom.bones[0] = 0U;
  bottom.weights[0] = 1.0;
  VertexSkin top;
  top.bones[0] = 1U;
  top.weights[0] = 1.0;
  mesh.skins = {bottom, top};

  std::vector<Transform3D> bindPose = {mesh.skeleton.bones[0].restPose, mesh.skeleton.bones[1].restPose};
  std::vector<Mat4> matrices;
  kimia::computeSkinMatrices(mesh.skeleton, bindPose, matrices);
  MeshData posed;
  KIMIA_REQUIRE(kimia::skinMesh(mesh, matrices, posed));
  // Not "close": the bind pose must give the vertices straight back.
  for (usize i = 0; i < mesh.bindMesh.positions.size(); ++i) {
    KIMIA_REQUIRE(near3(posed.positions[i], mesh.bindMesh.positions[i], 1e-12));
  }
}

// --- The real FBX ---

KIMIA_TEST(fbx_skinned_bar_loads_its_skeleton_and_animation) {
  std::string error;
  auto asset = kimia::assets::loadFBXSkinned("Tests/assets/skinned_bar.fbx", error);
  if (!asset.has_value()) {
    std::printf("SKIP: Tests/assets/skinned_bar.fbx not next to the test runner\n");
    return;
  }
  KIMIA_REQUIRE(asset->hasSkeleton());
  KIMIA_REQUIRE(asset->hasAnimation());

  // Exactly the two bones the asset defines — the FBX scene root must NOT
  // become a nameless extra bone.
  const Skeleton& skeleton = asset->skinned.skeleton;
  KIMIA_REQUIRE(skeleton.boneCount() == 2U);
  KIMIA_REQUIRE(skeleton.bones[0].name == "BoneRoot");
  KIMIA_REQUIRE(skeleton.bones[1].name == "BoneTip");
  KIMIA_REQUIRE(skeleton.bones[0].parent == kimia::kNoParentBone);
  KIMIA_REQUIRE(skeleton.bones[1].parent == 0);
  KIMIA_REQUIRE(skeleton.isValid());
  // The tip bone sits one unit up the bar.
  KIMIA_REQUIRE(near3(skeleton.bones[1].restPose.position, Vec3{0.0, 1.0, 0.0}, 1e-6));

  // The bar is a box: 6 quads = 12 triangles = 36 emitted vertices.
  KIMIA_REQUIRE(asset->skinned.bindMesh.positions.size() == 36U);
  KIMIA_REQUIRE(asset->skinned.bindMesh.indices.size() == 36U);
  KIMIA_REQUIRE(asset->skinned.isValid());
  // Every vertex is weighted, and every weight sums to one.
  for (const VertexSkin& skin : asset->skinned.skins) {
    KIMIA_REQUIRE(near(skin.totalWeight(), 1.0, 1e-6));
  }

  const AnimationClip& clip = asset->clips[0];
  KIMIA_REQUIRE(clip.name == "Bend");
  KIMIA_REQUIRE(near(clip.duration, 1.0, 1e-6));
  // Only the tip actually moves, so only it gets a track.
  KIMIA_REQUIRE(clip.tracks.size() == 1U);
  KIMIA_REQUIRE(clip.tracks[0].bone == 1);
  KIMIA_REQUIRE(clip.tracks[0].keys.size() > 1U);
}

KIMIA_TEST(fbx_skinned_bar_really_bends_when_it_is_posed) {
  std::string error;
  auto asset = kimia::assets::loadFBXSkinned("Tests/assets/skinned_bar.fbx", error);
  if (!asset.has_value()) {
    std::printf("SKIP: Tests/assets/skinned_bar.fbx not next to the test runner\n");
    return;
  }
  MeshData rest;
  MeshData bent;
  KIMIA_REQUIRE(kimia::poseMesh(asset->skinned, asset->clips[0], 0.0, rest));
  KIMIA_REQUIRE(kimia::poseMesh(asset->skinned, asset->clips[0], 0.5, bent));

  // At the start of the clip nothing has moved yet.
  for (usize i = 0; i < rest.positions.size(); ++i) {
    KIMIA_REQUIRE(near3(rest.positions[i], asset->skinned.bindMesh.positions[i], 1e-9));
  }

  // Half way through the bend: the bottom of the bar (owned by the root
  // bone) is nailed down, and the top has swung a long way.
  f64 worstBottom = 0.0;
  f64 bestTop = 0.0;
  for (usize i = 0; i < rest.positions.size(); ++i) {
    const f64 moved = (bent.positions[i] - rest.positions[i]).length();
    if (asset->skinned.bindMesh.positions[i].y < 1.0) {
      if (moved > worstBottom) worstBottom = moved;
    } else if (moved > bestTop) {
      bestTop = moved;
    }
  }
  KIMIA_REQUIRE(worstBottom < 1e-9);  // the base does not budge
  KIMIA_REQUIRE(bestTop > 0.5);       // the tip really swings
}

// --- Stage 33: the figure rig ---

KIMIA_TEST(figure_rig_is_a_valid_skeleton_with_named_joints) {
  const Skeleton rig = kimia::makeFigureRig(1.7);
  KIMIA_REQUIRE(rig.boneCount() == static_cast<usize>(kimia::FigureBone::Count));
  KIMIA_REQUIRE(rig.isValid());  // parents always before children

  // The joints a walking figure needs, wired up the way a body is.
  KIMIA_REQUIRE(rig.bones[0].name == "Hips");
  KIMIA_REQUIRE(rig.bones[0].parent == kimia::kNoParentBone);
  KIMIA_REQUIRE(rig.findBone("Head") >= 0);
  KIMIA_REQUIRE(rig.findBone("LeftFoot") >= 0);
  KIMIA_REQUIRE(rig.findBone("RightHand") >= 0);
  // A hand hangs off an arm, a foot off a leg — not straight off the hips.
  // findBone returns a SIGNED index (-1 = missing), so it is checked and
  // converted deliberately. Indexing a vector with it directly is what
  // broke the build on Clang: GCC's -Wconversion does not imply
  // -Wsign-conversion in C++, but Clang's does.
  const auto boneAt = [&rig](const char* name) -> const kimia::Bone& {
    const i32 index = rig.findBone(name);
    KIMIA_REQUIRE(index >= 0);
    return rig.bones[static_cast<usize>(index)];
  };
  KIMIA_REQUIRE(boneAt("LeftHand").parent == rig.findBone("LeftArm"));
  KIMIA_REQUIRE(boneAt("LeftFoot").parent == rig.findBone("LeftLeg"));

  // Every limb bone must have LENGTH. A zero-length bone cannot swing:
  // the first build gave the legs a sideways offset only, and they stayed
  // rigid however fast the figure ran.
  for (const char* name : {"LeftArm", "LeftHand", "LeftLeg", "LeftFoot"}) {
    KIMIA_REQUIRE(std::abs(boneAt(name).restPose.position.y) > 0.05);
  }
}

KIMIA_TEST(figure_stands_on_the_ground_at_its_own_height) {
  const Skeleton rig = kimia::makeFigureRig(1.7);
  std::vector<Transform3D> pose;
  std::vector<kimia::FigureLimb> limbs;
  kimia::poseFigure(rig, kimia::FigureMotion{}, pose);
  kimia::figureLimbs(rig, pose, Vec3{0.0, 0.0, 0.0}, 0.0, limbs);
  KIMIA_REQUIRE(limbs.size() >= 10U);

  f64 lowest = 1e9;
  f64 highest = -1e9;
  for (const kimia::FigureLimb& limb : limbs) {
    lowest = std::min(lowest, std::min(limb.from.y, limb.to.y));
    highest = std::max(highest, std::max(limb.from.y, limb.to.y));
  }
  // Feet on the floor, head near the stated height. The first attempt left
  // the figure floating half a metre up because the thighs had no length.
  KIMIA_REQUIRE(lowest >= -0.05);
  KIMIA_REQUIRE(lowest < 0.15);
  KIMIA_REQUIRE(highest > 1.5);
  KIMIA_REQUIRE(highest < 1.95);

  // A shorter figure really is shorter.
  const Skeleton child = kimia::makeFigureRig(1.0);
  kimia::poseFigure(child, kimia::FigureMotion{}, pose);
  kimia::figureLimbs(child, pose, Vec3{0.0, 0.0, 0.0}, 0.0, limbs);
  f64 top = -1e9;
  for (const kimia::FigureLimb& limb : limbs) top = std::max(top, std::max(limb.from.y, limb.to.y));
  KIMIA_REQUIRE(top < 1.2);
}

KIMIA_TEST(figure_walks_with_opposite_arm_to_opposite_leg) {
  const Skeleton rig = kimia::makeFigureRig(1.7);
  std::vector<Transform3D> pose;
  std::vector<kimia::FigureLimb> limbs;

  // Limb 7 is LeftLeg->LeftFoot, 9 is RightLeg->RightFoot, 3 is the left
  // forearm. (Reading the wrong index is how I first convinced myself the
  // legs were not moving when they were.)
  kimia::FigureMotion walking;
  walking.speed = 4.0;
  walking.time = 0.4;
  kimia::poseFigure(rig, walking, pose);
  kimia::figureLimbs(rig, pose, Vec3{0.0, 0.0, 0.0}, 0.0, limbs);
  const f64 leftFoot = limbs[7].to.z;
  const f64 rightFoot = limbs[9].to.z;
  const f64 leftHand = limbs[3].to.z;

  // The feet are on opposite sides of the body: that is a stride.
  KIMIA_REQUIRE(std::abs(leftFoot) > 0.05);
  KIMIA_REQUIRE(leftFoot * rightFoot < 0.0);
  // And the arm opposite the leading leg swings the other way, which is
  // what makes a walk read as a walk rather than a shuffle.
  KIMIA_REQUIRE(leftFoot * leftHand < 0.0);
}

KIMIA_TEST(figure_stride_grows_with_speed_and_stops_when_still) {
  const Skeleton rig = kimia::makeFigureRig(1.7);
  std::vector<Transform3D> pose;
  std::vector<kimia::FigureLimb> limbs;

  const auto strideAt = [&](f64 speed) {
    f64 lowest = 1e9;
    f64 highest = -1e9;
    for (i32 step = 0; step < 80; ++step) {
      kimia::FigureMotion motion;
      motion.speed = speed;
      motion.time = static_cast<f64>(step) * 0.02;
      kimia::poseFigure(rig, motion, pose);
      kimia::figureLimbs(rig, pose, Vec3{0.0, 0.0, 0.0}, 0.0, limbs);
      lowest = std::min(lowest, limbs[7].to.z);
      highest = std::max(highest, limbs[7].to.z);
    }
    return highest - lowest;
  };

  const f64 amble = strideAt(1.0);
  const f64 run = strideAt(4.0);
  const f64 sprint = strideAt(8.0);
  KIMIA_REQUIRE(amble > 0.02);
  KIMIA_REQUIRE(run > amble);
  KIMIA_REQUIRE(sprint > run);
  // A sprint is a stride, not a stretch: it stays a plausible step.
  KIMIA_REQUIRE(sprint < 1.5);

  // Standing still is perfectly still, at any moment in time. A figure
  // that jiggles on the spot looks broken.
  kimia::FigureMotion early;
  early.time = 1.0;
  kimia::poseFigure(rig, early, pose);
  kimia::figureLimbs(rig, pose, Vec3{0.0, 0.0, 0.0}, 0.0, limbs);
  const f64 first = limbs[7].to.z;
  kimia::FigureMotion later;
  later.time = 9.9;
  kimia::poseFigure(rig, later, pose);
  kimia::figureLimbs(rig, pose, Vec3{0.0, 0.0, 0.0}, 0.0, limbs);
  KIMIA_REQUIRE(first == limbs[7].to.z);
}

KIMIA_TEST(figure_faces_where_it_is_told_and_stands_where_it_is_put) {
  const Skeleton rig = kimia::makeFigureRig(1.7);
  std::vector<Transform3D> pose;
  std::vector<kimia::FigureLimb> limbs;
  kimia::poseFigure(rig, kimia::FigureMotion{}, pose);

  // Facing 0, the left shoulder is off to +X.
  kimia::figureLimbs(rig, pose, Vec3{0.0, 0.0, 0.0}, 0.0, limbs);
  const f64 shoulderX = limbs[2].to.x;
  KIMIA_REQUIRE(shoulderX > 0.05);

  // Turned a quarter turn, that same shoulder has swung round to Z.
  kimia::figureLimbs(rig, pose, Vec3{0.0, 0.0, 0.0}, 1.5707963267948966, limbs);
  KIMIA_REQUIRE(std::abs(limbs[2].to.x) < 0.02);
  KIMIA_REQUIRE(std::abs(limbs[2].to.z) > 0.05);

  // Moved across the pitch, the whole figure moves with it.
  kimia::figureLimbs(rig, pose, Vec3{5.0, 0.0, -3.0}, 0.0, limbs);
  KIMIA_REQUIRE(near(limbs[2].to.x, 5.0 + shoulderX, 1e-9));
  KIMIA_REQUIRE(near3(limbs[0].from, Vec3{5.0, limbs[0].from.y, -3.0}, 1e-9));
}

KIMIA_TEST(figure_tucks_in_the_air_and_lies_flat_when_downed) {
  const Skeleton rig = kimia::makeFigureRig(1.7);
  std::vector<Transform3D> pose;
  std::vector<kimia::FigureLimb> limbs;

  const auto topOf = [&](const kimia::FigureMotion& motion) {
    kimia::poseFigure(rig, motion, pose);
    kimia::figureLimbs(rig, pose, Vec3{0.0, 0.0, 0.0}, 0.0, limbs);
    f64 top = -1e9;
    for (const kimia::FigureLimb& limb : limbs) top = std::max(top, std::max(limb.from.y, limb.to.y));
    return top;
  };

  const f64 standing = topOf(kimia::FigureMotion{});

  // Knocked out: down on the floor, clearly not still standing.
  kimia::FigureMotion downed;
  downed.downed = true;
  const f64 flat = topOf(downed);
  KIMIA_REQUIRE(flat < standing * 0.6);

  // Airborne: the legs tuck, so the feet come UP relative to standing.
  // Measured against the standing pose rather than an invented number —
  // what matters is that the jump differs from a stand, not that the feet
  // clear some particular height.
  const auto lowestOf = [&](const kimia::FigureMotion& motion) {
    kimia::poseFigure(rig, motion, pose);
    kimia::figureLimbs(rig, pose, Vec3{0.0, 0.0, 0.0}, 0.0, limbs);
    f64 lowest = 1e9;
    for (const kimia::FigureLimb& limb : limbs) lowest = std::min(lowest, std::min(limb.from.y, limb.to.y));
    return lowest;
  };
  kimia::FigureMotion jumping;
  jumping.airborne = true;
  KIMIA_REQUIRE(lowestOf(jumping) > lowestOf(kimia::FigureMotion{}) + 0.03);
}
