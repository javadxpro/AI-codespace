#pragma once

#include <kimia/GraphicsTypes.h>
#include <kimia/Mat4.h>
#include <kimia/Quat.h>
#include <kimia/Vec.h>

#include <string>
#include <vector>

namespace kimia {

// --- Skeletons and skinning (stage 25) ---
//
// A skeleton is a flat array of bones. Every bone names its parent by index
// and a parent ALWAYS comes before its children, so one forward pass builds
// every world matrix — no recursion, no sorting at draw time.
//
// Two matrices matter per bone:
//   * the bind pose (where the bone was when the mesh was attached), kept as
//     its inverse because that is the only form skinning ever needs;
//   * the current local pose, which animation writes.
//
// A vertex is bound to at most kMaxBoneInfluences bones with weights that
// sum to 1. That is the usual game budget: four bones is enough for a
// shoulder or a hip, and it keeps the skinning loop tight.

inline constexpr u32 kMaxBoneInfluences = 4U;

// A bone with no parent (a root).
inline constexpr i32 kNoParentBone = -1;

// A local position/rotation/scale, kept apart from Scene's Transform so the
// Graphics module does not depend on Scene.
struct Transform3D {
  Vec3 position{0.0, 0.0, 0.0};
  Quat rotation{};
  Vec3 scale{1.0, 1.0, 1.0};

  Mat4 toMat4() const {
    return Mat4::translation(position) * rotation.toMat4() * Mat4::scaling(scale);
  }
};

struct Bone {
  std::string name;
  i32 parent = kNoParentBone;  // index into Skeleton::bones, always < own index
  // Where this bone sits relative to its parent in the rest pose.
  Transform3D restPose;
  // Inverse of the bone's world matrix in the bind pose. Skinning needs the
  // inverse only, so it is stored ready to use.
  Mat4 inverseBindPose;
};

struct Skeleton {
  std::vector<Bone> bones;

  usize boneCount() const { return bones.size(); }
  bool isEmpty() const { return bones.empty(); }

  // Index of a bone by name, or -1. Linear: skeletons are small and this is
  // used when loading, not per frame.
  i32 findBone(const std::string& name) const;

  // True when every parent index is valid AND appears before its child, so
  // a single forward pass is enough to pose the whole skeleton.
  bool isValid() const;
};

// Per-vertex bone binding. `bones[i]` is an index into Skeleton::bones and
// `weights[i]` how much it pulls; unused slots have weight 0.
struct VertexSkin {
  u32 bones[kMaxBoneInfluences] = {0U, 0U, 0U, 0U};
  f64 weights[kMaxBoneInfluences] = {0.0, 0.0, 0.0, 0.0};

  f64 totalWeight() const;
  // Scales the weights so they sum to 1. A vertex with no influences at all
  // is left alone (skinning treats it as rigid).
  void normalize();
};

// A mesh plus the skeleton it is bound to. The mesh positions/normals are
// the BIND POSE: skinning writes a posed copy rather than editing them, so
// the same asset can be posed many times.
struct SkinnedMesh {
  MeshData bindMesh;
  Skeleton skeleton;
  std::vector<VertexSkin> skins;  // one per bindMesh vertex

  bool isValid() const;
};

// --- Animation ---

// One keyframe of a bone's local transform.
struct BoneKey {
  f64 time = 0.0;  // seconds from the start of the clip
  Transform3D pose;
};

// Every keyframe for a single bone.
struct BoneTrack {
  i32 bone = kNoParentBone;  // index into Skeleton::bones
  std::vector<BoneKey> keys;  // sorted by time, at least one
};

struct AnimationClip {
  std::string name;
  f64 duration = 0.0;  // seconds; 0 for a single-pose clip
  bool loop = true;
  std::vector<BoneTrack> tracks;

