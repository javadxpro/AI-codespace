#include <kimia/Particles.h>
#include <kimia_test.h>

#include <cmath>
#include <string>

namespace {

using kimia::Emitter;
using kimia::EmitterBook;
using kimia::Particle;
using kimia::ParticleSystem;
using kimia::Vec3;
using kimia::f64;
using kimia::i32;
using kimia::u32;
using kimia::usize;

bool near(f64 a, f64 b, f64 tolerance = 1e-9) { return std::abs(a - b) <= tolerance; }

Emitter spark() {
  Emitter emitter;
  emitter.name = "spark";
  emitter.count = 20U;
  emitter.life = 1.0;
  emitter.speed = 4.0;
  emitter.spread = 1.0;
  emitter.gravity = -8.0;
  emitter.size = 0.2;
  emitter.shrink = 1.0;
  emitter.colorStart = Vec3{1.0, 1.0, 0.0};
  emitter.colorEnd = Vec3{1.0, 0.0, 0.0};
  return emitter;
}

}  // namespace

// --- Particles: the puffs, sparks and smoke a game needs ---

KIMIA_TEST(particles_a_burst_makes_particles_that_die_on_time) {
  ParticleSystem system;
  KIMIA_REQUIRE(system.count() == 0U);
  system.burst(spark(), Vec3{0.0, 1.0, 0.0}, 1U);
  KIMIA_REQUIRE(system.count() == 20U);

  // They all start where the burst was.
  for (const Particle& particle : system.particles()) {
    KIMIA_REQUIRE(near(particle.position.y, 1.0));
  }

  // Half a second in, they are still alive and have MOVED.
  system.step(0.5);
  KIMIA_REQUIRE(system.count() == 20U);
  bool moved = false;
  for (const Particle& particle : system.particles()) {
    if (std::abs(particle.position.y - 1.0) > 0.01) moved = true;
  }
  KIMIA_REQUIRE(moved);

  // Lives vary between 0.7x and 1.3x, so a burst FADES OUT rather than
  // vanishing all at once — that difference is what stops it looking like
  // a light switch. Sampled at 1.0 s, when the short-lived ones have gone
  // and the long-lived ones have not. (Stepping to 1.4 s first showed
  // nothing left, which was my arithmetic, not the engine's.)
  system.step(0.5);
  const usize halfway = system.count();
  KIMIA_REQUIRE(halfway > 0U);
  KIMIA_REQUIRE(halfway < 20U);
  // And eventually every one is gone.
  system.step(5.0);
  KIMIA_REQUIRE(system.count() == 0U);
}

KIMIA_TEST(particles_gravity_pulls_them_the_way_it_is_told) {
  ParticleSystem falling;
  Emitter heavy = spark();
  heavy.gravity = -20.0;
  heavy.speed = 0.0;  // no launch: gravity alone
  heavy.spread = 0.0;
  falling.burst(heavy, Vec3{0.0, 5.0, 0.0}, 7U);
  falling.step(0.5);
  for (const Particle& particle : falling.particles()) {
    KIMIA_REQUIRE(particle.position.y < 5.0);
  }

  // Positive gravity floats upward, which is how smoke is made.
  ParticleSystem smoke;
  Emitter rising = spark();
  rising.gravity = 6.0;
  rising.speed = 0.0;
  rising.spread = 0.0;
  smoke.burst(rising, Vec3{0.0, 1.0, 0.0}, 7U);
  smoke.step(0.5);
  for (const Particle& particle : smoke.particles()) {
    KIMIA_REQUIRE(particle.position.y > 1.0);
  }

  // Two recipes at once must not fight over one setting: sparks falling
  // and smoke rising have to coexist in the same frame.
  ParticleSystem both;
  both.burst(heavy, Vec3{0.0, 5.0, 0.0}, 3U);
  both.burst(rising, Vec3{10.0, 1.0, 0.0}, 4U);
  both.step(0.5);
  bool sawFalling = false;
  bool sawRising = false;
  for (const Particle& particle : both.particles()) {
    if (particle.position.x < 5.0 && particle.position.y < 5.0) sawFalling = true;
    if (particle.position.x > 5.0 && particle.position.y > 1.0) sawRising = true;
  }
  KIMIA_REQUIRE(sawFalling);
  KIMIA_REQUIRE(sawRising);
}

KIMIA_TEST(particles_spread_decides_a_beam_or_a_ball) {
  // Spread 0 is a jet, spread 1 is an explosion. Without that control
  // every effect looks the same.
  ParticleSystem beam;
  Emitter jet = spark();
  jet.spread = 0.0;
  jet.direction = Vec3{0.0, 1.0, 0.0};
  jet.gravity = 0.0;
  beam.burst(jet, Vec3{0.0, 0.0, 0.0}, 11U);
  beam.step(0.3);
  // Everything went essentially straight up.
  for (const Particle& particle : beam.particles()) {
    KIMIA_REQUIRE(particle.position.y > 0.0);
    KIMIA_REQUIRE(std::abs(particle.position.x) < 0.05);
    KIMIA_REQUIRE(std::abs(particle.position.z) < 0.05);
  }

  ParticleSystem ball;
  Emitter blast = spark();
  blast.spread = 1.0;
  blast.gravity = 0.0;
  ball.burst(blast, Vec3{0.0, 0.0, 0.0}, 11U);
  ball.step(0.3);
  // A ball really does go sideways as well as up.
  bool sideways = false;
  for (const Particle& particle : ball.particles()) {
    if (std::abs(particle.position.x) > 0.2 || std::abs(particle.position.z) > 0.2) sideways = true;
  }
  KIMIA_REQUIRE(sideways);
}

