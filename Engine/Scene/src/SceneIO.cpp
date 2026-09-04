#include <kimia/SceneIO.h>

#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

namespace kimia {

namespace {

constexpr const char* kHeader = "# KIMIA scene v1";

std::string trimmed(const std::string& line) {
  usize begin = 0;
  while (begin < line.size() && (line[begin] == ' ' || line[begin] == '\t' || line[begin] == '\r')) ++begin;
  usize end = line.size();
  while (end > begin && (line[end - 1U] == ' ' || line[end - 1U] == '\t' || line[end - 1U] == '\r')) --end;
  return line.substr(begin, end - begin);
}

// Splits a line into tokens. A double-quoted section becomes ONE token with
// quotes removed and \" / \\ unescaped, so names may contain spaces.
std::vector<std::string> tokenizeLine(const std::string& line) {
  std::vector<std::string> tokens;
  usize i = 0;
  while (i < line.size()) {
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
    if (i >= line.size()) break;
    if (line[i] == '"') {
      ++i;
      std::string token;
      bool closed = false;
      while (i < line.size()) {
        const char c = line[i];
        if (c == '\\' && i + 1U < line.size()) {
          token += line[i + 1U];
          i += 2U;
          continue;
        }
        if (c == '"') {
          ++i;
          closed = true;
          break;
        }
        token += c;
        ++i;
      }
      tokens.push_back(std::move(token));
      if (!closed) break;  // unterminated quote: rest of line is the token
      continue;
    }
    const usize start = i;
    while (i < line.size() && line[i] != ' ' && line[i] != '\t') ++i;
    tokens.push_back(line.substr(start, i - start));
  }
  return tokens;
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

// Reads tokens[i+1 .. i+3] as a Vec3 and advances i past them.
bool parseVec3(const std::vector<std::string>& tokens, usize& i, Vec3& out) {
  if (i + 3U >= tokens.size()) return false;
  f64 x = 0.0, y = 0.0, z = 0.0;
  if (!parseF64(tokens[i + 1U], x) || !parseF64(tokens[i + 2U], y) || !parseF64(tokens[i + 3U], z)) return false;
  out = Vec3{x, y, z};
  i += 4U;
  return true;
}

// Deterministic shortest-ish formatting: guarantees byte-stable round-trips.
std::string format(f64 value) {
  std::ostringstream stream;
  stream << std::setprecision(9) << value;
  return stream.str();
}

std::string quoteName(const std::string& name) {
  std::string out;
  out.reserve(name.size() + 2U);
  out += '"';
  for (const char c : name) {
    if (c == '\\' || c == '"') out += '\\';
    out += c;
  }
  out += '"';
  return out;
}

const char* meshName(MeshKind kind) {
  switch (kind) {
    case MeshKind::cube:
      return "cube";
    case MeshKind::plane:
      return "plane";
    case MeshKind::sphere:
      return "sphere";
  }
  return "cube";
}

std::optional<MeshKind> meshKind(const std::string& token) {
  if (token == "cube") return MeshKind::cube;
  if (token == "plane") return MeshKind::plane;
  if (token == "sphere") return MeshKind::sphere;
  return std::nullopt;
}

}  // namespace

bool SceneIO::save(const Scene& scene, std::string& out) {
  std::ostringstream stream;
  stream << kHeader << '\n';
  scene.forEach([&stream](EntityHandle, const EntityData& entity) {
    const Transform& t = entity.transform;
    stream << "e " << quoteName(entity.name) << " mesh " << meshName(entity.mesh);
    stream << " pos " << format(t.position.x) << ' ' << format(t.position.y) << ' ' << format(t.position.z);
    stream << " scale " << format(t.scale.x) << ' ' << format(t.scale.y) << ' ' << format(t.scale.z);
    stream << " color " << format(entity.color.x) << ' ' << format(entity.color.y) << ' ' << format(entity.color.z);
    stream << " rough " << format(entity.roughness) << '\n';
  });
  if (scene.demoShot.has_value()) {
    stream << "# demo " << format(scene.demoShot->aim) << ' ' << format(scene.demoShot->power) << '\n';
  }
  out = stream.str();
  return true;
}

bool SceneIO::saveToFile(const Scene& scene, const std::string& path) {
  std::string text;
  if (!save(scene, text)) return false;
  std::ofstream file(path, std::ios::binary);
  if (!file) return false;
  file << text;
  return static_cast<bool>(file);
}

bool SceneIO::load(const std::string& text, Scene& out, std::string& error) {
  static_cast<void>(error);  // tolerant loader: text is never a hard error
  Scene result;
  std::istringstream stream(text);
  std::string rawLine;
  while (std::getline(stream, rawLine)) {
    const std::string line = trimmed(rawLine);
    if (line.empty()) continue;
    if (line[0] == '#') {
      // Comments are ignored except "# demo <aim> <power>".
      const std::string body = trimmed(line.substr(1));
      const std::vector<std::string> tokens = tokenizeLine(body);
      if (tokens.size() >= 3U && tokens[0] == "demo") {
        f64 aim = 0.0;
        f64 power = 0.0;
        if (parseF64(tokens[1], aim) && parseF64(tokens[2], power)) {
          result.demoShot = DemoShot{aim, power};
        }
      }
      continue;
    }
    const std::vector<std::string> tokens = tokenizeLine(line);
    if (tokens.empty() || tokens[0] != "e" || tokens.size() < 2U) continue;

    EntityData entity;
    entity.name = tokens[1];
    bool complete = true;
    usize i = 2U;
    while (i < tokens.size() && complete) {
      const std::string& keyword = tokens[i];
      if (keyword == "mesh" && i + 1U < tokens.size()) {
        const auto kind = meshKind(tokens[i + 1U]);
        if (!kind.has_value()) {
          complete = false;  // unknown mesh kind: ignore the entity line
          break;
        }
        entity.mesh = *kind;
        i += 2U;
        continue;
      }
      if (keyword == "pos") {
        Vec3 value;
        if (!parseVec3(tokens, i, value)) {
          complete = false;
          break;
        }
        entity.transform.position = value;
        continue;
      }
      if (keyword == "scale") {
        Vec3 value;
        if (!parseVec3(tokens, i, value)) {
          complete = false;
          break;
        }
        entity.transform.scale = value;
        continue;
      }
      if (keyword == "color") {
        Vec3 value;
        if (!parseVec3(tokens, i, value)) {
          complete = false;
          break;
        }
        entity.color = value;
        continue;
      }
      if (keyword == "rough" && i + 1U < tokens.size()) {
        f64 value = 0.0;
        if (!parseF64(tokens[i + 1U], value)) {
          complete = false;
          break;
        }
        entity.roughness = value;
        i += 2U;
        continue;
      }
      ++i;  // unknown keyword: skip it (tolerant)
    }
    if (!complete) continue;  // partial line: ignored
    result.create(entity);
  }
  out = std::move(result);
  return true;
}

bool SceneIO::loadFromFile(const std::string& path, Scene& out, std::string& error) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    error = "cannot open scene file: " + path;
    return false;
  }
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return load(buffer.str(), out, error);
}

}  // namespace kimia
