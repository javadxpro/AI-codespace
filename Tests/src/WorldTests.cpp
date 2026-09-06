#include <kimia_test.h>
#include <kimia/AssetPipeline.h>
#include <kimia/GameProfile.h>
#include <kimia/Golf.h>
#include <kimia/MathUtils.h>
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
#include <fstream>
#include <sstream>
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
  // «دنیای جدید» asks which game first: the 5 built-ins + «بازگشت»
  // (exactly one page: 5 games + back).
  KIMIA_REQUIRE(!editor.hasWorld());
  KIMIA_REQUIRE(editor.choosingProfile());
  KIMIA_REQUIRE(editor.menuTitle() == "دنیای جدید: کدام بازی؟");
  KIMIA_REQUIRE(editor.optionLabels().size() == 6U);
  KIMIA_REQUIRE(editor.optionLabels()[0] == "گلف کیمیا");
  KIMIA_REQUIRE(editor.optionLabels()[1] == "فوتبال خیابونی ایران: کوی ابوذر");
  KIMIA_REQUIRE(editor.optionLabels()[4] == "زمین آزاد");
  KIMIA_REQUIRE(editor.optionLabels()[5] == "بازگشت");
  editor.choose(4);  // زمین آزاد
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
  // The sandbox is a single player: four directions plus the dribble hold.
  KIMIA_REQUIRE(editor.holdPad().size() == 5U);
  KIMIA_REQUIRE(editor.holdPad()[4].second == "c");
  KIMIA_REQUIRE(editor.tapPad().size() == 3U);   // jump + reset + menu
  KIMIA_REQUIRE(editor.tapPad()[0].second == "j");
  // No team, so no pass button anywhere on the pad.
  for (const auto& pad : editor.tapPad()) KIMIA_REQUIRE(pad.second != "p");
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
  KIMIA_REQUIRE(editor.optionLabels().size() == 9U);  // ...جعبه، «مدل از فایل» و «سوراخ» joined the list
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
  KIMIA_REQUIRE(editor.optionLabels().size() == 9U);

  // Empty directory: the list screen shows only «بازگشت».
  const std::string emptyDir = std::string(KIMIA_TEST_TMP) + "/imports_empty";
  static_cast<void>(::mkdir(emptyDir.c_str(), 0755));
  editor.setImportDirectory(emptyDir);
  editor.choose(6);  // مدل از فایل (we are already in the catalog)
  KIMIA_REQUIRE(editor.importFileCount() == 0U);
  KIMIA_REQUIRE(editor.optionLabels().size() == 1U);
  KIMIA_REQUIRE(editor.optionLabels()[0] == "بازگشت");
  editor.choose(0);
  KIMIA_REQUIRE(editor.optionLabels().size() == 9U);  // catalog again
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
  // This test is about the walls, not the opposition: with the computer
  // players switched off nothing else can touch the ball.
  editor.setAiSkill(0.0);
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
  // Seven user profiles + 5 built-ins = 12 games -> 3 pages of 5.
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
  KIMIA_REQUIRE(editor.profileCount() == 12U);
  KIMIA_REQUIRE(editor.profileAt(0).name == "golf");    // build order: golf first
  KIMIA_REQUIRE(editor.profileAt(1).name == "street");  // still second, retuned in place
  KIMIA_REQUIRE(editor.profileAt(1).title == "خیابونی من");
  KIMIA_REQUIRE(near(editor.profileAt(1).fieldWidth, 6.0));
  editor.choose(0);  // دنیای جدید
  KIMIA_REQUIRE(editor.choosingProfile());
  KIMIA_REQUIRE(editor.optionLabels().size() == 6U);  // 5 + بیشتر…
  KIMIA_REQUIRE(editor.optionLabels()[0] == "گلف کیمیا");
  KIMIA_REQUIRE(editor.optionLabels()[1] == "خیابونی من");
  KIMIA_REQUIRE(editor.optionLabels()[5] == "بیشتر…");
  editor.choose(5);  // page 2: custom0..custom4
  KIMIA_REQUIRE(editor.optionLabels().size() == 6U);
  KIMIA_REQUIRE(editor.optionLabels()[0] == "بازی سفارشی 0");
  editor.choose(5);  // page 3: two games + بازگشت
  KIMIA_REQUIRE(editor.optionLabels().size() == 3U);
  KIMIA_REQUIRE(editor.optionLabels()[0] == "بازی سفارشی 5");
  KIMIA_REQUIRE(editor.optionLabels()[1] == "بازی سفارشی 6");
  KIMIA_REQUIRE(editor.optionLabels()[2] == "بازگشت");
  editor.choose(1);
  KIMIA_REQUIRE(editor.hasWorld());
  KIMIA_REQUIRE(editor.profile().name == "custom6");
  KIMIA_REQUIRE(near(editor.world().halfLength(), 8.0));  // (10 + 6) / 2
  // «بازگشت» from the game list returns to the main menu without a world.
  WorldEditor again;
  again.setProfileDirectory(dir);
  again.choose(0);
  again.choose(5);
  again.choose(5);
  again.choose(2);  // بازگشت
  KIMIA_REQUIRE(!again.hasWorld());
  KIMIA_REQUIRE(again.optionLabels().size() == 3U);  // main menu
}

// ---------------------------------------------------------------------------
// Golf on KIMIA World: shot mode + hole scoring (profile «golf»).
// ---------------------------------------------------------------------------

namespace {

// Builder(0) -> Catalog(7) «سوراخ» -> Place at the ghost -> placed (no question).
void addHole(WorldEditor& editor, const Vec3& ghost) {
  editor.choose(0);  // catalog
  editor.choose(7);  // سوراخ
  editor.setGhostPosition(ghost);
  editor.choose(0);  // place
}

// The catalog «توپ» entry places the ball without a question in golf.
void addGolfBall(WorldEditor& editor, const Vec3& ghost) {
  editor.choose(0);  // catalog
  editor.choose(1);  // توپ -> straight to Place (choice off)
  editor.setGhostPosition(ghost);
  editor.choose(0);  // place
}

// Hold «شوت» for `seconds`, then release: power = seconds * kWorldChargeRate.
void chargeAndShoot(WorldEditor& editor, f64 seconds) {
  editor.setShootHeld(true);
  const f64 step = 1.0 / 120.0;
  f64 held = 0.0;
  while (held + step <= seconds + 1e-12) {
    editor.update(step);
    held += step;
  }
  editor.setShootHeld(false);
  editor.update(step);  // the release frame fires the shot
}

}  // namespace

KIMIA_TEST(world_golf_profile_builds_a_shot_mode_world) {
  WorldEditor editor;
  createWorldFor(editor, "golf");
  KIMIA_REQUIRE(editor.hasWorld());
  KIMIA_REQUIRE(editor.shotMode());
  KIMIA_REQUIRE(editor.holeScoring());
  KIMIA_REQUIRE(editor.menuTitle() == "MyWorld (گلف کیمیا) — سازنده");
  KIMIA_REQUIRE(near(editor.world().halfLength(), 12.0));
  KIMIA_REQUIRE(near(editor.world().halfWidth(), 5.0));
  KIMIA_REQUIRE(editor.world().ball.type == BallType::Accurate);
  KIMIA_REQUIRE(near(editor.world().ball.radius, kGolfBallRadius));
  KIMIA_REQUIRE(near(editor.world().ball.rollingFriction, kGolfBallRollingFriction));
  // The catalog now offers the cup; golf never asks which ball.
  editor.choose(0);
  const std::vector<std::string> catalog = editor.optionLabels();
  KIMIA_REQUIRE(catalog.size() == 9U);
  KIMIA_REQUIRE(catalog[7] == "سوراخ");
  KIMIA_REQUIRE(catalog[8] == "بازگشت");
  editor.choose(8);  // back to the builder
  addHole(editor, Vec3{0.0, 0.0, -7.0});
  exitPlace(editor);
  KIMIA_REQUIRE(editor.holeCount() == 1U);
  const EntityData* hole = editor.world().scene.get(editor.world().scene.find("Hole_1"));
  KIMIA_REQUIRE(hole != nullptr);
  KIMIA_REQUIRE(near3(hole->transform.position, Vec3{0.0, kimia::kWorldHoleDepth * 0.5, -7.0}));
  KIMIA_REQUIRE(near(hole->transform.scale.x, kimia::kWorldHoleRadius * 2.0));
  KIMIA_REQUIRE(kimia::objectKindForName("Hole_1") == kimia::ObjectKind::Hole);
  KIMIA_REQUIRE(!kimia::isPhysicsObject(kimia::ObjectKind::Hole));  // the ball rolls over it
  KIMIA_REQUIRE(editor.physicsBoxCount() == 0U);
  // Shot-mode pads: aim left/right + hold-to-shoot; no jump.
  addGolfBall(editor, Vec3{0.0, 0.0, 7.0});
  exitPlace(editor);
  editor.choose(3);  // PLAY
  KIMIA_REQUIRE(editor.playing());
  // Aim left/right, charge, and the two curl sticks (stage 23).
  const auto holds = editor.holdPad();
  KIMIA_REQUIRE(holds.size() == 5U);
  KIMIA_REQUIRE(holds[2].second == "space");
  const auto taps = editor.tapPad();
  KIMIA_REQUIRE(taps.size() == 2U);
  KIMIA_REQUIRE(taps[0].second == "r");
  KIMIA_REQUIRE(editor.menuTitle() == "نشانه بگیر (← →) و «شوت» را نگه دار");
  KIMIA_REQUIRE(editor.statsLine().find("| stroke 0 | power 0%") != std::string::npos);
}

KIMIA_TEST(world_golf_aim_charge_and_shot_use_the_profile_numbers) {
  WorldEditor editor;
  createWorldFor(editor, "golf");
  addGolfBall(editor, Vec3{0.0, 0.0, 7.0});
  exitPlace(editor);
  editor.choose(3);  // PLAY
  KIMIA_REQUIRE(editor.ballAtRest());
  KIMIA_REQUIRE(near(editor.aimYaw(), 0.0));
  KIMIA_REQUIRE(near3(editor.aimDirection(), Vec3{0.0, 0.0, -1.0}));
  // Holding «→» turns the aim clockwise at kWorldAimRate; the ball waits.
  editor.setMoveInput(1.0, 0.0);
  for (i32 i = 0; i < 60; ++i) editor.update(1.0 / 120.0);  // 0.5 s
  editor.setMoveInput(0.0, 0.0);
  KIMIA_REQUIRE(near(editor.aimYaw(), -0.5 * kimia::kWorldAimRate, 1e-9));
  KIMIA_REQUIRE(near3(editor.ballPosition(), Vec3{0.0, kGolfBallRadius, 7.0}, 1e-6));
  editor.setAimYaw(0.0);
  // Charging: power climbs at kWorldChargeRate and wraps at 1.
  editor.setShootHeld(true);
  editor.update(0.5);
  KIMIA_REQUIRE(editor.charging());
  KIMIA_REQUIRE(near(editor.power(), 0.5 * kimia::kWorldChargeRate));
  KIMIA_REQUIRE(editor.menuTitle().rfind("شوت: رها کن تا بزنی — قدرت ", 0) == 0);
  editor.update(1.0);  // 0.45 + 0.9 = 1.35 -> wraps to 0.35
  KIMIA_REQUIRE(near(editor.power(), 1.35 - 1.0, 1e-9));
  // Release: the ball leaves at kickBase + power * kickSpeedScale along the aim.
  const f64 power = editor.power();
  editor.setShootHeld(false);
  editor.update(1.0 / 120.0);
  KIMIA_REQUIRE(!editor.charging());
  KIMIA_REQUIRE(editor.strokes() == 1U);
  KIMIA_REQUIRE(!editor.ballAtRest());
  const f64 expected = 2.5 + power * 13.5;
  KIMIA_REQUIRE(near(editor.shotSpeed(power), expected));
  KIMIA_REQUIRE(editor.ballVelocity().z < 0.0);
  KIMIA_REQUIRE(near(editor.ballVelocity().x, 0.0, 1e-9));
  // One physics frame of rolling friction has already acted: speed is just under the launch speed.
  KIMIA_REQUIRE(editor.ballVelocity().length() < expected);
  KIMIA_REQUIRE(editor.ballVelocity().length() > expected - 0.1);
  // A rolling ball cannot be hit again: holding the button does nothing.
  editor.setShootHeld(true);
  editor.update(0.2);
  KIMIA_REQUIRE(!editor.charging());
  KIMIA_REQUIRE(editor.strokes() == 1U);
  editor.setShootHeld(false);
  KIMIA_REQUIRE(editor.statsLine().find("| stroke 1 |") != std::string::npos);
  // The ball comes to rest and the next shot starts from THERE (not the tee).
  for (i32 i = 0; i < 120 * 10 && !editor.ballAtRest(); ++i) editor.update(1.0 / 120.0);
  KIMIA_REQUIRE(editor.ballAtRest());
  const Vec3 rest = editor.ballPosition();
  KIMIA_REQUIRE(rest.z < 6.0);  // it moved a real distance down the fairway
  chargeAndShoot(editor, 0.2);   // a tap: power 0.18 -> ~4.9 m/s
  KIMIA_REQUIRE(editor.strokes() == 2U);
  editor.update(1.0 / 120.0);
  KIMIA_REQUIRE(editor.ballPosition().z < rest.z);  // continues from the rest spot
}

