# KIMIA on the web — WebGL2 + WebAssembly (Emscripten)

KIMIA runs in the browser as real client-side WebGL2/WASM: the whole engine
(physics, world logic, and the **GL renderer** — Cook-Torrance PBR, point
lights, fog, filmic tone mapping) compiles to WebAssembly and draws straight
onto a WebGL2 canvas. No server-side frame rendering, no compromise on the
graphics quality of the desktop build.

This is **not** the WebViewer. The WebViewer is the headless/PS4 path where a
server rasterises frames and streams them; the Emscripten build is the real
WebGL route where the game itself runs in the user's browser.

## Prerequisites

- **Emscripten SDK** (`emsdk`). Install and activate it once:

  ```sh
  git clone https://github.com/emscripten-core/emsdk.git
  cd emsdk
  ./emsdk install latest
  ./emsdk activate latest
  ```

- Activate the toolchain in **every new shell** before building:

  ```sh
  # Linux / macOS
  source /path/to/emsdk/emsdk_env.sh

  # Windows (PowerShell / cmd)
  C:\path\to\emsdk\emsdk_env.bat
  ```

  This puts `emcc`/`em++` on `PATH` and sets `EMSDK`, which the CMake preset
  reads.

- CMake **3.25+** (already required for the PC build).

## Build

```sh
cmake --preset webgl-release
cmake --build --preset webgl-release --parallel
```

The output is `out/build/webgl-release/kimia_webgl.html` (+ `.js`/`.wasm`).

## Run

WebAssembly needs to be served over HTTP (browsers block `file://` WASM and,
for `ALLOW_MEMORY_GROWTH`, same-origin fetch). Any static server works:

```sh
# Emscripten ships one:
emrun out/build/webgl-release/kimia_webgl.html

# or, from the build directory, plain Python:
python3 -m http.server 8000
# then open http://localhost:8000/kimia_webgl.html
```

## How it works

| Concern | Implementation |
| --- | --- |
| GL entry points | `GLFunctions` wires the WebGL2/GLES3 symbols directly under `__EMSCRIPTEN__` (no `dlopen`); `glBufferData`'s pointer-width size and `glClearDepthf` get ABI shims. |
| Shaders | `Shaders.cpp` emits the same shader body with `#version 300 es` + fragment `precision` on Emscripten and `#version 330 core` on desktop GL. |
| Context | `EGLContext` creates a WebGL2 context on the page's `<canvas id="canvas">` via `emscripten_webgl_create_context` instead of `dlopen("libEGL.so")`. |
| App | `Examples/WebGLApp.cpp` boots the renderer and drives it with `emscripten_set_main_loop`. |
| Shell | `Web/webgl-shell.html` provides the canvas, the charcoal brand theme, and device-pixel-aware resizing. |

The colour pipeline (ACES tone map + exact sRGB) and the BRDF are the exact
same code the desktop GL and software rasterisers run, so a frame looks
identical across backends — the same guarantee the rest of the engine makes.

## Notes / roadmap

- **Emscripten build is not part of the sandbox CI**: the prebuilt SDK
  downloads from `storage.googleapis.com`, which the build sandbox cannot
  reach, so the WebGL target is validated by building on a machine with a
  working `emsdk`. The native PS4/Linux build and the 475-test suite remain
  the continuously-verified baseline.
- The showcase scene in `WebGLApp.cpp` is a smoke-test of the full pipeline
  (metal, dielectric, emissive, point light, fog). Wiring the interactive
  world editor (`WorldEditorApp`) — profile loading via `--preload-file`,
  pointer/touch input through the Emscripten HTML5 API, and audio — is the
  next step.
- WebGL2 is required (WebGL1 lacks the engine's GL 3.3 feature set: VAOs,
  GLSL 300 es, depth-comparison shadow samplers). Devices without WebGL2
  fall back to the WASM software rasteriser, the same dependable path the
  PS4 headless build uses.
