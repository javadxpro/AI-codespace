#include <kimia_test.h>
#include <kimia/Golf.h>
#include <kimia/Physics.h>
#include <kimia/World.h>
#include <kimia/WorldIO.h>

#include <sys/stat.h>

#include <algorithm>
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
using kimia::kWorldBlockMedium;
using kimia::kWorldBlockSmall;
using kimia::kWorldFantasyFriction;
using kimia::kWorldFantasyRadius;
using kimia::kWorldFantasyRestitution;
using kimia::kWorldFantasyRollingFriction;
using kimia::kWorldCrateKickScale;
using kimia::kWorldCrateKickUp;
using kimia::kWorldGoalMedium;
using kimia::kWorldKickBase;
using kimia::kWorldKickSpeedScale;
using kimia::kWorldPlayerFast;
using kimia::kWorldPlayerNormal;
using kimia::kWorldPlayerSlow;
using kimia::kWorldWallLong;

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

// Walks the editor into the builder with a fresh empty world.
WorldEditor editorWithWorld() {
  WorldEditor editor;
  editor.choose(0);  // Main -> create world -> Builder
  return editor;
}

void exitPlace(WorldEditor& editor) { editor.choose(1); }  // «بازگشت» from Place

// Builder(0) -> Catalog(2) -> block size -> Place at the ghost -> placed.
void addBlock(WorldEditor& editor, i32 sizeOption, const Vec3& ghost) {
  editor.choose(0);  // catalog
  editor.choose(2);  // block
  editor.choose(sizeOption);
  editor.setGhostPosition(ghost);
  editor.choose(0);  // place
}

void addGoal(WorldEditor& editor, i32 widthOption, const Vec3& ghost) {
  editor.choose(0);  // catalog
  editor.choose(4);  // goal
  editor.choose(widthOption);
  editor.setGhostPosition(ghost);
  editor.choose(0);  // place
}

void addCrate(WorldEditor& editor, const Vec3& ghost) {
  editor.choose(0);  // catalog
  editor.choose(5);  // جعبه (no size question: fixed 1x1x1)
  editor.setGhostPosition(ghost);
  editor.choose(0);  // place
}

void addPlayer(WorldEditor& editor, i32 speedOption, const Vec3& ghost) {
  editor.choose(0);  // catalog
  editor.choose(0);  // player
  editor.choose(speedOption);
  editor.setGhostPosition(ghost);
  editor.choose(0);  // place
}

void addBall(WorldEditor& editor, i32 typeOption, const Vec3& ghost) {
  editor.choose(0);  // catalog
  editor.choose(1);  // ball
  editor.choose(typeOption);
  editor.setGhostPosition(ghost);
  editor.choose(0);  // place
}

void enterManage(WorldEditor& editor) {
  editor.choose(1);  // builder -> manage
}

}  // namespace

KIMIA_TEST(world_create_project_gives_empty_ground) {
  WorldEditor editor;
  KIMIA_REQUIRE(!editor.hasWorld());
  KIMIA_REQUIRE(editor.optionLabels().size() == 3U);  // main menu
  editor.choose(0);
  KIMIA_REQUIRE(editor.hasWorld());
  KIMIA_REQUIRE(editor.world().name == "MyWorld");
  KIMIA_REQUIRE(editor.world().score == 0U);
  KIMIA_REQUIRE(editor.world().scene.find("Ground") != kimia::kNullEntity);
  KIMIA_REQUIRE(editor.objectCount() == 0U);  // nothing but the ground
  KIMIA_REQUIRE(editor.goalCount() == 0U);
  KIMIA_REQUIRE(editor.optionLabels().size() == 6U);  // builder menu
}

KIMIA_TEST(world_catalog_adds_block_with_chosen_size) {
  WorldEditor editor = editorWithWorld();
  addBlock(editor, 0, Vec3{2.0, 0.0, 1.0});  // small block
  exitPlace(editor);
  KIMIA_REQUIRE(editor.objectCount() == 1U);
  const EntityData* block = editor.world().scene.get(editor.world().scene.find("Block_1"));
  KIMIA_REQUIRE(block != nullptr);
  KIMIA_REQUIRE(near3(block->transform.position, Vec3{2.0, kWorldBlockSmall * 0.5, 1.0}));
  KIMIA_REQUIRE(near3(block->transform.scale, Vec3{kWorldBlockSmall, kWorldBlockSmall, kWorldBlockSmall}));
  KIMIA_REQUIRE(editor.physicsBoxCount() == 1U);
  addBlock(editor, 1, Vec3{0.0, 0.0, 0.0});  // medium block — numbering continues
  exitPlace(editor);
  KIMIA_REQUIRE(editor.objectCount() == 2U);
  KIMIA_REQUIRE(editor.world().scene.find("Block_2") != kimia::kNullEntity);
  KIMIA_REQUIRE(editor.physicsBoxCount() == 2U);
}

