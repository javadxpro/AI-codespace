#include <kimia/Physics.h>
#include <kimia/Vec.h>
#include <kimia_test.h>

#include <cmath>
#include <utility>

namespace {
using kimia::PhysicsWorld;
using kimia::SphereBody;
using kimia::Vec3;
using kimia::f64;
using kimia::u32;

constexpr f64 kG = kimia::kGravity;
constexpr f64 kDt = 1.0 / 120.0;

bool near(f64 a, f64 b, f64 eps = 1e-9) { return std::abs(a - b) <= eps; }
}  // namespace

KIMIA_TEST(physics_sphere_falls_with_gravity_exact) {
  PhysicsWorld world;
  SphereBody ball;
  ball.position = Vec3{0.0, 5.0, 0.0};
  ball.restitution = 0.0;
  const u32 id = world.addSphere(ball);
  // No colliders: pure free fall. Semi-implicit Euler after N steps:
  //   y_N = y0 - g * dt^2 * N*(N+1)/2,  v_N = -g * dt * N
  const u32 steps = 10;
  for (u32 n = 0; n < steps; ++n) world.step();
  f64 expectedY = 5.0;
  for (u32 n = 1; n <= steps; ++n) expectedY -= kG * kDt * kDt * static_cast<f64>(n);
  KIMIA_REQUIRE(near(world.sphere(id)->position.y, expectedY, 1e-9));
  KIMIA_REQUIRE(near(world.sphere(id)->velocity.y, -kG * kDt * static_cast<f64>(steps), 1e-9));
  KIMIA_REQUIRE(world.sphere(id)->velocity.x == 0.0);
  KIMIA_REQUIRE(world.sphere(id)->velocity.z == 0.0);
  KIMIA_REQUIRE(world.sphere(id)->position.x == 0.0);
  KIMIA_REQUIRE(near(world.time(), static_cast<f64>(steps) * kDt));
  KIMIA_REQUIRE(world.stepCount() == steps);
}

KIMIA_TEST(physics_restitution_bounce_height) {
  PhysicsWorld world;
  world.addPlane(0.0);
  SphereBody ball;
  ball.position = Vec3{0.0, 5.0, 0.0};
  ball.radius = 0.12;
  ball.restitution = 0.4;
  ball.friction = 0.0;
  ball.rollingFriction = 0.0;
  const u32 id = world.addSphere(ball);

  // Track the apex of the FIRST bounce: rest height is radius (0.12), drop
  // height above rest is 4.88, expected apex ~= rest + e^2 * 4.88 ~= 0.9008.
  bool rising = false;
  bool apexDone = false;
  f64 maxDuringRise = 0.0;
  f64 firstApex = 0.0;
  for (int i = 0; i < 480; ++i) {  // 4 s of 120 Hz
    world.advance(kDt);
    const SphereBody* body = world.sphere(id);
    if (!apexDone) {
      if (body->velocity.y > 0.0) {
        rising = true;
        maxDuringRise = std::max(maxDuringRise, body->position.y);
      } else if (rising) {
        apexDone = true;
        firstApex = maxDuringRise;
      }
    }
  }
  KIMIA_REQUIRE(apexDone);
  KIMIA_REQUIRE(firstApex > 0.87 && firstApex < 0.93);
}

KIMIA_TEST(physics_friction_slows_ball_by_constant_force) {
  PhysicsWorld world;
  world.addPlane(0.0);
  SphereBody ball;
  ball.position = Vec3{0.0, 0.12, 0.0};
  ball.velocity = Vec3{1.0, 0.0, 0.0};
  ball.radius = 0.12;
  ball.friction = 0.05;  // small friction: a measurable loss over 1 s
  ball.rollingFriction = 0.0;
  ball.restitution = 0.0;
  const u32 id = world.addSphere(ball);
  for (int i = 0; i < 120; ++i) world.advance(kDt);  // 1 s
  const Vec3 velocity = world.sphere(id)->velocity;
  // Constant-force friction: v(1s) = v0 - friction * g * 1s = 1 - 0.4905.
  KIMIA_REQUIRE(near(velocity.x, 1.0 - 0.05 * kG, 1e-6));
  KIMIA_REQUIRE(near(velocity.y, 0.0, 1e-6));
  KIMIA_REQUIRE(velocity.z == 0.0);
  // Ball stays on the plane surface.
  KIMIA_REQUIRE(near(world.sphere(id)->position.y, 0.12, 1e-6));
}

