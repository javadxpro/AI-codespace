#pragma once

#include <kimia/GameProfile.h>
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
// (Player speeds, kick formula, jump height, ball presets and the sandbox
// floor size live in GameProfile.h — a world copies them from its profile.)

inline constexpr f64 kWorldKickReach = 0.55;  // horizontal distance player-ball
inline constexpr f64 kWorldPlayerRadius = 0.35;  // XZ radius the ball collides with
inline constexpr f64 kWorldPlayerRestitution = 0.4;  // bounce off a still player

// --- Ball control (stage 23) ---
// Dribbling: a ball this close to a walking player is nudged along instead
// of blasted, so the ball stays at the feet rather than running away.
inline constexpr f64 kWorldDribbleReach = 0.85;
// The dribbled ball is kept at this distance ahead of the feet and never
// pushed faster than the player's own speed times this.
inline constexpr f64 kWorldDribbleHold = 0.42;
inline constexpr f64 kWorldDribbleSpeed = 1.15;
// Curl: a shot with the curl stick fully over gets this much side spin
// (rad/s). Positive curls to the right of the aim.
inline constexpr f64 kWorldMaxCurl = 14.0;

// --- Skill moves (stage 26) ---
// A trick takes real time: you are committed the moment you start one, and
// a defender can take the ball off you while you are showing off. That is
// what makes it a risk and not a free win.
inline constexpr f64 kTrickNutmegTime = 0.45;   // knock it through their legs
inline constexpr f64 kTrickRouletteTime = 0.70; // spin away with the ball
inline constexpr f64 kTrickJuggleTime = 0.55;   // flick it up and keep it up
// How far in front the ball is pushed by a nutmeg, in meters.
inline constexpr f64 kTrickNutmegPush = 3.2;
// A roulette turns the player this far (radians) — a half turn away.
inline constexpr f64 kTrickRouletteTurn = 3.14159265358979323846;
// Upward speed of a juggle flick, m/s.
inline constexpr f64 kTrickJuggleLift = 4.5;
// Style points, awarded only when the trick COMPLETES.
inline constexpr u32 kTrickNutmegPoints = 50U;
inline constexpr u32 kTrickRoulettePoints = 30U;
inline constexpr u32 kTrickJugglePoints = 10U;
// A nutmeg only counts against an opponent within this range.
inline constexpr f64 kTrickNutmegRange = 2.5;
// Get the ball this far from your feet mid-trick and the move is lost.
inline constexpr f64 kTrickLoseBall = 2.0;

// --- Computer players (stage 27) ---
// Only the closest team-mate on each side actually goes for the ball. The
// rest hold their shape, or a match turns into every player chasing the
// same ball in a heap — which is what school football looks like, not a
// game worth playing against.
// How fast a computer player runs, as a share of the human's speed. Even
// a perfect one is a shade slower so a good player can beat them.
inline constexpr f64 kAiMaxSpeedFactor = 0.92;
// A chaser this close to the ball is considered to be ON it.
inline constexpr f64 kAiTackleReach = 0.75;
// How hard a tackle knocks the ball away, m/s at full skill.
inline constexpr f64 kAiTacklePush = 4.0;
// A keeper never strays further than this from its own line.
inline constexpr f64 kAiKeeperRange = 2.5;
// Support players hold this far behind the ball, up their own half.
inline constexpr f64 kAiSupportGap = 3.0;
// Below this skill the computer players do not move at all.
inline constexpr f64 kAiIdleSkill = 0.05;

