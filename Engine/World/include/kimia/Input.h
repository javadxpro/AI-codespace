#pragma once

#include <kimia/Types.h>
#include <kimia/Vec.h>

#include <string>
#include <vector>

namespace kimia {

// --- The input system: one ACTION, many ways to do it ---
//
// A game says "jump". A person might tap a button on glass, press Space,
// or push A on a controller. Wiring each of those separately means three
// chances to forget one, and it means a rule has to care what hardware
// the player owns.
//
// So the game names ACTIONS, and each action lists the bindings that
// trigger it. Rules, animations and sounds all hang off the action name.

// What kind of control a binding describes.
enum class Source {
  Key,       // a keyboard key, by name: "space", "j"
  Touch,     // an on-screen button the engine draws
  Pad,       // a gamepad button: "a", "b", "x", "y", "l1", "start"
  Stick,     // an on-screen or physical stick: reports a direction
};

struct Binding {
  Source source = Source::Key;
  std::string code;  // key name, pad button name, or the touch button's id
};

// Where an on-screen control sits, in fractions of the screen so a layout
// works on any size of phone.
struct TouchSpot {
  f64 x = 0.8;
  f64 y = 0.8;
  f64 size = 0.12;
  std::string label;
};

struct Control {
  std::string name;   // "jump", "kick", "sprint"
  std::vector<Binding> bindings;
  // Drawn on screen when this control has a Touch binding.
  TouchSpot spot;
  // The animation clip this control plays, and the model it belongs to.
  // Empty means the control does not animate anything.
  std::string clipFile;  // the FBX the clip came from
  std::string clip;      // the clip's name inside it
  // Optional character entity. Empty means "find a compatible character";
  // setting it makes a button drive exactly this player/enemy.
  std::string target;
  std::string sound;     // a sound to play, by name
};

// A stick's current push, -1..1 on each axis. Kept apart from actions
// because a direction is not a press.
struct StickState {
  f64 x = 0.0;
  f64 y = 0.0;
  bool active = false;
};

// Everything the game's controls are made of.
struct InputMap {
  std::vector<Control> controls;
  // The movement stick, if the game wants one on screen.
  bool showStick = false;
  TouchSpot stickSpot{0.18, 0.78, 0.16, "move"};

  const Control* find(const std::string& name) const;
  Control* find(const std::string& name);
  void set(const Control& control);  // add or replace by name
  bool remove(const std::string& name);
  // Which action a raw control fires, or empty. This is the lookup the
  // engine does every frame.
  std::string actionFor(Source source, const std::string& code) const;
  // The on-screen buttons to draw.
  std::vector<const Control*> touchControls() const;
  // Which on-screen button is under a point, or empty.
  std::string touchAt(f64 pixelX, f64 pixelY, i32 width, i32 height) const;
};

const char* sourceName(Source source);
bool sourceFromName(const std::string& name, Source& out);

}  // namespace kimia
