#include <kimia/Scene.h>
#include <kimia/SceneIO.h>
#include <kimia/Vec.h>
#include <kimia_test.h>

#include <cmath>
#include <string>
#include <sys/stat.h>

#ifndef KIMIA_ASSET_DIR
#error "KIMIA_ASSET_DIR must be defined by CMake"
#endif
#ifndef KIMIA_TEST_TMP
#error "KIMIA_TEST_TMP must be defined by CMake"
#endif

namespace {
using kimia::DemoShot;
using kimia::EntityData;
using kimia::EntityHandle;
using kimia::MeshKind;
using kimia::Scene;
using kimia::Vec3;
using kimia::f64;
using kimia::usize;

constexpr f64 kEps = 1e-9;

bool near(f64 a, f64 b) { return std::abs(a - b) <= kEps; }
bool near3(const Vec3& a, const Vec3& b) {
  return std::abs(a.x - b.x) <= kEps && std::abs(a.y - b.y) <= kEps && std::abs(a.z - b.z) <= kEps;
}

std::string tmpPath(const std::string& name) {
  const int created = ::mkdir(KIMIA_TEST_TMP, 0755);
  static_cast<void>(created == 0 || errno == EEXIST);
  return std::string(KIMIA_TEST_TMP) + "/" + name;
}

EntityData makeBall() {
  EntityData ball;
  ball.name = "Ball";
  ball.mesh = MeshKind::sphere;
  ball.color = Vec3{0.95, 0.95, 0.92};
  ball.roughness = 0.3;
  return ball;
}
}  // namespace

KIMIA_TEST(scene_handles_are_1_based_null_is_zero) {
  Scene scene;
  const EntityHandle a = scene.create("A");
  const EntityHandle b = scene.create("B");
  KIMIA_REQUIRE(a == 1U);
  KIMIA_REQUIRE(b == 2U);
  KIMIA_REQUIRE(kimia::kNullEntity == 0U);
  KIMIA_REQUIRE(scene.get(kimia::kNullEntity) == nullptr);
  KIMIA_REQUIRE(!scene.alive(kimia::kNullEntity));
}

KIMIA_TEST(scene_create_get_destroy) {
  Scene scene;
  KIMIA_REQUIRE(scene.count() == 0U);
  const EntityHandle handle = scene.create(makeBall());
  KIMIA_REQUIRE(scene.count() == 1U);
  const EntityData* ball = scene.get(handle);
  KIMIA_REQUIRE(ball != nullptr);
  KIMIA_REQUIRE(ball->name == "Ball");
  KIMIA_REQUIRE(ball->mesh == MeshKind::sphere);
  KIMIA_REQUIRE(near(ball->roughness, 0.3));
  KIMIA_REQUIRE(near3(ball->color, Vec3{0.95, 0.95, 0.92}));
  KIMIA_REQUIRE(scene.destroy(handle));
  KIMIA_REQUIRE(scene.count() == 0U);
  KIMIA_REQUIRE(scene.get(handle) == nullptr);
  KIMIA_REQUIRE(!scene.alive(handle));
  KIMIA_REQUIRE(!scene.destroy(handle));  // double destroy: false, no-op
}

KIMIA_TEST(scene_handles_never_reused) {
  Scene scene;
  const EntityHandle first = scene.create();
  KIMIA_REQUIRE(scene.destroy(first));
  const EntityHandle second = scene.create();
  KIMIA_REQUIRE(second == first + 1U);
  KIMIA_REQUIRE(scene.get(first) == nullptr);
  KIMIA_REQUIRE(scene.get(second) != nullptr);
}

KIMIA_TEST(scene_foreach_sees_all_and_in_handle_order) {
  Scene scene;
  scene.create("A");
  scene.create("B");
  scene.create("C");
  usize visited = 0;
  EntityHandle previous = 0U;
  bool ordered = true;
  scene.forEach([&](EntityHandle handle, const EntityData&) {
    if (handle <= previous) ordered = false;
    previous = handle;
    ++visited;
  });
  KIMIA_REQUIRE(visited == 3U);
  KIMIA_REQUIRE(ordered);
}

KIMIA_TEST(scene_default_entity_values) {
  Scene scene;
  const EntityHandle handle = scene.create("Default");
  const EntityData* entity = scene.get(handle);
  KIMIA_REQUIRE(entity != nullptr);
  KIMIA_REQUIRE(near3(entity->transform.position, Vec3{0.0, 0.0, 0.0}));
  KIMIA_REQUIRE(near3(entity->transform.scale, Vec3{1.0, 1.0, 1.0}));
  KIMIA_REQUIRE(entity->mesh == MeshKind::cube);
  KIMIA_REQUIRE(near(entity->roughness, 0.5));
  KIMIA_REQUIRE(near3(entity->color, Vec3{1.0, 1.0, 1.0}));
}