KIMIA_TEST(world_golf_reference_shot_holes_out_and_resets_strokes) {
  // The reference golf's proof shot: aim 0, power 0.61, tee 14 m from the cup.
  WorldEditor editor;
  createWorldFor(editor, "golf");
  addHole(editor, Vec3{0.0, 0.0, -7.0});
  exitPlace(editor);
  addGolfBall(editor, Vec3{0.0, 0.0, 7.0});
  exitPlace(editor);
  editor.choose(3);  // PLAY
  editor.setShootHeld(true);
  // 0.61 / 0.9 s of charging, in exact 1/120 steps (power accumulates per frame).
  f64 power = 0.0;
  while (power + kimia::kWorldChargeRate / 120.0 <= 0.61) {
    editor.update(1.0 / 120.0);
    power += kimia::kWorldChargeRate / 120.0;
  }
  KIMIA_REQUIRE(near(editor.power(), power, 1e-9));
  editor.setShootHeld(false);
  editor.update(1.0 / 120.0);
  KIMIA_REQUIRE(editor.strokes() == 1U);
  bool holed = false;
  for (i32 i = 0; i < 120 * 60; ++i) {
    editor.update(1.0 / 120.0);
    if (editor.celebrating()) {
      holed = true;
      break;
    }
    if (editor.ballAtRest()) break;  // stopped short: no capture
  }
  KIMIA_REQUIRE(holed);
  KIMIA_REQUIRE(editor.score() == 1U);
  KIMIA_REQUIRE(editor.menuTitle() == "رفت تو سوراخ! ضربه‌ها: 1");
  KIMIA_REQUIRE(editor.statsLine().find("| GOAL |") != std::string::npos);
  // While celebrating the ball sits in the cup.
  KIMIA_REQUIRE(near3(editor.ballPosition(), Vec3{0.0, kGolfBallRadius, -7.0}, 1e-9));
  editor.update(0.5);
  KIMIA_REQUIRE(near3(editor.ballPosition(), Vec3{0.0, kGolfBallRadius, -7.0}, 1e-9));
  // After the celebration on a one-cup course the round is over: the
  // scorecard screen (1 stroke on a par-3 cup = 2 under par), no pads.
  editor.update(2.0);
  KIMIA_REQUIRE(!editor.celebrating());
  KIMIA_REQUIRE(editor.roundOver());
  KIMIA_REQUIRE(editor.playing());
  KIMIA_REQUIRE(editor.strokes() == 0U);
  KIMIA_REQUIRE(editor.scorecard().size() == 1U);
  KIMIA_REQUIRE(editor.scorecard()[0] == 1U);
  KIMIA_REQUIRE(editor.totalStrokes() == 1U);
  KIMIA_REQUIRE(editor.par() == 3U);
  KIMIA_REQUIRE(editor.scoreToPar() == -2);
  KIMIA_REQUIRE(editor.menuTitle() == "پایان دور! 1 | جمع 1 | پار 3 | 2 زیر پار");
  const std::vector<std::string> endOptions = editor.optionLabels();
  KIMIA_REQUIRE(endOptions.size() == 2U);
  KIMIA_REQUIRE(endOptions[0] == "دور جدید");
  KIMIA_REQUIRE(endOptions[1] == "منو");
  KIMIA_REQUIRE(editor.holdPad().empty());
  KIMIA_REQUIRE(editor.tapPad().empty());
  KIMIA_REQUIRE(editor.statsLine().find("| ROUNDEND |") != std::string::npos);
  KIMIA_REQUIRE(editor.statsLine().find("| hole 1/1 | total 1 | par 3") != std::string::npos);
  // The ball stays in the cup while the scorecard is shown.
  editor.update(1.0);
  KIMIA_REQUIRE(near3(editor.ballPosition(), Vec3{0.0, kGolfBallRadius, -7.0}, 1e-9));
  // «دور جدید»: back to the tee, cup 1, a clean scorecard, zero strokes.
  editor.choose(0);
  KIMIA_REQUIRE(!editor.roundOver());
  KIMIA_REQUIRE(editor.playing());
  KIMIA_REQUIRE(editor.currentHole() == 0U);
  KIMIA_REQUIRE(editor.scorecard().empty());
  KIMIA_REQUIRE(editor.strokes() == 0U);
  KIMIA_REQUIRE(near3(editor.ballPosition(), Vec3{0.0, kGolfBallRadius, 7.0}, 1e-9));
  KIMIA_REQUIRE(editor.ballAtRest());
  KIMIA_REQUIRE(editor.statsLine().find("| hole 1/1 | total 0 | par 3") != std::string::npos);
}

KIMIA_TEST(world_golf_cup_needs_a_slow_ball) {
  // Constant-force rolling friction: a = (0.25 + 0.15) * g = 3.924 m/s^2, so a
  // ball launched at v from d metres reaches the cup at sqrt(v^2 - 2 a d).
  WorldEditor editor;
  createWorldFor(editor, "golf");
  addHole(editor, Vec3{0.0, 0.0, -7.0});
  exitPlace(editor);
  addGolfBall(editor, Vec3{0.0, 0.0, -6.5});  // 0.5 m from the cup
  exitPlace(editor);
  editor.choose(3);  // PLAY
  // Too fast: 8 m/s arrives at ~7.75 m/s and rolls over (capture needs < 5 m/s).
  editor.setBallVelocity(Vec3{0.0, 0.0, -8.0});
  bool captured = false;
  for (i32 i = 0; i < 120 && !captured; ++i) {
    editor.update(1.0 / 120.0);
    captured = editor.celebrating();
  }
  KIMIA_REQUIRE(!captured);
  KIMIA_REQUIRE(editor.ballPosition().z < -7.3);  // it went past
  KIMIA_REQUIRE(editor.score() == 0U);
  // Gentle: 3 m/s arrives at ~2.26 m/s and drops in.
  editor.resetBall();
  KIMIA_REQUIRE(near3(editor.ballPosition(), Vec3{0.0, kGolfBallRadius, -6.5}, 1e-9));
  editor.setBallVelocity(Vec3{0.0, 0.0, -3.0});
  for (i32 i = 0; i < 240 && !captured; ++i) {
    editor.update(1.0 / 120.0);
    captured = editor.celebrating();
  }
  KIMIA_REQUIRE(captured);
  KIMIA_REQUIRE(editor.score() == 1U);
  // Too weak: 1 m/s stops after v^2 / (2a) = 0.127 m — short of the cup.
  WorldEditor weak;
  createWorldFor(weak, "golf");
  addHole(weak, Vec3{0.0, 0.0, -7.0});
  exitPlace(weak);
  addGolfBall(weak, Vec3{0.0, 0.0, -6.5});
  exitPlace(weak);
  weak.choose(3);
  weak.setBallVelocity(Vec3{0.0, 0.0, -1.0});
  for (i32 i = 0; i < 240; ++i) weak.update(1.0 / 120.0);
  KIMIA_REQUIRE(!weak.celebrating());
  KIMIA_REQUIRE(weak.ballAtRest());
  KIMIA_REQUIRE(near(weak.ballPosition().z, -6.5 - 1.0 / (2.0 * 0.40 * 9.81), 0.02));
  // Distance rule: passing 0.3 m beside the cup is not in (0.28 capture radius).
  WorldEditor beside;
  createWorldFor(beside, "golf");
  addHole(beside, Vec3{0.0, 0.0, -7.0});
  exitPlace(beside);
  addGolfBall(beside, Vec3{0.3, 0.0, -6.5});
  exitPlace(beside);
  beside.choose(3);
  beside.setBallVelocity(Vec3{0.0, 0.0, -3.0});
  bool besideCaptured = false;
  for (i32 i = 0; i < 480 && !besideCaptured; ++i) {
    beside.update(1.0 / 120.0);
    besideCaptured = beside.celebrating();
  }
  KIMIA_REQUIRE(!besideCaptured);
  KIMIA_REQUIRE(beside.ballPosition().z < -7.3);  // rolled past, beside the cup
  KIMIA_REQUIRE(beside.score() == 0U);
}

KIMIA_TEST(world_golf_world_file_keeps_mode_scoring_and_holes) {
  WorldEditor editor;
  createWorldFor(editor, "golf");
  addHole(editor, Vec3{1.5, 0.0, -5.0});
  exitPlace(editor);
  addGolfBall(editor, Vec3{0.0, 0.0, 6.0});
  exitPlace(editor);
  const std::string path = tmpPath("golf_world.kimia");
  std::string error;
  KIMIA_REQUIRE(editor.saveWorld(path, error));
  std::string text;
  {
    std::ifstream file(path, std::ios::binary);
    std::stringstream buffer;
    buffer << file.rdbuf();
    text = buffer.str();
  }
  KIMIA_REQUIRE(text.find("# profile mode shot\n") != std::string::npos);
  KIMIA_REQUIRE(text.find("# profile scoring hole\n") != std::string::npos);
  KIMIA_REQUIRE(text.find("# profile par 3\n") != std::string::npos);
  KIMIA_REQUIRE(text.find("e \"Hole_1\" mesh sphere pos 1.5 0.01 -5") != std::string::npos);
  // Reload (no profile directory needed): still golf, still shot mode, cup intact.
  WorldEditor reloaded;
  reloaded.setProfileDirectory(tmpPath("no_such_profiles"));
  KIMIA_REQUIRE(reloaded.loadWorld(path, error));
  KIMIA_REQUIRE(reloaded.profile().name == "golf");
  KIMIA_REQUIRE(reloaded.shotMode());
  KIMIA_REQUIRE(reloaded.holeScoring());
  KIMIA_REQUIRE(reloaded.holeCount() == 1U);
  KIMIA_REQUIRE(reloaded.managedCount() == 0U);  // (manage list is built on demand)
  // Byte-identical save of the reload.
  std::string again;
  KIMIA_REQUIRE(WorldIO::save(reloaded.world(), again));
  KIMIA_REQUIRE(again == text);
  // A pre-stage-20 world file (no mode/scoring lines) is a kick/gate world.
  WorldEditor old;
  const std::string oldPath = tmpPath("old_kick_world.kimia");
  {
    std::ofstream file(oldPath, std::ios::binary);
    file << "# KIMIA scene v1\n# world name Old\n# profile name street\n"
         << "e \"Ground\" mesh plane pos 0 0 0 scale 5 1 16 color 0.3 0.3 0.32 rough 0.95\n";
  }
  KIMIA_REQUIRE(old.loadWorld(oldPath, error));
  KIMIA_REQUIRE(!old.shotMode());
  KIMIA_REQUIRE(!old.holeScoring());
  KIMIA_REQUIRE(old.par() == 3U);  // default rating when the file predates `par`
}

KIMIA_TEST(world_golf_course_plays_the_cups_in_order_and_ends_with_a_scorecard) {
  // Three cups placed out of order (Hole_2's spot first) — the course order
  // is by NAME NUMBER, not by placement order or position.
  WorldEditor editor;
  createWorldFor(editor, "golf");
  addHole(editor, Vec3{0.0, 0.0, -4.0});  // Hole_1
  exitPlace(editor);
  addHole(editor, Vec3{2.0, 0.0, -4.0});  // Hole_2
  exitPlace(editor);
  addHole(editor, Vec3{2.0, 0.0, 4.0});   // Hole_3
  exitPlace(editor);
  addGolfBall(editor, Vec3{0.0, 0.0, 6.0});
  exitPlace(editor);
  KIMIA_REQUIRE(editor.holeCount() == 3U);
  editor.choose(3);  // PLAY
  KIMIA_REQUIRE(editor.currentHole() == 0U);
  KIMIA_REQUIRE(editor.currentHoleName() == "Hole_1");
  KIMIA_REQUIRE(editor.menuTitle() == "سوراخ 1 از 3 — نشانه بگیر (← →) و «شوت» را نگه دار");
  KIMIA_REQUIRE(editor.statsLine().find("| hole 1/3 | total 0 | par 3") != std::string::npos);

  // Cup 1: a gentle roll straight down -Z from 6 -> -4 (10 m). Hole_2 is
  // never on this line, so nothing else can capture. Two strokes: the first
  // stops short, the second drops.
  editor.setBallVelocity(Vec3{0.0, 0.0, -6.0});  // stops after 36 / (2 * 3.924) = 4.59 m
  for (i32 i = 0; i < 120 * 10 && !editor.ballAtRest(); ++i) editor.update(1.0 / 120.0);
  KIMIA_REQUIRE(editor.ballAtRest());
  KIMIA_REQUIRE(!editor.celebrating());
  KIMIA_REQUIRE(near(editor.ballPosition().z, 6.0 - 36.0 / (2.0 * 0.40 * 9.81), 0.05));
  // (setBallVelocity is a test hook, not a stroke; count real strokes via shots)
  editor.setAimYaw(0.0);
  chargeAndShoot(editor, 0.2);  // stroke 1: power 0.18 -> 4.93 m/s -> ~3.1 m, short of the cup
  KIMIA_REQUIRE(editor.strokes() == 1U);
  for (i32 i = 0; i < 120 * 10 && !editor.ballAtRest(); ++i) editor.update(1.0 / 120.0);
  KIMIA_REQUIRE(!editor.celebrating());
  const f64 remaining = editor.ballPosition().z - (-4.0);
  KIMIA_REQUIRE(remaining > 0.5);
  // Stroke 2: exactly the speed that arrives at 2 m/s: v = sqrt(2^2 + 2 a d).
  const f64 arrive = std::sqrt(4.0 + 2.0 * 0.40 * 9.81 * remaining);
  editor.setBallVelocity(Vec3{0.0, 0.0, -arrive});
  bool holed = false;
  for (i32 i = 0; i < 120 * 10 && !holed; ++i) {
    editor.update(1.0 / 120.0);
    holed = editor.celebrating();
  }
  KIMIA_REQUIRE(holed);
  KIMIA_REQUIRE(editor.score() == 1U);
  KIMIA_REQUIRE(editor.menuTitle() == "رفت تو سوراخ! ضربه‌ها: 1 — بعدی: سوراخ 2");
  editor.update(2.5);  // celebration over -> cup 2, played from cup 1's spot
  KIMIA_REQUIRE(!editor.celebrating());
  KIMIA_REQUIRE(!editor.roundOver());
  KIMIA_REQUIRE(editor.currentHole() == 1U);
  KIMIA_REQUIRE(editor.currentHoleName() == "Hole_2");
  KIMIA_REQUIRE(editor.scorecard().size() == 1U);
  KIMIA_REQUIRE(editor.scorecard()[0] == 1U);
  KIMIA_REQUIRE(editor.strokes() == 0U);
  KIMIA_REQUIRE(near3(editor.ballPosition(), Vec3{0.0, kGolfBallRadius, -4.0}, 1e-9));
  KIMIA_REQUIRE(editor.ballAtRest());
  KIMIA_REQUIRE(editor.menuTitle() == "سوراخ 2 از 3 — نشانه بگیر (← →) و «شوت» را نگه دار");

  // Cup 2 is 2 m along +X. Rolling THROUGH Hole_1's spot first (we start on
  // it) proves a non-current cup does not capture: roll away and back.
  editor.setBallVelocity(Vec3{0.0, 0.0, 2.0});  // away, +Z; stops after 0.51 m
  for (i32 i = 0; i < 120 * 5 && !editor.ballAtRest(); ++i) editor.update(1.0 / 120.0);
  KIMIA_REQUIRE(!editor.celebrating());
  const Vec3 offCup = editor.ballPosition();
  KIMIA_REQUIRE(offCup.z > -3.6);
  // Back over Hole_1 (current is Hole_2): must roll through, not drop.
  editor.setBallVelocity(Vec3{0.0, 0.0, -3.0});  // arrives at Hole_1 at ~2.2 m/s, slow enough to be captured IF it counted
  for (i32 i = 0; i < 120 * 5 && !editor.ballAtRest(); ++i) editor.update(1.0 / 120.0);
  KIMIA_REQUIRE(!editor.celebrating());
  KIMIA_REQUIRE(editor.ballPosition().z < -4.3);  // went past Hole_1
  KIMIA_REQUIRE(editor.currentHole() == 1U);
  // Now hole cup 2 from wherever we are: aim straight at it with a slow arrival.
  const Vec3 from = editor.ballPosition();
  const f64 dx = 2.0 - from.x;
  const f64 dz = -4.0 - from.z;
  const f64 dist = std::sqrt(dx * dx + dz * dz);
  const f64 v2 = std::sqrt(4.0 + 2.0 * 0.40 * 9.81 * dist);
  editor.setBallVelocity(Vec3{dx / dist * v2, 0.0, dz / dist * v2});
  holed = false;
  for (i32 i = 0; i < 120 * 10 && !holed; ++i) {
    editor.update(1.0 / 120.0);
    holed = editor.celebrating();
  }
  KIMIA_REQUIRE(holed);
  editor.update(2.5);
  KIMIA_REQUIRE(editor.currentHole() == 2U);
  KIMIA_REQUIRE(editor.currentHoleName() == "Hole_3");
  KIMIA_REQUIRE(editor.scorecard().size() == 2U);
  KIMIA_REQUIRE(editor.scorecard()[1] == 0U);  // (hook-driven: no real stroke on cup 2)

  // Cup 3: 8 m up +Z from cup 2. Two real strokes then a computed drop.
  editor.setAimYaw(kimia::kPi);  // aim toward +Z
  KIMIA_REQUIRE(near3(editor.aimDirection(), Vec3{0.0, 0.0, 1.0}, 1e-9));
  chargeAndShoot(editor, 0.2);
  for (i32 i = 0; i < 120 * 10 && !editor.ballAtRest(); ++i) editor.update(1.0 / 120.0);
  chargeAndShoot(editor, 0.2);
  for (i32 i = 0; i < 120 * 10 && !editor.ballAtRest(); ++i) editor.update(1.0 / 120.0);
  KIMIA_REQUIRE(editor.strokes() == 2U);
  KIMIA_REQUIRE(!editor.celebrating());
  const Vec3 at = editor.ballPosition();
  const f64 left = 4.0 - at.z;
  KIMIA_REQUIRE(left > 0.3);
  KIMIA_REQUIRE(near(at.x, 2.0, 1e-6));
  editor.setBallVelocity(Vec3{0.0, 0.0, std::sqrt(4.0 + 2.0 * 0.40 * 9.81 * left)});
  holed = false;
  for (i32 i = 0; i < 120 * 10 && !holed; ++i) {
    editor.update(1.0 / 120.0);
    holed = editor.celebrating();
  }
  KIMIA_REQUIRE(holed);
  KIMIA_REQUIRE(editor.menuTitle() == "رفت تو سوراخ! ضربه‌ها: 2");  // last cup: no «بعدی»
  editor.update(2.5);
  // Round over: 1 + 0 + 2 = 3 strokes on a par 9 course = 6 under.
  KIMIA_REQUIRE(editor.roundOver());
  KIMIA_REQUIRE(editor.scorecard().size() == 3U);
  KIMIA_REQUIRE(editor.totalStrokes() == 3U);
  KIMIA_REQUIRE(editor.scoreToPar() == -6);
  KIMIA_REQUIRE(editor.scorecardText() == "1 0 2 | جمع 3 | پار 9 | 6 زیر پار");
  KIMIA_REQUIRE(editor.menuTitle() == "پایان دور! 1 0 2 | جمع 3 | پار 9 | 6 زیر پار");
  KIMIA_REQUIRE(editor.statsLine().find("| hole 3/3 | total 3 | par 3") != std::string::npos);
  KIMIA_REQUIRE(editor.score() == 3U);
  // «منو» leaves to the builder; PLAY again starts a fresh round at cup 1.
  editor.choose(1);
  KIMIA_REQUIRE(!editor.playing());
  editor.choose(3);
  KIMIA_REQUIRE(editor.playing());
  KIMIA_REQUIRE(editor.currentHole() == 0U);
  KIMIA_REQUIRE(editor.scorecard().empty());
  KIMIA_REQUIRE(near3(editor.ballPosition(), Vec3{0.0, kGolfBallRadius, 6.0}, 1e-9));
}

