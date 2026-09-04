#include <kimia_test.h>
#include <kimia/Golf.h>
#include <kimia/SceneIO.h>

#include <sys/stat.h>

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <string>

namespace {

using kimia::BuilderTool;
using kimia::EntityData;
using kimia::EntityHandle;
using kimia::GolfGame;
using kimia::GolfMode;
using kimia::Vec3;
using kimia::f64;
using kimia::i32;
using kimia::kGolfCupCaptureDistance;
using kimia::kGolfCupCaptureSpeed;
using kimia::kGolfLaunchBaseSpeed;
using kimia::kGolfLaunchPowerScale;

constexpr f64 kEps = 1e-9;

bool near(f64 a, f64 b, f64 eps = kEps) { return std::abs(a - b) <= eps; }

bool near3(const Vec3& a, const Vec3& b, f64 eps = 1e-9) {
  return near(a.x, b.x, eps) && near(a.y, b.y, eps) && near(a.z, b.z, eps);
}

std::string tmpPath(const std::string& name) {
  const int created = ::mkdir(KIMIA_TEST_TMP, 0755);
  static_cast<void>(created == 0 || errno == EEXIST);
  return std::string(KIMIA_TEST_TMP) + "/" + name;
}

EntityHandle findNamed(const GolfGame& game, const std::string& name) {
  EntityHandle found = kimia::kNullEntity;
  game.scene().forEach([&found, &name](EntityHandle handle, const EntityData& entity) {
    if (found == kimia::kNullEntity && entity.name == name) found = handle;
  });
  return found;
}

}  // namespace

KIMIA_TEST(golf_launch_speed_formula_exact) {
  GolfGame game;
  // power 0 -> base speed 2.5 (launch is instantaneous, no physics step yet)
  game.chargeBegin();
  game.chargeEnd();
  KIMIA_REQUIRE(near(game.ballSpeed(), kGolfLaunchBaseSpeed));
  // power 0.45 after 0.5 s of charge at 0.9/s
  game.togglePlayEdit();  // back to AIM
  game.togglePlayEdit();
  game.chargeBegin();
  game.update(0.5);
  game.chargeEnd();
  KIMIA_REQUIRE(near(game.power(), 0.45));
  KIMIA_REQUIRE(near(game.ballSpeed(), kGolfLaunchBaseSpeed + 0.45 * kGolfLaunchPowerScale));
  // near-max power: speed < 16.0 but close
  game.togglePlayEdit();
  game.togglePlayEdit();
  game.chargeBegin();
  game.update(1.105);
  game.chargeEnd();
  KIMIA_REQUIRE(game.ballSpeed() > 15.9 && game.ballSpeed() <= kGolfLaunchBaseSpeed + kGolfLaunchPowerScale);
}

KIMIA_TEST(golf_aim_direction_controls_velocity) {
  GolfGame game;
  KIMIA_REQUIRE(near(game.aimYaw(), 0.0));
  game.aimRight(0.1);  // yaw = -0.09
  KIMIA_REQUIRE(near(game.aimYaw(), -0.09));
  game.chargeBegin();
  game.chargeEnd();
  const Vec3 velocity = game.ballVelocity();
  KIMIA_REQUIRE(velocity.x > 0.0);  // -sin(-0.09) > 0
  KIMIA_REQUIRE(velocity.y == 0.0);
  KIMIA_REQUIRE(velocity.z < 0.0);  // -cos(-0.09) < 0
  KIMIA_REQUIRE(near(velocity.x / velocity.length(), std::sin(0.09), 1e-6));
  KIMIA_REQUIRE(near(velocity.z / velocity.length(), -std::cos(0.09), 1e-6));
}

KIMIA_TEST(golf_charge_climbs_and_wraps) {
  GolfGame game;
  game.chargeBegin();
  KIMIA_REQUIRE(game.power() == 0.0);
  game.update(0.2);
  const f64 p1 = game.power();
  game.update(0.2);
  KIMIA_REQUIRE(game.power() > p1);  // monotonic climb
  game.update(1.0);                  // 0.36 + 0.9 = 1.26 -> wraps to 0.26
  KIMIA_REQUIRE(game.power() > 0.2 && game.power() < 0.3);
}

