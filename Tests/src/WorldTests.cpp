#include <kimia_test.h>
#include <kimia/Golf.h>
#include <kimia/Physics.h>
#include <kimia/World.h>
#include <kimia/WorldIO.h>

#include <sys/stat.h>

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <string>

namespace {

using kimia::BallConfig;
using kimia::BallType;
using kimia::EnvironmentKind;
using kimia::EntityData;
using kimia::EntityHandle;
using kimia::PhysicsWorld;
using kimia::SphereBody;
using kimia::Vec3;
using kimia::WorldData;
using kimia::WorldEditor;
using kimia::WorldIO;
using kimia::f64;
using kimia::i32;
using kimia::u32;
using kimia::kGolfBallFriction;
using kimia::kGolfBallRadius;
using kimia::kGolfBallRestitution;
using kimia::kGolfBallRollingFriction;
using kimia::kWorldFantasyFriction;
using kimia::kWorldFantasyRadius;
using kimia::kWorldFantasyRestitution;
using kimia::kWorldFantasyRollingFriction;
using kimia::kWorldGoalZ;
using kimia::kWorldKickBase;
using kimia::kWorldKickSpeedScale;
using kimia::kWorldKickUp;
using kimia::kWorldPlayerFast;
using kimia::kWorldPlayerNormal;
using kimia::kWorldPlayerSlow;

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

bool fileExists(const std::string& path) {
  std::FILE* probe = std::fopen(path.c_str(), "rb");
  if (probe == nullptr) return false;
  std::fclose(probe);
  return true;
}

// Walks the editor into the object menu with a fresh world.
WorldEditor editorWithWorld() {
  WorldEditor editor;
  editor.choose(0);  // Main -> create world -> Menu
  return editor;
}

// From the object menu, opens the player question.
void askPlayer(WorldEditor& editor) { editor.choose(0); }

// From the object menu, opens the ball question.
void askBall(WorldEditor& editor) { editor.choose(1); }

// From the object menu, opens the environment question.
void askEnvironment(WorldEditor& editor) { editor.choose(2); }

}  // namespace

KIMIA_TEST(world_create_world_builds_default_scene) {
  WorldEditor editor;
  KIMIA_REQUIRE(!editor.hasWorld());
  KIMIA_REQUIRE(editor.optionLabels().size() == 3U);  // main menu: new/open/quit
  editor.choose(0);
  KIMIA_REQUIRE(editor.hasWorld());
  KIMIA_REQUIRE(editor.world().name == "MyWorld");
  KIMIA_REQUIRE(editor.world().score == 0U);
  KIMIA_REQUIRE(editor.world().scene.find("Ground") != kimia::kNullEntity);
  KIMIA_REQUIRE(editor.world().scene.find("GoalPostLeft") != kimia::kNullEntity);
  KIMIA_REQUIRE(editor.world().scene.find("GoalPostRight") != kimia::kNullEntity);
  KIMIA_REQUIRE(editor.world().scene.find("GoalBar") != kimia::kNullEntity);
  KIMIA_REQUIRE(editor.optionLabels().size() == 6U);  // object menu
}

KIMIA_TEST(world_add_player_answers_map_to_speed) {
  WorldEditor editor = editorWithWorld();
  KIMIA_REQUIRE(near(editor.world().player.speed, kWorldPlayerNormal));
  askPlayer(editor);
  KIMIA_REQUIRE(editor.optionLabels().size() == 4U);  // fast/normal/slow/back
  editor.choose(0);  // fast
  KIMIA_REQUIRE(near(editor.world().player.speed, kWorldPlayerFast));
  KIMIA_REQUIRE(editor.optionLabels().size() == 6U);  // back on the object menu
  askPlayer(editor);
  editor.choose(2);  // slow
  KIMIA_REQUIRE(near(editor.world().player.speed, kWorldPlayerSlow));
  askPlayer(editor);
  editor.choose(1);  // normal
  KIMIA_REQUIRE(near(editor.world().player.speed, kWorldPlayerNormal));
}

