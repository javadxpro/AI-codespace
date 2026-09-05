#include <kimia/Log.h>
#include <kimia/Profiler.h>
#include <kimia/Time.h>
#include <kimia/Version.h>
#include <kimia_test.h>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>

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

KIMIA_TEST(engine_version_is_semantic_and_written_down) {
  // Version.h and CMakeLists.txt carry the same literal.
  KIMIA_REQUIRE(std::string(kimia::kEngineVersion) == KIMIA_CMAKE_VERSION);
  // MAJOR.MINOR.PATCH, and the numeric fields spell exactly the same string.
  char rebuilt[32];
  std::snprintf(rebuilt, sizeof(rebuilt), "%u.%u.%u", kimia::kEngineVersionMajor, kimia::kEngineVersionMinor,
                kimia::kEngineVersionPatch);
  KIMIA_REQUIRE(std::string(kimia::kEngineVersion) == rebuilt);
  KIMIA_REQUIRE(std::string(kimia::kEngineVersionString) == std::string("KIMIA ") + kimia::kEngineVersion);
  // Every release is written down: the NEWEST entry of CHANGELOG.md (the first
  // «## نسخهٔ» heading) is this version — a bump without its notebook entry fails.
  std::FILE* file = std::fopen(KIMIA_SOURCE_DIR "/CHANGELOG.md", "rb");
  KIMIA_REQUIRE(file != nullptr);
  std::string text;
  char chunk[4096];
  for (;;) {
    const std::size_t got = std::fread(chunk, 1U, sizeof(chunk), file);
    text.append(chunk, got);
    if (got < sizeof(chunk)) break;
  }
  std::fclose(file);
  const std::string heading = "## نسخهٔ ";
  const std::size_t first = text.find(heading);
  KIMIA_REQUIRE(first != std::string::npos);
  const std::string version = kimia::kEngineVersion;
  KIMIA_REQUIRE(text.compare(first + heading.size(), version.size(), version) == 0);
  const char after = text[first + heading.size() + version.size()];
  KIMIA_REQUIRE(after == ' ' || after == '\n');  // "0.1.0 —", not "0.1.01"
}