KIMIA_TEST(sceneio_roundtrip_byte_identical) {
  Scene scene;
  EntityData green;
  green.name = "Green";
  green.mesh = MeshKind::plane;
  green.color = Vec3{0.22, 0.45, 0.24};
  green.roughness = 0.95;
  scene.create(green);
  EntityData wall;
  wall.name = "Wall \"north\" \\ 1";  // escapes must round-trip
  wall.mesh = MeshKind::cube;
  wall.transform.position = Vec3{2.4, 0.5, -0.001};
  wall.transform.scale = Vec3{0.5, 1.0, 4.4};
  wall.color = Vec3{0.7, 0.68, 0.62};
  wall.roughness = 0.5;
  scene.create(wall);
  scene.create(makeBall());
  scene.demoShot = DemoShot{0.25, 0.75};

  std::string first;
  KIMIA_REQUIRE(kimia::SceneIO::save(scene, first));
  KIMIA_REQUIRE(first.find("# KIMIA scene v1") == 0U);

  Scene loaded;
  std::string error;
  KIMIA_REQUIRE(kimia::SceneIO::load(first, loaded, error));
  KIMIA_REQUIRE(loaded.count() == 3U);

  const EntityData* wall2 = loaded.get(2U);
  KIMIA_REQUIRE(wall2 != nullptr);
  KIMIA_REQUIRE(wall2->name == "Wall \"north\" \\ 1");
  KIMIA_REQUIRE(wall2->mesh == MeshKind::cube);
  KIMIA_REQUIRE(near3(wall2->transform.position, Vec3{2.4, 0.5, -0.001}));
  KIMIA_REQUIRE(near3(wall2->transform.scale, Vec3{0.5, 1.0, 4.4}));
  KIMIA_REQUIRE(near(wall2->roughness, 0.5));
  KIMIA_REQUIRE(near3(wall2->color, Vec3{0.7, 0.68, 0.62}));
  KIMIA_REQUIRE(loaded.demoShot.has_value());
  KIMIA_REQUIRE(near(loaded.demoShot->aim, 0.25) && near(loaded.demoShot->power, 0.75));

  // Save -> load -> save must be byte-identical.
  std::string second;
  KIMIA_REQUIRE(kimia::SceneIO::save(loaded, second));
  KIMIA_REQUIRE(first == second);
}

KIMIA_TEST(sceneio_loads_handoff_example_file) {
  Scene scene;
  std::string error;
  KIMIA_REQUIRE(
      kimia::SceneIO::loadFromFile(std::string(KIMIA_ASSET_DIR) + "/handoff_scene.kimia", scene, error));
  KIMIA_REQUIRE(scene.count() == 7U);

  const EntityData* green = scene.get(1U);
  KIMIA_REQUIRE(green != nullptr);
  KIMIA_REQUIRE(green->name == "Green" && green->mesh == MeshKind::plane);
  KIMIA_REQUIRE(near3(green->color, Vec3{0.22, 0.45, 0.24}) && near(green->roughness, 0.95));

  const EntityData* pole = scene.get(5U);
  KIMIA_REQUIRE(pole != nullptr);
  KIMIA_REQUIRE(pole->name == "FlagPole" && pole->mesh == MeshKind::cube);
  KIMIA_REQUIRE(near3(pole->transform.position, Vec3{0.0, 0.9, -7.0}));
  KIMIA_REQUIRE(near3(pole->transform.scale, Vec3{0.03, 1.8, 0.03}));

  const EntityData* ball = scene.get(7U);
  KIMIA_REQUIRE(ball != nullptr);
  KIMIA_REQUIRE(ball->name == "Ball" && ball->mesh == MeshKind::sphere);
  KIMIA_REQUIRE(near3(ball->color, Vec3{0.95, 0.95, 0.92}) && near(ball->roughness, 0.3));

  KIMIA_REQUIRE(scene.demoShot.has_value());
  KIMIA_REQUIRE(near(scene.demoShot->aim, 0.0));
  KIMIA_REQUIRE(near(scene.demoShot->power, 0.61));
}

