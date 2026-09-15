#include <kimia_test.h>
#include <kimia/HierarchyTreePanel.h>

KIMIA_TEST(HierarchyTree_DrawEmptyDoesNotCrash) {
  kimia::ui::drawHierarchyTreePanel({0, 0, 200, 300}, {}, 0);
}

KIMIA_TEST(HierarchyTree_DrawSingleRoot) {
  std::vector<kimia::ui::HierarchyNode> v(1);
  v[0].name = "Root";
  v[0].depth = 0;
  v[0].hasChildren = false;
  kimia::ui::drawHierarchyTreePanel({0, 0, 200, 300}, v, 0);
}

KIMIA_TEST(HierarchyTree_DrawNestedTree) {
  std::vector<kimia::ui::HierarchyNode> v;
  v.push_back({"Player",  0, true, true,  true});
  v.push_back({"Body",    1, true, true,  false});
  v.push_back({"Head",    2, true, false, false});
  v.push_back({"Arms",    2, true, true,  false});
  v.push_back({"LeftArm", 3, true, false, false});
  v.push_back({"RightArm",3, true, false, true});
  v.push_back({"Ball",    0, true, false, false});
  v.push_back({"Ground",  0, true, false, false});
  kimia::ui::drawHierarchyTreePanel({0, 0, 240, 320}, v, 0);
  kimia::ui::drawHierarchyTreePanel({0, 0, 240, 320}, v, -50);
  kimia::ui::drawHierarchyTreePanel({0, 0, 240, 320}, v, 100);
}

KIMIA_TEST(HierarchyTree_DrawCollapsed) {
  std::vector<kimia::ui::HierarchyNode> v(1);
  v[0].name = "Player";
  v[0].hasChildren = true;
  v[0].expanded = false;
  v[0].selected = true;
  kimia::ui::drawHierarchyTreePanel({0, 0, 200, 300}, v, 0);
}

KIMIA_TEST(HierarchyTree_DrawDeepNesting) {
  std::vector<kimia::ui::HierarchyNode> v(1);
  v[0].name = "deep";
  v[0].depth = 8;
  v[0].hasChildren = false;
  kimia::ui::drawHierarchyTreePanel({0, 0, 300, 200}, v, 0);
}

KIMIA_TEST(HierarchyTree_DrawAtPhonePortrait) {
  std::vector<kimia::ui::HierarchyNode> v;
  for (int i = 0; i < 15; ++i) {
    kimia::ui::HierarchyNode n;
    n.name = "n" + std::to_string(i);
    n.depth = i % 3;
    n.hasChildren = (i % 2 == 0);
    n.selected = (i == 5);
    v.push_back(n);
  }
  kimia::ui::drawHierarchyTreePanel({0, 0, 240, 320}, v, 0);
}

KIMIA_TEST(HierarchyTree_DrawAtTabletLandscape) {
  std::vector<kimia::ui::HierarchyNode> v;
  for (int i = 0; i < 30; ++i) {
    kimia::ui::HierarchyNode n;
    n.name = "n" + std::to_string(i);
    n.depth = i % 4;
    n.hasChildren = (i % 3 == 0);
    v.push_back(n);
  }
  kimia::ui::drawHierarchyTreePanel({0, 0, 800, 400}, v, 0);
}
