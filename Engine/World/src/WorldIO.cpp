#include <kimia/WorldIO.h>

#include <kimia/SceneIO.h>

#include <cstdio>
#include <fstream>
#include <sstream>

namespace kimia {

namespace {

std::string escapeName(const std::string& name) {
  std::string out;
  out.reserve(name.size());
  for (const char c : name) {
    if (c == '\\') {
      out += "\\\\";
    } else if (c == '\n') {
      out += "\\n";
    } else {
      out += c;
    }
  }
  return out;
}

std::string unescapeName(const std::string& text) {
  std::string out;
  out.reserve(text.size());
  for (usize i = 0; i < text.size(); ++i) {
    if (text[i] == '\\' && i + 1U < text.size()) {
      ++i;
      out += text[i] == 'n' ? '\n' : text[i];
    } else {
      out += text[i];
    }
  }
  return out;
}

bool parseF64(const std::string& token, f64& out) {
  if (token.empty()) return false;
  try {
    usize consumed = 0;
    out = std::stod(token, &consumed);
    return consumed == token.size();
  } catch (...) {
    return false;
  }
}

std::string formatF64(f64 value) {
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "%.6f", value);
  return buffer;
}

const char* ballTypeName(BallType type) { return type == BallType::Fantasy ? "fantasy" : "accurate"; }

const char* environmentName(EnvironmentKind kind) {
  switch (kind) {
    case EnvironmentKind::Sand:
      return "sand";
    case EnvironmentKind::Night:
      return "night";
    default:
      return "grass";
  }
}

}  // namespace

bool WorldIO::save(const WorldData& world, std::string& out) {
  std::string sceneText;
  if (!SceneIO::save(world.scene, sceneText)) return false;
  std::ostringstream stream;
  stream << "# KIMIA scene v1\n";
  stream << "# world name " << escapeName(world.name) << '\n';
  stream << "# player speed " << formatF64(world.player.speed) << " color " << formatF64(world.player.color.x)
         << ' ' << formatF64(world.player.color.y) << ' ' << formatF64(world.player.color.z) << '\n';
  stream << "# ball type " << ballTypeName(world.ball.type) << '\n';
  stream << "# env " << environmentName(world.environment) << '\n';
  stream << "# score " << world.score << '\n';
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
  std::istringstream stream(text);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.rfind("# world name ", 0) == 0) {
      out.name = unescapeName(line.substr(13U));
    } else if (line.rfind("# player speed ", 0) == 0) {
      std::istringstream tokens(line.substr(15U));
      std::string speedToken;
      tokens >> speedToken;
      f64 speed = kWorldPlayerNormal;
      if (parseF64(speedToken, speed)) out.player.speed = speed;
      std::string keyword;
      std::string tr;
      std::string tg;
      std::string tb;
      f64 r = 0.0;
      f64 g = 0.0;
      f64 b = 0.0;
      if (tokens >> keyword >> tr >> tg >> tb && keyword == "color" && parseF64(tr, r) && parseF64(tg, g) &&
          parseF64(tb, b)) {
        out.player.color = Vec3{r, g, b};
      }
    } else if (line.rfind("# ball type ", 0) == 0) {
      applyBallType(out.ball, line.substr(12U) == "fantasy" ? BallType::Fantasy : BallType::Accurate);
    } else if (line.rfind("# env ", 0) == 0) {
      const std::string kind = line.substr(6U);
      if (kind == "sand") {
        out.environment = EnvironmentKind::Sand;
      } else if (kind == "night") {
        out.environment = EnvironmentKind::Night;
      } else {
        out.environment = EnvironmentKind::Grass;
      }
    } else if (line.rfind("# score ", 0) == 0) {
      const std::string token = line.substr(8U);
      try {
        usize consumed = 0;
        const unsigned long long value = std::stoull(token, &consumed);
        if (consumed == token.size()) out.score = static_cast<u32>(value);
      } catch (...) {
      }
    }
  }
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
