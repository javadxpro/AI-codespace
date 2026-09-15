// SearchPanel tests — see Engine/EditorUI/include/kimia/SearchPanel.h.

#include <kimia_test.h>
#include <kimia/SearchPanel.h>

KIMIA_TEST(Search_MatchEmptyNeedleMatchesEverything) {
  KIMIA_REQUIRE(kimia::ui::searchMatch("", ""));
  KIMIA_REQUIRE(kimia::ui::searchMatch("", "anything"));
  KIMIA_REQUIRE(kimia::ui::searchMatch("", "Cube_1"));
}

KIMIA_TEST(Search_MatchCaseInsensitive) {
  KIMIA_REQUIRE(kimia::ui::searchMatch("cube", "Cube_1"));
  KIMIA_REQUIRE(kimia::ui::searchMatch("CUBE", "cube_1"));
  KIMIA_REQUIRE(kimia::ui::searchMatch("CuBe", "cUbE_1"));
}

KIMIA_TEST(Search_MatchSubstring) {
  KIMIA_REQUIRE(kimia::ui::searchMatch("ub", "Cube_1"));
  KIMIA_REQUIRE(kimia::ui::searchMatch("e_1", "Cube_1"));
}

KIMIA_TEST(Search_NoMatch) {
  KIMIA_REQUIRE(!kimia::ui::searchMatch("xyz", "Cube_1"));
  KIMIA_REQUIRE(!kimia::ui::searchMatch("Player", "Cube_1"));
}

KIMIA_TEST(Search_FilterEmptyReturnsAll) {
  std::vector<std::string> names{"a", "b", "c"};
  const auto out = kimia::ui::searchFilter("", names);
  KIMIA_REQUIRE(out.size() == 3);
  KIMIA_REQUIRE(out[0] == 0);
  KIMIA_REQUIRE(out[1] == 1);
  KIMIA_REQUIRE(out[2] == 2);
}

KIMIA_TEST(Search_FilterCaseInsensitive) {
  std::vector<std::string> names{"Cube_1", "Sphere_1", "Cube_2", "Player_1"};
  const auto out = kimia::ui::searchFilter("cube", names);
  KIMIA_REQUIRE(out.size() == 2);
  KIMIA_REQUIRE(out[0] == 0);
  KIMIA_REQUIRE(out[1] == 2);
}

KIMIA_TEST(Search_FilterSubstring) {
  std::vector<std::string> names{"Cube_1", "Cube_2", "Sphere_1"};
  const auto out = kimia::ui::searchFilter("ub", names);
  KIMIA_REQUIRE(out.size() == 2);
}

KIMIA_TEST(Search_FilterEmptyNames) {
  std::vector<std::string> names{};
  const auto out = kimia::ui::searchFilter("cube", names);
  KIMIA_REQUIRE(out.empty());
}

KIMIA_TEST(Search_FilterNoMatch) {
  std::vector<std::string> names{"Cube_1", "Sphere_1"};
  const auto out = kimia::ui::searchFilter("xyz", names);
  KIMIA_REQUIRE(out.empty());
}

KIMIA_TEST(Search_DrawWithEmptyQueryDoesNotCrash) {
  kimia::ui::drawSearchPanel({0, 0, 200, 20}, "", "Search entities…");
}

KIMIA_TEST(Search_DrawWithQueryDoesNotCrash) {
  kimia::ui::drawSearchPanel({0, 0, 200, 20}, "cube", "Search entities…");
}

KIMIA_TEST(Search_DrawAtPhonePortrait) {
  kimia::ui::drawSearchPanel({0, 0, 240, 20}, "abc", "Search…");
}

KIMIA_TEST(Search_DrawAtTabletLandscape) {
  kimia::ui::drawSearchPanel({0, 0, 800, 20}, "def", "Search…");
}
