#include <kimia/Camera.h>
#include <kimia/Mat4.h>
#include <kimia/MathUtils.h>
#include <kimia/Quat.h>
#include <kimia/Vec.h>
#include <kimia_test.h>

#include <cmath>

namespace {
using kimia::f64;
using kimia::Vec2;
using kimia::Vec3;
using kimia::Vec4;

constexpr f64 kEps = 1e-9;
constexpr f64 kEps4 = 1e-4;

bool near(f64 a, f64 b, f64 eps = kEps) { return std::abs(a - b) <= eps; }
bool near3(const Vec3& a, const Vec3& b, f64 eps = kEps) {
  return std::abs(a.x - b.x) <= eps && std::abs(a.y - b.y) <= eps && std::abs(a.z - b.z) <= eps;
}
bool near4(const Vec4& a, const Vec4& b, f64 eps = kEps) {
  return std::abs(a.x - b.x) <= eps && std::abs(a.y - b.y) <= eps && std::abs(a.z - b.z) <= eps &&
         std::abs(a.w - b.w) <= eps;
}
bool nearMat(const kimia::Mat4& a, const kimia::Mat4& b, f64 eps = kEps) {
  for (kimia::i32 i = 0; i < 16; ++i) {
    if (std::abs(a.m_[static_cast<kimia::usize>(i)] - b.m_[static_cast<kimia::usize>(i)]) > eps) return false;
  }
  return true;
}
}  // namespace

KIMIA_TEST(vec2_add_sub_scale) {
  const Vec2 a{1.0, 2.0};
  const Vec2 b{3.0, 4.0};
  KIMIA_REQUIRE(near3(Vec3{a.x, a.y, 0.0}, Vec3{1.0, 2.0, 0.0}));
  const Vec2 sum = a + b;
  KIMIA_REQUIRE(near(sum.x, 4.0) && near(sum.y, 6.0));
  const Vec2 diff = a - b;
  KIMIA_REQUIRE(near(diff.x, -2.0) && near(diff.y, -2.0));
  const Vec2 scaled = a * 2.0;
  KIMIA_REQUIRE(near(scaled.x, 2.0) && near(scaled.y, 4.0));
}

KIMIA_TEST(vec2_dot_length_normalize) {
  const Vec2 a{3.0, 4.0};
  KIMIA_REQUIRE(near(a.lengthSquared(), 25.0));
  KIMIA_REQUIRE(near(a.length(), 5.0));
  const Vec2 n = a.normalized();
  KIMIA_REQUIRE(near(n.x, 0.6, kEps4) && near(n.y, 0.8, kEps4));
  KIMIA_REQUIRE(near(n.length(), 1.0, kEps4));
  KIMIA_REQUIRE(near(kimia::dot(a, n), 5.0, kEps4));
}

KIMIA_TEST(vec3_cross_right_handed) {
  const Vec3 x{1.0, 0.0, 0.0};
  const Vec3 y{0.0, 1.0, 0.0};
  KIMIA_REQUIRE(near3(kimia::cross(x, y), Vec3{0.0, 0.0, 1.0}));
  KIMIA_REQUIRE(near3(kimia::cross(y, x), Vec3{0.0, 0.0, -1.0}));
  KIMIA_REQUIRE(near(kimia::dot(kimia::cross(x, y), x), 0.0));
}

KIMIA_TEST(vec3_length_normalize) {
  const Vec3 v{3.0, 0.0, 4.0};
  KIMIA_REQUIRE(near(v.length(), 5.0));
  const Vec3 n = v.normalized();
  KIMIA_REQUIRE(near(n.x, 0.6, kEps4) && near(n.y, 0.0) && near(n.z, 0.8, kEps4));
  KIMIA_REQUIRE(near(n.length(), 1.0, kEps4));
}

