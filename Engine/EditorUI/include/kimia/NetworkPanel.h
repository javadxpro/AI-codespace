#pragma once
#include "EditorUI.h"
#include <vector>
#include <string>

namespace kimia::ui {

enum class ConnectionState { Connected, Connecting, Disconnected };

struct PeerEntry {
  std::string name;
  std::string address;
  i32 pingMs = 0;
  ConnectionState state = ConnectionState::Disconnected;
};

void drawNetworkPanel(const Rect& rect,
                      const std::vector<PeerEntry>& peers,
                      const std::string& localAddress);

}