KIMIA_TEST(golf_launch_counts_stroke_and_enters_roll) {
  GolfGame game;
  KIMIA_REQUIRE(game.mode() == GolfMode::Aim);
  KIMIA_REQUIRE(game.strokes() == 0U);
  game.chargeBegin();
  KIMIA_REQUIRE(game.mode() == GolfMode::Charge);
  game.update(0.5);
  game.chargeEnd();
  KIMIA_REQUIRE(game.mode() == GolfMode::Roll);
  KIMIA_REQUIRE(game.strokes() == 1U);
  KIMIA_REQUIRE(near3(game.ballPosition(), game.teePosition()));  // launched from the tee
}

KIMIA_TEST(golf_cup_capture_needs_both_conditions) {
  GolfGame game;
  const Vec3 hole = game.holePosition();
  // Close enough (0.2 < 0.28) but moving too fast: no capture yet.
  game.placeBall(Vec3{hole.x, 0.14, hole.z + 0.2}, Vec3{0.0, 0.0, -8.0});
  game.update(1.0 / 60.0);
  KIMIA_REQUIRE(game.mode() == GolfMode::Roll);
  KIMIA_REQUIRE(!game.ballInHole());
  // Slow enough and close enough: sunk.
  game.placeBall(Vec3{hole.x, 0.14, hole.z + 0.2}, Vec3{0.0, 0.0, -1.0});
  game.update(1.0 / 60.0);
  KIMIA_REQUIRE(game.mode() == GolfMode::Sunk);
  KIMIA_REQUIRE(game.ballInHole());
  // After the celebration the round resets.
  game.update(3.0);
  KIMIA_REQUIRE(game.mode() == GolfMode::Aim);
  KIMIA_REQUIRE(!game.ballInHole());
  KIMIA_REQUIRE(game.strokes() == 0U);
  KIMIA_REQUIRE(near3(game.ballPosition(), game.teePosition()));
}

KIMIA_TEST(golf_out_returns_ball_to_tee_and_keeps_strokes) {
  GolfGame game;
  game.chargeBegin();
  game.update(0.1);
  game.chargeEnd();  // strokes = 1
  // Put the ball far from the cup with a tiny velocity: it rolls out.
  game.placeBall(Vec3{5.0, 0.14, 5.0}, Vec3{0.0, 0.0, 0.2});
  bool sawOut = false;
  for (i32 i = 0; i < 2400; ++i) {
    game.update(1.0 / 60.0);
    if (game.mode() == GolfMode::Out) sawOut = true;
    if (game.mode() == GolfMode::Aim) break;
  }
  KIMIA_REQUIRE(sawOut);
  KIMIA_REQUIRE(game.mode() == GolfMode::Aim);
  KIMIA_REQUIRE(game.strokes() == 1U);  // OUT keeps the stroke count
  KIMIA_REQUIRE(near3(game.ballPosition(), game.teePosition()));
}

KIMIA_TEST(golf_builder_wall_place_and_undo) {
  GolfGame game;
  KIMIA_REQUIRE(game.wallCount() == 1U);  // the default course's Wall_1
  game.setTool(BuilderTool::Wall);
  KIMIA_REQUIRE(game.mode() == GolfMode::Edit);
  game.moveGhost(1.0, 0.0, false);
  KIMIA_REQUIRE(game.place());
  KIMIA_REQUIRE(game.wallCount() == 2U);
  KIMIA_REQUIRE(game.physicsWorld().boxCount() == 2U);
  const EntityHandle placed = findNamed(game, "Wall_2");
  KIMIA_REQUIRE(placed != kimia::kNullEntity);
  const EntityData* wall = game.scene().get(placed);
  KIMIA_REQUIRE(wall != nullptr);
  KIMIA_REQUIRE(near3(wall->transform.position, Vec3{1.0, 0.5, 0.0}));
  KIMIA_REQUIRE(near3(wall->transform.scale, Vec3{0.5, 1.0, 4.0}));  // default axis Z
  KIMIA_REQUIRE(game.undo());
  KIMIA_REQUIRE(game.wallCount() == 1U);
  KIMIA_REQUIRE(game.physicsWorld().boxCount() == 1U);
}

