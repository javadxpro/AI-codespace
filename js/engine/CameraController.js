import * as THREE from '../../vendor/three.module.js';
import { CONFIG } from '../config.js';
import { clamp } from '../core/utils.js';

export class CameraController {
  constructor(camera, world, input) {
    this.camera = camera;
    this.world = world;
    this.input = input;
    this.thirdPerson = false;
    this._fwd = new THREE.Vector3();
    this._right = new THREE.Vector3();
    this._aimOrigin = new THREE.Vector3();
    this._aimDir = new THREE.Vector3();
  }

  setThirdPerson(on) {
    this.thirdPerson = !!on;
    this.world.thirdPerson = this.thirdPerson;
  }

  toggle() {
    this.setThirdPerson(!this.thirdPerson);
    return this.thirdPerson;
  }

  getAimRay(origin, dir) {
    const p = this.world.player;
    origin.set(p.position.x, p.position.y + CONFIG.eyeHeight, p.position.z);
    const cp = Math.cos(p.pitch);
    dir.set(Math.sin(p.yaw) * cp, Math.sin(p.pitch), Math.cos(p.yaw) * cp).normalize();
    return { origin, dir };
  }

  update(dt) {
    const p = this.world.player;
    const input = this.input;
    const sens = input.lookSensitivity * (input.settingsSensitivity || 1);
    p.yaw -= input.look.dx * sens;
    const inv = input.invertY ? -1 : 1;
    p.pitch -= input.look.dy * sens * inv;
    p.pitch = clamp(p.pitch, -1.32, 1.32);

    const mv = input.moveVector();
    const speed = mv.sprint ? CONFIG.sprintSpeed : CONFIG.walkSpeed;
    const sy = Math.sin(p.yaw);
    const cy = Math.cos(p.yaw);
    this._fwd.set(sy, 0, cy);
    this._right.set(cy, 0, -sy);
    p.position.x += (this._fwd.x * mv.z + this._right.x * mv.x) * speed * dt;
    p.position.z += (this._fwd.z * mv.z + this._right.z * mv.x) * speed * dt;

    const half = this.world.terrain.size * 0.5 - 1.2;
    p.position.x = clamp(p.position.x, -half, half);
    p.position.z = clamp(p.position.z, -half, half);

    const ground = this.world.terrain.getHeightAt(p.position.x, p.position.z);
    if (input.jumpPressed && p.onGround) p.vy = CONFIG.jumpSpeed;
    p.vy -= CONFIG.gravity * dt;
    p.position.y += p.vy * dt;
    if (p.position.y <= ground) {
      p.position.y = ground;
      p.vy = 0;
      p.onGround = true;
    } else {
      p.onGround = false;
    }

    const eyeY = p.position.y + CONFIG.eyeHeight;
    if (!this.thirdPerson) {
      this.camera.position.set(p.position.x, eyeY, p.position.z);
      this.camera.rotation.set(p.pitch, p.yaw, 0, 'YXZ');
    } else {
      const dist = CONFIG.thirdPersonDistance;
      const cp = Math.cos(p.pitch);
      const camX = p.position.x - Math.sin(p.yaw) * cp * dist;
      const camY = eyeY - Math.sin(p.pitch) * dist + CONFIG.thirdPersonHeight * 0.15;
      const camZ = p.position.z - Math.cos(p.yaw) * cp * dist;
      const minY = this.world.terrain.getHeightAt(camX, camZ) + 0.45;
      this.camera.position.set(camX, Math.max(camY, minY), camZ);
      this.camera.lookAt(p.position.x, eyeY, p.position.z);
    }

    this.world.updateAvatar(this.thirdPerson);
    this.world.sky.follow(p.position);
  }
}
