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

std::optional<std::vector<MeshData>> loadFBXImpl(const std::string& path, std::string& error) {
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

  std::vector<MeshData> result;
  result.reserve(scene->meshes.count);
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
      result.push_back(std::move(out));
    }
  }
  ufbx_free_scene(scene);
  if (result.empty()) {
    error = "FBX contains no meshes: " + path;
    return std::nullopt;
  }
  return result;
}

}  // namespace

std::optional<AssetType> detectType(const std::string& path) {
  const std::string ext = extension(path);
  if (ext == ".obj" || ext == ".fbx") return AssetType::mesh;
  if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") return AssetType::image;
  if (ext == ".wav" || ext == ".mp3") return AssetType::audio;
  return std::nullopt;
}

std::optional<MeshLoadResult> loadMesh(const std::string& path, std::string& error) {
  const std::string ext = extension(path);
  if (ext == ".obj") {
    MeshData mesh;
    if (!loadFromOBJFile(path, mesh, error)) return std::nullopt;
    MeshLoadResult result;
    result.mesh = std::move(mesh);
    result.sourceFormat = "obj";
    return result;
  }
  if (ext == ".fbx") {
    auto meshes = loadFBXImpl(path, error);
    if (!meshes.has_value()) return std::nullopt;
    MeshLoadResult result;
    result.mesh = std::move(meshes->front());
    result.sourceFormat = "fbx";
    return result;
  }
  error = "unsupported mesh format (expected .obj or .fbx): " + path;
  return std::nullopt;
}

std::optional<std::vector<MeshData>> loadFBXAll(const std::string& path, std::string& error) {
  return loadFBXImpl(path, error);
}

std::optional<Image> loadImage(const std::string& path, std::string& error) { return Image::load(path, error); }

std::optional<AudioBuffer> loadAudio(const std::string& path, std::string& error) { return AudioBuffer::load(path, error); }

}  // namespace assets
}  // namespace kimia
