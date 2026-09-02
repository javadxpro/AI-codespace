import { t } from '../i18n.js';
import { TouchControls, bindHoldButton } from './TouchControls.js';

export class HUD {
  constructor({ bus, editor, world, storage, engine }) {
    this.bus = bus;
    this.editor = editor;
    this.world = world;
    this.storage = storage;
    this.engine = engine;
    this.settings = Object.assign(
      { sensitivity: 1, invertY: false, quality: engine.quality },
      storage.loadSettings(),
    );
    this._applySettings();
    this._els = {
      modes: document.getElementById('modes'),
      worldName: document.getElementById('world-name'),
      btnCam: document.getElementById('btn-cam'),
      btnMenu: document.getElementById('btn-menu'),
      toast: document.getElementById('toast'),
      toolsBuild: document.getElementById('tools-build'),
      toolsTerrain: document.getElementById('tools-terrain'),
      toolsSelect: document.getElementById('tools-select'),
      toolsObserve: document.getElementById('tools-observe'),
      action: document.getElementById('btn-action'),
      jump: document.getElementById('btn-jump'),
      fps: document.getElementById('fps'),
      overlay: document.getElementById('overlay'),
      overlayBody: document.getElementById('overlay-body'),
      file: document.getElementById('file-input'),
      radius: document.getElementById('radius'),
      strength: document.getElementById('strength'),
      radiusVal: document.getElementById('radius-val'),
      strengthVal: document.getElementById('strength-val'),
      color: document.getElementById('color-input'),
      snap: document.getElementById('btn-snap'),
      boot: document.getElementById('boot'),
    };

    new TouchControls(document.getElementById('joystick'), engine.input);
    bindHoldButton(this._els.action, engine.input, 'actionHeld', 'actionPressed', 'actionReleased');
    bindHoldButton(this._els.jump, engine.input, 'jumpHeld', 'jumpPressed', 'jumpReleased');

    this._bindChrome();
    this._bindTools();
    this._bindKeys();
    this.setMode('observe');
    this._els.worldName.value = world.name;

    bus.on('mode:change', (m) => this.setMode(m, true));
    bus.on('selection:change', () => this._refreshSelect());
    bus.on('toast', (key) => this.toast(t[key] || key));
    bus.on('engine:fps', (fps) => {
      this._els.fps.textContent = `${Math.round(fps)} ${t.fps}`;
    });
    bus.on('world:loaded', () => {
      this._els.worldName.value = this.world.name;
    });
  }

  hideBoot() {
    this._els.boot.classList.add('hide');
  }

  toast(msg) {
    const el = this._els.toast;
    el.textContent = msg;
    el.classList.add('show');
    clearTimeout(this._toastTimer);
    this._toastTimer = setTimeout(() => el.classList.remove('show'), 1800);
  }

  _applySettings() {
    this.engine.input.settingsSensitivity = Number(this.settings.sensitivity) || 1;
    this.engine.input.invertY = !!this.settings.invertY;
    if (this.settings.quality && this.settings.quality !== this.engine.quality) {
      this.engine.setQuality(this.settings.quality);
    }
    this.storage.saveSettings(this.settings);
  }

  setMode(mode, fromEditor) {
    if (!fromEditor && this.editor.mode !== mode) this.editor.setMode(mode);
    for (const btn of this._els.modes.querySelectorAll('[data-mode]')) {
      btn.classList.toggle('active', btn.dataset.mode === mode);
    }
    this._els.toolsBuild.classList.toggle('hidden', mode !== 'build');
    this._els.toolsTerrain.classList.toggle('hidden', mode !== 'terrain');
    this._els.toolsSelect.classList.toggle('hidden', mode !== 'select');
    this._els.toolsObserve.classList.toggle('hidden', mode !== 'observe');
    this._els.action.classList.toggle('hidden', mode === 'observe' || mode === 'select');
    this._els.action.textContent = mode === 'terrain' ? '◉' : '+';
    this._els.action.setAttribute('aria-label', mode === 'terrain' ? t.raise : t.place);
    document.body.dataset.mode = mode;
    this._refreshSelect();
  }