KIMIA_TEST(world_wall_length_and_axis_questions) {
  WorldEditor editor = editorWithWorld();
  editor.choose(0);  // catalog
  editor.choose(3);  // wall
  editor.choose(2);  // بلند (9)
  KIMIA_REQUIRE(editor.optionLabels().size() == 3U);  // axis question
  editor.choose(1);  // محور X (عرض)
  editor.setGhostPosition(Vec3{0.0, 0.0, -3.0});
  editor.choose(0);  // place
  exitPlace(editor);
  const EntityData* wall = editor.world().scene.get(editor.world().scene.find("Wall_1"));
  KIMIA_REQUIRE(wall != nullptr);
  KIMIA_REQUIRE(near3(wall->transform.scale, Vec3{kWorldWallLong, 1.0, 0.5}));
  KIMIA_REQUIRE(near3(wall->transform.position, Vec3{0.0, 0.5, -3.0}));
  editor.choose(0);  // catalog
  editor.choose(3);  // wall
  editor.choose(0);  // کوتاه
  editor.choose(0);  // محور Z
  editor.setGhostPosition(Vec3{1.0, 0.0, 0.0});
  editor.choose(0);  // place
  exitPlace(editor);
  const EntityData* wall2 = editor.world().scene.get(editor.world().scene.find("Wall_2"));
  KIMIA_REQUIRE(wall2 != nullptr);
  KIMIA_REQUIRE(near3(wall2->transform.scale, Vec3{0.5, 1.0, 3.0}));
}

KIMIA_TEST(world_goal_object_builds_physics_and_capture) {
  WorldEditor editor = editorWithWorld();
  addGoal(editor, 1, Vec3{0.0, 0.0, -4.0});  // medium goal (3 wide)
  exitPlace(editor);
  KIMIA_REQUIRE(editor.goalCount() == 1U);
  const EntityData* goal = editor.world().scene.get(editor.world().scene.find("Goal_1"));
  KIMIA_REQUIRE(goal != nullptr);
  KIMIA_REQUIRE(near3(goal->transform.scale, Vec3{kWorldGoalMedium, 2.0, 0.12}));
  // Physics: two posts + one crossbar.
  KIMIA_REQUIRE(editor.physicsBoxCount() == 3U);
}

KIMIA_TEST(world_add_player_answers_map_to_speed) {
  WorldEditor editor = editorWithWorld();
  addPlayer(editor, 0, Vec3{1.0, 0.0, 2.0});  // fast
  exitPlace(editor);
  KIMIA_REQUIRE(near(editor.world().player.speed, kWorldPlayerFast));
  const EntityData* player = editor.world().scene.get(editor.world().scene.find("Player"));
  KIMIA_REQUIRE(player != nullptr);
  KIMIA_REQUIRE(near3(player->transform.position, Vec3{1.0, 0.5, 2.0}));
  addPlayer(editor, 2, Vec3{0.0, 0.0, 0.0});  // slow — reconfigures, does not duplicate
  exitPlace(editor);
  KIMIA_REQUIRE(near(editor.world().player.speed, kWorldPlayerSlow));
  KIMIA_REQUIRE(editor.objectCount() == 1U);  // still one player object
  KIMIA_REQUIRE(near3(player->transform.position, Vec3{0.0, 0.5, 0.0}));
}

