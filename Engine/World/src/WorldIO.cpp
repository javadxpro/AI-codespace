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