  _bindChrome() {
    this._els.modes.addEventListener('click', (e) => {
      const btn = e.target.closest('[data-mode]');
      if (btn) this.editor.setMode(btn.dataset.mode);
    });
    this._els.btnCam.textContent = this.world.thirdPerson ? t.thirdPerson : t.firstPerson;
    this._els.worldName.addEventListener('change', () => {
      this.world.name = this._els.worldName.value.trim() || 'جهان';
    });
    this._els.btnCam.addEventListener('click', () => {
      const third = this.engine.cameraController.toggle();
      this._els.btnCam.textContent = third ? t.thirdPerson : t.firstPerson;
    });
    this._els.btnMenu.addEventListener('click', () => this._openMenu());
    document.getElementById('overlay-close').addEventListener('click', () => this._closeOverlay());
    this._els.overlay.addEventListener('click', (e) => {
      if (e.target === this._els.overlay) this._closeOverlay();
    });
    this._els.file.addEventListener('change', async () => {
      const file = this._els.file.files[0];
      this._els.file.value = '';
      if (!file) return;
      try {
        const data = await this.storage.fromFile(file);
        this.world.deserialize(data);
        this.editor.history.clear();
        this.editor.select(null);
        this.toast(t.loaded);
      } catch {
        this.toast('فایل نامعتبر است');
      }
    });
  }

  _bindTools() {
    this._els.toolsBuild.querySelectorAll('[data-prim]').forEach((b) => {
      b.addEventListener('click', () => {
        this.editor.setPrimitive(b.dataset.prim);
        this._els.toolsBuild.querySelectorAll('[data-prim]').forEach((x) => x.classList.toggle('active', x === b));
      });
    });
    document.querySelectorAll('[data-mat]').forEach((b) => {
      b.addEventListener('click', () => {
        this.editor.setMaterial(b.dataset.mat);
        document.querySelectorAll('[data-mat]').forEach((x) => x.classList.toggle('active', x.dataset.mat === b.dataset.mat));
      });
    });
    document.querySelectorAll('[data-brush]').forEach((b) => {
      b.addEventListener('click', () => {
        this.editor.brush = b.dataset.brush;
        document.querySelectorAll('[data-brush]').forEach((x) => x.classList.toggle('active', x === b));
      });
    });
    document.querySelectorAll('[data-xform]').forEach((b) => {
      b.addEventListener('click', () => {
        this.editor.transformMode = b.dataset.xform;
        document.querySelectorAll('[data-xform]').forEach((x) => x.classList.toggle('active', x === b));
      });
    });
    const onColor = (hex) => {
      this.editor.setColor(hex);
      this._els.color.value = hex;
      const c2 = document.getElementById('color-select');
      if (c2) c2.value = hex;
    };
    this._els.color.addEventListener('input', () => onColor(this._els.color.value));
    document.getElementById('color-select').addEventListener('input', (e) => onColor(e.target.value));
    document.getElementById('presets').addEventListener('click', (e) => {
      const hex = e.target.dataset.hex;
      if (!hex) return;
      onColor(hex);
    });
    this._els.radius.addEventListener('input', () => {
      this.editor.brushRadius = Number(this._els.radius.value);
      this._els.radiusVal.textContent = Number(this._els.radius.value).toFixed(1);
    });
    this._els.strength.addEventListener('input', () => {
      this.editor.brushStrength = Number(this._els.strength.value);
      this._els.strengthVal.textContent = Number(this._els.strength.value).toFixed(2);
    });
    this._els.snap.addEventListener('click', () => {
      this.editor.snap = !this.editor.snap;
      this._els.snap.classList.toggle('active', this.editor.snap);
    });
    document.getElementById('btn-undo').addEventListener('click', () => this.editor.history.undo());
    document.getElementById('btn-redo').addEventListener('click', () => this.editor.history.redo());
    document.getElementById('btn-delete').addEventListener('click', () => this.editor.removeSelected());
    document.getElementById('btn-dup').addEventListener('click', () => this.editor.duplicateSelected());
    document.getElementById('nudge').addEventListener('click', (e) => {
      const n = e.target.closest('[data-nudge]')?.dataset.nudge;
      if (!n) return;
      const [kind, dir] = n.split(':');
      if (kind === 'move') this.editor.nudge('move', dir);
      else if (kind === 'rot') this.editor.nudge('rotate', dir === '-' ? -1 : 1);
      else if (kind === 'scl') this.editor.nudge('scale', dir === '-' ? -1 : 1);
    });
  }

