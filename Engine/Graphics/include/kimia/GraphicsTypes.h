#pragma once

#include <kimia/Types.h>
#include <kimia/Vec.h>

#include <string>
#include <vector>

namespace kimia {

// CPU-side triangle mesh. Layout contract:
// - positions / normals / uvs have the same size (one entry per vertex)
// - indices are triangle indices (size is a multiple of 3)
// - triangles are counter-clockwise when viewed from outside
struct MeshData {
  std::string name;
  std::vector<Vec3> positions;
  std::vector<Vec3> normals;
  std::vector<Vec2> uvs;
  std::vector<u32> indices;

  void clear() {
    name.clear();
    positions.clear();
    normals.clear();
    uvs.clear();
    indices.clear();
  }

  u64 vertexCount() const { return positions.size(); }
  u64 triangleCount() const { return indices.size() / 3U; }

  bool isValid() const {
    return !positions.empty() && positions.size() == normals.size() &&
           (uvs.empty() || uvs.size() == positions.size()) && indices.size() % 3U == 0U;
  }
};

}  // namespace kimia
