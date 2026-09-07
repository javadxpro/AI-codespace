#include <kimia/RuntimeLoop.h>

#include <cmath>
#include <stdexcept>

namespace kimia {

RuntimeLoop::RuntimeLoop(const RuntimeLoopOptions& options)
    : fixedStep_(1.0 / options.simulationHz),
      maxFrameSeconds_(options.maxFrameSeconds),
      clock_(fixedStep_, options.maxStepsPerFrame) {
  if (!(options.simulationHz > 0.0) || !std::isfinite(options.simulationHz) || options.maxStepsPerFrame == 0U ||
      !(options.maxFrameSeconds > 0.0) || !std::isfinite(options.maxFrameSeconds)) {
    throw std::invalid_argument("invalid runtime loop options");
  }
}

u32 RuntimeLoop::tick(f64 hostSeconds, const std::function<void(f64)>& update) {
  if (paused_ || update == nullptr || !(hostSeconds > 0.0) || !std::isfinite(hostSeconds)) return 0U;
  if (hostSeconds > maxFrameSeconds_) {
    hostSeconds = maxFrameSeconds_;
    ++stats_.clampedFrames;
  }
  const u32 steps = clock_.advance(hostSeconds, [this, &update](f64 step) {
    update(step);
    ++stats_.simulationSteps;
  });
  // FixedTimeStep deliberately discards excess accumulator time at its step
  // cap. Count it as a dropped frame budget, not as a physics error.
  if (steps == 0U && hostSeconds >= fixedStep_) ++stats_.droppedSteps;
  return steps;
}

void RuntimeLoop::reset() {
  clock_.reset();
  stats_ = RuntimeLoopStats{};
}

}  // namespace kimia
