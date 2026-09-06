#include <kimia/Logic.h>
#include <kimia_test.h>

#include <cmath>
#include <string>
#include <vector>

namespace {

using kimia::Act;
using kimia::Action;
using kimia::Compare;
using kimia::Condition;
using kimia::Effect;
using kimia::LogicBook;
using kimia::LogicInput;
using kimia::LogicRuntime;
using kimia::Rule;
using kimia::Trigger;
using kimia::f64;
using kimia::i32;
using kimia::usize;

Rule makeRule(const char* name, Trigger trigger, const char* subject = "") {
  Rule rule;
  rule.name = name;
  rule.trigger = trigger;
  rule.subject = subject;
  return rule;
}

Action makeAction(Act act, const char* target, const char* text, f64 number) {
  Action action;
  action.act = act;
  action.target = target;
  action.text = text;
  action.number = number;
  return action;
}

// Runs `frames` frames at 60 Hz with a fixed input.
void run(LogicRuntime& runtime, LogicBook& book, const LogicInput& input, i32 frames) {
  std::vector<Effect> effects;
  for (i32 i = 0; i < frames; ++i) {
    effects.clear();
    runtime.step(book, input, effects);
  }
}

}  // namespace

// --- Visual logic: making a game without writing code ---

KIMIA_TEST(logic_a_rule_reads_as_a_sentence) {
  // The rule list is the main thing a user reads, so a rule has to
  // describe itself in words rather than as a row of dropdown values.
  Rule rule = makeRule("score on space", Trigger::KeyPressed, "space");
  Condition condition;
  condition.variable = "score";
  condition.compare = Compare::Less;
  condition.number = 10.0;
  rule.conditions.push_back(condition);
  rule.actions.push_back(makeAction(Act::AddVariable, "score", "", 1.0));

  const std::string sentence = kimia::describeRule(rule);
  KIMIA_REQUIRE(sentence.find("WHEN key space") != std::string::npos);
  KIMIA_REQUIRE(sentence.find("IF score < 10") != std::string::npos);
  KIMIA_REQUIRE(sentence.find("DO add score 1") != std::string::npos);
  // Numbers read as people write them: 10, not 10.000.
  KIMIA_REQUIRE(sentence.find("10.0") == std::string::npos);
}

KIMIA_TEST(logic_names_survive_a_round_trip) {
  // The editor and the save file both use these names, so a mismatch
  // would silently turn one trigger into another.
  for (const char* name : {"start", "every-frame", "key", "key-held", "collision", "area-enter",
                           "area-exit", "timer", "variable", "event"}) {
    Trigger trigger = Trigger::Start;
    KIMIA_REQUIRE(kimia::triggerFromName(name, trigger));
    KIMIA_REQUIRE(std::string(kimia::triggerName(trigger)) == name);
  }
  for (const char* name : {"==", "!=", "<", "<=", ">", ">="}) {
    Compare compare = Compare::Equal;
    KIMIA_REQUIRE(kimia::compareFromName(name, compare));
    KIMIA_REQUIRE(std::string(kimia::compareName(compare)) == name);
  }
  for (const char* name : {"set", "add", "move", "move-to", "rotate", "spawn", "destroy", "sound",
                           "animate", "message", "raise", "scene", "wait", "end-game"}) {
    Act act = Act::SetVariable;
    KIMIA_REQUIRE(kimia::actFromName(name, act));
    KIMIA_REQUIRE(std::string(kimia::actName(act)) == name);
  }
  // Nonsense is refused rather than silently becoming the first entry.
  Trigger trigger = Trigger::EveryFrame;
  KIMIA_REQUIRE(!kimia::triggerFromName("nonsense", trigger));
  KIMIA_REQUIRE(trigger == Trigger::EveryFrame);  // left alone
}

KIMIA_TEST(logic_setting_an_unknown_variable_creates_it) {
  // Making the user declare a variable before using it would be a step
  // with no purpose.
  LogicBook book;
  KIMIA_REQUIRE(book.find("score") == nullptr);
  KIMIA_REQUIRE(book.numberOf("score") == 0.0);  // reading is safe too

  book.setNumber("score", 7.0);
  KIMIA_REQUIRE(book.find("score") != nullptr);
  KIMIA_REQUIRE(book.numberOf("score") == 7.0);
  book.setNumber("score", 9.0);
  KIMIA_REQUIRE(book.variables.size() == 1U);  // set, not a second variable

  book.setText("name", "Kimia");
  KIMIA_REQUIRE(book.find("name")->isText);
  KIMIA_REQUIRE(book.find("name")->text == "Kimia");
  // An unnamed variable is ignored rather than creating a blank one.
  book.setNumber("", 1.0);
  KIMIA_REQUIRE(book.variables.size() == 2U);
}

