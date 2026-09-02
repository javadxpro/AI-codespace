export class History {
  constructor(bus, limit = 70) {
    this.bus = bus;
    this.limit = limit;
    this.stack = [];
    this.index = -1;
  }

  push(cmd) {
    this.stack = this.stack.slice(0, this.index + 1);
    this.stack.push(cmd);
    if (this.stack.length > this.limit) this.stack.shift();
    else this.index = this.stack.length - 1;
    this.index = this.stack.length - 1;
    this._emit();
  }

  undo() {
    if (this.index < 0) return false;
    this.stack[this.index].undo();
    this.index--;
    this._emit();
    return true;
  }

  redo() {
    if (this.index >= this.stack.length - 1) return false;
    this.index++;
    this.stack[this.index].redo();
    this._emit();
    return true;
  }

  clear() {
    this.stack = [];
    this.index = -1;
    this._emit();
  }

  state() {
    return {
      canUndo: this.index >= 0,
      canRedo: this.index < this.stack.length - 1,
    };
  }

  _emit() {
    this.bus.emit('history:change', this.state());
  }
}
