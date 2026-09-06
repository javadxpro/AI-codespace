#include <kimia/GameProfile.h>

#include <kimia/TextFormat.h>

#include <dirent.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

namespace kimia {

namespace {

constexpr const char* kHeader = "# KIMIA profile v1";

// Jump apex limits: 0 disables jumping, 5 m is already a superhero.
constexpr f64 kJumpMin = 0.0;
constexpr f64 kJumpMax = 5.0;
constexpr f64 kSpeedMin = 0.5;
constexpr f64 kSpeedMax = 20.0;
constexpr f64 kKickMin = 0.0;
constexpr f64 kKickMax = 40.0;
constexpr f64 kParMin = 1.0;
constexpr f64 kParMax = 20.0;

f64 clampF64(f64 value, f64 low, f64 high) { return std::min(high, std::max(low, value)); }

bool isIdentifier(const std::string& token) {
  if (token.empty()) return false;
  for (const char c : token) {
    const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' ||
                    c == '-';
    if (!ok) return false;
  }
  return true;
}

bool hasProfileExtension(const std::string& name) {
  static const char* kExt = ".kimiaprofile";
  const usize extLength = 13U;
  if (name.size() <= extLength) return false;
  for (usize i = 0; i < extLength; ++i) {
    char c = name[name.size() - extLength + i];
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    if (c != kExt[i]) return false;
  }
  return true;
}

// Trailing '\r' from files edited on Windows must not break the last value.
std::string stripCR(std::string line) {
  while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) line.pop_back();
  return line;
}

}  // namespace

const char* ballTypeName(BallType type) { return type == BallType::Fantasy ? "fantasy" : "accurate"; }

bool ballTypeFromName(const std::string& name, BallType& out) {
  if (name == "fantasy") {
    out = BallType::Fantasy;
    return true;
  }
  if (name == "accurate") {
    out = BallType::Accurate;
    return true;
  }
  return false;
}

const char* environmentName(EnvironmentKind kind) {
  switch (kind) {
    case EnvironmentKind::Sand:
      return "sand";
    case EnvironmentKind::Night:
      return "night";
    case EnvironmentKind::Asphalt:
      return "asphalt";
    default:
      return "grass";
  }
}

bool environmentFromName(const std::string& name, EnvironmentKind& out) {
  if (name == "grass") {
    out = EnvironmentKind::Grass;
  } else if (name == "sand") {
    out = EnvironmentKind::Sand;
  } else if (name == "night") {
    out = EnvironmentKind::Night;
  } else if (name == "asphalt") {
    out = EnvironmentKind::Asphalt;
  } else {
    return false;
  }
  return true;
}

const char* playModeName(PlayMode mode) { return mode == PlayMode::Shot ? "shot" : "kick"; }

bool playModeFromName(const std::string& name, PlayMode& out) {
  if (name == "kick") {
    out = PlayMode::Kick;
    return true;
  }
  if (name == "shot") {
    out = PlayMode::Shot;
    return true;
  }
  return false;
}

const char* scoringName(Scoring scoring) { return scoring == Scoring::Hole ? "hole" : "gate"; }

bool scoringFromName(const std::string& name, Scoring& out) {
  if (name == "gate") {
    out = Scoring::Gate;
    return true;
  }
  if (name == "hole") {
    out = Scoring::Hole;
    return true;
  }
  return false;
}

