#include <kimia/BitmapFont.h>
#include <kimia/Hud.h>

#include <algorithm>
#include <cmath>
#include <sstream>

namespace kimia {

namespace {

// A number reads as a person writes it: 3, not 3.000000.
std::string tidyNumber(f64 value) {
  std::ostringstream text;
  text.precision(3);
  text << std::fixed << value;
  std::string out = text.str();
  while (out.size() > 1U && out.back() == '0') out.pop_back();
  if (!out.empty() && out.back() == '.') out.pop_back();
  return out;
}

// Fractions of the screen become pixels here, so a layout survives being
// shown at a different size.
i32 toPixels(f64 fraction, i32 span) {
  const f64 value = fraction * static_cast<f64>(span);
  return static_cast<i32>(std::lround(value));
}

}  // namespace

const char* panelKindName(PanelKind kind) {
  switch (kind) {
    case PanelKind::Bar: return "bar";
    case PanelKind::Box: return "box";
    case PanelKind::Button: return "button";
    case PanelKind::Label: break;
  }
  return "label";
}

bool panelKindFromName(const std::string& name, PanelKind& out) {
  if (name == "label") {
    out = PanelKind::Label;
    return true;
  }
  if (name == "bar") {
    out = PanelKind::Bar;
    return true;
  }
  if (name == "box") {
    out = PanelKind::Box;
    return true;
  }
  if (name == "button") {
    out = PanelKind::Button;
    return true;
  }
  return false;
}

const Panel* HudLayout::find(const std::string& name) const {
  for (const Panel& panel : panels) {
    if (panel.name == name) return &panel;
  }
  return nullptr;
}

Panel* HudLayout::find(const std::string& name) {
  for (Panel& panel : panels) {
    if (panel.name == name) return &panel;
  }
  return nullptr;
}

void HudLayout::set(const Panel& panel) {
  if (panel.name.empty()) return;
  for (Panel& existing : panels) {
    if (existing.name != panel.name) continue;
    existing = panel;  // moving a panel is a replace, not a second panel
    return;
  }
  panels.push_back(panel);
}

bool HudLayout::remove(const std::string& name) {
  for (usize i = 0; i < panels.size(); ++i) {
    if (panels[i].name != name) continue;
    panels.erase(panels.begin() + static_cast<std::ptrdiff_t>(i));
    return true;
  }
  return false;
}

std::string fillPlaceholders(const std::string& text, const LogicBook& book) {
  std::string out;
  out.reserve(text.size());
  for (usize i = 0; i < text.size(); ++i) {
    if (text[i] != '{') {
      out += text[i];
      continue;
    }
    const usize close = text.find('}', i);
    if (close == std::string::npos) {
      // An unclosed brace is just text, not the start of an endless read.
      out += text[i];
      continue;
    }
    const std::string name = text.substr(i + 1U, close - i - 1U);
    const Variable* variable = book.find(name);
    if (variable != nullptr) {
      out += variable->isText ? variable->text : tidyNumber(variable->number);
    }
    // An unknown variable leaves nothing behind: a typo should look wrong
    // on screen rather than showing the braces to the player.
    i = close;
  }
  return out;
}

void drawHud(Image& image, const HudLayout& layout, const LogicBook& book) {
  if (image.width <= 0 || image.height <= 0) return;
  for (const Panel& panel : layout.panels) {
    if (!panel.visible) continue;
    const i32 x = toPixels(panel.x, image.width);
    const i32 y = toPixels(panel.y, image.height);
    const i32 width = std::max(1, toPixels(panel.width, image.width));
    const i32 height = std::max(1, toPixels(panel.height, image.height));

    switch (panel.kind) {
      case PanelKind::Box:
        font::fillRect(image, x, y, width, height, panel.background, panel.opacity);
        break;

      case PanelKind::Bar: {
        const f64 value = book.numberOf(panel.variable);
        // A bar with no maximum would divide by zero; treat it as empty.
        const f64 fraction = panel.maximum > 0.0 ? value / panel.maximum : 0.0;
        font::drawBar(image, x, y, width, height, std::min(1.0, std::max(0.0, fraction)), panel.color,
                      panel.background);
        break;
      }

      case PanelKind::Label:
      case PanelKind::Button: {
        // A button gets a backdrop so it looks pressable; a plain label
        // only gets one if the user asked for opacity.
        if (panel.kind == PanelKind::Button || panel.opacity > 0.0) {
          font::fillRect(image, x, y, width, height, panel.background, panel.opacity);
        }
        const std::string shown = fillPlaceholders(panel.text, book);
        // Centre the text in its panel, which is what people expect of a
        // button and looks right for a label too.
        const i32 textW = font::textWidth(shown, panel.scale);
        const i32 textH = font::textHeight(panel.scale);
        font::drawText(image, x + std::max(0, (width - textW) / 2), y + std::max(0, (height - textH) / 2),
                       shown, panel.color, panel.scale);
        break;
      }
    }
  }
}

std::string buttonAt(const HudLayout& layout, i32 imageWidth, i32 imageHeight, f64 pixelX, f64 pixelY) {
  if (imageWidth <= 0 || imageHeight <= 0) return std::string();
  // Later panels are drawn on top, so they are checked first: a person
  // means the button they can see.
  for (usize i = layout.panels.size(); i > 0U; --i) {
    const Panel& panel = layout.panels[i - 1U];
    if (!panel.visible || panel.kind != PanelKind::Button) continue;
    const f64 left = panel.x * static_cast<f64>(imageWidth);
    const f64 top = panel.y * static_cast<f64>(imageHeight);
    const f64 right = left + panel.width * static_cast<f64>(imageWidth);
    const f64 bottom = top + panel.height * static_cast<f64>(imageHeight);
    if (pixelX < left || pixelX > right || pixelY < top || pixelY > bottom) continue;
    return panel.name;
  }
  return std::string();
}

}  // namespace kimia