KIMIA_TEST(vec4_ops) {
  const Vec4 a{1.0, 2.0, 3.0, 4.0};
  const Vec4 b{5.0, 6.0, 7.0, 8.0};
  const Vec4 sum = a + b;
  KIMIA_REQUIRE(near(sum.w, 12.0));
  KIMIA_REQUIRE(near(kimia::dot(a, b), 70.0));
  KIMIA_REQUIRE(near3(a.xyz(), Vec3{1.0, 2.0, 3.0}));
}

KIMIA_TEST(mat4_identity_multiply) {
  const kimia::Mat4 identity;
  const Vec4 v{1.0, 2.0, 3.0, 4.0};
  KIMIA_REQUIRE(near4(identity * v, v));
  KIMIA_REQUIRE(near3(identity * v.xyz(), Vec3{1.0, 2.0, 3.0}));
  KIMIA_REQUIRE(nearMat(identity * identity, identity));
}

KIMIA_TEST(mat4_translation_affine) {
  const kimia::Mat4 t = kimia::Mat4::translation(Vec3{2.0, 3.0, 4.0});
  KIMIA_REQUIRE(near3(t * Vec3{1.0, 1.0, 1.0}, Vec3{3.0, 4.0, 5.0}));
  // Directions ignore translation.
  KIMIA_REQUIRE(near3(t.transformDirection(Vec3{1.0, 2.0, 3.0}), Vec3{1.0, 2.0, 3.0}));
}

KIMIA_TEST(mat4_perspective_ndc_values) {
  // 90-degree fov, square aspect, near 0.1, far 100.
  const kimia::Mat4 p = kimia::Mat4::perspective(kimia::kHalfPi, 1.0, 0.1, 100.0);
  // Point at z = -10 (4 decimals, exact to the projection math).
  const Vec4 clip10 = p * Vec4{0.0, 0.0, -10.0, 1.0};
  KIMIA_REQUIRE(near(clip10.w, 10.0, kEps4));
  KIMIA_REQUIRE(near(clip10.z / clip10.w, 0.981982, kEps4));
  // Point on the near plane maps to NDC z = -1.
  const Vec4 clipNear = p * Vec4{0.0, 0.0, -0.1, 1.0};
  KIMIA_REQUIRE(near(clipNear.z / clipNear.w, -1.0, kEps4));
  // Right edge of the frustum at z = -10: tan(45deg) = 1 -> x_ndc = 1.
  const Vec4 clipEdge = p * Vec4{10.0, 0.0, -10.0, 1.0};
  KIMIA_REQUIRE(near(clipEdge.x / clipEdge.w, 1.0, kEps4));
}

KIMIA_TEST(mat4_lookat_values) {
  // Looking straight down -Z from the origin is the identity view.
  const kimia::Mat4 view = kimia::Mat4::lookAt(Vec3{0.0, 0.0, 0.0}, Vec3{0.0, 0.0, -1.0}, Vec3{0.0, 1.0, 0.0});
  KIMIA_REQUIRE(near3(view * Vec3{1.0, 2.0, 3.0}, Vec3{1.0, 2.0, 3.0}));
  // Camera at (0, 2, 10) looking at (0, 2, 0): the target lands at view z = -10.
  const kimia::Mat4 view2 = kimia::Mat4::lookAt(Vec3{0.0, 2.0, 10.0}, Vec3{0.0, 2.0, 0.0}, Vec3{0.0, 1.0, 0.0});
  KIMIA_REQUIRE(near3(view2 * Vec3{0.0, 2.0, 0.0}, Vec3{0.0, 0.0, -10.0}, kEps4));
  // The world +X axis maps to view +X (right).
  KIMIA_REQUIRE(near3(view2 * Vec3{1.0, 2.0, 0.0}, Vec3{1.0, 0.0, -10.0}, kEps4));
}

