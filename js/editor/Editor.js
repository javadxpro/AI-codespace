import * as THREE from '../../vendor/three.module.js';
import { CONFIG } from '../config.js';
import { roundTo } from '../core/utils.js';
import { geometry, createGhostMaterial } from '../world/Entities.js';

/**
 * Tool coordinator: observe / build / terrain / select.
 * Keeps mutation of World behind undoable commands.
 */
export class Editor {
  constructor({ engine, world, bus, history }) {
    this.engine = engine;
    this.world = world;
    this.bus = bus;
    this.history = history;
    this.mode = 'observe';
    this.primitive = 'cube';
    this.materialId = 'wood';
    this.color = '#c4a36a';
    this.snap = true;
    this.brush = 'raise';
    this.brushRadius = 3.4;
    this.brushStrength = 0.48;
    this.transformMode = 'move';
    this.selectedId = null;

    this.raycaster = new THREE.Raycaster();
    this._ndc = new THREE.Vector2();
    this._hitPoint = new THREE.Vector3();
    this._plane = new THREE.Plane(new THREE.Vector3(0, 1, 0), 0);
    this._planeHit = new THREE.Vector3();
    this._dragOffset = new THREE.Vector3();
    this._dragging = false;
    this._transformBefore = null;
    this._stroke = null;
    this.ghost = new THREE.Mesh(geometry('cube'), createGhostMaterial());
    this.ghost.visible = false;
    this.ghost.userData.ignoreRay = true;
    this.engine.scene.add(this.ghost);
    this._outline = new THREE.BoxHelper(this.ghost, 0xe4b34a);
    this._outline.visible = false;
    this.engine.scene.add(this._outline);

    this.brushRing = this._makeBrushRing();
    this.engine.scene.add(this.brushRing);
  }

  _makeBrushRing() {
    const g = new THREE.RingGeometry(0.92, 1, 40);
    g.rotateX(-Math.PI / 2);
    const m = new THREE.MeshBasicMaterial({
      color: 0xe4b34a,
      transparent: true,
      opacity: 0.55,
      depthWrite: false,
      side: THREE.DoubleSide,
    });
    const mesh = new THREE.Mesh(g, m);
    mesh.visible = false;
    mesh.userData.ignoreRay = true;
    return mesh;
  }

  setMode(mode) {
    this.mode = mode;
    this.engine.input.primaryLooks = mode === 'observe';
    if (mode !== 'select') this.select(null);
    if (mode !== 'build') this.ghost.visible = false;
    if (mode !== 'terrain') this.brushRing.visible = false;
    if (mode !== 'terrain') this._endStroke();
    this.bus.emit('mode:change', mode);
  }

  setPrimitive(type) {
    this.primitive = type;
    this.ghost.geometry = geometry(type);
  }

  setMaterial(id) {
    this.materialId = id;
    if (this.selectedId) {
      const e = this.world.entities.get(this.selectedId);
      if (e) {
        const before = e.toJSON();
        e.applySurface(id, this.color);
        this._pushSurface(before, e.toJSON());
      }
    }
    this.bus.emit('material:change', { id, color: this.color });
  }

  setColor(hex) {
    this.color = hex;
    if (this.selectedId) {
      const e = this.world.entities.get(this.selectedId);
      if (e) {
        const before = e.toJSON();
        e.applySurface(this.materialId, hex);
        this._pushSurface(before, e.toJSON());
      }
    }
    this.bus.emit('color:change', hex);
  }

  _pushSurface(before, after) {
    this.history.push({
      name: 'surface',
      undo: () => {
        const e = this.world.entities.get(before.id);
        if (e) e.applySurface(before.material, before.color);
      },
      redo: () => {
        const e = this.world.entities.get(after.id);
        if (e) e.applySurface(after.material, after.color);
      },
    });
  }

  select(id) {
    this.selectedId = id;
    this._outline.visible = !!id;
    this.bus.emit('selection:change', id);
  }

  _hitsFromNdc(x, y) {
    this._ndc.set(x, y);
    this.raycaster.setFromCamera(this._ndc, this.engine.camera);
    const list = [this.world.terrain.mesh, ...this.world.entities.meshes()];
    const hits = this.raycaster.intersectObjects(list, false);
    for (const h of hits) {
      if (h.object.userData.ignoreRay) continue;
      if (h.object === this.world.avatar || h.object.parent === this.world.avatar) continue;
      return h;
    }
    return null;
  }

  _raycast() {
    const input = this.engine.input;
    if (input.isTouch || this.mode === 'observe') return this._hitsFromNdc(0, 0);
    return this._hitsFromNdc(input.pointer.ndcX, input.pointer.ndcY);
  }

