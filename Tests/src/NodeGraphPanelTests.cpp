#include <kimia_test.h>
#include <kimia/NodeGraphPanel.h>

KIMIA_TEST(NodeGraph_DrawEmptyDoesNotCrash) {
  kimia::ui::drawNodeGraphPanel({0, 0, 360, 240}, {}, {}, -1, 0, 0, 1.0f);
}

KIMIA_TEST(NodeGraph_DrawOneNode) {
  std::vector<kimia::ui::NodeItem> nodes(1);
  nodes[0].title = "Input";
  nodes[0].x = 20;
  nodes[0].y = 20;
  nodes[0].w = 100;
  nodes[0].h = 60;
  nodes[0].r = 80; nodes[0].g = 100; nodes[0].b = 160;
  nodes[0].pins.push_back({"Out", true});
  kimia::ui::drawNodeGraphPanel({0, 0, 360, 240}, nodes, {}, 0, 0, 0, 1.0f);
}

KIMIA_TEST(NodeGraph_DrawManyNodesAndConnections) {
  std::vector<kimia::ui::NodeItem> nodes;
  std::vector<kimia::ui::NodeConnection> conns;

  kimia::ui::NodeItem in;
  in.title = "In";
  in.x = 20; in.y = 60; in.w = 90; in.h = 60;
  in.r = 60; in.g = 100; in.b = 160;
  in.pins.push_back({"out", true});
  nodes.push_back(in);

  kimia::ui::NodeItem mid;
  mid.title = "Add";
  mid.x = 150; mid.y = 30; mid.w = 90; mid.h = 80;
  mid.r = 100; mid.g = 60; mid.b = 100;
  mid.pins.push_back({"a", false});
  mid.pins.push_back({"b", false});
  mid.pins.push_back({"sum", true});
  nodes.push_back(mid);

  kimia::ui::NodeItem out;
  out.title = "Out";
  out.x = 280; out.y = 60; out.w = 90; out.h = 60;
  out.r = 60; out.g = 160; out.b = 100;
  out.pins.push_back({"in", false});
  nodes.push_back(out);

  conns.push_back({0, 0, 1, 0});
  conns.push_back({1, 2, 2, 0});

  kimia::ui::drawNodeGraphPanel({0, 0, 480, 240}, nodes, conns, 1,
                                0, 0, 1.0f);
}

KIMIA_TEST(NodeGraph_DrawWithPan) {
  std::vector<kimia::ui::NodeItem> nodes(1);
  nodes[0].title = "Moved";
  nodes[0].x = 0; nodes[0].y = 0;
  nodes[0].w = 100; nodes[0].h = 60;
  nodes[0].pins.push_back({"out", true});
  kimia::ui::drawNodeGraphPanel({0, 0, 360, 240}, nodes, {}, 0,
                                50, -20, 1.0f);
}

KIMIA_TEST(NodeGraph_DrawWithZoom) {
  std::vector<kimia::ui::NodeItem> nodes(1);
  nodes[0].title = "Zoomed";
  nodes[0].x = 20; nodes[0].y = 20;
  nodes[0].w = 100; nodes[0].h = 60;
  nodes[0].pins.push_back({"out", true});
  // Zoom in.
  kimia::ui::drawNodeGraphPanel({0, 0, 360, 240}, nodes, {}, 0,
                                0, 0, 2.0f);
  // Zoom out.
  kimia::ui::drawNodeGraphPanel({0, 0, 360, 240}, nodes, {}, 0,
                                0, 0, 0.5f);
  // Zero zoom (no draw).
  kimia::ui::drawNodeGraphPanel({0, 0, 360, 240}, nodes, {}, 0,
                                0, 0, 0.0f);
}

KIMIA_TEST(NodeGraph_DrawWithInvalidConnections) {
  // Bad indices should be silently skipped.
  std::vector<kimia::ui::NodeItem> nodes(1);
  nodes[0].title = "N";
  nodes[0].x = 20; nodes[0].y = 20;
  nodes[0].w = 100; nodes[0].h = 60;
  nodes[0].pins.push_back({"p", true});
  std::vector<kimia::ui::NodeConnection> conns;
  conns.push_back({0, 0, 99, 0});
  conns.push_back({-1, 0, 0, 0});
  conns.push_back({5, 1, 0, 0});
  kimia::ui::drawNodeGraphPanel({0, 0, 360, 240}, nodes, conns, 0,
                                0, 0, 1.0f);
}

KIMIA_TEST(NodeGraph_DrawAtPhonePortrait) {
  std::vector<kimia::ui::NodeItem> nodes;
  for (int i = 0; i < 5; ++i) {
    kimia::ui::NodeItem n;
    n.title = "N" + std::to_string(i);
    n.x = i * 70; n.y = 20;
    n.w = 60; n.h = 50;
    n.pins.push_back({"out", true});
    nodes.push_back(n);
  }
  std::vector<kimia::ui::NodeConnection> conns;
  for (int i = 0; i < 4; ++i) {
    conns.push_back({i, 0, i + 1, 0});
  }
  kimia::ui::drawNodeGraphPanel({0, 0, 240, 320}, nodes, conns, 1,
                                0, 0, 1.0f);
}

KIMIA_TEST(NodeGraph_DrawAtTabletLandscape) {
  std::vector<kimia::ui::NodeItem> nodes;
  std::vector<kimia::ui::NodeConnection> conns;
  for (int i = 0; i < 10; ++i) {
    kimia::ui::NodeItem n;
    n.title = "T" + std::to_string(i);
    n.x = (i % 5) * 90; n.y = (i / 5) * 100;
    n.w = 80; n.h = 60;
    n.r = static_cast<kimia::u8>((i * 30) & 255);
    n.g = static_cast<kimia::u8>((i * 60) & 255);
    n.b = static_cast<kimia::u8>((i * 90) & 255);
    n.pins.push_back({"in", false});
    n.pins.push_back({"out", true});
    nodes.push_back(n);
    if (i > 0) conns.push_back({i - 1, 1, i, 0});
  }
  kimia::ui::drawNodeGraphPanel({0, 0, 480, 320}, nodes, conns, 4,
                                0, 0, 1.0f);
}
