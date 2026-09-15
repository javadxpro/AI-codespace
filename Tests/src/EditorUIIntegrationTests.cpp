// EditorUI integration tests — connect EditorUI to a real WorldEditor
// instance and verify the round-trip: scene → snapshot → UI → commands
// → engine.
#include <kimia_test.h>
#include <kimia/EditorUI.h>
#include <kimia/Panel.h>
#include <kimia/HostBridge.h>
#include <kimia/RasterBridge.h>
#include <kimia/World.h>
#include <kimia/Scene.h>
#include <kimia/GameProfile.h>
#include <kimia/Image.h>
#include <kimia/Types.h>

#include <cmath>
#include <cstdio>
#include <set>
#include <string>

KIMIA_TEST(EditorUI_RoundTrip_SelectsEntity) {
  std::string err;
  KIMIA_REQUIRE(kimia::ui::initialize(err));
  kimia::ui::resize(1080, 2400);

  // Build a real world with a Player, Ball and Ground.
  const kimia::GameProfile* profile = nullptr;
  for (const auto& p : kimia::builtinProfiles()) {
    if (p.name == "golf") { profile = &p; break; }
  }
  KIMIA_REQUIRE(profile != nullptr);

  kimia::WorldEditor editor;
  editor.createWorld(*profile);
  editor.createObject("player", {0.0, 0.0, 4.0});
  editor.createObject("ball", {0.0, 0.0, 3.0});
  editor.createObject("hole", {0.0, 0.0, -2.0});
  KIMIA_REQUIRE(editor.entityNames().size() >= 4U);

  // Snapshot the scene, draw the editor (no GL), then apply the resulting
  // commands back to the engine.
  kimia::ui::FrameContext ctx;
  ctx.scene = kimia::ui::snapshotFrom(&editor);
  KIMIA_REQUIRE(ctx.scene.valid);
  KIMIA_REQUIRE(ctx.scene.entities.size() >= 4U);

  // Tap on the Object Tree area to select an entity.
  // Object Tree is the left slot. After resize to 1080×2400 with
  // leftFrac=0.22 and topFrac=0.06, the panel content rect is
  // approximately (0, 144, 238, 1728). The first entity row is just
  // below the header; tapping near (50, 200) lands on it.
  kimia::ui::submitPointer({0, kimia::ui::PointerAction::Down, 50.0f, 200.0f});
  kimia::ui::submitPointer({0, kimia::ui::PointerAction::Up,   50.0f, 200.0f});
  kimia::ui::draw(ctx);

  // Apply commands.
  for (const auto& c : ctx.commands) {
    kimia::ui::applyCommand(&editor, c);
  }

  // The engine should now have a selection.
  const std::string& sel = editor.selectedName();
  KIMIA_REQUIRE(!sel.empty());
  KIMIA_REQUIRE(editor.entityNames().size() >= 4U);

  kimia::ui::shutdown();
}

KIMIA_TEST(EditorUI_RoundTrip_ToolbarPlayPause) {
  std::string err;
  KIMIA_REQUIRE(kimia::ui::initialize(err));
  kimia::ui::resize(1080, 2400);

  const kimia::GameProfile* profile = nullptr;
  for (const auto& p : kimia::builtinProfiles()) {
    if (p.name == "golf") { profile = &p; break; }
  }
  kimia::WorldEditor editor;
  editor.createWorld(*profile);
  editor.createObject("player", {0.0, 0.0, 4.0});
  editor.createObject("ball", {0.0, 0.0, 3.0});

  // Tap the Play button on the toolbar. The toolbar is the top slot.
  // Toolbar buttons are at y ≈ topH/2 = 0.06*2400/2 = 72 (after a 32dp
  // toolbar). Play is the 5th button (after Sel/Move/Rot/Scale +
  // separator + Play).
  const float playX = 8.0f + 4.0f * 36.0f * 2.0f + 8.0f + 12.0f + 18.0f;  // density ~2
  const float playY = 100.0f;
  kimia::ui::submitPointer({0, kimia::ui::PointerAction::Down, playX, playY});
  kimia::ui::submitPointer({0, kimia::ui::PointerAction::Up,   playX, playY});

  kimia::ui::FrameContext ctx;
  ctx.scene = kimia::ui::snapshotFrom(&editor);
  kimia::ui::draw(ctx);
  for (const auto& c : ctx.commands) {
    kimia::ui::applyCommand(&editor, c);
  }

  KIMIA_REQUIRE(editor.playing());
  KIMIA_REQUIRE(!editor.paused());
  kimia::ui::shutdown();
}

KIMIA_TEST(EditorUI_RoundTrip_PositionFieldUpdatesEntity) {
  std::string err;
  KIMIA_REQUIRE(kimia::ui::initialize(err));
  kimia::ui::resize(1080, 2400);

  const kimia::GameProfile* profile = nullptr;
  for (const auto& p : kimia::builtinProfiles()) {
    if (p.name == "golf") { profile = &p; break; }
  }
  kimia::WorldEditor editor;
  editor.createWorld(*profile);
  editor.createObject("block", {1.0, 2.0, 3.0});
  editor.selectEntity("block");

  // Snapshot, simulate an Edit on the Position field by pushing a
  // SetPosition command directly, then re-snapshot and verify.
  kimia::ui::UiCommand cmd;
  cmd.kind = kimia::ui::UiCommandKind::SetPosition;
  cmd.name = "block";
  cmd.x = 9.5f; cmd.y = -1.0f; cmd.z = 4.0f;
  KIMIA_REQUIRE(kimia::ui::applyCommand(&editor, cmd));

  const kimia::EntityData* data = editor.world().scene.get(
      editor.world().scene.find("block"));
  KIMIA_REQUIRE(data != nullptr);
  KIMIA_REQUIRE(std::abs(data->transform.position.x - 9.5) < 1e-4);
  KIMIA_REQUIRE(std::abs(data->transform.position.y - -1.0) < 1e-4);
  KIMIA_REQUIRE(std::abs(data->transform.position.z - 4.0) < 1e-4);

  // The snapshot must reflect the new position.
  auto snap = kimia::ui::snapshotFrom(&editor);
  for (const auto& e : snap.entities) {
    if (e.name == "block") {
      KIMIA_REQUIRE(std::abs(e.posX - 9.5f) < 1e-3f);
      KIMIA_REQUIRE(std::abs(e.posY - -1.0f) < 1e-3f);
      KIMIA_REQUIRE(std::abs(e.posZ - 4.0f) < 1e-3f);
      return;
    }
  }
  KIMIA_REQUIRE(false);  // entity not found in snapshot
  kimia::ui::shutdown();
}

