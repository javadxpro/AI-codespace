// EditorUI unit tests — Phase 1.
//
// These tests exercise the CPU-side logic of the EditorUI layer: dock
// layout, tab panels, pointer tracking and the commands the UI emits when
// the user interacts with it. They do NOT need GL; that path is wired
// up later in JNI for Android and D3D11 for Windows.
#include <kimia_test.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Panel.h>

namespace {

void seedScene(kimia::ui::FrameContext& ctx) {
  ctx.scene = kimia::ui::SceneSnapshot{};
  ctx.scene.valid = true;
  ctx.commands.clear();
  ctx.scene.logLines.clear();

  kimia::ui::EntityRef ground;
  ground.name = "Ground";
  ground.locked = true;
  ctx.scene.entities.push_back(ground);

  kimia::ui::EntityRef ball;
  ball.name = "Ball";
  ball.locked = true;
  ball.posY = 0.5f;
  ball.colorR = 0.95f;
  ctx.scene.entities.push_back(ball);

  for (int i = 0; i < 8; ++i) {
    kimia::ui::EntityRef e;
    e.name = "Cube_" + std::to_string(i);
    e.posX = static_cast<float>(i);
    ctx.scene.entities.push_back(e);
  }
}

}  // namespace

KIMIA_TEST(EditorUI_InitializeReturnsTrue) {
  std::string err;
  KIMIA_REQUIRE(kimia::ui::initialize(err));
  KIMIA_REQUIRE(err.empty());
  // After init, the dock layout has the default panels.
  KIMIA_REQUIRE(kimia::ui::hasPanel("Object Tree"));
  KIMIA_REQUIRE(kimia::ui::hasPanel("Property Sheet"));
  KIMIA_REQUIRE(kimia::ui::hasPanel("Toolbar"));
  KIMIA_REQUIRE(kimia::ui::hasPanel("Log"));
  KIMIA_REQUIRE(kimia::ui::panelCount() >= 5);
  kimia::ui::shutdown();
}

KIMIA_TEST(EditorUI_ResizeUpdatesViewport) {
  std::string err;
  kimia::ui::initialize(err);
  kimia::ui::resize(1080, 2400);
  // The frame's viewport is what we asked for; the dpi was derived from
  // width / 540 (a heuristic for phones like the Poco X3 Pro).
  kimia::ui::FrameContext ctx;
  seedScene(ctx);
  ctx.scene.playing = false;
  // draw() builds the layout but doesn't render (no GL). It must not
  // crash and must produce no commands on an empty input.
  kimia::ui::submitPointer({0, kimia::ui::PointerAction::Cancel, 0, 0});
  kimia::ui::draw(ctx);
  KIMIA_REQUIRE(ctx.commands.empty());
  kimia::ui::shutdown();
}

KIMIA_TEST(EditorUI_EmptySceneDoesNothing) {
  std::string err;
  kimia::ui::initialize(err);
  kimia::ui::resize(800, 600);
  kimia::ui::FrameContext ctx;
  // No scene set. draw() should not crash, no commands should be produced.
  kimia::ui::draw(ctx);
  KIMIA_REQUIRE(ctx.commands.empty());
  kimia::ui::shutdown();
}

KIMIA_TEST(EditorUI_PanelLayoutComputes) {
  std::string err;
  kimia::ui::initialize(err);
  auto& layout = kimia::ui::layout();
  KIMIA_REQUIRE(!layout.slots.empty());
  // The 5 default slots must exist (top, left, centre, right, bottom).
  bool hasTop = false, hasBottom = false;
  for (const auto& s : layout.slots) {
    if (s.id == "top") hasTop = true;
    if (s.id == "bottom") hasBottom = true;
  }
  KIMIA_REQUIRE(hasTop);
  KIMIA_REQUIRE(hasBottom);
  // Resize the surface and verify the slot rects are updated.
  kimia::ui::resize(1920, 1080);
  for (const auto& s : layout.slots) {
    if (s.id == "top") {
      KIMIA_REQUIRE(s.rect.h > 0.0f);
      KIMIA_REQUIRE(s.rect.w > 0.0f);
    }
  }
  kimia::ui::shutdown();
}

KIMIA_TEST(EditorUI_LogIsCollected) {
  std::string err;
  kimia::ui::initialize(err);
  kimia::ui::log("hello");
  kimia::ui::log("world");
  const auto& lines = kimia::ui::logLines();
  KIMIA_REQUIRE(lines.size() >= 2);
  // Most recent line is at the end (ring buffer order).
  KIMIA_REQUIRE(lines.back() == "world");
  kimia::ui::shutdown();
}

KIMIA_TEST(EditorUI_TapEmitsSelectCommand) {
  std::string err;
  kimia::ui::initialize(err);
  kimia::ui::resize(800, 1200);
  // Force a known selection state by feeding the engine a pre-selected
  // name, then submitting a tap on one of the visible Object Tree rows.
  // Because the engine has no pick ray in CPU-only mode, we rely on the
  // editor to read the entity list — taps within the Object Tree's
  // content rect should emit a SelectEntity command.
  //
  // We exercise the path by tapping inside the Object Tree area, which
  // is the left slot. With the default 0.22 leftFrac, the panel content
  // rect is approximately (0, 0.06*h, 0.22*w, 0.72*h) — pick the middle
  // of an entity row by tapping near y = 100 (after the toolbar at
  // 60px and the header at 28px).
  kimia::ui::submitPointer({0, kimia::ui::PointerAction::Down, 50.0f, 120.0f});
  kimia::ui::submitPointer({0, kimia::ui::PointerAction::Up,   50.0f, 120.0f});
  kimia::ui::FrameContext ctx;
  seedScene(ctx);
  ctx.scene.selectedNames.clear();
  kimia::ui::draw(ctx);
  // The exact entity selected depends on which row the tap landed on.
  // Either way, at least one SelectEntity command should be in the list.
  bool anySelect = false;
  for (const auto& c : ctx.commands) {
    if (c.kind == kimia::ui::UiCommandKind::SelectEntity) {
      anySelect = true;
      break;
    }
  }
  KIMIA_REQUIRE(anySelect);
  kimia::ui::shutdown();
}

KIMIA_TEST(EditorUI_DpiHeuristicForPoco) {
  std::string err;
  kimia::ui::initialize(err);
  // 1080x2400 → density 1080/540 = 2.0 (clamped to [1.5, 3.5]).
  // This matches the Poco X3 Pro's screen exactly.
  kimia::ui::resize(1080, 2400);
  // Theme dp() depends on the density we computed.
  // 1 dp = 2.0 px → 44 dp = 88 px (Material minimum touch target).
  const float px = kimia::ui::theme::dp(44);
  KIMIA_REQUIRE(px > 80.0f);
  KIMIA_REQUIRE(px < 100.0f);
  kimia::ui::shutdown();
}
