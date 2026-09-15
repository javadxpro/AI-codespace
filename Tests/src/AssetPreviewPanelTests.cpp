#include <kimia_test.h>
#include <kimia/AssetPreviewPanel.h>

KIMIA_TEST(AssetPreview_DrawNoneDoesNotCrash) {
  kimia::ui::drawAssetPreviewPanel({0, 0, 200, 200},
                                   kimia::ui::AssetPreviewKind::None, "");
}

KIMIA_TEST(AssetPreview_DrawModel) {
  kimia::ui::drawAssetPreviewPanel({0, 0, 200, 200},
                                   kimia::ui::AssetPreviewKind::Model3D,
                                   "tree.obj");
}

KIMIA_TEST(AssetPreview_DrawImage) {
  kimia::ui::drawAssetPreviewPanel({0, 0, 200, 200},
                                   kimia::ui::AssetPreviewKind::Image2D,
                                   "ground.png");
}

KIMIA_TEST(AssetPreview_DrawText) {
  kimia::ui::drawAssetPreviewPanel({0, 0, 200, 200},
                                   kimia::ui::AssetPreviewKind::Text,
                                   "readme.txt");
}

KIMIA_TEST(AssetPreview_DrawUnknown) {
  kimia::ui::drawAssetPreviewPanel({0, 0, 200, 200},
                                   kimia::ui::AssetPreviewKind::Unknown,
                                   "blob.bin");
}

KIMIA_TEST(AssetPreview_DrawWithLongName) {
  kimia::ui::drawAssetPreviewPanel({0, 0, 200, 200},
                                   kimia::ui::AssetPreviewKind::Model3D,
                                   "Very_Long_Asset_Name_That_Truncates.txt");
}

KIMIA_TEST(AssetPreview_DrawAtPhonePortrait) {
  kimia::ui::drawAssetPreviewPanel({0, 0, 200, 240},
                                   kimia::ui::AssetPreviewKind::Image2D, "p");
}

KIMIA_TEST(AssetPreview_DrawAtTabletLandscape) {
  kimia::ui::drawAssetPreviewPanel({0, 0, 320, 320},
                                   kimia::ui::AssetPreviewKind::Model3D, "m");
}
