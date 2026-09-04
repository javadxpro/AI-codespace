#include <kimia/Log.h>
#include <iostream>
#include <utility>

namespace kimia {
Logger& Logger::instance() {
  static Logger logger;
  return logger;
}

void Logger::setSink(LogSink sink) {
  std::lock_guard<std::mutex> lock(mutex_);
  sink_ = std::move(sink);
}

void Logger::write(LogLevel level, const std::string& message) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (sink_) {
    sink_(level, message);
    return;
  }
  std::ostream& stream = level == LogLevel::error ? std::cerr : std::clog;
  stream << message << '\n';
}

void log(LogLevel level, const std::string& message) {
  Logger::instance().write(level, message);
}
}
