// ObjectTree tests — see Engine/EditorUI/include/kimia/ObjectTree.h.

#include <kimia_test.h>
#include <kimia/ObjectTree.h>

KIMIA_TEST(ObjectTree_DrawEmptyDoesNotCrash) {
  kimia::ui::drawObjectTree({0, 0, 200, 300}, {}, 0);
}

KIMIA_TEST(ObjectTree_DrawOneEntry) {
  std::vector<kimia::ui::ObjectTreeEntry> v(1);
  v[0].name = "Cube_1";
  v[0].swatch = {0.8f, 0.4f, 0.2f, 1.0f};
  kimia::ui::drawObjectTree({0, 0, 200, 300}, v, 0);
}

KIMIA_TEST(ObjectTree_DrawManyEntries) {
  std::vector<kimia::ui::ObjectTreeEntry> v;
  for (int i = 0; i < 50; ++i) {
    kimia::ui::ObjectTreeEntry e;
    e.name = "Cube_" + std::to_string(i);
    e.swatch = {static_cast<kimia::f32>(i) * 0.02f, 0.5f, 0.5f, 1.0f};
    e.selected = (i % 3 == 0);
    v.push_back(e);
  }
  kimia::ui::drawObjectTree({0, 0, 200, 300}, v, 0);
  kimia::ui::drawObjectTree({0, 0, 200, 300}, v, -100);
  kimia::ui::drawObjectTree({0, 0, 200, 300}, v, 200);
}

KIMIA_TEST(ObjectTree_DrawWithLongName) {
  std::vector<kimia::ui::ObjectTreeEntry> v(1);
  v[0].name = "Very_Long_Entity_Name_That_Exceeds_The_Panel_Width_For_Truncation_Tests_1234567890";
  v[0].selected = true;
  kimia::ui::drawObjectTree({0, 0, 200, 300}, v, 0);
}

KIMIA_TEST(ObjectTree_DrawAtPhonePortrait) {
  std::vector<kimia::ui::ObjectTreeEntry> v;
  for (int i = 0; i < 20; ++i) {
    kimia::ui::ObjectTreeEntry e;
    e.name = "Entity_" + std::to_string(i);
    v.push_back(e);
  }
  kimia::ui::drawObjectTree({0, 0, 240, 320}, v, 0);
}

KIMIA_TEST(ObjectTree_DrawAtTabletLandscape) {
  std::vector<kimia::ui::ObjectTreeEntry> v;
  for (int i = 0; i < 30; ++i) {
    kimia::ui::ObjectTreeEntry e;
    e.name = "Entity_" + std::to_string(i);
    v.push_back(e);
  }
  kimia::ui::drawObjectTree({0, 0, 800, 600}, v, 0);
}

KIMIA_TEST(ObjectTree_EntryDefaultColor) {
  // Default-constructed entries: empty name, swatch gray, not selected.
  kimia::ui::ObjectTreeEntry e;
  KIMIA_REQUIRE(e.name.empty());
  KIMIA_REQUIRE(!e.selected);
  KIMIA_REQUIRE(e.swatch.r > 0.0f);  // swatch is some non-black default
}
