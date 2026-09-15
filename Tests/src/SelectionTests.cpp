// Selection tests — see Engine/EditorUI/include/kimia/Selection.h.

#include <kimia_test.h>
#include <kimia/Selection.h>

#include <vector>
#include <string>

KIMIA_TEST(Selection_SingleSelectReturnsOne) {
  const auto out = kimia::ui::singleSelect({}, "cube_1");
  KIMIA_REQUIRE(out.size() == 1);
  KIMIA_REQUIRE(out[0] == "cube_1");
}

KIMIA_TEST(Selection_ShiftTapAddToEmpty) {
  const auto out = kimia::ui::shiftTapSelect({}, "cube_1");
  KIMIA_REQUIRE(out.size() == 1);
  KIMIA_REQUIRE(out[0] == "cube_1");
}

KIMIA_TEST(Selection_ShiftTapAddToExisting) {
  const std::vector<std::string> prev{"cube_1"};
  const auto out = kimia::ui::shiftTapSelect(prev, "cube_2");
  KIMIA_REQUIRE(out.size() == 2);
}

KIMIA_TEST(Selection_ShiftTapRemoveExisting) {
  const std::vector<std::string> prev{"cube_1", "cube_2"};
  const auto out = kimia::ui::shiftTapSelect(prev, "cube_1");
  KIMIA_REQUIRE(out.size() == 1);
  KIMIA_REQUIRE(out[0] == "cube_2");
}

KIMIA_TEST(Selection_BoxSelectNoIntersection) {
  const std::vector<std::string> names{"a", "b", "c"};
  const std::vector<kimia::ui::Rect> rects{
    {0, 0, 10, 10}, {20, 20, 10, 10}, {40, 40, 10, 10}};
  // Box far from all rects.
  const auto out = kimia::ui::boxSelect(names, rects, {200, 200, 50, 50});
  KIMIA_REQUIRE(out.empty());
}

KIMIA_TEST(Selection_BoxSelectAll) {
  const std::vector<std::string> names{"a", "b", "c"};
  const std::vector<kimia::ui::Rect> rects{
    {0, 0, 10, 10}, {20, 20, 10, 10}, {40, 40, 10, 10}};
  // Box covers everything.
  const auto out = kimia::ui::boxSelect(names, rects, {-10, -10, 100, 100});
  KIMIA_REQUIRE(out.size() == 3);
}

KIMIA_TEST(Selection_BoxSelectMiddle) {
  const std::vector<std::string> names{"a", "b", "c"};
  const std::vector<kimia::ui::Rect> rects{
    {0, 0, 10, 10}, {20, 20, 10, 10}, {40, 40, 10, 10}};
  // Box covers only the middle rect (b).
  const auto out = kimia::ui::boxSelect(names, rects, {15, 15, 20, 20});
  KIMIA_REQUIRE(out.size() == 1);
  KIMIA_REQUIRE(out[0] == "b");
}

KIMIA_TEST(Selection_BoxSelectMismatchedLengths) {
  // When names and rects have different lengths, only the prefix is
  // considered. This is the defensive behaviour for when the Scene
  // View passes a partial list during a layout pass.
  const std::vector<std::string> names{"a", "b", "c"};
  const std::vector<kimia::ui::Rect> rects{{0, 0, 100, 100}};
  const auto out = kimia::ui::boxSelect(names, rects, {-10, -10, 100, 100});
  KIMIA_REQUIRE(out.size() == 1);
  KIMIA_REQUIRE(out[0] == "a");
}

KIMIA_TEST(Selection_ShiftTapOnSameNameTwiceTogglesAndUntoggles) {
  std::vector<std::string> s;
  s = kimia::ui::shiftTapSelect(s, "cube");
  KIMIA_REQUIRE(s.size() == 1);
  s = kimia::ui::shiftTapSelect(s, "cube");
  KIMIA_REQUIRE(s.empty());
}
