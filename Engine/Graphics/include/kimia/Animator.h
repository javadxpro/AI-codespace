#pragma once

#include <kimia/Skeleton.h>

#include <map>
#include <string>
#include <vector>

namespace kimia {

// A clip can come from a different FBX than the character being animated.
// The animator retargets local tracks by bone name, which is what makes a
// button able to use `kick.fbx` on a player imported from `idle.fbx`.
struct AnimatorClip {
  const Skeleton* skeleton = nullptr;
  const AnimationClip* clip = nullptr;
  std::string sourceAsset;

  bool valid() const { return skeleton != nullptr && clip != nullptr && !clip->isEmpty(); }
};

struct AnimatorBinding {
  std::string action;
  AnimatorClip clip;
  bool loop = true;
  f64 speed = 1.0;
  f64 blendSeconds = 0.15;
};

// Small data-driven animation state machine. It owns no skeleton or FBX
// memory; WorldEditor keeps the imported assets cached and gives the animator
// stable pointers into those assets. Bindings are actions such as "jump",
// "kick" or "button_a", and each action may point at a clip in any
// compatible FBX file.
class Animator final {
public:
  explicit Animator(const Skeleton* target = nullptr) : target_(target) {}

  void setTarget(const Skeleton* target) { target_ = target; }
  const Skeleton* target() const { return target_; }

  bool bindAction(const std::string& action, const AnimatorClip& clip, bool loop = true, f64 speed = 1.0,
                  f64 blendSeconds = 0.15);
  bool unbindAction(const std::string& action);
  bool hasAction(const std::string& action) const { return bindings_.find(action) != bindings_.end(); }

  // Starts an already-bound action. Switching actions keeps the previous
  // state alive for the requested blend interval and blends local transforms
  // with quaternion slerp.
  bool playAction(const std::string& action);
  // Convenience for a one-off clip. It still goes through the same binding
  // and transition path as a named action.
  bool playClip(const AnimatorClip& clip, bool loop = false, f64 speed = 1.0, f64 blendSeconds = 0.15);

  void update(f64 seconds);
  void stop();

  bool playing() const { return current_.valid(); }
  const std::string& currentAction() const { return currentAction_; }
  const std::string& currentClip() const;
  const std::string& currentSourceAsset() const;
  f64 currentTime() const { return current_.time; }
  f64 blendAmount() const;

  // Samples the current (possibly cross-faded) pose in the TARGET skeleton's
  // bone order. Returns false when there is no playable state or no matching
  // bones between the source clip and target rig.
  bool samplePose(std::vector<Transform3D>& out) const;

  // A source and target rig are retargetable when at least one named bone is
  // shared. Names are compared both literally and after removing common FBX
  // prefixes such as `mixamorig:` and `Armature|`.
  static bool retargetable(const Skeleton& source, const Skeleton& target);

private:
  struct State {
    AnimatorClip clip;
    std::string action;
    bool loop = true;
    f64 speed = 1.0;
    f64 time = 0.0;
    f64 blendSeconds = 0.15;

    bool valid() const { return clip.valid(); }
  };

  static bool sameClip(const AnimatorClip& a, const AnimatorClip& b);
  static std::string normalizedBoneName(const std::string& name);
  bool sampleState(const State& state, std::vector<Transform3D>& out) const;

  const Skeleton* target_ = nullptr;
  std::map<std::string, AnimatorBinding> bindings_;
  State current_;
  State previous_;
  std::string currentAction_;
  f64 blendTime_ = 0.0;
};

}  // namespace kimia