KIMIA_TEST(golf_wall_length_clamped_and_axis_toggle) {
  GolfGame game;
  game.setTool(BuilderTool::Wall);
  game.adjustWallLength(100.0);
  KIMIA_REQUIRE(game.wallLength() == 12.0);
  game.adjustWallLength(-100.0);
  KIMIA_REQUIRE(game.wallLength() == 2.0);
  game.adjustWallLength(4.0);
  KIMIA_REQUIRE(game.wallLength() == 6.0);
  game.toggleWallAxis();
  KIMIA_REQUIRE(!game.wallAxisZ());
  game.moveGhost(3.0, 1.0, false);
  KIMIA_REQUIRE(game.place());
  const EntityData* wall = game.scene().get(findNamed(game, "Wall_2"));
  KIMIA_REQUIRE(wall != nullptr);
  KIMIA_REQUIRE(near3(wall->transform.scale, Vec3{6.0, 1.0, 0.5}));
}

KIMIA_TEST(golf_tee_and_hole_move_with_undo) {
  GolfGame game;
  KIMIA_REQUIRE(near3(game.teePosition(), Vec3{0.0, 0.14, 7.0}));
  game.setTool(BuilderTool::Tee);
  game.moveGhost(2.0, 1.0, false);
  KIMIA_REQUIRE(game.place());
  KIMIA_REQUIRE(near3(game.teePosition(), Vec3{2.0, 0.14, 1.0}));
  KIMIA_REQUIRE(near3(game.ballPosition(), game.teePosition()));  // ball follows the tee
  KIMIA_REQUIRE(game.undo());
  KIMIA_REQUIRE(near3(game.teePosition(), Vec3{0.0, 0.14, 7.0}));
  game.setTool(BuilderTool::Hole);
  game.moveGhost(-1.0, -2.0, false);  // ghost is relative: (2,0,1) -> (1,0,-1)
  KIMIA_REQUIRE(game.place());
  KIMIA_REQUIRE(near3(game.holePosition(), Vec3{1.0, 0.01, -1.0}));
  KIMIA_REQUIRE(game.undo());
  KIMIA_REQUIRE(near3(game.holePosition(), Vec3{0.0, 0.01, -7.0}));
}

KIMIA_TEST(golf_save_load_roundtrip_with_demo_line) {
  GolfGame game;
  game.chargeBegin();
  game.update(0.5);   // power 0.45
  game.chargeEnd();   // records lastShot {aim 0.0, power 0.45}
  game.setTool(BuilderTool::Wall);
  game.moveGhost(1.0, 0.0, false);
  KIMIA_REQUIRE(game.place());  // walls 2
  const std::string path = tmpPath("golf_roundtrip.kimia");
  std::string error;
  KIMIA_REQUIRE(game.saveCourse(path, error));
  std::FILE* probe = std::fopen(path.c_str(), "rb");
  KIMIA_REQUIRE(probe != nullptr);
  std::fclose(probe);
  GolfGame reloaded;
  KIMIA_REQUIRE(reloaded.loadCourse(path, error));
  KIMIA_REQUIRE(reloaded.wallCount() == 2U);
  KIMIA_REQUIRE(near3(reloaded.teePosition(), Vec3{0.0, 0.14, 7.0}));
  KIMIA_REQUIRE(near3(reloaded.holePosition(), Vec3{0.0, 0.01, -7.0}));
  KIMIA_REQUIRE(reloaded.demoShot().has_value());
  KIMIA_REQUIRE(near(reloaded.demoShot()->aim, 0.0));
  KIMIA_REQUIRE(near(reloaded.demoShot()->power, 0.45));
  KIMIA_REQUIRE(reloaded.mode() == GolfMode::Edit);  // loading is a builder action
}

KIMIA_TEST(golf_stats_line_has_required_format) {
  GolfGame game;
  KIMIA_REQUIRE(game.statsLine() ==
                "KIMIA GOLF | AIM | stroke 0 | power 50% | tool wall len 4 Z | walls 1");
  game.chargeBegin();
  game.update(0.5);
  const std::string charging = game.statsLine();
  KIMIA_REQUIRE(charging.find("CHARGE") != std::string::npos);
  KIMIA_REQUIRE(charging.find("power 45%") != std::string::npos);
  game.chargeEnd();
  const std::string rolling = game.statsLine();
  KIMIA_REQUIRE(rolling.find("ROLL") != std::string::npos);
  KIMIA_REQUIRE(rolling.find("stroke 1") != std::string::npos);
  game.setTool(BuilderTool::Tee);
  KIMIA_REQUIRE(game.statsLine().find("tool tee") != std::string::npos);
  KIMIA_REQUIRE(game.statsLine().find("EDIT") != std::string::npos);
}