std::vector<GameProfile> builtinProfiles() {
  std::vector<GameProfile> profiles;

  // گلف — game #1: learn and stress the engine. No runner: aim, charge,
  // shoot the accurate ball from where it rests into a cup. The launch
  // numbers are the reference golf's (2.5 + power * 13.5, no pop).
  GameProfile golf;
  golf.name = "golf";
  golf.title = "گلف کیمیا";
  golf.fieldLength = 24.0;
  golf.fieldWidth = 10.0;
  golf.environment = EnvironmentKind::Grass;
  golf.playerSpeed = kWorldPlayerNormal;
  golf.jumpHeight = 0.0;
  golf.ballDefault = BallType::Accurate;
  golf.ballChoice = false;
  golf.kickBase = 2.5;
  golf.kickSpeedScale = 13.5;
  golf.kickUp = 0.0;
  golf.mode = PlayMode::Shot;
  golf.scoring = Scoring::Hole;
  golf.par = 3U;
  profiles.push_back(golf);

  // فوتبال خیابونی ایران: کوی ابوذر — a tight 5v5 court between walls, a
  // bouncy ball that begs for tricks, a fast player with a high jump.
  GameProfile street;
  street.name = "street";
  street.title = "فوتبال خیابونی ایران: کوی ابوذر";
  street.fieldLength = 16.0;
  street.fieldWidth = 5.0;
  street.environment = EnvironmentKind::Asphalt;
  street.playerSpeed = 5.0;
  street.jumpHeight = 1.8;
  street.ballDefault = BallType::Fantasy;
  street.ballChoice = false;
  street.kickBase = 3.0;
  street.kickSpeedScale = 0.6;
  street.kickUp = 2.0;
  street.teamSize = 5U;         // ۵ در برابر ۵
  street.matchSeconds = 300.0;  // ۵ دقیقه
  profiles.push_back(street);

  // زمین چمن: کوی ابوذر — the professional 11v11 game: a real ball, a real
  // pitch, no fantasy.
  GameProfile grass;
  grass.name = "grass";
  grass.title = "زمین چمن: کوی ابوذر";
  grass.fieldLength = 40.0;
  grass.fieldWidth = 25.0;
  grass.environment = EnvironmentKind::Grass;
  grass.playerSpeed = 4.0;
  grass.jumpHeight = 0.6;
  grass.ballDefault = BallType::Accurate;
  grass.ballChoice = false;
  grass.kickBase = 4.0;
  grass.kickSpeedScale = 0.8;
  grass.kickUp = 0.8;
  grass.teamSize = 11U;        // ۱۱ در برابر ۱۱
  grass.matchSeconds = 600.0;  // ۱۰ دقیقه
  profiles.push_back(grass);

  // مسابقه واقعی: بتل گراند — PUBG-like, 4-player squads; this 40 x 40 field
  // is the Arena mode. Not pay-to-win.
  GameProfile battleground;
  battleground.name = "battleground";
  battleground.title = "مسابقه واقعی: بتل گراند";
  battleground.fieldLength = 40.0;
  battleground.fieldWidth = 40.0;
  battleground.environment = EnvironmentKind::Sand;
  battleground.playerSpeed = 4.5;
  battleground.jumpHeight = 1.0;
  battleground.ballDefault = BallType::Accurate;
  battleground.ballChoice = false;
  battleground.kickBase = 2.0;
  battleground.kickSpeedScale = 0.5;
  battleground.kickUp = 1.2;
  battleground.teamSize = 4U;         // تیم‌های ۴ نفره
  battleground.matchSeconds = 420.0;  // ۷ دقیقه
  profiles.push_back(battleground);

  // زمین آزاد — the sandbox: exactly the editor's original behaviour.
  profiles.push_back(GameProfile{});
  return profiles;
}

std::vector<GameProfile> loadProfiles(const std::string& dir) {
  std::vector<GameProfile> profiles = builtinProfiles();
  std::vector<std::string> files;
  DIR* handle = ::opendir(dir.c_str());
  if (handle != nullptr) {
    while (dirent* entry = ::readdir(handle)) {
      const std::string name = entry->d_name;
      if (hasProfileExtension(name)) files.push_back(name);
    }
    ::closedir(handle);
  }
  std::sort(files.begin(), files.end());
  for (const std::string& file : files) {
    const std::string path = (!dir.empty() && dir.back() != '/') ? dir + "/" + file : dir + file;
    GameProfile loaded;
    std::string error;
    if (!ProfileIO::loadFromFile(path, loaded, error)) continue;  // unreadable files never break the menu
    const auto same = std::find_if(profiles.begin(), profiles.end(),
                                   [&loaded](const GameProfile& p) { return p.name == loaded.name; });
    if (same != profiles.end()) {
      *same = loaded;
    } else {
      profiles.push_back(loaded);
    }
  }
  return profiles;
}

