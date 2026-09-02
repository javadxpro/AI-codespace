import * as THREE from '../../vendor/three.module.js';
import { CONFIG } from '../config.js';
import { clamp, lerp, round3 } from '../core/utils.js';
import { terrainMaterial, hexToColor } from './Materials.js';

function hash2(ix, iz) {
  let n = Math.imul(ix, 374761393) + Math.imul(iz, 668265263);
  n = Math.imul(n ^ (n >>> 13), 1274126177);
  return ((n ^ (n >>> 16)) >>> 0) / 4294967296;
}

function valueNoise(x, z) {
  const x0 = Math.floor(x);
  const z0 = Math.floor(z);
  const fx = x - x0;
  const fz = z - z0;
  const sx = fx * fx * (3 - 2 * fx);
  const sz = fz * fz * (3 - 2 * fz);
  const a = hash2(x0, z0);
  const b = hash2(x0 + 1, z0);
  const c = hash2(x0, z0 + 1);
  const d = hash2(x0 + 1, z0 + 1);
  return lerp(lerp(a, b, sx), lerp(c, d, sx), sz);
}

function fbm(x, z) {
  let v = 0;
  let a = 0.5;
  let f = 1;
  for (let i = 0; i < 5; i++) {
    v += a * valueNoise(x * f, z * f);
    a *= 0.5;
    f *= 2.03;
  }
  return v;
}

function heightColor(h, island) {
  if (h < 0.35) return [0.76, 0.70, 0.52];
  if (h < 1.4) return [0.45, 0.55, 0.28];
  if (h < 3.2) return [0.32, 0.48, 0.24];
  if (island < 0.18) return [0.76, 0.70, 0.52];
  return [0.52, 0.50, 0.48];
}

/**
 * Heightfield terrain. Vertices live on XZ; Y is elevation.
 * Designed so a future streaming / chunk system can replace this class.
 */
export class Terrain {
  constructor({ size = CONFIG.terrainSize, segments = CONFIG.terrainSegments } = {}) {
    this.size = size;
    this.segments = segments;
    this.n = segments + 1;
    this.heights = new Float32Array(this.n * this.n);
    this.geo = this._makeGeometry();
    this.mesh = new THREE.Mesh(this.geo, terrainMaterial());
    this.mesh.receiveShadow = false;
    this.mesh.castShadow = false;
    this.mesh.userData.isTerrain = true;
    this.mesh.name = 'terrain';
  }

  _makeGeometry() {
    const n = this.n;
    const size = this.size;
    const seg = this.segments;
    const pos = new Float32Array(n * n * 3);
    const col = new Float32Array(n * n * 3);
    const uv = new Float32Array(n * n * 2);
    const idx = new Uint32Array(seg * seg * 6);
    for (let z = 0; z < n; z++) {
      for (let x = 0; x < n; x++) {
        const i = z * n + x;
        pos[i * 3] = (x / seg - 0.5) * size;
        pos[i * 3 + 1] = 0;
        pos[i * 3 + 2] = (z / seg - 0.5) * size;
        uv[i * 2] = x / seg;
        uv[i * 2 + 1] = z / seg;
      }
    }
    let t = 0;
    for (let z = 0; z < seg; z++) {
      for (let x = 0; x < seg; x++) {
        const a = z * n + x;
        const b = a + 1;
        const c = (z + 1) * n + x;
        const d = c + 1;
        idx[t++] = a; idx[t++] = c; idx[t++] = b;
        idx[t++] = b; idx[t++] = c; idx[t++] = d;
      }
    }
    const geo = new THREE.BufferGeometry();
    geo.setAttribute('position', new THREE.BufferAttribute(pos, 3));
    geo.setAttribute('color', new THREE.BufferAttribute(col, 3));
    geo.setAttribute('uv', new THREE.BufferAttribute(uv, 2));
    geo.setIndex(new THREE.BufferAttribute(idx, 1));
    geo.computeVertexNormals();
    geo.computeBoundingSphere();
    return geo;
  }

  index(ix, iz) {
    return iz * this.n + ix;
  }

  generateIsland(seed = 7) {
    const n = this.n;
    const seg = this.segments;
    const pos = this.geo.attributes.position;
    const col = this.geo.attributes.color;
    const ox = seed * 17.2;
    const oz = seed * 9.7;
    for (let z = 0; z < n; z++) {
      for (let x = 0; x < n; x++) {
        const i = this.index(x, z);
        const nx = (x / seg) * 2 - 1;
        const nz = (z / seg) * 2 - 1;
        const d = Math.sqrt(nx * nx + nz * nz);
        const island = Math.max(0, 1 - Math.pow(d * 1.12, 1.65));
        const noise = fbm(x * 0.11 + ox, z * 0.11 + oz);
        const ridges = Math.abs(fbm(x * 0.07 + 40, z * 0.07 + 12) - 0.5) * 2;
        let h = island * (0.55 + noise * 3.4 + ridges * 1.1) - 0.35 * (1 - island);
        if (d > 0.92) h = Math.min(h, -0.4);
        h = clamp(h, CONFIG.heightMin, CONFIG.heightMax);
        this.heights[i] = h;
        pos.setY(i, h);
        const [r, g, b] = heightColor(h, island);
        col.setXYZ(i, r, g, b);
      }
    }
    this._afterMutate();
  }

