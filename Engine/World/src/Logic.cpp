#include <kimia/Logic.h>

#include <sstream>

namespace kimia {

namespace {

// A small table beats a switch here: the editor needs the names in both
// directions, and one table cannot drift out of step with itself.
struct TriggerName {
  Trigger trigger;
  const char* name;
};
const TriggerName kTriggers[] = {
    {Trigger::Start, "start"},         {Trigger::EveryFrame, "every-frame"},
    {Trigger::KeyPressed, "key"},      {Trigger::KeyHeld, "key-held"},
    {Trigger::Collision, "collision"}, {Trigger::AreaEnter, "area-enter"},
    {Trigger::AreaExit, "area-exit"},  {Trigger::Timer, "timer"},
    {Trigger::VariableIs, "variable"}, {Trigger::Event, "event"},
};

struct CompareName {
  Compare compare;
  const char* name;
};
const CompareName kCompares[] = {
    {Compare::Equal, "=="},        {Compare::NotEqual, "!="}, {Compare::Less, "<"},
    {Compare::LessOrEqual, "<="},  {Compare::Greater, ">"},   {Compare::GreaterOrEqual, ">="},
};

struct ActName {
  Act act;
  const char* name;
};
const ActName kActs[] = {
    {Act::SetVariable, "set"},        {Act::AddVariable, "add"},
    {Act::Move, "move"},              {Act::MoveTo, "move-to"},
    {Act::Rotate, "rotate"},          {Act::Spawn, "spawn"},
    {Act::Destroy, "destroy"},        {Act::PlaySound, "sound"},
    {Act::PlayAnimation, "animate"},  {Act::ShowMessage, "message"},
    {Act::RaiseEvent, "raise"},       {Act::GoToScene, "scene"},
    {Act::Effect_, "effect"},         {Act::Wait, "wait"},
    {Act::EndGame, "end-game"},
};

std::string trimZeros(f64 value) {
  std::ostringstream text;
  text.precision(3);
  text << std::fixed << value;
  std::string out = text.str();
  // 3.000 reads as 3, which matters when the whole point is legibility.
  while (out.size() > 1U && out.back() == '0') out.pop_back();
  if (!out.empty() && out.back() == '.') out.pop_back();
  return out;
}

}  // namespace

const char* triggerName(Trigger trigger) {
  for (const TriggerName& entry : kTriggers) {
    if (entry.trigger == trigger) return entry.name;
  }
  return "start";
}

bool triggerFromName(const std::string& name, Trigger& out) {
  for (const TriggerName& entry : kTriggers) {
    if (name != entry.name) continue;
    out = entry.trigger;
    return true;
  }
  return false;
}

const char* compareName(Compare compare) {
  for (const CompareName& entry : kCompares) {
    if (entry.compare == compare) return entry.name;
  }
  return "==";
}

bool compareFromName(const std::string& name, Compare& out) {
  for (const CompareName& entry : kCompares) {
    if (name != entry.name) continue;
    out = entry.compare;
    return true;
  }
  return false;
}

const char* actName(Act act) {
  for (const ActName& entry : kActs) {
    if (entry.act == act) return entry.name;
  }
  return "set";
}

bool actFromName(const std::string& name, Act& out) {
  for (const ActName& entry : kActs) {
    if (name != entry.name) continue;
    out = entry.act;
    return true;
  }
  return false;
}

bool compareNumbers(f64 left, Compare compare, f64 right) {
  switch (compare) {
    case Compare::Equal: return left == right;
    case Compare::NotEqual: return left != right;
    case Compare::Less: return left < right;
    case Compare::LessOrEqual: return left <= right;
    case Compare::Greater: return left > right;
    case Compare::GreaterOrEqual: return left >= right;
  }
  return false;
}

const Variable* LogicBook::find(const std::string& name) const {
  for (const Variable& variable : variables) {
    if (variable.name == name) return &variable;
  }
  return nullptr;
}

Variable* LogicBook::find(const std::string& name) {
  for (Variable& variable : variables) {
    if (variable.name == name) return &variable;
  }
  return nullptr;
}

f64 LogicBook::numberOf(const std::string& name) const {
  const Variable* variable = find(name);
  // A rule pointing at a variable somebody deleted reads zero rather than
  // stopping the game.
  return variable == nullptr ? 0.0 : variable->number;
}

void LogicBook::setNumber(const std::string& name, f64 value) {
  if (name.empty()) return;
  Variable* variable = find(name);
  if (variable == nullptr) {
    // Setting an unknown variable CREATES it. Asking the user to declare
    // one first would be a step with no purpose.
    Variable fresh;
    fresh.name = name;
    fresh.number = value;
    variables.push_back(fresh);
    return;
  }
  variable->number = value;
  variable->isText = false;
}

void LogicBook::setText(const std::string& name, const std::string& value) {
  if (name.empty()) return;
  Variable* variable = find(name);
  if (variable == nullptr) {
    Variable fresh;
    fresh.name = name;
    fresh.text = value;
    fresh.isText = true;
    variables.push_back(fresh);
    return;
  }
  variable->text = value;
  variable->isText = true;
}

std::string describeRule(const Rule& rule) {
  std::ostringstream out;
  out << "WHEN " << triggerName(rule.trigger);
  if (!rule.subject.empty()) out << ' ' << rule.subject;
  if (!rule.other.empty()) out << " + " << rule.other;
  if (rule.trigger == Trigger::Timer) out << " every " << trimZeros(rule.number) << "s";

  for (const Condition& condition : rule.conditions) {
    out << "  IF " << condition.variable << ' ' << compareName(condition.compare) << ' ';
    if (condition.useText) {
      out << condition.text;
    } else {
      out << trimZeros(condition.number);
    }
  }

  for (const Action& action : rule.actions) {
    out << "  DO " << actName(action.act);
    if (!action.target.empty()) out << ' ' << action.target;
    if (!action.text.empty()) out << ' ' << action.text;
    switch (action.act) {
      case Act::Move:
      case Act::MoveTo:
      case Act::Spawn:
        out << " (" << trimZeros(action.amount.x) << ", " << trimZeros(action.amount.y) << ", "
            << trimZeros(action.amount.z) << ")";
        break;
      case Act::SetVariable:
      case Act::AddVariable:
      case Act::Rotate:
      case Act::Wait:
        out << ' ' << trimZeros(action.number);
        break;
      default:
        break;
    }
  }
  return out.str();
}

// --- Running the rules ---

namespace {

bool listHas(const std::vector<std::string>& list, const std::string& value) {
  for (const std::string& entry : list) {
    if (entry == value) return true;
  }
  return false;
}

// A collision is reported once, but "Ball hits Wall" and "Wall hits Ball"
// are the same event to a person writing a rule.
bool pairHas(const std::vector<std::string>& pairs, const std::string& a, const std::string& b) {
  return listHas(pairs, a + "|" + b) || listHas(pairs, b + "|" + a);
}

}  // namespace

void LogicRuntime::reset() {
  started_ = false;
  finished_ = false;
  won_ = false;
  timers_.clear();
  waits_.clear();
  insideLast_.clear();
  raised_.clear();
}

bool LogicRuntime::conditionsHold(const LogicBook& book, const Rule& rule) const {
  for (const Condition& condition : rule.conditions) {
    const Variable* variable = book.find(condition.variable);
    if (condition.useText) {
      const std::string value = variable == nullptr ? std::string() : variable->text;
      const bool same = value == condition.text;
      if (condition.compare == Compare::NotEqual ? same : !same) return false;
      continue;
    }
    const f64 value = variable == nullptr ? 0.0 : variable->number;
    if (!compareNumbers(value, condition.compare, condition.number)) return false;
  }
  return true;
}

bool LogicRuntime::matches(const LogicBook& book, const Rule& rule, const LogicInput& input, usize index) {
  switch (rule.trigger) {
    case Trigger::Start:
      return false;  // handled by begin(), never by step()
    case Trigger::EveryFrame:
      return true;
    case Trigger::KeyPressed:
      return listHas(input.keysPressed, rule.subject);
    case Trigger::KeyHeld:
      return listHas(input.keysHeld, rule.subject);
    case Trigger::Collision:
      return pairHas(input.collisions, rule.subject, rule.other);
    case Trigger::AreaEnter: {
      // Entering means inside now and not inside last frame — otherwise
      // the rule would fire every frame somebody stands in the area.
      const std::string key = rule.subject + "|" + rule.other;
      return listHas(input.areaPairs, key) && !listHas(insideLast_, key);
    }
    case Trigger::AreaExit: {
      const std::string key = rule.subject + "|" + rule.other;
      return !listHas(input.areaPairs, key) && listHas(insideLast_, key);
    }
    case Trigger::Timer: {
      if (rule.number <= 0.0) return false;
      f64& due = timers_[index];
      due += input.seconds;
      if (due < rule.number) return false;
      // Subtract rather than zero, so a slow frame does not lose time and
      // a fast timer still keeps its average rate.
      due -= rule.number;
      return true;
    }
    case Trigger::VariableIs:
      return compareNumbers(book.numberOf(rule.subject), Compare::Equal, rule.number);
    case Trigger::Event:
      return listHas(input.events, rule.subject) || listHas(raised_, rule.subject);
  }
  return false;
}

void LogicRuntime::perform(LogicBook& book, const Rule& rule, std::vector<Effect>& out) {
  for (const Action& action : rule.actions) {
    switch (action.act) {
      case Act::SetVariable:
        if (!action.text.empty() && action.target.empty()) {
          book.setNumber(action.text, action.number);
        } else {
          book.setNumber(action.target.empty() ? action.text : action.target, action.number);
        }
        break;
      case Act::AddVariable: {
        const std::string name = action.target.empty() ? action.text : action.target;
        book.setNumber(name, book.numberOf(name) + action.number);
        break;
      }
      case Act::RaiseEvent:
        // Raised events are visible to later rules in the SAME frame, so a
        // chain of rules resolves at once instead of one link per frame.
        raised_.push_back(action.text);
        break;
      case Act::EndGame:
        finished_ = true;
        won_ = action.number != 0.0;
        break;
      default:
        break;  // everything else is for the caller to carry out
    }
    // The caller sees every action, including the variable ones, so a HUD
    // can show what changed.
    Effect effect;
    effect.act = action.act;
    effect.target = action.target;
    effect.text = action.text;
    effect.number = action.number;
    effect.amount = action.amount;
    out.push_back(effect);
  }
}

void LogicRuntime::begin(LogicBook& book, std::vector<Effect>& out) {
  reset();
  started_ = true;
  timers_.assign(book.rules.size(), 0.0);
  waits_.assign(book.rules.size(), 0.0);
  for (const Rule& rule : book.rules) {
    if (!rule.enabled || rule.trigger != Trigger::Start) continue;
    if (!conditionsHold(book, rule)) continue;
    perform(book, rule, out);
  }
}

void LogicRuntime::step(LogicBook& book, const LogicInput& input, std::vector<Effect>& out) {
  if (!started_) begin(book, out);
  if (finished_) return;  // the game is over: rules stop running
  // Rules can be added or removed while the editor is open.
  if (timers_.size() != book.rules.size()) timers_.resize(book.rules.size(), 0.0);
  if (waits_.size() != book.rules.size()) waits_.resize(book.rules.size(), 0.0);

  raised_.clear();
  for (usize i = 0; i < book.rules.size(); ++i) {
    const Rule& rule = book.rules[i];
    if (!rule.enabled) continue;
    // A rule that asked to wait is counting down, not firing.
    if (waits_[i] > 0.0) {
      waits_[i] -= input.seconds;
      continue;
    }
    if (!matches(book, rule, input, i)) continue;
    if (!conditionsHold(book, rule)) continue;
    perform(book, rule, out);
    // A Wait action parks this rule rather than the whole game.
    for (const Action& action : rule.actions) {
      if (action.act == Act::Wait) waits_[i] = action.number;
    }
    if (finished_) break;
  }
  insideLast_ = input.areaPairs;
}

}  // namespace kimia
