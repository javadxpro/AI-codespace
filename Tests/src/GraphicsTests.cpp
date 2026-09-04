#include <kimia/GraphicsTypes.h>
#include <kimia/Mesh.h>
#include <kimia/Vec.h>
#include <kimia_test.h>

#include <cmath>

namespace {
using kimia::Vec2;
using kimia::Vec3;
using kimia::f64;
using kimia::i32;
using kimia::usize;

constexpr f64 kEps = 1e-9;

bool near3(const Vec3& a, const Vec3& b, f64 eps = kEps) {
  return std::abs(a.x - b.x) <= eps && std::abs(a.y - b.y) <= eps && std::abs(a.z - b.z) <= eps;
}

bool isUnit(const Vec3& v, f64 eps = 1e-6) { return std::abs(v.length() - 1.0) <= eps; }

// Every triangle must be counter-clockwise seen from outside: the face normal
// must point the same way as its first vertex normal. Zero-area triangles
// (pole rings of the sphere grid) are skipped.
void requireOutwardWinding(const kimia::MeshData& mesh) {
  for (usize i = 0; i + 2U < mesh.indices.size(); i += 3U) {
    const Vec3 p0 = mesh.positions[mesh.indices[i]];
    const Vec3 p1 = mesh.positions[mesh.indices[i + 1U]];
    const Vec3 p2 = mesh.positions[mesh.indices[i + 2U]];
    const Vec3 faceNormal = kimia::cross(p1 - p0, p2 - p0);
    if (faceNormal.lengthSquared() < 1e-24) continue;
    const Vec3 vertexNormal = mesh.normals[mesh.indices[i]];
    KIMIA_REQUIRE(kimia::dot(faceNormal, vertexNormal) > 0.0);
  }
}
}  // namespace

KIMIA_TEST(cube_24v_36i_with_per_face_normals) {
  const kimia::MeshData cube = kimia::makeCube(2.0);
  KIMIA_REQUIRE(cube.positions.size() == 24U);
  KIMIA_REQUIRE(cube.indices.size() == 36U);
  KIMIA_REQUIRE(cube.normals.size() == 24U);
  KIMIA_REQUIRE(cube.uvs.size() == 24U);
  KIMIA_REQUIRE(cube.isValid());
  // Corner positions lie exactly on the cube surface (half extent = 1).
  for (const Vec3& p : cube.positions) {
    KIMIA_REQUIRE(std::abs(std::abs(p.x) - 1.0) <= kEps || std::abs(p.x) <= kEps);
    KIMIA_REQUIRE(std::abs(std::abs(p.y) - 1.0) <= kEps || std::abs(p.y) <= kEps);
    KIMIA_REQUIRE(std::abs(std::abs(p.z) - 1.0) <= kEps || std::abs(p.z) <= kEps);
  }
  // Normals are unit axis vectors and equal the outward direction of the face.
  for (const Vec3& n : cube.normals) KIMIA_REQUIRE(isUnit(n));
  i32 frontFaceCount = 0;
  for (usize i = 0; i < cube.normals.size(); ++i) {
    const Vec3& n = cube.normals[i];
    KIMIA_REQUIRE(kimia::dot(n, cube.positions[i]) > 0.0);
    if (near3(n, Vec3{0.0, 0.0, -1.0})) ++frontFaceCount;
  }
  KIMIA_REQUIRE(frontFaceCount == 4);
  requireOutwardWinding(cube);
}

KIMIA_TEST(cube_uvs_cover_01) {
  const kimia::MeshData cube = kimia::makeCube(1.0);
  for (const Vec2& uv : cube.uvs) {
    KIMIA_REQUIRE(uv.x >= -kEps && uv.x <= 1.0 + kEps);
    KIMIA_REQUIRE(uv.y >= -kEps && uv.y <= 1.0 + kEps);
  }
}

KIMIA_TEST(sphere_reference_153v_768i_outward) {
  // The reference sphere (16 rings x 8 segments): 153 vertices / 768 indices.
  const kimia::MeshData sphere = kimia::makeSphere(16, 8);
  KIMIA_REQUIRE(sphere.positions.size() == 153U);
  KIMIA_REQUIRE(sphere.indices.size() == 768U);
  KIMIA_REQUIRE(sphere.normals.size() == 153U);
  KIMIA_REQUIRE(sphere.isValid());
  for (usize i = 0; i < sphere.positions.size(); ++i) {
    KIMIA_REQUIRE(isUnit(sphere.normals[i], 1e-5));
    // Outward: normal points away from the origin (dot > 0 everywhere).
    KIMIA_REQUIRE(kimia::dot(sphere.normals[i], sphere.positions[i]) > 0.0);
  }
  requireOutwardWinding(sphere);
}

KIMIA_TEST(sphere_vertex_count_formula) {
  // verts = (rings+1)*(segments+1), indices = rings*segments*6.
  const kimia::MeshData sphere = kimia::makeSphere(4, 6);
  KIMIA_REQUIRE(sphere.positions.size() == 35U);
  KIMIA_REQUIRE(sphere.indices.size() == 144U);
  requireOutwardWinding(sphere);
}

KIMIA_TEST(plane_4v_6i_faces_up) {
  const kimia::MeshData plane = kimia::makePlane(4.0, 2.0);
  KIMIA_REQUIRE(plane.positions.size() == 4U);
  KIMIA_REQUIRE(plane.indices.size() == 6U);
  KIMIA_REQUIRE(plane.isValid());
  for (const Vec3& p : plane.positions) KIMIA_REQUIRE(std::abs(p.y) <= kEps);
  for (const Vec3& n : plane.normals) KIMIA_REQUIRE(near3(n, Vec3{0.0, 1.0, 0.0}));
  requireOutwardWinding(plane);
}

KIMIA_TEST(mesh_data_validation_contract) {
  kimia::MeshData empty;
  KIMIA_REQUIRE(!empty.isValid());
  kimia::MeshData cube = kimia::makeCube(1.0);
  KIMIA_REQUIRE(cube.vertexCount() == 24U);
  KIMIA_REQUIRE(cube.triangleCount() == 12U);
  cube.indices.pop_back();
  KIMIA_REQUIRE(!cube.isValid());
}
