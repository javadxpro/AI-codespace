#pragma once

#include <kimia/Types.h>

#include <string>
#include <vector>

namespace kimia {

// Ball physics presets. Accurate = the golf tuning; fantasy = bouncy/slick.
enum class BallType { Accurate, Fantasy };

// Ground/sky presets. Asphalt is the street-football surface.
enum class EnvironmentKind { Grass, Sand, Night, Asphalt };

// How the player plays the ball.
//   Kick: a running character; walking into the ball kicks it (football).
//   Shot: no runner — aim with the arrows, hold «شوت» to charge, release to
//         shoot from where the ball rests (golf, and later aimed passes).
enum class PlayMode { Kick, Shot };

// What scores.
//   Gate: the ball crosses a goal line between the posts («دروازه»).
//   Hole: the ball rolls slowly into a cup («سوراخ»).
enum class Scoring { Gate, Hole };

// --- Sandbox tuning (the numbers the editor shipped with before profiles) ---

// Player speed presets (units/second).
inline constexpr f64 kWorldPlayerFast = 6.0;
inline constexpr f64 kWorldPlayerNormal = 4.0;
inline constexpr f64 kWorldPlayerSlow = 2.5;

// A kick launches the ball along the movement direction with
// speed = kickBase + playerSpeed * kickSpeedScale plus a small pop (kickUp).
// In shot mode the same line reads speed = kickBase + power * kickSpeedScale
// where power is the charge (0..1) — one line, one meaning: how the ball leaves.
inline constexpr f64 kWorldKickBase = 2.0;
inline constexpr f64 kWorldKickSpeedScale = 0.5;
inline constexpr f64 kWorldKickUp = 1.2;
inline constexpr f64 kWorldJumpHeight = 1.2;  // player jump: feet apex (v = sqrt(2 g h))

inline constexpr f64 kWorldFloorHalf = 10.0;  // the sandbox floor is 20 x 20

// Accurate ball = the golf tuning (a test pins these to kGolfBall*).
inline constexpr f64 kWorldAccurateRadius = 0.12;
inline constexpr f64 kWorldAccurateRestitution = 0.40;
inline constexpr f64 kWorldAccurateFriction = 0.25;
inline constexpr f64 kWorldAccurateRollingFriction = 0.15;

// Fantasy ball: high bounce, low friction, no roll decay (the spec's words).
inline constexpr f64 kWorldFantasyRestitution = 0.85;
inline constexpr f64 kWorldFantasyFriction = 0.05;
inline constexpr f64 kWorldFantasyRollingFriction = 0.0;
inline constexpr f64 kWorldFantasyRadius = 0.15;

// Field size limits (meters). Small enough to stay inside the 20-unit
// inspector clamp era, large enough for a 40 x 40 battleground map.
inline constexpr f64 kProfileFieldMin = 4.0;
inline constexpr f64 kProfileFieldMax = 80.0;

// A game profile: the data that turns the KIMIA engine into one particular
// game (street football, grass football, battleground, ...). Every feature
// lives in the engine; a profile only selects and tunes it. A new world
// copies the profile it was created from, so a world file stays
// self-contained. Every key here is consumed by the engine today — nothing
// is decorative.
struct GameProfile {
  std::string name = "sandbox";      // stable id: one ASCII token
  std::string title = "زمین آزاد";   // menu label (any text)
  f64 fieldLength = kWorldFloorHalf * 2.0;  // Z extent (goals face Z)
  f64 fieldWidth = kWorldFloorHalf * 2.0;   // X extent
  EnvironmentKind environment = EnvironmentKind::Grass;  // default for new worlds
  f64 playerSpeed = kWorldPlayerNormal;     // default player speed
  f64 jumpHeight = kWorldJumpHeight;        // feet apex in meters
  BallType ballDefault = BallType::Accurate;
  bool ballChoice = true;  // ask «دقیق یا فانتزی؟» when the ball is added
  f64 kickBase = kWorldKickBase;
  f64 kickSpeedScale = kWorldKickSpeedScale;
  f64 kickUp = kWorldKickUp;
  PlayMode mode = PlayMode::Kick;    // kick = football runner, shot = golf aim/charge
  Scoring scoring = Scoring::Gate;   // gate = goal line, hole = cup

  f64 halfLength() const { return fieldLength * 0.5; }
  f64 halfWidth() const { return fieldWidth * 0.5; }
};

const char* ballTypeName(BallType type);  // "accurate" / "fantasy"
bool ballTypeFromName(const std::string& name, BallType& out);
const char* environmentName(EnvironmentKind kind);  // "grass" / "sand" / "night" / "asphalt"
bool environmentFromName(const std::string& name, EnvironmentKind& out);
const char* playModeName(PlayMode mode);  // "kick" / "shot"
bool playModeFromName(const std::string& name, PlayMode& out);
const char* scoringName(Scoring scoring);  // "gate" / "hole"
bool scoringFromName(const std::string& name, Scoring& out);

// The built-in profiles in menu order (= the build order of the games):
// golf, street, grass, battleground, sandbox.
std::vector<GameProfile> builtinProfiles();

// The built-ins overridden/extended by every `*.kimiaprofile` file in `dir`:
// a file whose profile `name` matches a built-in replaces it in place (so
// street football can be retuned without C++); other files are appended in
// sorted filename order. A missing or empty directory yields the built-ins.
std::vector<GameProfile> loadProfiles(const std::string& dir);

// Text format — line based and tolerant like SceneIO:
//
//   # KIMIA profile v1
//   name street
//   title فوتبال خیابونی ایران
//   field 16.000000 5.000000
//   environment asphalt
//   player speed 5.000000 jump 1.800000
//   ball fantasy choice off
//   kick 3.000000 0.600000 2.000000
//   mode kick
//   scoring gate
//
// `#` lines are comments, unknown keys are skipped, a line missing any of
// its values is ignored as a whole (never half-applied), out-of-range
// numbers are clamped. `name` is the only required line. Numbers print as
// %.6f, so save -> load -> save is byte-identical.
class ProfileIO {
public:
  static std::string save(const GameProfile& profile);
  static bool saveToFile(const GameProfile& profile, const std::string& path, std::string& error);
  static bool load(const std::string& text, GameProfile& out, std::string& error);
  static bool loadFromFile(const std::string& path, GameProfile& out, std::string& error);

  // The body lines (no header) — WorldIO embeds them as `# profile <line>`.
  static std::vector<std::string> lines(const GameProfile& profile);
  // Applies one body line; false when the line was unknown or incomplete.
  static bool parseLine(const std::string& line, GameProfile& out);
};

}  // namespace kimia