KIMIA_TEST(physics_strong_friction_stops_ball_exactly) {
  PhysicsWorld world;
  world.addPlane(0.0);
  SphereBody ball;
  ball.position = Vec3{0.0, 0.12, 0.0};
  ball.velocity = Vec3{1.0, 0.0, 0.0};
  ball.radius = 0.12;
  ball.friction = 4.0;  // 39.24 m/s^2: 1 m/s stops in ~25 ms
  ball.rollingFriction = 0.0;
  ball.restitution = 0.0;
  const u32 id = world.addSphere(ball);
  for (int i = 0; i < 120; ++i) world.advance(kDt);  // 1 s
  const Vec3 velocity = world.sphere(id)->velocity;
  KIMIA_REQUIRE(velocity.x == 0.0);  // friction never overshoots below zero
  KIMIA_REQUIRE(near(velocity.y, 0.0, 1e-6));
  KIMIA_REQUIRE(velocity.z == 0.0);
  KIMIA_REQUIRE(near(world.sphere(id)->position.y, 0.12, 1e-6));
}

KIMIA_TEST(physics_resting_ball_settles_without_bouncing) {
  PhysicsWorld world;
  world.addPlane(0.0);
  SphereBody ball;
  ball.position = Vec3{0.0, 0.15, 0.0};  // exactly at rest height
  ball.radius = 0.15;
  ball.restitution = 0.85;  // fantasy ball: must still settle
  ball.friction = 0.05;
  ball.rollingFriction = 0.0;
  const u32 id = world.addSphere(ball);
  f64 maxY = 0.15;
  for (int i = 0; i < 600; ++i) {  // 5 s
    world.advance(kDt);
    maxY = std::max(maxY, world.sphere(id)->position.y);
  }
  // No bounce at all: slow impacts (below the threshold) settle instead.
  KIMIA_REQUIRE(near(maxY, 0.15, 1e-9));
  const Vec3 velocity = world.sphere(id)->velocity;
  KIMIA_REQUIRE(velocity.x == 0.0);
  KIMIA_REQUIRE(near(velocity.y, 0.0, 1e-12));
  KIMIA_REQUIRE(velocity.z == 0.0);
}

KIMIA_TEST(physics_box_side_collision) {
  PhysicsWorld world;
  // Tall box: side face at x = 1.5, ball contacts at x = 1.0.
  world.addBox(Vec3{2.0, 0.0, 0.0}, Vec3{0.5, 10.0, 2.0});
  SphereBody ball;
  ball.position = Vec3{0.0, 0.0, 0.0};
  ball.velocity = Vec3{1.0, 0.0, 0.0};
  ball.radius = 0.5;
  ball.restitution = 0.5;
  ball.friction = 0.0;
  const u32 id = world.addSphere(ball);
  f64 maxX = -1e18;
  for (int i = 0; i < 240; ++i) {  // 2 s
    world.advance(kDt);
    maxX = std::max(maxX, world.sphere(id)->position.x);
  }
  // Never penetrates the face (contact plane at x = 1.0).
  KIMIA_REQUIRE(maxX <= 1.0 + 1e-9);
  KIMIA_REQUIRE(maxX >= 1.0 - 1e-6);  // it did reach the face
  // Bounced back with restitution 0.5: velocity now negative.
  KIMIA_REQUIRE(world.sphere(id)->velocity.x < 0.0);
  KIMIA_REQUIRE(near(world.sphere(id)->velocity.x, -0.5, 1e-9));
  KIMIA_REQUIRE(world.sphere(id)->velocity.z == 0.0);
}

KIMIA_TEST(physics_box_miss_passes_untouched) {
  PhysicsWorld world;
  world.addBox(Vec3{2.0, 0.0, 5.0}, Vec3{0.5, 0.5, 0.5});  // far away on z
  SphereBody ball;
  ball.position = Vec3{0.0, 0.0, 0.0};
  ball.velocity = Vec3{1.0, 0.0, 0.0};
  ball.radius = 0.5;
  const u32 id = world.addSphere(ball);
  for (int i = 0; i < 120; ++i) world.advance(kDt);  // 1 s
  KIMIA_REQUIRE(near(world.sphere(id)->position.x, 1.0, 1e-9));
  KIMIA_REQUIRE(near(world.sphere(id)->velocity.x, 1.0, 1e-12));
  KIMIA_REQUIRE(world.sphere(id)->position.z == 0.0);
  KIMIA_REQUIRE(world.sphere(id)->collisionCount == 0U);
}

