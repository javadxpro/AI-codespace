#include <kimia/Animator.h>

#include <algorithm>
#include <cctype>
#include <cmath>

namespace kimia {

namespace {

constexpr f64 kEpsilon = 1e-12;

std::string lower(std::string value) {
  for (char& character : value) {
    character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
  }
  return value;
}

}  // namespace

bool Animator::bindAction(const std::string& action, const AnimatorClip& clip, bool loop, f64 speed,
                          f64 blendSeconds) {
  if (action.empty() || !clip.valid() || target_ == nullptr || !retargetable(*clip.skeleton, *target_)) return false;
  AnimatorBinding binding;
  binding.action = action;
  binding.clip = clip;
  binding.loop = loop;
  binding.speed = speed > 0.01 ? speed : 0.01;
  binding.blendSeconds = blendSeconds > 0.0 ? blendSeconds : 0.0;
  bindings_[action] = std::move(binding);
  return true;
}

bool Animator::unbindAction(const std::string& action) {
  const auto found = bindings_.find(action);
  if (found == bindings_.end()) return false;
  bindings_.erase(found);
  if (currentAction_ == action) stop();
  return true;
}

bool Animator::sameClip(const AnimatorClip& a, const AnimatorClip& b) {
  return a.skeleton == b.skeleton && a.clip == b.clip && a.sourceAsset == b.sourceAsset;
}

bool Animator::playAction(const std::string& action) {
  const auto found = bindings_.find(action);
  if (found == bindings_.end()) return false;
  const AnimatorBinding& binding = found->second;
  if (!binding.clip.valid() || target_ == nullptr || !retargetable(*binding.clip.skeleton, *target_)) return false;

  if (current_.valid() && sameClip(current_.clip, binding.clip) && currentAction_ == action) {
    current_.time = 0.0;
    current_.loop = binding.loop;
    current_.speed = binding.speed;
    current_.blendSeconds = binding.blendSeconds;
    previous_ = State{};
    blendTime_ = 0.0;
    return true;
  }

  if (current_.valid()) {
    previous_ = current_;
    blendTime_ = 0.0;
  } else {
    previous_ = State{};
    blendTime_ = 0.0;
  }
  current_.clip = binding.clip;
  current_.action = action;
  current_.loop = binding.loop;
  current_.speed = binding.speed;
  current_.time = 0.0;
  current_.blendSeconds = binding.blendSeconds;
  currentAction_ = action;
  return true;
}

bool Animator::playClip(const AnimatorClip& clip, bool loop, f64 speed, f64 blendSeconds) {
  if (!bindAction("__direct__", clip, loop, speed, blendSeconds)) return false;
  return playAction("__direct__");
}

void Animator::update(f64 seconds) {
  if (!(seconds > 0.0) || !std::isfinite(seconds)) return;

  if (previous_.valid()) {
    previous_.time += seconds * previous_.speed;
    if (previous_.loop && previous_.clip.clip->duration > kEpsilon) {
      previous_.time = clipTime(*previous_.clip.clip, previous_.time);
    }
    blendTime_ += seconds;
    if (blendTime_ >= std::max(kEpsilon, current_.blendSeconds)) {
      previous_ = State{};
      blendTime_ = 0.0;
    }
  }

  if (!current_.valid()) return;
  current_.time += seconds * current_.speed;
  const f64 duration = current_.clip.clip->duration;
  if (current_.loop) {
    if (duration > kEpsilon) current_.time = clipTime(*current_.clip.clip, current_.time);
    return;
  }
  if (duration > kEpsilon && current_.time >= duration) {
    // One-shots retire at their exact end. The last pose was available for
    // the frame that reached the end, then the next state can take over.
    current_ = State{};
    currentAction_.clear();
    previous_ = State{};
    blendTime_ = 0.0;
  }
}

void Animator::stop() {
  current_ = State{};
  previous_ = State{};
  currentAction_.clear();
  blendTime_ = 0.0;
}

const std::string& Animator::currentClip() const {
  static const std::string empty;
  return current_.valid() ? current_.clip.clip->name : empty;
}

const std::string& Animator::currentSourceAsset() const {
  static const std::string empty;
  return current_.valid() ? current_.clip.sourceAsset : empty;
}

f64 Animator::blendAmount() const {
  if (!previous_.valid() || current_.blendSeconds <= 0.0) return 1.0;
  return std::min(1.0, std::max(0.0, blendTime_ / current_.blendSeconds));
}

std::string Animator::normalizedBoneName(const std::string& name) {
  std::string value = lower(name);
  const std::string prefixes[] = {"mixamorig:", "mixamorig_", "armature|", "armature:", "rig|", "skeleton|"};
  for (const std::string& prefix : prefixes) {
    if (value.rfind(prefix, 0U) == 0U) {
      value.erase(0U, prefix.size());
      break;
    }
  }
  return value;
}

bool Animator::retargetable(const Skeleton& source, const Skeleton& target) {
  if (source.isEmpty() || target.isEmpty()) return false;
  for (const Bone& sourceBone : source.bones) {
    const std::string sourceName = normalizedBoneName(sourceBone.name);
    for (const Bone& targetBone : target.bones) {
      if (sourceName == normalizedBoneName(targetBone.name)) return true;
    }
  }
  return false;
}

bool Animator::sampleState(const State& state, std::vector<Transform3D>& out) const {
  if (!state.valid() || target_ == nullptr) return false;
  std::vector<Transform3D> sourcePose;
  kimia::samplePose(*state.clip.skeleton, *state.clip.clip, state.time, sourcePose);
  out.resize(target_->bones.size());
  for (usize i = 0; i < target_->bones.size(); ++i) out[i] = target_->bones[i].restPose;

  bool matched = false;
  for (usize sourceIndex = 0; sourceIndex < state.clip.skeleton->bones.size(); ++sourceIndex) {
    const std::string sourceName = normalizedBoneName(state.clip.skeleton->bones[sourceIndex].name);
    i32 targetIndex = target_->findBone(state.clip.skeleton->bones[sourceIndex].name);
    if (targetIndex < 0) {
      for (usize candidate = 0; candidate < target_->bones.size(); ++candidate) {
        if (sourceName == normalizedBoneName(target_->bones[candidate].name)) {
          targetIndex = static_cast<i32>(candidate);
          break;
        }
      }
    }
    if (targetIndex < 0 || static_cast<usize>(targetIndex) >= out.size() || sourceIndex >= sourcePose.size()) continue;
    out[static_cast<usize>(targetIndex)] = sourcePose[sourceIndex];
    matched = true;
  }
  return matched;
}

bool Animator::samplePose(std::vector<Transform3D>& out) const {
  if (!current_.valid() || target_ == nullptr) return false;
  std::vector<Transform3D> targetPose;
  if (!sampleState(current_, targetPose)) return false;
  if (previous_.valid() && blendAmount() < 1.0) {
    std::vector<Transform3D> previousPose;
    if (sampleState(previous_, previousPose)) {
      const usize count = std::min(targetPose.size(), previousPose.size());
      const f64 amount = blendAmount();
      for (usize i = 0; i < count; ++i) {
        targetPose[i] = blendTransforms(previousPose[i], targetPose[i], amount);
      }
    }
  }
  out = std::move(targetPose);
  return true;
}

}  // namespace kimia
