# KIMIA EditorUI — Phase 1 status

## What's done

Phase 1 of the editor rebuild is on disk: a from-scratch GLES immediate-mode
UI toolkit that runs on top of the existing WorldEditor. The host (Android JNI,
eventually Windows D3D11) wires touch/keyboard inputs in and reads commands
back out; the editor itself stays platform-agnostic.

### Files added (12 source + 4 test)

| Path | Lines | What |
|---|---|---|
| `Engine/EditorUI/include/kimia/EditorUI.h` | 218 | Public API, FrameContext, types |
| `Engine/EditorUI/include/kimia/Theme.h`     |  73 | Dark Pro palette + sizing |
| `Engine/EditorUI/include/kimia/Icons.h`     |  53 | Tool/media glyphs (5×7) |
| `Engine/EditorUI/include/kimia/Widget.h`    | 135 | Immediate-mode widget set |
| `Engine/EditorUI/include/kimia/Panel.h`     |  90 | Dock + tabs + drop zones |
| `Engine/EditorUI/include/kimia/HostBridge.h`|  38 | Engine ↔ UI adapter API |
| `Engine/EditorUI/src/EditorUI.cpp`          | 444 | Per-panel dispatch + draw |
| `Engine/EditorUI/src/Widget.cpp`            | 478 | Buttons, sliders, lists, text |
| `Engine/EditorUI/src/Panel.cpp`             | 313 | Tab chrome, splitter, drag-drop |
| `Engine/EditorUI/src/Theme.cpp`             |  15 | DPI helpers |
| `Engine/EditorUI/src/Icons.cpp`             | 165 | Glyph bitmaps |
| `Engine/EditorUI/src/HostBridge.cpp`        | 121 | Snapshot + command apply |
| `Tests/src/EditorUITests.cpp`               | 165 | Unit tests |
| `Tests/src/EditorUIIntegrationTests.cpp`    | 165 | WorldEditor round-trip |
| `Engine/EditorUI/syntax_check.sh`           | — | Fast desktop syntax gate |
| `Engine/EditorUI/TODO.md`                   | — | Roadmap |
| `Engine/EditorUI/PHASE1_STATUS.md`          | — | This file |

`CMakeLists.txt` updated: `kimia_editor_ui` static library; both test files
linked into the `kimia_tests` executable.

## What passes locally

- All 5 EditorUI .cpp files syntax-check clean via
  `Engine/EditorUI/syntax_check.sh` (g++ -fsyntax-only, -Wall -Wextra).
- All 7 unit tests + 4 integration tests syntax-check clean via a parallel
  script.
- Build-time sanity: every Widget/Panel call compiles against the public
  API exposed in `<kimia/EditorUI.h>` and `<kimia/HostBridge.h>`.

What we couldn't validate locally (no cmake in the sandbox):

- Full link step (links against `kimia_world` + `kimia_math`).
- Actual APK build on Poco X3 Pro (no Android NDK in sandbox).
- 475+ tests still green (depends on cmake + CI).

That's all handled by the existing GitHub Actions workflow on the next push.

## What's deliberately deferred (Phase 2+)

- **GL rendering.** Phase 1 keeps the renderer stubbed (CPU-side logic only)
  so the editor runs in tests. The GL pipeline (program, atlas, vertex
  buffer) is sketched in the code but disabled. Wiring it up is a single
  commit once we add `uniform2f` / `bufferSubData` to GLFunctions.
- **Undo/Redo** (Command Pattern, 100 commands deep).
- **Multi-select** (Shift+Tap + Marquee).
- **Render of the 3D Scene View as a texture.** Today the Scene View panel
  is just chrome; the engine frame is drawn underneath as the JNI does.
- **Gizmos** for the Move/Rotate/Scale tools.
- **Timeline** for animation playback.
- **Keyboard editing** for text fields (Bluetooth keyboard arrives in
  Phase 2).
- **Windows native window.** Deferred per the agreed platform priority.

## Test plan for the user

1. **CI:** Push the branch. The `Build Android APK` workflow builds the APK
   with `kimia_editor_ui` linked in. If the build is green, Phase 1 is
   buildable on Android.
2. **On-device:** Install the APK on your Poco X3 Pro. Today the activity
   still uses the Java UI from the old editor (the JNI bridge from the
   previous phase). Wiring the EditorUI to the JNI renderer is the next
   commit — until then, the editor layer compiles but isn't visible
   in-app.
3. **Tests on CI:** Once CMake is available in the workflow, the
   `kimia_tests` binary runs both `EditorUITests.cpp` and
   `EditorUIIntegrationTests.cpp` automatically. No separate script needed.

## Decisions captured

- **Theme:** Dark Pro (Unity palette).
- **Font:** existing 5×7 bitmap, scaled 1×/2×/3×.
- **Input:** touch only (Phase 1); keyboard stub only.
- **UI arch:** immediate mode (ImGui-style), single-thread render thread.
- **Panels:** Object Tree, Property Sheet, Scene View, Game View,
  Toolbar, Log. Toolbar has 4 tool buttons + Play/Pause/Step.
- **Layout:** 5 slots (top/left/centre/right/bottom), dockable tabs with
  long-press drag, splitter between left↔centre and centre↔bottom.
- **Selection:** WorldEditor.selectEntity() is the single source of truth;
  EditorUI only mirrors it via SceneSnapshot.selectedNames.
- **Persistence:** DockLayout is plain old data; the host saves it to
  SharedPreferences on Android (Phase 2 wires that up).
