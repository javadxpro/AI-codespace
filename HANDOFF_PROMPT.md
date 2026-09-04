# Handoff Prompt — KIMIA Engine (paste everything below this line into the new AI)

You are continuing development of **KIMIA**, a from-scratch C++17 game engine and
game ecosystem. A previous AI session built everything described below and
stopped at git commit `9a81b69`. Your job is to continue from **exactly this
state** — nothing is missing, nothing is half-finished, the tree is clean and
all tests pass. Do not restart, do not rewrite what exists, do not "improve"
working systems. Continue the roadmap.

## Who I am (the user)

- I speak **Persian (Farsi)** — every reply to me must be in Persian.
- I build and run on an **Android phone in Termux** (weak GPU, software
  rendering) and sometimes Ubuntu. I download code snapshots, paste terminal
  output back to you. Give me exact reproducible commands.
- I am a hands-on creator: **I author the game content myself, never you.**
  No hardcoded demos, courses, or levels authored by the AI — tools must let
  *me* make them.
- Standing rules from me (never violate):
  - No explain-only answers, no pseudocode, no fake files, no "we'll do it
    later" for something you can do now. Write the real code, build it with
    the real toolchain, run it, and paste real errors/outputs if anything
    fails.
  - Every system ships with tests. Every change keeps the full suite green.
  - Small, staged, conventional commits (`feat(...)`, `fix(...)`, `docs(...)`).
  - Docs for every shipped system live in `Documentation/`.
  - Gameplay/physics/object-management first; **beautification later**.
  - Fix **every** bug, including things that are "only a warning" — the tree
    must compile with `-Werror` and with zero warnings even with
    `-DKIMIA_WERROR=OFF`.
  - FOSS dependencies only. Current deps: stb (stb_image/stb_image_write,
    vendored), SDL2 optional (KIMIA_ENABLE_SDL2), zlib only via PNG encode.

## Project state at commit `9a81b69` (verified facts)

Repo layout:

- `Engine/Core` — math-adjacent utilities, logging, Time.
- `Engine/Math` — Vec/Mat/Quat; right-handed, +Y up, −Z forward,
  column-major Mat4. (Historic traps already fixed: inverse-transpose
  normals, cube winding.)
- `Engine/Graphics` — GL backend behind a `GLFunctions` indirection
  (Vulkan swap point later), gamma-correct-ish lighting, key-light shadow
  mapping, PNG capture (`capturePNG`).
- `Engine/Scene` — ECS-style Scene; **entity handles are 1-based**;
  `Scene::forEach` passes `const EntityData&` (name, transform, …);
  `SceneIO` v1 text serialization (round-trip + tolerant-load tests).
- `Engine/Physics` — fixed-timestep sim, sphere-vs-plane + sphere-vs-box,
  restitution/friction, rolling friction; accumulator cap fixed; verified
  stable at 60 Hz and 120 Hz.
- `Engine/Renderer` — scene→draw translation.
- `Engine/App` — window/input (SDL2 when available), `Engine` class,
  **WebViewer** (`kimia::web::Server`): streams PNG frames + receives touch
  input over HTTP; `DrainedInput{held,taps,lookX,lookY,zoom}` where *held*
  drains as a level snapshot and taps/look/zoom drain as edges. Routes: `/`,
  `/frame.png` (503 until first frame), `/stats`, `/input`. `Engine::initialize`
  auto-sets `XDG_RUNTIME_DIR=/tmp/kimia-xdg` when unset and on Android
  defaults `LIBGL_ALWAYS_SOFTWARE=1` (both no-overwrite).
- `Examples/GolfGame` (`kimia_golf`) — the first game: aim/charge/shoot/roll/
  sink, stroke counter, full in-game **course BUILDER** (place walls/tee/hole,
  length ±, axis flip, undo, save/load `.kimia` files, play-test with F).
  Builder taps **auto-switch to edit mode** when pressed in play mode.
- `Examples/RemoteView`, `Examples/First3DScene` — older demos, working.
- `Tests` — **118 tests full / 102 headless, all passing.**
- `Documentation/` — MakingCourses.md (file format + builder keys),
  Termux.md, Physics.md, Textures.md, Renderer.md, etc.
- `Tools/termux_build.sh`, `Tools/termux_path_web.sh` — Termux pipelines.
- `ROADMAP.md` — source of truth for what comes next.

Build + verify (this is the acceptance gate — run it after every change):

```bash
cmake -B build -DKIMIA_WERROR=ON && cmake --build build -j4      # zero warnings
DISPLAY=:99 ./build/bin/kimia_tests                               # 118/118
cmake -B build-headless -DKIMIA_ENABLE_SDL2=OFF && cmake --build build-headless -j4
./build-headless/bin/kimia_tests                                  # 102/102
# Web check:
./build/bin/kimia_golf --web --edit --lowfx --port 8080
curl -s http://localhost:8080/stats      # KIMIA GOLF | EDIT | ... | walls N
curl -s -X POST http://localhost:8080/input?tap=return   # walls +1
```

Golf specifics you must preserve: ball radius .12, restitution .40,
friction .40, roll friction .22; launch speed `2.5 + power*13.5`; cup capture
when dist < 0.28 and speed < 5.0; default tee (0,·,7), hole (0,·,−7);
the demo course is player-authored via a `# demo aim power` line
(currently `0 0.61`). Course file: `.kimia` SceneIO v1 text.
Web pad buttons (all curl-verified working): holds a/d/space (aim, SHOOT);
taps 1/2/3 (tool wall/tee/hole), Enter (place), q/e (length), r (axis),
u (undo), s (save `my_course.kimia` in cwd), l (load), f (play/edit), quit.
`/stats` line shows mode/stroke/power/tool/len/axis/walls so every button is
observable.

## Your task: KIMIA World — the option-driven editor

The next (and current) goal, in my own words: I want a complete engine-editor
**like Unity**, where I **write no code and know no keys** — everything is
menus and options. Example: I add a ball and it asks me
**«دقیق باشه یا فانتزی؟»** (accurate physics or fantasy physics?) and my
answer changes the behavior. My acceptance test, step by step:

**Create Project → Open KIMIA World → Create World → Add Player → Add Ball →
Add Environment → Press PLAY → I am playing a game I just made.**

Requirements:

1. Option-driven object creation: adding any object asks a few plain
   questions (physics model, size, colors, speed…) with selectable answers;
   answers map to real engine components (e.g. "fantasy" ball = low friction,
   high bounce, no rolling decay; "accurate" = current tuned golf physics).
2. Works on my phone: the editor UI must run through the existing **WebViewer**
   browser route (port 8080) — menus as tappable buttons + a stats/status line.
   A terminal flow is fine as a secondary front end.
3. Worlds save/load as text files (extend SceneIO v1 compatibly — old
   `.kimia` golf courses must still load).
4. PLAY launches a playable simulation of the world I just assembled.
5. Follow every project convention above: tests per system, `-Werror` clean,
   docs, staged conventional commits, real builds with pasted output.

Start by reading `ROADMAP.md` (the KIMIA World entry is the top open item),
then `Examples/GolfGame/main.cpp` (the builder is the seed of this editor) and
`Engine/App/include/kimia/WebViewer.h`. Then implement it for real — first
milestone: Create World → Add Player → Add Ball (with the accurate/fantasy
question) → PLAY in the browser.

Before your first commit, run the full acceptance gate above and show me the
real test counts. Reply in Persian.
