# Handoff Prompt — KIMIA Engine, rebuild-from-scratch edition
(paste everything below this line into the new AI)

## 1. Your mission

You are rebuilding **KIMIA** — a from-scratch C++17 game engine + game
ecosystem — **by creating every file yourself in the GitHub repository you are
connected to.** You will NOT receive a snapshot or a zip; you create all files
from this specification. This is a faithful rebuild of a working engine whose
previous version passed 118/118 tests — so the bar is "rebuild it *without
bugs*", not "invent something similar".

**About the 25 MB file limit: it is a non-issue here.** The entire project is
≈1.1 MB of source (~20,600 lines total). The largest single file is the
vendored `stb_image.h` (280 KB). Keep every file small and modular anyway
(that is the house style): nothing above ~700 lines of source.

After the engine rebuild is green, your **real task** begins: **KIMIA World**,
an option-driven Unity-like editor (section 8).

## 2. Who I am (the user) — non-negotiable rules

- I speak **Persian** — reply to me in Persian, always.
- I run on an **Android phone in Termux** (weak GPU → software rendering) and
  sometimes Ubuntu. I paste terminal output back to you. Always give exact,
  copy-pasteable commands.
- **I author game content, never you.** No hardcoded courses/levels/demos
  written by the AI — build tools that let *me* make content.
- No explain-only answers, no pseudocode, no fake/placeholder files, no "TODO
  for later" on anything you can finish now.
- **Zero-warning policy:** the tree must compile with `-Werror` AND with
  `-Werror` off. Fix anything that is "only a warning" — I explicitly demand
  this.
- FOSS deps only: `stb` (vendored stb_image / stb_image_write), SDL2 optional
  (`-DKIMIA_ENABLE_SDL2=ON|OFF`), zlib via PNG encoding.
- Small, staged **conventional commits** (`feat(core):`, `fix(physics):`,
  `docs(...)`). Docs for every shipped system in `Documentation/`.

## 3. HOW to build it bug-free (the method — follow it literally)

1. **Build bottom-up in this exact order**, one module at a time:
   `Core → Math → Graphics → Scene → Physics → Renderer → Platform/App
   → Examples/Golf → WebViewer → docs`.
2. **Per module:** write headers, then implementation, then that module's
   tests. Compile with `-DKIMIA_WERROR=ON`. Run the full test binary.
   **Never start the next module while any test is red or any warning exists.**
   Commit each green module as its own staged commit.
3. **Test the behaviors, not the lines.** For each module the required test
   coverage is listed in section 5. A clean exit code with wrong output is a
   failure — assert real values (e.g. a specific matrix element to 4 decimals,
   a sphere's exact vertex/index counts, a physics step's exact position).
4. **Verify with the real toolchain, then paste the real output** to me
   (compile log tail + test counts). If something fails, paste the actual
   error — never paraphrase.
5. **Double-gate every build:** once with `-DKIMIA_WERROR=ON` and once with a
   plain `-Werror`-free configure; grep both logs for `warning` — the count
   must be zero.
