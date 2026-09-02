/**
 * Extension point for future world generators.
 *
 * v1 uses `Terrain.generateIsland` (cheap value-noise).
 * Later:
 *   - AI world builder can return a kimiya-world JSON object
 *   - city layout can stamp prefab entities
 *   - weather systems should call `world.sky.setSun`
 *
 * Keep this module free of renderer details.
 */
export function generateLayout(/* params */) {
  return null;
}
