#include <kimia/Mesh.h>

#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace kimia {

namespace {

struct ObjFaceCorner {
  i64 position = 0;
  i64 uv = 0;
  i64 normal = 0;
};

// Resolves an OBJ index: 1-based positive or negative (relative to count).
bool resolveIndex(i64 raw, usize count, usize& out) {
  if (raw > 0) {
    if (static_cast<usize>(raw) > count) return false;
    out = static_cast<usize>(raw) - 1U;
    return true;
  }
  if (raw < 0) {
    const i64 resolved = static_cast<i64>(count) + raw;
    if (resolved < 0) return false;
    out = static_cast<usize>(resolved);
    return true;
  }
  return false;  // index 0 is not allowed in OBJ
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

bool parseI64(const std::string& token, i64& out) {
  if (token.empty()) return false;
  try {
    usize consumed = 0;
    out = std::stoll(token, &consumed);
    return consumed == token.size();
  } catch (...) {
    return false;
  }
}

std::string trimmed(const std::string& line) {
  usize begin = 0;
  while (begin < line.size() && (line[begin] == ' ' || line[begin] == '\t' || line[begin] == '\r')) ++begin;
  usize end = line.size();
  while (end > begin && (line[end - 1U] == ' ' || line[end - 1U] == '\t' || line[end - 1U] == '\r')) --end;
  return line.substr(begin, end - begin);
}

std::vector<std::string> splitWhitespace(const std::string& text) {
  std::vector<std::string> tokens;
  std::istringstream stream(text);
  std::string token;
  while (stream >> token) tokens.push_back(token);
  return tokens;
}

// Quantized vertex key for deduplication.
using DedupeKey = std::tuple<i64, i64, i64, i64, i64, i64, i64, i64>;

i64 quantize(f64 value) { return static_cast<i64>(std::llround(value * 1000000.0)); }

DedupeKey vertexKey(const Vec3& position, const Vec3& normal, const Vec2& uv) {
  return DedupeKey{quantize(position.x), quantize(position.y), quantize(position.z), quantize(normal.x),
                   quantize(normal.y),   quantize(normal.z),   quantize(uv.x),         quantize(uv.y)};
}

}  // namespace

void dedupeVertices(MeshData& mesh) {
  std::map<DedupeKey, u32> seen;
  MeshData merged;
  merged.name = mesh.name;
  merged.indices.reserve(mesh.indices.size());
  for (u32 index : mesh.indices) {
    if (index >= mesh.positions.size()) return;  // defensive; validated upstream
    const DedupeKey key =
        vertexKey(mesh.positions[static_cast<usize>(index)], mesh.normals[static_cast<usize>(index)],
                  mesh.uvs.empty() ? Vec2{} : mesh.uvs[static_cast<usize>(index)]);
    auto found = seen.find(key);
    if (found != seen.end()) {
      merged.indices.push_back(found->second);
      continue;
    }
    const u32 newIndex = static_cast<u32>(merged.positions.size());
    merged.positions.push_back(mesh.positions[static_cast<usize>(index)]);
    merged.normals.push_back(mesh.normals[static_cast<usize>(index)]);
    if (!mesh.uvs.empty()) merged.uvs.push_back(mesh.uvs[static_cast<usize>(index)]);
    merged.indices.push_back(newIndex);
    seen.emplace(key, newIndex);
  }
  if (mesh.uvs.empty()) merged.uvs.clear();
  mesh = std::move(merged);
}

MeshData makeCube(f64 size) {
  struct Face {
    Vec3 n;
    Vec3 u;
    Vec3 v;
  };
  const Face faces[6] = {
      {Vec3{1.0, 0.0, 0.0}, Vec3{0.0, 0.0, -1.0}, Vec3{0.0, 1.0, 0.0}},   // +X
      {Vec3{-1.0, 0.0, 0.0}, Vec3{0.0, 0.0, 1.0}, Vec3{0.0, 1.0, 0.0}},   // -X
      {Vec3{0.0, 1.0, 0.0}, Vec3{1.0, 0.0, 0.0}, Vec3{0.0, 0.0, -1.0}},   // +Y
      {Vec3{0.0, -1.0, 0.0}, Vec3{1.0, 0.0, 0.0}, Vec3{0.0, 0.0, 1.0}},   // -Y
      {Vec3{0.0, 0.0, 1.0}, Vec3{1.0, 0.0, 0.0}, Vec3{0.0, 1.0, 0.0}},    // +Z
      {Vec3{0.0, 0.0, -1.0}, Vec3{-1.0, 0.0, 0.0}, Vec3{0.0, 1.0, 0.0}},  // -Z
  };
  MeshData mesh;
  mesh.name = "Cube";
  const f64 half = size * 0.5;
  for (const Face& face : faces) {
    for (i32 corner = 0; corner < 4; ++corner) {
      const f64 a = corner == 1 || corner == 2 ? 1.0 : 0.0;
      const f64 b = corner == 2 || corner == 3 ? 1.0 : 0.0;
      mesh.positions.push_back(face.n * half + face.u * (half * (2.0 * a - 1.0)) + face.v * (half * (2.0 * b - 1.0)));
      mesh.normals.push_back(face.n);
      mesh.uvs.push_back(Vec2{a, b});
    }
    const u32 base = static_cast<u32>(mesh.positions.size() - 4U);
    mesh.indices.insert(mesh.indices.end(),
                        {base, static_cast<u32>(base + 1U), static_cast<u32>(base + 2U), base, static_cast<u32>(base + 2U),
                         static_cast<u32>(base + 3U)});
  }
  return mesh;
}

MeshData makePlane(f64 width, f64 depth) {
  MeshData mesh;
  mesh.name = "Plane";
  const f64 hw = width * 0.5;
  const f64 hd = depth * 0.5;
  mesh.positions = {
      Vec3{-hw, 0.0, -hd},
      Vec3{-hw, 0.0, hd},
      Vec3{hw, 0.0, hd},
      Vec3{hw, 0.0, -hd},
  };
  mesh.normals.assign(4, Vec3{0.0, 1.0, 0.0});
  mesh.uvs = {
      Vec2{0.0, 0.0},
      Vec2{0.0, 1.0},
      Vec2{1.0, 1.0},
      Vec2{1.0, 0.0},
  };
  mesh.indices = {0U, 1U, 2U, 0U, 2U, 3U};
  return mesh;
}

MeshData makeSphere(u32 rings, u32 segments) {
  if (rings < 2U) rings = 2U;
  if (segments < 3U) segments = 3U;
  MeshData mesh;
  mesh.name = "Sphere";
  const f64 twoPi = 3.14159265358979323846 * 2.0;
  for (u32 r = 0; r <= rings; ++r) {
    const f64 phi = 3.14159265358979323846 * static_cast<f64>(r) / static_cast<f64>(rings);
    const f64 radius = std::sin(phi);
    const f64 y = std::cos(phi);
    for (u32 s = 0; s <= segments; ++s) {
      const f64 theta = twoPi * static_cast<f64>(s) / static_cast<f64>(segments);
      const Vec3 position{radius * std::cos(theta), y, radius * std::sin(theta)};
      mesh.positions.push_back(position);
      mesh.normals.push_back(position.normalized());
      mesh.uvs.push_back(Vec2{static_cast<f64>(s) / static_cast<f64>(segments),
                              static_cast<f64>(r) / static_cast<f64>(rings)});
    }
  }
  const u32 rowWidth = segments + 1U;
  for (u32 r = 0; r < rings; ++r) {
    for (u32 s = 0; s < segments; ++s) {
      const u32 a = r * rowWidth + s;
      const u32 b = a + 1U;
      const u32 c = a + rowWidth + 1U;
      const u32 d = a + rowWidth;
      mesh.indices.insert(mesh.indices.end(), {a, b, c, a, c, d});
    }
  }
  return mesh;
}

bool loadFromOBJText(const std::string& text, MeshData& out, std::string& error, bool dedupe) {
  std::vector<Vec3> objPositions;
  std::vector<Vec3> objNormals;
  std::vector<Vec2> objUVs;
  std::vector<ObjFaceCorner> corners;
  MeshData result;

  std::istringstream stream(text);
  std::string rawLine;
  usize lineNumber = 0;
  while (std::getline(stream, rawLine)) {
    ++lineNumber;
    const std::string line = trimmed(rawLine);
    if (line.empty() || line[0] == '#') continue;
    const std::vector<std::string> tokens = splitWhitespace(line);
    if (tokens.empty()) continue;

    const std::string& keyword = tokens[0];
    if (keyword == "o" || keyword == "g") {
      if (result.name.empty() && tokens.size() >= 2U) {
        result.name = tokens[1];
        continue;
      }
      continue;
    }
    if (keyword == "v" && tokens.size() >= 4U) {
      f64 x = 0.0, y = 0.0, z = 0.0;
      if (!parseF64(tokens[1], x) || !parseF64(tokens[2], y) || !parseF64(tokens[3], z)) {
        error = "OBJ line " + std::to_string(lineNumber) + ": bad vertex";
        return false;
      }
      objPositions.push_back(Vec3{x, y, z});
      continue;
    }
    if (keyword == "vn" && tokens.size() >= 4U) {
      f64 x = 0.0, y = 0.0, z = 0.0;
      if (!parseF64(tokens[1], x) || !parseF64(tokens[2], y) || !parseF64(tokens[3], z)) {
        error = "OBJ line " + std::to_string(lineNumber) + ": bad normal";
        return false;
      }
      objNormals.push_back(Vec3{x, y, z});
      continue;
    }
    if (keyword == "vt" && tokens.size() >= 3U) {
      f64 u = 0.0, v = 0.0;
      if (!parseF64(tokens[1], u) || !parseF64(tokens[2], v)) {
        error = "OBJ line " + std::to_string(lineNumber) + ": bad UV";
        return false;
      }
      objUVs.push_back(Vec2{u, 1.0 - v});  // OBJ v=0 is bottom; images have row 0 on top
      continue;
    }
    if (keyword == "f" && tokens.size() >= 4U) {
      corners.clear();
      bool ok = true;
      for (usize t = 1; t < tokens.size(); ++t) {
        ObjFaceCorner corner;
        const std::string& token = tokens[t];
        // Split on '/' into up to 3 parts: position[/uv[/normal]].
        std::string parts[3];
        i32 partCount = 0;
        usize start = 0;
        while (start <= token.size() && partCount < 3) {
          const usize slash = token.find('/', start);
          if (slash == std::string::npos) {
            parts[partCount] = token.substr(start);
            ++partCount;
            break;
          }
          parts[partCount] = token.substr(start, slash - start);
          ++partCount;
          start = slash + 1U;
        }
        if (partCount < 1 || parts[0].empty()) {
          ok = false;
          break;
        }
        i64 raw = 0;
        if (!parseI64(parts[0], raw)) {
          ok = false;
          break;
        }
        corner.position = raw;
        if (partCount >= 2 && !parts[1].empty()) {
          if (!parseI64(parts[1], raw)) {
            ok = false;
            break;
          }
          corner.uv = raw;
        }
        if (partCount >= 3 && !parts[2].empty()) {
          if (!parseI64(parts[2], raw)) {
            ok = false;
            break;
          }
          corner.normal = raw;
        }
        corners.push_back(corner);
      }
      if (!ok) {
        error = "OBJ line " + std::to_string(lineNumber) + ": bad face";
        return false;
      }
      // One vertex per face corner, then fan-triangulate by index (a quad
      // face yields 4 vertices / 6 indices, matching the canonical counts).
      std::vector<u32> cornerVertices;
      cornerVertices.reserve(corners.size());
      for (const ObjFaceCorner& corner : corners) {
        usize pi = 0;
        if (!resolveIndex(corner.position, objPositions.size(), pi)) {
          error = "OBJ line " + std::to_string(lineNumber) + ": position index out of range";
          return false;
        }
        Vec3 normal{0.0, 0.0, 0.0};
        if (corner.normal != 0) {
          usize ni = 0;
          if (!resolveIndex(corner.normal, objNormals.size(), ni)) {
            error = "OBJ line " + std::to_string(lineNumber) + ": normal index out of range";
            return false;
          }
          normal = objNormals[ni];
        }
        Vec2 uv{0.0, 0.0};
        if (corner.uv != 0) {
          usize ui = 0;
          if (!resolveIndex(corner.uv, objUVs.size(), ui)) {
            error = "OBJ line " + std::to_string(lineNumber) + ": UV index out of range";
            return false;
          }
          uv = objUVs[ui];
        }
        cornerVertices.push_back(static_cast<u32>(result.positions.size()));
        result.positions.push_back(objPositions[pi]);
        result.normals.push_back(normal);
        if (!objUVs.empty()) result.uvs.push_back(uv);
      }
      for (usize k = 1; k + 1U < cornerVertices.size(); ++k) {
        result.indices.push_back(cornerVertices[0]);
        result.indices.push_back(cornerVertices[k]);
        result.indices.push_back(cornerVertices[k + 1U]);
      }
      continue;
    }
    // Unknown keywords (mtllib, s, usemtl, ...) are skipped: tolerant load.
  }

  if (result.positions.empty()) {
    error = "OBJ contains no geometry";
    return false;
  }
  if (objNormals.empty()) {
    // No vn in file: normals are left as zero vectors (absent), renderers may
    // regenerate them. Keep the contract positions.size() == normals.size().
  }
  if (objUVs.empty()) result.uvs.clear();
  if (result.name.empty()) result.name = "Mesh";
  if (dedupe) dedupeVertices(result);
  out = std::move(result);
  return true;
}

bool loadFromOBJFile(const std::string& path, MeshData& out, std::string& error, bool dedupe) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    error = "cannot open file: " + path;
    return false;
  }
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return loadFromOBJText(buffer.str(), out, error, dedupe);
}

