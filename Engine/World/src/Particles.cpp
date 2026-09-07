#include <kimia/Particles.h>

#include <cmath>

namespace kimia {

namespace {

// A tiny deterministic generator. Deliberately not <random>: the same
// burst has to look the same on every machine and in every replay, and
// the standard engines make no such promise across implementations.
struct Scatter {
  u64 state = 1U;
  explicit Scatter(u32 seed) : state(static_cast<u64>(seed) * u64{2654435761} + u64{1}) {}
  f64 next() {
    state = state * u64{6364136223846793005} + u64{1442695040888963407};
    // The top bits are the well-mixed ones.
    return static_cast<f64>((state >> 33) & u64{0x7FFFFF}) / static_cast<f64>(0x7FFFFF);
  }
  // -1..1
  f64 signed1() { return next() * 2.0 - 1.0; }
};

Vec3 lerp(const Vec3& a, const Vec3& b, f64 t) { return a + (b - a) * t; }

}  // namespace

Vec3 Particle::colorNow() const { return lerp(colorStart, colorEnd, through()); }

f64 Particle::sizeNow() const {
  const f64 left = 1.0 - shrink * through();
  return size * (left > 0.0 ? left : 0.0);
}

void ParticleSystem::burst(const Emitter& emitter, const Vec3& at, u32 seed) {
  if (emitter.count == 0U || emitter.life <= 0.0) return;
  Scatter scatter(seed);

  Vec3 aim = emitter.direction;
  const f64 aimLength = aim.length();
  // A recipe with no direction throws particles in every direction rather
  // than refusing to fire.
  aim = aimLength > 1e-9 ? aim * (1.0 / aimLength) : Vec3{0.0, 1.0, 0.0};

  for (u32 i = 0; i < emitter.count; ++i) {
    Particle particle;
    particle.position = at;

    // Spread blends between the aim and a random direction: 0 is a tight
    // beam, 1 is a ball. Anything in between is a cone, which is what a
    // person actually wants for a spark or a puff.
    Vec3 random{scatter.signed1(), scatter.signed1(), scatter.signed1()};
    const f64 randomLength = random.length();
    random = randomLength > 1e-9 ? random * (1.0 / randomLength) : aim;
    const f64 mix = emitter.spread < 0.0 ? 0.0 : (emitter.spread > 1.0 ? 1.0 : emitter.spread);
    Vec3 heading = lerp(aim, random, mix);
    const f64 headingLength = heading.length();
    heading = headingLength > 1e-9 ? heading * (1.0 / headingLength) : aim;

    // Vary the speed a little, or a burst looks like a solid shell.
    particle.velocity = heading * (emitter.speed * (0.6 + 0.4 * scatter.next()));
    particle.life = emitter.life * (0.7 + 0.6 * scatter.next());
    particle.size = emitter.size;
    particle.shrink = emitter.shrink;
    particle.colorStart = emitter.colorStart;
    particle.colorEnd = emitter.colorEnd;
    particle.gravity = emitter.gravity;
    particle.drag = emitter.drag;
    particles_.push_back(particle);
  }
}

void ParticleSystem::step(f64 seconds) {
  if (seconds <= 0.0) return;
  for (usize i = particles_.size(); i > 0U; --i) {
    Particle& particle = particles_[i - 1U];
    particle.age += seconds;
    if (!particle.alive()) {
      particles_.erase(particles_.begin() + static_cast<std::ptrdiff_t>(i - 1U));
      continue;
    }
    particle.velocity.y += particle.gravity * seconds;
    if (particle.drag > 0.0) {
      const f64 keep = 1.0 - particle.drag * seconds;
      particle.velocity = particle.velocity * (keep > 0.0 ? keep : 0.0);
    }
    particle.position += particle.velocity * seconds;
  }
}

const Emitter* EmitterBook::find(const std::string& name) const {
  for (const Emitter& emitter : emitters) {
    if (emitter.name == name) return &emitter;
  }
  return nullptr;
}

void EmitterBook::set(const Emitter& emitter) {
  if (emitter.name.empty()) return;
  for (Emitter& existing : emitters) {
    if (existing.name != emitter.name) continue;
    existing = emitter;  // editing a recipe is a replace, not a second one
    return;
  }
  emitters.push_back(emitter);
}

bool EmitterBook::remove(const std::string& name) {
  for (usize i = 0; i < emitters.size(); ++i) {
    if (emitters[i].name != name) continue;
    emitters.erase(emitters.begin() + static_cast<std::ptrdiff_t>(i));
    return true;
  }
  return false;
}

}  // namespace kimia
