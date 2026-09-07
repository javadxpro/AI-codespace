#pragma once

#include <kimia/Types.h>
#include <kimia/Vec.h>

#include <string>
#include <vector>

namespace kimia {

// --- Particles: the puffs, sparks and smoke a game needs ---
//
// An explosion, a dust cloud, a trail of sparks. Without them a game reads
// as a set of boxes moving about; with them it reads as a game.
//
// The design is deliberately a RECIPE plus a SIMULATION, not a scripting
// system: a person says "orange sparks, thrown upward, fading over half a
// second", and the engine does the rest. Anything more expressive would
// be a programming language wearing a costume.

// One recipe: what a burst looks like. Saved with the world, so a user's
// explosion belongs to their game rather than to the engine.
struct Emitter {
  std::string name;

  u32 count = 24U;        // particles per burst
  f64 life = 0.8;         // seconds each one lasts
  f64 speed = 3.0;        // how fast they leave the middle
  f64 spread = 1.0;       // 0 = a straight beam, 1 = a full ball
  Vec3 direction{0.0, 1.0, 0.0};  // which way the beam points
  f64 gravity = -4.0;     // downward pull; positive floats upward like smoke
  f64 size = 0.12;        // how big each particle starts
  f64 shrink = 1.0;       // how much of its size it loses over its life
  Vec3 colorStart{1.0, 0.75, 0.2};
  Vec3 colorEnd{0.5, 0.1, 0.0};
  f64 drag = 0.0;         // 0 = no air resistance
};

// One live particle. Plain data: the simulation is a loop over a vector,
// which is what a phone CPU is happiest with.
struct Particle {
  Vec3 position{0.0, 0.0, 0.0};
  Vec3 velocity{0.0, 0.0, 0.0};
  f64 age = 0.0;
  f64 life = 1.0;
  f64 size = 0.1;
  f64 shrink = 1.0;
  Vec3 colorStart{1.0, 1.0, 1.0};
  Vec3 colorEnd{1.0, 1.0, 1.0};
  // Carried per particle, not per system: smoke rising and sparks falling
  // must be able to exist at the same moment.
  f64 gravity = 0.0;
  f64 drag = 0.0;

  bool alive() const { return age < life; }
  // 0 at birth, 1 at death.
  f64 through() const { return life > 0.0 ? age / life : 1.0; }
  Vec3 colorNow() const;
  f64 sizeNow() const;
};

// Every particle in flight, and the recipes the world knows.
class ParticleSystem {
public:
  // Fires one burst of `emitter` at `at`. `seed` makes the scatter
  // repeatable: the same burst twice looks the same, so a replay of a
  // match is a replay rather than a different film.
  void burst(const Emitter& emitter, const Vec3& at, u32 seed);
  // Ages everything and drops what has died.
  void step(f64 seconds);
  void clear() { particles_.clear(); }

  const std::vector<Particle>& particles() const { return particles_; }
  usize count() const { return particles_.size(); }

private:
  std::vector<Particle> particles_;
};

// The recipes a world keeps, addressed by name so a rule can say
// "play the explosion here" with no code.
struct EmitterBook {
  std::vector<Emitter> emitters;
  const Emitter* find(const std::string& name) const;
  void set(const Emitter& emitter);  // add or replace by name
  bool remove(const std::string& name);
};

}  // namespace kimia
