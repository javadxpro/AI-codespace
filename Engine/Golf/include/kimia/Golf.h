#pragma once

#include <kimia/Physics.h>
#include <kimia/Scene.h>
#include <kimia/Types.h>
#include <kimia/Vec.h>

#include <optional>
#include <string>
#include <vector>

namespace kimia {

// Golf tuning (the "accurate" ball). Friction follows the constant-force
// contact model: the ball decelerates by (friction + rollingFriction) * g
// m/s^2 while touching the ground, so it stops after v0 / (mu_total * g)
// seconds. mu_total = 0.40 makes the spec demo shot (aim 0, power 0.61,
// launch 10.735 m/s) roll 14.7 m and cross the cup at 2.3 m/s — a hole-out.
inline constexpr f64 kGolfBallRadius = 0.12;
inline constexpr f64 kGolfBallRestitution = 0.40;
inline constexpr f64 kGolfBallFriction = 0.25;
inline constexpr f64 kGolfBallRollingFriction = 0.15;
inline constexpr f64 kGolfLaunchBaseSpeed = 2.5;
inline constexpr f64 kGolfLaunchPowerScale = 13.5;
inline constexpr f64 kGolfCupCaptureDistance = 0.28;
inline constexpr f64 kGolfCupCaptureSpeed = 5.0;
inline constexpr f64 kGolfWallMinLength = 2.0;
inline constexpr f64 kGolfWallMaxLength = 12.0;

// EDIT <-> AIM -> CHARGE -> ROLL -> SUNK/OUT -> AIM
enum class GolfMode { Edit, Aim, Charge, Roll, Sunk, Out };

enum class BuilderTool { Wall, Tee, Hole };

// Gameplay + course builder logic for the reference golf game. The course is
// a SceneIO-v1 scene: walls are cube entities named "Wall_*" (they become
// static physics boxes), "Tee" and "Hole" are the marker entities, and the
// "# demo <aim> <power>" line holds the player-authored demo shot.
class GolfGame {
public:
  GolfGame();

  // --- Course ---
  void resetToDefaultCourse();                                  // spec sample course
  bool loadCourse(const std::string& path, std::string& error);
  bool loadCourseText(const std::string& text, std::string& error);
  bool saveCourse(const std::string& path, std::string& error);  // writes "# demo" too
  const Scene& scene() const { return scene_; }
  std::optional<DemoShot> demoShot() const { return scene_.demoShot; }

  // --- Play ---
  void update(f64 hostSeconds);
  void aimLeft(f64 dt) { aimYaw_ += 0.9 * dt; }
  void aimRight(f64 dt) { aimYaw_ -= 0.9 * dt; }
  void chargeBegin();   // AIM -> CHARGE (hold SHOOT)
  void chargeEnd();     // CHARGE -> ROLL, launches with the current power
  void launchDemoShot();  // launches the course's demo aim/power

  GolfMode mode() const { return mode_; }
  u32 strokes() const { return strokes_; }
  f64 power() const { return power_; }
  f64 aimYaw() const { return aimYaw_; }
  Vec3 ballPosition() const;
  Vec3 ballVelocity() const;
  f64 ballSpeed() const;
  Vec3 teePosition() const { return teeRest_; }
  Vec3 holePosition() const { return holePos_; }
  bool ballInHole() const { return sunk_; }
  const PhysicsWorld& physicsWorld() const { return world_; }

  // --- Builder (any of these in a play mode switches to EDIT and acts) ---
  void setTool(BuilderTool tool);
  BuilderTool tool() const { return tool_; }
  void adjustWallLength(f64 delta);  // Q/E, clamped 2..12
  f64 wallLength() const { return wallLength_; }
  bool wallAxisZ() const { return wallAxisZ_; }
  void toggleWallAxis();             // R
  void moveGhost(f64 dx, f64 dz, bool fine);  // WASD/arrows; Shift = 0.1 step
  Vec3 ghostPosition() const { return ghost_; }
  bool place();                      // Enter
  bool undo();                       // U
  usize wallCount() const;
  void togglePlayEdit();             // F: EDIT <-> AIM

  // Editor/debug helper: drop the ball anywhere with a velocity (ROLL mode).
  void placeBall(const Vec3& position, const Vec3& velocity);

  // Required stats format (every button must be observable in it):
  //   KIMIA GOLF | <EDIT|AIM|CHARGE|ROLL|SUNK|OUT> | stroke N | power P%
  //   | tool <wall|tee|hole> len <L><X|Z> | walls <W>
  std::string statsLine() const;

private:
  struct UndoAction {
    enum Kind { PlacedWall, MovedTee, MovedHole };
    Kind kind = PlacedWall;
    EntityHandle wallHandle = kNullEntity;
    Vec3 before{0.0, 0.0, 0.0};
  };

  void rebuildPhysics();
  void resetBallToTee();
  void launch(f64 shotPower);
  Vec3 aimDirection() const;
  EntityHandle findNamed(const std::string& name) const;
  Vec3 wallScale() const;
  u32 nextWallNumber() const;

  Scene scene_;
  PhysicsWorld world_;
  u32 ballId_ = 0U;

  EntityHandle teeHandle_ = kNullEntity;
  EntityHandle holeHandle_ = kNullEntity;
  Vec3 teeRest_{0.0, 0.14, 7.0};
  Vec3 holePos_{0.0, 0.01, -7.0};

  GolfMode mode_ = GolfMode::Aim;
  u32 strokes_ = 0U;
  f64 power_ = 0.5;
  f64 aimYaw_ = 0.0;
  f64 modeTimer_ = 0.0;
  bool sunk_ = false;

  BuilderTool tool_ = BuilderTool::Wall;
  f64 wallLength_ = 4.0;
  bool wallAxisZ_ = true;
  Vec3 ghost_{0.0, 0.0, 0.0};
  std::vector<UndoAction> undoStack_;

  std::optional<DemoShot> lastShot_;
};

}  // namespace kimia