KIMIA_TEST(sceneio_tolerant_load) {
  const char* messy =
      "# KIMIA scene v1\n"
      "garbage line that is not an entity\n"
      "unknown_keyword 1 2 3\n"
      "e \"Broken\" mesh dodecahedron pos 0 0 0\n"   // unknown mesh kind -> ignored
      "e \"Partial\" mesh cube pos 1 2\n"            // partial line -> ignored
      "e \"Good\" mesh plane pos 1 2 3 scale 2 2 2 whatever 42 rough 0.1\n"
      "# demo 1.5\n"                                 // partial demo -> ignored
      "# demo 0.25 0.75\n"
      "e\n";                                          // no name -> ignored
  Scene scene;
  std::string error;
  KIMIA_REQUIRE(kimia::SceneIO::load(messy, scene, error));
  KIMIA_REQUIRE(scene.count() == 1U);
  const EntityData* good = scene.get(1U);
  KIMIA_REQUIRE(good != nullptr);
  KIMIA_REQUIRE(good->name == "Good" && good->mesh == MeshKind::plane);
  KIMIA_REQUIRE(near3(good->transform.position, Vec3{1.0, 2.0, 3.0}));
  KIMIA_REQUIRE(near3(good->transform.scale, Vec3{2.0, 2.0, 2.0}));
  KIMIA_REQUIRE(near(good->roughness, 0.1));
  KIMIA_REQUIRE(scene.demoShot.has_value());
  KIMIA_REQUIRE(near(scene.demoShot->aim, 0.25) && near(scene.demoShot->power, 0.75));
}

KIMIA_TEST(sceneio_names_with_spaces_roundtrip) {
  Scene scene;
  EntityData entity;
  entity.name = "Green Grass Field";
  entity.mesh = MeshKind::plane;
  scene.create(entity);
  std::string text;
  KIMIA_REQUIRE(kimia::SceneIO::save(scene, text));
  Scene loaded;
  std::string error;
  KIMIA_REQUIRE(kimia::SceneIO::load(text, loaded, error));
  const EntityData* read = loaded.get(1U);
  KIMIA_REQUIRE(read != nullptr);
  KIMIA_REQUIRE(read->name == "Green Grass Field");
  std::string again;
  KIMIA_REQUIRE(kimia::SceneIO::save(loaded, again));
  KIMIA_REQUIRE(text == again);
}

KIMIA_TEST(sceneio_empty_and_missing_inputs) {
  Scene scene;
  std::string error;
  KIMIA_REQUIRE(kimia::SceneIO::load("", scene, error));
  KIMIA_REQUIRE(scene.count() == 0U);
  KIMIA_REQUIRE(!scene.demoShot.has_value());
  KIMIA_REQUIRE(!kimia::SceneIO::loadFromFile("/nonexistent/scene.kimia", scene, error));
  KIMIA_REQUIRE(!error.empty());
}

KIMIA_TEST(sceneio_file_roundtrip) {
  Scene scene;
  EntityData hole;
  hole.name = "Hole";
  hole.mesh = MeshKind::cube;
  hole.transform.position = Vec3{0.0, 0.01, -7.0};
  hole.transform.scale = Vec3{0.56, 0.02, 0.56};
  hole.color = Vec3{0.05, 0.05, 0.05};
  hole.roughness = 0.8;
  scene.create(hole);
  const std::string path = tmpPath("scene_roundtrip.kimia");
  KIMIA_REQUIRE(kimia::SceneIO::saveToFile(scene, path));
  Scene loaded;
  std::string error;
  KIMIA_REQUIRE(kimia::SceneIO::loadFromFile(path, loaded, error));
  KIMIA_REQUIRE(loaded.count() == 1U);
  const EntityData* read = loaded.get(1U);
  KIMIA_REQUIRE(read != nullptr);
  KIMIA_REQUIRE(read->name == "Hole");
  KIMIA_REQUIRE(near3(read->transform.position, Vec3{0.0, 0.01, -7.0}));
  KIMIA_REQUIRE(near3(read->transform.scale, Vec3{0.56, 0.02, 0.56}));
  KIMIA_REQUIRE(near(read->roughness, 0.8));
}

KIMIA_TEST(scene_find_by_name) {
  Scene scene;
  const EntityHandle a = scene.create(EntityData{"Alpha", {}, MeshKind::cube, "", Vec3{1.0, 1.0, 1.0}, 0.5});
  scene.create(EntityData{"Beta", {}, MeshKind::plane, "", Vec3{1.0, 1.0, 1.0}, 0.5});
  KIMIA_REQUIRE(scene.find("Beta") != kimia::kNullEntity);
  KIMIA_REQUIRE(scene.find("Alpha") == a);
  KIMIA_REQUIRE(scene.find("Missing") == kimia::kNullEntity);
  scene.destroy(a);
  KIMIA_REQUIRE(scene.find("Alpha") == kimia::kNullEntity);  // destroyed -> gone
}