  bool isEmpty() const { return tracks.empty(); }
};

// Samples one track at `time`, interpolating between the surrounding keys
// (linear for position/scale, slerp for rotation). Before the first key or
// after the last it holds that key — clamping is the caller's job via the
// clip's loop flag.
Transform3D sampleTrack(const BoneTrack& track, f64 time);

// Wraps `time` into the clip: looping clips repeat, one-shot clips stop at
// the end. Returns 0 for a clip with no duration.
f64 clipTime(const AnimationClip& clip, f64 time);

// Poses `skeleton` at `time` and writes one local transform per bone into
// `out` (resized to the bone count). Bones with no track keep their rest
// pose, so a clip that only animates an arm leaves the legs standing.
void samplePose(const Skeleton& skeleton, const AnimationClip& clip, f64 time, std::vector<Transform3D>& out);

// Turns local poses into a world matrix per bone (one forward pass; a
// parent is always already done). `localPoses` must have one entry per bone.
void computeWorldMatrices(const Skeleton& skeleton, const std::vector<Transform3D>& localPoses,
                          std::vector<Mat4>& out);

// The matrices skinning actually multiplies by: worldMatrix * inverseBind
// for each bone. Pass these to skinMesh().
void computeSkinMatrices(const Skeleton& skeleton, const std::vector<Transform3D>& localPoses,
                         std::vector<Mat4>& out);

// Deforms `mesh.bindMesh` by `skinMatrices` into `out`. Topology (indices,
// uvs, names) is copied unchanged; only positions and normals move. Returns
// false when the inputs do not line up.
bool skinMesh(const SkinnedMesh& mesh, const std::vector<Mat4>& skinMatrices, MeshData& out);

// The whole chain in one call: sample the clip at `time` and write the
// deformed mesh. Convenient for a renderer that just wants "this model, at
// this moment".
bool poseMesh(const SkinnedMesh& mesh, const AnimationClip& clip, f64 time, MeshData& out);

// --- Posing a figure without a model (stage 33) ---
//
// The engine must never invent game CONTENT, but a character still has to
// be visible before anyone has supplied an FBX. These build a plain
// jointed figure — a rig, not a character — so the players on the pitch
// have arms and legs that move instead of being boxes that slide about.
// Bring your own model and it replaces this entirely.

// The bones a walking figure needs, in the order makeFigureRig() builds
// them. Parents always come before children.
enum class FigureBone : u32 {
  Hips = 0U, Chest, Head,
  LeftArm, LeftHand, RightArm, RightHand,
  LeftLeg, LeftFoot, RightLeg, RightFoot,
  Count,
};

// A rig roughly `height` metres tall, standing at the origin with its feet
// on the ground. Every bone's rest pose and inverse bind matrix is filled
// in, so it can be posed immediately.
Skeleton makeFigureRig(f64 height = 1.7);

// How a figure is moving, which is all the engine needs to choose a pose.
struct FigureMotion {
  f64 speed = 0.0;      // metres per second along the ground
  f64 time = 0.0;       // seconds, for the swing cycle
  bool airborne = false;
  bool downed = false;  // arena: knocked out
};

// Poses `rig` for that motion: legs and arms swing when walking, the
// figure tucks when airborne, and lies flat when downed. Writes one local
// transform per bone.
void poseFigure(const Skeleton& rig, const FigureMotion& motion, std::vector<Transform3D>& out);

// A drawable segment of a posed figure: where a limb is and how long.
struct FigureLimb {
  Vec3 from{0.0, 0.0, 0.0};
  Vec3 to{0.0, 0.0, 0.0};
  f64 thickness = 0.08;
};

// Turns a posed rig into the segments to draw, in world space, for a
// figure standing at `position` and facing `yaw` radians. This is what a
// renderer with no skinned mesh uses.
void figureLimbs(const Skeleton& rig, const std::vector<Transform3D>& pose, const Vec3& position, f64 yaw,
                 std::vector<FigureLimb>& out);

}  // namespace kimia
