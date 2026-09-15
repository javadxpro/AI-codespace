#include <kimia/NetworkPanel.h>
#include <kimia/EditorUI.h>
#include <kimia/Widget.h>
#include <kimia/Theme.h>
#include <cstdio>

namespace kimia::ui {

namespace {
const char* stateName(ConnectionState s) {
  switch (s) {
    case ConnectionState::Connected:    return "OK";
    case ConnectionState::Connecting:   return "...";
    case ConnectionState::Disconnected: return "OFF";
  }
  return "?";
}
Color stateColor(ConnectionState s) {
  using namespace theme;
  switch (s) {
    case ConnectionState::Connected:    return kSuccess;
    case ConnectionState::Connecting:   return kAccent;
    case ConnectionState::Disconnected: return kTextMuted;
  }
  return kText;
}
}

void drawNetworkPanel(const Rect& rect,
                      const std::vector<PeerEntry>& peers,
                      const std::string& localAddress) {
  using namespace theme;
  drawRect(rect, kPanel, 0.0f);
  drawText("Network", rect.x + 6.0f, rect.y + 4.0f, 1, kText);

  // Local address row.
  drawText("Local:", rect.x + 4.0f, rect.y + 18.0f, 1, kTextMuted);
  drawText(localAddress.c_str(),
           rect.x + 50.0f, rect.y + 18.0f, 1, kText);

  pushClip(rect);
  constexpr f32 rowH = 18.0f;
  const f32 startY = rect.y + 36.0f;
  char buf[32];
  for (std::size_t i = 0; i < peers.size(); ++i) {
    const f32 y = startY + static_cast<float>(i) * rowH;
    if (y + rowH > rect.y + rect.h) break;

    drawText(peers[i].name.c_str(),
             rect.x + 4.0f, y + 4.0f, 1, kText);
    drawText(peers[i].address.c_str(),
             rect.x + 80.0f, y + 4.0f, 1, kTextMuted);
    drawText(stateName(peers[i].state),
             rect.x + rect.w - 70.0f, y + 4.0f, 1,
             stateColor(peers[i].state));
    std::snprintf(buf, sizeof(buf), "%d ms", peers[i].pingMs);
    drawText(buf, rect.x + rect.w - 30.0f, y + 4.0f,
             1, peers[i].pingMs < 50 ? kSuccess :
                (peers[i].pingMs < 150 ? kText : kWarning));
  }
  popClip();
}

}