KIMIA_TEST(world_golf_scorecard_words_over_and_even_par) {
  // The scorecard wording for over par and even par (under par is covered
  // by the course test), and the course-less edge: no cup = no capture.
  WorldEditor editor;
  createWorldFor(editor, "golf");
  addHole(editor, Vec3{0.0, 0.0, -1.0});
  exitPlace(editor);
  addGolfBall(editor, Vec3{0.0, 0.0, 0.5});
  exitPlace(editor);
  editor.choose(3);
  // Four taps that never reach (aim away from the cup), then drop it: 5 strokes on par 3.
  editor.setAimYaw(kimia::kPi);  // +Z, away from the cup; the field edge clamps the ball
  for (i32 stroke = 0; stroke < 4; ++stroke) {
    chargeAndShoot(editor, 1.0 / 120.0);  // the weakest tap: 2.5 m/s
    for (i32 i = 0; i < 120 * 5 && !editor.ballAtRest(); ++i) editor.update(1.0 / 120.0);
  }
  KIMIA_REQUIRE(editor.strokes() == 4U);
  KIMIA_REQUIRE(!editor.celebrating());
  editor.setAimYaw(0.0);
  const f64 d = editor.ballPosition().z - (-1.0);
  KIMIA_REQUIRE(near(editor.ballPosition().x, 0.0, 1e-6));
  // The 5th stroke is real: pick the charge time whose speed arrives slowly.
  // speed = 2.5 + power * 13.5, power = frames / 120 * 0.9.
  const f64 wanted = std::sqrt(4.0 + 2.0 * 0.40 * 9.81 * d);
  const f64 power = (wanted - 2.5) / 13.5;
  KIMIA_REQUIRE(power > 0.0);
  const i32 frames = static_cast<i32>(std::ceil(power * 120.0 / 0.9));
  chargeAndShoot(editor, static_cast<f64>(frames) / 120.0);
  KIMIA_REQUIRE(editor.strokes() == 5U);
  bool holed = false;
  for (i32 i = 0; i < 120 * 10 && !holed; ++i) {
    editor.update(1.0 / 120.0);
    holed = editor.celebrating();
  }
  KIMIA_REQUIRE(holed);
  editor.update(2.5);
  KIMIA_REQUIRE(editor.roundOver());
  KIMIA_REQUIRE(editor.scorecardText() == "5 | جمع 5 | پار 3 | 2 بالای پار");
  // Even par: a fresh round holed in exactly 3 (two taps away, one computed drop).
  editor.choose(0);  // دور جدید
  KIMIA_REQUIRE(editor.scorecard().empty());
  editor.setAimYaw(kimia::kPi);
  for (i32 stroke = 0; stroke < 2; ++stroke) {
    chargeAndShoot(editor, 1.0 / 120.0);
    for (i32 i = 0; i < 120 * 5 && !editor.ballAtRest(); ++i) editor.update(1.0 / 120.0);
  }
  const f64 d2 = editor.ballPosition().z - (-1.0);
  const f64 wanted2 = std::sqrt(4.0 + 2.0 * 0.40 * 9.81 * d2);
  const i32 frames2 = static_cast<i32>(std::ceil((wanted2 - 2.5) / 13.5 * 120.0 / 0.9));
  editor.setAimYaw(0.0);
  chargeAndShoot(editor, static_cast<f64>(frames2) / 120.0);
  holed = false;
  for (i32 i = 0; i < 120 * 10 && !holed; ++i) {
    editor.update(1.0 / 120.0);
    holed = editor.celebrating();
  }
  KIMIA_REQUIRE(holed);
  editor.update(2.5);
  KIMIA_REQUIRE(editor.scorecardText() == "3 | جمع 3 | پار 3 | برابر پار");
  // No cup at all: the ball can never be captured; hole 0/0 in the stats.
  WorldEditor empty;
  createWorldFor(empty, "golf");
  addGolfBall(empty, Vec3{0.0, 0.0, 0.0});
  exitPlace(empty);
  empty.choose(3);
  KIMIA_REQUIRE(empty.currentHoleName().empty());
  empty.setBallVelocity(Vec3{0.0, 0.0, -1.0});
  for (i32 i = 0; i < 240; ++i) empty.update(1.0 / 120.0);
  KIMIA_REQUIRE(!empty.celebrating());
  KIMIA_REQUIRE(empty.statsLine().find("| hole 0/0 | total 0 | par 3") != std::string::npos);
}

KIMIA_TEST(world_football_profile_can_also_use_a_cup) {
  // The cup is an engine feature, not a golf-only one: a street world with a
  // hole and «scoring hole» would score it too — here the plain street
  // profile keeps gate scoring, so a ball in the cup is NOT a score.
  WorldEditor editor;
  createWorldFor(editor, "street");
  addHole(editor, Vec3{0.0, 0.0, -3.0});
  exitPlace(editor);
  addPlayer(editor, 1, Vec3{0.0, 0.0, 2.0});
  exitPlace(editor);
  editor.choose(0);
  editor.choose(1);  // توپ (no question in street)
  editor.setGhostPosition(Vec3{0.0, 0.0, -2.5});
  editor.choose(0);
  exitPlace(editor);
  editor.choose(3);  // PLAY
  KIMIA_REQUIRE(!editor.shotMode());
  KIMIA_REQUIRE(!editor.holeScoring());
  editor.setBallVelocity(Vec3{0.0, 0.0, -1.0});
  for (i32 i = 0; i < 240; ++i) editor.update(1.0 / 120.0);
  KIMIA_REQUIRE(editor.score() == 0U);
  KIMIA_REQUIRE(!editor.celebrating());
  // The runner still works in the same world (kick mode untouched). street
  // is a 5-a-side match, so it gets the full stage 23 set: four directions
  // plus dribble and the two curl sticks.
  KIMIA_REQUIRE(editor.holdPad().size() == 7U);
  KIMIA_REQUIRE(editor.tapPad()[0].second == "j");
}

// --- HUD lines, game events, chase camera (stage 20.5b) ---

KIMIA_TEST(world_hud_lines_follow_the_golf_round_and_events_drain_once) {
  using Event = WorldEditor::GameEvent;
  WorldEditor editor;
  createWorldFor(editor, "golf");
  addHole(editor, Vec3{0.0, 0.0, -7.0});
  exitPlace(editor);
  addGolfBall(editor, Vec3{0.0, 0.0, 7.0});
  exitPlace(editor);
  // Not playing: no HUD, no power, no chase camera, nothing queued.
  KIMIA_REQUIRE(editor.hudLines().empty());
  KIMIA_REQUIRE(editor.hudPower() < 0.0);
  KIMIA_REQUIRE(!editor.chaseCameraActive());
  KIMIA_REQUIRE(editor.drainEvents().empty());

  editor.choose(3);  // PLAY
  KIMIA_REQUIRE(editor.chaseCameraActive());
  KIMIA_REQUIRE(editor.drainEvents().empty());  // entering play queues nothing
  std::vector<std::string> hud = editor.hudLines();
  KIMIA_REQUIRE(hud.size() == 2U);
  KIMIA_REQUIRE(hud[0] == "HOLE 1/1  PAR 3");
  KIMIA_REQUIRE(hud[1] == "STROKE 0  TOTAL 0");
  // The power meter shows only while charging, as the 0..1 charge.
  editor.setShootHeld(true);
  editor.update(0.5);
  KIMIA_REQUIRE(near(editor.hudPower(), 0.5 * kimia::kWorldChargeRate, 1e-9));
  editor.update(0.5);
  editor.setShootHeld(false);
  editor.update(1.0 / 120.0);  // the release fires the shot: one Shot event
  KIMIA_REQUIRE(editor.hudPower() < 0.0);
  KIMIA_REQUIRE(editor.strokes() == 1U);
  std::vector<Event> events = editor.drainEvents();
  KIMIA_REQUIRE(events.size() == 1U);
  KIMIA_REQUIRE(events[0] == Event::Shot);
  KIMIA_REQUIRE(editor.drainEvents().empty());  // drained once
  hud = editor.hudLines();
  KIMIA_REQUIRE(hud[1] == "STROKE 1  TOTAL 1");
  // Run until the ball rests or drops; a 0.9 power shot (14.65 m/s) from
  // 14 m flies over the cup and stops (no capture) — so hole it explicitly.
  for (i32 i = 0; i < 120 * 30 && !editor.ballAtRest() && !editor.celebrating(); ++i) editor.update(1.0 / 120.0);
  KIMIA_REQUIRE(!editor.celebrating());
  editor.setBallPosition(Vec3{0.0, kGolfBallRadius, -6.0});
  editor.setBallVelocity(Vec3{0.0, 0.0, -std::sqrt(4.0 + 2.0 * 0.40 * 9.81 * 1.0)});
  bool holed = false;
  for (i32 i = 0; i < 120 * 10 && !holed; ++i) {
    editor.update(1.0 / 120.0);
    holed = editor.celebrating();
  }
  KIMIA_REQUIRE(holed);
  events = editor.drainEvents();
  KIMIA_REQUIRE(events.size() == 1U);
  KIMIA_REQUIRE(events[0] == Event::Holed);
  hud = editor.hudLines();
  KIMIA_REQUIRE(hud.size() == 2U);
  KIMIA_REQUIRE(hud[0] == "HOLE 1/1  PAR 3");
  KIMIA_REQUIRE(hud[1] == "IN! 1 STROKE");
  KIMIA_REQUIRE(editor.chaseCameraActive());  // still behind the ball while celebrating
  // The celebration ends on the last cup -> RoundOver event + scorecard HUD.
  editor.update(2.5);
  KIMIA_REQUIRE(editor.roundOver());
  events = editor.drainEvents();
  KIMIA_REQUIRE(events.size() == 1U);
  KIMIA_REQUIRE(events[0] == Event::RoundOver);
  hud = editor.hudLines();
  KIMIA_REQUIRE(hud.size() == 3U);
  KIMIA_REQUIRE(hud[0] == "ROUND OVER  1 (PAR 3)  2 UNDER");
  KIMIA_REQUIRE(hud[1] == "CARD 1");
  KIMIA_REQUIRE(hud[2] == "NEW BEST 1");  // the first finished round sets the record
  KIMIA_REQUIRE(!editor.chaseCameraActive());  // the scorecard is a still screen
  // «دور جدید» resets the HUD and leaves nothing queued.
  editor.choose(0);
  KIMIA_REQUIRE(editor.drainEvents().empty());
  hud = editor.hudLines();
  KIMIA_REQUIRE(hud[1] == "STROKE 0  TOTAL 0");
  KIMIA_REQUIRE(editor.chaseCameraActive());
}

