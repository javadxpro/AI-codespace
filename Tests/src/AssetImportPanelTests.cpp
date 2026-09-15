#include <kimia_test.h>
#include <kimia/AssetImportPanel.h>

KIMIA_TEST(AssetImport_DrawEmptyDoesNotCrash) {
  kimia::ui::drawAssetImportPanel({0, 0, 240, 200}, {});
}

KIMIA_TEST(AssetImport_DrawAllPending) {
  std::vector<kimia::ui::ImportEntry> v;
  for (int i = 0; i < 5; ++i) {
    kimia::ui::ImportEntry e;
    e.sourcePath = "C:/tmp/" + std::to_string(i) + ".obj";
    e.destName = "imported_" + std::to_string(i) + ".obj";
    e.status = kimia::ui::ImportStatus::Pending;
    v.push_back(e);
  }
  kimia::ui::drawAssetImportPanel({0, 0, 240, 200}, v);
}

KIMIA_TEST(AssetImport_DrawImportingWithProgress) {
  std::vector<kimia::ui::ImportEntry> v(3);
  v[0].destName = "a.obj";
  v[0].status = kimia::ui::ImportStatus::Done;
  v[1].destName = "b.obj";
  v[1].status = kimia::ui::ImportStatus::Importing;
  v[1].progress = 0.5f;
  v[2].destName = "c.obj";
  v[2].status = kimia::ui::ImportStatus::Importing;
  v[2].progress = 0.1f;
  kimia::ui::drawAssetImportPanel({0, 0, 280, 200}, v);
}

KIMIA_TEST(AssetImport_DrawFailedWithError) {
  std::vector<kimia::ui::ImportEntry> v(1);
  v[0].destName = "broken.obj";
  v[0].status = kimia::ui::ImportStatus::Failed;
  v[0].error = "OBJ parse: line 12 unexpected token";
  kimia::ui::drawAssetImportPanel({0, 0, 280, 60}, v);
}

KIMIA_TEST(AssetImport_DrawAtPhonePortrait) {
  std::vector<kimia::ui::ImportEntry> v;
  for (int i = 0; i < 4; ++i) {
    kimia::ui::ImportEntry e;
    e.destName = "f" + std::to_string(i);
    e.status = static_cast<kimia::ui::ImportStatus>(i % 4);
    if (e.status == kimia::ui::ImportStatus::Importing) e.progress = 0.5f;
    v.push_back(e);
  }
  kimia::ui::drawAssetImportPanel({0, 0, 240, 320}, v);
}

KIMIA_TEST(AssetImport_DrawAtTabletLandscape) {
  std::vector<kimia::ui::ImportEntry> v;
  for (int i = 0; i < 12; ++i) {
    kimia::ui::ImportEntry e;
    e.destName = "file_" + std::to_string(i);
    e.status = static_cast<kimia::ui::ImportStatus>(i % 4);
    v.push_back(e);
  }
  kimia::ui::drawAssetImportPanel({0, 0, 480, 320}, v);
}