KIMIA_TEST(mat4_rotation_directions) {
  // Right-hand rule about each axis.
  KIMIA_REQUIRE(near3(kimia::Mat4::rotationX(kimia::kHalfPi) * Vec3{0.0, 1.0, 0.0}, Vec3{0.0, 0.0, 1.0}, kEps4));
  KIMIA_REQUIRE(near3(kimia::Mat4::rotationY(kimia::kHalfPi) * Vec3{1.0, 0.0, 0.0}, Vec3{0.0, 0.0, -1.0}, kEps4));
  KIMIA_REQUIRE(near3(kimia::Mat4::rotationZ(kimia::kHalfPi) * Vec3{1.0, 0.0, 0.0}, Vec3{0.0, 1.0, 0.0}, kEps4));
}

KIMIA_TEST(mat4_inverse_of_translation) {
  const kimia::Mat4 t = kimia::Mat4::translation(Vec3{2.0, 3.0, 4.0});
  const kimia::Mat4 inv = t.inverse();
  KIMIA_REQUIRE(near3(inv * Vec3{2.0, 3.0, 4.0}, Vec3{0.0, 0.0, 0.0}));
  KIMIA_REQUIRE(nearMat(t * inv, kimia::Mat4{}));
}

KIMIA_TEST(mat4_inverse_roundtrip_composite) {
  const kimia::Mat4 m =
      kimia::Mat4::translation(Vec3{2.0, 3.0, 4.0}) * kimia::Mat4::rotationY(0.7) * kimia::Mat4::scaling(Vec3{2.0, 0.5, 3.0});
  const kimia::Mat4 inv = m.inverse();
  KIMIA_REQUIRE(nearMat(m * inv, kimia::Mat4{}, 1e-9));
  KIMIA_REQUIRE(nearMat(inv * m, kimia::Mat4{}, 1e-9));
}

KIMIA_TEST(mat4_normal_matrix_inverse_transpose) {
  // Non-uniform scale: the normal matrix must be the inverse-transpose.
  const kimia::Mat4 m = kimia::Mat4::scaling(Vec3{2.0, 1.0, 0.5});
  const kimia::Mat4 normalMat = m.inverseTranspose();
  const Vec3 nx = normalMat.transformDirection(Vec3{1.0, 0.0, 0.0});
  KIMIA_REQUIRE(near(nx.x, 0.5, kEps4) && near(nx.y, 0.0) && near(nx.z, 0.0));
  const Vec3 ny = normalMat.transformDirection(Vec3{0.0, 1.0, 0.0});
  KIMIA_REQUIRE(near3(ny, Vec3{0.0, 1.0, 0.0}));
  const Vec3 nz = normalMat.transformDirection(Vec3{0.0, 0.0, 1.0});
  KIMIA_REQUIRE(near(nz.z, 2.0, kEps4));
}

KIMIA_TEST(mat4_multiply_is_associative) {
  const kimia::Mat4 a = kimia::Mat4::rotationY(0.3);
  const kimia::Mat4 b = kimia::Mat4::translation(Vec3{1.0, 2.0, 3.0});
  const Vec4 v{1.0, 1.0, 1.0, 1.0};
  KIMIA_REQUIRE(near4((a * b) * v, a * (b * v), kEps4));
}

KIMIA_TEST(quat_identity_rotates_nothing) {
  const kimia::Quat q;
  KIMIA_REQUIRE(near3(q.rotate(Vec3{1.0, 2.0, 3.0}), Vec3{1.0, 2.0, 3.0}));
}

KIMIA_TEST(quat_axis_angle_rotates_x_to_y) {
  // 90 degrees about +Z: +X rotates to +Y (right-hand rule).
  const kimia::Quat q = kimia::Quat::fromAxisAngle(Vec3{0.0, 0.0, 1.0}, kimia::kHalfPi);
  KIMIA_REQUIRE(near3(q.rotate(Vec3{1.0, 0.0, 0.0}), Vec3{0.0, 1.0, 0.0}, kEps4));
  KIMIA_REQUIRE(near3(q.rotate(Vec3{0.0, 1.0, 0.0}), Vec3{-1.0, 0.0, 0.0}, kEps4));
}