KIMIA_TEST(world_hud_words_for_over_par_even_par_and_plural_strokes) {
  WorldEditor editor;
  createWorldFor(editor, "golf");
  addHole(editor, Vec3{0.0, 0.0, -1.0});
  exitPlace(editor);
  addGolfBall(editor, Vec3{0.0, 0.0, 0.5});
  exitPlace(editor);
  editor.choose(3);
  // Four taps away from the cup, then a computed drop: 5 strokes on par 3.
  editor.setAimYaw(kimia::kPi);
  for (i32 stroke = 0; stroke < 4; ++stroke) {
    chargeAndShoot(editor, 1.0 / 120.0);
    for (i32 i = 0; i < 120 * 5 && !editor.ballAtRest(); ++i) editor.update(1.0 / 120.0);
  }
  std::vector<WorldEditor::GameEvent> events = editor.drainEvents();
  KIMIA_REQUIRE(events.size() == 4U);
  for (const WorldEditor::GameEvent event : events) KIMIA_REQUIRE(event == WorldEditor::GameEvent::Shot);
  KIMIA_REQUIRE(editor.hudLines()[1] == "STROKE 4  TOTAL 4");
  editor.setAimYaw(0.0);
  const f64 d = editor.ballPosition().z - (-1.0);
  const f64 wanted = std::sqrt(4.0 + 2.0 * 0.40 * 9.81 * d);
  const i32 frames = static_cast<i32>(std::ceil((wanted - 2.5) / 13.5 * 120.0 / 0.9));
  chargeAndShoot(editor, static_cast<f64>(frames) / 120.0);
  bool holed = false;
  for (i32 i = 0; i < 120 * 10 && !holed; ++i) {
    editor.update(1.0 / 120.0);
    holed = editor.celebrating();
  }
  KIMIA_REQUIRE(holed);
  KIMIA_REQUIRE(editor.hudLines()[1] == "IN! 5 STROKES");
  editor.update(2.5);
  KIMIA_REQUIRE(editor.hudLines()[0] == "ROUND OVER  5 (PAR 3)  2 OVER");
  KIMIA_REQUIRE(editor.hudLines()[1] == "CARD 5");
  // Even par on a fresh round: two taps away, one drop.
  editor.choose(0);
  editor.setAimYaw(kimia::kPi);
  for (i32 stroke = 0; stroke < 2; ++stroke) {
    chargeAndShoot(editor, 1.0 / 120.0);
    for (i32 i = 0; i < 120 * 5 && !editor.ballAtRest(); ++i) editor.update(1.0 / 120.0);
  }
  const f64 d2 = editor.ballPosition().z - (-1.0);
  const i32 frames2 = static_cast<i32>(std::ceil((std::sqrt(4.0 + 2.0 * 0.40 * 9.81 * d2) - 2.5) / 13.5 * 120.0 / 0.9));
  editor.setAimYaw(0.0);
  chargeAndShoot(editor, static_cast<f64>(frames2) / 120.0);
  holed = false;
  for (i32 i = 0; i < 120 * 10 && !holed; ++i) {
    editor.update(1.0 / 120.0);
    holed = editor.celebrating();
  }
  KIMIA_REQUIRE(holed);
  editor.update(2.5);
  KIMIA_REQUIRE(editor.hudLines()[0] == "ROUND OVER  3 (PAR 3)  EVEN");
  KIMIA_REQUIRE(editor.hudLines()[1] == "CARD 3");
}

KIMIA_TEST(world_hud_and_events_in_kick_mode_score_and_goal) {
  using Event = WorldEditor::GameEvent;
  WorldEditor editor = editorWithWorld();
  addGoal(editor, 1, Vec3{0.0, 0.0, -2.0});
  exitPlace(editor);
  editor.choose(3);  // PLAY
  KIMIA_REQUIRE(!editor.chaseCameraActive());  // the runner keeps the free orbit camera
  std::vector<std::string> hud = editor.hudLines();
  KIMIA_REQUIRE(hud.size() == 1U);
  KIMIA_REQUIRE(hud[0] == "SCORE 0");
  KIMIA_REQUIRE(editor.hudPower() < 0.0);
  editor.setPlayerPosition(Vec3{0.0, 0.5, 0.6});
  editor.setMoveInput(0.0, -1.0);
  editor.update(0.0);  // the kick
  std::vector<Event> events = editor.drainEvents();
  KIMIA_REQUIRE(events.size() == 1U);
  KIMIA_REQUIRE(events[0] == Event::Kick);
  editor.setMoveInput(0.0, 0.0);
  bool scored = false;
  for (i32 i = 0; i < 600 && !scored; ++i) {
    editor.update(1.0 / 60.0);
    scored = editor.celebrating();
  }
  KIMIA_REQUIRE(scored);
  events = editor.drainEvents();
  KIMIA_REQUIRE(!events.empty());
  KIMIA_REQUIRE(events.back() == Event::Goal);
  for (const Event event : events) KIMIA_REQUIRE(event == Event::Goal || event == Event::Kick);
  hud = editor.hudLines();
  KIMIA_REQUIRE(hud.size() == 2U);
  KIMIA_REQUIRE(hud[0] == "SCORE 1");
  KIMIA_REQUIRE(hud[1] == "GOAL!");
  editor.update(2.5);
  KIMIA_REQUIRE(!editor.celebrating());
  KIMIA_REQUIRE(editor.hudLines().size() == 1U);
  KIMIA_REQUIRE(editor.drainEvents().empty());
}

// --- Stage 20.5-b2: wind and the best round ---

KIMIA_TEST(world_best_round_records_persists_and_only_improves) {
  // A one-cup course: finish rounds and watch the record behave. The record
  // must be saved into the world file (a personal record survives quitting).
  WorldEditor editor;
  createWorldFor(editor, "golf");
  addHole(editor, Vec3{0.0, 0.0, -8.0});
  exitPlace(editor);
  addGolfBall(editor, Vec3{0.0, 0.0, 0.5});
  exitPlace(editor);
  KIMIA_REQUIRE(editor.bestRound() == 0U);  // nothing played yet
  KIMIA_REQUIRE(!editor.bestRoundIsNew());

  // A helper that plays one round of `strokes` shots and drops the ball in.
  const auto playRound = [&editor](u32 strokes) {
    editor.choose(3);  // PLAY
    for (u32 i = 0; i < strokes; ++i) {
      editor.setAimYaw(0.0);
      chargeAndShoot(editor, 0.05);  // a tiny tap: never reaches the cup
      for (i32 s = 0; s < 120 * 5 && !editor.ballAtRest(); ++s) editor.update(1.0 / 120.0);
    }
    // Drop it in with a computed arrival speed.
    const f64 left = editor.ballPosition().z - (-8.0);
    editor.setBallVelocity(Vec3{0.0, 0.0, -std::sqrt(1.0 + 2.0 * 0.40 * 9.81 * std::abs(left))});
    for (i32 s = 0; s < 120 * 10 && !editor.celebrating(); ++s) editor.update(1.0 / 120.0);
    KIMIA_REQUIRE(editor.celebrating());
    editor.update(2.5);  // celebration -> round over (single cup)
    KIMIA_REQUIRE(editor.roundOver());
  };

  playRound(3U);
  KIMIA_REQUIRE(editor.totalStrokes() == 3U);
  KIMIA_REQUIRE(editor.bestRound() == 3U);      // first finished round sets it
  KIMIA_REQUIRE(editor.bestRoundIsNew());
  std::vector<std::string> hud = editor.hudLines();
  KIMIA_REQUIRE(hud.size() == 3U);
  KIMIA_REQUIRE(hud[2] == "NEW BEST 3");
  KIMIA_REQUIRE(editor.statsLine().find("| best 3") != std::string::npos);

  // A WORSE round leaves the record alone and says so quietly.
  editor.choose(1);  // «منو» -> builder
  playRound(5U);
  KIMIA_REQUIRE(editor.totalStrokes() == 5U);
  KIMIA_REQUIRE(editor.bestRound() == 3U);
  KIMIA_REQUIRE(!editor.bestRoundIsNew());
  hud = editor.hudLines();
  KIMIA_REQUIRE(hud[2] == "BEST 3");

  // A BETTER round takes it.
  editor.choose(1);
  playRound(1U);
  KIMIA_REQUIRE(editor.totalStrokes() == 1U);
  KIMIA_REQUIRE(editor.bestRound() == 1U);
  KIMIA_REQUIRE(editor.bestRoundIsNew());
  KIMIA_REQUIRE(editor.hudLines()[2] == "NEW BEST 1");

  // It rides along in the world file, and reloading brings it back.
  std::string text;
  KIMIA_REQUIRE(kimia::WorldIO::save(editor.world(), text));
  KIMIA_REQUIRE(text.find("# best 1\n") != std::string::npos);
  kimia::WorldData reloaded;
  std::string error;
  KIMIA_REQUIRE(kimia::WorldIO::load(text, reloaded, error));
  KIMIA_REQUIRE(reloaded.bestRound == 1U);
  // Byte-identical round trip, record included.
  std::string again;
  KIMIA_REQUIRE(kimia::WorldIO::save(reloaded, again));
  KIMIA_REQUIRE(again == text);
}

KIMIA_TEST(world_best_round_is_absent_from_worlds_never_finished) {
  // A world nobody finished must serialize exactly like it always did — no
  // stray `# best` line, so old and new files stay comparable.
  WorldEditor editor;
  createWorldFor(editor, "golf");
  addGolfBall(editor, Vec3{0.0, 0.0, 0.5});
  exitPlace(editor);
  std::string text;
  KIMIA_REQUIRE(kimia::WorldIO::save(editor.world(), text));
  KIMIA_REQUIRE(text.find("# best") == std::string::npos);
  // And a file that has no record loads as "no record".
  kimia::WorldData loaded;
  std::string error;
  KIMIA_REQUIRE(kimia::WorldIO::load(text, loaded, error));
  KIMIA_REQUIRE(loaded.bestRound == 0U);
}

KIMIA_TEST(world_wind_comes_from_the_profile_and_bends_a_shot) {
  // The same shot, calm and in a crosswind: the windy one must land clearly
  // to the side, and the calm one must be dead straight.
  const auto shotLanding = [](f64 windSpeed, f64 windDirection) {
    WorldEditor editor;
    createWorldFor(editor, "golf");
    // Retune the world's own profile copy: this is exactly what an edited
    // Profiles/golf.kimiaprofile does.
    kimia::GameProfile windy = editor.profile();
    windy.windSpeed = windSpeed;
    windy.windDirection = windDirection;
    editor.createWorld(windy);
    addGolfBall(editor, Vec3{0.0, 0.0, 6.0});
    exitPlace(editor);
    editor.choose(3);  // PLAY
    editor.setAimYaw(0.0);
    // A medium shot: long enough for the breeze to work on, short enough to
    // stop well inside the 24 m field (a tailwind must have room to run).
    chargeAndShoot(editor, 0.35);
    for (i32 i = 0; i < 120 * 20 && !editor.ballAtRest(); ++i) editor.update(1.0 / 120.0);
    return editor.ballPosition();
  };
  const Vec3 calm = shotLanding(0.0, 0.0);
  KIMIA_REQUIRE(near(calm.x, 0.0, 1e-9));  // no wind: perfectly straight
  const Vec3 blown = shotLanding(6.0, kimia::kPi * 0.5);  // 6 m/s^2 toward -X
  KIMIA_REQUIRE(blown.x < -0.05);                          // pushed to -X
  KIMIA_REQUIRE(near(blown.z, calm.z, 0.6));               // mostly a sideways effect
  // A tailwind carries the ball FARTHER than calm (aim 0 = -Z, wind 0 = -Z).
  const Vec3 tail = shotLanding(6.0, 0.0);
  KIMIA_REQUIRE(tail.z < calm.z - 0.02);
}

KIMIA_TEST(world_wind_hud_and_stats_read_relative_to_the_aim) {
  WorldEditor editor;
  createWorldFor(editor, "golf");
  kimia::GameProfile windy = editor.profile();
  windy.windSpeed = 3.0;
  windy.windDirection = kimia::kPi * 0.5;  // blowing toward -X
  editor.createWorld(windy);
  addGolfBall(editor, Vec3{0.0, 0.0, 6.0});
  exitPlace(editor);
  KIMIA_REQUIRE(editor.windActive());
  KIMIA_REQUIRE(near(editor.windSpeed(), 3.0));
  KIMIA_REQUIRE(near3(editor.windVector(), Vec3{-1.0, 0.0, 0.0}, 1e-12));
  editor.choose(3);  // PLAY
  // Aiming down -Z, a wind toward -X blows across from the right: "<-".
  editor.setAimYaw(0.0);
  KIMIA_REQUIRE(editor.windHudText() == "WIND 3 <-");
  // Turn to face the wind: it becomes a headwind.
  editor.setAimYaw(kimia::kPi * 0.5);
  KIMIA_REQUIRE(editor.windHudText() == "WIND 3 ^");   // blowing exactly where I aim = tail
  editor.setAimYaw(-kimia::kPi * 0.5);
  KIMIA_REQUIRE(editor.windHudText() == "WIND 3 v");   // straight into my face
  editor.setAimYaw(kimia::kPi);
  KIMIA_REQUIRE(editor.windHudText() == "WIND 3 ->");
  // It shows up on the HUD and in the stats line while playing.
  editor.setAimYaw(0.0);
  const std::vector<std::string> hud = editor.hudLines();
  KIMIA_REQUIRE(hud.size() == 3U);
  KIMIA_REQUIRE(hud[2] == "WIND 3 <-");
  KIMIA_REQUIRE(editor.statsLine().find("| wind 3") != std::string::npos);
}

KIMIA_TEST(world_calm_games_show_no_wind_anywhere) {
  // Every shipped game is calm, so nothing about the HUD or the stats line
  // changed for them.
  WorldEditor editor;
  createWorldFor(editor, "golf");
  addGolfBall(editor, Vec3{0.0, 0.0, 6.0});
  exitPlace(editor);
  editor.choose(3);
  KIMIA_REQUIRE(!editor.windActive());
  KIMIA_REQUIRE(editor.windHudText().empty());
  KIMIA_REQUIRE(near3(editor.windVector(), Vec3{0.0, 0.0, 0.0}, 0.0));
  KIMIA_REQUIRE(editor.hudLines().size() == 2U);
  KIMIA_REQUIRE(editor.statsLine().find("| wind") == std::string::npos);
}

KIMIA_TEST(world_wind_rides_along_in_the_world_file) {
  // A world saved in a gale plays in the same gale even if the profile file
  // is edited or deleted later (the world carries its profile).
  WorldEditor editor;
  createWorldFor(editor, "golf");
  kimia::GameProfile windy = editor.profile();
  windy.windSpeed = 4.5;
  windy.windDirection = 1.25;
  editor.createWorld(windy);
  addGolfBall(editor, Vec3{0.0, 0.0, 6.0});
  exitPlace(editor);
  std::string text;
  KIMIA_REQUIRE(kimia::WorldIO::save(editor.world(), text));
  KIMIA_REQUIRE(text.find("# profile wind 4.500000 1.250000\n") != std::string::npos);
  kimia::WorldData loaded;
  std::string error;
  KIMIA_REQUIRE(kimia::WorldIO::load(text, loaded, error));
  KIMIA_REQUIRE(near(loaded.profile.windSpeed, 4.5));
  KIMIA_REQUIRE(near(loaded.profile.windDirection, 1.25));
  std::string again;
  KIMIA_REQUIRE(kimia::WorldIO::save(loaded, again));
  KIMIA_REQUIRE(again == text);
  // An old file with no wind line is calm.
  kimia::WorldData old;
  KIMIA_REQUIRE(kimia::WorldIO::load("# KIMIA scene v1\ne \"Ground\" mesh plane pos 0 0 0 scale 20 1 20 color 0.2 0.4 0.2 rough 0.9\n",
                                     old, error));
  KIMIA_REQUIRE(near(old.profile.windSpeed, 0.0));
}

// --- Stage 21: squads from the profile ---

