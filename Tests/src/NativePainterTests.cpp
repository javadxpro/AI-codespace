// NativePainter integration tests — Phase 2.
//
// End-to-end check that the same code path the Android jni_glue uses
// (paintNativeEditor) actually produces pixels and applies UI commands
// to a real WorldEditor.
#include <kimia_test.h>
#include <kimia/NativePainter.h>
#include <kimia/EditorUI.h>
#include <kimia/HostBridge.h>
#include <kimia/World.h>
#include <kimia/Scene.h>
#include <kimia/GameProfile.h>
#include <kimia/Image.h>
#include <kimia/Types.h>

#include <cmath>
#include <cstdio>
#include <string>

namespace {

// Build a small RGB frame (matches what the Android software renderer
// emits) filled with a known "scene" colour so we can later prove the
// editor overlay painted something on top.
kimia::Image makeSceneFrame(kimia::i32 w, kimia::i32 h,
                            kimia::u8 r, kimia::u8 g, kimia::u8 b) {
  kimia::Image img;
  img.width = w;
  img.height = h;
  img.channels = 3;
  img.pixels.assign(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u,
                    static_cast<kimia::u8>(0));
  for (size_t i = 0; i < img.pixels.size(); i += 3) {
    img.pixels[i + 0] = r;
    img.pixels[i + 1] = g;
    img.pixels[i + 2] = b;
  }
  return img;
}

bool anyPixelChanged(const kimia::Image& frame,
                     kimia::u8 baseR, kimia::u8 baseG, kimia::u8 baseB) {
  // Compare against the original scene colour; the editor must have
  // touched at least one pixel.
  const size_t count = static_cast<size_t>(frame.width) *
                       static_cast<size_t>(frame.height);
  for (size_t i = 0; i < count; ++i) {
    const size_t t = i * 3u;
    if (frame.pixels[t + 0] != baseR ||
        frame.pixels[t + 1] != baseG ||
        frame.pixels[t + 2] != baseB) return true;
  }
  return false;
}

const kimia::GameProfile* findGolf() {
  for (const auto& p : kimia::builtinProfiles()) {
    if (p.name == "golf") return &p;
  }
  return nullptr;
}

}  // namespace

KIMIA_TEST(NativePainter_DrawsOverRgbScene) {
  std::string err;
  KIMIA_REQUIRE(kimia::ui::initialize(err));
  kimia::ui::resize(720, 1600);

  const auto* profile = findGolf();
  KIMIA_REQUIRE(profile != nullptr);

  kimia::WorldEditor editor;
  editor.createWorld(*profile);
  editor.createObject("player", {0.0, 0.0, 4.0});
  editor.createObject("ball", {0.0, 0.0, 3.0});

  // Sky-blue scene frame; the editor must draw its panels on top.
  auto frame = makeSceneFrame(720, 1600, 30, 100, 180);

  kimia::ui::paintNativeEditor(frame, editor);

  KIMIA_REQUIRE(anyPixelChanged(frame, 30, 100, 180));
  kimia::ui::shutdown();
}

KIMIA_TEST(NativePainter_SelectCommandUpdatesEngine) {
  // Tap a row in the Object Tree and verify the engine's selection
  // changed. The Object Tree lives in the left slot; with resize(720,
  // 1600) and the default dock layout its content rect starts around
  // (0, 144) and the first selectable row is just below the header.
  std::string err;
  KIMIA_REQUIRE(kimia::ui::initialize(err));
  kimia::ui::resize(720, 1600);

  const auto* profile = findGolf();
  KIMIA_REQUIRE(profile != nullptr);

  kimia::WorldEditor editor;
  editor.createWorld(*profile);
  editor.createObject("block_a", {1.0, 0.0, 0.0});
  editor.createObject("block_b", {2.0, 0.0, 0.0});

  auto frame = makeSceneFrame(720, 1600, 0, 0, 0);
  // First frame just to let the dock layout settle.
  kimia::ui::paintNativeEditor(frame, editor);

  // Tap on the Object Tree area. The Object Tree is the left slot;
  // its content rect is approximately (0, 144, 158, ~1310). The first
  // entity row sits right below the "Entities" label, so tap near
  // (40, 240) — well inside the left slot and past the header.
  kimia::ui::submitPointer({0, kimia::ui::PointerAction::Down, 40.0f, 240.0f});
  kimia::ui::submitPointer({0, kimia::ui::PointerAction::Up,   40.0f, 240.0f});

  frame = makeSceneFrame(720, 1600, 0, 0, 0);
  kimia::ui::paintNativeEditor(frame, editor);

  KIMIA_REQUIRE(!editor.selectedName().empty());
  kimia::ui::shutdown();
}

KIMIA_TEST(NativePainter_PreservesUncoveredScenePixels) {
  // Even after the editor paints its panels, the corner pixels of the
  // scene frame must still be the original sky colour. Panels don't
  // cover the whole viewport; only a panel-sized area changes.
  std::string err;
  KIMIA_REQUIRE(kimia::ui::initialize(err));
  kimia::ui::resize(720, 1600);

  const auto* profile = findGolf();
  KIMIA_REQUIRE(profile != nullptr);

  kimia::WorldEditor editor;
  editor.createWorld(*profile);

  auto frame = makeSceneFrame(720, 1600, 80, 80, 200);
  const kimia::u8 baseR = 80, baseG = 80, baseB = 200;

  kimia::ui::paintNativeEditor(frame, editor);

  // The bottom-right corner of the surface is well outside every panel
  // for this layout; it must still hold the scene colour.
  const size_t corner = (static_cast<size_t>(frame.height - 1) * frame.width +
                         static_cast<size_t>(frame.width - 1)) * 3u;
  KIMIA_REQUIRE(frame.pixels[corner + 0] == baseR);
  KIMIA_REQUIRE(frame.pixels[corner + 1] == baseG);
  KIMIA_REQUIRE(frame.pixels[corner + 2] == baseB);

  kimia::ui::shutdown();
}

KIMIA_TEST(NativePainter_DrawTwiceStaysConsistent) {
  // Drawing the editor twice in a row must not double the overlay or
  // leave it in a broken state — takeDrawCmds drains the buffer, so a
  // second draw with the same input produces the same pixels.
  std::string err;
  KIMIA_REQUIRE(kimia::ui::initialize(err));
  kimia::ui::resize(720, 1600);

  const auto* profile = findGolf();
  KIMIA_REQUIRE(profile != nullptr);

  kimia::WorldEditor editor;
  editor.createWorld(*profile);
  editor.createObject("cube", {0.0, 0.0, 0.0});

  auto frame1 = makeSceneFrame(720, 1600, 0, 0, 0);
  auto frame2 = makeSceneFrame(720, 1600, 0, 0, 0);

  kimia::ui::paintNativeEditor(frame1, editor);
  kimia::ui::paintNativeEditor(frame2, editor);

  // Same scene, same UI state — bytes must match exactly.
  KIMIA_REQUIRE(frame1.pixels.size() == frame2.pixels.size());
  for (size_t i = 0; i < frame1.pixels.size(); ++i) {
    KIMIA_REQUIRE(frame1.pixels[i] == frame2.pixels[i]);
  }

  kimia::ui::shutdown();
}
