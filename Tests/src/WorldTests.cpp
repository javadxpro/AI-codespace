#include <kimia_test.h>
#include <kimia/AssetPipeline.h>
#include <kimia/GameProfile.h>
#include <kimia/Golf.h>
#include <kimia/Physics.h>
#include <kimia/World.h>
#include <kimia/WorldIO.h>

#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstring>
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

// Index of the sandbox game («زمین آزاد») in the profile menu.
kimia::usize sandboxIndex(const WorldEditor& editor) {
  for (kimia::usize i = 0; i < editor.profileCount(); ++i) {
    if (editor.profileAt(i).name == "sandbox") return i;
  }
  return 0U;
}

// Walks the editor into the builder with a fresh empty SANDBOX world (the
// 20 x 20 floor and the golf-tuned ball every test below was written for).
WorldEditor editorWithWorld() {
  WorldEditor editor;
  editor.choose(0);  // Main -> «کدام بازی؟»
  editor.choose(static_cast<i32>(sandboxIndex(editor)));  // زمین آزاد -> Builder
  return editor;
}

// Picks the game with this profile name from the «کدام بازی؟» screen.
void createWorldFor(WorldEditor& editor, const char* profileName) {
  editor.choose(0);  // Main -> «کدام بازی؟»
  for (kimia::usize i = 0; i < editor.profileCount(); ++i) {
    if (editor.profileAt(i).name == profileName) {
      editor.choose(static_cast<i32>(i));
      return;
    }
  }
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
  // «دنیای جدید» asks which game first: the 4 built-ins + «بازگشت».
  KIMIA_REQUIRE(!editor.hasWorld());
  KIMIA_REQUIRE(editor.choosingProfile());
  KIMIA_REQUIRE(editor.menuTitle() == "دنیای جدید: کدام بازی؟");
  KIMIA_REQUIRE(editor.optionLabels().size() == 5U);
  KIMIA_REQUIRE(editor.optionLabels()[0] == "فوتبال خیابونی ایران: کوی ابوذر");
  KIMIA_REQUIRE(editor.optionLabels()[3] == "زمین آزاد");
  editor.choose(3);  // زمین آزاد
  KIMIA_REQUIRE(editor.hasWorld());
  KIMIA_REQUIRE(editor.world().name == "MyWorld");
  KIMIA_REQUIRE(editor.profile().name == "sandbox");
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

KIMIA_TEST(world_manage_hierarchy_inspector_move_color_delete) {
  WorldEditor editor = editorWithWorld();
  addBlock(editor, 1, Vec3{2.0, 0.0, 0.0});
  exitPlace(editor);
  addGoal(editor, 0, Vec3{0.0, 0.0, -5.0});
  exitPlace(editor);
  KIMIA_REQUIRE(editor.objectCount() == 2U);

  // Hierarchy: the manage screen lists every object by name.
  enterManage(editor);
  KIMIA_REQUIRE(editor.managedCount() == 2U);
  KIMIA_REQUIRE(editor.optionLabels().size() == 3U);  // Block_1, Goal_1, بازگشت
  KIMIA_REQUIRE(editor.optionLabels()[0] == "Block_1");
  KIMIA_REQUIRE(editor.optionLabels()[1] == "Goal_1");

  // Pick Block_1 -> Inspector with live values in the title.
  editor.choose(0);
  KIMIA_REQUIRE(editor.managedName() == "Block_1");
  const std::string title = editor.menuTitle();
  KIMIA_REQUIRE(title.find("Block_1") != std::string::npos);
  KIMIA_REQUIRE(title.find("x 2.00") != std::string::npos);
  KIMIA_REQUIRE(editor.optionLabels().size() == 5U);  // X+/X-/Z+/Z-/بیشتر

  // Precise numeric nudges (0.1 per tap).
  editor.choose(0);  // X +0.1
  editor.choose(1);  // X -0.1 (back to 2.0)
  editor.choose(2);  // Z +0.1
  const EntityData* block = editor.world().scene.get(editor.world().scene.find("Block_1"));
  KIMIA_REQUIRE(block != nullptr);
  KIMIA_REQUIRE(near3(block->transform.position, Vec3{2.0, 0.5, 0.1}));

  // Page 2: height and scale.
  editor.choose(4);  // بیشتر…
  KIMIA_REQUIRE(editor.optionLabels()[0] == "بالا +۰٫۱");
  editor.choose(0);  // بالا +0.1
  editor.choose(2);  // بزرگتر +0.1
  KIMIA_REQUIRE(near3(block->transform.position, Vec3{2.0, 0.6, 0.1}));
  KIMIA_REQUIRE(near3(block->transform.scale, Vec3{1.1, 1.1, 1.1}));
  editor.choose(3);  // کوچکتر -0.1
  KIMIA_REQUIRE(near3(block->transform.scale, Vec3{1.0, 1.0, 1.0}));

  // Page 3: free drag with the arrows (2 units/s), color, delete.
  editor.choose(4);  // بیشتر…
  editor.choose(0);  // جابه‌جایی با جهت
  KIMIA_REQUIRE(editor.optionLabels().size() == 1U);  // پایان
  editor.setMoveInput(1.0, 0.0);
  editor.update(0.5);  // +1.0 on x
  editor.setMoveInput(0.0, 0.0);
  editor.choose(0);  // پایان -> back to the inspector
  KIMIA_REQUIRE(editor.managedName() == "Block_1");
  KIMIA_REQUIRE(near3(block->transform.position, Vec3{3.0, 0.6, 0.1}));

  // Color: the flow returns to the inspector afterwards.
  editor.choose(1);  // رنگ (still on page 3)
  editor.choose(0);  // قرمز
  KIMIA_REQUIRE(near3(block->color, Vec3{0.85, 0.15, 0.15}));
  KIMIA_REQUIRE(editor.managedName() == "Block_1");

  // Delete: confirm yes -> back on the hierarchy list.
  editor.choose(2);  // حذف (still on page 3)
  KIMIA_REQUIRE(editor.optionLabels().size() == 2U);  // بله/نه
  editor.choose(0);  // بله
  KIMIA_REQUIRE(editor.objectCount() == 1U);
  KIMIA_REQUIRE(editor.world().scene.find("Block_1") == kimia::kNullEntity);
  KIMIA_REQUIRE(editor.optionLabels().size() == 2U);  // Goal_1 + بازگشت
  editor.choose(1);  // بازگشت
  KIMIA_REQUIRE(editor.optionLabels().size() == 6U);  // back on the builder
}

KIMIA_TEST(world_hierarchy_pages_with_more_than_five_objects) {
  WorldEditor editor = editorWithWorld();
  for (i32 i = 0; i < 6; ++i) {
    addBlock(editor, 1, Vec3{static_cast<f64>(i), 0.0, 0.0});
    exitPlace(editor);
  }
  KIMIA_REQUIRE(editor.objectCount() == 6U);
  enterManage(editor);
  KIMIA_REQUIRE(editor.managedCount() == 6U);
  // Page 1: Block_1..Block_5 + «بیشتر…».
  KIMIA_REQUIRE(editor.optionLabels().size() == 6U);
  KIMIA_REQUIRE(editor.optionLabels()[0] == "Block_1");
  KIMIA_REQUIRE(editor.optionLabels()[5] == "بیشتر…");
  editor.choose(5);  // page 2
  // Page 2: Block_6 + «بازگشت».
  KIMIA_REQUIRE(editor.optionLabels().size() == 2U);
  KIMIA_REQUIRE(editor.optionLabels()[0] == "Block_6");
  KIMIA_REQUIRE(editor.optionLabels()[1] == "بازگشت");
  editor.choose(1);  // بازگشت -> builder
  KIMIA_REQUIRE(editor.optionLabels().size() == 6U);
}

KIMIA_TEST(world_inspector_edits_survive_save_and_load) {
  WorldEditor editor = editorWithWorld();
  addBlock(editor, 1, Vec3{2.0, 0.0, 0.0});
  exitPlace(editor);
  enterManage(editor);
  editor.choose(0);       // Block_1 -> inspector
  editor.choose(0);       // X +0.1
  editor.choose(4);       // page 2
  editor.choose(0);       // بالا +0.1
  editor.choose(2);       // بزرگتر +0.1
  const EntityData* block = editor.world().scene.get(editor.world().scene.find("Block_1"));
  KIMIA_REQUIRE(block != nullptr);
  KIMIA_REQUIRE(near3(block->transform.position, Vec3{2.1, 0.6, 0.0}));
  KIMIA_REQUIRE(near3(block->transform.scale, Vec3{1.1, 1.1, 1.1}));

  const std::string path = tmpPath("inspector_world.kimia");
  std::string error;
  KIMIA_REQUIRE(editor.saveWorld(path, error));
  WorldEditor reloaded;
  KIMIA_REQUIRE(reloaded.loadWorld(path, error));
  const EntityData* again = reloaded.world().scene.get(reloaded.world().scene.find("Block_1"));
  KIMIA_REQUIRE(again != nullptr);
  KIMIA_REQUIRE(near3(again->transform.position, Vec3{2.1, 0.6, 0.0}));
  KIMIA_REQUIRE(near3(again->transform.scale, Vec3{1.1, 1.1, 1.1}));
  KIMIA_REQUIRE(reloaded.physicsBoxCount() == 1U);  // collider rebuilt from the edit
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
  KIMIA_REQUIRE(reloaded.profile().name == "sandbox");
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
  // No profile lines and no "Ground" plane: the sandbox game, 20 x 20.
  KIMIA_REQUIRE(loaded.profile.name == "sandbox");
  KIMIA_REQUIRE(near(loaded.halfLength(), 10.0));
  KIMIA_REQUIRE(near(loaded.halfWidth(), 10.0));

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
  KIMIA_REQUIRE(editor.tapPad().size() == 3U);   // jump + reset + menu
  KIMIA_REQUIRE(editor.tapPad()[0].second == "j");
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

KIMIA_TEST(world_play_player_jumps_and_lands) {
  WorldEditor editor = editorWithWorld();
  editor.choose(3);  // PLAY
  KIMIA_REQUIRE(editor.playing());
  KIMIA_REQUIRE(near(editor.playerPosition().y, 0.5));  // standing on the floor
  editor.jumpPressed();
  f64 maxY = 0.0;
  for (i32 i = 0; i < 180; ++i) {  // 3 s: up and back down
    editor.update(1.0 / 60.0);
    maxY = std::max(maxY, editor.playerPosition().y);
  }
  // Feet apex ~1.18 (discrete) -> center apex ~1.68.
  KIMIA_REQUIRE(maxY > 1.5);
  KIMIA_REQUIRE(maxY < 1.9);
  KIMIA_REQUIRE(near(editor.playerPosition().y, 0.5, 1e-9));  // landed back
  KIMIA_REQUIRE(editor.physicsCharacterOnGround());
}

KIMIA_TEST(world_play_player_cannot_walk_through_blocks) {
  WorldEditor editor = editorWithWorld();
  addBlock(editor, 1, Vec3{2.0, 0.0, 0.0});  // medium 1x1x1 at (2, 0.5, 0)
  exitPlace(editor);
  editor.choose(3);  // PLAY
  editor.setPlayerPosition(Vec3{0.0, 0.5, 0.0});
  editor.setMoveInput(1.0, 0.0);
  for (i32 i = 0; i < 120; ++i) editor.update(1.0 / 60.0);  // 2 s at 4 m/s = 8 m
  const Vec3 player = editor.playerPosition();
  KIMIA_REQUIRE(near(player.x, 2.0 - 0.5 - 0.3, 1e-6));  // stopped at the block face
  KIMIA_REQUIRE(near(player.y, 0.5, 1e-6));              // still standing
  KIMIA_REQUIRE(near(player.z, 0.0, 1e-6));
}

KIMIA_TEST(world_play_player_jumps_onto_a_block) {
  WorldEditor editor = editorWithWorld();
  addBlock(editor, 1, Vec3{1.0, 0.0, 0.0});  // medium block, top face at y = 1.0
  exitPlace(editor);
  editor.choose(3);  // PLAY
  editor.setPlayerPosition(Vec3{0.1, 0.5, 0.0});  // in front of the west face
  editor.jumpPressed();
  editor.update(1.0 / 60.0);  // land first: the jump buffers until grounded
  editor.update(1.0 / 60.0);  // the jump fires here (feet leave the floor)
  // Rise at the face: with dt = 1/60 and v = sqrt(2 g h), the feet clear
  // the 1.0 m top at step 19 of the flight.
  for (i32 i = 0; i < 18; ++i) editor.update(1.0 / 60.0);
  KIMIA_REQUIRE(editor.playerPosition().y > 1.5);  // feet above the top face
  // Glide over the top: 12 steps at 4 m/s = 0.8 m -> center x ~ 0.9.
  editor.setMoveInput(1.0, 0.0);
  for (i32 i = 0; i < 12; ++i) editor.update(1.0 / 60.0);
  editor.setMoveInput(0.0, 0.0);
  // Drop onto the top face and settle.
  for (i32 i = 0; i < 120; ++i) editor.update(1.0 / 60.0);
  const Vec3 player = editor.playerPosition();
  KIMIA_REQUIRE(near(player.y, 1.5, 1e-6));  // standing ON the block (feet at 1.0)
  KIMIA_REQUIRE(player.x > 0.5 && player.x < 1.5);  // over the top face
  KIMIA_REQUIRE(editor.physicsCharacterOnGround());
}

KIMIA_TEST(world_stats_line_reports_config) {
  WorldEditor editor = editorWithWorld();
  KIMIA_REQUIRE(editor.statsLine() ==
                "KIMIA WORLD | BUILDER | world MyWorld | game sandbox | player normal | ball accurate | env grass | "
                "score 0 | objects 0");
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
  KIMIA_REQUIRE(editor.optionLabels().size() == 8U);  // ...جعبه و «مدل از فایل» joined the list
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
  // The character controller now stops the player at the crate's face (the
  // player can no longer walk through crates), so the shoved crate rides
  // just ahead of the player instead of racing ahead alone.
  KIMIA_REQUIRE(editor.cratePosition("Crate_1").z < -1.9);  // it did slide away
  KIMIA_REQUIRE(editor.cratePosition("Crate_1").z < editor.playerPosition().z - 0.8);
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

KIMIA_TEST(world_catalog_places_model_from_file) {
  // The user drops an OBJ into the import directory; the catalog lists it
  // and placing it creates a Model_* entity that survives save/load.
  const std::string importsDir = std::string(KIMIA_TEST_TMP) + "/imports";
  static_cast<void>(::mkdir(importsDir.c_str(), 0755));
  const std::string objPath = importsDir + "/mini.obj";
  {
    std::FILE* file = std::fopen(objPath.c_str(), "wb");
    KIMIA_REQUIRE(file != nullptr);
    const char* obj = "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nf 1 2 3\nf 1 3 4\n";
    std::fwrite(obj, 1U, std::strlen(obj), file);
    std::fclose(file);
  }
  WorldEditor editor = editorWithWorld();
  editor.setImportDirectory(importsDir);
  editor.choose(0);  // catalog
  editor.choose(6);  // مدل از فایل
  KIMIA_REQUIRE(editor.importFileCount() == 1U);
  KIMIA_REQUIRE(editor.optionLabels().size() == 2U);
  KIMIA_REQUIRE(editor.optionLabels()[0] == "mini.obj");
  KIMIA_REQUIRE(editor.optionLabels()[1] == "بازگشت");
  editor.choose(0);  // mini.obj
  KIMIA_REQUIRE(editor.optionLabels().size() == 4U);  // size question
  editor.choose(1);  // متوسط (1.0)
  KIMIA_REQUIRE(editor.placing());
  KIMIA_REQUIRE(editor.ghostKind() == kimia::ObjectKind::Model);
  editor.setGhostPosition(Vec3{2.0, 0.0, 1.0});
  editor.choose(0);  // place
  exitPlace(editor);
  KIMIA_REQUIRE(editor.objectCount() == 1U);
  const EntityData* model = editor.world().scene.get(editor.world().scene.find("Model_1"));
  KIMIA_REQUIRE(model != nullptr);
  KIMIA_REQUIRE(model->meshFile == importsDir + "/mini.obj");
  KIMIA_REQUIRE(near3(model->transform.position, Vec3{2.0, 0.0, 1.0}));
  KIMIA_REQUIRE(near3(model->transform.scale, Vec3{1.0, 1.0, 1.0}));
  KIMIA_REQUIRE(editor.dynamicBoxCount() == 0U);  // no collider bodies
  KIMIA_REQUIRE(editor.physicsBoxCount() == 0U);
  enterManage(editor);
  KIMIA_REQUIRE(editor.managedKindName() == "model");
  // Save/load round-trip keeps the mesh file reference.
  const std::string path = tmpPath("model_world.kimia");
  std::string error;
  KIMIA_REQUIRE(editor.saveWorld(path, error));
  WorldEditor reloaded;
  KIMIA_REQUIRE(reloaded.loadWorld(path, error));
  const EntityData* again = reloaded.world().scene.get(reloaded.world().scene.find("Model_1"));
  KIMIA_REQUIRE(again != nullptr);
  KIMIA_REQUIRE(again->meshFile == importsDir + "/mini.obj");
}

KIMIA_TEST(world_import_file_list_pages_and_empty_dir) {
  // 7 files -> one page of 5 + «بیشتر…», then 2 + «بازگشت»; empty -> just
  // «بازگشت» which returns to the catalog.
  const std::string importsDir = std::string(KIMIA_TEST_TMP) + "/imports_many";
  static_cast<void>(::mkdir(importsDir.c_str(), 0755));
  for (int i = 1; i <= 7; ++i) {
    const std::string path = importsDir + "/model" + std::to_string(i) + ".obj";
    std::FILE* file = std::fopen(path.c_str(), "wb");
    KIMIA_REQUIRE(file != nullptr);
    std::fwrite("v 0 0 0\n", 1U, 8U, file);
    std::fclose(file);
  }
  WorldEditor editor = editorWithWorld();
  editor.setImportDirectory(importsDir);
  editor.choose(0);  // catalog
  editor.choose(6);  // مدل از فایل
  KIMIA_REQUIRE(editor.importFileCount() == 7U);
  KIMIA_REQUIRE(editor.optionLabels().size() == 6U);  // 5 files + بیشتر
  KIMIA_REQUIRE(editor.optionLabels()[5] == "بیشتر…");
  editor.choose(5);  // next page
  KIMIA_REQUIRE(editor.optionLabels().size() == 3U);  // 2 files + بازگشت
  KIMIA_REQUIRE(editor.optionLabels()[2] == "بازگشت");
  editor.choose(2);  // back to the catalog
  KIMIA_REQUIRE(editor.optionLabels().size() == 8U);

  // Empty directory: the list screen shows only «بازگشت».
  const std::string emptyDir = std::string(KIMIA_TEST_TMP) + "/imports_empty";
  static_cast<void>(::mkdir(emptyDir.c_str(), 0755));
  editor.setImportDirectory(emptyDir);
  editor.choose(6);  // مدل از فایل (we are already in the catalog)
  KIMIA_REQUIRE(editor.importFileCount() == 0U);
  KIMIA_REQUIRE(editor.optionLabels().size() == 1U);
  KIMIA_REQUIRE(editor.optionLabels()[0] == "بازگشت");
  editor.choose(0);
  KIMIA_REQUIRE(editor.optionLabels().size() == 8U);  // catalog again
}

KIMIA_TEST(world_ball_spawns_above_overlapping_objects) {
  // A crate placed right on the ball spawn used to make the crate float on
  // the ball (or the ball sink when a block covered the spawn). The ball now
  // spawns on top of the overlapping collider and settles there.
  WorldEditor editor = editorWithWorld();
  addCrate(editor, Vec3{0.0, 0.0, 0.0});  // exactly on the ball spawn
  exitPlace(editor);
  editor.choose(3);  // PLAY
  KIMIA_REQUIRE(editor.playing());
  KIMIA_REQUIRE(editor.ballPosition().y > 1.0);  // raised above the crate
  for (int i = 0; i < 120; ++i) editor.update(1.0 / 60.0);  // 2 s
  // Rests on the crate top (y = 1.0) + radius, dead center — no drift.
  KIMIA_REQUIRE(near(editor.ballPosition().y, 1.12, 1e-3));
  KIMIA_REQUIRE(near(editor.ballPosition().x, 0.0, 1e-6));
  KIMIA_REQUIRE(near(editor.ballPosition().z, 0.0, 1e-6));
  KIMIA_REQUIRE(editor.ballVelocity().length() < 1e-3);
  editor.backToMenu();
  // A block on the spawn: the ball pops out on top of the block instead of
  // sinking to the floor inside it.
  addBlock(editor, 1, Vec3{0.0, 0.0, 0.0});  // medium block at the spawn
  exitPlace(editor);
  editor.choose(3);  // PLAY
  KIMIA_REQUIRE(editor.ballPosition().y > 1.0);
  for (int i = 0; i < 120; ++i) editor.update(1.0 / 60.0);
  KIMIA_REQUIRE(near(editor.ballPosition().y, 1.12, 1e-3));  // on the block top
}

KIMIA_TEST(world_model_placement_fits_mesh_to_chosen_size) {
  // The spider model is ~193 units in its file; placing it at «متوسط»
  // must fit it into a 1-unit cube (Unity-style import normalization).
  WorldEditor editor = editorWithWorld();
  editor.setImportDirectory(std::string(KIMIA_ASSET_DIR));
  editor.choose(0);  // catalog
  editor.choose(6);  // مدل از فایل
  // Page 1 shows the first 5 OBJ/FBX files; spider.obj is on page 2.
  editor.choose(5);  // بیشتر…
  kimia::usize spiderIndex = editor.importFileCount();  // sentinel, replaced below
  for (kimia::usize i = 0; i < editor.importFileCount(); ++i) {
    if (editor.importFileAt(i) == "spider.obj") spiderIndex = i;
  }
  // spider.obj is listed on page 2: index within the page.
  const kimia::usize pageBegin = 5U;
  KIMIA_REQUIRE(spiderIndex >= pageBegin);
  editor.choose(static_cast<i32>(spiderIndex - pageBegin));
  editor.choose(1);  // متوسط (1.0)
  editor.setGhostPosition(Vec3{3.0, 0.0, 0.0});
  editor.choose(0);  // place
  exitPlace(editor);
  const EntityData* model = editor.world().scene.get(editor.world().scene.find("Model_1"));
  KIMIA_REQUIRE(model != nullptr);
  // The stored scale must be the fit factor = 1 / largest bbox dimension.
  std::string error;
  auto loaded = kimia::assets::loadMesh(model->meshFile, error);
  KIMIA_REQUIRE(loaded.has_value());
  Vec3 lo = loaded->mesh.positions[0];
  Vec3 hi = loaded->mesh.positions[0];
  for (const Vec3& p : loaded->mesh.positions) {
    lo.x = std::min(lo.x, p.x);
    lo.y = std::min(lo.y, p.y);
    lo.z = std::min(lo.z, p.z);
    hi.x = std::max(hi.x, p.x);
    hi.y = std::max(hi.y, p.y);
    hi.z = std::max(hi.z, p.z);
  }
  const f64 size = std::max(hi.x - lo.x, std::max(hi.y - lo.y, hi.z - lo.z));
  KIMIA_REQUIRE(size > 100.0);  // the spider really is huge
  const f64 fit = 1.0 / size;
  KIMIA_REQUIRE(near(model->transform.scale.x, fit, 1e-12));
  KIMIA_REQUIRE(near(model->transform.scale.y, fit, 1e-12));
  KIMIA_REQUIRE(near(model->transform.scale.z, fit, 1e-12));
  KIMIA_REQUIRE(model->transform.scale.x < 0.01);
}

// --- Game profiles: the same engine, a different game per profile ---

KIMIA_TEST(world_street_profile_builds_its_court) {
  WorldEditor editor;
  createWorldFor(editor, "street");
  KIMIA_REQUIRE(editor.hasWorld());
  KIMIA_REQUIRE(editor.profile().name == "street");
  KIMIA_REQUIRE(editor.menuTitle() == "MyWorld (فوتبال خیابونی ایران: کوی ابوذر) — سازنده");
  // The ground IS the field: 5 wide (X), 16 long (Z), asphalt.
  const EntityData* ground = editor.world().scene.get(editor.world().scene.find("Ground"));
  KIMIA_REQUIRE(ground != nullptr);
  KIMIA_REQUIRE(near3(ground->transform.scale, Vec3{5.0, 1.0, 16.0}));
  KIMIA_REQUIRE(near3(ground->color, kimia::environmentColors(EnvironmentKind::Asphalt).floor));
  KIMIA_REQUIRE(editor.world().environment == EnvironmentKind::Asphalt);
  // Profile defaults are the world's answers until the user changes them.
  KIMIA_REQUIRE(near(editor.world().player.speed, 5.0));
  KIMIA_REQUIRE(editor.world().ball.type == BallType::Fantasy);
  KIMIA_REQUIRE(near(editor.world().ball.radius, kWorldFantasyRadius));
  KIMIA_REQUIRE(editor.statsLine().find("game street") != std::string::npos);
  KIMIA_REQUIRE(editor.statsLine().find("ball fantasy") != std::string::npos);
}

KIMIA_TEST(world_street_profile_skips_the_ball_question) {
  WorldEditor editor;
  createWorldFor(editor, "street");
  editor.choose(0);  // catalog
  editor.choose(1);  // توپ — street has one ball: straight to placement
  KIMIA_REQUIRE(editor.placing());
  KIMIA_REQUIRE(editor.ghostKind() == kimia::ObjectKind::Ball);
  editor.setGhostPosition(Vec3{0.0, 0.0, 2.0});
  editor.choose(0);  // place
  exitPlace(editor);
  const EntityData* ball = editor.world().scene.get(editor.world().scene.find("Ball"));
  KIMIA_REQUIRE(ball != nullptr);
  KIMIA_REQUIRE(editor.world().ball.type == BallType::Fantasy);
  KIMIA_REQUIRE(near3(ball->transform.position, Vec3{0.0, kWorldFantasyRadius, 2.0}));
  KIMIA_REQUIRE(editor.objectCount() == 1U);

  // The sandbox still asks «دقیق باشه یا فانتزی؟».
  WorldEditor sandbox = editorWithWorld();
  sandbox.choose(0);
  sandbox.choose(1);
  KIMIA_REQUIRE(!sandbox.placing());
  KIMIA_REQUIRE(sandbox.menuTitle() == "توپ: دقیق باشه یا فانتزی؟");
}

KIMIA_TEST(world_profile_field_bounds_player_ball_and_ghost) {
  WorldEditor editor;
  createWorldFor(editor, "street");  // 5 wide: half width 2.5
  // Ghost: clamped 0.5 inside the edge on X, free along the 16 m length.
  editor.choose(0);
  editor.choose(2);  // block
  editor.choose(1);  // medium
  editor.setMoveInput(1.0, 0.0);
  editor.update(5.0);  // would travel 10 units
  KIMIA_REQUIRE(near(editor.ghostPosition().x, 2.0, 1e-9));
  editor.setMoveInput(0.0, 1.0);
  editor.update(2.0);  // +4 along Z is still inside (half length 8)
  KIMIA_REQUIRE(near(editor.ghostPosition().z, 4.0, 1e-9));
  editor.setMoveInput(0.0, 0.0);
  editor.choose(1);  // back
  editor.choose(0);
  editor.choose(1);  // ball (no question)
  editor.setGhostPosition(Vec3{0.0, 0.0, 0.0});
  editor.choose(0);
  exitPlace(editor);

  editor.choose(3);  // PLAY
  KIMIA_REQUIRE(editor.playing());
  editor.setPlayerPosition(Vec3{0.0, 0.5, -6.0});  // far from the ball
  editor.setMoveInput(1.0, 0.0);
  for (i32 i = 0; i < 300; ++i) editor.update(1.0 / 60.0);  // 5 s toward +X
  editor.setMoveInput(0.0, 0.0);
  KIMIA_REQUIRE(near(editor.playerPosition().x, 2.5 - 0.6, 1e-9));  // half width - player margin

  editor.setBallPosition(Vec3{0.0, kWorldFantasyRadius, 0.0});
  editor.setBallVelocity(Vec3{10.0, 0.0, 0.0});
  for (i32 i = 0; i < 120; ++i) editor.update(1.0 / 60.0);
  KIMIA_REQUIRE(near(editor.ballPosition().x, 2.5 - kWorldFantasyRadius, 1e-9));
  KIMIA_REQUIRE(editor.ballVelocity().x == 0.0);
}

KIMIA_TEST(world_profile_kick_and_jump_tuning_apply_in_play) {
  WorldEditor editor;
  createWorldFor(editor, "street");
  editor.choose(0);
  editor.choose(1);  // ball
  editor.setGhostPosition(Vec3{0.0, 0.0, 0.0});
  editor.choose(0);
  exitPlace(editor);
  editor.choose(3);  // PLAY
  editor.setPlayerPosition(Vec3{0.0, 0.5, 0.0});
  editor.setBallPosition(Vec3{0.6, kWorldFantasyRadius, 0.0});
  editor.setMoveInput(1.0, 0.0);
  editor.update(0.0);
  const Vec3 velocity = editor.ballVelocity();
  // street: kick 3.0 + 5.0 * 0.6 = 6.0, pop 2.0 (sandbox: 2 + 4*0.5 = 4, pop 1.2)
  KIMIA_REQUIRE(near(velocity.x, 3.0 + 5.0 * 0.6));
  KIMIA_REQUIRE(near(velocity.y, 2.0));
  editor.setMoveInput(0.0, 0.0);

  // Jump apex from the profile: feet 1.8 -> center ~2.3 (discrete ~2.27).
  editor.setPlayerPosition(Vec3{0.0, 0.5, -5.0});
  editor.jumpPressed();
  f64 maxY = 0.0;
  for (i32 i = 0; i < 180; ++i) {
    editor.update(1.0 / 60.0);
    maxY = std::max(maxY, editor.playerPosition().y);
  }
  KIMIA_REQUIRE(maxY > 2.1);
  KIMIA_REQUIRE(maxY < 2.4);
  KIMIA_REQUIRE(near(editor.playerPosition().y, 0.5, 1e-9));
}

KIMIA_TEST(world_file_carries_its_profile_and_field) {
  WorldEditor editor;
  createWorldFor(editor, "grass");  // 40 x 25
  editor.choose(0);
  editor.choose(2);  // block
  editor.choose(2);  // large
  editor.setGhostPosition(Vec3{10.0, 0.0, -15.0});  // outside a 20 x 20 sandbox, inside the pitch
  editor.choose(0);
  exitPlace(editor);
  const std::string path = tmpPath("grass_world.kimia");
  std::string error;
  KIMIA_REQUIRE(editor.saveWorld(path, error));

  // A fresh editor with NO grass profile available still plays this world
  // as grass football: the file is self-contained.
  WorldEditor reloaded;
  reloaded.setProfileDirectory(tmpPath("no_profiles_here"));
  KIMIA_REQUIRE(reloaded.loadWorld(path, error));
  KIMIA_REQUIRE(reloaded.profile().name == "grass");
  KIMIA_REQUIRE(reloaded.profile().title == "زمین چمن: کوی ابوذر");
  KIMIA_REQUIRE(near(reloaded.world().halfLength(), 20.0));
  KIMIA_REQUIRE(near(reloaded.world().halfWidth(), 12.5));
  KIMIA_REQUIRE(!reloaded.profile().ballChoice);
  KIMIA_REQUIRE(near(reloaded.profile().kickBase, 4.0));
  const EntityData* block = reloaded.world().scene.get(reloaded.world().scene.find("Block_1"));
  KIMIA_REQUIRE(block != nullptr);
  KIMIA_REQUIRE(near3(block->transform.position, Vec3{10.0, 1.0, -15.0}));

  // Byte-stable: save -> load -> save is identical.
  std::string first;
  std::string second;
  KIMIA_REQUIRE(WorldIO::save(editor.world(), first));
  KIMIA_REQUIRE(WorldIO::save(reloaded.world(), second));
  KIMIA_REQUIRE(first == second);
  KIMIA_REQUIRE(first.find("# profile name grass\n") != std::string::npos);
  KIMIA_REQUIRE(first.find("# profile field 40.000000 25.000000\n") != std::string::npos);
}

KIMIA_TEST(world_old_file_takes_field_from_its_ground) {
  // A stage-17 file: no profile lines, a 20 x 20 ground -> sandbox 20 x 20.
  const std::string text =
      "# KIMIA scene v1\n"
      "# world name Old\n"
      "e \"Ground\" mesh plane pos 0 0 0 scale 12 1 30 color 0.22 0.45 0.24 rough 0.95\n";
  WorldData loaded;
  std::string error;
  KIMIA_REQUIRE(WorldIO::load(text, loaded, error));
  KIMIA_REQUIRE(loaded.profile.name == "sandbox");
  KIMIA_REQUIRE(near(loaded.profile.fieldWidth, 12.0));
  KIMIA_REQUIRE(near(loaded.profile.fieldLength, 30.0));
  // The ball type / env lines of old files still win over the profile defaults.
  const std::string fantasy = text + "# ball type fantasy\n# env night\n";
  KIMIA_REQUIRE(WorldIO::load(fantasy, loaded, error));
  KIMIA_REQUIRE(loaded.ball.type == BallType::Fantasy);
  KIMIA_REQUIRE(loaded.environment == EnvironmentKind::Night);
}

KIMIA_TEST(world_profile_menu_pages_and_user_profiles) {
  // Seven user profiles + 4 built-ins = 11 games -> 3 pages of 5.
  const std::string dir = tmpPath("many_profiles");
  const int created = ::mkdir(dir.c_str(), 0755);
  static_cast<void>(created == 0 || errno == EEXIST);
  if (DIR* handle = ::opendir(dir.c_str())) {  // start empty even after a previous run
    while (dirent* entry = ::readdir(handle)) {
      const std::string file = entry->d_name;
      if (file != "." && file != "..") std::remove((dir + "/" + file).c_str());
    }
    ::closedir(handle);
  }
  for (int i = 0; i < 7; ++i) {
    kimia::GameProfile custom;
    custom.name = "custom" + std::to_string(i);
    custom.title = "بازی سفارشی " + std::to_string(i);
    custom.fieldLength = 10.0 + static_cast<f64>(i);
    std::string error;
    KIMIA_REQUIRE(kimia::ProfileIO::saveToFile(custom, dir + "/custom" + std::to_string(i) + ".kimiaprofile", error));
  }
  // A user file that RETUNES a built-in (same name) replaces it in place.
  {
    kimia::GameProfile street;
    street.name = "street";
    street.title = "خیابونی من";
    street.fieldLength = 12.0;
    street.fieldWidth = 6.0;
    std::string error;
    KIMIA_REQUIRE(kimia::ProfileIO::saveToFile(street, dir + "/my_street.kimiaprofile", error));
  }
  WorldEditor editor;
  editor.setProfileDirectory(dir);
  KIMIA_REQUIRE(editor.profileCount() == 11U);
  KIMIA_REQUIRE(editor.profileAt(0).name == "street");  // still first
  KIMIA_REQUIRE(editor.profileAt(0).title == "خیابونی من");
  KIMIA_REQUIRE(near(editor.profileAt(0).fieldWidth, 6.0));
  editor.choose(0);  // دنیای جدید
  KIMIA_REQUIRE(editor.choosingProfile());
  KIMIA_REQUIRE(editor.optionLabels().size() == 6U);  // 5 + بیشتر…
  KIMIA_REQUIRE(editor.optionLabels()[0] == "خیابونی من");
  KIMIA_REQUIRE(editor.optionLabels()[5] == "بیشتر…");
  editor.choose(5);  // page 2
  KIMIA_REQUIRE(editor.optionLabels().size() == 6U);
  KIMIA_REQUIRE(editor.optionLabels()[0] == "بازی سفارشی 1");
  editor.choose(5);  // page 3: one game + بازگشت
  KIMIA_REQUIRE(editor.optionLabels().size() == 2U);
  KIMIA_REQUIRE(editor.optionLabels()[0] == "بازی سفارشی 6");
  KIMIA_REQUIRE(editor.optionLabels()[1] == "بازگشت");
  editor.choose(0);
  KIMIA_REQUIRE(editor.hasWorld());
  KIMIA_REQUIRE(editor.profile().name == "custom6");
  KIMIA_REQUIRE(near(editor.world().halfLength(), 8.0));  // (10 + 6) / 2
  // «بازگشت» from the game list returns to the main menu without a world.
  WorldEditor again;
  again.setProfileDirectory(dir);
  again.choose(0);
  again.choose(5);
  again.choose(5);
  again.choose(1);  // بازگشت
  KIMIA_REQUIRE(!again.hasWorld());
  KIMIA_REQUIRE(again.optionLabels().size() == 3U);  // main menu
}
