import * as THREE from '../../vendor/three.module.js';
import { CONFIG } from '../config.js';
import { uid, round3 } from '../core/utils.js';
import { createMaterial } from './Materials.js';

const GEO = {};

function geometry(type) {
  if (GEO[type]) return GEO[type];
  let g;
  switch (type) {
    case 'sphere':
      g = new THREE.SphereGeometry(0.5, 14, 10);
      break;
    case 'cylinder':
      g = new THREE.CylinderGeometry(0.45, 0.45, 1, 10);
      break;
    case 'cone':
      g = new THREE.ConeGeometry(0.5, 1, 10);
      break;
    case 'pyramid':
      g = new THREE.ConeGeometry(0.55, 1, 4);
      break;
    case 'prism':
      g = new THREE.CylinderGeometry(0.5, 0.5, 1, 6);
      break;
    case 'slab':
      g = new THREE.BoxGeometry(1, 0.25, 1);
      break;
    case 'torus':
      g = new THREE.TorusGeometry(0.42, 0.16, 8, 14);
      break;
    case 'cube':
    default:
      g = new THREE.BoxGeometry(1, 1, 1);
      break;
  }
  GEO[type] = g;
  return g;
}

export function createGhostMaterial() {
  return new THREE.MeshLambertMaterial({
    color: 0x7dcb7a,
    transparent: true,
    opacity: 0.42,
    depthWrite: false,
  });
}

export class Entity {
  constructor({ id, type, mesh, materialId, color }) {
    this.id = id;
    this.type = type;
    this.mesh = mesh;
    this.materialId = materialId;
    this.color = color;
    mesh.userData.entityId = id;
    mesh.userData.isEntity = true;
  }

  applySurface(materialId, color) {
    const prev = this.mesh.material;
    this.materialId = materialId;
    this.color = color;
    this.mesh.material = createMaterial(materialId, color);
    if (prev && prev !== this.mesh.material) prev.dispose();
  }

  toJSON() {
    const p = this.mesh.position;
    const r = this.mesh.rotation;
    const s = this.mesh.scale;
    return {
      id: this.id,
      type: this.type,
      position: [round3(p.x), round3(p.y), round3(p.z)],
      rotation: [round3(r.x), round3(r.y), round3(r.z)],
      scale: [round3(s.x), round3(s.y), round3(s.z)],
      material: this.materialId,
      color: this.color,
    };
  }
}

export class EntityManager {
  constructor(scene) {
    this.scene = scene;
    this.map = new Map();
  }

  get size() {
    return this.map.size;
  }

  get(id) {
    return this.map.get(id);
  }

  meshes() {
    const out = [];
    for (const e of this.map.values()) out.push(e.mesh);
    return out;
  }

  add({ type, position, rotation, scale, materialId, color, id }) {
    if (this.map.size >= CONFIG.maxEntities) return null;
    const mesh = new THREE.Mesh(geometry(type || 'cube'), createMaterial(materialId || 'wood', color));
    mesh.castShadow = false;
    mesh.receiveShadow = false;
    if (position) mesh.position.fromArray(position);
    if (rotation) mesh.rotation.set(rotation[0], rotation[1], rotation[2]);
    if (scale) mesh.scale.fromArray(scale);
    const ent = new Entity({
      id: id || uid('e'),
      type: type || 'cube',
      mesh,
      materialId: materialId || 'wood',
      color: color || '#c4a36a',
    });
    this.map.set(ent.id, ent);
    this.scene.add(mesh);
    return ent;
  }

  remove(id) {
    const ent = this.map.get(id);
    if (!ent) return null;
    this.scene.remove(ent.mesh);
    if (ent.mesh.material) ent.mesh.material.dispose();
    this.map.delete(id);
    return ent.toJSON();
  }

  addFromJSON(data) {
    return this.add({
      id: data.id,
      type: data.type,
      position: data.position,
      rotation: data.rotation,
      scale: data.scale,
      materialId: data.material,
      color: data.color,
    });
  }

  clear() {
    for (const id of [...this.map.keys()]) this.remove(id);
  }

  toJSON() {
    return [...this.map.values()].map((e) => e.toJSON());
  }

  fromJSON(list) {
    this.clear();
    if (!list) return;
    for (const item of list) this.addFromJSON(item);
  }
}

export { geometry };
