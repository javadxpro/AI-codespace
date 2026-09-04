#pragma once

#include <kimia/Physics.h>
#include <kimia/Scene.h>
#include <kimia/Types.h>
#include <kimia/Vec.h>

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace kimia {

// --- World tuning constants ---

// Player speed presets (units/second).
inline constexpr f64 kWorldPlayerFast = 6.0;
inline constexpr f64 kWorldPlayerNormal = 4.0;
inline constexpr f64 kWorldPlayerSlow = 2.5;

// A kick launches the ball along the movement direction with
// speed = kWorldKickBase + playerSpeed * kWorldKickSpeedScale plus a small pop.
inline constexpr f64 kWorldKickBase = 2.0;
inline constexpr f64 kWorldKickSpeedScale = 0.5;
inline constexpr f64 kWorldKickUp = 1.2;
inline constexpr f64 kWorldKickReach = 0.55;  // horizontal distance player-ball
inline constexpr f64 kWorldPlayerRadius = 0.35;  // XZ radius the ball collides with
inline constexpr f64 kWorldPlayerRestitution = 0.4;  // bounce off a still player

// Fantasy ball: high bounce, low friction, no roll decay (the spec's words).
inline constexpr f64 kWorldFantasyRestitution = 0.85;
inline constexpr f64 kWorldFantasyFriction = 0.05;
inline constexpr f64 kWorldFantasyRollingFriction = 0.0;
inline constexpr f64 kWorldFantasyRadius = 0.15;

inline constexpr f64 kWorldFloorHalf = 10.0;  // the floor is 20 x 20

// --- Object builder constants (all chosen from menus) ---
inline constexpr f64 kWorldBlockSmall = 0.5;
inline constexpr f64 kWorldBlockMedium = 1.0;
inline constexpr f64 kWorldBlockLarge = 2.0;
inline constexpr f64 kWorldWallShort = 3.0;
inline constexpr f64 kWorldWallMedium = 6.0;
inline constexpr f64 kWorldWallLong = 9.0;
inline constexpr f64 kWorldGoalSmall = 2.0;
inline constexpr f64 kWorldGoalMedium = 3.0;
inline constexpr f64 kWorldGoalLarge = 4.0;
inline constexpr f64 kWorldGoalHeight = 2.0;
inline constexpr f64 kWorldPlaceSpeed = 2.0;       // ghost/move speed
inline constexpr f64 kWorldPlaceSpeedFine = 0.5;   // with Shift (ریز)

// Ball physics presets. Accurate = the golf tuning; fantasy = bouncy/slick.
enum class BallType { Accurate, Fantasy };

enum class EnvironmentKind { Grass, Sand, Night };

// Game-object kinds, inferred from entity names (fully SceneIO-v1
// compatible — nothing extra is stored in the file):
//   "Player" -> player, "Ball" -> ball, "Wall_*" -> wall,
//   "Block_*" -> block, "Goal*" -> goal, "Crate_*" -> dynamic crate,
//   "Model_*" -> placed mesh file (OBJ/FBX), anything else -> decoration.
enum class ObjectKind { Player, Ball, Block, Wall, Goal, Crate, Model, Decoration };

ObjectKind objectKindForName(const std::string& name);
bool isPhysicsObject(ObjectKind kind);           // block/wall/goal: static colliders
bool isLegacyGoalPart(const std::string& name);  // GoalPostLeft/Right/Bar

// Dynamic crates: a fixed 1x1x1 box the player can shove and kick. Crates
// fall under gravity, rest on the floor/blocks, stack on each other and
// collide with the ball (they can push it into the goal).
inline constexpr f64 kWorldCrateSize = 1.0;
inline constexpr f64 kWorldCrateMass = 1.0;
inline constexpr f64 kWorldCrateRestitution = 0.25;
inline constexpr f64 kWorldCrateFriction = 0.5;
inline constexpr f64 kWorldCrateRollingFriction = 0.05;
inline constexpr f64 kWorldCrateKickScale = 0.6;  // kick speed = ball kick * 0.6
inline constexpr f64 kWorldCrateKickUp = 0.8;     // plus a small pop
inline constexpr f64 kWorldBallMass = 0.4;        // the ball is lighter than a crate

// Placed model files (OBJ/FBX): uniform size multiplier chosen from a menu.
inline constexpr f64 kWorldModelSmall = 0.5;
inline constexpr f64 kWorldModelMedium = 1.0;
inline constexpr f64 kWorldModelLarge = 2.0;

struct PlayerConfig {
  f64 speed = kWorldPlayerNormal;
  Vec3 color{0.2, 0.5, 0.9};
};

struct BallConfig {
  BallType type = BallType::Accurate;
  f64 radius = 0.12;
  f64 restitution = 0.40;
  f64 friction = 0.40;
  f64 rollingFriction = 0.22;
  Vec3 color{0.95, 0.95, 0.92};
};

struct EnvironmentColors {
  Vec3 floor{0.22, 0.45, 0.24};
  Vec3 clear{0.40, 0.62, 0.88};
};

// The user-authored world: the answers to the editor's questions plus a
// SceneIO-v1 scene (ground + the objects the user built). Worlds serialize
// through WorldIO.
struct WorldData {
  std::string name = "MyWorld";
  PlayerConfig player;
  BallConfig ball;
  EnvironmentKind environment = EnvironmentKind::Grass;
  Scene scene;
  u32 score = 0U;
};

void applyBallType(BallConfig& ball, BallType type);
EnvironmentColors environmentColors(EnvironmentKind kind);

// Fills world.scene with an EMPTY ground (just the floor plane) — the user
// builds their game on it object by object.
void buildEmptyWorldScene(WorldData& world);

// The option-driven editor / builder. Every interaction is a menu option
// (Num1..Num6 taps) or a named action; nothing requires a keyboard.
//
//   Main ─ Create World ─> Builder ─ Catalog ─ questions ─> Place
//                                    ├─ Manage (list/move/delete/color)
//                                    ├─ Environment
//                                    ├─ PLAY ─> Play <-> Goal
//                                    └─ Save / Main
class WorldEditor {
public:
  WorldEditor();

  // --- Menu model (what the web page shows) ---
  std::string menuTitle() const;
  std::vector<std::string> optionLabels() const;   // bound to Num1..Num6
  std::vector<std::pair<std::string, std::string>> holdPad() const;  // label, key
  std::vector<std::pair<std::string, std::string>> tapPad() const;   // label, key

  void choose(i32 optionIndex);  // a menu option was tapped
  void resetBall();              // play: put the ball back at its spawn
  void backToMenu();             // play: leave the game, back to the builder
  bool quitRequested() const { return quitRequested_; }

  // --- Import directory (files the user can place in the scene) ---
  // The editor lists OBJ/FBX files from this directory in the catalog and
  // the user places them as Model_* entities (Unity-style: drop a file into
  // the project assets folder, place it in the scene).
  void setImportDirectory(const std::string& dir) { importDir_ = dir; }
  const std::string& importDirectory() const { return importDir_; }
  void refreshImportFiles();
  usize importFileCount() const { return importFiles_.size(); }
  const std::string& importFileAt(usize index) const { return importFiles_[index]; }

  // --- World lifecycle ---
  const WorldData& world() const { return world_; }
  bool hasWorld() const { return hasWorld_; }
  void setWorldPath(const std::string& path) { worldPath_ = path; }
  const std::string& worldPath() const { return worldPath_; }
  void createWorld();  // resets to a fresh EMPTY ground
  bool loadWorld(const std::string& path, std::string& error);
  bool saveWorld(const std::string& path, std::string& error);
  std::string lastError() const { return lastError_; }

  // --- Builder info (for rendering) ---
  Vec3 ghostPosition() const { return ghost_; }
  void setGhostPosition(const Vec3& position) { ghost_ = position; }
  ObjectKind ghostKind() const { return pendingKind_; }
  f64 ghostSize() const { return pendingSize_; }    // block size / wall length / goal width
  bool ghostAxisZ() const { return pendingAxisZ_; }  // wall axis
  const EntityData* selectedEntity() const;          // in manage screens
  bool placing() const { return screen_ == Screen::Place; }
  bool movingObject() const { return screen_ == Screen::Move; }
  bool selectingObject() const {
    return screen_ == Screen::Manage || screen_ == Screen::Move || screen_ == Screen::ConfirmDelete ||
           screen_ == Screen::AskColor;
  }
  usize managedCount() const { return managed_.size(); }
  usize managedIndex() const { return managedIndex_; }
  std::string managedName() const;
  std::string managedKindName() const;
  usize goalCount() const;    // goal groups (legacy trios count as one)
  usize objectCount() const;  // all entities except the ground
  usize physicsBoxCount() const { return physics_.boxCount(); }
  usize dynamicBoxCount() const { return physics_.dynamicBoxCount(); }
  // A crate's position: the physics body while playing, the placed entity
  // position while building (crates reset to their placed spots on PLAY).
  Vec3 cratePosition(const std::string& name) const;

  // --- Play simulation ---
  void update(f64 hostSeconds);
  void setMoveInput(f64 x, f64 z);  // held direction (-1..1 per axis)
  void setFineMove(bool fine) { fine_ = fine; }
  bool playing() const { return screen_ == Screen::Play || screen_ == Screen::Goal; }
  bool celebrating() const { return screen_ == Screen::Goal; }
  Vec3 playerPosition() const { return playerPos_; }
  Vec3 ballPosition() const;
  Vec3 ballVelocity() const;
  u32 score() const { return world_.score; }

  // Debug/test hooks.
  void setPlayerPosition(const Vec3& position) { playerPos_ = position; }
  void setBallPosition(const Vec3& position);
  void setBallVelocity(const Vec3& velocity);
  Vec3 crateVelocity(const std::string& name) const;
  void setCrateVelocity(const std::string& name, const Vec3& velocity);

  std::string statsLine() const;

private:
  enum class Screen {
    Main, Builder, Catalog, AskPlayer, AskBall, AskBlock, AskWallLen, AskWallAxis, AskGoal, Place,
    Manage, Move, ConfirmDelete, AskColor, AskEnvironment, Play, Goal, AskModelFile, AskModelSize,
  };

  Vec3 ballRest() const;
  Vec3 playerRest() const;
  void rebuildPhysics();
  void resetBallToCenter();
  void enterPlay();
  void applyEnvironmentToScene();
  void beginPlace();      // ghost to the origin, enter Place
  void confirmPlace();    // create/update the pending object at the ghost
  void refreshManaged();  // snapshot of all objects (everything but Ground)
  void deleteManaged();
  void applyManagedColor(const Vec3& color);
  EntityHandle playerEntity() const { return world_.scene.find("Player"); }
  EntityHandle ballEntity() const { return world_.scene.find("Ball"); }

  WorldData world_;
  bool hasWorld_ = false;
  Screen screen_ = Screen::Main;
  std::string worldPath_ = "my_world.kimia";
  std::string lastError_;
  bool quitRequested_ = false;

  PhysicsWorld physics_;
  u32 ballId_ = 0U;
  std::map<std::string, u32> crateIds_;  // crate entity name -> dynamic box id
  std::vector<u32> crateBodyIds_;        // dynamic box ids in scene order
  Vec3 playerPos_{0.0, 0.5, 4.0};
  Vec3 moveInput_{0.0, 0.0, 0.0};
  bool fine_ = false;
  f64 goalTimer_ = 0.0;

  // Pending object (place flow).
  ObjectKind pendingKind_ = ObjectKind::Block;
  f64 pendingSize_ = kWorldBlockMedium;
  bool pendingAxisZ_ = true;
  Vec3 ghost_{0.0, 0.0, 0.0};

  // Model file placement (catalog -> file list -> size -> place).
  std::string importDir_ = "assets";
  std::vector<std::string> importFiles_;  // OBJ/FBX names, sorted
  usize importPage_ = 0U;                 // 5 files per screen
  std::string pendingFile_;

  // Management.
  std::vector<EntityHandle> managed_;
  usize managedIndex_ = 0U;
};

}  // namespace kimia