6. Beware the traps that bit the previous build (all real, all fixed once —
   don't re-introduce them):
   - Normal matrix = **inverse-transpose** of the model matrix (not the model
     matrix) when scale is non-uniform.
   - Cube mesh winding: outward-facing triangles, CCW from outside; the
     correct counts are **24 vertices / 36 indices** (4 verts per face, no
     sharing, for correct normals). Plane = 4v/6i. Sphere (rings×segments
     8×12 style) = **153 v / 768 i**, outward CCW.
   - Fixed-timestep physics must **cap the accumulator** (e.g. max 5 steps)
     or a slow frame spirals; and the same sim must be stable at both 60 Hz
     and 120 Hz host rates (test both — a phase-lock bug once flipped a
     velocity sign at 120 Hz).
   - `-Werror=shadow` is on: lambdas inside functions must not reuse outer
     parameter names.
   - `std::mutex` is not movable — hold it via pointer or make the owning
     class non-copyable/non-movable.
   - Format strings: `%f` needs `static_cast<double>` on floats under
     `-Wdouble-promotion`.
   - Web input semantics: **held keys drain as a LEVEL snapshot** (the
     browser only sends changes; server keeps state), **taps drain as EDGES**
     (cleared after read). Getting this backwards makes buttons stick or
     disappear.
   - In pipelines, `grep -i error` can hide "command not found" — always look
     at the real tail of build output, not just a grep.

## 4. Repository blueprint (create exactly this tree)

```
CMakeLists.txt                  # top level: C++17, options KIMIA_WERROR (default ON),
                                # KIMIA_ENABLE_SDL2 (auto-detect), add_subdirectory for all
Engine/Core                     # Types.h (i32/u32/f32/f64 aliases), Log.h/.cpp (levels,
                                # file+console), Profiler.h/.cpp (scoped timers, report),
                                # ImageWriter.h/.cpp (PNG via stb_image_write + zlib? no:
                                # stb_image_write directly)
Engine/Math                     # Vec2/3/4, Mat4 (COLUMN-major, right-handed, +Y up,
                                # −Z forward), Quat, Camera (perspective lookAt), MathUtils
Engine/Graphics                 # GraphicsTypes.h (MeshData: positions/normals/uvs/indices),
                                # Image.h/.cpp (load via stb_image), PrimitiveMeshes
                                # (makeCube 24v/36i, makePlane 4v/6i, makeSphere 153v/768i)
Engine/Scene                    # Entity.h (EntityHandle = u32, 1-BASED; 0 = null),
                                # Scene.h/.cpp (create/destroy/get, Transform, name,
                                # mesh/color/rough refs, forEach(cb(handle, const EntityData&))),
                                # SceneIO.h/.cpp — text serialization v1 (format below)
Engine/Physics                  # PhysicsWorld: fixed dt = 1/120 s, gravity −9.81 +Y,
                                # dynamic spheres vs static planes (y = const) and static
                                # AABB boxes; restitution, friction, rolling friction;
                                # accumulator with 5-step cap
Engine/Renderer                 # GLFunctions.h/.cpp (dlopen/GetProcAddress-style GL 3.3
                                # core loader — the future Vulkan swap point),
                                # Shader, Mesh, Texture, Shaders (Phong + gamma + key-light
                                # shadow map pass), Renderer.h/.cpp (draw scene, shadows,
                                # capturePNG via stb_image_write)
Engine/Platform                 # Key.h (enum: letters, Num1..Num0, arrows, Return, Space,
                                # Shift...), InputState.h/.cpp (down/pressed/released),
                                # Window/SDLWindow (hidden-window mode supported)
Engine/App                      # Time.h/.cpp (fixed-step helper with accumulator cap),
                                # Engine.h/.cpp (owns window, GL context, inputBackend(),
                                # initialize() sets XDG_RUNTIME_DIR=/tmp/kimia-xdg when
                                # unset — mkdir 0700 — and on __ANDROID__ sets
                                # LIBGL_ALWAYS_SOFTWARE=1 if unset; both no-overwrite),
                                # WebViewer.h/.cpp — see section 6
Examples/HelloWindow            # trivial SDL/GL smoke app
Examples/First3DScene           # lit cube+plane+sphere, camera orbit, PNG capture
Examples/RemoteView             # renders a scene headless, serves it via WebViewer
Examples/GolfGame               # kimia_golf — see section 7
Tests                           # harness/kimia_test.h/.cpp (tiny assert framework, no
                                # external dep; main returns pass/fail counts),
                                # src/CoreTests MathTests GraphicsTests SceneTests
                                # PhysicsTests RendererTests WebTests + main.cpp
Documentation                   # Architecture Building Math Physics Renderer SceneSystem
                                # Textures MakingCourses Termux Contributing (.md each)
ThirdParty/stb                  # stb_image.h, stb_image_write.h (vendored, official)
Tools                           # termux_build.sh (+ path scripts) — see section 6
README.md  ROADMAP.md  .gitignore
```

**SceneIO v1 text format** (this exact grammar; a real file that must
round-trip):

```
# KIMIA scene v1
e "Green" mesh plane pos 0 0 0 scale 1 1 1 color 0.22 0.45 0.24 rough 0.95
e "Wall_1" mesh cube pos 2.4 0.5 0 scale 0.5 1 4.4 color 0.7 0.68 0.62 rough 0.5
e "Tee" mesh cube pos 0 0.01 7 scale 0.4 0.02 0.4 color 0.9 0.85 0.3 rough 0.8
e "Hole" mesh cube pos 0 0.01 -7 scale 0.56 0.02 0.56 color 0.05 0.05 0.05 rough 0.8
e "FlagPole" mesh cube pos 0 0.9 -7 scale 0.03 1.8 0.03 color 0.9 0.9 0.9 rough 0.4
e "FlagCloth" mesh cube pos 0.31 1.62 -7 scale 0.6 0.35 0.02 color 0.85 0.15 0.15 rough 0.6
e "Ball" mesh sphere pos 0 0 0 scale 1 1 1 color 0.95 0.95 0.92 rough 0.3
# demo 0.000000 0.610000
```

Rules: `#` lines are comments EXCEPT `# demo <aim> <power>` which the golf
game reads as the player-authored demo shot; unknown tokens are skipped
(tolerant load); mesh ∈ {cube, plane, sphere}.

## 5. Required test coverage (the bug-free contract)

Your suite must cover at least these behaviors (the previous suite had 118
tests full / 102 headless — yours may differ in count, but must cover all of
this and end 100% green, twice: with SDL on and off):

- **Math:** Mat4 multiply/perspective/lookAt values; inverse-transpose normal
  matrix on non-uniform scale; Quat↔Mat4 round-trip; Vec ops.
- **Graphics:** makeCube = 24v/36i with per-face normals; makeSphere =
  153v/768i, all normals unit-length and outward (dot(normal, pos) > 0);
  makePlane = 4v/6i.
- **Scene:** create/destroy/get; handles 1-based, freed handles null;
  forEach sees all; SceneIO **round-trip** (save→load→identical) and
  **tolerant load** (garbage tokens skipped, partial lines ignored).
- **Physics:** sphere falls with gravity (exact y after N steps); restitution
  bounce height; friction decay; sphere-vs-AABB side collision; **stability
  at 60 Hz and 120 Hz host rates with the accumulator cap**.
- **Renderer (headless via EGL/OSMesa or skipped gracefully):** draw + PNG
  capture is non-empty; shadow pass toggles.
- **Web (raw sockets, no browser):** GET `/` returns HTML page; GET
  `/frame.png` is 503 before first publish and 200 (image/png) after; POST
  `/input?tap=x&down=1` then drain → tap edge appears once; held keys drain
  as level (still down on second drain until up).

## 6. WebViewer + Termux (the display route — critical)

`kimia::web::Server` (Engine/App): plain POSIX sockets + threads, zlib only
for PNG (already produced by stb). API: `start(port, pageHtml)`,
`publishFrame(pngBytes, statsLine)`, `DrainedInput drain()` →
`{held, taps, lookX, lookY, zoom}`, `stop()`. Routes: `/` (the control page),
`/frame.png` (503 until first frame), `/stats` (last stats line),
`POST /input?key=<k>&down=0|1` and `POST /input?tap=<k>`, plus
`lookX/lookY/zoom` params. `held` = level state (server-side map updated by
down/up), `taps`/look/zoom = edges cleared on drain. `makePageHtml(title,
padButtons, keymapJs, hint)` generates a touch-pad page.
`stop()` closes the listen fd and sleeps ~50 ms for in-flight handlers.

`Tools/termux_build.sh` must: detect Termux, `pkg install` the toolchain
(clang cmake make ninja zlib x11-repo mesa/sdl2 as needed), repair a broken
cmake package (`pkg uninstall -y cmake && pkg install -y cmake` when
`CMAKE_ROOT` errors appear), configure + build with `-DKIMIA_WERROR=ON`, and
print the exact next commands.

## 7. Golf game spec (Examples/GolfGame — the reference game)

- Modes: EDIT ⇄ AIM → CHARGE → ROLL → SUNK/OUT, stroke counter.
- Ball: radius .12, restitution .40, friction .40, rolling friction .22;
  launch speed `2.5 + power*13.5`; cup capture when horizontal dist < 0.28
  and speed < 5.0; default tee (0,·,7), hole (0,·,−7).
- CLI: `--edit`, `--course <file>`, `--demo` (replay the `# demo` line),
  `--web [--port N]` (default 8080, hidden window, 30 fps), `--lowfx`,
  `--headless-demo`.
- Builder: tools wall/tee/hole (keys 1/2/3), move ghost (WASD/arrows, Shift =
  fine 0.1 step), Q/E wall length 2..12, R axis (Z/X), Enter place,
  U undo, S save (`my_course.kimia` in cwd unless `--course`), L load,
  F toggle play/edit. **Any builder key pressed in play mode auto-switches
  to EDIT and acts** (previous users reported "buttons don't work" when they
  were silently mode-locked — never do silent no-ops again).
- Web pad (16 buttons): holds a/d/space (aim left/right, SHOOT charge);
  taps 1/2/3, Enter, q/e, r, u, s, l, f, quit. JS keymap:
  `{a:'h:a',d:'h:d',' ':'h:space',Enter:'t:return',q:'t:q',e:'t:e',r:'t:r',u:'t:u',s:'t:s',l:'t:l',f:'t:f','1':'t:num1','2':'t:num2','3':'t:num3'}`
- `/stats` line (every button must be observable in it):
  `KIMIA GOLF | <EDIT|AIM|CHARGE|ROLL|SUNK> | stroke N | power P% | tool <wall|tee|hole> len <L><X|Z> | walls <W>`

## 8. THE MAIN TASK after the rebuild is green: KIMIA World

My own words: I want a complete engine-editor **like Unity** where I **write
no code and know no keys** — everything is menus and options. Example: I add
a ball and it asks **«دقیق باشه یا فانتزی؟»** (accurate or fantasy?) and the
answer changes real behavior. My acceptance test:

**Create Project → Open KIMIA World → Create World → Add Player → Add Ball →
Add Environment → Press PLAY → I am playing a game I just made.**

Requirements: option-driven object creation (each object asks a few plain
questions whose answers map to real engine components — e.g. fantasy ball =
high bounce, low friction, no roll decay; accurate = the golf tuning above);
UI runs through WebViewer on port 8080 as tappable buttons + status line
(terminal menu acceptable as secondary); worlds save/load as SceneIO-v1-
compatible text (old `.kimia` files must still load); PLAY launches a
playable simulation. Same conventions: tests, zero warnings, docs, staged
commits, real builds with pasted output. First milestone: Create World → Add
Player → Add Ball (with the accurate/fantasy question) → PLAY in the browser.

## 9. Before your first commit and before every "done"

Run and paste:

```bash
cmake -B build -DKIMIA_WERROR=ON && cmake --build build -j4 2>&1 | tail -5
cmake -B build-warn -DKIMIA_WERROR=OFF && cmake --build build-warn -j4 2>&1 | grep -c warning   # must print 0
./build/bin/kimia_tests | tail -1                          # all passed
cmake -B build-headless -DKIMIA_ENABLE_SDL2=OFF && cmake --build build-headless -j4
./build-headless/bin/kimia_tests | tail -1                 # all passed
./build/bin/kimia_golf --web --edit --lowfx --port 8080 &  # then:
curl -s http://localhost:8080/stats
curl -s -X POST http://localhost:8080/input?tap=return && sleep 0.5 && curl -s http://localhost:8080/stats   # walls +1
```

A change is not "done" until the command that exercises it has actually run
and its real output was shown. Reply in Persian.
