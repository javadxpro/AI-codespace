import * as THREE from '../../vendor/three.module.js';
import { CONFIG } from '../config.js';
import { InputManager } from './InputManager.js';
import { CameraController } from './CameraController.js';

export class Engine {
  constructor(canvas, bus) {
    this.canvas = canvas;
    this.bus = bus;
    this.quality = localStorage.getItem('kimiya.quality') || 'medium';
    this.scene = new THREE.Scene();
    this.camera = new THREE.PerspectiveCamera(CONFIG.cameraFov, 1, CONFIG.cameraNear, CONFIG.cameraFar);
    this.renderer = new THREE.WebGLRenderer({
      canvas,
      antialias: this.quality === 'high',
      powerPreference: 'low-power',
      alpha: false,
      stencil: false,
      depth: true,
    });
    this.renderer.setClearColor(0xb7c9d4, 1);
    if ('outputColorSpace' in this.renderer) {
      this.renderer.outputColorSpace = THREE.SRGBColorSpace;
    }
    this.clock = new THREE.Clock();
    this.input = new InputManager();
    this.input.attach(canvas);
    this.world = null;
    this.editor = null;
    this.cameraController = null;
    this.paused = false;
    this.fps = 0;
    this._frames = 0;
    this._fpsT = 0;
    this._bound = this._tick.bind(this);
    this.resize();
    window.addEventListener('resize', () => this.resize());
    window.addEventListener('orientationchange', () => setTimeout(() => this.resize(), 180));
    document.addEventListener('visibilitychange', () => {
      this.paused = document.hidden;
      if (!document.hidden) this.clock.getDelta();
    });
    canvas.addEventListener('webglcontextlost', (e) => e.preventDefault());
  }

  bind(world, editor) {
    this.world = world;
    this.editor = editor;
    this.cameraController = new CameraController(this.camera, world, this.input);
    this.cameraController.setThirdPerson(!!world.thirdPerson);
  }

  setQuality(q) {
    this.quality = q;
    localStorage.setItem('kimiya.quality', q);
    this.resize();
  }

  resize() {
    const w = Math.max(1, window.innerWidth);
    const h = Math.max(1, window.innerHeight);
    const cap = CONFIG.qualityCaps[this.quality] || 1.25;
    const pr = Math.min(window.devicePixelRatio || 1, cap);
    this.renderer.setPixelRatio(pr);
    this.renderer.setSize(w, h, false);
    this.camera.aspect = w / h;
    this.camera.updateProjectionMatrix();
    this.input.setSize(w, h);
  }

  start() {
    this.clock.start();
    this._raf = requestAnimationFrame(this._bound);
  }

  _tick() {
    this._raf = requestAnimationFrame(this._bound);
    if (this.paused) return;
    const dt = Math.min(0.05, this.clock.getDelta());
    this._frames++;
    this._fpsT += dt;
    if (this._fpsT >= 0.5) {
      this.fps = this._frames / this._fpsT;
      this._frames = 0;
      this._fpsT = 0;
      this.bus.emit('engine:fps', this.fps);
    }
    if (this.cameraController) this.cameraController.update(dt);
    if (this.editor) this.editor.update(dt);
    this.renderer.render(this.scene, this.camera);
    this.input.endFrame();
  }
}