std::vector<std::string> ProfileIO::lines(const GameProfile& profile) {
  std::vector<std::string> out;
  out.push_back("name " + profile.name);
  out.push_back("title " + escapeLineText(profile.title));
  out.push_back("field " + formatFixed6(profile.fieldLength) + ' ' + formatFixed6(profile.fieldWidth));
  out.push_back(std::string("environment ") + environmentName(profile.environment));
  out.push_back("player speed " + formatFixed6(profile.playerSpeed) + " jump " + formatFixed6(profile.jumpHeight));
  out.push_back(std::string("ball ") + ballTypeName(profile.ballDefault) + " choice " +
                (profile.ballChoice ? "on" : "off"));
  out.push_back("kick " + formatFixed6(profile.kickBase) + ' ' + formatFixed6(profile.kickSpeedScale) + ' ' +
                formatFixed6(profile.kickUp));
  out.push_back(std::string("mode ") + playModeName(profile.mode));
  out.push_back(std::string("scoring ") + scoringName(profile.scoring));
  out.push_back("par " + std::to_string(profile.par));
  out.push_back("wind " + formatFixed6(profile.windSpeed) + ' ' + formatFixed6(profile.windDirection));
  out.push_back("team " + std::to_string(profile.teamSize));
  out.push_back("match " + formatFixed6(profile.matchSeconds));
  return out;
}

