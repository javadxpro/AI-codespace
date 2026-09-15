#include <kimia_test.h>
#include <kimia/AssetMetadataPanel.h>

KIMIA_TEST(AssetMeta_DrawEmptyDoesNotCrash) {
  kimia::ui::AssetMetadata md;
  kimia::ui::drawAssetMetadataPanel({0, 0, 240, 240}, md);
}

KIMIA_TEST(AssetMeta_DrawWithData) {
  kimia::ui::AssetMetadata md;
  md.name = "tree.obj";
  md.type = "Wavefront OBJ";
  md.path = "assets/tree.obj";
  md.sizeBytes = 4096ULL * 4096;
  md.modifiedSec = 1700000000;
  md.createdSec = 1690000000;
  md.author = "Alice";
  md.license = "CC-BY";
  md.refCount = 3;
  kimia::ui::drawAssetMetadataPanel({0, 0, 320, 280}, md);
}

KIMIA_TEST(AssetMeta_DrawWithHugeSize) {
  kimia::ui::AssetMetadata md;
  md.sizeBytes = 1024ULL * 1024 * 1024 * 5;
  kimia::ui::drawAssetMetadataPanel({0, 0, 240, 240}, md);
}

KIMIA_TEST(AssetMeta_DrawWithZeroSize) {
  kimia::ui::AssetMetadata md;
  md.sizeBytes = 0;
  kimia::ui::drawAssetMetadataPanel({0, 0, 240, 240}, md);
}

KIMIA_TEST(AssetMeta_DrawAtPhonePortrait) {
  kimia::ui::AssetMetadata md;
  md.name = "x";
  kimia::ui::drawAssetMetadataPanel({0, 0, 240, 320}, md);
}

KIMIA_TEST(AssetMeta_DrawAtTabletLandscape) {
  kimia::ui::AssetMetadata md;
  md.name = "scene_main";
  md.type = "KIMIA Scene";
  md.path = "scenes/main.kimia";
  md.sizeBytes = 1024 * 256;
  md.author = "Bob";
  md.license = "MIT";
  md.refCount = 12;
  kimia::ui::drawAssetMetadataPanel({0, 0, 480, 320}, md);
}
