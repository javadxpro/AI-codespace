#include <kimia/RuntimeLoop.h>
#include <kimia_test.h>

KIMIA_TEST(runtime_loop_ticks_at_a_fixed_rate) {
  kimia::RuntimeLoopOptions options;
  options.simulationHz = 60.0;
  options.maxStepsPerFrame = 8U;
  kimia::RuntimeLoop loop(options);
  kimia::u32 steps = 0U;
  KIMIA_REQUIRE(loop.tick(1.0 / 30.0, [&steps](kimia::f64 dt) {
                  KIMIA_REQUIRE(dt == 1.0 / 60.0);
                  ++steps;
                }) == 2U);
  KIMIA_REQUIRE(steps == 2U);
  KIMIA_REQUIRE(loop.stats().simulationSteps == 2U);
}

KIMIA_TEST(runtime_loop_clamps_a_stalled_frame) {
  kimia::RuntimeLoopOptions options;
  options.simulationHz = 60.0;
  options.maxStepsPerFrame = 4U;
  options.maxFrameSeconds = 0.1;
  kimia::RuntimeLoop loop(options);
  KIMIA_REQUIRE(loop.tick(2.0, [](kimia::f64) {}) == 4U);
  KIMIA_REQUIRE(loop.stats().clampedFrames == 1U);
}

KIMIA_TEST(runtime_loop_pause_blocks_simulation) {
  kimia::RuntimeLoop loop;
  kimia::u32 steps = 0U;
  loop.pause(true);
  KIMIA_REQUIRE(loop.tick(1.0, [&steps](kimia::f64) { ++steps; }) == 0U);
  KIMIA_REQUIRE(steps == 0U);
  loop.pause(false);
  KIMIA_REQUIRE(loop.tick(1.0 / 60.0, [&steps](kimia::f64) { ++steps; }) == 1U);
  KIMIA_REQUIRE(steps == 1U);
}

KIMIA_TEST(runtime_loop_reset_clears_clock_and_stats) {
  kimia::RuntimeLoop loop;
  KIMIA_REQUIRE(loop.tick(1.0 / 60.0, [](kimia::f64) {}) == 1U);
  loop.beginRenderFrame();
  loop.reset();
  KIMIA_REQUIRE(loop.stats().simulationSteps == 0U);
  KIMIA_REQUIRE(loop.stats().renderedFrames == 0U);
  KIMIA_REQUIRE(loop.tick(1.0 / 120.0, [](kimia::f64) {}) == 0U);
}
