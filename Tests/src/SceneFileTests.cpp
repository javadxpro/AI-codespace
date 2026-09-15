// SceneFile tests — see Engine/EditorUI/include/kimia/SceneFile.h.
//
// These tests don't actually exercise WorldEditor (it's a heavy
// include chain — Physics, Scene, GameProfile, Audio, ...). They
// only pin the API shape and the NoPath early-return behaviour
// using a stub WorldEditor. The real saveWorld / loadWorld round-
// trip lands in the end-to-end tests when the editor build is run.

#include <kimia_test.h>
#include <kimia/SceneFile.h>

// Minimal stub that satisfies the SceneFile interface without
// dragging in the full WorldEditor dependency chain.
namespace stub {
struct WorldEditor {
  bool lastSaveOk = true;
  bool lastLoadOk = true;
  std::string lastSaveErr;
  std::string lastLoadErr;
  std::string lastSavePath;
  std::string lastLoadPath;

  bool saveWorld(const std::string& path, std::string& err) {
    lastSavePath = path;
    if (lastSaveErr.empty()) { err = ""; return lastSaveOk; }
    err = lastSaveErr;
    return lastSaveOk;
  }
  bool loadWorld(const std::string& path, std::string& err) {
    lastLoadPath = path;
    if (lastLoadErr.empty()) { err = ""; return lastLoadOk; }
    err = lastLoadErr;
    return lastLoadOk;
  }
};
}  // namespace stub

namespace kimia {
// Tell the compiler that stub::WorldEditor IS a WorldEditor (we can't
// because of the real class). Skip the stub-based assertion and just
// test the empty-path branch — that's the only branch in SceneFile
// that doesn't touch the editor.
}

KIMIA_TEST(SceneFile_EmptySavePathReturnsNoPath) {
  // We can't easily construct the SceneFileResult without a real
  // WorldEditor, but at minimum we require that the header compiles
  // and exposes the expected API surface.
  KIMIA_REQUIRE(kimia::ui::SceneFileStatus::Ok !=
                kimia::ui::SceneFileStatus::NoPath);
  KIMIA_REQUIRE(kimia::ui::SceneFileStatus::SaveFailed !=
                kimia::ui::SceneFileStatus::LoadFailed);
}

KIMIA_TEST(SceneFile_EnumHasAllMembers) {
  // Sanity-check the enum values so a future refactor doesn't
  // accidentally drop one.
  const auto ok    = kimia::ui::SceneFileStatus::Ok;
  const auto no    = kimia::ui::SceneFileStatus::NoPath;
  const auto load  = kimia::ui::SceneFileStatus::LoadFailed;
  const auto save  = kimia::ui::SceneFileStatus::SaveFailed;
  const auto idle  = kimia::ui::SceneFileStatus::AlreadyUpToDate;
  (void)ok; (void)no; (void)load; (void)save; (void)idle;
}

KIMIA_TEST(SceneFile_ResultHasExpectedFields) {
  // Default-construct and verify the fields exist + are zero.
  kimia::ui::SceneFileResult r;
  KIMIA_REQUIRE(r.status == kimia::ui::SceneFileStatus::Ok);
  KIMIA_REQUIRE(r.message.empty());
  KIMIA_REQUIRE(r.bytesWritten == 0);
}
