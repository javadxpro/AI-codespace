#pragma once

#include <kimia/Time.h>
#include <kimia/Types.h>

#include <functional>

namespace kimia {

// Platform-independent simulation clock used by the PC runtime and the
// future native editor. Rendering is allowed to run at a different rate; game
// state is advanced only in fixed-size steps.
struct RuntimeLoopOptions {
  f64 simulationHz = 60.0;
  u32 maxStepsPerFrame = 8U;
  f64 maxFrameSeconds = 0.25;
};

struct RuntimeLoopStats {
  u64 renderedFrames = 0U;
  u64 simulationSteps = 0U;
  u64 clampedFrames = 0U;
  u64 droppedSteps = 0U;
};

class RuntimeLoop final {
public:
  explicit RuntimeLoop(const RuntimeLoopOptions& options = RuntimeLoopOptions{});

  // Advances simulation by hostSeconds. The callback receives exactly the
  // configured fixed step for every simulation tick. Negative and NaN input
  // are ignored; very large frame gaps are clamped before entering physics.
  u32 tick(f64 hostSeconds, const std::function<void(f64)>& update);

  void beginRenderFrame() { ++stats_.renderedFrames; }
  void pause(bool value) { paused_ = value; }
  bool paused() const { return paused_; }
  void reset();

  f64 fixedStep() const { return fixedStep_; }
  f64 interpolation() const { return clock_.interpolation(); }
  const RuntimeLoopStats& stats() const { return stats_; }

private:
  f64 fixedStep_;
  f64 maxFrameSeconds_;
  FixedTimeStep clock_;
  bool paused_ = false;
  RuntimeLoopStats stats_;
};

}  // namespace kimia