  update(dt) {
    const hit = this._raycast();
    if (this.mode === 'build') this._updateBuild(hit);
    else if (this.mode === 'terrain') this._updateTerrain(hit, dt);
    else if (this.mode === 'select') this._updateSelect(hit, dt);
    else {
      this.ghost.visible = false;
      this.brushRing.visible = false;
    }
    this._updateOutline();
  }

  _updateOutline() {
    const e = this.selectedId ? this.world.entities.get(this.selectedId) : null;
    if (!e) {
      this._outline.visible = false;
      return;
    }
    this._outline.visible = true;
    this._outline.setFromObject(e.mesh);
  }

  _placePoint(hit) {
    if (!hit) return null;
    const p = this._hitPoint.copy(hit.point);
    if (hit.face) p.addScaledVector(hit.face.normal, 0.02);
    const type = this.primitive;
    let hy = 0.5;
    if (type === 'slab') hy = 0.125;
    else if (type === 'torus') hy = 0.16;
    else if (type === 'sphere') hy = 0.5;
    p.y += hy;
    if (this.snap) {
      const s = CONFIG.snapStep;
      p.x = roundTo(p.x, s);
      p.z = roundTo(p.z, s);
      p.y = roundTo(p.y, s);
    }
    return p;
  }

  _updateBuild(hit) {
    const p = this._placePoint(hit);
    this.ghost.visible = !!p;
    if (!p) return;
    this.ghost.position.copy(p);
    this.ghost.material.color.set(0x7dcb7a);
    const input = this.engine.input;
    if (input.interactPressed()) this.placeAt(p);
  }

  placeAt(p) {
    if (this.world.entities.size >= CONFIG.maxEntities) {
      this.bus.emit('toast', 'limit');
      return;
    }
    const ent = this.world.entities.add({
      type: this.primitive,
      position: [p.x, p.y, p.z],
      materialId: this.materialId,
      color: this.color,
    });
    if (!ent) return;
    const data = ent.toJSON();
    this.history.push({
      name: 'place',
      undo: () => this.world.entities.remove(data.id),
      redo: () => this.world.entities.addFromJSON(data),
    });
    this.bus.emit('entity:added', data);
  }

  _updateTerrain(hit, dt) {
    const ok = hit && hit.object.userData.isTerrain;
    this.brushRing.visible = !!ok;
    if (ok) {
      this.brushRing.position.copy(hit.point);
      this.brushRing.position.y += 0.06;
      const s = this.brushRadius;
      this.brushRing.scale.set(s, 1, s);
    }
    const input = this.engine.input;
    if (ok && input.interactHeld()) {
      if (!this._stroke) {
        this._stroke = {
          before: this.world.terrain.cloneHeights(),
          flattenY: this.world.terrain.getHeightAt(hit.point.x, hit.point.z),
        };
      }
      this.world.terrain.applyBrush(hit.point.x, hit.point.z, {
        mode: this.brush,
        radius: this.brushRadius,
        strength: this.brushStrength,
        dt,
        flattenY: this._stroke.flattenY,
        paintHex: this.color,
      });
    }
    if (this._stroke && (input.pointer.released || input.actionReleased || !input.interactHeld())) {
      this._endStroke();
    }
  }

  _endStroke() {
    if (!this._stroke) return;
    const before = this._stroke.before;
    const after = this.world.terrain.cloneHeights();
    this._stroke = null;
    this.history.push({
      name: 'terrain',
      undo: () => this.world.terrain.setHeights(before),
      redo: () => this.world.terrain.setHeights(after),
    });
    this.bus.emit('terrain:changed');
  }