KIMIA_TEST(EditorUI_DockLayoutHasAllFiveSlots) {
  std::string err;
  KIMIA_REQUIRE(kimia::ui::initialize(err));
  auto& layout = kimia::ui::layout();
  // Verify the 5 default slots.
  std::set<std::string> expected = {"top", "left", "centre", "right", "bottom"};
  std::set<std::string> actual;
  for (const auto& s : layout.slots) actual.insert(s.id);
  KIMIA_REQUIRE(actual == expected);

  // Each slot has exactly one default tab.
  for (const auto& s : layout.slots) {
    KIMIA_REQUIRE(s.tabs.size() == 1U);
  }
  // Top is the toolbar; left is Object Tree; centre is Property Sheet;
  // right is Scene View; bottom is Log.
  KIMIA_REQUIRE(layout.slots[0].tabs[0] == "Toolbar");
  KIMIA_REQUIRE(layout.slots[1].tabs[0] == "Object Tree");
  KIMIA_REQUIRE(layout.slots[2].tabs[0] == "Property Sheet");
  KIMIA_REQUIRE(layout.slots[3].tabs[0] == "Scene View");
  KIMIA_REQUIRE(layout.slots[4].tabs[0] == "Log");
  kimia::ui::shutdown();
}

KIMIA_TEST(EditorUI_DrawThenRasterOverProducesPixels) {
  // End-to-end check that EditorUI::draw() actually produces a non-empty
  // DrawCmd list, that RasterBridge::rasteriseOver() consumes it, and
  // that an RGB frame afterwards contains the editor's pixels.
  std::string err;
  KIMIA_REQUIRE(kimia::ui::initialize(err));
  kimia::ui::resize(720, 1600);

  const kimia::GameProfile* profile = nullptr;
  for (const auto& p : kimia::builtinProfiles()) {
    if (p.name == "golf") { profile = &p; break; }
  }
  KIMIA_REQUIRE(profile != nullptr);

  kimia::WorldEditor editor;
  editor.createWorld(*profile);
  editor.createObject("player", {0.0, 0.0, 4.0});
  editor.createObject("ball", {0.0, 0.0, 3.0});

  // Render the editor into a 720x1600 RGB frame.
  kimia::Image frame;
  frame.width = 720;
  frame.height = 1600;
  frame.channels = 3;
  frame.pixels.assign(static_cast<size_t>(frame.width) *
                          static_cast<size_t>(frame.height) * 3u, 0u);

  kimia::ui::FrameContext ctx;
  ctx.scene = kimia::ui::snapshotFrom(&editor);
  kimia::ui::draw(ctx);

  const std::vector<kimia::ui::DrawCmd> cmds = kimia::ui::takeDrawCmds();
  // The toolbar / panels should have emitted at least one rect each.
  KIMIA_REQUIRE(!cmds.empty());
  kimia::ui::rasteriseOver(cmds, frame);

  // At least one pixel in the frame must be non-zero (the editor painted
  // its background / toolbar somewhere).
  bool anyChange = false;
  for (size_t i = 0; i < frame.pixels.size(); i += 3) {
    if (frame.pixels[i] != 0 || frame.pixels[i + 1] != 0 ||
        frame.pixels[i + 2] != 0) { anyChange = true; break; }
  }
  KIMIA_REQUIRE(anyChange);

  kimia::ui::shutdown();
}

KIMIA_TEST(EditorUI_DrawClearsBufferAfterTake) {
  // The host's draw loop relies on takeDrawCmds() draining the buffer
  // every frame; verify that calling it twice in a row without an
  // intervening draw returns an empty list the second time.
  std::string err;
  KIMIA_REQUIRE(kimia::ui::initialize(err));
  kimia::ui::resize(720, 1600);

  const kimia::GameProfile* profile = nullptr;
  for (const auto& p : kimia::builtinProfiles()) {
    if (p.name == "golf") { profile = &p; break; }
  }
  KIMIA_REQUIRE(profile != nullptr);

  kimia::WorldEditor editor;
  editor.createWorld(*profile);
  editor.createObject("player", {0.0, 0.0, 4.0});

  kimia::ui::FrameContext ctx;
  ctx.scene = kimia::ui::snapshotFrom(&editor);
  kimia::ui::draw(ctx);

  const std::vector<kimia::ui::DrawCmd> first = kimia::ui::takeDrawCmds();
  KIMIA_REQUIRE(!first.empty());
  const std::vector<kimia::ui::DrawCmd> second = kimia::ui::takeDrawCmds();
  KIMIA_REQUIRE(second.empty());

  kimia::ui::shutdown();
}
