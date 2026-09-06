#include <kimia/Hud.h>
#include <kimia_test.h>

#include <cmath>
#include <string>

namespace {

using kimia::HudLayout;
using kimia::Image;
using kimia::LogicBook;
using kimia::Panel;
using kimia::PanelKind;
using kimia::Vec3;
using kimia::f64;
using kimia::i32;
using kimia::u8;
using kimia::usize;

Panel makePanel(const char* name, PanelKind kind) {
  Panel panel;
  panel.name = name;
  panel.kind = kind;
  return panel;
}

// A blank frame to draw a HUD over.
Image blankFrame(i32 width = 320, i32 height = 240) {
  Image image;
  image.width = width;
  image.height = height;
  image.channels = 3;
  image.pixels.assign(static_cast<usize>(width) * static_cast<usize>(height) * 3U, 0U);
  return image;
}

usize litPixels(const Image& image) {
  usize count = 0;
  for (usize i = 0; i + 2U < image.pixels.size(); i += 3U) {
    if (image.pixels[i] != 0U || image.pixels[i + 1U] != 0U || image.pixels[i + 2U] != 0U) ++count;
  }
  return count;
}

}  // namespace

// --- The game's own interface, laid out by the user ---

KIMIA_TEST(hud_text_reads_the_game_variables) {
  // "Score: {score}" has to show the game's real number, or a HUD is just
  // a fixed caption.
  LogicBook book;
  book.setNumber("score", 7.0);
  book.setNumber("lives", 3.0);
  book.setText("player", "Kimia");

  KIMIA_REQUIRE(kimia::fillPlaceholders("Score: {score}", book) == "Score: 7");
  KIMIA_REQUIRE(kimia::fillPlaceholders("{lives} lives", book) == "3 lives");
  KIMIA_REQUIRE(kimia::fillPlaceholders("{player} has {score}", book) == "Kimia has 7");
  // Numbers read as a person writes them.
  book.setNumber("half", 2.5);
  KIMIA_REQUIRE(kimia::fillPlaceholders("{half}", book) == "2.5");

  // A typo leaves nothing rather than showing braces to the player: it
  // should look wrong, not broken.
  KIMIA_REQUIRE(kimia::fillPlaceholders("[{nope}]", book) == "[]");
  // An unclosed brace is ordinary text, not the start of an endless read.
  KIMIA_REQUIRE(kimia::fillPlaceholders("100{ complete", book) == "100{ complete");
  KIMIA_REQUIRE(kimia::fillPlaceholders("plain", book) == "plain");
  KIMIA_REQUIRE(kimia::fillPlaceholders("", book).empty());
}

KIMIA_TEST(hud_panels_are_placed_by_name_not_duplicated) {
  HudLayout layout;
  Panel label = makePanel("score", PanelKind::Label);
  label.text = "one";
  layout.set(label);
  KIMIA_REQUIRE(layout.panels.size() == 1U);

  // Dragging a panel calls set() over and over with the same name.
  label.text = "two";
  label.x = 0.5;
  layout.set(label);
  KIMIA_REQUIRE(layout.panels.size() == 1U);
  KIMIA_REQUIRE(layout.find("score")->text == "two");
  KIMIA_REQUIRE(layout.find("score")->x == 0.5);

  // A different name really is a second panel.
  layout.set(makePanel("lives", PanelKind::Bar));
  KIMIA_REQUIRE(layout.panels.size() == 2U);
  // A nameless panel is refused rather than saved unreachable.
  layout.set(makePanel("", PanelKind::Label));
  KIMIA_REQUIRE(layout.panels.size() == 2U);

  KIMIA_REQUIRE(layout.remove("score"));
  KIMIA_REQUIRE(!layout.remove("score"));
  KIMIA_REQUIRE(layout.find("score") == nullptr);
}

KIMIA_TEST(hud_draws_onto_the_frame_and_hides_when_told) {
  LogicBook book;
  book.setNumber("score", 42.0);
  HudLayout layout;
  Panel label = makePanel("score", PanelKind::Label);
  label.text = "Score {score}";
  label.x = 0.05;
  label.y = 0.05;
  label.width = 0.5;
  label.height = 0.15;
  layout.set(label);

  Image frame = blankFrame();
  KIMIA_REQUIRE(litPixels(frame) == 0U);
  kimia::drawHud(frame, layout, book);
  KIMIA_REQUIRE(litPixels(frame) > 100U);  // something really was drawn

  // A hidden panel draws nothing, which is how a rule shows a win screen
  // only at the end.
  Panel hidden = *layout.find("score");
  hidden.visible = false;
  layout.set(hidden);
  Image second = blankFrame();
  kimia::drawHud(second, layout, book);
  KIMIA_REQUIRE(litPixels(second) == 0U);

  // An empty layout over a frame leaves it untouched.
  Image third = blankFrame();
  kimia::drawHud(third, HudLayout{}, book);
  KIMIA_REQUIRE(litPixels(third) == 0U);
}