  _updateSelect(hit, dt) {
    this.ghost.visible = false;
    this.brushRing.visible = false;
    const input = this.engine.input;

    const pickEvent = input.isTouch ? input.pointer.pressed : input.pointer.started;
    if (pickEvent) {
      const pick = this._hitsFromNdc(input.pointer.ndcX, input.pointer.ndcY);
      if (pick && pick.object.userData.entityId) {
        this.select(pick.object.userData.entityId);
        const sel = this.world.entities.get(this.selectedId);
        this._dragging = !input.isTouch;
        this._transformBefore = sel.toJSON();
        this._plane.setFromNormalAndCoplanarPoint(this._plane.normal, sel.mesh.position);
        if (this.raycaster.ray.intersectPlane(this._plane, this._planeHit)) {
          this._dragOffset.copy(sel.mesh.position).sub(this._planeHit);
        }
      } else {
        this.select(null);
        this._dragging = false;
      }
    }
    if (e && input.interactPressed() && input.isTouch) {
      this._dragging = true;
      this._transformBefore = e.toJSON();
    }

    if (e && this._dragging && input.interactHeld()) {
      if (this.transformMode === 'move') {
        this._plane.constant = -e.mesh.position.y;
        this._plane.setFromNormalAndCoplanarPoint(new THREE.Vector3(0, 1, 0), e.mesh.position);
        if (this.raycaster.ray.intersectPlane(this._plane, this._planeHit)) {
          e.mesh.position.x = this._planeHit.x + this._dragOffset.x;
          e.mesh.position.z = this._planeHit.z + this._dragOffset.z;
          if (this.snap) {
            e.mesh.position.x = roundTo(e.mesh.position.x, CONFIG.snapStep);
            e.mesh.position.z = roundTo(e.mesh.position.z, CONFIG.snapStep);
          }
        }
      } else if (this.transformMode === 'rotate') {
        e.mesh.rotation.y -= input.pointerDx * 0.008;
      } else if (this.transformMode === 'scale') {
        const k = 1 + (-input.pointerDy) * 0.008;
        const s = clampScale(e.mesh.scale.x * k);
        e.mesh.scale.setScalar(s);
      }
    }

    if (this._dragging && (input.pointer.released || input.actionReleased)) {
      this._commitTransform();
    }
  }

  _commitTransform() {
    if (!this._dragging) return;
    this._dragging = false;
    const e = this.selectedId ? this.world.entities.get(this.selectedId) : null;
    if (!e || !this._transformBefore) return;
    const before = this._transformBefore;
    const after = e.toJSON();
    this._transformBefore = null;
    this.history.push({
      name: 'transform',
      undo: () => this._applyTransform(before),
      redo: () => this._applyTransform(after),
    });
  }

  _applyTransform(data) {
    const e = this.world.entities.get(data.id);
    if (!e) return;
    e.mesh.position.fromArray(data.position);
    e.mesh.rotation.set(data.rotation[0], data.rotation[1], data.rotation[2]);
    e.mesh.scale.fromArray(data.scale);
  }

  nudge(kind, dir) {
    const e = this.selectedId ? this.world.entities.get(this.selectedId) : null;
    if (!e) return;
    const before = e.toJSON();
    if (kind === 'move') {
      if (dir === 'up') e.mesh.position.y += CONFIG.snapStep;
      if (dir === 'down') e.mesh.position.y -= CONFIG.snapStep;
      if (dir === 'x+') e.mesh.position.x += CONFIG.snapStep;
      if (dir === 'x-') e.mesh.position.x -= CONFIG.snapStep;
      if (dir === 'z+') e.mesh.position.z += CONFIG.snapStep;
      if (dir === 'z-') e.mesh.position.z -= CONFIG.snapStep;
    } else if (kind === 'rotate') {
      e.mesh.rotation.y += dir * CONFIG.rotateSnap;
    } else if (kind === 'scale') {
      const s = clampScale(e.mesh.scale.x + dir * 0.1);
      e.mesh.scale.setScalar(s);
    }
    const after = e.toJSON();
    this.history.push({
      name: 'nudge',
      undo: () => this._applyTransform(before),
      redo: () => this._applyTransform(after),
    });
  }

  removeSelected() {
    const id = this.selectedId;
    if (!id) return;
    const data = this.world.entities.get(id)?.toJSON();
    if (!data) return;
    this.world.entities.remove(id);
    this.select(null);
    this.history.push({
      name: 'remove',
      undo: () => this.world.entities.addFromJSON(data),
      redo: () => {
        this.world.entities.remove(data.id);
        if (this.selectedId === data.id) this.select(null);
      },
    });
    this.bus.emit('toast', 'deleted');
  }

  duplicateSelected() {
    const e = this.selectedId ? this.world.entities.get(this.selectedId) : null;
    if (!e) return;
    const data = e.toJSON();
    data.id = undefined;
    data.position = [data.position[0] + 1, data.position[1], data.position[2]];
    const copy = this.world.entities.addFromJSON(data);
    if (!copy) return;
    const saved = copy.toJSON();
    this.select(copy.id);
    this.history.push({
      name: 'dup',
      undo: () => this.world.entities.remove(saved.id),
      redo: () => this.world.entities.addFromJSON(saved),
    });
  }
}

function clampScale(s) {
  return Math.max(0.2, Math.min(8, s));
}
(saved.id),
      redo: () => this.world.entities.addFromJSON(saved),
    });
  }
}

function clampScale(s) {
  return Math.max(0.2, Math.min(8, s));
}
