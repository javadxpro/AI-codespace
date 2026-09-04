#include <kimia/Time.h>
#include <stdexcept>

namespace kimia {
FixedTimeStep::FixedTimeStep(f64 stepSeconds, u32 maxSteps)
    : stepSeconds_(stepSeconds), maxSteps_(maxSteps) {
  if (stepSeconds <= 0.0 || maxSteps == 0U) throw std::invalid_argument("invalid fixed timestep");
}

f64 FixedTimeStep::interpolation() const { return accumulator_ / stepSeconds_; }
}
