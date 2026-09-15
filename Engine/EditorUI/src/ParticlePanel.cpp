// ParticlePanel implementation — see ParticlePanel.h.
#include <kimia/ParticlePanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>

#include <cstdio>

namespace kimia::ui {

namespace {

void formatFloat(char* buf, usize n, f32 v) {
  std::snprintf(buf, n, "%.2f", static_cast<double>(v));
}

void formatVec3(char* buf, usize n, const Vec3& v) {
  std::snprintf(buf, n, "%.2f, %.2f, %.2f",
                static_cast<double>(v.x), static_cast<double>(v.y),
                static_cast<double>(v.z));
}

void drawRow(const Rect& row, const char* label, const char* value) {
  using namespace theme;
  constexpr f32 labelW = 70.0f;
  drawText(label, row.x + 4.0f, row.y + 4.0f, 1, kText);
  drawRect({row.x + labelW, row.y + 1.0f,
            row.w - labelW - 4.0f, row.h - 2.0f},
           kPanelAlt, 2.0f);
  drawText(value, row.x + labelW + 4.0f, row.y + 4.0f, 1, kText);
}

}  // namespace

bool drawParticlePanel(const Rect& rect, const ParticleProps& current,
                       ParticleProps& edited) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Particle Emitter", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  constexpr f32 rowH = 18.0f;
  f32 y = rect.y + 18.0f;
  char buf[64];

  formatFloat(buf, sizeof(buf), current.spawnRate);
  drawRow({rect.x, y, rect.w, rowH}, "Rate (1/s)", buf);
  y += rowH;

  std::snprintf(buf, sizeof(buf), "%.2f – %.2f s",
                static_cast<double>(current.lifeMin),
                static_cast<double>(current.lifeMax));
  drawRow({rect.x, y, rect.w, rowH}, "Life", buf);
  y += rowH;

  std::snprintf(buf, sizeof(buf), "%.2f – %.2f",
                static_cast<double>(current.sizeMin),
                static_cast<double>(current.sizeMax));
  drawRow({rect.x, y, rect.w, rowH}, "Size", buf);
  y += rowH;

  std::snprintf(buf, sizeof(buf), "%.2f – %.2f",
                static_cast<double>(current.speedMin),
                static_cast<double>(current.speedMax));
  drawRow({rect.x, y, rect.w, rowH}, "Speed", buf);
  y += rowH;

  formatFloat(buf, sizeof(buf), current.gravity);
  drawRow({rect.x, y, rect.w, rowH}, "Gravity", buf);
  y += rowH;

  formatVec3(buf, sizeof(buf), current.colorStart);
  drawRow({rect.x, y, rect.w, rowH}, "Color start", buf);
  y += rowH;

  formatVec3(buf, sizeof(buf), current.colorEnd);
  drawRow({rect.x, y, rect.w, rowH}, "Color end", buf);
  y += rowH + 2.0f;

  if (checkbox("Looped", edited.looped,
               {rect.x + 4.0f, y, rect.w * 0.5f - 4.0f, rowH})) {
    // checkbox flips in place
  }
  if (checkbox("Additive", edited.additive,
               {rect.x + rect.w * 0.5f, y, rect.w * 0.5f - 4.0f, rowH})) {
    // checkbox flips in place
  }

  (void)edited;
  return false;  // Phase 4+ is read-only.
}

}  // namespace kimia::ui