KIMIA_TEST(world_ball_question_accurate_vs_fantasy) {
  WorldEditor editor = editorWithWorld();
  KIMIA_REQUIRE(editor.world().ball.type == BallType::Accurate);
  askBall(editor);
  KIMIA_REQUIRE(editor.optionLabels().size() == 3U);  // accurate/fantasy/back
  editor.choose(0);  // accurate = the golf tuning
  KIMIA_REQUIRE(editor.world().ball.type == BallType::Accurate);
  KIMIA_REQUIRE(near(editor.world().ball.radius, kGolfBallRadius));
  KIMIA_REQUIRE(near(editor.world().ball.restitution, kGolfBallRestitution));
  KIMIA_REQUIRE(near(editor.world().ball.friction, kGolfBallFriction));
  KIMIA_REQUIRE(near(editor.world().ball.rollingFriction, kGolfBallRollingFriction));
  askBall(editor);
  editor.choose(1);  // fantasy = high bounce, low friction, no roll decay
  KIMIA_REQUIRE(editor.world().ball.type == BallType::Fantasy);
  KIMIA_REQUIRE(near(editor.world().ball.radius, kWorldFantasyRadius));
  KIMIA_REQUIRE(near(editor.world().ball.restitution, kWorldFantasyRestitution));
  KIMIA_REQUIRE(near(editor.world().ball.friction, kWorldFantasyFriction));
  KIMIA_REQUIRE(near(editor.world().ball.rollingFriction, kWorldFantasyRollingFriction));
  // The physics body follows the choice (rebuilt at the center).
  KIMIA_REQUIRE(near3(editor.ballPosition(), Vec3{0.0, 0.35, 0.0}, 1e-6));
}

KIMIA_TEST(world_add_environment_updates_colors) {
  WorldEditor editor = editorWithWorld();
  askEnvironment(editor);
  editor.choose(2);  // night
  KIMIA_REQUIRE(editor.world().environment == EnvironmentKind::Night);
  const EntityData* ground = editor.world().scene.get(editor.world().scene.find("Ground"));
  KIMIA_REQUIRE(ground != nullptr);
  KIMIA_REQUIRE(near3(ground->color, kimia::environmentColors(EnvironmentKind::Night).floor));
  askEnvironment(editor);
  editor.choose(1);  // sand
  KIMIA_REQUIRE(editor.world().environment == EnvironmentKind::Sand);
  KIMIA_REQUIRE(near3(ground->color, kimia::environmentColors(EnvironmentKind::Sand).floor));
  askEnvironment(editor);
  editor.choose(0);  // grass
  KIMIA_REQUIRE(editor.world().environment == EnvironmentKind::Grass);
  KIMIA_REQUIRE(near3(ground->color, kimia::environmentColors(EnvironmentKind::Grass).floor));
}

KIMIA_TEST(world_save_load_roundtrip_keeps_config) {
  WorldEditor editor = editorWithWorld();
  askPlayer(editor);
  editor.choose(0);  // fast
  askBall(editor);
  editor.choose(1);  // fantasy
  askEnvironment(editor);
  editor.choose(1);  // sand
  const std::string path = tmpPath("world_roundtrip.kimia");
  std::string error;
  KIMIA_REQUIRE(editor.saveWorld(path, error));
  KIMIA_REQUIRE(fileExists(path));

  WorldEditor reloaded;
  KIMIA_REQUIRE(reloaded.loadWorld(path, error));
  KIMIA_REQUIRE(reloaded.world().name == "MyWorld");
  KIMIA_REQUIRE(near(reloaded.world().player.speed, kWorldPlayerFast));
  KIMIA_REQUIRE(reloaded.world().ball.type == BallType::Fantasy);
  KIMIA_REQUIRE(near(reloaded.world().ball.radius, kWorldFantasyRadius));
  KIMIA_REQUIRE(reloaded.world().environment == EnvironmentKind::Sand);
  KIMIA_REQUIRE(reloaded.world().scene.find("GoalPostLeft") != kimia::kNullEntity);
  KIMIA_REQUIRE(reloaded.optionLabels().size() == 6U);  // lands on the object menu
}

