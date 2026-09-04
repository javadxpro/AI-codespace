#pragma once

#include <kimia/Physics.h>
#include <kimia/Scene.h>
#include <kimia/Types.h>
#include <kimia/Vec.h>

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

// Fantasy ball: high bounce, low friction, no roll decay (the spec's words).
inline constexpr f64 kWorldFantasyRestitution = 0.85;
inline constexpr f64 kWorldFantasyFriction = 0.05;
inline constexpr f64 kWorldFantasyRollingFriction = 0.0;
inline constexpr f64 kWorldFantasyRadius = 0.15;

// Goal: a gate at z = kWorldGoalZ, half width kWorldGoalHalfWidth; the ball
// scores when it crosses the line inside the posts, below the bar height.
inline constexpr f64 kWorldGoalZ = -8.5;
inline constexpr f64 kWorldGoalHalfWidth = 2.0;
inline constexpr f64 kWorldGoalBarHeight = 1.2;
inline constexpr f64 kWorldFloorHalf = 10.0;  // the floor is 20 x 20

// Ball physics presets. Accurate = the golf tuning; fantasy = bouncy/slick.
enum class BallType { Accurate, Fantasy };

enum class EnvironmentKind { Grass, Sand, Night };

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
// SceneIO-v1 scene (floor + goal). Worlds serialize through WorldIO.
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

// Fills world.scene with the default environment: the floor plane and the
// goal (two posts + crossbar). The floor color follows the environment.
void buildDefaultWorldScene(WorldData& world);

// The option-driven editor. Every interaction is a menu option (Num1..Num6
// taps) or a named action (reset/back); nothing requires a keyboard.
//
//   Main ─ Create World ─> Menu ─ Add Player  ─> AskPlayer (fast/normal/slow)
//                             ├─ Add Ball     ─> AskBall (accurate/fantasy)
//                             ├─ Add Environment -> AskEnvironment (grass/sand/night)
//                             ├─ PLAY         ─> Play <-> Goal
//                             └─ Save / Main
class WorldEditor {
public:
  WorldEditor();

  // --- Menu model (what the web page shows) ---
  std::string menuTitle() const;
  std::vector<std::string> optionLabels() const;   // bound to Num1..Num6
  std::vector<std::pair<std::string, std::string>> holdPad() const;  // label, key
  std::vector<std::pair<std::string, std::string>> tapPad() const;   // label, key

  void choose(i32 optionIndex);  // a menu option was tapped
  void resetBall();              // play: put the ball back at the center
  void backToMenu();             // play: leave the game, back to the object menu
  bool quitRequested() const { return quitRequested_; }

  // --- World lifecycle ---
  const WorldData& world() const { return world_; }
  bool hasWorld() const { return hasWorld_; }
  void setWorldPath(const std::string& path) { worldPath_ = path; }
  const std::string& worldPath() const { return worldPath_; }
  void createWorld();  // resets to a fresh default world
  bool loadWorld(const std::string& path, std::string& error);
  bool saveWorld(const std::string& path, std::string& error);
  std::string lastError() const { return lastError_; }

  // --- Play simulation ---
  void update(f64 hostSeconds);
  void setMoveInput(f64 x, f64 z);  // held direction (-1..1 per axis)
  bool playing() const { return screen_ == Screen::Play || screen_ == Screen::Goal; }
  bool celebrating() const { return screen_ == Screen::Goal; }
  Vec3 playerPosition() const { return playerPos_; }
  Vec3 ballPosition() const;
  Vec3 ballVelocity() const;
  u32 score() const { return world_.score; }

  // Debug/test hooks (the app may use them for tooling).
  void setPlayerPosition(const Vec3& position) { playerPos_ = position; }
  void setBallPosition(const Vec3& position);

  std::string statsLine() const;

private:
  enum class Screen { Main, Menu, AskPlayer, AskBall, AskEnvironment, Play, Goal };

  Vec3 ballRest() const { return Vec3{0.0, 0.35, 0.0}; }
  Vec3 playerRest() const { return Vec3{0.0, 0.5, 4.0}; }
  void rebuildPhysics();
  void resetBallToCenter();
  void enterPlay();
  void applyEnvironmentToScene();

  WorldData world_;
  bool hasWorld_ = false;
  Screen screen_ = Screen::Main;
  std::string worldPath_ = "my_world.kimia";
  std::string lastError_;
  bool quitRequested_ = false;

  PhysicsWorld physics_;
  u32 ballId_ = 0U;
  Vec3 playerPos_{0.0, 0.5, 4.0};
  Vec3 moveInput_{0.0, 0.0, 0.0};
  f64 goalTimer_ = 0.0;
};

}  // namespace kimia