KIMIA_TEST(logic_start_rules_fire_once_and_a_key_rule_fires_on_press) {
  LogicBook book;
  Rule setup = makeRule("setup", Trigger::Start);
  setup.actions.push_back(makeAction(Act::SetVariable, "score", "", 0.0));
  book.rules.push_back(setup);
  Rule press = makeRule("press", Trigger::KeyPressed, "space");
  press.actions.push_back(makeAction(Act::AddVariable, "score", "", 1.0));
  book.rules.push_back(press);

  LogicRuntime runtime;
  std::vector<Effect> effects;
  runtime.begin(book, effects);
  KIMIA_REQUIRE(runtime.started());
  KIMIA_REQUIRE(book.numberOf("score") == 0.0);
  KIMIA_REQUIRE(effects.size() == 1U);

  // Frames with nothing pressed change nothing.
  LogicInput idle;
  idle.seconds = 1.0 / 60.0;
  run(runtime, book, idle, 10);
  KIMIA_REQUIRE(book.numberOf("score") == 0.0);

  // A Start rule must NOT fire again every frame.
  LogicInput pressing;
  pressing.seconds = 1.0 / 60.0;
  pressing.keysPressed.push_back("space");
  run(runtime, book, pressing, 3);
  KIMIA_REQUIRE(book.numberOf("score") == 3.0);

  // A different key does nothing.
  LogicInput other;
  other.seconds = 1.0 / 60.0;
  other.keysPressed.push_back("j");
  run(runtime, book, other, 5);
  KIMIA_REQUIRE(book.numberOf("score") == 3.0);
}

KIMIA_TEST(logic_conditions_gate_a_rule) {
  LogicBook book;
  book.setNumber("lives", 3.0);
  Rule rule = makeRule("lose", Trigger::EveryFrame);
  Condition condition;
  condition.variable = "lives";
  condition.compare = Compare::LessOrEqual;
  condition.number = 0.0;
  rule.conditions.push_back(condition);
  rule.actions.push_back(makeAction(Act::EndGame, "", "", 0.0));
  book.rules.push_back(rule);

  LogicRuntime runtime;
  std::vector<Effect> effects;
  runtime.begin(book, effects);
  LogicInput input;
  input.seconds = 1.0 / 60.0;
  run(runtime, book, input, 5);
  KIMIA_REQUIRE(!runtime.finished());  // three lives left: the rule is gated

  book.setNumber("lives", 0.0);
  run(runtime, book, input, 1);
  KIMIA_REQUIRE(runtime.finished());
  KIMIA_REQUIRE(!runtime.won());  // end-game 0 = lost
}

KIMIA_TEST(logic_a_timer_keeps_its_own_rate) {
  LogicBook book;
  Rule tick = makeRule("tick", Trigger::Timer);
  tick.number = 0.5;
  tick.actions.push_back(makeAction(Act::AddVariable, "ticks", "", 1.0));
  book.rules.push_back(tick);

  LogicRuntime runtime;
  std::vector<Effect> effects;
  runtime.begin(book, effects);
  LogicInput input;
  input.seconds = 1.0 / 60.0;
  run(runtime, book, input, 121);  // just over two seconds
  // Four half-seconds fit in two seconds. Asserted as a range because a
  // 1/60 accumulator lands a hair either side of the boundary.
  const f64 ticks = book.numberOf("ticks");
  KIMIA_REQUIRE(ticks >= 3.0);
  KIMIA_REQUIRE(ticks <= 4.0);

  // A timer with no interval never fires rather than firing every frame.
  LogicBook broken;
  Rule bad = makeRule("bad", Trigger::Timer);
  bad.number = 0.0;
  bad.actions.push_back(makeAction(Act::AddVariable, "n", "", 1.0));
  broken.rules.push_back(bad);
  LogicRuntime second;
  second.begin(broken, effects);
  run(second, broken, input, 30);
  KIMIA_REQUIRE(broken.numberOf("n") == 0.0);
}

