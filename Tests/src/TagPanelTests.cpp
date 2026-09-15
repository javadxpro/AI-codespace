#include <kimia_test.h>
#include <kimia/TagPanel.h>

KIMIA_TEST(TagPanel_DrawEmptyDoesNotCrash) {
  kimia::ui::drawTagPanel({0, 0, 240, 200}, {}, "");
}

KIMIA_TEST(TagPanel_DrawOneTag) {
  std::vector<kimia::ui::Tag> v(1);
  v[0].name = "Player";
  v[0].color = {0.3f, 0.8f, 0.3f, 1.0f};
  kimia::ui::drawTagPanel({0, 0, 240, 200}, v, "");
}

KIMIA_TEST(TagPanel_DrawManyTags) {
  std::vector<kimia::ui::Tag> v;
  const char* names[] = {"Player", "Enemy", "NPC", "Static",
                         "Dynamic", "Trigger", "Pickup", "Boss", "Friendly"};
  const kimia::ui::Color colors[] = {
    {0.3f, 0.8f, 0.3f, 1.0f},
    {0.8f, 0.3f, 0.3f, 1.0f},
    {0.3f, 0.3f, 0.8f, 1.0f},
    {0.6f, 0.6f, 0.6f, 1.0f},
    {0.8f, 0.6f, 0.3f, 1.0f},
    {0.8f, 0.3f, 0.8f, 1.0f},
    {0.8f, 0.8f, 0.3f, 1.0f},
    {0.5f, 0.0f, 0.0f, 1.0f},
    {0.3f, 0.8f, 0.6f, 1.0f},
  };
  for (int i = 0; i < 9; ++i) {
    kimia::ui::Tag t;
    t.name = names[i];
    t.color = colors[i];
    v.push_back(t);
  }
  kimia::ui::drawTagPanel({0, 0, 240, 200}, v, "new");
}

KIMIA_TEST(TagPanel_DrawWithNewTagInput) {
  std::vector<kimia::ui::Tag> v;
  v.push_back({"Existing", {0.5f, 0.5f, 0.5f, 1.0f}});
  kimia::ui::drawTagPanel({0, 0, 240, 200}, v, "Boss");
  kimia::ui::drawTagPanel({0, 0, 240, 200}, v, "");
}

KIMIA_TEST(TagPanel_DrawAtPhonePortrait) {
  std::vector<kimia::ui::Tag> v;
  for (int i = 0; i < 5; ++i) {
    kimia::ui::Tag t;
    t.name = "t" + std::to_string(i);
    v.push_back(t);
  }
  kimia::ui::drawTagPanel({0, 0, 240, 320}, v, "");
}

KIMIA_TEST(TagPanel_DrawAtTabletLandscape) {
  std::vector<kimia::ui::Tag> v;
  for (int i = 0; i < 15; ++i) {
    kimia::ui::Tag t;
    t.name = "tag_" + std::to_string(i);
    t.color = {static_cast<float>(i) * 0.05f, 0.5f, 0.5f, 1.0f};
    v.push_back(t);
  }
  kimia::ui::drawTagPanel({0, 0, 480, 280}, v, "");
}