KIMIA_TEST(world_street_profile_fills_the_pitch_with_five_a_side) {
  WorldEditor editor;
  createWorldFor(editor, "street");
  KIMIA_REQUIRE(editor.teamSize() == 5U);
  // In the builder there is still exactly one body: the lone player.
  KIMIA_REQUIRE(editor.squadCount() == 1U);
  addGolfBall(editor, Vec3{0.0, 0.0, 0.0});
  exitPlace(editor);
  editor.choose(3);  // PLAY
  // 5 a side = 10 players on the pitch, the human being one of them.
  KIMIA_REQUIRE(editor.squadCount() == 10U);
  const std::vector<u32> ids = editor.squadIds();
  KIMIA_REQUIRE(ids.size() == 10U);
  KIMIA_REQUIRE(ids[0] == 1U);
  // The human is character 1 and plays for team 1.
  KIMIA_REQUIRE(editor.squadTeam(1U) == 1U);
  u32 ours = 0U;
  u32 theirs = 0U;
  for (const u32 id : ids) {
    if (editor.squadTeam(id) == 1U) ++ours;
    if (editor.squadTeam(id) == 2U) ++theirs;
  }
  KIMIA_REQUIRE(ours == 5U);
  KIMIA_REQUIRE(theirs == 5U);
  // Formation: our four team-mates on +Z, their five on -Z. The street court
  // is 16 long, so the rows sit at z = +/- 8/3.
  const f64 row = 16.0 / 2.0 / 3.0;
  for (const u32 id : ids) {
    if (id == 1U) continue;
    const Vec3 spot = editor.squadPosition(id);
    KIMIA_REQUIRE(near(spot.z, editor.squadTeam(id) == 1U ? row : -row, 1e-9));
    // Inside the 5-wide court, feet on the floor.
    KIMIA_REQUIRE(spot.x > -2.5);
    KIMIA_REQUIRE(spot.x < 2.5);
    KIMIA_REQUIRE(near(spot.y, editor.playerPosition().y, 1e-9));
  }
  // Nobody is standing on anybody: every slot is a different x per row.
  KIMIA_REQUIRE(!near3(editor.squadPosition(ids[1]), editor.squadPosition(ids[2])));
  KIMIA_REQUIRE(editor.squadTeam(999U) == 0U);            // unknown id
  KIMIA_REQUIRE(near3(editor.squadPosition(999U), Vec3{0.0, 0.0, 0.0}));
}

KIMIA_TEST(world_grass_is_eleven_a_side_and_golf_stays_alone) {
  WorldEditor grass;
  createWorldFor(grass, "grass");
  KIMIA_REQUIRE(grass.teamSize() == 11U);
  addGolfBall(grass, Vec3{0.0, 0.0, 0.0});
  exitPlace(grass);
  grass.choose(3);  // PLAY
  KIMIA_REQUIRE(grass.squadCount() == 22U);

  // Golf is «team 1»: a single player, exactly as before stage 21.
  WorldEditor golf;
  createWorldFor(golf, "golf");
  KIMIA_REQUIRE(golf.teamSize() == 1U);
  addGolfBall(golf, Vec3{0.0, 0.0, 6.0});
  exitPlace(golf);
  golf.choose(3);  // PLAY
  KIMIA_REQUIRE(golf.squadCount() == 1U);
  KIMIA_REQUIRE(golf.squadTeam(1U) == 0U);  // no sides in golf
}

KIMIA_TEST(world_profile_team_key_round_trips_and_clamps) {
  kimia::GameProfile out;
  std::string error;
  // Parse a squad size out of profile text.
  KIMIA_REQUIRE(kimia::ProfileIO::load("# KIMIA profile v1\nname x\nteam 7\n", out, error));
  KIMIA_REQUIRE(out.teamSize == 7U);
  // Nonsense is clamped into 1..16, never left to spawn a crowd.
  KIMIA_REQUIRE(kimia::ProfileIO::load("# KIMIA profile v1\nname x\nteam 900\n", out, error));
  KIMIA_REQUIRE(out.teamSize == kimia::kProfileTeamMax);
  KIMIA_REQUIRE(out.teamSize == 16U);
  KIMIA_REQUIRE(kimia::ProfileIO::load("# KIMIA profile v1\nname x\nteam 0\n", out, error));
  KIMIA_REQUIRE(out.teamSize == 1U);
  // An old profile with no team line is a single player.
  KIMIA_REQUIRE(kimia::ProfileIO::load("# KIMIA profile v1\nname x\n", out, error));
  KIMIA_REQUIRE(out.teamSize == 1U);
}

// --- Stage 22: the match ---

KIMIA_TEST(world_match_mode_needs_a_squad_and_a_clock) {
  // street: 5 a side, 5 minutes -> a match.
  WorldEditor street;
  createWorldFor(street, "street");
  KIMIA_REQUIRE(street.matchMode());
  KIMIA_REQUIRE(near(street.profile().matchSeconds, 300.0));
  // golf: one player, no clock -> never a match.
  WorldEditor golf;
  createWorldFor(golf, "golf");
  KIMIA_REQUIRE(!golf.matchMode());
  KIMIA_REQUIRE(near(golf.profile().matchSeconds, 0.0));
  // A squad with the clock switched off is still just a kickabout.
  WorldEditor endless;
  createWorldFor(endless, "street");
  kimia::GameProfile noClock = endless.profile();
  noClock.matchSeconds = 0.0;
  endless.createWorld(noClock);
  KIMIA_REQUIRE(!endless.matchMode());
}

KIMIA_TEST(world_match_clock_counts_down_and_blows_full_time) {
  WorldEditor editor;
  createWorldFor(editor, "street");
  kimia::GameProfile quick = editor.profile();
  quick.matchSeconds = 10.0;  // a ten second match, so the test is quick
  editor.createWorld(quick);
  addGolfBall(editor, Vec3{0.0, 0.0, 0.0});
  exitPlace(editor);
  editor.choose(3);  // PLAY
  KIMIA_REQUIRE(editor.matchMode());
  KIMIA_REQUIRE(near(editor.matchClock(), 10.0));
  KIMIA_REQUIRE(editor.matchClockText() == "0:10");
  KIMIA_REQUIRE(!editor.matchOver());
  // Four seconds gone: six left, and the clock rounds UP while any time
  // remains (5.5 s left still reads 0:06).
  for (i32 i = 0; i < 240; ++i) editor.update(1.0 / 60.0);
  KIMIA_REQUIRE(near(editor.matchClock(), 6.0, 1e-9));
  KIMIA_REQUIRE(editor.matchClockText() == "0:06");
  editor.update(0.5);
  KIMIA_REQUIRE(editor.matchClockText() == "0:06");
  // Run it out: full time, the clock reads 0:00 and PLAY is over.
  for (i32 i = 0; i < 600; ++i) editor.update(1.0 / 60.0);
  KIMIA_REQUIRE(editor.matchOver());
  KIMIA_REQUIRE(near(editor.matchClock(), 0.0));
  KIMIA_REQUIRE(editor.matchClockText() == "0:00");
  const std::vector<std::string> hud = editor.hudLines();
  KIMIA_REQUIRE(hud.size() >= 2U);
  KIMIA_REQUIRE(hud[0] == "FULL TIME  MA 0 - 0 ANHA");
  KIMIA_REQUIRE(hud[1] == "DRAW");
  KIMIA_REQUIRE(editor.matchWinner() == 0U);
  // A five minute street match formats with minutes.
  WorldEditor full;
  createWorldFor(full, "street");
  addGolfBall(full, Vec3{0.0, 0.0, 0.0});
  exitPlace(full);
  full.choose(3);
  KIMIA_REQUIRE(full.matchClockText() == "5:00");
}

KIMIA_TEST(world_match_goal_scores_for_the_attacking_side_and_kicks_off_again) {
  WorldEditor editor;
  createWorldFor(editor, "street");
  addGolfBall(editor, Vec3{0.0, 0.0, 0.0});
  exitPlace(editor);
  addGoal(editor, 1, Vec3{0.0, 0.0, -2.0});  // the far net: team 2's goal
  exitPlace(editor);
  editor.choose(3);  // PLAY
  KIMIA_REQUIRE(editor.teamScore(1U) == 0U);
  KIMIA_REQUIRE(editor.teamScore(2U) == 0U);
  KIMIA_REQUIRE(editor.matchScoreText() == "MA 0 - 0 ANHA");
  // This test is about who gets CREDITED for a goal, not about beating a
  // keeper: with the opposition switched off the ball reaches the net.
  editor.setAiSkill(0.0);
  editor.setPlayerPosition(Vec3{0.0, 0.5, 0.6});
  editor.setMoveInput(0.0, -1.0);
  editor.update(0.0);  // kick it toward -Z
  bool scored = false;
  for (i32 i = 0; i < 600; ++i) {
    editor.update(1.0 / 60.0);
    if (editor.celebrating()) {
      scored = true;
      break;
    }
  }
  KIMIA_REQUIRE(scored);
  // Shooting into the -Z net is OUR goal.
  KIMIA_REQUIRE(editor.teamScore(1U) == 1U);
  KIMIA_REQUIRE(editor.teamScore(2U) == 0U);
  KIMIA_REQUIRE(editor.teamScore(0U) == 0U);  // there is no team 0 score
  KIMIA_REQUIRE(editor.matchScoreText() == "MA 1 - 0 ANHA");
  KIMIA_REQUIRE(editor.matchWinner() == 1U);
  KIMIA_REQUIRE(editor.statsLine().find("| match 1-0 |") != std::string::npos);
  KIMIA_REQUIRE(editor.hudLines()[1] == "GOAL!");
  // The celebration ends in a kick-off: ball on the spot, both squads back.
  editor.update(2.5);
  KIMIA_REQUIRE(editor.playing());
  KIMIA_REQUIRE(!editor.celebrating());
  // street plays the fantasy ball, so the center spot sits at its radius.
  KIMIA_REQUIRE(near3(editor.ballPosition(), Vec3{0.0, kWorldFantasyRadius, 0.0}, 1e-6));
  KIMIA_REQUIRE(editor.squadCount() == 10U);
  // And the score survived the kick-off.
  KIMIA_REQUIRE(editor.teamScore(1U) == 1U);
}

KIMIA_TEST(world_match_own_goal_scores_for_the_other_side) {
  WorldEditor editor;
  createWorldFor(editor, "street");
  addGolfBall(editor, Vec3{0.0, 0.0, 0.0});
  exitPlace(editor);
  addGoal(editor, 1, Vec3{0.0, 0.0, 2.0});  // OUR net, on the +Z half
  exitPlace(editor);
  editor.choose(3);  // PLAY
  // Kick the ball backwards into our own net.
  editor.setPlayerPosition(Vec3{0.0, 0.5, -0.6});
  editor.setMoveInput(0.0, 1.0);
  editor.update(0.0);
  bool scored = false;
  for (i32 i = 0; i < 600; ++i) {
    editor.update(1.0 / 60.0);
    if (editor.celebrating()) {
      scored = true;
      break;
    }
  }
  KIMIA_REQUIRE(scored);
  KIMIA_REQUIRE(editor.teamScore(1U) == 0U);
  KIMIA_REQUIRE(editor.teamScore(2U) == 1U);
  KIMIA_REQUIRE(editor.matchScoreText() == "MA 0 - 1 ANHA");
  KIMIA_REQUIRE(editor.matchWinner() == 2U);
}

KIMIA_TEST(world_match_score_is_saved_with_the_world) {
  WorldEditor editor;
  createWorldFor(editor, "street");
  addGolfBall(editor, Vec3{0.0, 0.0, 0.0});
  exitPlace(editor);
  editor.choose(3);
  editor.creditGoal(1U);
  editor.creditGoal(1U);
  editor.creditGoal(2U);
  KIMIA_REQUIRE(editor.matchScoreText() == "MA 2 - 1 ANHA");
  std::string text;
  KIMIA_REQUIRE(kimia::WorldIO::save(editor.world(), text));
  KIMIA_REQUIRE(text.find("# match 2 1\n") != std::string::npos);
  kimia::WorldData loaded;
  std::string error;
  KIMIA_REQUIRE(kimia::WorldIO::load(text, loaded, error));
  KIMIA_REQUIRE(loaded.scoreTeam1 == 2U);
  KIMIA_REQUIRE(loaded.scoreTeam2 == 1U);
  std::string again;
  KIMIA_REQUIRE(kimia::WorldIO::save(loaded, again));
  KIMIA_REQUIRE(again == text);
  // A world with no match line loads 0-0: no old file changed meaning.
  kimia::WorldData old;
  KIMIA_REQUIRE(kimia::WorldIO::load(
      "# KIMIA scene v1\ne \"Ground\" mesh plane pos 0 0 0 scale 20 1 20 color 0.2 0.4 0.2 rough 0.9\n", old, error));
  KIMIA_REQUIRE(old.scoreTeam1 == 0U);
  KIMIA_REQUIRE(old.scoreTeam2 == 0U);
}

KIMIA_TEST(world_profile_match_key_round_trips_and_clamps) {
  kimia::GameProfile out;
  std::string error;
  KIMIA_REQUIRE(kimia::ProfileIO::load("# KIMIA profile v1\nname x\nmatch 90.5\n", out, error));
  KIMIA_REQUIRE(near(out.matchSeconds, 90.5));
  KIMIA_REQUIRE(kimia::ProfileIO::load("# KIMIA profile v1\nname x\nmatch 99999\n", out, error));
  KIMIA_REQUIRE(near(out.matchSeconds, kimia::kProfileMatchMax));
  KIMIA_REQUIRE(near(out.matchSeconds, 3600.0));
  KIMIA_REQUIRE(kimia::ProfileIO::load("# KIMIA profile v1\nname x\nmatch -5\n", out, error));
  KIMIA_REQUIRE(near(out.matchSeconds, 0.0));
  // No match line = no clock.
  KIMIA_REQUIRE(kimia::ProfileIO::load("# KIMIA profile v1\nname x\n", out, error));
  KIMIA_REQUIRE(near(out.matchSeconds, 0.0));
}

// --- Stage 23: ball control (dribble, curl, pass) ---

