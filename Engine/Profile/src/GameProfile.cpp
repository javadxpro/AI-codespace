#include <kimia/GameProfile.h>

#include <kimia/TextFormat.h>

#include <dirent.h>

#include <algorithm>
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

std::vector<GameProfile> builtinProfiles() {
  std::vector<GameProfile> profiles;

  // فوتبال خیابونی ایران — a tight 3v3 court between walls, a bouncy ball
  // that begs for tricks, a fast player with a high jump.
  GameProfile street;
  street.name = "street";
  street.title = "فوتبال خیابونی ایران";
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
  profiles.push_back(street);

  // زمین چمن — the professional game: a real ball, a real pitch, no fantasy.
  GameProfile grass;
  grass.name = "grass";
  grass.title = "زمین چمن";
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
  profiles.push_back(grass);

  // بتل گراند — a 40 x 40 arena; the precise game with the big map.
  GameProfile battleground;
  battleground.name = "battleground";
  battleground.title = "بتل گراند";
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
