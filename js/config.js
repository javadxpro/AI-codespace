/** Kimiya World Editor — runtime constants. Tuned for low-power mobile GPUs. */
export const CONFIG = {
  appId: 'kimiya-world',
  format: 'kimiya-world',
  version: 1,

  terrainSize: 64,
  terrainSegments: 48,
  heightMin: -5,
  heightMax: 18,
  waterLevel: 0.12,

  maxEntities: 700,
  eyeHeight: 1.62,
  playerRadius: 0.32,
  walkSpeed: 8.4,
  sprintSpeed: 13.2,
  gravity: 24,
  jumpSpeed: 8.4,
  thirdPersonDistance: 6.4,
  thirdPersonHeight: 1.35,

  cameraFov: 70,
  cameraNear: 0.1,
  cameraFar: 140,
  fogNear: 38,
  fogFar: 96,

  autosaveMs: 18000,
  lookSensitivity: 0.00235,
  snapStep: 0.5,
  rotateSnap: Math.PI / 12,

  qualityCaps: { low: 1, medium: 1.25, high: 1.7 },
};

export const PRIMITIVES = [
  'cube', 'sphere', 'cylinder', 'cone', 'pyramid', 'prism', 'slab', 'torus',
];

export const MATERIAL_IDS = ['dirt', 'stone', 'wood', 'metal', 'glass'];

export const COLOR_PRESETS = [
  '#e8d9b8', '#c4a36a', '#8b5a2b', '#6b4f2a',
  '#7a7d82', '#4f5b66', '#3ecfb8', '#4a7c9b',
  '#e4b34a', '#e56b5a', '#7dcb7a', '#1f2430',
];