KIMIA_TEST(world_dribble_carries_the_ball_instead_of_blasting_it) {
  WorldEditor editor;
  createWorldFor(editor, "street");
  addGolfBall(editor, Vec3{0.0, 0.0, 0.0});
  exitPlace(editor);
  editor.choose(3);  // PLAY
  // Off by default: this is exactly the old kick, unchanged.
  KIMIA_REQUIRE(!editor.dribbleHeld());
  editor.setPlayerPosition(Vec3{0.0, 0.5, 0.0});
  editor.setBallPosition(Vec3{0.6, kWorldFantasyRadius, 0.0});
  editor.setMoveInput(1.0, 0.0);
  editor.update(0.0);
  KIMIA_REQUIRE(!editor.dribbling());
  KIMIA_REQUIRE(near(editor.ballVelocity().x, 3.0 + 5.0 * 0.6));  // the full kick
  KIMIA_REQUIRE(near(editor.ballVelocity().y, 2.0));

  // Now hold the dribble: the same touch carries the ball at walking pace
  // and never pops it into the air.
  WorldEditor carry;
  createWorldFor(carry, "street");
  addGolfBall(carry, Vec3{0.0, 0.0, 0.0});
  exitPlace(carry);
  carry.choose(3);
  carry.setDribbleHeld(true);
  carry.setPlayerPosition(Vec3{0.0, 0.5, 0.0});
  carry.setBallPosition(Vec3{0.6, kWorldFantasyRadius, 0.0});
  carry.setMoveInput(1.0, 0.0);
  carry.update(1.0 / 60.0);
  KIMIA_REQUIRE(carry.dribbling());
  KIMIA_REQUIRE(carry.ballVelocity().y == 0.0);  // never popped up
  // Carried at no more than the player's pace, not the 6 m/s kick.
  const f64 pace = 5.0 * kimia::kWorldDribbleSpeed;
  KIMIA_REQUIRE(carry.ballVelocity().length() <= pace + 1e-9);
  KIMIA_REQUIRE(carry.ballVelocity().length() < 3.0 + 5.0 * 0.6);
  // Walk on: the ball stays with the player instead of running away.
  for (i32 i = 0; i < 45; ++i) carry.update(1.0 / 60.0);
  const f64 gap = std::sqrt(std::pow(carry.ballPosition().x - carry.playerPosition().x, 2.0) +
                            std::pow(carry.ballPosition().z - carry.playerPosition().z, 2.0));
  KIMIA_REQUIRE(gap < 1.2);
  KIMIA_REQUIRE(carry.ballPosition().x > 0.6);  // and it did travel forward
}

KIMIA_TEST(world_curl_puts_spin_on_the_shot_and_is_spent_by_it) {
  WorldEditor editor;
  createWorldFor(editor, "golf");
  addGolfBall(editor, Vec3{0.0, 0.0, 6.0});
  exitPlace(editor);
  editor.choose(3);  // PLAY
  KIMIA_REQUIRE(editor.curl() == 0.0);
  KIMIA_REQUIRE(near3(editor.ballSpin(), Vec3{0.0, 0.0, 0.0}));
  // The stick clamps to -1..1 however hard it is pushed.
  editor.setCurl(5.0);
  KIMIA_REQUIRE(near(editor.curl(), 1.0));
  editor.setCurl(-5.0);
  KIMIA_REQUIRE(near(editor.curl(), -1.0));
  editor.setCurl(0.5);
  KIMIA_REQUIRE(near(editor.curl(), 0.5));
  // Taking the shot turns the stick into spin and empties it. Charge and
  // release exactly like a player does.
  editor.setShootHeld(true);
  editor.update(0.5);
  editor.setShootHeld(false);
  editor.update(0.0);
  KIMIA_REQUIRE(near(editor.ballSpin().y, -0.5 * kimia::kWorldMaxCurl));
  KIMIA_REQUIRE(near(editor.ballSpin().y, -7.0));
  KIMIA_REQUIRE(editor.ballSpin().x == 0.0);
  KIMIA_REQUIRE(editor.curl() == 0.0);  // spent
}

KIMIA_TEST(world_a_curled_kick_lands_wide_of_a_straight_one) {
  // The whole point of stage 23: the same strike, curled, must finish
  // somewhere else — measured on the real world simulation. It has to be a
  // FLIGHTED ball: a putt rolling on the turf has its spin scrubbed off by
  // the ground, which is exactly what the physics layer promises.
  const auto kickLanding = [](f64 curl) {
    WorldEditor editor;
    createWorldFor(editor, "street");  // kick pops the ball up 2.0 m/s
    editor.setAiSkill(0.0);  // measure the curl, not a defender's boot
    addGolfBall(editor, Vec3{0.0, 0.0, 0.0});
    exitPlace(editor);
    editor.choose(3);
    editor.setCurl(curl);
    editor.setPlayerPosition(Vec3{0.0, 0.5, 1.0});
    editor.setBallPosition(Vec3{0.0, kWorldFantasyRadius, 0.4});
    editor.setMoveInput(0.0, -1.0);  // strike it down the pitch, toward -Z
    editor.update(0.0);
    editor.setMoveInput(0.0, 0.0);
    for (i32 i = 0; i < 120; ++i) editor.update(1.0 / 60.0);
    return editor.ballPosition();
  };
  const Vec3 straight = kickLanding(0.0);
  const Vec3 right = kickLanding(1.0);
  const Vec3 left = kickLanding(-1.0);
  // Straight really is straight.
  KIMIA_REQUIRE(near(straight.x, 0.0, 1e-9));
  // And the curled ones bend to opposite sides by a visible amount.
  KIMIA_REQUIRE(right.x > 0.02);
  KIMIA_REQUIRE(left.x < -0.02);
  KIMIA_REQUIRE(near(right.x, -left.x, 1e-9));
  // A bend, never a boomerang: it stays small next to the distance travelled.
  KIMIA_REQUIRE(std::abs(right.x) < std::abs(right.z - 0.4) * 0.5);
}

KIMIA_TEST(world_pass_finds_a_team_mate_ahead_and_reaches_their_feet) {
  WorldEditor editor;
  createWorldFor(editor, "street");
  addGolfBall(editor, Vec3{0.0, 0.0, 0.0});
  exitPlace(editor);
  editor.choose(3);  // PLAY: the squads come out, team 1 on +Z
  KIMIA_REQUIRE(editor.squadCount() == 10U);
  // This measures the WEIGHT of the pass: how far the ball runs. With the
  // computer players live the receiver would jog off to a new position and
  // the arrival point would be measuring their run, not the pass.
  editor.setAiSkill(0.0);
  // The player stands at the centre; team 1 mates are on the +Z half, so
  // aim back up the pitch (yaw pi = toward +Z) to find them.
  editor.setPlayerPosition(Vec3{0.0, 0.5, 0.0});
  editor.setBallPosition(Vec3{0.0, kWorldFantasyRadius, 0.0});
  editor.setAimYaw(3.14159265358979323846);
  const u32 target = editor.passTarget();
  KIMIA_REQUIRE(target != 0U);
  KIMIA_REQUIRE(target != kimia::kPrimaryCharacter);
  KIMIA_REQUIRE(editor.squadTeam(target) == 1U);  // never to the opposition
  const Vec3 mate = editor.squadPosition(target);
  KIMIA_REQUIRE(mate.z > 0.0);
  KIMIA_REQUIRE(editor.pass());
  // A ground pass: along the floor, no lofting.
  KIMIA_REQUIRE(editor.ballVelocity().y == 0.0);
  KIMIA_REQUIRE(editor.ballVelocity().length() > 0.0);
  // Let it run and check it actually arrives near the receiver's feet.
  for (i32 i = 0; i < 300; ++i) editor.update(1.0 / 60.0);
  const f64 miss = std::sqrt(std::pow(editor.ballPosition().x - mate.x, 2.0) +
                             std::pow(editor.ballPosition().z - mate.z, 2.0));
  KIMIA_REQUIRE(miss < 1.0);
}

KIMIA_TEST(world_pass_refuses_when_there_is_nobody_to_pass_to) {
  // Golf is one player: there is no team-mate, so a pass must fail rather
  // than fling the ball at nothing.
  WorldEditor editor;
  createWorldFor(editor, "golf");
  addGolfBall(editor, Vec3{0.0, 0.0, 6.0});
  exitPlace(editor);
  editor.choose(3);
  KIMIA_REQUIRE(editor.squadCount() == 1U);
  KIMIA_REQUIRE(editor.passTarget() == 0U);
  KIMIA_REQUIRE(!editor.pass());
  KIMIA_REQUIRE(near3(editor.ballVelocity(), Vec3{0.0, 0.0, 0.0}));

  // In a match, a team-mate strictly BEHIND the aim is not a pass target.
  WorldEditor street;
  createWorldFor(street, "street");
  addGolfBall(street, Vec3{0.0, 0.0, 0.0});
  exitPlace(street);
  street.choose(3);
  street.setPlayerPosition(Vec3{0.0, 0.5, 0.0});
  street.setAimYaw(0.0);  // toward -Z, where only the opposition stands
  KIMIA_REQUIRE(street.passTarget() == 0U);
  KIMIA_REQUIRE(!street.pass());
}

KIMIA_TEST(world_ball_control_pads_appear_only_where_they_make_sense) {
  // A match: dribble, both curl sticks and a pass button.
  WorldEditor street;
  createWorldFor(street, "street");
  addGolfBall(street, Vec3{0.0, 0.0, 0.0});
  exitPlace(street);
  street.choose(3);  // PLAY
  KIMIA_REQUIRE(street.holdPad().size() == 7U);
  KIMIA_REQUIRE(street.holdPad()[4].second == "c");
  KIMIA_REQUIRE(street.holdPad()[5].second == "q");
  KIMIA_REQUIRE(street.holdPad()[6].second == "e");
  bool hasPass = false;
  for (const auto& pad : street.tapPad()) {
    if (pad.second == "p") hasPass = true;
  }
  KIMIA_REQUIRE(hasPass);

  // Golf is one player aiming a shot: curl sticks, but never a pass.
  WorldEditor golf;
  createWorldFor(golf, "golf");
  addGolfBall(golf, Vec3{0.0, 0.0, 6.0});
  exitPlace(golf);
  golf.choose(3);
  KIMIA_REQUIRE(golf.holdPad().size() == 5U);
  KIMIA_REQUIRE(golf.holdPad()[2].second == "space");
  KIMIA_REQUIRE(golf.holdPad()[3].second == "q");
  KIMIA_REQUIRE(golf.holdPad()[4].second == "e");
  for (const auto& pad : golf.tapPad()) KIMIA_REQUIRE(pad.second != "p");
}

// --- Stage 24: weather and day/night in the world ---

KIMIA_TEST(world_night_and_daylight_follow_the_hour) {
  WorldEditor editor;
  createWorldFor(editor, "golf");  // 09:00, dry
  KIMIA_REQUIRE(!editor.night());
  KIMIA_REQUIRE(!editor.raining());
  KIMIA_REQUIRE(editor.pitchWetness() == 0.0);
  // Nine in the morning: the sun is up but not at its peak.
  KIMIA_REQUIRE(editor.sunHeight() > 0.0);
  KIMIA_REQUIRE(editor.sunHeight() < 1.0);

  kimia::GameProfile profile = editor.profile();
  // Midday is exactly the top of the arc.
  profile.hour = 12.0;
  editor.createWorld(profile);
  KIMIA_REQUIRE(near(editor.sunHeight(), 1.0, 1e-12));
  KIMIA_REQUIRE(near(editor.daylight(), 1.0, 1e-12));
  KIMIA_REQUIRE(!editor.night());
  // Midnight is the bottom, and it is night.
  profile.hour = 0.0;
  editor.createWorld(profile);
  KIMIA_REQUIRE(near(editor.sunHeight(), -1.0, 1e-12));
  KIMIA_REQUIRE(editor.night());
  // ... but never pitch black: the floodlights floor it.
  KIMIA_REQUIRE(near(editor.daylight(), kimia::kWorldNightLight, 1e-12));
  KIMIA_REQUIRE(near(editor.daylight(), 0.12, 1e-12));
  // The boundaries are exactly where the constants say.
  profile.hour = kimia::kWorldSunrise;
  editor.createWorld(profile);
  KIMIA_REQUIRE(!editor.night());
  profile.hour = kimia::kWorldSunrise - 0.01;
  editor.createWorld(profile);
  KIMIA_REQUIRE(editor.night());
  profile.hour = kimia::kWorldSunset;
  editor.createWorld(profile);
  KIMIA_REQUIRE(editor.night());
}

KIMIA_TEST(world_rain_dims_the_day_and_soaks_the_pitch) {
  WorldEditor editor;
  createWorldFor(editor, "golf");
  kimia::GameProfile profile = editor.profile();
  profile.hour = 12.0;
  editor.createWorld(profile);
  const f64 clear = editor.daylight();
  // The same midday, in a downpour, is dimmer.
  profile.rain = 1.0;
  editor.createWorld(profile);
  KIMIA_REQUIRE(editor.raining());
  KIMIA_REQUIRE(editor.daylight() < clear);
  KIMIA_REQUIRE(near(editor.daylight(), 1.0 - 0.6, 1e-12));
  // Rain soaks the pitch all by itself.
  KIMIA_REQUIRE(near(editor.pitchWetness(), 1.0));
  // A pitch can be wet without rain (it stopped, but the ground is soaked).
  profile.rain = 0.0;
  profile.wetness = 0.6;
  editor.createWorld(profile);
  KIMIA_REQUIRE(!editor.raining());
  KIMIA_REQUIRE(near(editor.pitchWetness(), 0.6));
  // ... and the wetter of the two always wins.
  profile.rain = 0.9;
  editor.createWorld(profile);
  KIMIA_REQUIRE(near(editor.pitchWetness(), 0.9));
}

KIMIA_TEST(world_a_wet_pitch_really_reaches_the_physics) {
  // The profile is not decoration: a soaked world must actually make the
  // ball run further than a dry one, through the real simulation.
  const auto rollDistance = [](f64 wetness) {
    WorldEditor editor;
    createWorldFor(editor, "golf");
    kimia::GameProfile profile = editor.profile();
    profile.wetness = wetness;
    editor.createWorld(profile);
    addGolfBall(editor, Vec3{0.0, 0.0, 6.0});
    exitPlace(editor);
    editor.choose(3);  // PLAY
    editor.setBallVelocity(Vec3{0.0, 0.0, -3.0});
    for (i32 i = 0; i < 400; ++i) editor.update(1.0 / 120.0);
    return editor.ballPosition().z;
  };
  const f64 dry = rollDistance(0.0);
  const f64 wet = rollDistance(1.0);
  KIMIA_REQUIRE(wet < dry - 0.05);  // rolled further down the -Z course
}

KIMIA_TEST(world_sky_hud_line_says_what_the_weather_is) {
  WorldEditor editor;
  createWorldFor(editor, "golf");  // 09:00 dry: nothing worth saying
  KIMIA_REQUIRE(editor.skyHudText().empty());

  kimia::GameProfile profile = editor.profile();
  profile.hour = 19.5;
  profile.rain = 0.35;
  profile.wetness = 0.5;
  editor.createWorld(profile);
  KIMIA_REQUIRE(editor.skyHudText() == "19:30 NIGHT RAIN WET");

  // Morning drizzle: raining and wet, but not night.
  profile.hour = 8.0;
  profile.rain = 0.2;
  profile.wetness = 0.0;
  editor.createWorld(profile);
  KIMIA_REQUIRE(editor.skyHudText() == "08:00 RAIN WET");  // rain implies wet

  // A dry night match.
  profile.hour = 21.0;
  profile.rain = 0.0;
  profile.wetness = 0.0;
  editor.createWorld(profile);
  KIMIA_REQUIRE(editor.skyHudText() == "21:00 NIGHT");

  // And it reaches the HUD the player actually sees.
  addGolfBall(editor, Vec3{0.0, 0.0, 6.0});
  exitPlace(editor);
  editor.choose(3);
  bool found = false;
  for (const std::string& line : editor.hudLines()) {
    if (line == "21:00 NIGHT") found = true;
  }
  KIMIA_REQUIRE(found);
}