KIMIA_TEST(quat_yaw_90_matches_rh_convention) {
  // 90 degrees about +Y: +X rotates to -Z (right-handed, -Z forward).
  const kimia::Quat q = kimia::Quat::fromAxisAngle(Vec3{0.0, 1.0, 0.0}, kimia::kHalfPi);
  KIMIA_REQUIRE(near3(q.rotate(Vec3{1.0, 0.0, 0.0}), Vec3{0.0, 0.0, -1.0}, kEps4));
  const kimia::Mat4 m = q.toMat4();
  KIMIA_REQUIRE(near3(m * Vec3{1.0, 0.0, 0.0}, Vec3{0.0, 0.0, -1.0}, kEps4));
}

KIMIA_TEST(quat_multiply_combines_rotations) {
  const kimia::Quat qz = kimia::Quat::fromAxisAngle(Vec3{0.0, 0.0, 1.0}, kimia::kHalfPi);
  const kimia::Quat qy = kimia::Quat::fromAxisAngle(Vec3{0.0, 1.0, 0.0}, kimia::kHalfPi);
  // qy * qz means "apply qz first, then qy".
  KIMIA_REQUIRE(near3((qy * qz).rotate(Vec3{1.0, 0.0, 0.0}), Vec3{0.0, 1.0, 0.0}, kEps4));
  KIMIA_REQUIRE(near3((qz * qy).rotate(Vec3{1.0, 0.0, 0.0}), Vec3{0.0, 0.0, -1.0}, kEps4));
}

KIMIA_TEST(quat_mat4_round_trip) {
  const kimia::Quat q = kimia::Quat::fromAxisAngle(Vec3{1.0, 2.0, 3.0}, 1.23);
  const kimia::Mat4 m = q.toMat4();
  const kimia::Quat q2 = kimia::Quat::fromMat4(m);
  const kimia::Mat4 m2 = q2.toMat4();
  KIMIA_REQUIRE(nearMat(m, m2, kEps4));
  // And the rotation actually matches on real vectors.
  KIMIA_REQUIRE(near3(q.rotate(Vec3{0.5, -0.25, 1.0}), m2 * Vec3{0.5, -0.25, 1.0}, kEps4));
}

KIMIA_TEST(camera_view_projection_chain) {
  kimia::Camera camera;
  camera.fovYRadians = kimia::kHalfPi;
  camera.aspect = 1.0;
  camera.nearPlane = 0.1;
  camera.farPlane = 100.0;
  camera.position = Vec3{0.0, 2.0, 10.0};
  camera.target = Vec3{0.0, 2.0, 0.0};
  // View matrix puts the target at view z = -10.
  KIMIA_REQUIRE(near3(camera.viewMatrix() * Vec3{0.0, 2.0, 0.0}, Vec3{0.0, 0.0, -10.0}, kEps4));
  // viewProjection = projection * view.
  const Vec4 clip = camera.viewProjectionMatrix() * Vec4{0.0, 2.0, 0.0, 1.0};
  KIMIA_REQUIRE(near(clip.w, 10.0, kEps4));
  KIMIA_REQUIRE(near(clip.z / clip.w, 0.981982, kEps4));
  const Vec4 viaTwoSteps = camera.projectionMatrix() * (camera.viewMatrix() * Vec4{0.0, 2.0, 0.0, 1.0});
  KIMIA_REQUIRE(near4(clip, viaTwoSteps, kEps4));
}

KIMIA_TEST(mathutils_lerp_clamp_angles) {
  KIMIA_REQUIRE(near(kimia::lerp(0.0, 10.0, 0.25), 2.5));
  KIMIA_REQUIRE(near(kimia::clamp(5.0, 0.0, 10.0), 5.0));
  KIMIA_REQUIRE(near(kimia::clamp(-1.0, 0.0, 10.0), 0.0));
  KIMIA_REQUIRE(near(kimia::clamp(11.0, 0.0, 10.0), 10.0));
  KIMIA_REQUIRE(near(kimia::radians(180.0), kimia::kPi, kEps4));
  KIMIA_REQUIRE(near(kimia::degrees(kimia::kPi), 180.0, kEps4));
}
