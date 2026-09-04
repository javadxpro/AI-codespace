#include <kimia/Log.h>
#include <kimia/Profiler.h>
#include <kimia/Time.h>
#include <kimia_test.h>
#include <cmath>
#include <stdexcept>

KIMIA_TEST(types_are_stable) {
  KIMIA_REQUIRE(sizeof(kimia::i32) == 4U);
  KIMIA_REQUIRE(sizeof(kimia::f32) == 4U);
}

KIMIA_TEST(fixed_step_caps_work) {
  kimia::FixedTimeStep clock(0.1, 5U);
  kimia::u32 steps = clock.advance(2.0, [](kimia::f64) {});
  KIMIA_REQUIRE(steps == 5U);
  KIMIA_REQUIRE(std::abs(clock.interpolation()) < 0.000001);
}

KIMIA_TEST(fixed_step_accumulates_fraction) {
  kimia::FixedTimeStep clock(0.1);
  kimia::u32 total = 0U;
  total += clock.advance(0.06, [](kimia::f64) {});
  total += clock.advance(0.06, [](kimia::f64) {});
  KIMIA_REQUIRE(total == 1U);
  KIMIA_REQUIRE(clock.interpolation() > 0.1 && clock.interpolation() < 0.3);
}

KIMIA_TEST(logger_sink_receives_message) {
  bool received = false;
  kimia::Logger::instance().setSink([&received](kimia::LogLevel level, const std::string& message) {
    received = level == kimia::LogLevel::info && message == "hello";
  });
  kimia::log(kimia::LogLevel::info, "hello");
  kimia::Logger::instance().setSink({});
  KIMIA_REQUIRE(received);
}

KIMIA_TEST(profiler_records_scope) {
  kimia::Profiler::instance().clear();
  { kimia::ScopedProfile sample("core"); }
  const auto samples = kimia::Profiler::instance().samples();
  KIMIA_REQUIRE(samples.size() == 1U);
  KIMIA_REQUIRE(samples.front().name == "core");
  KIMIA_REQUIRE(samples.front().milliseconds >= 0.0);
}
