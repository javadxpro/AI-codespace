export const clamp = (v, a, b) => Math.max(a, Math.min(b, v));
export const lerp = (a, b, t) => a + (b - a) * t;
export const round3 = (n) => Math.round(n * 1000) / 1000;
export const uid = (p = 'e') => `${p}_${Math.random().toString(36).slice(2, 9)}`;

export function isTouchDevice() {
  return navigator.maxTouchPoints > 0 || window.matchMedia('(pointer: coarse)').matches;
}

export function roundTo(n, step) {
  return Math.round(n / step) * step;
}

export function downloadText(filename, text, mime = 'application/json') {
  const blob = new Blob([text], { type: mime });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = filename;
  a.click();
  setTimeout(() => URL.revokeObjectURL(url), 1500);
}

export function readFileText(file) {
  return new Promise((resolve, reject) => {
    const r = new FileReader();
    r.onload = () => resolve(String(r.result));
    r.onerror = () => reject(r.error);
    r.readAsText(file);
  });
}