// --- Weather and the clock (stage 24) ---
inline constexpr f64 kWorldSunrise = 6.0;   // before this it is night
inline constexpr f64 kWorldSunset = 19.0;   // from this hour it is night
inline constexpr f64 kWorldNightLight = 0.12;  // floodlights: never pitch black

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
// Hole («سوراخ», golf scoring): the ball is in when its centre is within
// kWorldHoleCapture of the cup centre AND slower than kWorldHoleCaptureSpeed —
// the reference golf rule. Drawn as a flat dark disc of kWorldHoleRadius.
inline constexpr f64 kWorldHoleRadius = 0.22;
inline constexpr f64 kWorldHoleCapture = 0.28;
inline constexpr f64 kWorldHoleCaptureSpeed = 5.0;
inline constexpr f64 kWorldHoleDepth = 0.02;  // disc thickness (sits flush in the ground)
// Shot mode («mode shot»): aim turns at this rate, the charge fills 0 -> 1
// in 1/kWorldChargeRate seconds and wraps, and a rolling ball is "at rest"
// below kWorldShotStopSpeed (the next shot is taken from there).
inline constexpr f64 kWorldAimRate = 0.9;       // radians per second
inline constexpr f64 kWorldChargeRate = 0.9;    // power per second
inline constexpr f64 kWorldShotStopSpeed = 0.05;
inline constexpr f64 kWorldPlaceSpeed = 2.0;       // ghost/move speed
inline constexpr f64 kWorldPlaceSpeedFine = 0.5;   // with Shift (ریز)
inline constexpr f64 kWorldNudgeStep = 0.1;        // inspector: position/scale per tap

// Game-object kinds, inferred from entity names (fully SceneIO-v1
// compatible — nothing extra is stored in the file):
//   "Player" -> player, "Ball" -> ball, "Wall_*" -> wall,
//   "Block_*" -> block, "Goal*" -> goal, "Crate_*" -> dynamic crate,
//   "Model_*" -> placed mesh file (OBJ/FBX), "Hole_*" -> golf cup,
//   anything else -> decoration.
enum class ObjectKind { Player, Ball, Block, Wall, Goal, Crate, Model, Hole, Decoration };

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
  GameProfile profile;  // the game this world belongs to (copied on create)
  PlayerConfig player;
  BallConfig ball;
  EnvironmentKind environment = EnvironmentKind::Grass;
  Scene scene;
  u32 score = 0U;
  // Hole scoring: the best (lowest) completed round on this course, kept in
  // the world file so a personal record survives closing the game. 0 = no
  // round finished yet. A round only counts when every cup was holed.
  u32 bestRound = 0U;
  // Match mode (stage 22): goals per side, saved with the world so a match
  // in progress survives a save/load. Team 1 is the human's side.
  u32 scoreTeam1 = 0U;
  u32 scoreTeam2 = 0U;

  f64 halfLength() const { return profile.halfLength(); }  // Z
  f64 halfWidth() const { return profile.halfWidth(); }    // X
};

void applyBallType(BallConfig& ball, BallType type);
EnvironmentColors environmentColors(EnvironmentKind kind);

// Resets the world to its profile's defaults (player speed, ball type,
// environment) — the answers the user has not given yet.
void applyProfileDefaults(WorldData& world);

// Fills world.scene with an EMPTY ground (just the floor plane, sized by the
// profile's field) — the user builds their game on it object by object.
void buildEmptyWorldScene(WorldData& world);

// The option-driven editor / builder. Every interaction is a menu option
// (Num1..Num6 taps) or a named action; nothing requires a keyboard.
//
//   Main ─ Create World ─ «کدام بازی؟» (profile) ─> Builder ─ Catalog ─ questions ─> Place
//                                                       ├─ Manage (list/move/delete/color)
//                                                       ├─ Environment
//                                                       ├─ PLAY ─> Play <-> Goal
//                                                       └─ Save / Main
class WorldEditor {
public:
  WorldEditor();

