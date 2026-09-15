#include <kimia/AssetPreviewPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

namespace kimia::ui {

namespace {
const char* kindName(AssetPreviewKind k) {
  switch (k) {
    case AssetPreviewKind::None:    return "(no asset)";
    case AssetPreviewKind::Model3D: return "3D model";
    case AssetPreviewKind::Image2D: return "2D image";
    case AssetPreviewKind::Text:    return "Text";
    case AssetPreviewKind::Unknown: return "Unknown";
  }
  return "?";
}
}

void drawAssetPreviewPanel(const Rect& rect,
                           AssetPreviewKind kind,
                           const std::string& name) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Preview", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  // The preview canvas — a square inset below the title.
  const f32 titleH = 14.0f;
  const f32 side = std::min(rect.w - 16.0f, rect.h - titleH - 30.0f);
  const Rect preview{
      rect.x + (rect.w - side) * 0.5f, rect.y + titleH + 4.0f,
      side, side};
  drawRect(preview, kPanelAlt, 2.0f);

  // A glyph that hints at the kind.
  const char* glyph = "?";
  Color tint = kTextDim;
  switch (kind) {
    case AssetPreviewKind::Model3D: glyph = "M"; tint = {0.5f, 0.8f, 1.0f, 1.0f}; break;
    case AssetPreviewKind::Image2D: glyph = "P"; tint = {0.9f, 0.7f, 0.4f, 1.0f}; break;
    case AssetPreviewKind::Text:    glyph = "T"; tint = {0.6f, 1.0f, 0.6f, 1.0f}; break;
    default:                        glyph = "?"; tint = kTextDim;
  }
  drawText(glyph,
           preview.x + preview.w * 0.5f - 16.0f,
           preview.y + preview.h * 0.5f - 8.0f,
           3, tint);  // scale 3 → big glyph

  // Bottom: file name + kind.
  drawText(name.c_str(),
           rect.x + 6.0f, rect.y + rect.h - 16.0f, 1, kText);
  drawText(kindName(kind),
           rect.x + rect.w - 80.0f, rect.y + rect.h - 16.0f,
           1, kTextMuted);
}

}
