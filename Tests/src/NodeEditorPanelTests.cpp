#include <kimia_test.h>
#include <kimia/NodeEditorPanel.h>

KIMIA_TEST(NodeEditor_DrawEmptyDoesNotCrash) {
  kimia::ui::drawNodeEditorPanel({0, 0, 320, 240}, {}, {}, {0, 0});
}

KIMIA_TEST(NodeEditor_DrawOneNode) {
  std::vector<kimia::ui::NodeGraphNode> nodes(1);
  nodes[0].name = "Source";
  nodes[0].position = {10.0, 10.0};
  nodes[0].outputs.push_back("out");
  kimia::ui::drawNodeEditorPanel({0, 0, 320, 240}, nodes, {}, {0, 0});
}

KIMIA_TEST(NodeEditor_DrawTwoNodesWithLink) {
  std::vector<kimia::ui::NodeGraphNode> nodes(2);
  nodes[0].name = "A";
  nodes[0].position = {10.0, 10.0};
  nodes[0].outputs.push_back("o1");
  nodes[1].name = "B";
  nodes[1].position = {150.0, 10.0};
  nodes[1].inputs.push_back("i1");
  std::vector<kimia::ui::NodeGraphLink> links(1);
  links[0].fromNode = 0;
  links[0].fromOutput = 0;
  links[0].toNode = 1;
  links[0].toInput = 0;
  kimia::ui::drawNodeEditorPanel({0, 0, 320, 240}, nodes, links, {0, 0});
}

KIMIA_TEST(NodeEditor_DrawManyNodesAndLinks) {
  std::vector<kimia::ui::NodeGraphNode> nodes;
  for (int i = 0; i < 6; ++i) {
    kimia::ui::NodeGraphNode n;
    n.name = "n" + std::to_string(i);
    n.position = {static_cast<double>(i) * 80.0, 0.0};
    n.inputs.push_back("in");
    n.outputs.push_back("out");
    nodes.push_back(n);
  }
  std::vector<kimia::ui::NodeGraphLink> links;
  for (int i = 0; i + 1 < 6; ++i) {
    kimia::ui::NodeGraphLink l;
    l.fromNode = i;
    l.toNode = i + 1;
    links.push_back(l);
  }
  kimia::ui::drawNodeEditorPanel({0, 0, 480, 240}, nodes, links, {0, 0});
  kimia::ui::drawNodeEditorPanel({0, 0, 480, 240}, nodes, links, {100.0, 50.0});
}

KIMIA_TEST(NodeEditor_DrawWithInvalidLinkIndices) {
  // Invalid link indices (out of range) must be skipped, not crash.
  std::vector<kimia::ui::NodeGraphNode> nodes(1);
  nodes[0].name = "A";
  std::vector<kimia::ui::NodeGraphLink> links(2);
  links[0].fromNode = 0; links[0].toNode = -1;
  links[1].fromNode = -1; links[1].toNode = 0;
  kimia::ui::drawNodeEditorPanel({0, 0, 320, 240}, nodes, links, {0, 0});
}

KIMIA_TEST(NodeEditor_DrawAtPhonePortrait) {
  std::vector<kimia::ui::NodeGraphNode> nodes(1);
  nodes[0].name = "X";
  nodes[0].outputs.push_back("y");
  kimia::ui::drawNodeEditorPanel({0, 0, 240, 320}, nodes, {}, {0, 0});
}

KIMIA_TEST(NodeEditor_DrawAtTabletLandscape) {
  std::vector<kimia::ui::NodeGraphNode> nodes(3);
  for (int i = 0; i < 3; ++i) {
    nodes[i].name = "N" + std::to_string(i);
    nodes[i].position = {static_cast<double>(i) * 120.0, 50.0};
    nodes[i].outputs.push_back("o");
  }
  std::vector<kimia::ui::NodeGraphLink> links(2);
  links[0].fromNode = 0; links[0].toNode = 1;
  links[1].fromNode = 1; links[1].toNode = 2;
  nodes[1].inputs.push_back("i");
  nodes[2].inputs.push_back("i");
  kimia::ui::drawNodeEditorPanel({0, 0, 480, 320}, nodes, links, {0, 0});
}
