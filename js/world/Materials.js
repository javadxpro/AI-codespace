import * as THREE from '../../vendor/three.module.js';

/**
 * Surface library. Keep this table data-driven so later PBR / weather
 * layers can swap implementations without touching the editor.
 */
export const MATERIAL_DEFS = {
  dirt:  { kind: 'lambert', color: '#6b4f2a', label: 'خاک' },
  stone: { kind: 'lambert', color: '#7a7d82', label: 'سنگ' },
  wood:  { kind: 'lambert', color: '#8b5a2b', label: 'چوب' },
  metal: { kind: 'phong',   color: '#9aa3ad', label: 'فلز', shininess: 96, specular: '#d8dee6' },
  glass: { kind: 'phong',   color: '#c8e8ff', label: 'شیشه', shininess: 120, specular: '#ffffff', transparent: true, opacity: 0.38 },
};

export function createMaterial(id, colorHex) {
  const def = MATERIAL_DEFS[id] || MATERIAL_DEFS.wood;
  const color = new THREE.Color(colorHex || def.color);
  if (def.kind === 'phong') {
    return new THREE.MeshPhongMaterial({
      color,
      shininess: def.shininess ?? 40,
      specular: new THREE.Color(def.specular || '#777777'),
      transparent: !!def.transparent,
      opacity: def.opacity ?? 1,
      depthWrite: !def.transparent,
      vertexColors: false,
    });
  }
  return new THREE.MeshLambertMaterial({ color, vertexColors: false });
}

export function terrainMaterial() {
  return new THREE.MeshLambertMaterial({
    vertexColors: true,
    flatShading: false,
  });
}

export function hexToColor(hex) {
  return new THREE.Color(hex);
}
