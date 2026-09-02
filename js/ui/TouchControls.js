/** On-screen joystick. Writes into InputManager.joystick. */
export class TouchControls {
  constructor(root, input) {
    this.root = root;
    this.knob = root.querySelector('.joystick-knob');
    this.input = input;
    this._id = null;
    this._max = 42;
    root.addEventListener('pointerdown', this._down);
    window.addEventListener('pointermove', this._move);
    window.addEventListener('pointerup', this._up);
    window.addEventListener('pointercancel', this._up);
  }

  _down = (e) => {
    this._id = e.pointerId;
    this.root.classList.add('active');
    try { this.root.setPointerCapture(e.pointerId); } catch (_) { /* ignore */ }
    this._apply(e);
    e.preventDefault();
    e.stopPropagation();
  };

  _move = (e) => {
    if (this._id !== e.pointerId) return;
    this._apply(e);
  };

  _up = (e) => {
    if (this._id !== e.pointerId) return;
    this._id = null;
    this.root.classList.remove('active');
    this.input.joystick.x = 0;
    this.input.joystick.y = 0;
    this.knob.style.transform = 'translate(-50%, -50%)';
  };

  _apply(e) {
    const r = this.root.getBoundingClientRect();
    const cx = r.left + r.width / 2;
    const cy = r.top + r.height / 2;
    let dx = e.clientX - cx;
    let dy = e.clientY - cy;
    const max = r.width * 0.38;
    const len = Math.hypot(dx, dy);
    const k = len > max ? max / len : 1;
    dx *= k;
    dy *= k;
    let x = dx / max;
    let y = -dy / max;
    const dead = 0.12;
    if (Math.hypot(x, y) < dead) { x = 0; y = 0; }
    this.input.joystick.x = x;
    this.input.joystick.y = y;
    this.knob.style.transform = `translate(calc(-50% + ${dx}px), calc(-50% + ${dy}px))`;
  }
}

export function bindHoldButton(el, input, fieldHeld, fieldPressed, fieldReleased) {
  const down = (e) => {
    e.preventDefault();
    e.stopPropagation();
    input[fieldHeld] = true;
    input[fieldPressed] = true;
    el.classList.add('held');
  };
  const up = () => {
    if (!input[fieldHeld]) return;
    input[fieldHeld] = false;
    input[fieldReleased] = true;
    el.classList.remove('held');
  };
  el.addEventListener('pointerdown', down);
  window.addEventListener('pointerup', up);
  window.addEventListener('pointercancel', up);
}
