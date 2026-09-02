import * as THREE from '../../vendor/three.module.js';
import { CONFIG } from '../config.js';
import { round3 } from '../core/utils.js';
import { Terrain } from './Terrain.js';
import { EntityManager } from './Entities.js';
import { Sky } from './Sky.js';

/**
 * Authoritative world model. Renderer reads this; serializer writes this.
 * Future systems (AI builder, weather, city gen) should mutate World, not the GPU scene directly.
 */
export class World {
  constructor(scene, bus) {
    this.scene = scene;
    this.bus = bus;
    this.name = 'جهان نو';
    this.createdAt = new Date().toISOString();
    this.terrain = new Terrain();
    this.entities = new EntityManager(scene);
    this.sky = new Sky(scene);
    this.thirdPerson = false;
    this.player = {
      position: new THREE.Vector3(0, 2, 16),
      yaw: Math.PI,
      pitch: -0.12,
      vy: 0,
      onGround: true,
    };
    scene.add(this.terrain.mesh);
    this.avatar = this._makeAvatar();
    scene.add(this.avatar);
  }

  _makeAvatar() {
    const g = new THREE.Group();
    g.name = 'avatar';
    g.userData.ignoreRay = true;
    const bodyGeo = typeof THREE.CapsuleGeometry === 'function'
      ? new THREE.CapsuleGeometry(0.28, 0.7, 3, 6)
      : new THREE.CylinderGeometry(0.28, 0.28, 1.2, 8);
    const body = new THREE.Mesh(
      bodyGeo,
      new THREE.MeshLambertMaterial({ color: 0xe4b34a }),
    );
    body.position.y = 0.9;
    body.userData.ignoreRay = true;
    const head = new THREE.Mesh(
      new THREE.OctahedronGeometry(0.22, 0),
      new THREE.MeshLambertMaterial({ color: 0xf0d48a }),
    );
    head.position.y = 1.55;
    head.userData.ignoreRay = true;
    g.add(body);
    g.add(head);
    g.visible = false;
    return g;
  }

  createDefault() {
    this.name = 'جهان نو';
    this.createdAt = new Date().toISOString();
    this.entities.clear();
    this.terrain.generateIsland(Math.floor(Math.random() * 80) + 1);
    this.player.position.set(0, 0, this.terrain.size * 0.28);
    this.player.position.y = this.terrain.getHeightAt(0, this.player.position.z);
    this.player.yaw = Math.PI;
    this.player.pitch = -0.12;
    this.player.vy = 0;
    this.bus.emit('world:reset');
  }

  updateAvatar(thirdPerson) {
    this.thirdPerson = thirdPerson;
    this.avatar.visible = thirdPerson;
    this.avatar.position.copy(this.player.position);
    this.avatar.rotation.y = this.player.yaw;
  }

  serializeObject() {
    return {
      format: CONFIG.format,
      version: CONFIG.version,
      name: this.name,
      createdAt: this.createdAt,
      updatedAt: new Date().toISOString(),
      player: {
        position: this.player.position.toArray().map(round3),
        yaw: round3(this.player.yaw),
        pitch: round3(this.player.pitch),
        thirdPerson: this.thirdPerson,
      },
      terrain: this.terrain.toJSON(),
      entities: this.entities.toJSON(),
      environment: {
        sunElevation: this.sky.elevation,
        sunAzimuth: this.sky.azimuth,
      },
    };
  }

  serialize() {
    return JSON.stringify(this.serializeObject());
  }

  deserialize(data) {
    if (!data || data.format !== CONFIG.format) {
      throw new Error('فایل جهان کیمیا نیست');
    }
    this.name = data.name || 'جهان';
    this.createdAt = data.createdAt || new Date().toISOString();
    this.entities.fromJSON(data.entities || []);
    this.terrain.fromJSON(data.terrain);
    if (data.player) {
      if (data.player.position) this.player.position.fromArray(data.player.position);
      this.player.yaw = data.player.yaw ?? Math.PI;
      this.player.pitch = data.player.pitch ?? -0.12;
      this.thirdPerson = !!data.player.thirdPerson;
      this.player.vy = 0;
    }
    if (data.environment) {
      this.sky.setSun(
        data.environment.sunElevation ?? 0.92,
        data.environment.sunAzimuth ?? 0.52,
      );
    }
    this.bus.emit('world:loaded', data);
  }
}
