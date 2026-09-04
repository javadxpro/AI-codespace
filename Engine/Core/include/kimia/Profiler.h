#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace kimia {
struct ProfileSample { std::string name; double milliseconds; };

class Profiler final {
public:
  static Profiler& instance();
  void addSample(const std::string& name, double milliseconds);
  std::vector<ProfileSample> samples() const;
  void clear();
private:
  std::vector<ProfileSample> samples_;
};

class ScopedProfile final {
public:
  explicit ScopedProfile(const char* name);
  ~ScopedProfile();
  ScopedProfile(const ScopedProfile&) = delete;
  ScopedProfile& operator=(const ScopedProfile&) = delete;
private:
  const char* name_;
  std::chrono::steady_clock::time_point start_;
};
}
