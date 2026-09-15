// ParticlePanel — the right-hand dock that exposes the parameters
// of a single Emitter (spawn rate, particle life, gravity, color
// ramp, etc). The full preview window lives in Phase 5; for now
// this panel just lets the user read + edit the values.
//
// Phase 4+ placeholder: read-only inspector, same shape as the
// PhysicsPanel. The full inline-edit + preview loop lands in Phase 5.
#pragma once

#include "EditorUI.h"

namespace kimia::ui {

struct ParticleProps {
  f32 spawnRate = 10.0f;     // particles per second
  f32 lifeMin = 0.5f;        // seconds
  f32 lifeMax = 1.5f;
  f32 sizeMin = 0.05f;
  f32 sizeMax = 0.15f;
  f32 speedMin = 0.5f;
  f32 speedMax = 1.5f;
  f32 gravity = -4.0f;
  Vec3 colorStart{1.0, 0.75, 0.2};
  Vec3 colorEnd{0.5, 0.1, 0.0};
  bool looped = true;
  bool additive = true;
};

bool drawParticlePanel(const Rect& rect, const ParticleProps& current,
                       ParticleProps& edited);

}  // namespace kimia::ui
