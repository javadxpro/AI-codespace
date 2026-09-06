#include <kimia/Physics.h>
#include <kimia/MathUtils.h>
#include <kimia/Vec.h>
#include <kimia_test.h>

#include <cmath>
#include <utility>

namespace {
using kimia::DynamicBox;
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

KIMIA_TEST(physics_dynamic_box_falls_and_rests_on_floor) {
  PhysicsWorld world;
  world.addPlane(0.0);
  DynamicBox crate;
  crate.position = Vec3{1.0, 2.0, -1.0};
  crate.halfExtents = Vec3{0.5, 0.5, 0.5};
  crate.restitution = 0.25;
  const u32 id = world.addDynamicBox(crate);
  for (int i = 0; i < 360; ++i) world.advance(kDt);  // 3 s
  const DynamicBox* body = world.dynamicBox(id);
  KIMIA_REQUIRE(body != nullptr);
  // Rested on the floor: center at halfExtents.y = 0.5, velocity gone.
  KIMIA_REQUIRE(near(body->position.y, 0.5, 1e-6));
  KIMIA_REQUIRE(near(body->position.x, 1.0, 1e-6));
  KIMIA_REQUIRE(near(body->position.z, -1.0, 1e-6));
  KIMIA_REQUIRE(near(body->velocity.length(), 0.0, 1e-6));
  KIMIA_REQUIRE(body->collisionCount > 0U);
}

KIMIA_TEST(physics_dynamic_box_rests_on_static_box) {
  PhysicsWorld world;
  world.addPlane(0.0);
  world.addBox(Vec3{0.0, 0.5, 0.0}, Vec3{0.5, 0.5, 0.5});  // top face at y = 1.0
  DynamicBox crate;
  crate.position = Vec3{0.0, 2.5, 0.0};
  crate.halfExtents = Vec3{0.5, 0.5, 0.5};
  crate.restitution = 0.0;
  const u32 id = world.addDynamicBox(crate);
  for (int i = 0; i < 360; ++i) world.advance(kDt);  // 3 s
  // Rests on the block top: y = 1.0 + 0.5.
  KIMIA_REQUIRE(near(world.dynamicBox(id)->position.y, 1.5, 1e-4));
  KIMIA_REQUIRE(near(world.dynamicBox(id)->velocity.length(), 0.0, 1e-6));
}

KIMIA_TEST(physics_dynamic_box_stack_stays_stable) {
  PhysicsWorld world;
  world.addPlane(0.0);
  DynamicBox bottom;
  bottom.position = Vec3{0.0, 0.5, 0.0};
  const u32 bottomId = world.addDynamicBox(bottom);
  DynamicBox top;
  top.position = Vec3{0.0, 1.5, 0.0};
  const u32 topId = world.addDynamicBox(top);
  for (int i = 0; i < 720; ++i) world.advance(kDt);  // 6 s
  // The stack neither sinks nor slides apart.
  KIMIA_REQUIRE(near(world.dynamicBox(bottomId)->position.y, 0.5, 1e-4));
  KIMIA_REQUIRE(near(world.dynamicBox(topId)->position.y, 1.5, 1e-4));
  KIMIA_REQUIRE(near(world.dynamicBox(bottomId)->position.x, 0.0, 1e-6));
  KIMIA_REQUIRE(near(world.dynamicBox(topId)->position.x, 0.0, 1e-6));
  KIMIA_REQUIRE(near(world.dynamicBox(bottomId)->velocity.length(), 0.0, 1e-6));
  KIMIA_REQUIRE(near(world.dynamicBox(topId)->velocity.length(), 0.0, 1e-6));
}

KIMIA_TEST(physics_ball_knocks_dynamic_box_mass_ratio) {
  PhysicsWorld world;
  world.addPlane(0.0);
  SphereBody ball;
  ball.position = Vec3{-1.0, 0.12, 0.0};
  ball.velocity = Vec3{2.0, 0.0, 0.0};
  ball.radius = 0.12;
  ball.mass = 0.4;  // the KIMIA ball is lighter than a crate
  ball.restitution = 0.4;
  ball.friction = 0.0;
  ball.rollingFriction = 0.0;
  const u32 ballId = world.addSphere(ball);
  DynamicBox crate;
  crate.position = Vec3{0.0, 0.5, 0.0};
  crate.mass = 1.0;
  crate.restitution = 0.25;
  crate.friction = 0.0;
  crate.rollingFriction = 0.0;
  const u32 crateId = world.addDynamicBox(crate);

  bool hit = false;
  for (int i = 0; i < 600 && !hit; ++i) {
    world.advance(kDt);
    hit = world.dynamicBox(crateId)->velocity.x > 0.1;
  }
  KIMIA_REQUIRE(hit);
  // Elastic numbers: e = max(0.4, 0.25) = 0.4, m1 = 0.4, m2 = 1.0.
  //   v1' = (m1 - e*m2)/(m1+m2) * v1 = 0            -> the ball stops dead
  //   v2' = m1*(1+e)/(m1+m2) * v1 = 0.4 * v1 = 0.8  -> the crate slides off
  KIMIA_REQUIRE(near(world.sphere(ballId)->velocity.x, 0.0, 1e-6));
  KIMIA_REQUIRE(near(world.dynamicBox(crateId)->velocity.x, 0.8, 1e-3));
  KIMIA_REQUIRE(near(world.sphere(ballId)->position.x, -0.62, 1e-3));  // stopped at the face
}

KIMIA_TEST(physics_dynamic_box_knocks_dynamic_box) {
  PhysicsWorld world;
  world.addPlane(0.0);
  DynamicBox first;
  first.position = Vec3{-1.0, 0.5, 0.0};
  first.velocity = Vec3{2.0, 0.0, 0.0};
  first.restitution = 0.25;
  first.friction = 0.0;
  first.rollingFriction = 0.0;
  const u32 firstId = world.addDynamicBox(first);
  DynamicBox second;
  second.position = Vec3{0.0, 0.5, 0.0};
  second.restitution = 0.25;
  second.friction = 0.0;
  second.rollingFriction = 0.0;
  const u32 secondId = world.addDynamicBox(second);

  bool hit = false;
  for (int i = 0; i < 600 && !hit; ++i) {
    world.advance(kDt);
    hit = world.dynamicBox(secondId)->velocity.x > 0.1;
  }
  KIMIA_REQUIRE(hit);
  // Equal masses, e = 0.25: v1' = (1-e)/2 * v = 0.75, v2' = (1+e)/2 * v = 1.25.
  KIMIA_REQUIRE(near(world.dynamicBox(firstId)->velocity.x, 0.75, 1e-3));
  KIMIA_REQUIRE(near(world.dynamicBox(secondId)->velocity.x, 1.25, 1e-3));
}

KIMIA_TEST(physics_dynamic_box_slides_and_stops) {
  PhysicsWorld world;
  world.addPlane(0.0);
  DynamicBox crate;
  crate.position = Vec3{0.0, 0.5, 0.0};
  crate.velocity = Vec3{3.0, 0.0, 0.0};
  crate.friction = 0.5;
  crate.rollingFriction = 0.05;
  const u32 id = world.addDynamicBox(crate);
  for (int i = 0; i < 600; ++i) world.advance(kDt);  // 5 s
  const DynamicBox* body = world.dynamicBox(id);
  KIMIA_REQUIRE(near(body->velocity.x, 0.0, 1e-6));  // constant-force friction stops it
  // Slide distance = v^2 / (2 * 0.55 * g) = 9 / 10.791 ~= 0.834 m.
  KIMIA_REQUIRE(body->position.x > 0.7 && body->position.x < 0.95);
  KIMIA_REQUIRE(near(body->position.y, 0.5, 1e-6));  // never sank into the floor
}

KIMIA_TEST(physics_ball_rests_on_dynamic_box) {
  PhysicsWorld world;
  world.addPlane(0.0);
  DynamicBox crate;
  crate.position = Vec3{0.0, 0.5, 0.0};
  crate.restitution = 0.0;
  const u32 crateId = world.addDynamicBox(crate);
  SphereBody ball;
  ball.position = Vec3{0.0, 1.3, 0.0};
  ball.radius = 0.12;
  ball.restitution = 0.0;
  ball.friction = 0.0;
  ball.rollingFriction = 0.0;
  const u32 ballId = world.addSphere(ball);
  for (int i = 0; i < 360; ++i) world.advance(kDt);  // 3 s
  // The ball comes to rest ON TOP of the crate: y = 1.0 + 0.12.
  KIMIA_REQUIRE(near(world.sphere(ballId)->position.y, 1.12, 1e-4));
  KIMIA_REQUIRE(near(world.sphere(ballId)->velocity.length(), 0.0, 1e-6));
  KIMIA_REQUIRE(near(world.dynamicBox(crateId)->position.y, 0.5, 1e-4));
}

KIMIA_TEST(physics_sphere_embedded_in_box_pops_out_on_top) {
  // A sphere spawned INSIDE a grounded box must be ejected through the top,
  // never down through the floor (the old nearest-face heuristic sank it to
  // y = 0 fighting the plane).
  PhysicsWorld world;
  world.addPlane(0.0);
  world.addBox(Vec3{0.0, 0.5, 0.0}, Vec3{0.5, 0.5, 0.5});  // block on the floor
  SphereBody ball;
  ball.position = Vec3{0.0, 0.12, 0.0};  // inside the block (spawn overlap)
  ball.radius = 0.12;
  ball.restitution = 0.0;
  ball.friction = 0.0;
  ball.rollingFriction = 0.0;
  const u32 id = world.addSphere(ball);
  f64 minY = 0.12;
  for (int i = 0; i < 240; ++i) {
    world.advance(kDt);
    minY = std::min(minY, world.sphere(id)->position.y);
  }
  // Never sank into the floor; rests exactly on the block top (y = 1.12).
  KIMIA_REQUIRE(minY >= 0.11);
  KIMIA_REQUIRE(near(world.sphere(id)->position.y, 1.12, 1e-6));
  KIMIA_REQUIRE(near(world.sphere(id)->position.x, 0.0, 1e-6));
  KIMIA_REQUIRE(near(world.sphere(id)->velocity.length(), 0.0, 1e-6));
}

KIMIA_TEST(physics_resolve_spawn_height_raises_above_boxes) {
  PhysicsWorld world;
  world.addBox(Vec3{0.0, 0.5, 0.0}, Vec3{0.5, 0.5, 0.5});  // top at y = 1.0
  // Clear space: unchanged.
  KIMIA_REQUIRE(near(world.resolveSpawnHeight(Vec3{5.0, 0.12, 0.0}, 0.12, 10.0), 0.12, 1e-9));
  // Overlapping the block: raised to top + radius (+ margin).
  const f64 raised = world.resolveSpawnHeight(Vec3{0.0, 0.12, 0.0}, 0.12, 10.0);
  KIMIA_REQUIRE(raised > 1.1 && raised < 1.13);
  // A stack of dynamic crates: raised above the top crate (top at y = 2.0).
  DynamicBox bottom;
  bottom.position = Vec3{2.0, 0.5, 0.0};
  world.addDynamicBox(bottom);
  DynamicBox top;
  top.position = Vec3{2.0, 1.5, 0.0};
  world.addDynamicBox(top);
  const f64 stackRaised = world.resolveSpawnHeight(Vec3{2.0, 0.12, 0.0}, 0.12, 10.0);
  KIMIA_REQUIRE(stackRaised > 2.1 && stackRaised < 2.13);
  // The cap applies.
  KIMIA_REQUIRE(near(world.resolveSpawnHeight(Vec3{0.0, 0.12, 0.0}, 0.12, 1.05), 1.05, 1e-9));
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

// --- Character controller ---

KIMIA_TEST(physics_character_falls_and_lands_exactly) {
  PhysicsWorld world;
  world.addPlane(0.0);
  world.resetCharacter(Vec3{0.0, 1.7, 0.0});  // feet at 1.2 above the floor
  // Semi-implicit Euler free fall, closed form: feet(N) = 1.2 - g dt^2 N(N+1)/2.
  // The landing step is the first N with feet(N) <= 0.
  u32 expected = 0U;
  while (kG * kDt * kDt * static_cast<f64>(expected) * static_cast<f64>(expected + 1U) / 2.0 < 1.2) {
    ++expected;
  }
  u32 steps = 0U;
  while (world.character()->position.y > 0.5) {
    world.moveCharacter(kDt, Vec3{0.0, 0.0, 0.0});
    ++steps;
    KIMIA_REQUIRE(steps < 1000U);
  }
  KIMIA_REQUIRE(steps == expected);
  KIMIA_REQUIRE(near(world.character()->position.y, 0.5));  // feet exactly on the floor
  KIMIA_REQUIRE(near(world.character()->velocity.y, 0.0));
  KIMIA_REQUIRE(world.character()->onGround);
  world.moveCharacter(kDt, Vec3{0.0, 0.0, 0.0});
  KIMIA_REQUIRE(near(world.character()->position.y, 0.5));  // stays grounded
}

KIMIA_TEST(physics_character_jump_reaches_apex_height) {
  PhysicsWorld world;
  world.addPlane(0.0);
  world.resetCharacter(Vec3{0.0, 0.5, 0.0});
  for (u32 i = 0; i < 5; ++i) world.moveCharacter(kDt, Vec3{0.0, 0.0, 0.0});
  KIMIA_REQUIRE(world.character()->onGround);
  KIMIA_REQUIRE(world.characterJump(1.2));   // v = sqrt(2 g h)
  KIMIA_REQUIRE(!world.characterJump(1.2));  // airborne: no double jump
  f64 maxFeet = 0.0;
  while (!world.character()->onGround) {
    world.moveCharacter(kDt, Vec3{0.0, 0.0, 0.0});
    maxFeet = std::max(maxFeet, world.character()->position.y - 0.5);
  }
  // Gravity-first semi-implicit Euler (the engine's ordering), closed form:
  // feet(N) = dt*(N*v0 - g*dt*N*(N+1)/2); the last rising step is
  // N = floor(v0/(g*dt)).
  const f64 v0 = std::sqrt(2.0 * kG * 1.2);
  const u32 nApex = static_cast<u32>(std::floor(v0 / (kG * kDt)));
  const f64 apex = kDt * (static_cast<f64>(nApex) * v0 -
                          kG * kDt * static_cast<f64>(nApex) * static_cast<f64>(nApex + 1U) / 2.0);
  KIMIA_REQUIRE(std::abs(maxFeet - apex) < 1e-9);
  KIMIA_REQUIRE(apex < 1.2 && apex > 1.15);  // discrete apex sits just under the ask
  KIMIA_REQUIRE(near(world.character()->position.y, 0.5));  // landed back
}

KIMIA_TEST(physics_character_lands_on_a_crate_and_walks_off) {
  PhysicsWorld world;
  world.addPlane(0.0);
  world.addBox(Vec3{0.0, 0.5, 0.0}, Vec3{0.5, 0.5, 0.5});  // top face at y = 1.0
  world.resetCharacter(Vec3{0.0, 3.0, 0.0});               // dropped above the crate
  for (u32 i = 0; i < 200; ++i) world.moveCharacter(kDt, Vec3{0.0, 0.0, 0.0});
  KIMIA_REQUIRE(world.character()->onGround);
  KIMIA_REQUIRE(near(world.character()->position.y, 1.5));  // feet on the top (1.0)
  // Walking off the edge drops the character to the floor.
  for (u32 i = 0; i < 240; ++i) world.moveCharacter(kDt, Vec3{2.0, 0.0, 0.0});
  KIMIA_REQUIRE(near(world.character()->position.y, 0.5));
  KIMIA_REQUIRE(world.character()->onGround);
}

KIMIA_TEST(physics_character_wall_blocks_and_slides) {
  PhysicsWorld world;
  world.addPlane(0.0);
  world.addBox(Vec3{2.0, 0.5, 0.0}, Vec3{0.25, 0.5, 3.0});  // wall: 0.5 thick in X
  world.resetCharacter(Vec3{0.0, 0.5, -1.0});
  u32 touched = 0U;
  // Walk diagonally into the wall: X stops at the face, Z slides along.
  for (u32 i = 0; i < 240; ++i) {
    world.moveCharacter(kDt, Vec3{1.0, 0.0, 1.0});
    touched = std::max(touched, world.character()->collisionCount);
  }
  KIMIA_REQUIRE(touched > 0U);  // the wall really blocked the character
  KIMIA_REQUIRE(near(world.character()->position.x, 2.0 - 0.25 - 0.3, 1e-6));
  KIMIA_REQUIRE(near(world.character()->position.z, 1.0, 1e-6));  // slid two meters
  KIMIA_REQUIRE(near(world.character()->position.y, 0.5, 1e-6));  // never left the floor
}

KIMIA_TEST(physics_character_stops_at_dynamic_crate_face) {
  PhysicsWorld world;
  world.addPlane(0.0);
  DynamicBox crate;
  crate.position = Vec3{2.0, 0.5, 0.0};
  crate.halfExtents = Vec3{0.5, 0.5, 0.5};
  const u32 crateId = world.addDynamicBox(crate);
  world.resetCharacter(Vec3{0.0, 0.5, 0.0});
  for (u32 i = 0; i < 120; ++i) world.moveCharacter(kDt, Vec3{2.0, 0.0, 0.0});
  // The crate is solid for the character; the game layer shoves it along.
  KIMIA_REQUIRE(near(world.character()->position.x, 2.0 - 0.5 - 0.3, 1e-6));
  KIMIA_REQUIRE(near(world.dynamicBox(crateId)->position.x, 2.0, 1e-9));
  KIMIA_REQUIRE(near(world.character()->position.y, 0.5, 1e-6));
}

KIMIA_TEST(physics_character_high_fall_never_tunnels) {
  PhysicsWorld world;
  world.addPlane(0.0);
  world.addBox(Vec3{0.0, 0.25, 0.0}, Vec3{0.5, 0.25, 0.5});  // a 0.5 tall slab below
  world.resetCharacter(Vec3{0.0, 20.0, 0.0});                // a 20 m drop
  f64 minFeet = 20.0;
  for (u32 i = 0; i < 2400; ++i) {  // 20 s: plenty for the capped fall
    world.moveCharacter(kDt, Vec3{0.0, 0.0, 0.0});
    minFeet = std::min(minFeet, world.character()->position.y - 0.5);
  }
  KIMIA_REQUIRE(minFeet >= -1e-9);  // never below the floor surface
  KIMIA_REQUIRE(near(world.character()->position.y, 1.0));  // resting on the slab (top 0.5)
  KIMIA_REQUIRE(world.character()->onGround);
}

// --- Wind (stage 20.5-b2) ---

KIMIA_TEST(physics_wind_vector_matches_speed_and_direction) {
  using kimia::makeWind;
  using kimia::Wind;
  // Direction 0 blows toward -Z (the aim-yaw convention), +pi/2 toward -X.
  const Wind north = makeWind(3.0, 0.0);
  KIMIA_REQUIRE(near(north.acceleration.x, 0.0));
  KIMIA_REQUIRE(near(north.acceleration.z, -3.0));
  KIMIA_REQUIRE(near(north.speed(), 3.0));
  KIMIA_REQUIRE(near(north.direction(), 0.0));
  const Wind left = makeWind(2.0, kimia::kPi * 0.5);
  KIMIA_REQUIRE(near(left.acceleration.x, -2.0, 1e-12));
  KIMIA_REQUIRE(near(left.acceleration.z, 0.0, 1e-12));
  KIMIA_REQUIRE(near(left.direction(), kimia::kPi * 0.5, 1e-12));
  // Calm is exactly inactive; over-strong wind is clamped, never refused.
  KIMIA_REQUIRE(!makeWind(0.0, 1.234).active());
  KIMIA_REQUIRE(near(makeWind(0.0, 1.234).speed(), 0.0));
  KIMIA_REQUIRE(near(makeWind(999.0, 0.0).speed(), kimia::kMaxWindAcceleration));
  KIMIA_REQUIRE(near(makeWind(-5.0, 0.0).speed(), 0.0));
}

KIMIA_TEST(physics_wind_deflects_an_airborne_ball_by_the_closed_form) {
  // A ball thrown straight up in a sideways wind drifts exactly like a body
  // under constant acceleration: x_N = a * dt^2 * N(N+1)/2 — the same closed
  // form the free-fall test uses, because wind IS gravity turned sideways.
  PhysicsWorld world;
  world.setWind(kimia::makeWind(4.0, kimia::kPi * 0.5));  // 4 m/s^2 toward -X
  SphereBody ball;
  ball.position = Vec3{0.0, 5.0, 0.0};
  ball.radius = 0.12;
  const u32 id = world.addSphere(ball);
  const u32 steps = 60U;  // 0.5 s of pure flight (it starts 5 m up)
  for (u32 i = 0; i < steps; ++i) world.step();
  const f64 n = static_cast<f64>(steps);
  const f64 expected = -4.0 * kDt * kDt * n * (n + 1.0) * 0.5;
  KIMIA_REQUIRE(near(world.sphere(id)->position.x, expected, 1e-9));
  KIMIA_REQUIRE(near(world.sphere(id)->position.z, 0.0, 1e-12));  // no cross-axis leak
  // Gravity is untouched by the wind.
  KIMIA_REQUIRE(near(world.sphere(id)->position.y, 5.0 - kG * kDt * kDt * n * (n + 1.0) * 0.5, 1e-9));
}

KIMIA_TEST(physics_wind_bends_a_rolling_putt_within_the_friction_budget) {
  // On the ground the breeze bends a rolling ball, but the push is capped by
  // the friction the turf supplies, so it can never out-run the surface.
  const auto driftOf = [](f64 windAccel, bool onGround) {
    PhysicsWorld world;
    world.addPlane(0.0);
    world.setWind(kimia::makeWind(windAccel, kimia::kPi * 0.5));  // toward -X
    SphereBody ball;  // default golf tuning: friction 0.40, rolling 0.22
    ball.radius = 0.12;
    ball.position = Vec3{0.0, onGround ? 0.12 : 6.0, 0.0};
    ball.velocity = Vec3{0.0, 0.0, -4.0};
    const u32 id = world.addSphere(ball);
    for (u32 i = 0; i < 60U; ++i) world.step();  // 0.5 s
    return world.sphere(id)->position.x;
  };
  // A gentle 2 m/s^2 breeze: below the friction cap, so the ground push is
  // exactly kWindGroundFactor of the airborne one.
  const f64 rolling = driftOf(2.0, true);
  const f64 flying = driftOf(2.0, false);
  KIMIA_REQUIRE(rolling < -0.001);   // the putt really does bend
  KIMIA_REQUIRE(rolling > flying);   // less than the same shot in the air
  // The ground ball keeps kWindGroundFactor of the push and then loses part
  // of the sideways speed it gained to friction, so the measured ratio sits
  // just under the factor (0.252 with the golf tuning).
  KIMIA_REQUIRE(rolling / flying < kimia::kWindGroundFactor);
  KIMIA_REQUIRE(near(rolling / flying, 0.2522, 0.005));
  // The friction cap: the strongest legal gale cannot push the ball on the
  // ground harder than the surface friction it is fighting.
  const f64 gale = driftOf(kimia::kMaxWindAcceleration, true);
  const f64 cap = (0.40 + 0.22) * kG;  // the golf ball's friction budget
  const f64 n = 60.0;
  const f64 maxDrift = cap * kDt * kDt * n * (n + 1.0) * 0.5;
  KIMIA_REQUIRE(std::abs(gale) <= maxDrift + 1e-9);
}

KIMIA_TEST(physics_wind_never_creeps_a_resting_ball) {
  // The whole point of the airborne rule: a ball sitting on the ground must
  // not be blown across the course for ten minutes.
  PhysicsWorld world;
  world.addPlane(0.0);
  world.setWind(kimia::makeWind(kimia::kMaxWindAcceleration, kimia::kPi * 0.5));  // the strongest legal gale
  SphereBody ball;
  ball.position = Vec3{0.0, 0.12, 0.0};
  ball.radius = 0.12;
  const u32 id = world.addSphere(ball);
  for (u32 i = 0; i < 1200U; ++i) world.step();  // 10 seconds
  KIMIA_REQUIRE(std::abs(world.sphere(id)->position.x) < 0.02);
  KIMIA_REQUIRE(near(world.sphere(id)->position.y, 0.12, 1e-6));
}

KIMIA_TEST(physics_wind_respects_the_body_wind_factor) {
  // windFactor 0 = immune (a heavy ball), 0.5 = half the drift.
  PhysicsWorld world;
  world.setWind(kimia::makeWind(4.0, kimia::kPi * 0.5));
  SphereBody immune;
  immune.position = Vec3{0.0, 5.0, 0.0};
  immune.windFactor = 0.0;
  SphereBody half;
  half.position = Vec3{0.0, 5.0, 10.0};  // far apart: they must not touch each other
  half.windFactor = 0.5;
  const u32 immuneId = world.addSphere(immune);
  const u32 halfId = world.addSphere(half);
  for (u32 i = 0; i < 60U; ++i) world.step();
  const f64 n = 60.0;
  const f64 full = -4.0 * kDt * kDt * n * (n + 1.0) * 0.5;
  KIMIA_REQUIRE(near(world.sphere(immuneId)->position.x, 0.0, 1e-12));
  KIMIA_REQUIRE(near(world.sphere(halfId)->position.x, full * 0.5, 1e-9));
  KIMIA_REQUIRE(near(world.sphere(halfId)->position.z, 10.0, 1e-12));
}

KIMIA_TEST(physics_wind_is_deterministic_across_host_rates) {
  // The same shot in the same wind must land on the same spot whether the
  // host runs at 60 or 120 fps — otherwise a record would be meaningless.
  const auto fly = [](f64 hostDt, u32 frames) {
    PhysicsWorld world;
    world.addPlane(0.0);
    world.setWind(kimia::makeWind(3.5, 0.7));
    SphereBody ball;
    ball.position = Vec3{0.0, 0.12, 0.0};
    ball.radius = 0.12;
    ball.velocity = Vec3{2.0, 6.0, -1.0};
    const u32 id = world.addSphere(ball);
    for (u32 i = 0; i < frames; ++i) world.advance(hostDt);
    return world.sphere(id)->position;
  };
  const Vec3 at120 = fly(1.0 / 120.0, 240U);  // 2 s
  const Vec3 at60 = fly(1.0 / 60.0, 120U);    // 2 s
  KIMIA_REQUIRE(near(at120.x, at60.x, 1e-12));
  KIMIA_REQUIRE(near(at120.y, at60.y, 1e-12));
  KIMIA_REQUIRE(near(at120.z, at60.z, 1e-12));
}

KIMIA_TEST(physics_calm_world_is_bit_identical_to_no_wind_at_all) {
  // Regression guard: adding wind must not have changed a single number in
  // any existing world (every profile ships calm).
  const auto fly = [](bool setCalmWind) {
    PhysicsWorld world;
    world.addPlane(0.0);
    if (setCalmWind) world.setWind(kimia::makeWind(0.0, 1.1));
    SphereBody ball;
    ball.position = Vec3{0.0, 3.0, 0.0};
    ball.radius = 0.12;
    ball.velocity = Vec3{5.0, 0.0, -2.0};
    const u32 id = world.addSphere(ball);
    for (u32 i = 0; i < 600U; ++i) world.step();
    return world.sphere(id)->position;
  };
  const Vec3 untouched = fly(false);
  const Vec3 calm = fly(true);
  KIMIA_REQUIRE(untouched.x == calm.x);
  KIMIA_REQUIRE(untouched.y == calm.y);
  KIMIA_REQUIRE(untouched.z == calm.z);
}

// --- Stage 21: many characters ---

KIMIA_TEST(physics_characters_have_their_own_ids_and_teams) {
  PhysicsWorld world;
  world.addPlane(0.0);
  // Every world starts with exactly the player, character 1, team 0.
  KIMIA_REQUIRE(world.characterCount() == 1U);
  KIMIA_REQUIRE(world.characterIds().size() == 1U);
  KIMIA_REQUIRE(world.characterIds()[0] == kimia::kPrimaryCharacter);
  KIMIA_REQUIRE(world.characterIds()[0] == 1U);
  KIMIA_REQUIRE(world.character() == world.characterById(1U));
  KIMIA_REQUIRE(world.character()->team == 0U);

  kimia::CharacterBody mate;
  mate.position = Vec3{2.0, 0.5, 0.0};
  mate.team = 1U;
  const u32 mateId = world.addCharacter(mate);
  kimia::CharacterBody foe;
  foe.position = Vec3{-2.0, 0.5, 0.0};
  foe.team = 2U;
  const u32 foeId = world.addCharacter(foe);
  KIMIA_REQUIRE(mateId == 2U);
  KIMIA_REQUIRE(foeId == 3U);
  KIMIA_REQUIRE(world.characterCount() == 3U);
  KIMIA_REQUIRE(world.characterById(mateId)->team == 1U);
  KIMIA_REQUIRE(world.characterById(foeId)->team == 2U);
  KIMIA_REQUIRE(near(world.characterById(foeId)->position.x, -2.0));

  // Characters must NOT eat the sphere/box ids: they are a separate space.
  // The plane above took body id 1, so the first sphere is still 2 — exactly
  // what it was before characters became plural.
  SphereBody ball;
  ball.position = Vec3{0.0, 2.0, 0.0};
  KIMIA_REQUIRE(world.addSphere(ball) == 2U);

  KIMIA_REQUIRE(world.removeCharacter(mateId));
  KIMIA_REQUIRE(!world.removeCharacter(mateId));  // gone already
  KIMIA_REQUIRE(world.characterCount() == 2U);
  KIMIA_REQUIRE(world.characterById(mateId) == nullptr);
  const std::vector<u32> ids = world.characterIds();  // ascending
  KIMIA_REQUIRE(ids.size() == 2U);
  KIMIA_REQUIRE(ids[0] == 1U);
  KIMIA_REQUIRE(ids[1] == 3U);
}

KIMIA_TEST(physics_a_character_cannot_walk_through_another_character) {
  PhysicsWorld world;
  world.addPlane(0.0);
  world.resetCharacter(Vec3{0.0, 0.5, 0.0});
  kimia::CharacterBody wallOfFlesh;
  wallOfFlesh.position = Vec3{1.2, 0.5, 0.0};  // 0.3 + 0.3 = 0.6 wide gap left
  const u32 blockerId = world.addCharacter(wallOfFlesh);
  // Walk right for a full second at 4 m/s: without a body in the way the
  // player would reach x = 4.0.
  for (u32 i = 0; i < 60U; ++i) world.moveCharacter(1.0 / 60.0, Vec3{4.0, 0.0, 0.0});
  const kimia::CharacterBody* player = world.character();
  // Stopped touching the blocker: centers exactly one full width apart.
  KIMIA_REQUIRE(near(player->position.x, 1.2 - 0.6, 1e-9));
  KIMIA_REQUIRE(player->position.x < 0.61);
  KIMIA_REQUIRE(player->collisionCount > 0U);
  // The blocker never moved — it is not pushed, it is solid.
  KIMIA_REQUIRE(near(world.characterById(blockerId)->position.x, 1.2));
}

KIMIA_TEST(physics_a_character_can_stand_on_another_characters_head) {
  PhysicsWorld world;
  world.addPlane(0.0);
  kimia::CharacterBody base;
  base.position = Vec3{0.0, 0.5, 0.0};  // top of the head at y = 1.0
  world.addCharacter(base);
  // Drop the player from above onto that head.
  world.resetCharacter(Vec3{0.0, 3.0, 0.0});
  for (u32 i = 0; i < 120U; ++i) world.moveCharacter(1.0 / 60.0, Vec3{0.0, 0.0, 0.0});
  const kimia::CharacterBody* player = world.character();
  // Feet on the head: center = head top (1.0) + half height (0.5) = 1.5.
  KIMIA_REQUIRE(near(player->position.y, 1.5, 1e-9));
  KIMIA_REQUIRE(player->onGround);
  KIMIA_REQUIRE(near(player->velocity.y, 0.0, 1e-9));
}

KIMIA_TEST(physics_clear_keeps_the_player_and_drops_the_extra_characters) {
  PhysicsWorld world;
  world.addPlane(0.0);
  world.resetCharacter(Vec3{1.0, 0.5, 2.0});
  kimia::CharacterBody mate;
  mate.team = 2U;
  world.addCharacter(mate);
  KIMIA_REQUIRE(world.characterCount() == 2U);
  world.clear();
  // Rebuilding the level keeps player 1 exactly where it stood.
  KIMIA_REQUIRE(world.characterCount() == 1U);
  KIMIA_REQUIRE(near(world.character()->position.x, 1.0));
  KIMIA_REQUIRE(near(world.character()->position.z, 2.0));
  // And the next spawn starts the extra ids over again at 2.
  KIMIA_REQUIRE(world.addCharacter(kimia::CharacterBody{}) == 2U);
}

// --- Stage 23: spin and the Magnus force ---

KIMIA_TEST(physics_side_spin_bends_a_flying_ball_and_no_spin_flies_straight) {
  const auto fly = [](f64 spinY) {
    PhysicsWorld world;
    world.addPlane(0.0);
    SphereBody ball;
    ball.position = Vec3{0.0, 1.0, 0.0};
    ball.radius = 0.12;
    ball.velocity = Vec3{0.0, 2.0, -8.0};  // struck down the pitch, toward -Z
    ball.spin = Vec3{0.0, spinY, 0.0};
    const kimia::u32 id = world.addSphere(ball);
    for (kimia::u32 i = 0; i < 40U; ++i) world.step();
    return world.sphere(id)->position;
  };
  // No spin: dead straight, x stays exactly zero.
  const Vec3 straight = fly(0.0);
  KIMIA_REQUIRE(straight.x == 0.0);
  KIMIA_REQUIRE(straight.z < -1.0);
  // Spin one way bends one way, the other way bends the other, and by the
  // same amount — the force is symmetric.
  const Vec3 curled = fly(12.0);
  const Vec3 opposite = fly(-12.0);
  KIMIA_REQUIRE(curled.x < -0.05);
  KIMIA_REQUIRE(opposite.x > 0.05);
  KIMIA_REQUIRE(near(curled.x, -opposite.x, 1e-12));
  // The curl is a bend, not a teleport: it stays a fraction of the travel.
  KIMIA_REQUIRE(std::abs(curled.x) < std::abs(curled.z) * 0.5);
}

KIMIA_TEST(physics_spin_decays_in_the_air_and_is_scrubbed_off_by_the_ground) {
  PhysicsWorld world;
  world.addPlane(0.0);
  SphereBody ball;
  ball.position = Vec3{0.0, 4.0, 0.0};
  ball.radius = 0.12;
  ball.velocity = Vec3{0.0, 0.0, -4.0};
  ball.spin = Vec3{0.0, 20.0, 0.0};
  const kimia::u32 id = world.addSphere(ball);
  world.step();
  // One step in the air: spin drops by exactly the air decay factor.
  const f64 dt = 1.0 / 120.0;
  KIMIA_REQUIRE(near(world.sphere(id)->spin.y, 20.0 * (1.0 - kimia::kSpinAirDecay * dt), 1e-12));
  KIMIA_REQUIRE(world.sphere(id)->spin.y < 20.0);
  // Land it: the turf takes most of the spin away at the first contact.
  for (kimia::u32 i = 0; i < 240U; ++i) world.step();
  KIMIA_REQUIRE(world.sphere(id)->position.y <= 0.13);
  KIMIA_REQUIRE(std::abs(world.sphere(id)->spin.y) < 1.0);
}

KIMIA_TEST(physics_a_ball_immune_to_magnus_never_curls) {
  // magnusFactor 0 must be bit-identical to having no spin at all: this is
  // the guard that every existing world is untouched by stage 23.
  const auto fly = [](f64 factor, f64 spinY) {
    PhysicsWorld world;
    world.addPlane(0.0);
    SphereBody ball;
    ball.position = Vec3{0.0, 2.0, 0.0};
    ball.radius = 0.12;
    ball.velocity = Vec3{1.0, 1.0, -6.0};
    ball.spin = Vec3{0.0, spinY, 0.0};
    ball.magnusFactor = factor;
    const kimia::u32 id = world.addSphere(ball);
    for (kimia::u32 i = 0; i < 100U; ++i) world.step();
    return world.sphere(id)->position;
  };
  const Vec3 immune = fly(0.0, 18.0);
  const Vec3 spinless = fly(1.0, 0.0);
  KIMIA_REQUIRE(immune.x == spinless.x);
  KIMIA_REQUIRE(immune.y == spinless.y);
  KIMIA_REQUIRE(immune.z == spinless.z);
}

// --- Stage 24: a wet surface ---

KIMIA_TEST(physics_a_wet_pitch_lets_the_ball_run_further) {
  const auto roll = [](f64 wetness) {
    PhysicsWorld world;
    world.addPlane(0.0);
    world.setWetness(wetness);
    SphereBody ball;
    ball.position = Vec3{0.0, 0.12, 0.0};
    ball.radius = 0.12;
    ball.velocity = Vec3{4.0, 0.0, 0.0};
    const kimia::u32 id = world.addSphere(ball);
    for (kimia::u32 i = 0; i < 600U; ++i) world.step();
    return world.sphere(id)->position.x;
  };
  const f64 dry = roll(0.0);
  const f64 damp = roll(0.5);
  const f64 soaked = roll(1.0);
  // Wetter always means further, and it is a real difference, not noise.
  KIMIA_REQUIRE(damp > dry + 0.05);
  KIMIA_REQUIRE(soaked > damp + 0.05);
  // But never frictionless: even soaked, the ball stops inside the range a
  // 4 m/s roll should manage. (kWetMinGrip keeps a third of the grip.)
  KIMIA_REQUIRE(soaked < dry * 4.0);
}

KIMIA_TEST(physics_grip_factor_is_exact_and_dry_is_untouched) {
  PhysicsWorld world;
  // Dry must be EXACTLY 1.0: this is the guard that a dry world behaves
  // bit-identically to the engine before weather existed.
  KIMIA_REQUIRE(world.wetness() == 0.0);
  KIMIA_REQUIRE(world.gripFactor() == 1.0);
  // Soaked keeps exactly kWetMinGrip.
  world.setWetness(1.0);
  KIMIA_REQUIRE(near(world.gripFactor(), kimia::kWetMinGrip, 1e-12));
  KIMIA_REQUIRE(near(world.gripFactor(), 0.35, 1e-12));
  // Half wet is exactly half way between.
  world.setWetness(0.5);
  KIMIA_REQUIRE(near(world.gripFactor(), 1.0 - 0.65 * 0.5, 1e-12));
  KIMIA_REQUIRE(near(world.gripFactor(), 0.675, 1e-12));
  // And it clamps.
  world.setWetness(5.0);
  KIMIA_REQUIRE(world.wetness() == 1.0);
  world.setWetness(-5.0);
  KIMIA_REQUIRE(world.wetness() == 0.0);
  KIMIA_REQUIRE(world.gripFactor() == 1.0);
}

KIMIA_TEST(physics_dry_weather_changes_nothing_at_all) {
  // Two identical rolls, one on a world that never heard of wetness and one
  // explicitly set dry: the positions must match to the last bit.
  const auto roll = [](bool touchWetness) {
    PhysicsWorld world;
    world.addPlane(0.0);
    if (touchWetness) world.setWetness(0.0);
    SphereBody ball;
    ball.position = Vec3{0.0, 0.5, 0.0};
    ball.radius = 0.12;
    ball.velocity = Vec3{3.0, 0.0, -2.0};
    const kimia::u32 id = world.addSphere(ball);
    for (kimia::u32 i = 0; i < 300U; ++i) world.step();
    return world.sphere(id)->position;
  };
  const Vec3 untouched = roll(false);
  const Vec3 explicitDry = roll(true);
  KIMIA_REQUIRE(untouched.x == explicitDry.x);
  KIMIA_REQUIRE(untouched.y == explicitDry.y);
  KIMIA_REQUIRE(untouched.z == explicitDry.z);
}