KIMIA_TEST(logic_area_enter_fires_once_not_every_frame) {
  // The difference between "when you walk in" and "while you stand there"
  // is the whole value of the trigger.
  LogicBook book;
  Rule enter = makeRule("enter", Trigger::AreaEnter, "Player");
  enter.other = "Goal";
  enter.actions.push_back(makeAction(Act::AddVariable, "entries", "", 1.0));
  book.rules.push_back(enter);
  Rule leave = makeRule("leave", Trigger::AreaExit, "Player");
  leave.other = "Goal";
  leave.actions.push_back(makeAction(Act::AddVariable, "exits", "", 1.0));
  book.rules.push_back(leave);

  LogicRuntime runtime;
  std::vector<Effect> effects;
  runtime.begin(book, effects);

  LogicInput inside;
  inside.seconds = 1.0 / 60.0;
  inside.areaPairs.push_back("Player|Goal");
  run(runtime, book, inside, 10);
  KIMIA_REQUIRE(book.numberOf("entries") == 1.0);
  KIMIA_REQUIRE(book.numberOf("exits") == 0.0);

  LogicInput outside;
  outside.seconds = 1.0 / 60.0;
  run(runtime, book, outside, 10);
  KIMIA_REQUIRE(book.numberOf("entries") == 1.0);
  KIMIA_REQUIRE(book.numberOf("exits") == 1.0);

  // Walking back in fires it again.
  run(runtime, book, inside, 3);
  KIMIA_REQUIRE(book.numberOf("entries") == 2.0);
}

KIMIA_TEST(logic_collision_matches_either_order) {
  // "Ball hits Wall" and "Wall hits Ball" are one event to a person.
  LogicBook book;
  Rule hit = makeRule("hit", Trigger::Collision, "Ball");
  hit.other = "Wall";
  hit.actions.push_back(makeAction(Act::AddVariable, "hits", "", 1.0));
  book.rules.push_back(hit);

  LogicRuntime runtime;
  std::vector<Effect> effects;
  runtime.begin(book, effects);

  LogicInput forwards;
  forwards.seconds = 1.0 / 60.0;
  forwards.collisions.push_back("Ball|Wall");
  run(runtime, book, forwards, 1);
  KIMIA_REQUIRE(book.numberOf("hits") == 1.0);

  LogicInput backwards;
  backwards.seconds = 1.0 / 60.0;
  backwards.collisions.push_back("Wall|Ball");
  run(runtime, book, backwards, 1);
  KIMIA_REQUIRE(book.numberOf("hits") == 2.0);

  // An unrelated pair does nothing.
  LogicInput elsewhere;
  elsewhere.seconds = 1.0 / 60.0;
  elsewhere.collisions.push_back("Ball|Tree");
  run(runtime, book, elsewhere, 1);
  KIMIA_REQUIRE(book.numberOf("hits") == 2.0);
}

KIMIA_TEST(logic_a_raised_event_reaches_later_rules_in_the_same_frame) {
  // A chain of rules should resolve at once, not one link per frame, or
  // building anything out of several rules feels broken.
  LogicBook book;
  Rule first = makeRule("goal", Trigger::KeyPressed, "g");
  first.actions.push_back(makeAction(Act::RaiseEvent, "", "scored", 0.0));
  book.rules.push_back(first);
  Rule second = makeRule("on scored", Trigger::Event, "scored");
  second.actions.push_back(makeAction(Act::AddVariable, "score", "", 1.0));
  book.rules.push_back(second);

  LogicRuntime runtime;
  std::vector<Effect> effects;
  runtime.begin(book, effects);
  LogicInput input;
  input.seconds = 1.0 / 60.0;
  input.keysPressed.push_back("g");
  run(runtime, book, input, 1);
  KIMIA_REQUIRE(book.numberOf("score") == 1.0);

  // An event from outside the rules works the same way.
  LogicBook outside;
  Rule listener = makeRule("listen", Trigger::Event, "whistle");
  listener.actions.push_back(makeAction(Act::AddVariable, "blows", "", 1.0));
  outside.rules.push_back(listener);
  LogicRuntime other;
  other.begin(outside, effects);
  LogicInput blown;
  blown.seconds = 1.0 / 60.0;
  blown.events.push_back("whistle");
  run(other, outside, blown, 1);
  KIMIA_REQUIRE(outside.numberOf("blows") == 1.0);
}

