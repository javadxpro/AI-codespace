import { CONFIG } from '../config.js';
import { isTouchDevice } from '../core/utils.js';

/**
 * Unified keyboard / mouse / touch input.
 * HUD joystick writes into `joystick`; canvas writes look + pointer.
 */
export class InputManager {
  constructor() {
    this.keys = Object.create(null);
    this.pointer = {
      x: 0,
      y: 0,
      ndcX: 0,
      ndcY: 0,
      down: false,
      started: false,
      pressed: false,
      released: false,
      button: 0,
      isDrag: false,
      drag: 0,
    };
    this.look = { dx: 0, dy: 0 };
    this.joystick = { x: 0, y: 0 };
    this.pointerDx = 0;
    this.pointerDy = 0;
    this.actionHeld = false;
    this.actionPressed = false;
    this.actionReleased = false;
    this.jumpPressed = false;
    this.primaryLooks = true;
    this.lookSensitivity = CONFIG.lookSensitivity;
    this.settingsSensitivity = 1;
    this.invertY = false;
    this.isTouch = isTouchDevice();
    this.sprint = false;
    this._w = 1;
    this._h = 1;
    this._lookId = null;
    this._canvasId = null;
    this._lastX = 0;
    this._lastY = 0;
    this._downX = 0;
    this._downY = 0;
  }

  setSize(w, h) {
    this._w = w;
    this._h = h;
  }

  attach(canvas) {
    this.canvas = canvas;
    window.addEventListener('keydown', this._onKeyDown);
    window.addEventListener('keyup', this._onKeyUp);
    canvas.addEventListener('pointerdown', this._onDown);
    window.addEventListener('pointermove', this._onMove);
    window.addEventListener('pointerup', this._onUp);
    window.addEventListener('pointercancel', this._onUp);
    canvas.addEventListener('contextmenu', (e) => e.preventDefault());
    canvas.addEventListener('wheel', (e) => e.preventDefault(), { passive: false });
  }

  _onKeyDown = (e) => {
    const tag = (e.target && e.target.tagName) || '';
    if (tag === 'INPUT' || tag === 'TEXTAREA') return;
    this.keys[e.code] = true;
    if (e.code === 'Space') {
      this.jumpPressed = true;
      e.preventDefault();
    }
    if (e.code === 'ShiftLeft' || e.code === 'ShiftRight') this.sprint = true;
    if ((e.ctrlKey || e.metaKey) && e.code === 'KeyS') e.preventDefault();
  };

  _onKeyUp = (e) => {
    this.keys[e.code] = false;
    if (e.code === 'ShiftLeft' || e.code === 'ShiftRight') this.sprint = false;
  };

  _updateNdc(x, y) {
    this.pointer.x = x;
    this.pointer.y = y;
    this.pointer.ndcX = (x / this._w) * 2 - 1;
    this.pointer.ndcY = -(y / this._h) * 2 + 1;
  }

  _onDown = (e) => {
    if (e.target !== this.canvas) return;
    this._updateNdc(e.clientX, e.clientY);
    this.pointer.down = true;
    this.pointer.started = true;
    this.pointer.button = e.button;
    this.pointer.isDrag = false;
    this.pointer.drag = 0;
    this._canvasId = e.pointerId;
    this._downX = e.clientX;
    this._downY = e.clientY;
    this._lastX = e.clientX;
    this._lastY = e.clientY;
    const wantsLook =
      e.pointerType === 'touch' ||
      e.button === 2 ||
      (e.button === 0 && this.primaryLooks);
    if (wantsLook && this._lookId == null) {
      this._lookId = e.pointerId;
      try { this.canvas.setPointerCapture(e.pointerId); } catch (_) { /* ignore */ }
    }
    e.preventDefault();
  };

  _onMove = (e) => {
    this._updateNdc(e.clientX, e.clientY);
    if (!this.pointer.down) return;
    const dx = e.clientX - this._lastX;
    const dy = e.clientY - this._lastY;
    this._lastX = e.clientX;
    this._lastY = e.clientY;
    this.pointer.drag += Math.hypot(dx, dy);
    if (this.pointer.drag > 8) this.pointer.isDrag = true;
    this.pointerDx += dx;
    this.pointerDy += dy;
    if (this._lookId === e.pointerId) {
      this.look.dx += dx;
      this.look.dy += dy;
    }
  };

  _onUp = (e) => {
    if (this._lookId === e.pointerId) this._lookId = null;
    if (this._canvasId !== e.pointerId) return;
    this._canvasId = null;
    if (!this.pointer.down) return;
    const wasDown = this.pointer.down;
    this.pointer.down = false;
    if (wasDown && !this.pointer.isDrag && this.pointer.button === 0) {
      this.pointer.pressed = true;
    }
    this.pointer.released = true;
  };

  moveVector() {
    let x = this.joystick.x;
    let z = this.joystick.y;
    if (this.keys.KeyD || this.keys.ArrowRight) x += 1;
    if (this.keys.KeyA || this.keys.ArrowLeft) x -= 1;
    if (this.keys.KeyW || this.keys.ArrowUp) z += 1;
    if (this.keys.KeyS || this.keys.ArrowDown) z -= 1;
    const len = Math.hypot(x, z);
    if (len > 1) { x /= len; z /= len; }
    return { x, z, sprint: this.sprint || len > 0.92 };
  }

  interactHeld() {
    if (this.isTouch) return this.actionHeld;
    return this.pointer.down && this.pointer.button === 0 && !this.primaryLooks;
  }

  interactPressed() {
    if (this.isTouch) return this.actionPressed;
    return this.pointer.pressed && this.pointer.button === 0 && !this.primaryLooks;
  }

  endFrame() {
    this.pointer.pressed = false;
    this.pointer.started = false;
    this.pointer.released = false;
    this.look.dx = 0;
    this.look.dy = 0;
    this.pointerDx = 0;
    this.pointerDy = 0;
    this.actionPressed = false;
    this.actionReleased = false;
    this.jumpPressed = false;
  }
}
