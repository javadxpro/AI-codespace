/** Tiny pub/sub so engine, editor and HUD stay decoupled. */
export class EventBus {
  constructor() {
    this._m = new Map();
  }

  on(event, fn) {
    if (!this._m.has(event)) this._m.set(event, new Set());
    this._m.get(event).add(fn);
    return () => this.off(event, fn);
  }

  off(event, fn) {
    this._m.get(event)?.delete(fn);
  }

  emit(event, payload) {
    const set = this._m.get(event);
    if (!set) return;
    for (const fn of set) fn(payload);
  }
}