KIMIA_TEST(world_old_v1_file_loads_with_defaults) {
  // A plain SceneIO-v1 file without any world metadata (as written by older
  // tools or by hand) must load with sensible defaults.
  const std::string text =
      "# KIMIA scene v1\n"
      "e \"OldGround\" mesh plane pos 0 0 0 scale 10 1 10 color 0.5 0.5 0.5 rough 0.9\n";
  WorldData loaded;
  std::string error;
  KIMIA_REQUIRE(WorldIO::load(text, loaded, error));
  KIMIA_REQUIRE(loaded.name == "MyWorld");
  KIMIA_REQUIRE(loaded.ball.type == BallType::Accurate);
  KIMIA_REQUIRE(near(loaded.ball.restitution, kGolfBallRestitution));
  KIMIA_REQUIRE(loaded.environment == EnvironmentKind::Grass);
  KIMIA_REQUIRE(loaded.scene.find("OldGround") != kimia::kNullEntity);

  const std::string path = tmpPath("old_v1.kimia");
  {
    std::FILE* file = std::fopen(path.c_str(), "wb");
    KIMIA_REQUIRE(file != nullptr);
    std::fwrite(text.data(), 1U, text.size(), file);
    std::fclose(file);
  }
  WorldEditor editor;
  KIMIA_REQUIRE(editor.loadWorld(path, error));
  KIMIA_REQUIRE(editor.world().scene.find("OldGround") != kimia::kNullEntity);
  KIMIA_REQUIRE(editor.optionLabels().size() == 6U);
}

KIMIA_TEST(world_kick_launches_ball_along_input) {
  WorldEditor editor = editorWithWorld();
  editor.choose(3);  // PLAY
  KIMIA_REQUIRE(editor.playing());
  editor.setPlayerPosition(Vec3{0.0, 0.5, 0.0});
  editor.setBallPosition(Vec3{0.6, 0.35, 0.0});
  editor.setMoveInput(1.0, 0.0);
  editor.update(0.0);  // kick applies before any physics step
  const Vec3 velocity = editor.ballVelocity();
  KIMIA_REQUIRE(near(velocity.x, kWorldKickBase + kWorldPlayerNormal * kWorldKickSpeedScale));
  KIMIA_REQUIRE(near(velocity.y, kWorldKickUp));
  KIMIA_REQUIRE(velocity.z == 0.0);
}

KIMIA_TEST(world_goal_detection_scores_and_resets) {
  WorldEditor editor = editorWithWorld();
  editor.choose(3);  // PLAY
  editor.setBallPosition(Vec3{0.0, 0.2, kWorldGoalZ - 0.2});
  editor.update(0.01);
  KIMIA_REQUIRE(editor.celebrating());
  KIMIA_REQUIRE(editor.score() == 1U);
  KIMIA_REQUIRE(editor.statsLine().find("GOAL") != std::string::npos);
  editor.update(2.5);  // celebration ends, ball returns to the center
  KIMIA_REQUIRE(editor.playing());
  KIMIA_REQUIRE(!editor.celebrating());
  KIMIA_REQUIRE(editor.score() == 1U);
  KIMIA_REQUIRE(near3(editor.ballPosition(), Vec3{0.0, 0.35, 0.0}, 1e-6));
}