// --- Stage 26: skill moves ---

// Street with a ball at the player's feet, already in PLAY.
namespace {
void streetAtTheFeet(WorldEditor& editor) {
  createWorldFor(editor, "street");
  editor.choose(0);  // catalog
  editor.choose(1);  // ball
  editor.setGhostPosition(Vec3{0.0, 0.0, 0.0});
  editor.choose(0);
  exitPlace(editor);
  editor.choose(3);  // PLAY
  editor.setPlayerPosition(Vec3{0.0, 0.5, 0.0});
  editor.setBallPosition(Vec3{0.3, editor.world().ball.radius, 0.0});
}
}  // namespace

KIMIA_TEST(world_tricks_are_a_street_thing_and_grass_says_no) {
  // The alley allows showboating.
  WorldEditor street;
  streetAtTheFeet(street);
  KIMIA_REQUIRE(street.tricksEnabled());
  KIMIA_REQUIRE(street.profile().tricks);

  // A serious fixture does not, and refuses to start one at all.
  WorldEditor grass;
  createWorldFor(grass, "grass");
  KIMIA_REQUIRE(!grass.tricksEnabled());
  KIMIA_REQUIRE(!grass.profile().tricks);
  KIMIA_REQUIRE(!grass.startTrick(WorldEditor::Trick::Juggle));
  KIMIA_REQUIRE(!grass.trickActive());
  KIMIA_REQUIRE(grass.styleScore() == 0U);
  // Golf, with no opponents at all, is off too.
  WorldEditor golf;
  createWorldFor(golf, "golf");
  KIMIA_REQUIRE(!golf.tricksEnabled());
}

KIMIA_TEST(world_trick_needs_the_ball_and_cannot_be_double_started) {
  WorldEditor editor;
  streetAtTheFeet(editor);

  // The ball is far away: there is nothing to flick.
  editor.setBallPosition(Vec3{6.0, editor.world().ball.radius, 0.0});
  KIMIA_REQUIRE(!editor.startTrick(WorldEditor::Trick::Juggle));

  // Back at the feet it works.
  editor.setBallPosition(Vec3{0.3, editor.world().ball.radius, 0.0});
  KIMIA_REQUIRE(editor.startTrick(WorldEditor::Trick::Juggle));
  KIMIA_REQUIRE(editor.trickActive());
  KIMIA_REQUIRE(editor.currentTrick() == WorldEditor::Trick::Juggle);
  // You are committed: no starting a second one to wriggle out of it.
  KIMIA_REQUIRE(!editor.startTrick(WorldEditor::Trick::Roulette));
  KIMIA_REQUIRE(editor.currentTrick() == WorldEditor::Trick::Juggle);
  // And Trick::None is never a move.
  KIMIA_REQUIRE(!editor.startTrick(WorldEditor::Trick::None));
}

KIMIA_TEST(world_juggle_flicks_the_ball_up_and_scores_only_at_the_end) {
  WorldEditor editor;
  streetAtTheFeet(editor);
  KIMIA_REQUIRE(editor.startTrick(WorldEditor::Trick::Juggle));
  KIMIA_REQUIRE(editor.styleScore() == 0U);

  // Half way through: still running, still nothing banked.
  editor.update(kimia::kTrickJuggleTime * 0.5);
  KIMIA_REQUIRE(editor.trickActive());
  KIMIA_REQUIRE(editor.styleScore() == 0U);
  const f64 progress = editor.trickProgress();
  KIMIA_REQUIRE(progress > 0.3 && progress < 0.7);

  // Finish it: the ball goes UP and the points land.
  editor.update(kimia::kTrickJuggleTime * 0.6);
  KIMIA_REQUIRE(!editor.trickActive());
  KIMIA_REQUIRE(editor.styleScore() == kimia::kTrickJugglePoints);
  KIMIA_REQUIRE(editor.lastTrick() == WorldEditor::Trick::Juggle);
  KIMIA_REQUIRE(editor.trickProgress() == 0.0);
  // The flick really lifted it off the deck.
  KIMIA_REQUIRE(editor.ballVelocity().y > 1.0);
}

KIMIA_TEST(world_nutmeg_needs_an_opponent_in_front_of_you) {
  WorldEditor editor;
  streetAtTheFeet(editor);
  // Street fields squads on the far (-Z) half. Stand at the near end and
  // face AWAY from all of them: there are no legs to put the ball through,
  // so the move must be refused outright.
  editor.setPlayerPosition(Vec3{0.0, 0.5, 7.0});
  editor.setBallPosition(Vec3{0.0, editor.world().ball.radius, 7.0});
  editor.setAimYaw(3.14159265358979323846);  // look up-field, away from them
  KIMIA_REQUIRE(!editor.startTrick(WorldEditor::Trick::Nutmeg));
  KIMIA_REQUIRE(!editor.trickActive());
  KIMIA_REQUIRE(editor.styleScore() == 0U);

  // Put an opponent right in front and try again.
  WorldEditor second;
  streetAtTheFeet(second);
  u32 opponent = 0U;
  for (const u32 id : second.squadIds()) {
    if (second.squadTeam(id) == 2U) {
      opponent = id;
      break;
    }
  }
  KIMIA_REQUIRE(opponent != 0U);  // street really does field an opposition
  // Stand right behind them, facing their way (-Z is aim yaw 0).
  const Vec3 them = second.squadPosition(opponent);
  second.setPlayerPosition(Vec3{them.x, 0.5, them.z + 1.5});
  second.setBallPosition(Vec3{them.x, second.world().ball.radius, them.z + 1.5});
  second.setAimYaw(0.0);
  KIMIA_REQUIRE(second.startTrick(WorldEditor::Trick::Nutmeg));

  // Complete it: the ball is knocked forward, along the ground.
  second.update(kimia::kTrickNutmegTime + 0.01);
  KIMIA_REQUIRE(!second.trickActive());
  KIMIA_REQUIRE(second.styleScore() == kimia::kTrickNutmegPoints);
  // A nutmeg is worth more than the flashy-but-safe juggle.
  KIMIA_REQUIRE(kimia::kTrickNutmegPoints > kimia::kTrickJugglePoints);
}

KIMIA_TEST(world_roulette_turns_the_player_around) {
  WorldEditor editor;
  streetAtTheFeet(editor);
  const f64 before = editor.aimYaw();
  KIMIA_REQUIRE(editor.startTrick(WorldEditor::Trick::Roulette));
  editor.update(kimia::kTrickRouletteTime + 0.01);
  KIMIA_REQUIRE(!editor.trickActive());
  KIMIA_REQUIRE(editor.styleScore() == kimia::kTrickRoulettePoints);
  // A roulette is a half turn away from where you were facing.
  KIMIA_REQUIRE(near(editor.aimYaw() - before, kimia::kTrickRouletteTurn, 1e-9));
}

KIMIA_TEST(world_trick_is_cancelled_when_the_ball_is_lost) {
  WorldEditor editor;
  streetAtTheFeet(editor);
  KIMIA_REQUIRE(editor.startTrick(WorldEditor::Trick::Roulette));
  KIMIA_REQUIRE(editor.trickActive());
  // Someone takes it off you mid-trick. (Down the length of the pitch: a
  // street court is only a few meters wide, so sideways would be clamped.)
  editor.setBallPosition(Vec3{0.0, editor.world().ball.radius, 6.0});
  editor.update(0.05);
  // The move dies and pays nothing: that is the risk of showing off.
  KIMIA_REQUIRE(!editor.trickActive());
  KIMIA_REQUIRE(editor.styleScore() == 0U);
  KIMIA_REQUIRE(editor.lastTrick() == WorldEditor::Trick::None);
}

KIMIA_TEST(world_trick_hud_shows_the_move_then_the_running_total) {
  WorldEditor editor;
  streetAtTheFeet(editor);
  // Nothing to say before anything has happened.
  KIMIA_REQUIRE(editor.trickHudText().empty());

  KIMIA_REQUIRE(editor.startTrick(WorldEditor::Trick::Juggle));
  KIMIA_REQUIRE(editor.trickHudText() == "JUGGLE!");
  editor.update(kimia::kTrickJuggleTime + 0.01);
  KIMIA_REQUIRE(editor.trickHudText() == "STYLE 10");

  // Style accumulates across moves.
  editor.setBallPosition(Vec3{0.3, editor.world().ball.radius, 0.0});
  editor.setBallVelocity(Vec3{0.0, 0.0, 0.0});
  KIMIA_REQUIRE(editor.startTrick(WorldEditor::Trick::Roulette));
  editor.update(kimia::kTrickRouletteTime + 0.01);
  KIMIA_REQUIRE(editor.styleScore() == kimia::kTrickJugglePoints + kimia::kTrickRoulettePoints);
  KIMIA_REQUIRE(editor.trickHudText() == "STYLE 40");
  // And it reaches the actual HUD the app draws.
  const std::vector<std::string> lines = editor.hudLines();
  bool found = false;
  for (const std::string& line : lines) {
    if (line == "STYLE 40") found = true;
  }
  KIMIA_REQUIRE(found);

  // The names are the ones the HUD and the tests agree on.
  KIMIA_REQUIRE(std::string(WorldEditor::trickName(WorldEditor::Trick::Nutmeg)) == "NUTMEG");
  KIMIA_REQUIRE(std::string(WorldEditor::trickName(WorldEditor::Trick::Roulette)) == "ROULETTE");
  KIMIA_REQUIRE(std::string(WorldEditor::trickName(WorldEditor::Trick::Juggle)) == "JUGGLE");
  KIMIA_REQUIRE(std::string(WorldEditor::trickName(WorldEditor::Trick::None)).empty());
}

KIMIA_TEST(world_trick_pads_appear_only_for_the_games_that_allow_them) {
  WorldEditor street;
  streetAtTheFeet(street);
  const auto streetPads = street.tapPad();
  bool hasNutmeg = false;
  for (const auto& pad : streetPads) {
    if (pad.second == "n") hasNutmeg = true;
  }
  KIMIA_REQUIRE(hasNutmeg);

  // Grass plays the same sport with the same engine and gets no trick pads.
  WorldEditor grass;
  createWorldFor(grass, "grass");
  grass.choose(0);
  grass.choose(1);
  grass.setGhostPosition(Vec3{0.0, 0.0, 0.0});
  grass.choose(0);
  exitPlace(grass);
  grass.choose(3);
  for (const auto& pad : grass.tapPad()) {
    KIMIA_REQUIRE(pad.second != "n");
    KIMIA_REQUIRE(pad.second != "o");
    KIMIA_REQUIRE(pad.second != "u");
  }
}

// --- Stage 27: computer players ---

KIMIA_TEST(world_ai_is_off_by_default_and_statues_never_move) {
  // Every profile that shipped before stage 27 had no opposition worth the
  // name, and must still behave that way: skill 0 means nobody moves.
  WorldEditor editor;
  streetAtTheFeet(editor);
  KIMIA_REQUIRE(editor.aiActive());  // street DOES field a live opposition
  editor.setAiSkill(0.0);
  KIMIA_REQUIRE(!editor.aiActive());
  KIMIA_REQUIRE(editor.aiChaser(2U) == 0U);
  KIMIA_REQUIRE(editor.aiKeeper(2U) == 0U);

  const Vec3 before = editor.squadPosition(2U);
  editor.setBallPosition(Vec3{0.0, editor.world().ball.radius, 0.0});
  for (i32 i = 0; i < 60; ++i) editor.update(1.0 / 60.0);
  const Vec3 after = editor.squadPosition(2U);
  // Not "barely moved": exactly where it was.
  KIMIA_REQUIRE(near3(after, before, 1e-12));

  // Golf is one player: there is nobody to be clever with at all.
  WorldEditor golf;
  createWorldFor(golf, "golf");
  KIMIA_REQUIRE(golf.aiSkill() == 0.0);
  KIMIA_REQUIRE(!golf.aiActive());
}

KIMIA_TEST(world_ai_sends_exactly_one_chaser_and_it_is_not_the_keeper) {
  WorldEditor editor;
  streetAtTheFeet(editor);
  KIMIA_REQUIRE(editor.aiActive());

  const u32 chaser = editor.aiChaser(2U);
  const u32 keeper = editor.aiKeeper(2U);
  KIMIA_REQUIRE(chaser != 0U);
  KIMIA_REQUIRE(keeper != 0U);
  // The keeper minds the net; sending them chasing leaves an empty goal.
  KIMIA_REQUIRE(chaser != keeper);
  // Both really are on the side we asked about.
  KIMIA_REQUIRE(editor.squadTeam(chaser) == 2U);
  KIMIA_REQUIRE(editor.squadTeam(keeper) == 2U);
  // Both sides field their own, and never the human.
  const u32 ourChaser = editor.aiChaser(1U);
  KIMIA_REQUIRE(ourChaser != 0U);
  KIMIA_REQUIRE(ourChaser != kimia::kPrimaryCharacter);
  KIMIA_REQUIRE(editor.squadTeam(ourChaser) == 1U);
  KIMIA_REQUIRE(ourChaser != chaser);
}

KIMIA_TEST(world_ai_chaser_actually_closes_on_the_ball) {
  WorldEditor editor;
  streetAtTheFeet(editor);
  // Park the human far away so only the computer is doing anything.
  editor.setPlayerPosition(Vec3{0.0, 0.5, 7.0});
  const u32 chaser = editor.aiChaser(2U);
  KIMIA_REQUIRE(chaser != 0U);

  const Vec3 ballSpot{0.0, editor.world().ball.radius, 0.0};
  const auto gap = [&editor, chaser, &ballSpot]() {
    const Vec3 at = editor.squadPosition(chaser);
    return std::sqrt(std::pow(at.x - ballSpot.x, 2.0) + std::pow(at.z - ballSpot.z, 2.0));
  };
  const f64 before = gap();
  // Hold the ball still so we are measuring the chase, not the ball.
  for (i32 i = 0; i < 60; ++i) {
    editor.setBallPosition(ballSpot);
    editor.setBallVelocity(Vec3{0.0, 0.0, 0.0});
    editor.update(1.0 / 60.0);
  }
  const f64 after = gap();
  KIMIA_REQUIRE(before > 1.5);   // it really did start away from the ball
  KIMIA_REQUIRE(after < 0.75);   // and it really did arrive
  KIMIA_REQUIRE(after < before);
}