KIMIA_TEST(hud_a_bar_follows_its_variable) {
  // A health bar is only worth having if it actually tracks the health.
  LogicBook book;
  book.setNumber("lives", 100.0);
  HudLayout layout;
  Panel bar = makePanel("health", PanelKind::Bar);
  bar.variable = "lives";
  bar.maximum = 100.0;
  bar.x = 0.1;
  bar.y = 0.1;
  bar.width = 0.8;
  bar.height = 0.1;
  bar.color = Vec3{1.0, 0.0, 0.0};
  bar.background = Vec3{0.0, 0.0, 0.0};
  layout.set(bar);

  const auto redPixels = [&](const Image& image) {
    usize count = 0;
    for (usize i = 0; i + 2U < image.pixels.size(); i += 3U) {
      if (image.pixels[i] > 200U && image.pixels[i + 1U] < 50U) ++count;
    }
    return count;
  };

  Image full = blankFrame();
  kimia::drawHud(full, layout, book);
  const usize atFull = redPixels(full);
  KIMIA_REQUIRE(atFull > 100U);

  book.setNumber("lives", 25.0);
  Image quarter = blankFrame();
  kimia::drawHud(quarter, layout, book);
  const usize atQuarter = redPixels(quarter);
  KIMIA_REQUIRE(atQuarter > 0U);
  KIMIA_REQUIRE(atQuarter < atFull / 2U);  // visibly emptier

  book.setNumber("lives", 0.0);
  Image empty = blankFrame();
  kimia::drawHud(empty, layout, book);
  KIMIA_REQUIRE(redPixels(empty) == 0U);

  // Over-full does not overflow the bar, and a zero maximum does not
  // divide by zero.
  book.setNumber("lives", 500.0);
  Image over = blankFrame();
  kimia::drawHud(over, layout, book);
  KIMIA_REQUIRE(redPixels(over) <= atFull);
  Panel broken = *layout.find("health");
  broken.maximum = 0.0;
  layout.set(broken);
  Image safe = blankFrame();
  kimia::drawHud(safe, layout, book);  // must not crash
  KIMIA_REQUIRE(redPixels(safe) == 0U);
}

KIMIA_TEST(hud_a_button_is_found_where_it_was_drawn) {
  HudLayout layout;
  Panel button = makePanel("restart", PanelKind::Button);
  button.x = 0.25;
  button.y = 0.5;
  button.width = 0.5;
  button.height = 0.2;
  button.event = "restart";
  layout.set(button);

  // Positions are fractions, so the same layout works at any size.
  KIMIA_REQUIRE(kimia::buttonAt(layout, 400, 200, 200.0, 120.0) == "restart");  // middle of it
  KIMIA_REQUIRE(kimia::buttonAt(layout, 800, 400, 400.0, 240.0) == "restart");  // twice the size
  // Outside it, nothing.
  KIMIA_REQUIRE(kimia::buttonAt(layout, 400, 200, 10.0, 10.0).empty());
  KIMIA_REQUIRE(kimia::buttonAt(layout, 400, 200, 200.0, 20.0).empty());

  // A label is not pressable, however much it looks like one.
  Panel label = makePanel("title", PanelKind::Label);
  label.x = 0.0;
  label.y = 0.0;
  label.width = 1.0;
  label.height = 1.0;
  HudLayout labels;
  labels.set(label);
  KIMIA_REQUIRE(kimia::buttonAt(labels, 400, 200, 200.0, 100.0).empty());

  // A hidden button cannot be pressed.
  Panel gone = *layout.find("restart");
  gone.visible = false;
  layout.set(gone);
  KIMIA_REQUIRE(kimia::buttonAt(layout, 400, 200, 200.0, 120.0).empty());
}

KIMIA_TEST(hud_the_button_on_top_wins) {
  // Panels are drawn in order, so a later one covers an earlier one. A
  // tap must reach the button the player can actually see.
  HudLayout layout;
  Panel under = makePanel("under", PanelKind::Button);
  under.x = 0.0;
  under.y = 0.0;
  under.width = 1.0;
  under.height = 1.0;
  layout.set(under);
  Panel over = makePanel("over", PanelKind::Button);
  over.x = 0.4;
  over.y = 0.4;
  over.width = 0.2;
  over.height = 0.2;
  layout.set(over);

  KIMIA_REQUIRE(kimia::buttonAt(layout, 100, 100, 50.0, 50.0) == "over");
  // Away from the small one, the big one still answers.
  KIMIA_REQUIRE(kimia::buttonAt(layout, 100, 100, 10.0, 10.0) == "under");
}

KIMIA_TEST(hud_kind_names_survive_a_round_trip) {
  // The editor and the save file share these names.
  for (const char* name : {"label", "bar", "box", "button"}) {
    PanelKind kind = PanelKind::Label;
    KIMIA_REQUIRE(kimia::panelKindFromName(name, kind));
    KIMIA_REQUIRE(std::string(kimia::panelKindName(kind)) == name);
  }
  PanelKind kind = PanelKind::Bar;
  KIMIA_REQUIRE(!kimia::panelKindFromName("nonsense", kind));
  KIMIA_REQUIRE(kind == PanelKind::Bar);  // left alone
}
