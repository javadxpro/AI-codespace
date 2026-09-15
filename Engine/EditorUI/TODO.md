# KIMIA EditorUI — Re-architecture TODO

A native, GLES-rendered, immediate-mode UI toolkit that replaces the WebViewer
"Unity Workbench" with a single-surface editor that runs on Android and (later)
Windows. Code is platform-agnostic; the host wires inputs and feeds frames.

Names diverge from Unity on purpose:
  Hierarchy     → Object Tree
  Inspector     → Property Sheet
  Project       → Library
  Console       → Log
  Game view     → Game View
  Scene view    → Scene View
  Toolbar       → Toolbar (Play/Pause/Step + tools)

The theme is "Dark Pro" (Unity-feel) and the font stays the existing 5x7
ASCII bitmap to keep build size tiny.

---

## Phase 1 — Foundation + Object Tree (this commit)

- [x] UI toolkit core: layout rects, immediate-mode widget set
  (button/label/slider/textfield/checkbox/color/collapsing-header/separator)
- [x] Theme: Dark Pro colors, paddings, corner radii, animation durations
- [x] Bitmap font (existing) at 1x/2x/3x scales
- [x] Docking: tab bar, drop targets, drag handle, splitter
- [x] Toolbar: Play / Pause / Step + 4 transform tools (Select/Move/Rotate/Scale)
- [x] Object Tree panel: reads WorldEditor.entityNames() and selection
- [x] Scene View panel: passthrough to existing engine render
- [x] Property Sheet panel: minimal version showing name + position
- [x] Log panel: in-memory ring buffer from engine
- [x] Wire into Android JNI (replaces the old edit-panel Java + native bridge)
- [x] arm64-v8a only (Poco X3 Pro)
- [ ] Windows (postponed)
- [ ] Undo/Redo (Phase 2)
- [ ] Multi-select (Phase 2)
- [ ] Gizmo rendering (Phase 2)
- [ ] Timeline (Phase 3)
- [ ] Library (Phase 3)

## Files

```
Engine/EditorUI/
├── include/kimia/
│   ├── EditorUI.h          # public surface (init/draw/input)
│   ├── Panel.h             # Panel base + Docking
│   ├── Widget.h            # immediate-mode widget set
│   ├── Theme.h             # colors / sizes
│   └── Icons.h             # 5x7 glyphs for tools + media buttons
├── src/
│   ├── EditorUI.cpp
│   ├── Panel.cpp           # docking layout + tab dragging
│   ├── Widget.cpp          # button/slider/textfield/checkbox/...
│   ├── Theme.cpp
│   ├── Icons.cpp
│   └── InputState.cpp      # touch → UI events (tap, drag, pinch, long-press)
├── shaders/
│   ├── ui_vert.glsl        # #version 300 es
│   ├── ui_frag.glsl        # rounded rect SDF + text atlas sampler
│   └── ...
```

## Wiring

- `Engine/EditorUI/` → static lib `kimia_editor_ui`
- `Engine/App/` → pulls in editor UI for the desktop `kimia_world` shell
- `Android/app/src/main/cpp/jni_glue.cpp` → routes all touch to EditorUI's
  input layer; EditorUI owns the in-game panels. Existing JNI edit bridge
  (`nativeEdit*`) is removed because the editor lives entirely on the GL side
  now and reads/writes `WorldEditor` directly in the render loop.