KIMIA_TEST(physics_stability_60_and_120hz) {
  auto makeWorld = []() {
    PhysicsWorld world;
    world.addPlane(0.0);
    world.addBox(Vec3{2.0, 0.0, 0.0}, Vec3{0.5, 1.0, 2.0});
    SphereBody ball;
    ball.position = Vec3{0.0, 0.12, 0.3};
    ball.velocity = Vec3{0.8, 2.5, 0.1};
    ball.radius = 0.12;
    ball.restitution = 0.4;
    ball.friction = 0.4;
    ball.rollingFriction = 0.22;
    const u32 id = world.addSphere(ball);
    return std::make_pair(std::move(world), id);
  };
  auto pairA = makeWorld();
  auto pairB = makeWorld();
  PhysicsWorld& worldA = pairA.first;
  PhysicsWorld& worldB = pairB.first;
  // 4 seconds of simulation: A advanced at 60 Hz host rate (2 fixed steps
  // per frame), B at 120 Hz (1 per frame). Both must end bit-identical.
  for (int i = 0; i < 240; ++i) worldA.advance(1.0 / 60.0);
  for (int i = 0; i < 480; ++i) worldB.advance(1.0 / 120.0);
  KIMIA_REQUIRE(worldA.time() == worldB.time());
  KIMIA_REQUIRE(worldA.stepCount() == worldB.stepCount());
  const SphereBody* a = worldA.sphere(pairA.second);
  const SphereBody* b = worldB.sphere(pairB.second);
  KIMIA_REQUIRE(a != nullptr && b != nullptr);
  KIMIA_REQUIRE(near(a->position.x, b->position.x, 1e-12));
  KIMIA_REQUIRE(near(a->position.y, b->position.y, 1e-12));
  KIMIA_REQUIRE(near(a->position.z, b->position.z, 1e-12));
  KIMIA_REQUIRE(near(a->velocity.x, b->velocity.x, 1e-12));
  KIMIA_REQUIRE(near(a->velocity.y, b->velocity.y, 1e-12));
  KIMIA_REQUIRE(near(a->velocity.z, b->velocity.z, 1e-12));
}

KIMIA_TEST(physics_accumulator_cap) {
  PhysicsWorld world;  // max 5 steps per frame
  SphereBody ball;
  const u32 id = world.addSphere(ball);
  // A 10-second host frame must run at most 5 fixed steps (then drop).
  const u32 steps = world.advance(10.0);
  KIMIA_REQUIRE(steps == 5U);
  KIMIA_REQUIRE(world.stepCount() == 5U);
  KIMIA_REQUIRE(near(world.time(), 5.0 * kDt, 1e-12));
  // Normal frames resume cleanly after the cap.
  const u32 more = world.advance(kDt);
  KIMIA_REQUIRE(more == 1U);
  KIMIA_REQUIRE(world.stepCount() == 6U);
  KIMIA_REQUIRE(near(world.sphere(id)->velocity.y, -6.0 * kG * kDt, 1e-9));
}

KIMIA_TEST(physics_box_top_collision_lands) {
  PhysicsWorld world;
  // Box top face at y = 0.5; ball dropped from above must land on it and
  // come to rest on the top face (restitution 0 for a clean rest).
  world.addBox(Vec3{0.0, 0.0, 0.0}, Vec3{0.5, 0.5, 0.5});
  SphereBody ball;
  ball.position = Vec3{0.0, 3.0, 0.0};
  ball.radius = 0.25;
  ball.restitution = 0.0;
  ball.friction = 0.0;
  ball.rollingFriction = 0.0;
  const u32 id = world.addSphere(ball);
  for (int i = 0; i < 360; ++i) world.advance(kDt);  // 3 s
  // Resting on top: y = boxTop + radius = 0.5 + 0.25.
  KIMIA_REQUIRE(near(world.sphere(id)->position.y, 0.75, 1e-6));
  KIMIA_REQUIRE(near(world.sphere(id)->velocity.y, 0.0, 1e-6));
}

KIMIA_TEST(physics_ids_and_removal) {
  PhysicsWorld world;
  const u32 first = world.addSphere(SphereBody{});
  const u32 second = world.addSphere(SphereBody{});
  KIMIA_REQUIRE(first == 1U && second == 2U);
  KIMIA_REQUIRE(world.sphere(0U) == nullptr);
  KIMIA_REQUIRE(world.removeSphere(first));
  KIMIA_REQUIRE(world.sphere(first) == nullptr);
  KIMIA_REQUIRE(world.sphere(second) != nullptr);
  KIMIA_REQUIRE(!world.removeSphere(first));
  const u32 third = world.addSphere(SphereBody{});
  KIMIA_REQUIRE(third == 3U);  // ids are never reused
  world.clear();
  KIMIA_REQUIRE(world.sphereCount() == 0U);
  KIMIA_REQUIRE(world.time() == 0.0);
}