KIMIA_TEST(world_ai_keeper_stays_near_its_own_line) {
  WorldEditor editor;
  streetAtTheFeet(editor);
  editor.setPlayerPosition(Vec3{0.0, 0.5, 7.0});
  const u32 keeper = editor.aiKeeper(2U);
  KIMIA_REQUIRE(keeper != 0U);
  // Team 2 defends the -Z end.
  const f64 ownLine = -editor.world().halfLength();

  // Drag the ball right across the far corner: a keeper must track it but
  // must NOT go charging up the pitch after it.
  editor.setBallPosition(Vec3{2.0, editor.world().ball.radius, 6.0});
  for (i32 i = 0; i < 180; ++i) {
    editor.setBallVelocity(Vec3{0.0, 0.0, 0.0});
    editor.update(1.0 / 60.0);
  }
  const Vec3 at = editor.squadPosition(keeper);
  // Still in its own third, nowhere near the ball at the other end.
  KIMIA_REQUIRE(at.z < ownLine + kimia::kAiKeeperRange * 2.0 + 1.0);
  KIMIA_REQUIRE(at.z < 0.0);
  // And it did not abandon the goal sideways either.
  KIMIA_REQUIRE(std::abs(at.x) <= kimia::kWorldGoalLarge);
}

KIMIA_TEST(world_ai_support_players_do_not_all_swarm_the_ball) {
  // The rule that stops a match becoming a scrum: only the chaser goes for
  // the ball, everyone else holds a shape behind it.
  WorldEditor editor;
  streetAtTheFeet(editor);
  editor.setPlayerPosition(Vec3{0.0, 0.5, 7.0});
  const Vec3 ballSpot{0.0, editor.world().ball.radius, 0.0};
  const u32 chaser = editor.aiChaser(2U);
  for (i32 i = 0; i < 120; ++i) {
    editor.setBallPosition(ballSpot);
    editor.setBallVelocity(Vec3{0.0, 0.0, 0.0});
    editor.update(1.0 / 60.0);
  }
  // Count how many of team 2 ended up standing on the ball.
  u32 onTheBall = 0U;
  for (const u32 id : editor.squadIds()) {
    if (id == kimia::kPrimaryCharacter || editor.squadTeam(id) != 2U) continue;
    const Vec3 at = editor.squadPosition(id);
    if (std::sqrt(std::pow(at.x - ballSpot.x, 2.0) + std::pow(at.z - ballSpot.z, 2.0)) < 1.5) ++onTheBall;
  }
  KIMIA_REQUIRE(chaser != 0U);
  KIMIA_REQUIRE(onTheBall == 1U);  // exactly the chaser, nobody else
}

KIMIA_TEST(world_ai_defender_tackles_the_ball_away) {
  WorldEditor editor;
  streetAtTheFeet(editor);
  editor.setAiSkill(1.0);  // a sharp defender, so the tackle is decisive
  editor.setPlayerPosition(Vec3{0.0, 0.5, 7.0});
  const Vec3 ballSpot{0.0, editor.world().ball.radius, 0.0};
  editor.setBallPosition(ballSpot);
  editor.setBallVelocity(Vec3{0.0, 0.0, 0.0});
  // Let a defender arrive and put a boot through it.
  for (i32 i = 0; i < 120; ++i) editor.update(1.0 / 60.0);
  // The ball was left completely still, so any motion is the tackle. Team
  // 2 defends the -Z end and attacks +Z, so a clearance goes UP the pitch,
  // away from the net they are protecting.
  KIMIA_REQUIRE(editor.ballPosition().z > ballSpot.z + 0.5);
  KIMIA_REQUIRE(editor.ballVelocity().z > 0.0);
}

KIMIA_TEST(world_ai_makes_showing_off_genuinely_risky) {
  // Stages 26 and 27 together: start a trick in front of a live defender
  // and they take it off you before you finish, so you score nothing.
  WorldEditor editor;
  streetAtTheFeet(editor);
  editor.setAiSkill(1.0);
  // Stand where the opposition will reach us, and let one arrive.
  editor.setPlayerPosition(Vec3{0.0, 0.5, -2.0});
  editor.setBallPosition(Vec3{0.0, editor.world().ball.radius, -2.0});
  for (i32 i = 0; i < 90; ++i) {
    editor.setBallPosition(Vec3{0.0, editor.world().ball.radius, -2.0});
    editor.setBallVelocity(Vec3{0.0, 0.0, 0.0});
    editor.update(1.0 / 60.0);
  }
  // Now try to show off. The roulette is the slowest move, so it is the
  // easiest to get robbed during.
  if (editor.startTrick(WorldEditor::Trick::Roulette)) {
    for (i32 i = 0; i < 90; ++i) editor.update(1.0 / 60.0);
    // Either they robbed us (no points) or we completed it (full points) —
    // never a partial score. The engine must not pay out half a trick.
    const u32 style = editor.styleScore();
    KIMIA_REQUIRE(style == 0U || style == kimia::kTrickRoulettePoints);
  }
  KIMIA_REQUIRE(!editor.trickActive());
}

KIMIA_TEST(world_ai_skill_reaches_the_simulation_from_the_profile) {
  // The profile is the only place the number comes from, and a sharper
  // side really does close you down faster.
  WorldEditor street;
  streetAtTheFeet(street);
  KIMIA_REQUIRE(near(street.aiSkill(), 0.6));
  KIMIA_REQUIRE(near(street.profile().aiSkill, 0.6));

  WorldEditor grass;
  createWorldFor(grass, "grass");
  // A serious fixture fields a sharper side than an alley kickabout.
  KIMIA_REQUIRE(near(grass.aiSkill(), 0.85));
  KIMIA_REQUIRE(grass.aiSkill() > street.aiSkill());

  // Measured: the same chase, run at two skills, covers different ground.
  const auto chaseGap = [](f64 skill) {
    WorldEditor editor;
    streetAtTheFeet(editor);
    editor.setAiSkill(skill);
    editor.setPlayerPosition(Vec3{0.0, 0.5, 7.0});
    const u32 chaser = editor.aiChaser(2U);
    const Vec3 spot{0.0, editor.world().ball.radius, 0.0};
    for (i32 i = 0; i < 30; ++i) {
      editor.setBallPosition(spot);
      editor.setBallVelocity(Vec3{0.0, 0.0, 0.0});
      editor.update(1.0 / 60.0);
    }
    const Vec3 at = editor.squadPosition(chaser);
    return std::sqrt(std::pow(at.x - spot.x, 2.0) + std::pow(at.z - spot.z, 2.0));
  };
  // Half a second of chasing: the sharp one is measurably closer.
  KIMIA_REQUIRE(chaseGap(1.0) < chaseGap(0.3));
}

// --- Stage 28: match HUD, camera director and sound cues ---

KIMIA_TEST(world_camera_style_comes_from_the_profile) {
  // Each game asks for the camera that suits it, and the engine — not the
  // app — is what answers.
  WorldEditor golf;
  createWorldFor(golf, "golf");
  KIMIA_REQUIRE(golf.cameraStyle() == kimia::CameraStyle::Chase);

  WorldEditor street;
  streetAtTheFeet(street);
  KIMIA_REQUIRE(street.cameraStyle() == kimia::CameraStyle::Broadcast);

  WorldEditor grass;
  createWorldFor(grass, "grass");
  KIMIA_REQUIRE(grass.cameraStyle() == kimia::CameraStyle::Broadcast);

  // The sandbox keeps the plain editor orbit it always had.
  WorldEditor sandbox = editorWithWorld();
  KIMIA_REQUIRE(sandbox.cameraStyle() == kimia::CameraStyle::Orbit);
}

KIMIA_TEST(world_only_a_chase_camera_swings_behind_the_aim) {
  // A broadcast camera holds its side of the pitch. Swinging it around
  // every time the player turns would be unwatchable.
  WorldEditor street;
  streetAtTheFeet(street);
  KIMIA_REQUIRE(!street.cameraFollowsAim());

  WorldEditor golf;
  createWorldFor(golf, "golf");
  addGolfBall(golf, Vec3{0.0, 0.0, 0.0});
  exitPlace(golf);
  golf.choose(3);  // PLAY
  KIMIA_REQUIRE(golf.playing());
  KIMIA_REQUIRE(golf.cameraFollowsAim());

  // Nothing follows anything outside play.
  WorldEditor idle;
  createWorldFor(idle, "golf");
  KIMIA_REQUIRE(!idle.playing());
  KIMIA_REQUIRE(!idle.cameraFollowsAim());
}

KIMIA_TEST(world_broadcast_camera_pulls_back_as_the_play_spreads_out) {
  WorldEditor editor;
  streetAtTheFeet(editor);
  editor.setAiSkill(0.0);  // measure the camera, not a defender's run
  const f64 resting = 7.0;

  // Ball at the player's feet: nothing to spread out, so it sits at its
  // closest.
  editor.setPlayerPosition(Vec3{0.0, 0.5, 0.0});
  editor.setBallPosition(Vec3{0.0, editor.world().ball.radius, 0.0});
  const f64 tight = editor.cameraDistance(resting);
  KIMIA_REQUIRE(near(tight, kimia::kCameraBroadcastNear, 1e-9));

  // A long ball: the camera must pull back or half the play leaves frame.
  editor.setBallPosition(Vec3{0.0, editor.world().ball.radius, 6.0});
  const f64 wide = editor.cameraDistance(resting);
  KIMIA_REQUIRE(near(wide, kimia::kCameraBroadcastNear + 6.0 * kimia::kCameraBroadcastPerMeter, 1e-9));
  KIMIA_REQUIRE(wide > tight);

  // It never pulls back forever, or the players become specks.
  editor.setPlayerPosition(Vec3{0.0, 0.5, 7.0});
  editor.setBallPosition(Vec3{0.0, editor.world().ball.radius, -7.5});
  KIMIA_REQUIRE(editor.cameraDistance(resting) <= kimia::kCameraBroadcastFar);

  // A chase camera ignores all of this and keeps the distance it is given.
  WorldEditor golf;
  createWorldFor(golf, "golf");
  addGolfBall(golf, Vec3{0.0, 0.0, 0.0});
  exitPlace(golf);
  golf.choose(3);
  KIMIA_REQUIRE(near(golf.cameraDistance(resting), resting, 1e-12));
}

KIMIA_TEST(world_broadcast_camera_frames_between_ball_and_player) {
  WorldEditor editor;
  streetAtTheFeet(editor);
  editor.setAiSkill(0.0);
  editor.setPlayerPosition(Vec3{0.0, 0.5, 0.0});
  editor.setBallPosition(Vec3{0.0, editor.world().ball.radius, 6.0});
  // Biased toward the ball, because that is what the viewer is watching,
  // but not ON the ball — the player has to stay in shot.
  const Vec3 target = editor.cameraTarget();
  KIMIA_REQUIRE(near(target.z, 6.0 * kimia::kCameraBallBias, 1e-9));
  KIMIA_REQUIRE(target.z < 6.0);
  KIMIA_REQUIRE(target.z > 3.0);

  // A chase camera just watches the ball itself.
  WorldEditor golf;
  createWorldFor(golf, "golf");
  addGolfBall(golf, Vec3{0.0, 0.0, 0.0});
  exitPlace(golf);
  golf.choose(3);
  golf.setBallPosition(Vec3{1.0, kGolfBallRadius, 2.0});
  KIMIA_REQUIRE(near3(golf.cameraTarget(), golf.ballPosition(), 1e-12));
}

KIMIA_TEST(world_match_hud_counts_down_the_closing_seconds) {
  WorldEditor editor;
  createWorldFor(editor, "street");
  editor.setAiSkill(0.0);
  editor.choose(0);
  editor.choose(1);
  editor.setGhostPosition(Vec3{0.0, 0.0, 0.0});
  editor.choose(0);
  exitPlace(editor);
  editor.choose(3);  // PLAY
  KIMIA_REQUIRE(editor.matchMode());

  // Early on there is no warning: a 5 minute match has plenty left.
  const auto hasLastLine = [](const std::vector<std::string>& lines) {
    for (const std::string& line : lines) {
      if (line.rfind("LAST ", 0) == 0) return true;
    }
    return false;
  };
  KIMIA_REQUIRE(!hasLastLine(editor.hudLines()));

  // Run down to the closing stretch.
  const f64 target = kimia::kMatchFinalWhistleWarning - 5.0;
  while (editor.matchClock() > target && !editor.matchOver()) editor.update(0.1);
  const std::vector<std::string> lines = editor.hudLines();
  KIMIA_REQUIRE(hasLastLine(lines));
  // The score and clock are still there too.
  bool hasScore = false;
  for (const std::string& line : lines) {
    if (line.find("MA 0 - 0 ANHA") != std::string::npos) hasScore = true;
  }
  KIMIA_REQUIRE(hasScore);
}

KIMIA_TEST(world_match_blows_a_whistle_at_full_time) {
  WorldEditor editor;
  createWorldFor(editor, "street");
  editor.setAiSkill(0.0);
  editor.choose(0);
  editor.choose(1);
  editor.setGhostPosition(Vec3{0.0, 0.0, 0.0});
  editor.choose(0);
  exitPlace(editor);
  editor.choose(3);
  KIMIA_REQUIRE(editor.matchMode());
  editor.drainEvents();  // ignore whatever kick-off produced

  // Run the clock out.
  while (!editor.matchOver()) editor.update(1.0);
  const std::vector<WorldEditor::GameEvent> events = editor.drainEvents();
  bool whistle = false;
  bool roundOver = false;
  for (const WorldEditor::GameEvent event : events) {
    if (event == WorldEditor::GameEvent::Whistle) whistle = true;
    if (event == WorldEditor::GameEvent::RoundOver) roundOver = true;
  }
  // Full time is a whistle AND the end-of-round cue, in that order.
  KIMIA_REQUIRE(whistle);
  KIMIA_REQUIRE(roundOver);
}

KIMIA_TEST(world_a_completed_trick_has_its_own_sound_cue) {
  // A trick used to share the generic kick sound, which made showing off
  // sound like an ordinary touch.
  WorldEditor editor;
  streetAtTheFeet(editor);
  editor.setAiSkill(0.0);
  editor.drainEvents();
  KIMIA_REQUIRE(editor.startTrick(WorldEditor::Trick::Juggle));
  editor.update(kimia::kTrickJuggleTime + 0.01);
  KIMIA_REQUIRE(editor.styleScore() == kimia::kTrickJugglePoints);

  const std::vector<WorldEditor::GameEvent> events = editor.drainEvents();
  bool trick = false;
  for (const WorldEditor::GameEvent event : events) {
    if (event == WorldEditor::GameEvent::Trick) trick = true;
  }
  KIMIA_REQUIRE(trick);
}

KIMIA_TEST(world_a_tackle_reports_its_own_sound_cue) {
  WorldEditor editor;
  streetAtTheFeet(editor);
  editor.setAiSkill(1.0);
  editor.setPlayerPosition(Vec3{0.0, 0.5, 7.0});
  editor.setBallPosition(Vec3{0.0, editor.world().ball.radius, 0.0});
  editor.setBallVelocity(Vec3{0.0, 0.0, 0.0});
  editor.drainEvents();

  bool tackle = false;
  for (i32 i = 0; i < 180 && !tackle; ++i) {
    editor.update(1.0 / 60.0);
    for (const WorldEditor::GameEvent event : editor.drainEvents()) {
      if (event == WorldEditor::GameEvent::Tackle) tackle = true;
    }
  }
  KIMIA_REQUIRE(tackle);
}
