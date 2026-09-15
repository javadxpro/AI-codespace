#include <kimia_test.h>
#include <kimia/HierarchyPanel.h>

KIMIA_TEST(Hierarchy_DrawEmptyDoesNotCrash) {
  kimia::ui::drawHierarchyPanel({0, 0, 280, 240}, {}, -1, 0);
}

KIMIA_TEST(Hierarchy_DrawOneNode) {
  std::vector<kimia::ui::HierarchyNode> v(1);
  v[0] = {"Root", 0, true, true, -1};
  kimia::ui::drawHierarchyPanel({0, 0, 280, 240}, v, 0, 0);
}

KIMIA_TEST(Hierarchy_DrawManyNodes) {
  std::vector<kimia::ui::HierarchyNode> v;
  v.push_back({"World",       0, true,  true,  -1});
  v.push_back({"Player",      1, true,  true,   0});
  v.push_back({"Mesh",        2, false, true,   1});
  v.push_back({"Camera",      1, true,  true,   0});
  v.push_back({"MainCam",     2, true,  true,   3});
  v.push_back({"Light",       1, true,  true,   0});
  v.push_back({"Enemies",     1, true,  true,   0});
  v.push_back({"Ball_1",      2, true,  true,   6});
  v.push_back({"Ball_2",      2, true,  false,  6});
  v.push_back({"Hidden",      0, false, false, -1});
  kimia::ui::drawHierarchyPanel({0, 0, 320, 280}, v, 1, 0);
}

KIMIA_TEST(Hierarchy_DrawAtScroll) {
  std::vector<kimia::ui::HierarchyNode> v;
  for (int i = 0; i < 50; ++i) {
    v.push_back({"Node_" + std::to_string(i), i % 4,
                 i % 3 != 0, i % 5 != 0, -1});
  }
  kimia::ui::drawHierarchyPanel({0, 0, 280, 200}, v, 0, -100);
  kimia::ui::drawHierarchyPanel({0, 0, 280, 200}, v, 0, 100);
}

KIMIA_TEST(Hierarchy_DrawWithNegativeSelectedIndex) {
  std::vector<kimia::ui::HierarchyNode> v(1);
  v[0] = {"X", 0, true, true, -1};
  kimia::ui::drawHierarchyPanel({0, 0, 280, 200}, v, -5, 0);
}

KIMIA_TEST(Hierarchy_DrawWithOutOfRangeSelected) {
  std::vector<kimia::ui::HierarchyNode> v(1);
  v[0] = {"Y", 0, true, true, -1};
  kimia::ui::drawHierarchyPanel({0, 0, 280, 200}, v, 99, 0);
}

KIMIA_TEST(Hierarchy_DrawAtPhonePortrait) {
  std::vector<kimia::ui::HierarchyNode> v;
  v.push_back({"Root", 0, true, true, -1});
  v.push_back({"Child1", 1, true, true, 0});
  v.push_back({"Child2", 1, true, true, 0});
  v.push_back({"Child3", 1, true, false, 0});
  kimia::ui::drawHierarchyPanel({0, 0, 240, 320}, v, 1, 0);
}

KIMIA_TEST(Hierarchy_DrawAtTabletLandscape) {
  std::vector<kimia::ui::HierarchyNode> v;
  for (int i = 0; i < 30; ++i) {
    v.push_back({"N" + std::to_string(i), i / 5,
                 true, (i % 4) != 0, -1});
  }
  kimia::ui::drawHierarchyPanel({0, 0, 480, 320}, v, 5, 0);
}