  // --- Game profiles (the games this engine can make) ---
  // Built-ins plus every *.kimiaprofile in the profile directory; the menu
  // «دنیای جدید» lists them (5 per page + «بیشتر…»/«بازگشت»).
  void setProfileDirectory(const std::string& dir);
  const std::string& profileDirectory() const { return profileDir_; }
  void refreshProfiles();
  usize profileCount() const { return profiles_.size(); }
  const GameProfile& profileAt(usize index) const { return profiles_[index]; }
  const GameProfile& profile() const { return world_.profile; }  // the current world's game
  bool choosingProfile() const { return screen_ == Screen::AskProfile; }

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
  void createWorld();                             // fresh EMPTY ground with the current profile
  void createWorld(const GameProfile& profile);   // fresh EMPTY ground for this game
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
    return screen_ == Screen::Manage || screen_ == Screen::Inspector || screen_ == Screen::Move ||
           screen_ == Screen::ConfirmDelete || screen_ == Screen::AskColor;
  }
  // Screens where the arrow keys orbit the camera instead of moving anything.
  bool cameraControlled() const {
    return screen_ == Screen::Builder || screen_ == Screen::Manage || screen_ == Screen::Inspector ||
           screen_ == Screen::AskColor || screen_ == Screen::ConfirmDelete ||
           screen_ == Screen::AskEnvironment;
  }
  usize managedCount() const { return managed_.size(); }
  usize managedIndex() const { return managedIndex_; }
  std::string managedName() const;
  std::string managedKindName() const;
  void selectManagedAt(usize listIndex);  // hierarchy pick -> Inspector
  void nudgeSelectedPosition(f64 dx, f64 dy, f64 dz);
  void nudgeSelectedScale(f64 delta);
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
  bool playing() const { return screen_ == Screen::Play || screen_ == Screen::Goal || screen_ == Screen::RoundEnd; }
  bool celebrating() const { return screen_ == Screen::Goal; }
  Vec3 playerPosition() const { return playerPos_; }
  bool physicsCharacterOnGround() const { return physics_.character()->onGround; }

  // --- Squads (stage 21) ---
  // A profile with «team N» greater than 1 fills the pitch on entering PLAY:
  // N - 1 team-mates on the player's side (team 1) and N opponents (team 2).
  // They are solid physics characters standing in a line formation, so the
  // player and the ball collide with them. The engine only places bodies —
  // it never invents the pitch itself.
  u32 teamSize() const { return world_.profile.teamSize; }
  u32 squadCount() const { return static_cast<u32>(physics_.characterCount()); }
  std::vector<u32> squadIds() const { return physics_.characterIds(); }
  Vec3 squadPosition(u32 id) const;  // origin when the id is unknown
  u32 squadTeam(u32 id) const;       // 0 when the id is unknown

  // --- Match (stage 22) ---
  // A profile is a match when it fields squads AND runs a clock. The two
  // goals become team property: the one on -Z is team 2's, the one on +Z is
  // team 1's, and a ball crossing a goal line scores for the OTHER side.
  bool matchMode() const { return world_.profile.teamSize > 1U && world_.profile.matchSeconds > 0.0; }
  u32 teamScore(u32 team) const;      // 0 for any team but 1 and 2
  f64 matchClock() const { return matchClock_; }  // seconds left (counts down)
  bool matchOver() const { return matchOver_; }
  u32 matchWinner() const;            // 1, 2, or 0 for a draw / unfinished
  std::string matchClockText() const; // "5:00", "0:07"
  std::string matchScoreText() const; // "MA 2 - 1 ANHA"
  // Hand a goal to a side directly. The goal lines call this themselves; it
  // is public so a referee (or a test) can award one.
  void creditGoal(u32 team);
  Vec3 ballPosition() const;
  Vec3 ballVelocity() const;
  u32 score() const { return world_.score; }

  // Shot mode (profile «mode shot», e.g. golf): the arrows aim, holding
  // «شوت» charges, releasing shoots the resting ball. One shot = one stroke;
  // a hole resets the stroke count. The aim direction is on the ground plane.
  bool shotMode() const { return world_.profile.mode == PlayMode::Shot; }
  bool holeScoring() const { return world_.profile.scoring == Scoring::Hole; }
  void setShootHeld(bool held);           // hold to charge, release to shoot
  bool charging() const { return charging_; }
  bool ballAtRest() const;                // a shot can be taken
  f64 aimYaw() const { return aimYaw_; }  // radians, 0 = toward -Z
  Vec3 aimDirection() const;
  f64 power() const { return power_; }    // 0..1 while charging (wraps)
  u32 strokes() const { return strokes_; }
  f64 shotSpeed(f64 power) const;         // kickBase + power * kickSpeedScale
  usize holeCount() const;

  // A course («scoring hole»): the cups are played in name order, Hole_1
  // first. Only the current cup captures the ball; holing it moves the game
  // to the next cup (same ball position: the next tee is where you are, like
  // mini-golf) and records the strokes on the scorecard. After the last cup
  // the round ends with the scorecard screen («پایان دور»). The rating of a
  // cup is the profile's `par`; the total is compared with par * cups.
  usize currentHole() const { return currentHole_; }  // 0-based index into the sorted cups
  std::string currentHoleName() const;                // "Hole_3" ("" if the course has no cup)
  const std::vector<u32>& scorecard() const { return scorecard_; }  // strokes per holed cup
  u32 totalStrokes() const;
  u32 par() const { return world_.profile.par; }
  i32 scoreToPar() const;  // totalStrokes - par * holed cups (negative = under par)
  bool roundOver() const { return screen_ == Screen::RoundEnd; }
  std::string scorecardText() const;  // «۳ ۲ ۴ | جمع ۹ | پار ۹ | برابر پار»

  // The personal record on this course: the lowest total of a round in which
  // every cup was holed. 0 = no completed round yet. It is written into the
  // world file, so it survives quitting; `bestRoundIsNew()` is true while the
  // round-over screen is showing a round that just beat (or set) the record.
  u32 bestRound() const { return world_.bestRound; }
  bool bestRoundIsNew() const { return bestIsNew_; }

  // --- Wind (profile `wind <speed> <direction>`) ---
  // A constant horizontal breeze on the airborne ball, straight from the
  // world's profile. Calm (speed 0) unless the profile asks for it.
  f64 windSpeed() const { return world_.profile.windSpeed; }
  f64 windDirection() const { return world_.profile.windDirection; }
  bool windActive() const { return world_.profile.windSpeed > 0.0; }
  // Where the wind blows, as a unit vector on the ground plane ({0,0,0} when
  // calm) — the same convention as aimDirection().
  Vec3 windVector() const;
  // "WIND 3 <-" / "WIND 3 ->" / "WIND 3 ^" / "WIND 3 v": the compass arrow is
  // relative to the CAMERA behind the ball (i.e. relative to the aim), so the
  // player reads "the wind pushes my shot left". Empty when calm.
  std::string windHudText() const;

  // --- Weather and the clock (stage 24) ---
  bool raining() const;
  bool night() const;
  f64 pitchWetness() const;  // 0 dry .. 1 slick; the greater of rain and wet
  f64 sunHeight() const;     // -1 deep night .. 1 noon
  f64 daylight() const;      // 0 dark .. 1 full sun (floored by floodlights)
  std::string skyHudText() const;  // "19:30 NIGHT RAIN WET", empty when plain

  // Debug/test hooks.
  void setAimYaw(f64 yaw) { aimYaw_ = yaw; }
  // Overrides the profile's computer skill for this world. Setting 0 puts
  // the squads back to statues, which is how tests isolate everything that
  // is not the AI.
  void setAiSkill(f64 skill) { world_.profile.aiSkill = skill < 0.0 ? 0.0 : (skill > 1.0 ? 1.0 : skill); }

  // --- Ball control (stage 23) ---
  // Curl for the NEXT shot/pass, -1..1 (negative = left, positive = right).
  // It is a held stick, so it survives until the shot is taken and resets.
  void setCurl(f64 curl);
  f64 curl() const { return curl_; }
  // Dribbling is a DELIBERATE skill, held down like a sprint button: with
  // it held, a ball at the feet is carried instead of blasted. Released
  // (the default) every kick behaves exactly as it always has.
  void setDribbleHeld(bool held) { dribbleHeld_ = held; }
  bool dribbleHeld() const { return dribbleHeld_; }
  // True while the ball is actually being carried at the feet right now.
  bool dribbling() const { return dribbling_; }
  // Ball spin right now, radians/second (0 when there is no ball).
  Vec3 ballSpin() const;
  // --- Skill moves (stage 26) ---
  // Which trick the player is performing right now.
  enum class Trick { None, Nutmeg, Roulette, Juggle };

  // Starts a trick. It only works when the profile allows tricks, the ball
  // is at the player's feet, and no other trick is already running — you
  // cannot cancel out of a trick to dodge the risk. False when refused.
  bool startTrick(Trick trick);
  Trick currentTrick() const { return trick_; }
  bool trickActive() const { return trick_ != Trick::None; }
  // How far through the current trick, 0..1 (0 when none is running).
  f64 trickProgress() const;
  // Style points banked so far this round. Only COMPLETED tricks score.
  u32 styleScore() const { return styleScore_; }
  // The last trick that completed, for the HUD flash.
  Trick lastTrick() const { return lastTrick_; }
  // Name for the HUD/tests: "NUTMEG" / "ROULETTE" / "JUGGLE" / "".
  static const char* trickName(Trick trick);
  // HUD line: "NUTMEG!" while a trick runs, else "STYLE 80" once anything
  // has been scored. Empty when tricks are off or nothing has happened.
  std::string trickHudText() const;
  // True when this world's profile has skill moves switched on.
  bool tricksEnabled() const { return world_.profile.tricks; }

  // --- Computer players (stage 27) ---
  // How sharp this world's computer players are, 0 (statues) .. 1.
  f64 aiSkill() const { return world_.profile.aiSkill; }
  bool aiActive() const { return world_.profile.aiSkill > kAiIdleSkill && squadCount() > 1U; }
  // The character each side has sent for the ball right now (0 = nobody).
  // Only one per team: the rest hold their shape instead of swarming.
  u32 aiChaser(u32 team) const;
  // The keeper of a side: the player nearest its own goal line. 0 when the
  // side is too small to spare one.
  u32 aiKeeper(u32 team) const;
  // Where a given computer player wants to be right now. Useful on its own
  // for tests and for drawing debug markers.
  Vec3 aiTargetFor(u32 id) const;

  // Pass the ball to a team-mate: the nearest one in front of the aim, at
  // the speed that arrives at their feet. False when nobody is available.
  bool pass();
  // The team-mate a pass would find right now, 0 when there is nobody.
  u32 passTarget() const;
  void setPlayerPosition(const Vec3& position) {
    playerPos_ = position;
    physics_.resetCharacter(position);  // teleports keep the physics body in sync
  }
  void jumpPressed() { jumpQueued_ = true; }  // consumed in the next PLAY update
  void setBallPosition(const Vec3& position);
  void setBallVelocity(const Vec3& velocity);
  Vec3 crateVelocity(const std::string& name) const;
  void setCrateVelocity(const std::string& name, const Vec3& velocity);

  std::string statsLine() const;

  // --- On-frame HUD (drawn by the app with the bitmap font) ---
  // Short ASCII lines for the top-left of the frame while playing: what the
  // player needs at a glance without reading the stats line. Empty outside
  // PLAY. Hole scoring: "HOLE 2/3  PAR 3", "STROKE 1  TOTAL 4", "IN! 2 STROKES"
  // or "ROUND OVER  9 (PAR 9)  EVEN"; gate scoring: "SCORE 3", "GOAL!".
  std::vector<std::string> hudLines() const;
  // The charge meter (0..1) while charging in shot mode, else < 0 (hidden).
  f64 hudPower() const { return charging_ ? power_ : -1.0; }

  // --- Game events (the sound hooks) ---
  // Every update() that shoots / kicks / scores / ends the round pushes one
  // event; the app drains them once per frame and plays the sounds it has.
  // Events survive until drained so a slow frame never loses one.
  enum class GameEvent { Shot, Kick, Holed, Goal, RoundOver };
  std::vector<GameEvent> drainEvents();

  // --- Camera hint (shot mode) ---
  // Where a chase camera should stand: behind the ball, opposite the aim,
  // `distance` back and `height` up, looking at the ball. Only meaningful in
  // shot mode while playing; the app blends the orbit camera toward it.
  bool chaseCameraActive() const { return shotMode() && playing() && !roundOver(); }