bool meshToText(const MeshData& mesh, std::string& out) {
  if (!mesh.isValid()) return false;
  std::ostringstream stream;
  stream << "# KIMIA mesh v1\n";
  stream << "name " << (mesh.name.empty() ? "Mesh" : mesh.name) << '\n';
  stream << std::fixed << std::setprecision(9);
  stream << "positions " << mesh.positions.size() << '\n';
  for (const Vec3& p : mesh.positions) stream << p.x << ' ' << p.y << ' ' << p.z << '\n';
  stream << "normals " << mesh.normals.size() << '\n';
  for (const Vec3& n : mesh.normals) stream << n.x << ' ' << n.y << ' ' << n.z << '\n';
  stream << "uvs " << mesh.uvs.size() << '\n';
  for (const Vec2& uv : mesh.uvs) stream << uv.x << ' ' << uv.y << '\n';
  stream << "indices " << mesh.indices.size() << '\n';
  for (usize i = 0; i + 2U < mesh.indices.size(); i += 3U) {
    stream << mesh.indices[i] << ' ' << mesh.indices[i + 1U] << ' ' << mesh.indices[i + 2U] << '\n';
  }
  out = stream.str();
  return true;
}

bool meshFromText(const std::string& text, MeshData& out, std::string& error) {
  MeshData result;
  std::string section;
  usize expectedPositions = 0;
  usize expectedNormals = 0;
  usize expectedUVs = 0;
  usize expectedIndices = 0;
  usize lineNumber = 0;
  std::istringstream stream(text);
  std::string rawLine;
  bool headerSeen = false;

  auto readCount = [](const std::vector<std::string>& tokens) -> std::optional<usize> {
    if (tokens.size() != 2U) return std::nullopt;
    i64 parsed = 0;
    if (!parseI64(tokens[1], parsed) || parsed < 0) return std::nullopt;
    return static_cast<usize>(parsed);
  };

  while (std::getline(stream, rawLine)) {
    ++lineNumber;
    const std::string line = trimmed(rawLine);
    if (line.empty()) continue;
    if (line[0] == '#') {
      if (line.find("KIMIA mesh") != std::string::npos) headerSeen = true;
      continue;
    }
    const std::vector<std::string> tokens = splitWhitespace(line);
    if (tokens.empty()) continue;
    const std::string& keyword = tokens[0];
    if (keyword == "name") {
      result.name = tokens.size() >= 2U ? line.substr(line.find(tokens[1])) : "";
      section.clear();
      continue;
    }
    if (keyword == "positions" || keyword == "normals" || keyword == "uvs" || keyword == "indices") {
      const auto count = readCount(tokens);
      if (!count.has_value()) {
        error = "line " + std::to_string(lineNumber) + ": bad count";
        return false;
      }
      if (keyword == "positions") expectedPositions = *count;
      if (keyword == "normals") expectedNormals = *count;
      if (keyword == "uvs") expectedUVs = *count;
      if (keyword == "indices") expectedIndices = *count;
      section = keyword;
      continue;
    }
    // Data line for the current section (or unknown keyword: skipped).
    if (section == "positions" && tokens.size() >= 3U) {
      f64 x = 0.0, y = 0.0, z = 0.0;
      if (parseF64(tokens[0], x) && parseF64(tokens[1], y) && parseF64(tokens[2], z)) {
        result.positions.push_back(Vec3{x, y, z});
        continue;
      }
    }
    if (section == "normals" && tokens.size() >= 3U) {
      f64 x = 0.0, y = 0.0, z = 0.0;
      if (parseF64(tokens[0], x) && parseF64(tokens[1], y) && parseF64(tokens[2], z)) {
        result.normals.push_back(Vec3{x, y, z});
        continue;
      }
    }
    if (section == "uvs" && tokens.size() >= 2U) {
      f64 u = 0.0, v = 0.0;
      if (parseF64(tokens[0], u) && parseF64(tokens[1], v)) {
        result.uvs.push_back(Vec2{u, v});
        continue;
      }
    }
    if (section == "indices" && tokens.size() >= 3U) {
      i64 a = 0, b = 0, c = 0;
      if (parseI64(tokens[0], a) && parseI64(tokens[1], b) && parseI64(tokens[2], c) && a >= 0 && b >= 0 && c >= 0) {
        result.indices.push_back(static_cast<u32>(a));
        result.indices.push_back(static_cast<u32>(b));
        result.indices.push_back(static_cast<u32>(c));
        continue;
      }
    }
    // Unknown or malformed data lines are skipped (tolerant load).
  }

  if (!headerSeen && result.positions.empty()) {
    error = "not a KIMIA mesh file";
    return false;
  }
  if (result.positions.size() != expectedPositions || result.normals.size() != expectedNormals) {
    error = "vertex count mismatch";
    return false;
  }
  if (!result.uvs.empty() && result.uvs.size() != expectedUVs) {
    error = "uv count mismatch";
    return false;
  }
  if (result.indices.empty() || result.indices.size() != expectedIndices || result.indices.size() % 3U != 0U) {
    error = "bad index data";
    return false;
  }
  for (u32 index : result.indices) {
    if (index >= result.positions.size()) {
      error = "index out of range";
      return false;
    }
  }
  if (result.uvs.empty() && expectedUVs != 0U) result.uvs.resize(expectedPositions);
  out = std::move(result);
  return true;
}

}  // namespace kimia
