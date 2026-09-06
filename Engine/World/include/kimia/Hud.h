#pragma once

#include <kimia/Image.h>
#include <kimia/Logic.h>
#include <kimia/Types.h>
#include <kimia/Vec.h>

#include <string>
#include <vector>

namespace kimia {

// --- The game's own interface, laid out by the user ---
//
// The engine's built-in HUD is a stack of lines in the corner: fine for a
// football match the engine itself wrote, useless for somebody making
// their own game. A person needs to say "the health bar goes HERE, and it
// follows the `lives` variable".
//
// A panel is one thing on screen. Its text can pull in a variable with
// {braces}, so "Score: {score}" reads the game's own numbers with no code.

enum class PanelKind {
  Label,   // text
  Bar,     // a filled bar, e.g. health
  Box,     // a plain rectangle: a backdrop or a frame
  Button,  // a label you can press; raises an event when tapped
};

// Where a panel sits. Positions are FRACTIONS of the screen (0..1), not
// pixels, so a layout made on a phone still works on a desktop and
// survives the window being resized.
struct Panel {
  std::string name;
  PanelKind kind = PanelKind::Label;
  bool visible = true;

  f64 x = 0.02;  // left edge, 0..1
  f64 y = 0.02;  // top edge, 0..1
  f64 width = 0.3;
  f64 height = 0.06;

  std::string text;      // "Score: {score}" — braces read a variable
  std::string variable;  // Bar: which variable fills it
  f64 maximum = 100.0;   // Bar: the value that fills it completely
  std::string event;     // Button: the event a press raises

  Vec3 color{0.9, 0.9, 0.95};       // text / bar fill
  Vec3 background{0.1, 0.12, 0.15};  // panel backdrop
  f64 opacity = 0.75;
  i32 scale = 2;  // text size
};

// Everything the user laid out.
struct HudLayout {
  std::vector<Panel> panels;

  const Panel* find(const std::string& name) const;
  Panel* find(const std::string& name);
  // Adds or replaces by name: dragging a panel is a repeat call.
  void set(const Panel& panel);
  bool remove(const std::string& name);
};

// Replaces every {name} in `text` with that variable's value. An unknown
// variable becomes an empty string rather than leaving the braces on
// screen, because a typo should look wrong, not broken.
std::string fillPlaceholders(const std::string& text, const LogicBook& book);

// Draws the layout over an already-rendered frame.
void drawHud(Image& image, const HudLayout& layout, const LogicBook& book);

// Which button is under a screen point, or empty. Used for taps.
std::string buttonAt(const HudLayout& layout, i32 imageWidth, i32 imageHeight, f64 pixelX, f64 pixelY);

const char* panelKindName(PanelKind kind);
bool panelKindFromName(const std::string& name, PanelKind& out);

}  // namespace kimia