KIMIA_TEST(world_fantasy_ball_outrolls_accurate) {
  // Same launch on a plane: the fantasy ball (low friction, no roll decay)
  // keeps its speed far longer than the accurate (golf) ball.
  const auto makeBall = [](BallConfig config) {
    SphereBody body;
    body.position = Vec3{0.0, config.radius + 0.01, 0.0};
    body.velocity = Vec3{5.0, 0.0, 0.0};
    body.radius = config.radius;
    body.restitution = config.restitution;
    body.friction = config.friction;
    body.rollingFriction = config.rollingFriction;
    return body;
  };
  BallConfig accurate;
  kimia::applyBallType(accurate, BallType::Accurate);
  BallConfig fantasy;
  kimia::applyBallType(fantasy, BallType::Fantasy);
  PhysicsWorld accurateWorld;
  PhysicsWorld fantasyWorld;
  accurateWorld.addPlane(0.0);
  fantasyWorld.addPlane(0.0);
  const u32 accurateId = accurateWorld.addSphere(makeBall(accurate));
  const u32 fantasyId = fantasyWorld.addSphere(makeBall(fantasy));
  for (i32 i = 0; i < 360; ++i) {  // 3 seconds at 120 Hz
    accurateWorld.advance(1.0 / 120.0);
    fantasyWorld.advance(1.0 / 120.0);
  }
  const f64 accurateSpeed = accurateWorld.sphere(accurateId)->velocity.length();
  const f64 fantasySpeed = fantasyWorld.sphere(fantasyId)->velocity.length();
  KIMIA_REQUIRE(fantasySpeed > accurateSpeed * 3.0);
  KIMIA_REQUIRE(accurateSpeed > 0.3);   // still rolling after 3 s
  KIMIA_REQUIRE(fantasySpeed > 3.5);    // barely slowed down
}

KIMIA_TEST(world_stats_line_reports_config) {
  WorldEditor editor = editorWithWorld();
  KIMIA_REQUIRE(editor.statsLine() ==
                "KIMIA WORLD | MENU | world MyWorld | player normal | ball accurate | env grass | score 0");
  askBall(editor);
  editor.choose(1);  // fantasy
  KIMIA_REQUIRE(editor.statsLine().find("ball fantasy") != std::string::npos);
  askPlayer(editor);
  editor.choose(0);  // fast
  KIMIA_REQUIRE(editor.statsLine().find("player fast") != std::string::npos);
  askEnvironment(editor);
  editor.choose(2);  // night
  KIMIA_REQUIRE(editor.statsLine().find("env night") != std::string::npos);
}

KIMIA_TEST(world_play_controls_reset_and_back) {
  WorldEditor editor = editorWithWorld();
  editor.choose(3);  // PLAY
  KIMIA_REQUIRE(editor.playing());
  KIMIA_REQUIRE(editor.holdPad().size() == 4U);   // the four direction pads
  KIMIA_REQUIRE(editor.tapPad().size() == 2U);    // reset + menu
  KIMIA_REQUIRE(editor.optionLabels().empty());   // no menu options in play
  editor.setBallPosition(Vec3{3.0, 0.35, 3.0});
  editor.resetBall();
  KIMIA_REQUIRE(near3(editor.ballPosition(), Vec3{0.0, 0.35, 0.0}, 1e-9));
  editor.backToMenu();
  KIMIA_REQUIRE(!editor.playing());
  KIMIA_REQUIRE(editor.optionLabels().size() == 6U);
  KIMIA_REQUIRE(editor.statsLine().find("MENU") != std::string::npos);
}

KIMIA_TEST(world_player_stays_inside_floor) {
  WorldEditor editor = editorWithWorld();
  editor.choose(3);  // PLAY
  editor.setMoveInput(1.0, 0.0);
  for (i32 i = 0; i < 600; ++i) editor.update(1.0 / 60.0);  // 10 s of running
  const Vec3 player = editor.playerPosition();
  KIMIA_REQUIRE(player.x <= kimia::kWorldFloorHalf);
  KIMIA_REQUIRE(player.x >= -kimia::kWorldFloorHalf);
}

KIMIA_TEST(world_quit_flag_from_main_menu) {
  WorldEditor editor;
  KIMIA_REQUIRE(!editor.quitRequested());
  editor.choose(2);  // quit
  KIMIA_REQUIRE(editor.quitRequested());
}
