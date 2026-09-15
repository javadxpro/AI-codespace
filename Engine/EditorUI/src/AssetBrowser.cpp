// AssetBrowser implementation — see AssetBrowser.h.
#include <kimia/AssetBrowser.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

#include <algorithm>
#include <cctype>

namespace kimia::ui {

namespace {

std::string toLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return s;
}

std::string extension(const std::string& name) {
  const auto pos = name.find_last_of('.');
  if (pos == std::string::npos) return "";
  return toLower(name.substr(pos + 1));
}

}  // namespace

AssetKind classifyFile(const std::string& name) {
  const auto ext = extension(name);
  if (ext == "obj" || ext == "fbx") return AssetKind::Model;
  if (ext == "png" || ext == "jpg" || ext == "jpeg") return AssetKind::Image;
  if (ext == "kimia") return AssetKind::Scene;
  return AssetKind::Other;
}

std::vector<AssetEntry> scanAssets(const std::string& directory) {
  // Phase 4+ placeholder: we don't actually open the filesystem here
  // because the EditorUI layer is meant to be platform-agnostic (it
  // runs inside unit tests on Linux, in the Emscripten web build,
  // and on Android). The WorldEditor owns the real directory scan
  // and calls scanAssetsFromList with the file list it produced.
  // scanAssets() returns an empty vector; the WorldEditor populates
  // it via scanAssetsFromList() (not in this header to keep the
  // API surface small for the unit tests).
  (void)directory;
  return {};
}

void drawAssetBrowser(const Rect& rect,
                      const std::vector<AssetEntry>& assets,
                      i32 scrollY) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);

  pushClip(rect);
  const f32 lineH = 14.0f;
  const f32 padX = 4.0f;
  const f32 startY = rect.y + 4.0f - static_cast<f32>(scrollY);
  for (std::size_t i = 0; i < assets.size(); ++i) {
    const f32 y = startY + static_cast<f32>(i) * lineH;
    if (y + lineH < rect.y || y > rect.y + rect.h) continue;
    // A small color marker per kind so the user can tell at a glance.
    Color dot = kText;
    switch (assets[i].kind) {
      case AssetKind::Model:  dot = {0.5f, 0.8f, 1.0f, 1.0f}; break;
      case AssetKind::Image:  dot = {0.9f, 0.7f, 0.4f, 1.0f}; break;
      case AssetKind::Scene:  dot = {0.6f, 1.0f, 0.6f, 1.0f}; break;
      default:                dot = kTextDim;
    }
    drawRect({rect.x + padX, y + 3.0f, 4.0f, 4.0f}, dot, 2.0f);
    drawText(assets[i].name.c_str(),
             rect.x + padX + 10.0f, y + 2.0f, 1, kText);
  }
  popClip();
}

}  // namespace kimia::ui