KIMIA_TEST(world_ball_question_accurate_vs_fantasy) {
  WorldEditor editor = editorWithWorld();
  addBall(editor, 1, Vec3{0.0, 0.0, 0.0});  // fantasy
  exitPlace(editor);
  KIMIA_REQUIRE(editor.world().ball.type == BallType::Fantasy);
  KIMIA_REQUIRE(near(editor.world().ball.radius, kWorldFantasyRadius));
  KIMIA_REQUIRE(near(editor.world().ball.restitution, kWorldFantasyRestitution));
  KIMIA_REQUIRE(near(editor.world().ball.friction, kWorldFantasyFriction));
  KIMIA_REQUIRE(near(editor.world().ball.rollingFriction, kWorldFantasyRollingFriction));
  const EntityData* ball = editor.world().scene.get(editor.world().scene.find("Ball"));
  KIMIA_REQUIRE(ball != nullptr);
  const f64 diameter = kWorldFantasyRadius * 2.0;
  KIMIA_REQUIRE(near3(ball->transform.scale, Vec3{diameter, diameter, diameter}));
  KIMIA_REQUIRE(near3(editor.ballPosition(), Vec3{0.0, kWorldFantasyRadius, 0.0}, 1e-6));
  addBall(editor, 0, Vec3{1.0, 0.0, 2.0});  // accurate — same single entity
  exitPlace(editor);
  KIMIA_REQUIRE(editor.world().ball.type == BallType::Accurate);
  KIMIA_REQUIRE(near(editor.world().ball.radius, kGolfBallRadius));
  KIMIA_REQUIRE(near(editor.world().ball.restitution, kGolfBallRestitution));
  KIMIA_REQUIRE(near(editor.world().ball.friction, kGolfBallFriction));
  KIMIA_REQUIRE(near(editor.world().ball.rollingFriction, kGolfBallRollingFriction));
  KIMIA_REQUIRE(near3(ball->transform.position, Vec3{1.0, kGolfBallRadius, 2.0}));
  KIMIA_REQUIRE(editor.objectCount() == 1U);
}

KIMIA_TEST(world_manage_lists_moves_deletes_and_colors) {
  WorldEditor editor = editorWithWorld();
  addBlock(editor, 1, Vec3{2.0, 0.0, 0.0});
  exitPlace(editor);
  addGoal(editor, 0, Vec3{0.0, 0.0, -5.0});
  exitPlace(editor);
  KIMIA_REQUIRE(editor.objectCount() == 2U);

  enterManage(editor);
  KIMIA_REQUIRE(editor.managedCount() == 2U);
  KIMIA_REQUIRE(editor.optionLabels().size() == 6U);  // prev/next/move/delete/color/back
  const std::string first = editor.managedName();
  editor.choose(1);  // بعدی
  const std::string second = editor.managedName();
  KIMIA_REQUIRE(first != second);
  editor.choose(0);  // قبلی
  KIMIA_REQUIRE(editor.managedName() == first);

  // Move: arrows move the selected object live.
  editor.choose(2);  // جابه‌جایی
  KIMIA_REQUIRE(editor.optionLabels().size() == 1U);  // پایان
  editor.setMoveInput(1.0, 0.0);
  editor.update(0.5);  // 2 units/s * 0.5 s = +1.0 on x
  editor.setMoveInput(0.0, 0.0);
  editor.choose(0);  // پایان
  const EntityData* moved = editor.selectedEntity();
  KIMIA_REQUIRE(moved != nullptr);
  KIMIA_REQUIRE(near3(moved->transform.position, Vec3{3.0, 0.5, 0.0}));  // the block at (2,0.5,0)

  // Color: pick red.
  editor.choose(4);  // رنگ
  editor.choose(0);  // قرمز
  KIMIA_REQUIRE(near3(moved->color, Vec3{0.85, 0.15, 0.15}));

  // Delete: confirm yes.
  editor.choose(3);  // حذف
  KIMIA_REQUIRE(editor.optionLabels().size() == 2U);  // بله/نه
  editor.choose(0);  // بله
  KIMIA_REQUIRE(editor.objectCount() == 1U);
  KIMIA_REQUIRE(editor.world().scene.find("Block_1") == kimia::kNullEntity);
  editor.choose(5);  // بازگشت
  KIMIA_REQUIRE(editor.optionLabels().size() == 6U);  // back on the builder
}

KIMIA_TEST(world_empty_manage_shows_only_back) {
  WorldEditor editor = editorWithWorld();
  enterManage(editor);
  KIMIA_REQUIRE(editor.managedCount() == 0U);
  KIMIA_REQUIRE(editor.optionLabels().size() == 1U);
  editor.choose(0);
  KIMIA_REQUIRE(editor.optionLabels().size() == 6U);  // back on the builder
}