private:
  enum class Screen {
    Main, Builder, Catalog, AskPlayer, AskBall, AskBlock, AskWallLen, AskWallAxis, AskGoal, Place,
    Manage, Move, ConfirmDelete, AskColor, AskEnvironment, Play, Goal, AskModelFile, AskModelSize,
    Inspector, AskProfile, RoundEnd,
  };

  Vec3 ballRest() const;
  Vec3 playerRest() const;
  f64 kickSpeed() const;  // profile.kickBase + playerSpeed * profile.kickSpeedScale
  void shoot(f64 power);  // shot mode: launch the resting ball along the aim
  bool captureHole(const Vec3& position, f64 speed);  // hole scoring: ball in the current cup?
  std::vector<std::string> sortedHoleNames() const;   // Hole_1, Hole_2, ... (by number)
  void startRound();                                  // cup 0, empty scorecard
  void rebuildPhysics();
  void resetBallToCenter();
  void enterPlay();
  void spawnSquads();  // formation for the current profile's «team N»
  void kickOff();      // ball to the center spot, squads back to formation
  Vec3 takeCurlSpin();  // the curl stick as spin, and reset the stick
  void updateTrick(f64 seconds);  // advance and finish the running trick
  void updateAi(f64 seconds);     // drive every computer player one step
  f64 trickDuration(Trick trick) const;
  u32 trickPoints(Trick trick) const;
  bool opponentInFront(f64 range) const;  // is there someone to nutmeg?
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
  bool jumpQueued_ = false;
  f64 goalTimer_ = 0.0;
  f64 matchClock_ = 0.0;
  bool matchOver_ = false;

  // Shot mode state.
  f64 aimYaw_ = 0.0;
  f64 curl_ = 0.0;
  bool dribbling_ = false;
  bool dribbleHeld_ = false;
  Trick trick_ = Trick::None;
  Trick lastTrick_ = Trick::None;
  f64 trickTimer_ = 0.0;    // counts down while a trick runs
  f64 trickLength_ = 0.0;   // how long the running trick lasts
  u32 styleScore_ = 0U;
  f64 power_ = 0.0;
  bool charging_ = false;
  bool shootHeld_ = false;
  u32 strokes_ = 0U;
  usize currentHole_ = 0U;
  std::vector<u32> scorecard_;
  bool bestIsNew_ = false;  // the round on screen just set the record
  std::vector<GameEvent> events_;

  // Pending object (place flow).
  ObjectKind pendingKind_ = ObjectKind::Block;
  f64 pendingSize_ = kWorldBlockMedium;
  bool pendingAxisZ_ = true;
  Vec3 ghost_{0.0, 0.0, 0.0};

  // Game profiles («دنیای جدید» -> «کدام بازی؟»).
  std::string profileDir_ = "profiles";
  std::vector<GameProfile> profiles_;
  usize profilePage_ = 0U;  // 5 games per screen

  // Model file placement (catalog -> file list -> size -> place).
  std::string importDir_ = "assets";
  std::vector<std::string> importFiles_;  // OBJ/FBX names, sorted
  usize importPage_ = 0U;                 // 5 files per screen
  std::string pendingFile_;

  // Management (hierarchy list + inspector).
  std::vector<EntityHandle> managed_;
  usize managedIndex_ = 0U;
  usize managePage_ = 0U;     // 5 names per screen
  usize inspectorPage_ = 0U;  // 3 pages of inspector actions
};

}  // namespace kimia
