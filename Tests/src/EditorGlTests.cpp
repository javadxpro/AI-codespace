// EditorGl tests — Phase 3.
//
// Verifies the shell that Phase 4 will fill in: the atlas is built,
// init() returns true, paint() is a safe no-op when called without a
// GL context. The actual draw-call coverage will come once the GL
// pipeline lands.
#include <kimia_test.h>
#include <kimia/EditorGl.h>
#include <kimia/EditorUI.h>

#include <string>
#include <vector>

KIMIA_TEST(EditorGl_InitBuildsAtlasAndIsReady) {
  // Note: we don't run GL on Linux, so the shader compile path stays
  // disabled. init() builds the CPU-side atlas and flips the ready_
  // flag; Phase 4 will add the GL upload step.
  std::string err;
  kimia::ui::EditorGl gl;
  KIMIA_REQUIRE(gl.init(err));
  KIMIA_REQUIRE(err.empty());
  KIMIA_REQUIRE(gl.ready());
}

KIMIA_TEST(EditorGl_DestroyMakesNotReady) {
  std::string err;
  kimia::ui::EditorGl gl;
  gl.init(err);
  KIMIA_REQUIRE(gl.ready());
  gl.destroy();
  KIMIA_REQUIRE(!gl.ready());
}

KIMIA_TEST(EditorGl_PaintOnEmptyIsNoOp) {
  // Calling paint on an uninitialised instance must not crash.
  std::vector<kimia::ui::DrawCmd> cmds;
  kimia::ui::EditorGl gl;  // not init'd
  gl.paint(cmds, 100, 100);
  // Reaching here without crashing is the assertion.
  KIMIA_REQUIRE(true);
}

KIMIA_TEST(EditorGl_PaintEditorGlFunctionExists) {
  // paintEditorGl() is the free-function entry point callers use; it
  // exists as a symbol and accepts the right arguments. On a desktop
  // build without GL it's a no-op; on Android it'll dispatch to the
  // shader-backed renderer.
  std::vector<kimia::ui::DrawCmd> cmds;
  kimia::ui::paintEditorGl(cmds, 100, 100);
  KIMIA_REQUIRE(true);
}
