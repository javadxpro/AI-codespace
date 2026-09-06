#include <kimia/AssetPipeline.h>

// Vendored third-party code: keep strict warnings scoped to our own code.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include <ufbx.h>
#pragma GCC diagnostic pop

#include <cmath>
#include <cstdint>
#include <fstream>
#include <map>
#include <functional>
#include <sstream>
#include <vector>

namespace kimia {
namespace assets {

namespace {

std::string lowercased(std::string value) {
  for (char& c : value) {
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
  }
  return value;
}

std::string extension(const std::string& path) {
  const usize slash = path.find_last_of("/\\");
  const usize dot = path.find_last_of('.');
  if (dot == std::string::npos) return "";
  if (slash != std::string::npos && dot < slash) return "";
  return lowercased(path.substr(dot));
}

std::string directoryOf(const std::string& path) {
  const usize slash = path.find_last_of("/\\");
  if (slash == std::string::npos) return "";
  return path.substr(0, slash);
}

std::string joinPath(const std::string& dir, const std::string& name) {
  if (dir.empty()) return name;
  if (name.empty()) return dir;
  if (dir.back() == '/' || dir.back() == '\\') return dir + name;
  return dir + "/" + name;
}

bool isAbsolute(const std::string& path) {
  if (path.empty()) return false;
  if (path[0] == '/' || path[0] == '\\') return true;
  return path.size() >= 2U && path[1] == ':';  // Windows drive
}

// Normalizes a resolved path: backslashes to slashes, `./` segments dropped.
std::string normalizePath(const std::string& path) {
  std::string out;
  out.reserve(path.size());
  for (usize i = 0; i < path.size(); ++i) {
    const char c = path[i];
    if (c == '\\') {
      out.push_back('/');
      continue;
    }
    if (c == '.' && !out.empty() && out.back() == '/' && i + 1U < path.size() &&
        (path[i + 1U] == '/' || path[i + 1U] == '\\')) {
      continue;  // drop "." path segments
    }
    out.push_back(c);
  }
  return out;
}

bool readTextFile(const std::string& path, std::string& out, std::string& error) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    error = "cannot open file: " + path;
    return false;
  }
  std::ostringstream buffer;
  buffer << file.rdbuf();
  out = buffer.str();
  return true;
}

std::string trimmed(const std::string& line) {
  usize begin = 0;
  while (begin < line.size() && (line[begin] == ' ' || line[begin] == '\t' || line[begin] == '\r')) ++begin;
  usize end = line.size();
  while (end > begin && (line[end - 1U] == ' ' || line[end - 1U] == '\t' || line[end - 1U] == '\r')) --end;
  return line.substr(begin, end - begin);
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

// File-name-safe material name for extracted texture files.
std::string sanitizedName(const std::string& name) {
  std::string out;
  for (char c : name) {
    const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
    out.push_back(ok ? c : '_');
  }
  if (out.empty()) out = "material";
  return out;
}

Vec3 transformPoint(const ufbx_matrix& m, const ufbx_vec3& p) {
  return Vec3{
      m.m00 * p.x + m.m01 * p.y + m.m02 * p.z + m.m03,
      m.m10 * p.x + m.m11 * p.y + m.m12 * p.z + m.m13,
      m.m20 * p.x + m.m21 * p.y + m.m22 * p.z + m.m23,
  };
}

Vec3 transformDirection(const ufbx_matrix& m, const ufbx_vec3& d) {
  return Vec3{
      m.m00 * d.x + m.m01 * d.y + m.m02 * d.z,
      m.m10 * d.x + m.m11 * d.y + m.m12 * d.z,
      m.m20 * d.x + m.m21 * d.y + m.m22 * d.z,
  };
}

std::string ufbxString(const ufbx_string& s) { return std::string(s.data, s.length); }

// --- MTL (Wavefront material library) ---

// Tolerant parser for the subset we use: `newmtl`, `Kd` (diffuse color) and
// `map_Kd` (diffuse texture). All other statements are skipped. For
// `map_Kd`, the last token is the file name (option tokens like `-s` are
// ignored) — this matches the common exporters.
bool loadFromMTLText(const std::string& text, std::vector<MaterialData>& out, std::string& error) {
  out.clear();
  std::istringstream stream(text);
  std::string rawLine;
  usize current = 0U;
  bool hasCurrent = false;
  usize lineNumber = 0U;
  while (std::getline(stream, rawLine)) {
    ++lineNumber;
    const std::string line = trimmed(rawLine);
    if (line.empty() || line[0] == '#') continue;
    std::istringstream tokens(line);
    std::string keyword;
    tokens >> keyword;
    if (keyword == "newmtl") {
      std::string name;
      tokens >> name;
      if (name.empty()) continue;
      out.push_back(MaterialData{});
      out.back().name = name;
      current = out.size() - 1U;
      hasCurrent = true;
      continue;
    }
    if (!hasCurrent) continue;  // statements before the first newmtl are ignored
    if (keyword == "Kd") {
      f64 r = 0.0, g = 0.0, b = 0.0;
      std::string token;
      if (!(tokens >> token) || !parseF64(token, r)) continue;
      if (!(tokens >> token) || !parseF64(token, g)) continue;
      if (!(tokens >> token) || !parseF64(token, b)) continue;
      out[current].color = Vec3{r, g, b};
      continue;
    }
    if (keyword == "map_Kd") {
      std::vector<std::string> rest;
      std::string token;
      while (tokens >> token) rest.push_back(token);
      if (rest.empty()) continue;
      out[current].texturePath = rest.back();  // last token is the file name
      continue;
    }
    // Ka, Ks, Ns, d, illum, ...: skipped (tolerant).
  }
  static_cast<void>(lineNumber);
  if (out.empty()) {
    error = "MTL contains no materials";
    return false;
  }
  return true;
}

// --- FBX implementation (geometry + materials) ---

void writeEmbeddedTexture(const ufbx_texture* texture, const std::string& fbxPath, usize index,
                          std::string& outPath) {
  outPath.clear();
  if (texture == nullptr || texture->content.data == nullptr || texture->content.size <= 4U) return;
  const u8* bytes = static_cast<const u8*>(texture->content.data);
  const bool isPng = bytes[0] == 0x89 && bytes[1] == 'P' && bytes[2] == 'N' && bytes[3] == 'G';
  const bool isJpeg = bytes[0] == 0xFF && bytes[1] == 0xD8 && bytes[2] == 0xFF;
  if (!isPng && !isJpeg) return;  // unknown embedded format: keep only the path

  const usize slash = fbxPath.find_last_of("/\\");
  const usize dot = fbxPath.find_last_of('.');
  const std::string base = dot != std::string::npos && (slash == std::string::npos || dot > slash)
                               ? fbxPath.substr(0, dot)
                               : fbxPath;
  const std::string name = texture->name.length > 0U ? ufbxString(texture->name) : "texture";
  outPath = base + "_" + sanitizedName(name) + "_" + std::to_string(index) + (isPng ? ".png" : ".jpg");
  std::ofstream file(outPath, std::ios::binary);
  if (!file) {
    outPath.clear();
    return;
  }
  file.write(reinterpret_cast<const char*>(texture->content.data), static_cast<std::streamsize>(texture->content.size));
}

std::optional<MeshAsset> loadFBXImpl(const std::string& path, std::string& error) {
  ufbx_load_opts opts{};
  opts.target_axes = ufbx_axes_right_handed_y_up;
  opts.target_unit_meters = 1.0;
  opts.generate_missing_normals = true;
  ufbx_error fbxError;
  ufbx_scene* scene = ufbx_load_file(path.c_str(), &opts, &fbxError);
  if (scene == nullptr) {
    error = "cannot load FBX '" + path + "'";
    if (fbxError.description.length > 0U) {
      error += ": " + std::string(fbxError.description.data, fbxError.description.length);
    }
    return std::nullopt;
  }

  MeshAsset asset;
  asset.materials.reserve(scene->materials.count);
  for (usize i = 0; i < scene->materials.count; ++i) {
    const ufbx_material* material = scene->materials.data[i];
    MaterialData out;
    out.name = ufbxString(material->name);
    // Color: prefer PBR base color, fall back to the FBX diffuse color.
    const ufbx_material_map* colorMap = nullptr;
    if (material->pbr.base_color.has_value) {
      colorMap = &material->pbr.base_color;
    } else if (material->fbx.diffuse_color.has_value) {
      colorMap = &material->fbx.diffuse_color;
    }
    if (colorMap != nullptr) {
      out.color = Vec3{static_cast<f64>(colorMap->value_vec3.x), static_cast<f64>(colorMap->value_vec3.y),
                       static_cast<f64>(colorMap->value_vec3.z)};
    }
    // Diffuse texture: embedded content wins; otherwise resolve the filename
    // against the FBX file's directory.
    const ufbx_texture* texture = nullptr;
    if (material->pbr.base_color.texture != nullptr) {
      texture = material->pbr.base_color.texture;
    } else if (material->fbx.diffuse_color.texture != nullptr) {
      texture = material->fbx.diffuse_color.texture;
    }
    if (texture != nullptr) {
      writeEmbeddedTexture(texture, path, i, out.texturePath);
      if (out.texturePath.empty() && texture->filename.length > 0U) {
        const std::string filename = ufbxString(texture->filename);
        out.texturePath = isAbsolute(filename) ? filename : joinPath(directoryOf(path), filename);
      }
    }
    asset.materials.push_back(std::move(out));
  }

  asset.subMeshes.reserve(scene->meshes.count);
  std::vector<u32> triangleBuffer(64U);
  for (usize meshIndex = 0; meshIndex < scene->meshes.count; ++meshIndex) {
    const ufbx_mesh* mesh = scene->meshes.data[meshIndex];
    const ufbx_node* node = mesh->instances.count > 0U ? mesh->instances.data[0] : nullptr;
    const ufbx_matrix geometryToWorld = node != nullptr ? node->geometry_to_world : ufbx_identity_matrix;
    const ufbx_matrix normalMatrix = ufbx_matrix_for_normals(&geometryToWorld);

    MeshData out;
    out.name = node != nullptr && node->name.length > 0U
                   ? std::string(node->name.data, node->name.length)
                   : (mesh->name.length > 0U ? std::string(mesh->name.data, mesh->name.length)
                                             : "Mesh" + std::to_string(meshIndex));
    if (mesh->materials.count > 0U) {
      usize materialIndex = 0U;
      if (mesh->face_material.count > 0U) materialIndex = static_cast<usize>(mesh->face_material.data[0]);
      if (materialIndex >= mesh->materials.count) materialIndex = 0U;
      out.materialName = ufbxString(mesh->materials.data[materialIndex]->name);
    }

    out.positions.reserve(mesh->num_indices);
    out.normals.reserve(mesh->num_indices);
    out.uvs.reserve(mesh->num_indices);
    out.indices.reserve(mesh->num_triangles * 3U);

    const u32 maxTris = static_cast<u32>(mesh->max_face_triangles);
    if (triangleBuffer.size() < static_cast<usize>(maxTris) * 3U) {
      triangleBuffer.resize(static_cast<usize>(maxTris) * 3U);
    }

    for (usize faceIndex = 0; faceIndex < mesh->faces.count; ++faceIndex) {
      const ufbx_face face = mesh->faces.data[faceIndex];
      // tri indices are absolute vertex indices; buffer must hold at least
      // mesh->max_face_triangles * 3 entries.
      const u32 numTris = ufbx_triangulate_face(triangleBuffer.data(), triangleBuffer.size(), mesh, face);
      for (u32 t = 0; t < numTris; ++t) {
        for (u32 corner = 0; corner < 3U; ++corner) {
          const u32 vertexIndex = triangleBuffer[static_cast<usize>(t) * 3U + corner];
          if (vertexIndex >= mesh->num_indices) continue;

          const ufbx_vec3 localPos = ufbx_get_vertex_vec3(&mesh->vertex_position, vertexIndex);
          out.positions.push_back(transformPoint(geometryToWorld, localPos));

          Vec3 normal{0.0, 0.0, 0.0};
          if (mesh->vertex_normal.exists && mesh->vertex_normal.values.count > 0U) {
            const ufbx_vec3 localNormal = ufbx_get_vertex_vec3(&mesh->vertex_normal, vertexIndex);
            normal = transformDirection(normalMatrix, localNormal).normalized();
          }
          out.normals.push_back(normal);

          Vec2 uv{0.0, 0.0};
          if (mesh->vertex_uv.exists && mesh->vertex_uv.values.count > 0U) {
            const ufbx_vec2 localUV = ufbx_get_vertex_vec2(&mesh->vertex_uv, vertexIndex);
            uv = Vec2{static_cast<f64>(localUV.x), 1.0 - static_cast<f64>(localUV.y)};
          }
          out.uvs.push_back(uv);

          out.indices.push_back(static_cast<u32>(out.positions.size() - 1U));
        }
      }
    }
    if (!out.positions.empty()) {
      dedupeVertices(out);  // merge identical position+normal+uv tuples (lossless)
      asset.subMeshes.push_back(std::move(out));
    }
  }
  ufbx_free_scene(scene);
  if (asset.subMeshes.empty()) {
    error = "FBX contains no meshes: " + path;
    return std::nullopt;
  }
  asset.mesh = asset.subMeshes.front();
  return asset;
}

// OBJ text scan for the material library file name.
bool findMtlLib(const std::string& objText, std::string& out) {
  std::istringstream stream(objText);
  std::string rawLine;
  while (std::getline(stream, rawLine)) {
    const std::string line = trimmed(rawLine);
    if (line.empty() || line[0] == '#') continue;
    std::istringstream tokens(line);
    std::string keyword;
    tokens >> keyword;
    if (keyword != "mtllib") continue;
    std::string name;
    tokens >> name;
    if (!name.empty()) {
      out = name;
      return true;
    }
  }
  return false;
}

}  // namespace

std::optional<AssetType> detectType(const std::string& path) {
  const std::string ext = extension(path);
  if (ext == ".obj" || ext == ".fbx") return AssetType::mesh;
  if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") return AssetType::image;
  if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac") return AssetType::audio;
  return std::nullopt;  // .mtl (auxiliary), .blend (export path), unknown
}

std::optional<MeshLoadResult> loadMesh(const std::string& path, std::string& error) {
  const std::string ext = extension(path);
  if (ext == ".blend") {
    error = "'.blend' files cannot be loaded directly. In Blender use File > Export > "
            "Wavefront (.obj) or FBX (.fbx) and load the exported file instead: " +
            path;
    return std::nullopt;
  }
  if (ext == ".obj") {
    auto asset = loadOBJAsset(path, error);
    if (!asset.has_value()) return std::nullopt;
    MeshLoadResult result;
    result.mesh = std::move(asset->mesh);
    result.sourceFormat = "obj";
    return result;
  }
  if (ext == ".fbx") {
    auto asset = loadFBXImpl(path, error);
    if (!asset.has_value()) return std::nullopt;
    MeshLoadResult result;
    result.mesh = std::move(asset->mesh);
    result.sourceFormat = "fbx";
    return result;
  }
  error = "unsupported mesh format (expected .obj or .fbx): " + path;
  return std::nullopt;
}

// --- Skinned FBX (stage 25) ---

namespace {

Transform3D toTransform3D(const ufbx_transform& t) {
  Transform3D out;
  out.position = Vec3{static_cast<f64>(t.translation.x), static_cast<f64>(t.translation.y),
                      static_cast<f64>(t.translation.z)};
  out.rotation.x = static_cast<f64>(t.rotation.x);
  out.rotation.y = static_cast<f64>(t.rotation.y);
  out.rotation.z = static_cast<f64>(t.rotation.z);
  out.rotation.w = static_cast<f64>(t.rotation.w);
  out.scale = Vec3{static_cast<f64>(t.scale.x), static_cast<f64>(t.scale.y), static_cast<f64>(t.scale.z)};
  return out;
}

Mat4 toMat4(const ufbx_matrix& m) {
  Mat4 out;
  // ufbx keeps three basis columns plus a translation column; ours is
  // column-major too, with the last row 0,0,0,1.
  out.at(0, 0) = static_cast<f64>(m.cols[0].x);
  out.at(0, 1) = static_cast<f64>(m.cols[0].y);
  out.at(0, 2) = static_cast<f64>(m.cols[0].z);
  out.at(0, 3) = 0.0;
  out.at(1, 0) = static_cast<f64>(m.cols[1].x);
  out.at(1, 1) = static_cast<f64>(m.cols[1].y);
  out.at(1, 2) = static_cast<f64>(m.cols[1].z);
  out.at(1, 3) = 0.0;
  out.at(2, 0) = static_cast<f64>(m.cols[2].x);
  out.at(2, 1) = static_cast<f64>(m.cols[2].y);
  out.at(2, 2) = static_cast<f64>(m.cols[2].z);
  out.at(2, 3) = 0.0;
  out.at(3, 0) = static_cast<f64>(m.cols[3].x);
  out.at(3, 1) = static_cast<f64>(m.cols[3].y);
  out.at(3, 2) = static_cast<f64>(m.cols[3].z);
  out.at(3, 3) = 1.0;
  return out;
}

// Collects the bone nodes of a skin, parents first. FBX gives us a node
// tree; the engine wants a flat array where a parent always precedes its
// children, so walk each bone up to its root and add ancestors first.
void collectBones(const ufbx_skin_deformer* skin, std::vector<const ufbx_node*>& order,
                  std::map<const ufbx_node*, i32>& indexOf) {
  const auto contains = [&indexOf](const ufbx_node* node) { return indexOf.find(node) != indexOf.end(); };
  std::function<void(const ufbx_node*)> addWithParents = [&](const ufbx_node* node) {
    if (node == nullptr || contains(node)) return;
    // The scene root is not a bone: it is the implicit parent of everything
    // and would just add a nameless identity bone to every skeleton.
    if (node->is_root) return;
    // A bone's parent must exist first, so recurse upward before adding.
    if (node->parent != nullptr && !contains(node->parent)) {
      // Only pull in ancestors that are part of the skeleton chain, not the
      // whole scene root.
      addWithParents(node->parent);
    }
    indexOf[node] = static_cast<i32>(order.size());
    order.push_back(node);
  };
  for (usize i = 0; i < skin->clusters.count; ++i) {
    const ufbx_skin_cluster* cluster = skin->clusters.data[i];
    if (cluster->bone_node != nullptr) addWithParents(cluster->bone_node);
  }
}

}  // namespace

std::optional<SkinnedAsset> loadFBXSkinned(const std::string& path, std::string& error) {
  if (extension(path) != ".fbx") {
    error = "not an FBX file: " + path;
    return std::nullopt;
  }
  ufbx_load_opts opts{};
  opts.target_axes = ufbx_axes_right_handed_y_up;
  opts.target_unit_meters = 1.0;
  opts.generate_missing_normals = true;
  ufbx_error fbxError;
  ufbx_scene* scene = ufbx_load_file(path.c_str(), &opts, &fbxError);
  if (scene == nullptr) {
    error = "cannot load FBX '" + path + "'";
    if (fbxError.description.length > 0U) {
      error += ": " + std::string(fbxError.description.data, fbxError.description.length);
    }
    return std::nullopt;
  }

  // The first mesh that actually carries a skin deformer.
  const ufbx_mesh* mesh = nullptr;
  const ufbx_skin_deformer* skin = nullptr;
  for (usize i = 0; i < scene->meshes.count && skin == nullptr; ++i) {
    const ufbx_mesh* candidate = scene->meshes.data[i];
    if (candidate->skin_deformers.count == 0U) continue;
    mesh = candidate;
    skin = candidate->skin_deformers.data[0];
  }
  if (mesh == nullptr || skin == nullptr) {
    ufbx_free_scene(scene);
    error = "FBX has no skinned mesh: " + path;
    return std::nullopt;
  }

  SkinnedAsset asset;
  std::vector<const ufbx_node*> boneNodes;
  std::map<const ufbx_node*, i32> boneIndex;
  collectBones(skin, boneNodes, boneIndex);

  asset.skinned.skeleton.bones.reserve(boneNodes.size());
  for (const ufbx_node* node : boneNodes) {
    Bone bone;
    bone.name = ufbxString(node->name);
    const auto parentIt = node->parent != nullptr ? boneIndex.find(node->parent) : boneIndex.end();
    bone.parent = parentIt != boneIndex.end() ? parentIt->second : kNoParentBone;
    bone.restPose = toTransform3D(node->local_transform);
    // Default: the bind pose is wherever the bone rests. A cluster below
    // overrides this with the real bind matrix the exporter recorded.
    bone.inverseBindPose = toMat4(node->node_to_world).inverse();
    asset.skinned.skeleton.bones.push_back(std::move(bone));
  }
  // The clusters carry the authoritative bind matrices.
  for (usize i = 0; i < skin->clusters.count; ++i) {
    const ufbx_skin_cluster* cluster = skin->clusters.data[i];
    if (cluster->bone_node == nullptr) continue;
    const auto found = boneIndex.find(cluster->bone_node);
    if (found == boneIndex.end()) continue;
    asset.skinned.skeleton.bones[static_cast<usize>(found->second)].inverseBindPose =
        toMat4(cluster->geometry_to_bone);
  }

  // --- The mesh itself, with one skin entry per emitted vertex ---
  const ufbx_node* meshNode = mesh->instances.count > 0U ? mesh->instances.data[0] : nullptr;
  const ufbx_matrix geometryToWorld = meshNode != nullptr ? meshNode->geometry_to_world : ufbx_identity_matrix;
  const ufbx_matrix normalMatrix = ufbx_matrix_for_normals(&geometryToWorld);

  MeshData& out = asset.skinned.bindMesh;
  out.name = meshNode != nullptr && meshNode->name.length > 0U ? ufbxString(meshNode->name) : ufbxString(mesh->name);

  std::vector<u32> triangleBuffer(static_cast<usize>(mesh->max_face_triangles) * 3U + 3U);
  for (usize faceIndex = 0; faceIndex < mesh->faces.count; ++faceIndex) {
    const ufbx_face face = mesh->faces.data[faceIndex];
    const u32 numTris = ufbx_triangulate_face(triangleBuffer.data(), triangleBuffer.size(), mesh, face);
    for (u32 t = 0; t < numTris; ++t) {
      for (u32 corner = 0; corner < 3U; ++corner) {
        const u32 index = triangleBuffer[static_cast<usize>(t) * 3U + corner];
        if (index >= mesh->num_indices) continue;

        const ufbx_vec3 localPos = ufbx_get_vertex_vec3(&mesh->vertex_position, index);
        out.positions.push_back(transformPoint(geometryToWorld, localPos));

        Vec3 normal{0.0, 0.0, 0.0};
        if (mesh->vertex_normal.exists && mesh->vertex_normal.values.count > 0U) {
          const ufbx_vec3 localNormal = ufbx_get_vertex_vec3(&mesh->vertex_normal, index);
          normal = transformDirection(normalMatrix, localNormal).normalized();
        }
        out.normals.push_back(normal);

        Vec2 uv{0.0, 0.0};
        if (mesh->vertex_uv.exists && mesh->vertex_uv.values.count > 0U) {
          const ufbx_vec2 localUV = ufbx_get_vertex_vec2(&mesh->vertex_uv, index);
          uv = Vec2{static_cast<f64>(localUV.x), 1.0 - static_cast<f64>(localUV.y)};
        }
        out.uvs.push_back(uv);
        out.indices.push_back(static_cast<u32>(out.positions.size() - 1U));

        // The weights belong to the CONTROL POINT, which several emitted
        // vertices can share.
        VertexSkin vertexSkin;
        const u32 controlPoint = mesh->vertex_indices.data[index];
        if (controlPoint < skin->vertices.count) {
          const ufbx_skin_vertex& skinVertex = skin->vertices.data[controlPoint];
          // ufbx sorts weights heaviest first, so the first four are the
          // best four to keep.
          const u32 take = skinVertex.num_weights < kMaxBoneInfluences ? skinVertex.num_weights
                                                                       : kMaxBoneInfluences;
          for (u32 w = 0; w < take; ++w) {
            const ufbx_skin_weight& weight = skin->weights.data[skinVertex.weight_begin + w];
            if (weight.cluster_index >= skin->clusters.count) continue;
            const ufbx_skin_cluster* cluster = skin->clusters.data[weight.cluster_index];
            if (cluster->bone_node == nullptr) continue;
            const auto found = boneIndex.find(cluster->bone_node);
            if (found == boneIndex.end()) continue;
            vertexSkin.bones[w] = static_cast<u32>(found->second);
            vertexSkin.weights[w] = static_cast<f64>(weight.weight);
          }
          // Dropping the light influences leaves the sum short, so rescale.
          vertexSkin.normalize();
        }
        asset.skinned.skins.push_back(vertexSkin);
      }
    }
  }

  // --- Animation: one clip per stack, sampled on a fixed grid ---
  // FBX curves can be any interpolation; sampling them into plain keys keeps
  // the engine simple and the playback identical everywhere.
  constexpr f64 kSampleRate = 30.0;
  for (usize stackIndex = 0; stackIndex < scene->anim_stacks.count; ++stackIndex) {
    const ufbx_anim_stack* stack = scene->anim_stacks.data[stackIndex];
    AnimationClip clip;
    clip.name = ufbxString(stack->name);
    const f64 begin = stack->time_begin;
    const f64 end = stack->time_end;
    clip.duration = end > begin ? end - begin : 0.0;
    const i32 frames = clip.duration > 0.0 ? static_cast<i32>(clip.duration * kSampleRate) + 1 : 1;

    for (usize boneSlot = 0; boneSlot < boneNodes.size(); ++boneSlot) {
      const ufbx_node* node = boneNodes[boneSlot];
      BoneTrack track;
      track.bone = static_cast<i32>(boneSlot);
      track.keys.reserve(static_cast<usize>(frames));
      bool moves = false;
      const Transform3D rest = toTransform3D(node->local_transform);
      for (i32 frame = 0; frame < frames; ++frame) {
        const f64 offset = static_cast<f64>(frame) / kSampleRate;
        const f64 time = begin + (offset > clip.duration ? clip.duration : offset);
        BoneKey key;
        key.time = time - begin;
        key.pose = toTransform3D(ufbx_evaluate_transform(stack->anim, node, time));
        // Only keep a track that actually does something.
        if (!moves) {
          const Vec3 dp = key.pose.position - rest.position;
          const Vec3 ds = key.pose.scale - rest.scale;
          const f64 dr = std::abs(key.pose.rotation.x - rest.rotation.x) +
                         std::abs(key.pose.rotation.y - rest.rotation.y) +
                         std::abs(key.pose.rotation.z - rest.rotation.z) +
                         std::abs(key.pose.rotation.w - rest.rotation.w);
          if (dp.length() > 1e-9 || ds.length() > 1e-9 || dr > 1e-9) moves = true;
        }
        track.keys.push_back(key);
      }
      if (moves) clip.tracks.push_back(std::move(track));
    }
    if (!clip.tracks.empty()) asset.clips.push_back(std::move(clip));
  }

  ufbx_free_scene(scene);
  if (out.positions.empty()) {
    error = "skinned FBX produced no geometry: " + path;
    return std::nullopt;
  }
  return asset;
}

std::optional<MeshAsset> loadMeshAsset(const std::string& path, std::string& error) {
  const std::string ext = extension(path);
  if (ext == ".obj") return loadOBJAsset(path, error);
  if (ext == ".fbx") return loadFBXAsset(path, error);
  error = "unsupported mesh format (expected .obj or .fbx): " + path;
  return std::nullopt;
}

std::optional<std::vector<MeshData>> loadFBXAll(const std::string& path, std::string& error) {
  auto asset = loadFBXImpl(path, error);
  if (!asset.has_value()) return std::nullopt;
  return asset->subMeshes;
}

std::optional<MeshAsset> loadOBJAsset(const std::string& path, std::string& error) {
  MeshAsset asset;
  std::vector<OBJFaceGroup> groups;
  if (!loadFromOBJFile(path, asset.mesh, error, false, &groups)) return std::nullopt;
  if (groups.empty()) return asset;  // no usemtl at all: plain mesh, no materials

  // The mtllib statement lives in the OBJ text (the loader skips it).
  std::string objText;
  if (!readTextFile(path, objText, error)) return std::nullopt;
  std::string mtlName;
  if (findMtlLib(objText, mtlName)) {
    const std::string mtlPath = isAbsolute(mtlName) ? mtlName : joinPath(directoryOf(path), mtlName);
    std::string mtlText;
    std::string mtlError;  // a missing/unreadable MTL is tolerated
    if (readTextFile(mtlPath, mtlText, mtlError) && loadFromMTLText(mtlText, asset.materials, mtlError)) {
      // Resolve texture paths against the MTL file's directory.
      for (MaterialData& material : asset.materials) {
        if (!material.texturePath.empty() && !isAbsolute(material.texturePath)) {
          material.texturePath = normalizePath(joinPath(directoryOf(mtlPath), material.texturePath));
        }
      }
    }
    // MTL missing or broken: the mesh still loads, materials fall back to
    // default-white placeholders (Unity-style tolerance).
  }

  // Every referenced material gets a table entry; unknown names become
  // default-white placeholders (tolerant, like Unity's default material).
  const auto ensureMaterial = [&asset](const std::string& name) -> usize {
    for (usize i = 0; i < asset.materials.size(); ++i) {
      if (asset.materials[i].name == name) return i;
    }
    MaterialData placeholder;
    placeholder.name = name;
    asset.materials.push_back(placeholder);
    return asset.materials.size() - 1U;
  };

  // Split the usemtl runs into sub-meshes (vertex/index ranges are
  // contiguous because the loader appends faces sequentially).
  for (const OBJFaceGroup& group : groups) {
    MeshData sub;
    sub.name = asset.mesh.name;
    sub.materialName = group.material;
    sub.positions.assign(asset.mesh.positions.begin() + static_cast<std::ptrdiff_t>(group.vertexBegin),
                         asset.mesh.positions.begin() + static_cast<std::ptrdiff_t>(group.vertexBegin + group.vertexCount));
    sub.normals.assign(asset.mesh.normals.begin() + static_cast<std::ptrdiff_t>(group.vertexBegin),
                       asset.mesh.normals.begin() + static_cast<std::ptrdiff_t>(group.vertexBegin + group.vertexCount));
    if (!asset.mesh.uvs.empty()) {
      sub.uvs.assign(asset.mesh.uvs.begin() + static_cast<std::ptrdiff_t>(group.vertexBegin),
                     asset.mesh.uvs.begin() + static_cast<std::ptrdiff_t>(group.vertexBegin + group.vertexCount));
    }
    sub.indices.reserve(group.indexCount);
    for (usize i = 0; i < group.indexCount; ++i) {
      sub.indices.push_back(asset.mesh.indices[group.indexBegin + i] - static_cast<u32>(group.vertexBegin));
    }
    dedupeVertices(sub);
    if (!group.material.empty()) {
      const usize materialIndex = ensureMaterial(group.material);
      sub.materialName = asset.materials[materialIndex].name;
    }
    asset.subMeshes.push_back(std::move(sub));
  }
  asset.mesh.materialName = asset.subMeshes.size() == 1U ? asset.subMeshes.front().materialName : "";
  return asset;
}

std::optional<MeshAsset> loadFBXAsset(const std::string& path, std::string& error) {
  return loadFBXImpl(path, error);
}

std::optional<Image> loadImage(const std::string& path, std::string& error) { return Image::load(path, error); }

std::optional<AudioBuffer> loadAudio(const std::string& path, std::string& error) { return AudioBuffer::load(path, error); }

}  // namespace assets
}  // namespace kimia
