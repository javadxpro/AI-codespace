#include <kimia_test.h>
#include <kimia/NetworkPanel.h>

KIMIA_TEST(Network_DrawEmptyDoesNotCrash) {
  kimia::ui::drawNetworkPanel({0, 0, 280, 200}, {}, "192.168.1.10");
}

KIMIA_TEST(Network_DrawOnePeer) {
  std::vector<kimia::ui::PeerEntry> v(1);
  v[0].name = "Server";
  v[0].address = "192.168.1.1:7777";
  v[0].pingMs = 12;
  v[0].state = kimia::ui::ConnectionState::Connected;
  kimia::ui::drawNetworkPanel({0, 0, 280, 200}, v, "192.168.1.10:7777");
}

KIMIA_TEST(Network_DrawManyPeers) {
  std::vector<kimia::ui::PeerEntry> v;
  const char* names[] = {"Server", "Alice", "Bob", "Charlie", "Dave"};
  for (int i = 0; i < 5; ++i) {
    kimia::ui::PeerEntry p;
    p.name = names[i];
    p.address = "10.0.0." + std::to_string(i + 1) + ":7777";
    p.pingMs = 10 + i * 30;
    p.state = static_cast<kimia::ui::ConnectionState>(i % 3);
    v.push_back(p);
  }
  kimia::ui::drawNetworkPanel({0, 0, 320, 240}, v, "10.0.0.100:7777");
}

KIMIA_TEST(Network_DrawWithHighLatency) {
  std::vector<kimia::ui::PeerEntry> v(1);
  v[0].name = "Slow";
  v[0].address = "10.0.0.1:7777";
  v[0].pingMs = 500;
  v[0].state = kimia::ui::ConnectionState::Connected;
  kimia::ui::drawNetworkPanel({0, 0, 280, 100}, v, "");
}

KIMIA_TEST(Network_DrawAtPhonePortrait) {
  std::vector<kimia::ui::PeerEntry> v;
  for (int i = 0; i < 3; ++i) {
    kimia::ui::PeerEntry p;
    p.name = "P" + std::to_string(i);
    p.address = "127.0.0.1";
    v.push_back(p);
  }
  kimia::ui::drawNetworkPanel({0, 0, 240, 320}, v, "127.0.0.1");
}

KIMIA_TEST(Network_DrawAtTabletLandscape) {
  std::vector<kimia::ui::PeerEntry> v;
  for (int i = 0; i < 12; ++i) {
    kimia::ui::PeerEntry p;
    p.name = "peer_" + std::to_string(i);
    p.address = "192.168.1." + std::to_string(i + 1);
    p.pingMs = i * 20;
    p.state = static_cast<kimia::ui::ConnectionState>(i % 3);
    v.push_back(p);
  }
  kimia::ui::drawNetworkPanel({0, 0, 480, 320}, v, "192.168.1.100");
}
