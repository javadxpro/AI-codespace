import { EventBus } from './core/EventBus.js';
import { Engine } from './engine/Engine.js';
import { World } from './world/World.js';
import { Editor } from './editor/Editor.js';
import { History } from './editor/History.js';
import { Storage } from './persist/Storage.js';
import { HUD } from './ui/HUD.js';
import { CONFIG } from './config.js';
import { t } from './i18n.js';

function bootError(msg) {
  const boot = document.getElementById('boot');
  if (!boot) return;
  boot.innerHTML = `<div class="boot-card"><h1>کیمیا</h1><p>${msg}</p></div>`;
}

async function main() {
  const canvas = document.getElementById('view');
  const bus = new EventBus();
  const engine = new Engine(canvas, bus);
  const world = new World(engine.scene, bus);
  const storage = new Storage();
  const saved = storage.load();
  if (saved) {
    try { world.deserialize(saved); }
    catch { world.createDefault(); }
  } else {
    world.createDefault();
  }

  const history = new History(bus);
  const editor = new Editor({ engine, world, bus, history });
  engine.bind(world, editor);
  const hud = new HUD({ bus, editor, world, storage, engine });

  editor.setMode('observe');
  engine.start();
  requestAnimationFrame(() => hud.hideBoot());

  if (saved) hud.toast(t.loaded);
  else hud.toast(t.welcome);

  setInterval(() => {
    try { storage.save(world); } catch (_) { /* quota */ }
  }, CONFIG.autosaveMs);

  window.addEventListener('beforeunload', () => {
    try { storage.save(world); } catch (_) { /* ignore */ }
  });
}

window.addEventListener('error', (e) => bootError(e.message || 'خطای ناشناخته'));
window.addEventListener('unhandledrejection', (e) => bootError(String(e.reason || e)));

if (!window.WebGLRenderingContext) bootError('این مرورگر WebGL ندارد.');
else main();