  getHeightAt(x, z) {
    const seg = this.segments;
    const n = this.n;
    const u = (x / this.size + 0.5) * seg;
    const v = (z / this.size + 0.5) * seg;
    const u0 = clamp(Math.floor(u), 0, seg);
    const v0 = clamp(Math.floor(v), 0, seg);
    const u1 = clamp(u0 + 1, 0, seg);
    const v1 = clamp(v0 + 1, 0, seg);
    const fu = clamp(u - u0, 0, 1);
    const fv = clamp(v - v0, 0, 1);
    const h00 = this.heights[this.index(u0, v0)];
    const h10 = this.heights[this.index(u1, v0)];
    const h01 = this.heights[this.index(u0, v1)];
    const h11 = this.heights[this.index(u1, v1)];
    return lerp(lerp(h00, h10, fu), lerp(h01, h11, fu), fv);
  }

  cloneHeights() {
    return Float32Array.from(this.heights);
  }

  setHeights(arr) {
    this.heights.set(arr);
    const pos = this.geo.attributes.position;
    for (let i = 0; i < this.heights.length; i++) pos.setY(i, this.heights[i]);
    this._afterMutate();
  }

  _neighborAvg(ix, iz) {
    const n = this.n;
    let s = 0;
    let c = 0;
    for (let dz = -1; dz <= 1; dz++) {
      for (let dx = -1; dx <= 1; dx++) {
        const x = ix + dx;
        const z = iz + dz;
        if (x < 0 || z < 0 || x >= n || z >= n) continue;
        s += this.heights[this.index(x, z)];
        c++;
      }
    }
    return s / Math.max(1, c);
  }

  /**
   * Pen-like sculpt. `flattenY` is sampled at stroke start.
   * `paintHex` used only for the paint brush.
   */
  applyBrush(px, pz, { mode, radius, strength, dt, flattenY, paintHex }) {
    const n = this.n;
    const pos = this.geo.attributes.position;
    const col = this.geo.attributes.color;
    const r = Math.max(0.4, radius);
    const r2 = r * r;
    const rate = mode === 'smooth' ? 5.5 : mode === 'flatten' ? 4.2 : 5.8;
    const amt = strength * rate * dt;
    const paint = paintHex ? hexToColor(paintHex) : null;
    for (let iz = 0; iz < n; iz++) {
      for (let ix = 0; ix < n; ix++) {
        const i = this.index(ix, iz);
        const x = pos.getX(i);
        const z = pos.getZ(i);
        const dx = x - px;
        const dz = z - pz;
        const d2 = dx * dx + dz * dz;
        if (d2 > r2) continue;
        const w = 0.5 * (1 + Math.cos(Math.PI * Math.sqrt(d2) / r));
        let y = this.heights[i];
        if (mode === 'raise') y += amt * w;
        else if (mode === 'lower') y -= amt * w;
        else if (mode === 'flatten') y += (flattenY - y) * Math.min(1, amt * w * 1.4);
        else if (mode === 'smooth') {
          const avg = this._neighborAvg(ix, iz);
          y += (avg - y) * Math.min(1, amt * w);
        } else if (mode === 'paint' && paint) {
          const k = Math.min(1, w * strength * dt * 8);
          col.setXYZ(
            i,
            lerp(col.getX(i), paint.r, k),
            lerp(col.getY(i), paint.g, k),
            lerp(col.getZ(i), paint.b, k),
          );
        }
        y = clamp(y, CONFIG.heightMin, CONFIG.heightMax);
        this.heights[i] = y;
        pos.setY(i, y);
      }
    }
    pos.needsUpdate = true;
    col.needsUpdate = true;
    this._afterMutate();
  }

  _afterMutate() {
    this.geo.computeVertexNormals();
    this.geo.computeBoundingSphere();
    this.geo.attributes.position.needsUpdate = true;
  }

  toJSON() {
    const col = this.geo.attributes.color;
    const colors = new Array(this.heights.length * 3);
    for (let i = 0; i < this.heights.length; i++) {
      colors[i * 3] = round3(col.getX(i));
      colors[i * 3 + 1] = round3(col.getY(i));
      colors[i * 3 + 2] = round3(col.getZ(i));
    }
    return {
      size: this.size,
      segments: this.segments,
      heights: Array.from(this.heights, round3),
      colors,
    };
  }

  fromJSON(data) {
    if (!data) return;
    if (data.size && data.segments && (data.segments !== this.segments || data.size !== this.size)) {
      this.size = data.size;
      this.segments = data.segments;
      this.n = this.segments + 1;
      this.heights = new Float32Array(this.n * this.n);
      const old = this.mesh.material;
      this.geo.dispose();
      this.geo = this._makeGeometry();
      this.mesh.geometry = this.geo;
      this.mesh.material = old;
    }
    if (data.heights) this.setHeights(data.heights);
    if (data.colors && data.colors.length) {
      const col = this.geo.attributes.color;
      const len = Math.min(col.count, data.colors.length / 3);
      for (let i = 0; i < len; i++) {
        col.setXYZ(i, data.colors[i * 3], data.colors[i * 3 + 1], data.colors[i * 3 + 2]);
      }
      col.needsUpdate = true;
    }
  }
}