KIMIA_TEST(logic_the_caller_is_told_what_to_do) {
  // The runtime decides WHAT should happen; the world carries it out. That
  // split is what lets all of this be tested with no renderer at all.
  LogicBook book;
  Rule rule = makeRule("kick", Trigger::KeyPressed, "space");
  rule.actions.push_back(makeAction(Act::PlaySound, "", "kick", 0.0));
  Action move = makeAction(Act::Move, "Ball", "", 0.0);
  move.amount = kimia::Vec3{0.0, 0.0, -5.0};
  rule.actions.push_back(move);
  rule.actions.push_back(makeAction(Act::Destroy, "Crate_1", "", 0.0));
  book.rules.push_back(rule);

  LogicRuntime runtime;
  std::vector<Effect> effects;
  runtime.begin(book, effects);
  effects.clear();
  LogicInput input;
  input.seconds = 1.0 / 60.0;
  input.keysPressed.push_back("space");
  runtime.step(book, input, effects);

  KIMIA_REQUIRE(effects.size() == 3U);
  KIMIA_REQUIRE(effects[0].act == Act::PlaySound);
  KIMIA_REQUIRE(effects[0].text == "kick");
  KIMIA_REQUIRE(effects[1].act == Act::Move);
  KIMIA_REQUIRE(effects[1].target == "Ball");
  KIMIA_REQUIRE(effects[1].amount.z == -5.0);
  KIMIA_REQUIRE(effects[2].act == Act::Destroy);
  KIMIA_REQUIRE(effects[2].target == "Crate_1");
}

KIMIA_TEST(logic_a_disabled_rule_does_nothing_and_a_finished_game_stops) {
  LogicBook book;
  Rule off = makeRule("off", Trigger::EveryFrame);
  off.enabled = false;
  off.actions.push_back(makeAction(Act::AddVariable, "n", "", 1.0));
  book.rules.push_back(off);

  LogicRuntime runtime;
  std::vector<Effect> effects;
  runtime.begin(book, effects);
  LogicInput input;
  input.seconds = 1.0 / 60.0;
  run(runtime, book, input, 20);
  KIMIA_REQUIRE(book.numberOf("n") == 0.0);

  // Once the game is over, rules stop: a win screen should not keep
  // counting score behind itself.
  LogicBook over;
  Rule ender = makeRule("end", Trigger::EveryFrame);
  ender.actions.push_back(makeAction(Act::EndGame, "", "", 1.0));
  over.rules.push_back(ender);
  Rule counter = makeRule("count", Trigger::EveryFrame);
  counter.actions.push_back(makeAction(Act::AddVariable, "n", "", 1.0));
  over.rules.push_back(counter);
  LogicRuntime second;
  second.begin(over, effects);
  run(second, over, input, 30);
  KIMIA_REQUIRE(second.finished());
  KIMIA_REQUIRE(second.won());
  KIMIA_REQUIRE(over.numberOf("n") == 0.0);  // the break inside the frame stopped it

  // And it stays stopped on LATER frames. This needs the counter to come
  // FIRST, so the frame that ends the game still runs it once: any frame
  // after that must add nothing. Without this ordering the in-frame break
  // hides the fact that a finished game is never stepped again — which is
  // exactly what mutation testing caught.
  LogicBook after;
  Rule tally = makeRule("count", Trigger::EveryFrame);
  tally.actions.push_back(makeAction(Act::AddVariable, "n", "", 1.0));
  after.rules.push_back(tally);
  Rule stop = makeRule("end", Trigger::EveryFrame);
  stop.actions.push_back(makeAction(Act::EndGame, "", "", 1.0));
  after.rules.push_back(stop);

  LogicRuntime third;
  third.begin(after, effects);
  run(third, after, input, 1);
  const f64 atTheEnd = after.numberOf("n");
  KIMIA_REQUIRE(atTheEnd == 1.0);  // counted once, then the game ended
  run(third, after, input, 20);
  KIMIA_REQUIRE(after.numberOf("n") == atTheEnd);  // and never again
}
