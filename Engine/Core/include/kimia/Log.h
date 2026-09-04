#pragma once

#include <kimia/Types.h>
#include <functional>
#include <mutex>
#include <string>

namespace kimia {
enum class LogLevel { debug, info, warning, error };
using LogSink = std::function<void(LogLevel, const std::string&)>;

class Logger final {
public:
  static Logger& instance();
  void setSink(LogSink sink);
  void write(LogLevel level, const std::string& message);
private:
  Logger() = default;
  std::mutex mutex_;
  LogSink sink_;
};

void log(LogLevel level, const std::string& message);
}