KIMIA_TEST(golf_demo_shot_launches_with_course_values) {
  GolfGame game;
  KIMIA_REQUIRE(game.demoShot().has_value());
  KIMIA_REQUIRE(near(game.demoShot()->aim, 0.0));
  KIMIA_REQUIRE(near(game.demoShot()->power, 0.61));
  game.launchDemoShot();
  KIMIA_REQUIRE(game.mode() == GolfMode::Roll);
  KIMIA_REQUIRE(game.strokes() == 1U);
  KIMIA_REQUIRE(near(game.aimYaw(), 0.0));
  KIMIA_REQUIRE(near(game.ballSpeed(), kGolfLaunchBaseSpeed + 0.61 * kGolfLaunchPowerScale));
  KIMIA_REQUIRE(game.ballVelocity().x == 0.0);
  KIMIA_REQUIRE(game.ballVelocity().z < 0.0);  // straight at the hole
}

KIMIA_TEST(golf_builder_keys_switch_play_to_edit_and_act) {
  GolfGame game;
  KIMIA_REQUIRE(game.mode() == GolfMode::Aim);
  game.setTool(BuilderTool::Hole);
  KIMIA_REQUIRE(game.mode() == GolfMode::Edit);
  KIMIA_REQUIRE(game.tool() == BuilderTool::Hole);
  game.togglePlayEdit();
  KIMIA_REQUIRE(game.mode() == GolfMode::Aim);
  game.adjustWallLength(0.5);
  KIMIA_REQUIRE(game.mode() == GolfMode::Edit);  // acts AND switches
  KIMIA_REQUIRE(game.wallLength() == 4.5);
  game.togglePlayEdit();
  KIMIA_REQUIRE(game.mode() == GolfMode::Aim);
  KIMIA_REQUIRE(game.place());  // place from play mode: switches to EDIT and acts
  KIMIA_REQUIRE(game.mode() == GolfMode::Edit);
  KIMIA_REQUIRE(near3(game.holePosition(), Vec3{0.0, 0.01, 0.0}));  // hole tool, ghost at origin
  game.setTool(BuilderTool::Wall);
  KIMIA_REQUIRE(game.place());  // wall at the ghost
  KIMIA_REQUIRE(game.wallCount() == 2U);
}

KIMIA_TEST(golf_default_course_layout) {
  GolfGame game;
  KIMIA_REQUIRE(near3(game.teePosition(), Vec3{0.0, 0.14, 7.0}));
  KIMIA_REQUIRE(near3(game.holePosition(), Vec3{0.0, 0.01, -7.0}));
  KIMIA_REQUIRE(game.wallCount() == 1U);
  KIMIA_REQUIRE(game.strokes() == 0U);
  KIMIA_REQUIRE(game.mode() == GolfMode::Aim);
  KIMIA_REQUIRE(findNamed(game, "Green") != kimia::kNullEntity);
  KIMIA_REQUIRE(findNamed(game, "FlagPole") != kimia::kNullEntity);
  KIMIA_REQUIRE(findNamed(game, "FlagCloth") != kimia::kNullEntity);
  KIMIA_REQUIRE(findNamed(game, "Ball") != kimia::kNullEntity);
  const f64 teeHoleDistance = std::abs(game.teePosition().z - game.holePosition().z);
  KIMIA_REQUIRE(near(teeHoleDistance, 14.0));
}

KIMIA_TEST(golf_demo_shot_sinks_from_default_tee) {
  GolfGame game;
  game.launchDemoShot();
  bool sunk = false;
  for (i32 i = 0; i < 3600; ++i) {  // up to 60 s of simulation
    game.update(1.0 / 60.0);
    if (game.mode() == GolfMode::Sunk) {
      sunk = true;
      break;
    }
    if (game.mode() == GolfMode::Aim) break;  // ended without capture
  }
  KIMIA_REQUIRE(sunk);  // the spec's demo shot (aim 0, power 0.61) is tuned to hole out
}