  _bindKeys() {
    window.addEventListener('keydown', (e) => {
      const tag = (e.target && e.target.tagName) || '';
      if (tag === 'INPUT' || tag === 'TEXTAREA') {
        if (e.code === 'Escape') e.target.blur();
        return;
      }
      if (e.code === 'Digit1') this.editor.setMode('observe');
      if (e.code === 'Digit2') this.editor.setMode('build');
      if (e.code === 'Digit3') this.editor.setMode('terrain');
      if (e.code === 'Digit4') this.editor.setMode('select');
      if (e.code === 'KeyF') this._els.btnCam.click();
      if (e.code === 'KeyG') this._els.snap.click();
      if (e.code === 'Delete' || e.code === 'Backspace') this.editor.removeSelected();
      if ((e.ctrlKey || e.metaKey) && e.code === 'KeyZ') {
        e.preventDefault();
        if (e.shiftKey) this.editor.history.redo();
        else this.editor.history.undo();
      }
      if ((e.ctrlKey || e.metaKey) && e.code === 'KeyY') {
        e.preventDefault();
        this.editor.history.redo();
      }
      if ((e.ctrlKey || e.metaKey) && e.code === 'KeyS') {
        e.preventDefault();
        this._save();
      }
      if (e.code === 'KeyQ') this.editor.nudge('rotate', -1);
      if (e.code === 'KeyE') this.editor.nudge('rotate', 1);
    });
  }

  _refreshSelect() {
    const has = !!this.editor.selectedId;
    document.getElementById('select-empty').classList.toggle('hidden', has);
    document.getElementById('select-ops').classList.toggle('hidden', !has);
  }

  _save() {
    this.storage.save(this.world);
    this.toast(t.saved);
  }

  _closeOverlay() {
    this._els.overlay.classList.add('hidden');
    this._els.overlayBody.innerHTML = '';
  }

  _openOverlay(title, html) {
    document.getElementById('overlay-title').textContent = title;
    this._els.overlayBody.innerHTML = html;
    this._els.overlay.classList.remove('hidden');
  }

  _openMenu() {
    this._openOverlay(
      t.menu,
      `<div class="menu-grid">
        <button data-act="save">${t.save}</button>
        <button data-act="load">${t.load}</button>
        <button data-act="new">${t.newWorld}</button>
        <button data-act="download">${t.download}</button>
        <button data-act="upload">${t.upload}</button>
        <button data-act="help">${t.help}</button>
        <button data-act="settings">${t.settings}</button>
      </div>`,
    );
    this._els.overlayBody.querySelector('.menu-grid').addEventListener('click', (e) => {
      const act = e.target.dataset.act;
      if (!act) return;
      if (act === 'save') { this._save(); this._closeOverlay(); }
      if (act === 'load') {
        const data = this.storage.load();
        if (data) {
          this.world.deserialize(data);
          this.editor.history.clear();
          this.toast(t.loaded);
        } else this.toast('ذخیره‌ای پیدا نشد');
        this._closeOverlay();
      }
      if (act === 'new') {
        if (confirm(t.confirmNew)) {
          this.world.createDefault();
          this.editor.history.clear();
          this.editor.select(null);
          this._els.worldName.value = this.world.name;
          this.toast(t.newCreated);
        }
        this._closeOverlay();
      }
      if (act === 'download') { this.storage.download(this.world); this._closeOverlay(); }
      if (act === 'upload') { this._closeOverlay(); this._els.file.click(); }
      if (act === 'help') this._openHelp();
      if (act === 'settings') this._openSettings();
    });
  }

  _openHelp() {
    this._openOverlay(t.help, `<pre class="help">${t.helpBody}</pre>`);
  }

  _openSettings() {
    const s = this.settings;
    this._openOverlay(
      t.settings,
      `<label class="field">${t.sensitivity}
        <input id="set-sens" type="range" min="0.4" max="2.2" step="0.05" value="${s.sensitivity}">
      </label>
      <label class="check"><input id="set-inv" type="checkbox" ${s.invertY ? 'checked' : ''}> ${t.invertY}</label>
      <p class="field-label">${t.quality}</p>
      <div class="row" id="set-q">
        <button data-q="low" class="${s.quality === 'low' ? 'active' : ''}">${t.qualityLow}</button>
        <button data-q="medium" class="${s.quality === 'medium' ? 'active' : ''}">${t.qualityMed}</button>
        <button data-q="high" class="${s.quality === 'high' ? 'active' : ''}">${t.qualityHigh}</button>
      </div>`,
    );
    this._els.overlayBody.querySelector('#set-sens').addEventListener('input', (e) => {
      this.settings.sensitivity = Number(e.target.value);
      this._applySettings();
    });
    this._els.overlayBody.querySelector('#set-inv').addEventListener('change', (e) => {
      this.settings.invertY = e.target.checked;
      this._applySettings();
    });
    this._els.overlayBody.querySelector('#set-q').addEventListener('click', (e) => {
      const q = e.target.dataset.q;
      if (!q) return;
      this.settings.quality = q;
      this.engine.setQuality(q);
      this._applySettings();
      this._els.overlayBody.querySelectorAll('[data-q]').forEach((b) => b.classList.toggle('active', b.dataset.q === q));
    });
  }
}
