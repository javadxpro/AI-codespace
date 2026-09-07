#pragma once

#include <kimia/Types.h>
#include <kimia/Vec.h>

#include <string>
#include <vector>

namespace kimia {

// --- Visual logic: making a game without writing code ---
//
// A game's behaviour is expressed as RULES, each one shaped like a
// sentence a person would say out loud:
//
//     WHEN <something happens>  IF <this is true>  DO <these things>
//
// That is the whole model. It is deliberately flat rather than a graph of
// wires: on a phone screen a node graph is unreadable, and almost all game
// logic really is a list of "when this, do that". Rules run top to bottom,
// so ordering is something the user controls rather than a mystery.
//
// Everything is addressed by NAME (entity names, tags, variable names) so
// the editor stays stringly-typed and nothing here needs to know about
// handles or engine internals.

// What can set a rule off.
enum class Trigger {
  Start,       // once, when play begins
  EveryFrame,  // every update
  KeyPressed,  // `subject` is the key name ("space", "j")
  KeyHeld,
  Collision,   // `subject` collides with `other` (names or tags)
  AreaEnter,   // `subject` enters the area around `other`
  AreaExit,
  Timer,       // every `number` seconds
  VariableIs,  // `subject` variable compares against `number`/`text`
  Event,       // a named event raised by another rule or by the game
};

// How a condition compares.
enum class Compare { Equal, NotEqual, Less, LessOrEqual, Greater, GreaterOrEqual };

// An optional gate on a rule. A rule with no conditions always fires.
struct Condition {
  std::string variable;  // variable name, or "<entity>.x" style property
  Compare compare = Compare::Equal;
  f64 number = 0.0;
  std::string text;   // used when the variable holds text
  bool useText = false;
};

// What a rule does.
enum class Act {
  SetVariable,    // variable = number/text
  AddVariable,    // variable += number
  Move,           // move `target` by (x, y, z) — a nudge, per second
  MoveTo,         // place `target` at (x, y, z)
  Rotate,         // turn `target` by x degrees per second about Y
  Spawn,          // create a copy of `target` at (x, y, z)
  Destroy,        // remove `target`
  PlaySound,      // `text` is the sound name
  PlayAnimation,  // `text` is the clip name on `target`
  ShowMessage,    // put `text` on the HUD
  RaiseEvent,     // fire a named event other rules can listen for
  GoToScene,      // load another scene by name
  Effect_,        // play a named particle burst at `target` (or at `amount`)
  Wait,           // pause this rule's remaining actions for `number` seconds
  EndGame,        // win or lose: `number` != 0 means win
};

struct Action {
  Act act = Act::SetVariable;
  std::string target;  // entity name or tag the action applies to
  std::string text;    // sound/clip/message/event/variable name
  f64 number = 0.0;
  Vec3 amount{0.0, 0.0, 0.0};
};

// One complete rule: the sentence.
struct Rule {
  std::string name;  // what the user called it, for the rule list
  bool enabled = true;
  Trigger trigger = Trigger::Start;
  std::string subject;  // whose event this is (entity name, tag or key)
  std::string other;    // the second party, for collisions and areas
  f64 number = 0.0;     // timer interval, area radius, and so on
  std::vector<Condition> conditions;
  std::vector<Action> actions;
};

// A variable the game keeps. Kept as both a number and a text so the
// editor never has to ask the user to declare a type up front.
struct Variable {
  std::string name;
  f64 number = 0.0;
  std::string text;
  bool isText = false;
};

// The whole logic of a world: its rules and its variables.
struct LogicBook {
  std::vector<Rule> rules;
  std::vector<Variable> variables;

  const Variable* find(const std::string& name) const;
  Variable* find(const std::string& name);
  // Reads a variable as a number; 0 when it does not exist, so a rule
  // referring to a deleted variable degrades rather than breaking.
  f64 numberOf(const std::string& name) const;
  void setNumber(const std::string& name, f64 value);
  void setText(const std::string& name, const std::string& value);
};

// --- Names, for the editor and the save file ---
const char* triggerName(Trigger trigger);
bool triggerFromName(const std::string& name, Trigger& out);
const char* compareName(Compare compare);
bool compareFromName(const std::string& name, Compare& out);
const char* actName(Act act);
bool actFromName(const std::string& name, Act& out);

// A rule as a readable sentence, for the rule list:
//   "WHEN key space  IF score < 10  DO add score 1"
std::string describeRule(const Rule& rule);

// True when `left <cmp> right`.
bool compareNumbers(f64 left, Compare compare, f64 right);

// --- Running the rules ---
//
// The runtime is deliberately separated from the world: it says WHAT
// should happen ("move Ball by 0,0,-1", "play kick") and the caller does
// it. That keeps every decision testable without a renderer, a window or
// a physics step, and it means the same rules drive the editor preview and
// the exported game.

// One thing the rules decided should happen this frame.
struct Effect {
  Act act = Act::SetVariable;
  std::string target;
  std::string text;
  f64 number = 0.0;
  Vec3 amount{0.0, 0.0, 0.0};
};

// What the world tells the runtime about this frame. The runtime asks no
// questions of the engine; it is handed the facts.
struct LogicInput {
  f64 seconds = 0.0;                     // frame time
  std::vector<std::string> keysPressed;  // pressed THIS frame
  std::vector<std::string> keysHeld;
  // Pairs that touched this frame, as "<a>|<b>". The caller reports them
  // once per pair; the runtime matches either order.
  std::vector<std::string> collisions;
  // Named events raised from outside (a goal, a trick, a whistle).
  std::vector<std::string> events;
  // Entities currently inside another's area, as "<who>|<area>".
  std::vector<std::string> areaPairs;
};

// Runs a LogicBook. Holds the state a rule set needs between frames:
// which timers are due, what was inside an area last frame, and which
// rules are waiting.
class LogicRuntime {
public:
  // Call when play starts: clears state and fires the Start rules.
  void begin(LogicBook& book, std::vector<Effect>& out);
  // Call once a frame.
  void step(LogicBook& book, const LogicInput& input, std::vector<Effect>& out);
  // Events a rule raised, for rules that listen and for the caller.
  const std::vector<std::string>& raised() const { return raised_; }
  bool started() const { return started_; }
  // Whether the game has been declared over, and whether it was won.
  bool finished() const { return finished_; }
  bool won() const { return won_; }
  void reset();

private:
  bool matches(const LogicBook& book, const Rule& rule, const LogicInput& input, usize index);
  bool conditionsHold(const LogicBook& book, const Rule& rule) const;
  void perform(LogicBook& book, const Rule& rule, std::vector<Effect>& out);

  bool started_ = false;
  bool finished_ = false;
  bool won_ = false;
  std::vector<f64> timers_;                // one per rule
  std::vector<f64> waits_;                 // one per rule
  std::vector<std::string> insideLast_;    // area pairs from last frame
  std::vector<std::string> raised_;        // events raised this frame
};

}  // namespace kimia