bool ProfileIO::parseLine(const std::string& rawLine, GameProfile& out) {
  const std::string line = stripCR(rawLine);
  std::istringstream tokens(line);
  std::string key;
  if (!(tokens >> key)) return false;
  if (key == "name") {
    std::string value;
    std::string extra;
    if (!(tokens >> value) || !isIdentifier(value)) return false;
    if (tokens >> extra) return false;  // the id is exactly one token
    out.name = value;
    return true;
  }
  if (key == "title") {
    const usize at = line.find("title ");
    if (at == std::string::npos || at + 6U >= line.size()) return false;
    out.title = unescapeLineText(line.substr(at + 6U));
    return true;
  }
  if (key == "field") {
    std::string a;
    std::string b;
    f64 length = 0.0;
    f64 width = 0.0;
    if (!(tokens >> a >> b) || !parseF64Token(a, length) || !parseF64Token(b, width)) return false;
    out.fieldLength = clampF64(length, kProfileFieldMin, kProfileFieldMax);
    out.fieldWidth = clampF64(width, kProfileFieldMin, kProfileFieldMax);
    return true;
  }
  if (key == "environment") {
    std::string value;
    EnvironmentKind kind = EnvironmentKind::Grass;
    if (!(tokens >> value) || !environmentFromName(value, kind)) return false;
    out.environment = kind;
    return true;
  }
  if (key == "player") {
    std::string speedKey;
    std::string speedToken;
    std::string jumpKey;
    std::string jumpToken;
    f64 speed = 0.0;
    f64 jump = 0.0;
    if (!(tokens >> speedKey >> speedToken >> jumpKey >> jumpToken) || speedKey != "speed" || jumpKey != "jump" ||
        !parseF64Token(speedToken, speed) || !parseF64Token(jumpToken, jump)) {
      return false;
    }
    out.playerSpeed = clampF64(speed, kSpeedMin, kSpeedMax);
    out.jumpHeight = clampF64(jump, kJumpMin, kJumpMax);
    return true;
  }
  if (key == "ball") {
    std::string typeToken;
    std::string choiceKey;
    std::string choiceToken;
    BallType type = BallType::Accurate;
    if (!(tokens >> typeToken >> choiceKey >> choiceToken) || !ballTypeFromName(typeToken, type) ||
        choiceKey != "choice" || (choiceToken != "on" && choiceToken != "off")) {
      return false;
    }
    out.ballDefault = type;
    out.ballChoice = choiceToken == "on";
    return true;
  }
  if (key == "kick") {
    std::string a;
    std::string b;
    std::string c;
    f64 base = 0.0;
    f64 scale = 0.0;
    f64 up = 0.0;
    if (!(tokens >> a >> b >> c) || !parseF64Token(a, base) || !parseF64Token(b, scale) || !parseF64Token(c, up)) {
      return false;
    }
    out.kickBase = clampF64(base, kKickMin, kKickMax);
    out.kickSpeedScale = clampF64(scale, kKickMin, kKickMax);
    out.kickUp = clampF64(up, kKickMin, kKickMax);
    return true;
  }
  if (key == "mode") {
    std::string value;
    PlayMode mode = PlayMode::Kick;
    if (!(tokens >> value) || !playModeFromName(value, mode)) return false;
    out.mode = mode;
    return true;
  }
  if (key == "scoring") {
    std::string value;
    Scoring scoring = Scoring::Gate;
    if (!(tokens >> value) || !scoringFromName(value, scoring)) return false;
    out.scoring = scoring;
    return true;
  }
  if (key == "par") {
    std::string value;
    f64 par = 0.0;
    if (!(tokens >> value) || !parseF64Token(value, par)) return false;
    out.par = static_cast<u32>(clampF64(std::floor(par), kParMin, kParMax));
    return true;
  }
  if (key == "wind") {
    // `wind <speed> <direction>` — a line missing either value is ignored
    // as a whole (never half-applied), like every other key here.
    std::string speedToken;
    std::string directionToken;
    f64 speed = 0.0;
    f64 direction = 0.0;
    if (!(tokens >> speedToken >> directionToken) || !parseF64Token(speedToken, speed) ||
        !parseF64Token(directionToken, direction)) {
      return false;
    }
    out.windSpeed = clampF64(speed, 0.0, kProfileWindMax);
    out.windDirection = direction;
    return true;
  }
  if (key == "match") {
    std::string value;
    f64 seconds = 0.0;
    if (!(tokens >> value) || !parseF64Token(value, seconds)) return false;
    out.matchSeconds = clampF64(seconds, 0.0, kProfileMatchMax);
    return true;
  }
  if (key == "team") {
    std::string value;
    f64 team = 0.0;
    if (!(tokens >> value) || !parseF64Token(value, team)) return false;
    out.teamSize = static_cast<u32>(clampF64(std::floor(team), static_cast<f64>(kProfileTeamMin),
                                             static_cast<f64>(kProfileTeamMax)));
    return true;
  }
  return false;
}

std::string ProfileIO::save(const GameProfile& profile) {
  std::string out = std::string(kHeader) + '\n';
  for (const std::string& line : lines(profile)) out += line + '\n';
  return out;
}

bool ProfileIO::saveToFile(const GameProfile& profile, const std::string& path, std::string& error) {
  std::ofstream file(path, std::ios::binary);
  if (!file) {
    error = "failed to write profile file: " + path;
    return false;
  }
  file << save(profile);
  if (!file) {
    error = "failed to write profile file: " + path;
    return false;
  }
  return true;
}

bool ProfileIO::load(const std::string& text, GameProfile& out, std::string& error) {
  GameProfile parsed;
  bool named = false;
  std::istringstream stream(text);
  std::string line;
  while (std::getline(stream, line)) {
    line = stripCR(line);
    if (line.empty() || line[0] == '#') continue;
    const bool wasName = line.rfind("name ", 0) == 0;
    if (parseLine(line, parsed) && wasName) named = true;
  }
  if (!named) {
    error = "profile has no name line";
    return false;
  }
  out = parsed;
  return true;
}

bool ProfileIO::loadFromFile(const std::string& path, GameProfile& out, std::string& error) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    error = "failed to open profile file: " + path;
    return false;
  }
  std::ostringstream buffer;
  buffer << file.rdbuf();
  if (file.bad()) {
    error = "failed to read profile file: " + path;
    return false;
  }
  return load(buffer.str(), out, error);
}

}  // namespace kimia
