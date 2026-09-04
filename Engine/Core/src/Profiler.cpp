#include <kimia/Profiler.h>

namespace kimia {
Profiler& Profiler::instance() {
  static Profiler profiler;
  return profiler;
}

void Profiler::addSample(const std::string& name, double milliseconds) {
  samples_.push_back({name, milliseconds});
}

std::vector<ProfileSample> Profiler::samples() const { return samples_; }
void Profiler::clear() { samples_.clear(); }

ScopedProfile::ScopedProfile(const char* name)
    : name_(name), start_(std::chrono::steady_clock::now()) {}

ScopedProfile::~ScopedProfile() {
  const auto elapsed = std::chrono::steady_clock::now() - start_;
  const double milliseconds = std::chrono::duration<double, std::milli>(elapsed).count();
  Profiler::instance().addSample(name_, milliseconds);
}
}
