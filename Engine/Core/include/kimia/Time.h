#pragma once

#include <kimia/Types.h>

namespace kimia {
class FixedTimeStep final {
public:
  explicit FixedTimeStep(f64 stepSeconds, u32 maxSteps = 5U);
  template <typename StepFn>
  u32 advance(f64 frameSeconds, StepFn&& step) {
    accumulator_ += frameSeconds;
    u32 count = 0U;
    while (accumulator_ >= stepSeconds_ && count < maxSteps_) {
      step(stepSeconds_);
      accumulator_ -= stepSeconds_;
      ++count;
    }
    if (count == maxSteps_ && accumulator_ >= stepSeconds_) accumulator_ = 0.0;
    return count;
  }
  f64 interpolation() const;
private:
  f64 stepSeconds_;
  f64 accumulator_ = 0.0;
  u32 maxSteps_;
};
}
