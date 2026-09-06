#include <kimia/WorldIO.h>

#include <kimia/SceneIO.h>
#include <kimia/TextFormat.h>

#include <fstream>
#include <sstream>

namespace kimia {

namespace {

constexpr const char* kProfilePrefix = "# profile ";  // + one ProfileIO body line

// Old files carry no field size: the ground plane they saved is the field.
// Any world without a `# profile field` line takes its size from the ground
// so a 20 x 20 sandbox file still plays on 20 x 20.
void fieldFromGround(WorldData& world) {
  const EntityData* ground = world.scene.get(world.scene.find("Ground"));
  if (ground == nullptr || ground->mesh != MeshKind::plane) return;
  const f64 width = ground->transform.scale.x;
  const f64 length = ground->transform.scale.z;
  if (width >= kProfileFieldMin && width <= kProfileFieldMax) world.profile.fieldWidth = width;
  if (length >= kProfileFieldMin && length <= kProfileFieldMax) world.profile.fieldLength = length;
}

}  // namespace

namespace {

// Rule lines pack several values onto one line, so a value containing a
// space has to survive being split apart again. escapeLineText only
// guards newlines and backslashes, which is right for a whole-line value
// and wrong here — a rule called "on goal" arrived back as "on".
std::string escapeWord(const std::string& text) {
  if (text.empty()) return "-";
  std::string out = escapeLineText(text);
  std::string packed;
  packed.reserve(out.size());
  for (const char c : out) {
    if (c == ' ') {
      packed += "\\s";
    } else {
      packed += c;
    }
  }
  return packed;
}

std::string unescapeWord(const std::string& token) {
  if (token == "-") return std::string();
  std::string spaced;
  spaced.reserve(token.size());
  for (usize i = 0; i < token.size(); ++i) {
    if (token[i] == '\\' && i + 1U < token.size() && token[i + 1U] == 's') {
      spaced += ' ';
      ++i;
      continue;
    }
    spaced += token[i];
  }
  return unescapeLineText(spaced);
}

// Splits on spaces. Rule text is escaped on the way out, so a name with a
// space in it survives as one token.
std::vector<std::string> splitWords(const std::string& line) {
  std::vector<std::string> parts;
  std::istringstream stream(line);
  std::string word;
  while (stream >> word) parts.push_back(word);
  return parts;
}

f64 parseNumber(const std::string& token) {
  try {
    return std::stod(token);
  } catch (...) {
    return 0.0;  // a corrupt number reads as zero rather than refusing the file
  }
}

}  // namespace

bool WorldIO::save(const WorldData& world, std::string& out) {
  std::string sceneText;
  if (!SceneIO::save(world.scene, sceneText)) return false;
  std::ostringstream stream;
  stream << "# KIMIA scene v1\n";
  stream << "# world name " << escapeLineText(world.name) << '\n';
  for (const std::string& line : ProfileIO::lines(world.profile)) stream << kProfilePrefix << line << '\n';
  stream << "# player speed " << formatFixed6(world.player.speed) << " color " << formatFixed6(world.player.color.x)
         << ' ' << formatFixed6(world.player.color.y) << ' ' << formatFixed6(world.player.color.z) << '\n';
  stream << "# ball type " << ballTypeName(world.ball.type) << '\n';
  stream << "# env " << environmentName(world.environment) << '\n';
  stream << "# score " << world.score << '\n';
  // Match score: only written for a match in progress, so no existing world
  // file grew a line.
  if (world.scoreTeam1 > 0U || world.scoreTeam2 > 0U) {
    stream << "# match " << world.scoreTeam1 << ' ' << world.scoreTeam2 << '\n';
  }
  // The personal record on this course (hole scoring). Written only once a
  // round has been finished, so files of worlds that were never played stay
  // byte-identical to the ones older versions wrote.
  if (world.bestRound > 0U) stream << "# best " << world.bestRound << '\n';

  // Visual logic. Written only when the world actually has rules, so every
  // file made before logic existed still saves byte-identically.
  for (const Variable& variable : world.logic.variables) {
    stream << "# var " << escapeWord(variable.name) << ' ' << (variable.isText ? "text" : "number") << ' '
           << (variable.isText ? escapeWord(variable.text) : formatFixed6(variable.number)) << '\n';
  }
  for (const Rule& rule : world.logic.rules) {
    // One line per rule, then one per condition and action belonging to
    // it. Flat lines survive hand-editing far better than nesting does.
    stream << "# rule " << escapeWord(rule.name) << ' ' << (rule.enabled ? "on" : "off") << ' '
           << triggerName(rule.trigger) << ' ' << escapeWord(rule.subject) << ' ' << escapeWord(rule.other)
           << ' ' << formatFixed6(rule.number) << '\n';
    for (const Condition& condition : rule.conditions) {
      stream << "# if " << escapeWord(condition.variable) << ' ' << compareName(condition.compare) << ' '
             << (condition.useText ? "text" : "number") << ' '
             << (condition.useText ? escapeWord(condition.text) : formatFixed6(condition.number)) << '\n';
    }
    for (const Action& action : rule.actions) {
      stream << "# do " << actName(action.act) << ' ' << escapeWord(action.target) << ' '
             << escapeWord(action.text) << ' ' << formatFixed6(action.number)
             << ' ' << formatFixed6(action.amount.x) << ' ' << formatFixed6(action.amount.y) << ' '
             << formatFixed6(action.amount.z) << '\n';
    }
  }
  // Entity lines: everything after the v1 header line.
  const usize headerEnd = sceneText.find('\n');
  if (headerEnd != std::string::npos) stream << sceneText.substr(headerEnd + 1U);
  out = stream.str();
  return true;
}

bool WorldIO::saveToFile(const WorldData& world, const std::string& path, std::string& error) {
  std::string text;
  if (!save(world, text)) {
    error = "failed to serialize world";
    return false;
  }
  std::ofstream file(path, std::ios::binary);
  if (!file) {
    error = "failed to write world file: " + path;
    return false;
  }
  file << text;
  return static_cast<bool>(file);
}

bool WorldIO::load(const std::string& text, WorldData& out, std::string& error) {
  Scene scene;
  if (!SceneIO::load(text, scene, error)) return false;
  out = WorldData{};
  out.scene = std::move(scene);
  // Scan the comment lines for our metadata; anything unknown is ignored
  // (tolerant load — old v1 files simply use the defaults).
  bool hasField = false;
  std::istringstream stream(text);
  std::string line;
  while (std::getline(stream, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line.rfind("# world name ", 0) == 0) {
      out.name = unescapeLineText(line.substr(13U));
    } else if (line.rfind(kProfilePrefix, 0) == 0) {
      const std::string body = line.substr(10U);
      if (ProfileIO::parseLine(body, out.profile) && body.rfind("field ", 0) == 0) hasField = true;
    } else if (line.rfind("# player speed ", 0) == 0) {
      std::istringstream tokens(line.substr(15U));
      std::string speedToken;
      tokens >> speedToken;
      f64 speed = kWorldPlayerNormal;
      if (parseF64Token(speedToken, speed)) out.player.speed = speed;
      std::string keyword;
      std::string tr;
      std::string tg;
      std::string tb;
      f64 r = 0.0;
      f64 g = 0.0;
      f64 b = 0.0;
      if (tokens >> keyword >> tr >> tg >> tb && keyword == "color" && parseF64Token(tr, r) &&
          parseF64Token(tg, g) && parseF64Token(tb, b)) {
        out.player.color = Vec3{r, g, b};
      }
    } else if (line.rfind("# ball type ", 0) == 0) {
      BallType type = BallType::Accurate;
      ballTypeFromName(line.substr(12U), type);
      applyBallType(out.ball, type);
    } else if (line.rfind("# env ", 0) == 0) {
      EnvironmentKind kind = EnvironmentKind::Grass;
      environmentFromName(line.substr(6U), kind);
      out.environment = kind;
    } else if (line.rfind("# score ", 0) == 0) {
      const std::string token = line.substr(8U);
      try {
        usize consumed = 0;
        const unsigned long long value = std::stoull(token, &consumed);
        if (consumed == token.size()) out.score = static_cast<u32>(value);
      } catch (...) {
      }
    } else if (line.rfind("# match ", 0) == 0) {
      std::istringstream tokens(line.substr(8U));
      unsigned long long first = 0ULL;
      unsigned long long second = 0ULL;
      if (tokens >> first >> second) {
        out.scoreTeam1 = static_cast<u32>(first);
        out.scoreTeam2 = static_cast<u32>(second);
      }
    } else if (line.rfind("# best ", 0) == 0) {
      const std::string token = line.substr(7U);
      try {
        usize consumed = 0;
        const unsigned long long value = std::stoull(token, &consumed);
        if (consumed == token.size()) out.bestRound = static_cast<u32>(value);
      } catch (...) {
      }
    } else if (line.rfind("# var ", 0) == 0) {
      const std::vector<std::string> parts = splitWords(line.substr(6U));
      if (parts.size() >= 3U) {
        Variable variable;
        variable.name = unescapeWord(parts[0]);
        variable.isText = parts[1] == "text";
        if (variable.isText) {
          variable.text = unescapeWord(parts[2]);
        } else {
          variable.number = parseNumber(parts[2]);
        }
        out.logic.variables.push_back(variable);
      }
    } else if (line.rfind("# rule ", 0) == 0) {
      const std::vector<std::string> parts = splitWords(line.substr(7U));
      if (parts.size() >= 5U) {
        Rule rule;
        rule.name = unescapeWord(parts[0]);
        rule.enabled = parts[1] != "off";
        if (!triggerFromName(parts[2], rule.trigger)) rule.trigger = Trigger::Start;
        rule.subject = unescapeWord(parts[3]);
        rule.other = unescapeWord(parts[4]);
        if (parts.size() >= 6U) rule.number = parseNumber(parts[5]);
        out.logic.rules.push_back(rule);
      }
    } else if (line.rfind("# if ", 0) == 0) {
      // Belongs to the rule above it; a stray one with no rule is dropped
      // rather than inventing a rule to hang it on.
      const std::vector<std::string> parts = splitWords(line.substr(5U));
      if (parts.size() >= 4U && !out.logic.rules.empty()) {
        Condition condition;
        condition.variable = unescapeWord(parts[0]);
        if (!compareFromName(parts[1], condition.compare)) condition.compare = Compare::Equal;
        condition.useText = parts[2] == "text";
        if (condition.useText) {
          condition.text = unescapeWord(parts[3]);
        } else {
          condition.number = parseNumber(parts[3]);
        }
        out.logic.rules.back().conditions.push_back(condition);
      }
    } else if (line.rfind("# do ", 0) == 0) {
      const std::vector<std::string> parts = splitWords(line.substr(5U));
      if (parts.size() >= 7U && !out.logic.rules.empty()) {
        Action action;
        if (!actFromName(parts[0], action.act)) action.act = Act::SetVariable;
        action.target = unescapeWord(parts[1]);
        action.text = unescapeWord(parts[2]);
        action.number = parseNumber(parts[3]);
        action.amount = Vec3{parseNumber(parts[4]), parseNumber(parts[5]), parseNumber(parts[6])};
        out.logic.rules.back().actions.push_back(action);
      }
    }
  }
  if (!hasField) fieldFromGround(out);
  return true;
}

bool WorldIO::loadFromFile(const std::string& path, WorldData& out, std::string& error) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    error = "failed to open world file: " + path;
    return false;
  }
  std::ostringstream buffer;
  buffer << file.rdbuf();
  if (file.bad()) {
    error = "failed to read world file: " + path;
    return false;
  }
  return load(buffer.str(), out, error);
}

}  // namespace kimia
