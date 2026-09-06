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

// Wind (stage 20.5-b2): a constant horizontal breeze on the airborne ball.
// `windSpeed` is an acceleration in m/s^2 (0 = calm, the default for every
// game so far) and `windDirection` is radians, 0 = blowing toward -Z, the
// same convention as the aim yaw.
inline constexpr f64 kProfileWindMax = 20.0;  // matches kMaxWindAcceleration

// --- Weather and the clock (stage 24) ---
// `rain` is how hard it is coming down, 0 = dry ... 1 = downpour. `wet` is
// how slick that leaves the ground, 0 = dry grip ... 1 = ice rink. Rain
// wets the pitch by itself, but a profile can set them apart (a dry frosty
// morning, or a soaked pitch after the rain stopped).
inline constexpr f64 kProfileRainMax = 1.0;
inline constexpr f64 kProfileWetMax = 1.0;
// Kick-off time as hours on a 24 h clock, 12.0 = midday (the default, and
// what every game did before this stage existed).
inline constexpr f64 kProfileHourMax = 24.0;
inline constexpr f64 kProfileAiMax = 1.0;
inline constexpr f64 kProfileStaminaMax = 1.0;
// Weapon limits. Generous, but they stop a hand-edited profile making a
// one-shot instant-kill rifle with infinite ammo.
inline constexpr u32 kProfileHealthMax = 1000U;
inline constexpr u32 kProfileMagazineMax = 500U;
inline constexpr f64 kProfileFireRateMax = 30.0;
inline constexpr u32 kProfileDamageMax = 200U;
inline constexpr f64 kProfileRangeMax = 500.0;
inline constexpr f64 kProfileReloadMax = 10.0;

// How the camera follows the play (stage 28).
//   orbit     - the plain editor orbit, parked over the action
//   chase     - swings behind the aim: the golf/shot camera
//   broadcast - a touchline camera that pulls back to keep ball and
//               player both in frame, like a televised match
enum class CameraStyle { Orbit, Chase, Broadcast };

// Squad size limits: 1 = a single player (golf, sandbox), 11 = a full grass
// football side. The ceiling keeps a typo from spawning a crowd the phone
// cannot draw.
inline constexpr u32 kProfileTeamMin = 1U;
inline constexpr u32 kProfileTeamMax = 16U;

// Match clock limits: a street game is minutes, never more than an hour.
inline constexpr f64 kProfileMatchMax = 3600.0;

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
  u32 par = 3U;                      // hole scoring: strokes each cup is rated at (scorecard)
  f64 windSpeed = 0.0;               // m/s^2 on the airborne ball (0 = calm)
  f64 windDirection = 0.0;           // radians, 0 = toward -Z (like the aim yaw)
  f64 rain = 0.0;                    // 0 = dry ... 1 = downpour (stage 24)
  f64 wetness = 0.0;                 // 0 = dry grip ... 1 = very slick
  f64 hour = 12.0;                   // kick-off time, 0..24, 12 = midday
  // Squad size per side (stage 21): 5 = street football, 11 = grass, 4 = a
  // battleground team, 1 = the lone player of golf/sandbox. The engine
  // spawns teamSize - 1 team-mates plus teamSize opponents when a world
  // asks for a match; a profile of 1 keeps the single-player behaviour.
  u32 teamSize = 1U;
  // Match length in seconds (stage 22). 0 = no clock: the endless kickabout
  // every world played until now. A profile only becomes a match when it has
  // BOTH a squad (teamSize > 1) and a clock.
  f64 matchSeconds = 0.0;
  // Showboating (stage 26). Skill moves are a STREET thing: the alley is
  // where a nutmeg is worth more than a goal. A serious grass fixture turns
  // them off, so the same engine plays two different games.
  bool tricks = false;
  // How sharp the computer players are (stage 27). 0 = statues, which is
  // exactly how every squad behaved until now, so an old profile plays
  // unchanged. 1 = they chase hard and hold their shape.
  f64 aiSkill = 0.0;
  // Camera style (stage 28). A golf round wants the aim-following chase it
  // has always had; a match wants a broadcast camera that pulls back to
  // keep the play in frame. Sandbox keeps the plain editor orbit.
  CameraStyle camera = CameraStyle::Orbit;
  // The laws of the game (stage 29). Off everywhere except grass: an alley
  // kickabout has no linesman, and stopping a street game for a throw-in
  // would ruin it. A real fixture wants them.
  bool rules = false;
  // How quickly a sprinting player tires, 0 = never (the endless runner
  // every game had until now).
  f64 stamina = 0.0;
  // Arena mode (stage 30). Off everywhere except battleground: this turns
  // the football pitch into a third-person shooter. The engine provides
  // the weapon, the profile only supplies numbers — no pay-to-win, because
  // there is nothing to buy.
  bool arena = false;
  u32 health = 100U;      // hit points each fighter starts with
  u32 magazine = 30U;     // rounds before a reload
  f64 fireRate = 6.0;     // shots per second
  // Six hits to kill at 6 rounds a second is a one-second time-to-kill IF
  // every shot lands — which leaves room to react, take cover and shoot
  // back. The first tuning (five hits at 8/s) killed in half a second and
  // an eight-man arena produced 183 kills a minute.
  u32 damage = 17U;
  f64 range = 60.0;       // how far a shot carries, meters
  f64 reloadTime = 1.8;   // seconds

  f64 halfLength() const { return fieldLength * 0.5; }
  f64 halfWidth() const { return fieldWidth * 0.5; }
};

const char* ballTypeName(BallType type);  // "accurate" / "fantasy"
bool ballTypeFromName(const std::string& name, BallType& out);
const char* environmentName(EnvironmentKind kind);  // "grass" / "sand" / "night" / "asphalt"
bool environmentFromName(const std::string& name, EnvironmentKind& out);
const char* playModeName(PlayMode mode);  // "kick" / "shot"
bool playModeFromName(const std::string& name, PlayMode& out);
const char* cameraStyleName(CameraStyle style);  // "orbit" / "chase" / "broadcast"
bool cameraStyleFromName(const std::string& name, CameraStyle& out);
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
//   par 3                        (hole scoring: rated strokes per cup, 1..20)
//   wind 0.000000 0.000000       (speed m/s^2 0..20, direction radians)
//   team 5                       (players per side, 1..16; 1 = single player)
//   match 300.000000             (match length in seconds, 0 = no clock)
//   tricks on                    (skill moves + style points: on/off)
//   ai 0.700000                  (computer skill, 0 = statues .. 1 = sharp)
//   camera broadcast             (orbit / chase / broadcast)
//   rules on                     (throw-ins, offside and fouls: on/off)
//   stamina 0.600000             (how fast a runner tires, 0 = never)
//   arena on                     (third-person shooter mode: on/off)
//   weapon 100 30 8.0 24 60.0 1.8    (health mag fireRate damage range reload)
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
