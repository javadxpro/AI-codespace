# وب (WebGL2 + WebAssembly)

KIMIA در مرورگر به‌صورت **WASM/WebGL2 سمت کلاینت** اجرا می‌شود: کل موتور (فیزیک، منطق دنیا و رندر GL — PBR، نور نقطه‌ای، مه، تون‌مپ فیلمی) به WebAssembly کامپایل می‌شود و مستقیم روی canvas می‌راند. بدون رندر سمت سرور و بدون افت کیفیت نسبت به دسکتاپ.

این **WebViewer** نیست — WebViewer مسیر headless/PS4 است که سرور فریم رستر می‌کند و استریم می‌دهد؛ این بیلد، بازی را واقعاً در مرورگرِ کاربر اجرا می‌کند.

---

## پیش‌نیاز

- **Emscripten SDK** (`emsdk`)، نصب و فعال‌سازی یک‌بار:

```sh
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk && ./emsdk install latest && ./emsdk activate latest
```

- فعال‌سازی در هر شل جدید: `source /path/to/emsdk/emsdk_env.sh` (ویندوز: `emsdk_env.bat`).
- CMake ۳٫۲۵+.

## بیلد و اجرا

```sh
cmake --preset webgl-release
cmake --build --preset webgl-release --parallel
```

خروجی: `out/build/webgl-release/kimia_webgl.html` (+ `.js`/`.wasm`). WASM باید از HTTP سرو شود (مرورگر `file://` را مسدود می‌کند):

```sh
emrun out/build/webgl-release/kimia_webgl.html
# یا: python3 -m http.server 8000 → http://localhost:8000/kimia_webgl.html
```

## چطور کار می‌کند

| بخش | پیاده‌سازی |
| --- | --- |
| ورودی‌های GL | `GLFunctions` نمادهای WebGL2/GLES3 را مستقیم سیم می‌کند (بدون dlopen) + shim اندازه/عمق |
| شیدرها | همان بدنه، `#version 300 es` + precision روی Emscripten |
| کانتکست | `EGLContext` با `emscripten_webgl_create_context` روی `<canvas id="canvas">` |
| برنامه | `Examples/WebGLApp.cpp` با `emscripten_set_main_loop` |
| پوسته | `Web/webgl-shell.html` (canvas + برند + تغییر اندازه آگاه از DPI) |

خط لولهٔ رنگ (تون‌مپ ACES + sRGB) و BRDF همان کدِ GL دسکتاپ و رسترایزر نرم‌افزاری است؛ فریم در هر مسیر یکسان است.

## یادداشت

- بیلد Emscripten در CI/sandbox نیست: SDK از `storage.googleapis.com` دانلود می‌شود که sandbox به آن دسترسی ندارد؛ راستی‌آزمایی روی ماشینی با `emsdk` کار انجام می‌شود. بیلد لینوکس/PS4 و سوئیت ۴۷۵ تستی همچنان خط مبناست.
- WebGL2 لازم است (VAO، GLSL 300 es، sampler سایه با مقایسهٔ عمق). دستگاه بدون WebGL2 به رسترایزر نرم‌افزاری WASM برمی‌گردد.

---