KIMIA_TEST(particles_fade_and_shrink_as_they_age) {
  ParticleSystem system;
  system.burst(spark(), Vec3{0.0, 0.0, 0.0}, 5U);
  const Particle fresh = system.particles()[0];
  KIMIA_REQUIRE(near(fresh.through(), 0.0));
  // A new particle is its starting colour and full size.
  KIMIA_REQUIRE(near(fresh.colorNow().y, 1.0, 1e-6));
  KIMIA_REQUIRE(near(fresh.sizeNow(), 0.2, 1e-6));

  Particle aged = fresh;
  aged.age = aged.life * 0.5;
  // Half way through it is half way between the two colours.
  KIMIA_REQUIRE(near(aged.colorNow().y, 0.5, 1e-6));
  KIMIA_REQUIRE(near(aged.sizeNow(), 0.1, 1e-6));

  // At the end it has shrunk to nothing rather than popping out at full
  // size, and a size can never go negative.
  aged.age = aged.life;
  KIMIA_REQUIRE(near(aged.sizeNow(), 0.0, 1e-6));
  aged.age = aged.life * 2.0;
  KIMIA_REQUIRE(aged.sizeNow() >= 0.0);
}

KIMIA_TEST(particles_the_same_seed_gives_the_same_burst) {
  // A replay of a match has to be a replay, not a different film.
  ParticleSystem first;
  ParticleSystem second;
  first.burst(spark(), Vec3{1.0, 2.0, 3.0}, 42U);
  second.burst(spark(), Vec3{1.0, 2.0, 3.0}, 42U);
  KIMIA_REQUIRE(first.count() == second.count());
  for (usize i = 0; i < first.count(); ++i) {
    KIMIA_REQUIRE(near(first.particles()[i].velocity.x, second.particles()[i].velocity.x));
    KIMIA_REQUIRE(near(first.particles()[i].life, second.particles()[i].life));
  }

  // A different seed really does look different.
  ParticleSystem other;
  other.burst(spark(), Vec3{1.0, 2.0, 3.0}, 43U);
  bool differs = false;
  for (usize i = 0; i < first.count(); ++i) {
    if (!near(first.particles()[i].velocity.x, other.particles()[i].velocity.x)) differs = true;
  }
  KIMIA_REQUIRE(differs);
}

KIMIA_TEST(particles_a_broken_recipe_is_harmless) {
  // The editor lets people type anything; none of it may crash a game.
  ParticleSystem system;
  Emitter empty = spark();
  empty.count = 0U;
  system.burst(empty, Vec3{0.0, 0.0, 0.0}, 1U);
  KIMIA_REQUIRE(system.count() == 0U);

  Emitter instant = spark();
  instant.life = 0.0;
  system.burst(instant, Vec3{0.0, 0.0, 0.0}, 1U);
  KIMIA_REQUIRE(system.count() == 0U);  // dead on arrival is never spawned

  // No direction at all still fires, rather than refusing.
  Emitter aimless = spark();
  aimless.direction = Vec3{0.0, 0.0, 0.0};
  aimless.spread = 0.0;
  system.burst(aimless, Vec3{0.0, 0.0, 0.0}, 1U);
  KIMIA_REQUIRE(system.count() == 20U);

  // Stepping backwards or by nothing changes nothing.
  const Vec3 before = system.particles()[0].position;
  system.step(0.0);
  system.step(-1.0);
  KIMIA_REQUIRE(near(system.particles()[0].position.x, before.x));
  system.clear();
  KIMIA_REQUIRE(system.count() == 0U);
}

KIMIA_TEST(particles_recipes_are_kept_by_name) {
  EmitterBook book;
  Emitter fire = spark();
  fire.name = "explosion";
  book.set(fire);
  KIMIA_REQUIRE(book.emitters.size() == 1U);

  // Editing a recipe replaces it rather than making a second.
  fire.count = 99U;
  book.set(fire);
  KIMIA_REQUIRE(book.emitters.size() == 1U);
  KIMIA_REQUIRE(book.find("explosion")->count == 99U);

  Emitter nameless = spark();
  nameless.name.clear();
  book.set(nameless);
  KIMIA_REQUIRE(book.emitters.size() == 1U);  // refused, not saved unreachable

  KIMIA_REQUIRE(book.find("nope") == nullptr);
  KIMIA_REQUIRE(book.remove("explosion"));
  KIMIA_REQUIRE(!book.remove("explosion"));
}
