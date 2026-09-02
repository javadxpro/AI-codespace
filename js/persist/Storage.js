import { CONFIG } from '../config.js';
import { downloadText, readFileText } from '../core/utils.js';

const CURRENT = 'kimiya.world.current';
const SETTINGS = 'kimiya.settings';

export class Storage {
  load() {
    try {
      const raw = localStorage.getItem(CURRENT);
      if (!raw) return null;
      const data = JSON.parse(raw);
      if (data.format !== CONFIG.format) return null;
      return data;
    } catch {
      return null;
    }
  }

  save(world) {
    const obj = world.serializeObject();
    localStorage.setItem(CURRENT, JSON.stringify(obj));
    return obj;
  }

  download(world) {
    const name = (world.name || 'kimiya-world').replace(/[^\w\u0600-\u06FF\- ]+/g, '');
    downloadText(`${name}.kimiya.json`, world.serialize());
  }

  async fromFile(file) {
    const text = await readFileText(file);
    const data = JSON.parse(text);
    if (data.format !== CONFIG.format) throw new Error('invalid');
    return data;
  }

  loadSettings() {
    try {
      return JSON.parse(localStorage.getItem(SETTINGS) || '{}');
    } catch {
      return {};
    }
  }

  saveSettings(s) {
    localStorage.setItem(SETTINGS, JSON.stringify(s));
  }
}