KIMIA_TEST(world_add_environment_updates_colors) {
  WorldEditor editor = editorWithWorld();
  editor.choose(2);  // محیط
  editor.choose(2);  // شب
  const EntityData* ground = editor.world().scene.get(editor.world().scene.find("Ground"));
  KIMIA_REQUIRE(ground != nullptr);
  KIMIA_REQUIRE(near3(ground->color, kimia::environmentColors(EnvironmentKind::Night).floor));
  editor.choose(2);  // محیط
  editor.choose(1);  // شنی
  KIMIA_REQUIRE(near3(ground->color, kimia::environmentColors(EnvironmentKind::Sand).floor));
  editor.choose(2);
  editor.choose(0);  // چمن‌زار
  KIMIA_REQUIRE(near3(ground->color, kimia::environmentColors(EnvironmentKind::Grass).floor));
}

KIMIA_TEST(world_save_load_roundtrip_keeps_objects) {
  WorldEditor editor = editorWithWorld();
  addBlock(editor, 0, Vec3{2.0, 0.0, 1.0});
  exitPlace(editor);
  editor.choose(0);
  editor.choose(3);  // wall
  editor.choose(2);  // بلند
  editor.choose(1);  // محور X
  editor.setGhostPosition(Vec3{0.0, 0.0, -3.0});
  editor.choose(0);
  exitPlace(editor);
  addGoal(editor, 1, Vec3{0.0, 0.0, -6.0});
  exitPlace(editor);
  addPlayer(editor, 0, Vec3{0.0, 0.0, 4.0});
  exitPlace(editor);
  addBall(editor, 1, Vec3{0.0, 0.0, 1.0});
  exitPlace(editor);
  editor.choose(2);  // محیط
  editor.choose(1);  // شنی
  KIMIA_REQUIRE(editor.objectCount() == 5U);

  const std::string path = tmpPath("world_builder_roundtrip.kimia");
  std::string error;
  KIMIA_REQUIRE(editor.saveWorld(path, error));
  KIMIA_REQUIRE(fileExists(path));

  WorldEditor reloaded;
  KIMIA_REQUIRE(reloaded.loadWorld(path, error));
  KIMIA_REQUIRE(reloaded.objectCount() == 5U);
  KIMIA_REQUIRE(reloaded.goalCount() == 1U);
  KIMIA_REQUIRE(reloaded.world().scene.find("Block_1") != kimia::kNullEntity);
  KIMIA_REQUIRE(reloaded.world().scene.find("Wall_1") != kimia::kNullEntity);
  KIMIA_REQUIRE(reloaded.world().scene.find("Goal_1") != kimia::kNullEntity);
  KIMIA_REQUIRE(reloaded.world().scene.find("Player") != kimia::kNullEntity);
  KIMIA_REQUIRE(reloaded.world().scene.find("Ball") != kimia::kNullEntity);
  KIMIA_REQUIRE(near(reloaded.world().player.speed, kWorldPlayerFast));
  KIMIA_REQUIRE(reloaded.world().ball.type == BallType::Fantasy);
  KIMIA_REQUIRE(reloaded.world().environment == EnvironmentKind::Sand);
  // Physics rebuilt from the file: block + wall + 3 goal boxes.
  KIMIA_REQUIRE(reloaded.physicsBoxCount() == 5U);
  KIMIA_REQUIRE(reloaded.optionLabels().size() == 6U);  // lands on the builder
}

