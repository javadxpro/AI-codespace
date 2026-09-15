#include <kimia_test.h>
#include <kimia/DockLayoutPanel.h>

KIMIA_TEST(DockLayout_DrawEmptyDoesNotCrash) {
  kimia::ui::Rect center;
  kimia::ui::drawDockLayoutPanel({0, 0, 320, 240}, {}, center);
  // Empty layout → center is the full rect.
  KIMIA_REQUIRE(center.w == 320.0f);
  KIMIA_REQUIRE(center.h == 240.0f);
}

KIMIA_TEST(DockLayout_DrawLeftOnly) {
  std::vector<kimia::ui::DockArea> v;
  kimia::ui::DockArea l;
  l.side = kimia::ui::DockSide::Left;
  l.size = 100.0f;
  l.tabs.push_back({"Hierarchy", "H"});
  v.push_back(l);
  kimia::ui::Rect center;
  kimia::ui::drawDockLayoutPanel({0, 0, 400, 240}, v, center);
  KIMIA_REQUIRE(center.x == 100.0f);
}

KIMIA_TEST(DockLayout_DrawAllSides) {
  std::vector<kimia::ui::DockArea> v;
  kimia::ui::DockArea left;
  left.side = kimia::ui::DockSide::Left;
  left.size = 80.0f;
  left.tabs.push_back({"Hierarchy", "H"});
  v.push_back(left);
  kimia::ui::DockArea right;
  right.side = kimia::ui::DockSide::Right;
  right.size = 100.0f;
  right.tabs.push_back({"Inspector", "I"});
  v.push_back(right);
  kimia::ui::DockArea top;
  top.side = kimia::ui::DockSide::Top;
  top.size = 20.0f;
  top.tabs.push_back({"Toolbar", "T"});
  v.push_back(top);
  kimia::ui::DockArea bottom;
  bottom.side = kimia::ui::DockSide::Bottom;
  bottom.size = 60.0f;
  bottom.tabs.push_back({"Log", "L"});
  v.push_back(bottom);
  kimia::ui::Rect center;
  kimia::ui::drawDockLayoutPanel({0, 0, 480, 320}, v, center);
  KIMIA_REQUIRE(center.w == 300.0f);
  KIMIA_REQUIRE(center.h == 240.0f);
}

KIMIA_TEST(DockLayout_DrawWithManyTabsInOneDock) {
  std::vector<kimia::ui::DockArea> v;
  kimia::ui::DockArea left;
  left.side = kimia::ui::DockSide::Left;
  left.size = 200.0f;
  left.tabs.push_back({"Hierarchy", "H"});
  left.tabs.push_back({"Scenes", "S"});
  left.tabs.push_back({"Assets", "A"});
  left.tabs.push_back({"Console", "C"});
  left.activeTab = 1;
  v.push_back(left);
  kimia::ui::Rect center;
  kimia::ui::drawDockLayoutPanel({0, 0, 400, 240}, v, center);
}

KIMIA_TEST(DockLayout_DrawAtPhonePortrait) {
  std::vector<kimia::ui::DockArea> v;
  kimia::ui::DockArea l;
  l.side = kimia::ui::DockSide::Left;
  l.size = 60.0f;
  l.tabs.push_back({"H", "H"});
  v.push_back(l);
  kimia::ui::Rect center;
  kimia::ui::drawDockLayoutPanel({0, 0, 240, 320}, v, center);
  KIMIA_REQUIRE(center.w == 180.0f);
}

KIMIA_TEST(DockLayout_DrawAtTabletLandscape) {
  std::vector<kimia::ui::DockArea> v;
  kimia::ui::DockArea l;
  l.side = kimia::ui::DockSide::Left;
  l.size = 120.0f;
  l.tabs.push_back({"H", "H"});
  v.push_back(l);
  kimia::ui::DockArea r;
  r.side = kimia::ui::DockSide::Right;
  r.size = 120.0f;
  r.tabs.push_back({"I", "I"});
  v.push_back(r);
  kimia::ui::Rect center;
  kimia::ui::drawDockLayoutPanel({0, 0, 800, 400}, v, center);
  KIMIA_REQUIRE(center.w == 560.0f);
}