KIMIA_TEST(world_old_v1_file_loads_with_defaults) {
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

KIMIA_TEST(world_legacy_goal_trio_forms_one_goal) {
  // Files written by older versions used three entities per goal; they must
  // still load as ONE goal with working capture.
  const std::string text =
      "# KIMIA scene v1\n"
      "e \"Ground\" mesh plane pos 0 0 0 scale 20 1 20 color 0.22 0.45 0.24 rough 0.95\n"
      "e \"GoalPostLeft\" mesh cube pos -2 1 -2 scale 0.12 2 0.12 color 0.9 0.9 0.9 rough 0.4\n"
      "e \"GoalPostRight\" mesh cube pos 2 1 -2 scale 0.12 2 0.12 color 0.9 0.9 0.9 rough 0.4\n"
      "e \"GoalBar\" mesh cube pos 0 2.02 -2 scale 4.12 0.12 0.12 color 0.9 0.9 0.9 rough 0.4\n"
      "e \"Ball\" mesh sphere pos 0 0.12 0 scale 0.24 0.24 0.24 color 0.95 0.95 0.92 rough 0.3\n"
      "e \"Player\" mesh cube pos 0 0.5 4 scale 0.6 1 0.6 color 0.2 0.5 0.9 rough 0.5\n";
  const std::string path = tmpPath("legacy_goal.kimia");
  {
    std::FILE* file = std::fopen(path.c_str(), "wb");
    KIMIA_REQUIRE(file != nullptr);
    std::fwrite(text.data(), 1U, text.size(), file);
    std::fclose(file);
  }
  WorldEditor editor;
  std::string error;
  KIMIA_REQUIRE(editor.loadWorld(path, error));
  KIMIA_REQUIRE(editor.goalCount() == 1U);  // the trio counts as ONE goal
  KIMIA_REQUIRE(editor.physicsBoxCount() == 3U);

  // Crossing the legacy goal scores.
  editor.choose(3);  // PLAY
  editor.setPlayerPosition(Vec3{0.0, 0.5, 0.6});
  editor.setMoveInput(0.0, -1.0);
  editor.update(0.0);  // kick toward -Z
  bool scored = false;
  for (i32 i = 0; i < 600; ++i) {
    editor.update(1.0 / 60.0);
    if (editor.celebrating()) {
      scored = true;
      break;
    }
  }
  KIMIA_REQUIRE(scored);
  KIMIA_REQUIRE(editor.score() == 1U);
}

KIMIA_TEST(world_kick_launches_ball_along_input) {
  WorldEditor editor = editorWithWorld();
  addBall(editor, 0, Vec3{0.0, 0.0, 0.0});
  exitPlace(editor);
  editor.choose(3);  // PLAY
  KIMIA_REQUIRE(editor.playing());
  editor.setPlayerPosition(Vec3{0.0, 0.5, 0.0});
  editor.setBallPosition(Vec3{0.6, kGolfBallRadius, 0.0});
  editor.setMoveInput(1.0, 0.0);
  editor.update(0.0);  // kick applies before any physics step
  const Vec3 velocity = editor.ballVelocity();
  KIMIA_REQUIRE(near(velocity.x, kimia::kWorldKickBase + kWorldPlayerNormal * kimia::kWorldKickSpeedScale));
  KIMIA_REQUIRE(near(velocity.y, kimia::kWorldKickUp));
  KIMIA_REQUIRE(velocity.z == 0.0);
}

KIMIA_TEST(world_goal_crossing_scores_and_resets) {
  WorldEditor editor = editorWithWorld();
  addGoal(editor, 1, Vec3{0.0, 0.0, -2.0});
  exitPlace(editor);
  editor.choose(3);  // PLAY
  editor.setPlayerPosition(Vec3{0.0, 0.5, 0.6});
  editor.setMoveInput(0.0, -1.0);
  editor.update(0.0);  // kick toward the goal
  KIMIA_REQUIRE(editor.playing());
  bool scored = false;
  for (i32 i = 0; i < 600; ++i) {
    editor.update(1.0 / 60.0);
    if (editor.celebrating()) {
      scored = true;
      break;
    }
  }
  KIMIA_REQUIRE(scored);
  KIMIA_REQUIRE(editor.score() == 1U);
  KIMIA_REQUIRE(editor.statsLine().find("GOAL") != std::string::npos);
  editor.update(2.5);  // celebration ends, ball returns to its spawn
  KIMIA_REQUIRE(editor.playing());
  KIMIA_REQUIRE(!editor.celebrating());
  KIMIA_REQUIRE(editor.score() == 1U);
  KIMIA_REQUIRE(near3(editor.ballPosition(), Vec3{0.0, kGolfBallRadius, 0.0}, 1e-6));
}

KIMIA_TEST(world_ball_spawn_follows_entity_position) {
  WorldEditor editor = editorWithWorld();
  addBall(editor, 0, Vec3{1.5, 0.0, 2.5});
  exitPlace(editor);
  editor.choose(3);  // PLAY
  KIMIA_REQUIRE(near3(editor.ballPosition(), Vec3{1.5, kGolfBallRadius, 2.5}, 1e-6));
  KIMIA_REQUIRE(near3(editor.playerPosition(), Vec3{0.0, 0.5, 4.0}, 1e-6));  // default spawn
}

KIMIA_TEST(world_fantasy_ball_outrolls_accurate) {
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
  // Constant-force friction: the accurate ball (0.62 * g) stops in ~0.82 s,
  // the fantasy ball (0.05 * g) still rolls at ~3.54 m/s after 3 s.
  KIMIA_REQUIRE(accurateSpeed == 0.0);
  KIMIA_REQUIRE(fantasySpeed > 3.4);
  KIMIA_REQUIRE(fantasySpeed < 3.7);
  KIMIA_REQUIRE(fantasySpeed > accurateSpeed * 10.0);
}

KIMIA_TEST(world_play_controls_reset_and_back) {
  WorldEditor editor = editorWithWorld();
  editor.choose(3);  // PLAY
  KIMIA_REQUIRE(editor.playing());
  KIMIA_REQUIRE(editor.holdPad().size() == 4U);  // the four direction pads
  KIMIA_REQUIRE(editor.tapPad().size() == 2U);   // reset + menu
  KIMIA_REQUIRE(editor.optionLabels().empty());
  editor.setBallPosition(Vec3{3.0, kGolfBallRadius, 3.0});
  editor.resetBall();
  KIMIA_REQUIRE(near3(editor.ballPosition(), Vec3{0.0, kGolfBallRadius, 0.0}, 1e-9));
  editor.backToMenu();
  KIMIA_REQUIRE(!editor.playing());
  KIMIA_REQUIRE(editor.optionLabels().size() == 6U);
  KIMIA_REQUIRE(editor.statsLine().find("BUILDER") != std::string::npos);
}

KIMIA_TEST(world_player_stays_inside_floor) {
  WorldEditor editor = editorWithWorld();
  editor.choose(3);  // PLAY
  editor.setMoveInput(1.0, 0.0);
  for (i32 i = 0; i < 600; ++i) editor.update(1.0 / 60.0);
  const Vec3 player = editor.playerPosition();
  KIMIA_REQUIRE(player.x <= kimia::kWorldFloorHalf);
  KIMIA_REQUIRE(player.x >= -kimia::kWorldFloorHalf);
}

KIMIA_TEST(world_stats_line_reports_config) {
  WorldEditor editor = editorWithWorld();
  KIMIA_REQUIRE(editor.statsLine() ==
                "KIMIA WORLD | BUILDER | world MyWorld | player normal | ball accurate | env grass | score 0 | objects 0");
  addBall(editor, 1, Vec3{0.0, 0.0, 0.0});
  exitPlace(editor);
  KIMIA_REQUIRE(editor.statsLine().find("ball fantasy") != std::string::npos);
  KIMIA_REQUIRE(editor.statsLine().find("objects 1") != std::string::npos);
  addPlayer(editor, 0, Vec3{0.0, 0.0, 0.0});
  exitPlace(editor);
  KIMIA_REQUIRE(editor.statsLine().find("player fast") != std::string::npos);
  editor.choose(2);  // محیط
  editor.choose(2);  // شب
  KIMIA_REQUIRE(editor.statsLine().find("env night") != std::string::npos);
}

KIMIA_TEST(world_ghost_moves_with_arrows_in_place) {
  WorldEditor editor = editorWithWorld();
  editor.choose(0);  // catalog
  editor.choose(2);  // block
  editor.choose(1);  // medium
  KIMIA_REQUIRE(editor.placing());
  KIMIA_REQUIRE(editor.holdPad().size() == 5U);  // arrows + ریز
  editor.setMoveInput(1.0, 0.0);
  editor.update(0.25);  // +0.5 units
  editor.setMoveInput(0.0, 0.0);
  KIMIA_REQUIRE(near3(editor.ghostPosition(), Vec3{0.5, 0.0, 0.0}, 1e-9));
  editor.setFineMove(true);
  editor.setMoveInput(1.0, 0.0);
  editor.update(1.0);  // +0.5 units (fine speed)
  editor.setMoveInput(0.0, 0.0);
  editor.setFineMove(false);
  KIMIA_REQUIRE(near3(editor.ghostPosition(), Vec3{1.0, 0.0, 0.0}, 1e-9));
  editor.choose(1);  // بازگشت without placing
  KIMIA_REQUIRE(editor.objectCount() == 0U);
}

KIMIA_TEST(world_quit_flag_from_main_menu) {
  WorldEditor editor;
  KIMIA_REQUIRE(!editor.quitRequested());
  editor.choose(2);  // quit
  KIMIA_REQUIRE(editor.quitRequested());
}

KIMIA_TEST(world_ball_rests_still_without_input) {
  WorldEditor editor = editorWithWorld();
  addBall(editor, 1, Vec3{0.0, 0.0, 0.0});  // fantasy: the bounciest ball
  exitPlace(editor);
  editor.choose(3);  // PLAY
  KIMIA_REQUIRE(editor.playing());
  for (i32 i = 0; i < 300; ++i) editor.update(1.0 / 60.0);  // 5 s untouched
  // The ball never moves by itself: it sits exactly at its spawn, at rest.
  KIMIA_REQUIRE(near3(editor.ballPosition(), Vec3{0.0, kWorldFantasyRadius, 0.0}, 1e-9));
  const Vec3 velocity = editor.ballVelocity();
  KIMIA_REQUIRE(velocity.x == 0.0);
  KIMIA_REQUIRE(velocity.y == 0.0);
  KIMIA_REQUIRE(velocity.z == 0.0);
}

KIMIA_TEST(world_still_player_deflects_rolling_ball) {
  WorldEditor editor = editorWithWorld();
  addBall(editor, 0, Vec3{3.0, 0.0, 0.0});  // accurate ball at x=3
  exitPlace(editor);
  editor.choose(3);  // PLAY
  KIMIA_REQUIRE(editor.playing());
  editor.setPlayerPosition(Vec3{0.0, 0.5, 0.0});
  editor.setBallPosition(Vec3{3.0, kGolfBallRadius, 0.0});
  editor.setBallVelocity(Vec3{-3.0, 0.0, 0.0});  // rolling at the idle player
  f64 minX = 3.0;
  for (i32 i = 0; i < 240; ++i) {  // 4 s
    editor.update(1.0 / 60.0);
    minX = std::min(minX, editor.ballPosition().x);
  }
  const f64 contact = kGolfBallRadius + kimia::kWorldPlayerRadius;
  // The ball bounced off the player instead of passing through...
  KIMIA_REQUIRE(minX >= contact - 1e-9);
  KIMIA_REQUIRE(editor.ballPosition().x > contact);
  // ...and it came to a stop (friction) somewhere on the player side.
  KIMIA_REQUIRE(editor.ballVelocity().x == 0.0);
}

KIMIA_TEST(world_ball_cannot_leave_floor) {
  WorldEditor editor = editorWithWorld();
  addBall(editor, 0, Vec3{0.0, 0.0, 0.0});
  exitPlace(editor);
  editor.choose(3);  // PLAY
  KIMIA_REQUIRE(editor.playing());
  editor.setPlayerPosition(Vec3{-9.0, 0.5, 0.0});  // far away: no contact
  editor.setBallPosition(Vec3{9.0, kGolfBallRadius, 0.0});
  editor.setBallVelocity(Vec3{10.0, 0.0, 0.0});  // blasted at the boundary
  for (i32 i = 0; i < 120; ++i) editor.update(1.0 / 60.0);  // 2 s
  const Vec3 position = editor.ballPosition();
  // The ball stops exactly at the floor edge, no outward speed left.
  KIMIA_REQUIRE(near(position.x, kimia::kWorldFloorHalf - kGolfBallRadius, 1e-9));
  KIMIA_REQUIRE(position.z == 0.0);
  KIMIA_REQUIRE(editor.ballVelocity().x == 0.0);
}

KIMIA_TEST(world_catalog_places_crate) {
  WorldEditor editor = editorWithWorld();
  editor.choose(0);  // catalog
  KIMIA_REQUIRE(editor.optionLabels().size() == 7U);  // ...جعبه joined the list
  editor.choose(5);  // جعبه
  KIMIA_REQUIRE(editor.placing());
  KIMIA_REQUIRE(editor.ghostKind() == kimia::ObjectKind::Crate);
  editor.setGhostPosition(Vec3{1.5, 0.0, -1.0});
  editor.choose(0);  // place
  exitPlace(editor);

  const EntityData* crate = editor.world().scene.get(editor.world().scene.find("Crate_1"));
  KIMIA_REQUIRE(crate != nullptr);
  KIMIA_REQUIRE(near3(crate->transform.position, Vec3{1.5, kimia::kWorldCrateSize * 0.5, -1.0}));
  KIMIA_REQUIRE(near3(crate->transform.scale, Vec3{kimia::kWorldCrateSize, kimia::kWorldCrateSize, kimia::kWorldCrateSize}));
  KIMIA_REQUIRE(editor.dynamicBoxCount() == 1U);  // simulated body
  KIMIA_REQUIRE(editor.physicsBoxCount() == 0U);  // not a static collider
  enterManage(editor);
  KIMIA_REQUIRE(editor.managedKindName() == "crate");

  // The crate serializes like any other entity (name-based) and reloads
  // with its dynamic body rebuilt.
  const std::string path = tmpPath("crate_world.kimia");
  std::string error;
  KIMIA_REQUIRE(editor.saveWorld(path, error));
  WorldEditor reloaded;
  KIMIA_REQUIRE(reloaded.loadWorld(path, error));
  KIMIA_REQUIRE(reloaded.world().scene.find("Crate_1") != kimia::kNullEntity);
  KIMIA_REQUIRE(reloaded.dynamicBoxCount() == 1U);
  KIMIA_REQUIRE(near3(reloaded.cratePosition("Crate_1"), Vec3{1.5, 0.5, -1.0}, 1e-9));
}

KIMIA_TEST(world_crate_kick_launches_crate) {
  WorldEditor editor = editorWithWorld();
  addCrate(editor, Vec3{2.0, 0.0, 0.0});
  exitPlace(editor);
  editor.choose(3);  // PLAY
  KIMIA_REQUIRE(editor.playing());
  editor.setPlayerPosition(Vec3{2.0, 0.5, 0.9});
  editor.setMoveInput(0.0, -1.0);  // walk toward the crate (-Z)
  editor.update(1.0 / 60.0);
  // Kick: direction * (kWorldKickBase + speed * kWorldKickSpeedScale) *
  // kWorldCrateKickScale + (0, kWorldCrateKickUp, 0). Normal player (4 m/s):
  // kickSpeed = 2 + 4 * 0.5 = 4 -> crate velocity = (0, 0.8, -2.4).
  KIMIA_REQUIRE(near3(editor.crateVelocity("Crate_1"), Vec3{0.0, kWorldCrateKickUp, -4.0 * kWorldCrateKickScale}, 1e-6));
  // The kick only touches the crate: the ball still waits at its spawn.
  KIMIA_REQUIRE(near3(editor.ballPosition(), Vec3{0.0, kGolfBallRadius, 0.0}, 1e-9));
  for (int i = 0; i < 30; ++i) editor.update(1.0 / 60.0);  // half a second
  KIMIA_REQUIRE(editor.cratePosition("Crate_1").z < -2.0);  // it did slide away
  // Back in the builder the crate snaps to its placed spot.
  editor.backToMenu();
  KIMIA_REQUIRE(near3(editor.cratePosition("Crate_1"), Vec3{2.0, 0.5, 0.0}, 1e-9));
}

KIMIA_TEST(world_crate_and_ball_collide_in_play) {
  WorldEditor editor = editorWithWorld();
  addCrate(editor, Vec3{3.0, 0.0, 0.0});
  exitPlace(editor);
  editor.choose(3);  // PLAY
  editor.setBallPosition(Vec3{2.2, kGolfBallRadius, 0.0});
  editor.setBallVelocity(Vec3{2.0, 0.0, 0.0});
  bool crateMoved = false;
  for (int i = 0; i < 300 && !crateMoved; ++i) {
    editor.update(1.0 / 60.0);
    // A 0.4 kg ball at ~1.9 m/s shoves the 1 kg crate ~4 cm before the
    // crate's friction stops it (measured: rest at x = 3.0378).
    crateMoved = editor.cratePosition("Crate_1").x > 3.03;
  }
  KIMIA_REQUIRE(crateMoved);  // the ball shoved the crate
  // Ball (0.4 kg) vs crate (1.0 kg), e = 0.4: the ball transfers all its
  // motion and stops at the crate face.
  KIMIA_REQUIRE(editor.ballVelocity().x < 0.05);
  KIMIA_REQUIRE(editor.ballPosition().x < 2.4);
}
