#include <kimia/World.h>
#include <kimia/WorldIO.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <iomanip>
#include <sstream>

namespace kimia {

namespace {

constexpr f64 kGoalCelebration = 2.0;  // seconds after a goal (or a holed ball)

// Which side owns a goal, and therefore which side is punished by it. The
// goal on the far half (-Z) is team 2's net, so scoring there is team 1's
// goal; the one on the player's half (+Z) is team 1's net. A goal exactly on
// the halfway line (z == 0) counts for team 1: the classic single-goal
// kickabout every world built so far shoots toward -Z.
u32 scoringTeamForGoalZ(f64 goalZ) { return goalZ <= 0.0 ? 1U : 2U; }
constexpr f64 kKickMaxSpeed = 8.0;     // a ball rolling faster is not re-kicked
constexpr f64 kMoveEpsilon = 1e-6;
constexpr f64 kPlayerMargin = 0.6;  // keep the player inside the floor
constexpr f64 kEditMargin = 0.5;    // ghost / moved objects stay this far from the edge

const char* playerSpeedName(f64 speed) {
  if (speed >= kWorldPlayerFast - 0.5) return "fast";
  if (speed <= kWorldPlayerSlow + 0.5) return "slow";
  return "normal";
}

const char* screenName(int screen) {
  switch (screen) {
    case 0: return "MAIN";
    case 1: return "BUILDER";
    case 2: return "CATALOG";
    case 3: return "PLAYER";
    case 4: return "BALL";
    case 5: return "BLOCK";
    case 6: return "WALLLEN";
    case 7: return "WALLAXIS";
    case 8: return "GOALQ";
    case 9: return "PLACE";
    case 10: return "MANAGE";
    case 11: return "MOVE";
    case 12: return "DELETE";
    case 13: return "COLOR";
    case 14: return "ENV";
    case 15: return "PLAY";
    case 16: return "GOAL";
    case 17: return "MODELFILE";
    case 18: return "MODELSIZE";
    case 19: return "INSPECTOR";
    case 20: return "PROFILE";
    default: return "ROUNDEND";
  }
}

bool endsWith(const std::string& text, const char* suffix) {
  const usize length = std::strlen(suffix);
  return text.size() >= length && text.compare(text.size() - length, length, suffix) == 0;
}

// One goal = a "Goal*" entity (single cube: scale.x = width, scale.y = 2) or
// a legacy trio GoalPostLeft/GoalPostRight/GoalBar. Legacy parts share the
// same base name, so they collapse into one group.
std::string goalBase(const std::string& name) {
  static const char* kSuffixes[] = {"PostLeft", "PostRight", "Bar"};
  for (const char* suffix : kSuffixes) {
    const usize length = std::strlen(suffix);
    if (name.size() > length && name.compare(name.size() - length, length, suffix) == 0) {
      return name.substr(0, name.size() - length);
    }
  }
  return name;
}

struct GoalGroup {
  bool hasSingle = false;
  Vec3 singlePos{0.0, 0.0, 0.0};
  Vec3 singleScale{1.0, 1.0, 1.0};
  bool hasPosts = false;
  Vec3 leftPos{0.0, 0.0, 0.0};
  Vec3 rightPos{0.0, 0.0, 0.0};
  Vec3 barPos{0.0, 0.0, 0.0};
  bool valid() const { return hasSingle || hasPosts; }
  f64 z() const { return hasSingle ? singlePos.z : barPos.z; }
  f64 x() const { return hasSingle ? singlePos.x : (leftPos.x + rightPos.x) * 0.5; }
  f64 width() const { return hasSingle ? singleScale.x : std::abs(rightPos.x - leftPos.x); }
  f64 height() const { return hasSingle ? singlePos.y + singleScale.y * 0.5 : barPos.y; }
};

void scanGoals(const Scene& scene, std::map<std::string, GoalGroup>& groups) {
  groups.clear();
  scene.forEach([&groups](EntityHandle, const EntityData& entity) {
    if (entity.name.rfind("Goal", 0) != 0) return;
    GoalGroup& group = groups[goalBase(entity.name)];
    if (endsWith(entity.name, "PostLeft")) {
      group.hasPosts = true;
      group.leftPos = entity.transform.position;
    } else if (endsWith(entity.name, "PostRight")) {
      group.hasPosts = true;
      group.rightPos = entity.transform.position;
    } else if (endsWith(entity.name, "Bar")) {
      group.hasPosts = true;
      group.barPos = entity.transform.position;
    } else {
      group.hasSingle = true;
      group.singlePos = entity.transform.position;
      group.singleScale = entity.transform.scale;
    }
  });
}

}  // namespace

ObjectKind objectKindForName(const std::string& name) {
  if (name == "Player") return ObjectKind::Player;
  if (name == "Ball") return ObjectKind::Ball;
  if (name.rfind("Wall_", 0) == 0) return ObjectKind::Wall;
  if (name.rfind("Block_", 0) == 0) return ObjectKind::Block;
  if (name.rfind("Goal", 0) == 0) return ObjectKind::Goal;
  if (name.rfind("Crate_", 0) == 0) return ObjectKind::Crate;
  if (name.rfind("Model_", 0) == 0) return ObjectKind::Model;
  if (name.rfind("Hole_", 0) == 0) return ObjectKind::Hole;
  return ObjectKind::Decoration;
}

// Blocks, walls and goals become STATIC colliders; the player, the ball and
// the crates are dynamic (the crates are simulated, the player is kinematic).
bool isPhysicsObject(ObjectKind kind) {
  return kind == ObjectKind::Block || kind == ObjectKind::Wall || kind == ObjectKind::Goal;
}

bool isLegacyGoalPart(const std::string& name) {
  return endsWith(name, "PostLeft") || endsWith(name, "PostRight") || endsWith(name, "Bar");
}

void applyBallType(BallConfig& ball, BallType type) {
  if (type == BallType::Fantasy) {
    ball.type = BallType::Fantasy;
    ball.radius = kWorldFantasyRadius;
    ball.restitution = kWorldFantasyRestitution;
    ball.friction = kWorldFantasyFriction;
    ball.rollingFriction = kWorldFantasyRollingFriction;
    ball.color = Vec3{0.75, 0.25, 0.9};
  } else {
    ball.type = BallType::Accurate;
    ball.radius = kWorldAccurateRadius;
    ball.restitution = kWorldAccurateRestitution;
    ball.friction = kWorldAccurateFriction;
    ball.rollingFriction = kWorldAccurateRollingFriction;
    ball.color = Vec3{0.95, 0.95, 0.92};
  }
}

EnvironmentColors environmentColors(EnvironmentKind kind) {
  switch (kind) {
    case EnvironmentKind::Sand:
      return EnvironmentColors{Vec3{0.76, 0.70, 0.50}, Vec3{0.78, 0.60, 0.38}};
    case EnvironmentKind::Night:
      return EnvironmentColors{Vec3{0.16, 0.26, 0.20}, Vec3{0.03, 0.04, 0.10}};
    case EnvironmentKind::Asphalt:
      return EnvironmentColors{Vec3{0.30, 0.30, 0.32}, Vec3{0.62, 0.70, 0.82}};
    default:
      return EnvironmentColors{Vec3{0.22, 0.45, 0.24}, Vec3{0.40, 0.62, 0.88}};
  }
}

void applyProfileDefaults(WorldData& world) {
  world.player.speed = world.profile.playerSpeed;
  applyBallType(world.ball, world.profile.ballDefault);
  world.environment = world.profile.environment;
}

void buildEmptyWorldScene(WorldData& world) {
  world.scene.clear();
  EntityData ground;
  ground.name = "Ground";
  ground.mesh = MeshKind::plane;
  ground.transform.scale = Vec3{world.profile.fieldWidth, 1.0, world.profile.fieldLength};
  ground.color = environmentColors(world.environment).floor;
  ground.roughness = 0.95;
  world.scene.create(ground);
}

Vec3 WorldEditor::ballRest() const {
  const EntityData* ball = world_.scene.get(ballEntity());
  if (ball != nullptr) return ball->transform.position;
  return Vec3{0.0, world_.ball.radius, 0.0};
}

Vec3 WorldEditor::playerRest() const {
  const EntityData* player = world_.scene.get(playerEntity());
  if (player != nullptr) return player->transform.position;
  return Vec3{0.0, 0.5, 4.0};
}

WorldEditor::WorldEditor() {
  refreshProfiles();  // built-ins (plus any *.kimiaprofile next to the app)
  rebuildPhysics();
}

void WorldEditor::rebuildPhysics() {
  physics_.clear();
  physics_.addPlane(0.0);
  // The breeze comes from the world's profile, so a world plays in the same
  // wind it was saved with (calm by default: windSpeed 0).
  physics_.setWind(makeWind(world_.profile.windSpeed, world_.profile.windDirection));
  // Weather (stage 24): rain soaks the pitch, so the surface is as slick as
  // the wetter of «how wet the profile says it is» and «how hard it rains».
  physics_.setWetness(pitchWetness());
  crateIds_.clear();
  crateBodyIds_.clear();
  std::map<std::string, GoalGroup> goals;
  scanGoals(world_.scene, goals);
  world_.scene.forEach([this](EntityHandle, const EntityData& entity) {
    const ObjectKind kind = objectKindForName(entity.name);
    if (kind == ObjectKind::Block || kind == ObjectKind::Wall) {
      physics_.addBox(entity.transform.position, entity.transform.scale * 0.5);
    } else if (kind == ObjectKind::Goal && !isLegacyGoalPart(entity.name)) {
      // A single-entity goal: two posts + a crossbar.
      const f64 width = entity.transform.scale.x;
      const f64 halfHeight = entity.transform.scale.y * 0.5;
      const f64 top = entity.transform.position.y + halfHeight;
      const Vec3 at = entity.transform.position;
      physics_.addBox(Vec3{at.x - width * 0.5 + 0.06, at.y, at.z}, Vec3{0.06, halfHeight, 0.06});
      physics_.addBox(Vec3{at.x + width * 0.5 - 0.06, at.y, at.z}, Vec3{0.06, halfHeight, 0.06});
      physics_.addBox(Vec3{at.x, top, at.z}, Vec3{width * 0.5 + 0.06, 0.06, 0.06});
    } else if (kind == ObjectKind::Goal && isLegacyGoalPart(entity.name)) {
      physics_.addBox(entity.transform.position, entity.transform.scale * 0.5);
    } else if (kind == ObjectKind::Crate) {
      DynamicBox crate;
      crate.position = entity.transform.position;
      crate.halfExtents = entity.transform.scale * 0.5;
      crate.mass = kWorldCrateMass;
      crate.restitution = kWorldCrateRestitution;
      crate.friction = kWorldCrateFriction;
      crate.rollingFriction = kWorldCrateRollingFriction;
      const u32 id = physics_.addDynamicBox(crate);
      crateIds_[entity.name] = id;
      crateBodyIds_.push_back(id);
    }
  });
  SphereBody ball;
  ball.position = ballRest();
  ball.radius = world_.ball.radius;
  ball.mass = kWorldBallMass;
  ball.restitution = world_.ball.restitution;
  ball.friction = world_.ball.friction;
  ball.rollingFriction = world_.ball.rollingFriction;
  // Objects placed on the ball's spawn point must not swallow the ball:
  // raise it on top of the overlapping colliders instead of spawning inside.
  const f64 raised = physics_.resolveSpawnHeight(ball.position, ball.radius,
                                                 std::max(world_.halfLength(), world_.halfWidth()));
  if (raised > ball.position.y) ball.position.y = raised;
  ballId_ = physics_.addSphere(ball);
}

Vec3 WorldEditor::cratePosition(const std::string& name) const {
  if (playing()) {
    const auto found = crateIds_.find(name);
    if (found != crateIds_.end()) {
      const DynamicBox* crate = physics_.dynamicBox(found->second);
      if (crate != nullptr) return crate->position;
    }
  }
  const EntityData* entity = world_.scene.get(world_.scene.find(name));
  return entity != nullptr ? entity->transform.position : Vec3{0.0, 0.0, 0.0};
}

Vec3 WorldEditor::ballPosition() const {
  const SphereBody* ball = physics_.sphere(ballId_);
  return ball != nullptr ? ball->position : ballRest();
}

Vec3 WorldEditor::ballVelocity() const {
  const SphereBody* ball = physics_.sphere(ballId_);
  return ball != nullptr ? ball->velocity : Vec3{0.0, 0.0, 0.0};
}

void WorldEditor::setBallPosition(const Vec3& position) {
  SphereBody* ball = physics_.sphere(ballId_);
  if (ball != nullptr) ball->position = position;
}

void WorldEditor::setBallVelocity(const Vec3& velocity) {
  SphereBody* ball = physics_.sphere(ballId_);
  if (ball != nullptr) ball->velocity = velocity;
}

Vec3 WorldEditor::crateVelocity(const std::string& name) const {
  const auto found = crateIds_.find(name);
  if (found != crateIds_.end()) {
    const DynamicBox* crate = physics_.dynamicBox(found->second);
    if (crate != nullptr) return crate->velocity;
  }
  return Vec3{0.0, 0.0, 0.0};
}

void WorldEditor::setCrateVelocity(const std::string& name, const Vec3& velocity) {
  const auto found = crateIds_.find(name);
  if (found != crateIds_.end()) {
    DynamicBox* crate = physics_.dynamicBox(found->second);
    if (crate != nullptr) crate->velocity = velocity;
  }
}

void WorldEditor::setMoveInput(f64 x, f64 z) {
  moveInput_.x = x;
  moveInput_.z = z;
}

void WorldEditor::resetBallToCenter() {
  SphereBody* ball = physics_.sphere(ballId_);
  if (ball == nullptr) return;
  ball->position = ballRest();
  ball->velocity = Vec3{0.0, 0.0, 0.0};
}

void WorldEditor::applyEnvironmentToScene() {
  EntityData* ground = world_.scene.get(world_.scene.find("Ground"));
  if (ground != nullptr) ground->color = environmentColors(world_.environment).floor;
}

void WorldEditor::createWorld() { createWorld(world_.profile); }

void WorldEditor::createWorld(const GameProfile& profile) {
  world_ = WorldData{};
  world_.profile = profile;
  applyProfileDefaults(world_);
  buildEmptyWorldScene(world_);
  hasWorld_ = true;
  lastError_.clear();
  screen_ = Screen::Builder;
  managed_.clear();
  managedIndex_ = 0U;
  rebuildPhysics();
  resetBallToCenter();
}

bool WorldEditor::loadWorld(const std::string& path, std::string& error) {
  WorldData loaded;
  if (!WorldIO::loadFromFile(path, loaded, error)) {
    lastError_ = error;
    return false;
  }
  world_ = std::move(loaded);
  hasWorld_ = true;
  lastError_.clear();
  applyEnvironmentToScene();
  managed_.clear();
  managedIndex_ = 0U;
  screen_ = Screen::Builder;
  rebuildPhysics();
  resetBallToCenter();
  return true;
}

bool WorldEditor::saveWorld(const std::string& path, std::string& error) {
  if (!hasWorld_) {
    error = "no world to save";
    lastError_ = error;
    return false;
  }
  if (!WorldIO::saveToFile(world_, path, error)) {
    lastError_ = error;
    return false;
  }
  lastError_.clear();
  return true;
}

Vec3 WorldEditor::squadPosition(u32 id) const {
  const CharacterBody* body = physics_.characterById(id);
  if (body == nullptr) return Vec3{0.0, 0.0, 0.0};
  return body->position;
}

void WorldEditor::setSquadPosition(u32 id, const Vec3& position) {
  CharacterBody* body = physics_.characterById(id);
  if (body == nullptr) return;
  body->position = position;
  body->velocity = Vec3{0.0, 0.0, 0.0};
}

u32 WorldEditor::squadTeam(u32 id) const {
  const CharacterBody* body = physics_.characterById(id);
  if (body == nullptr) return 0U;
  return body->team;
}

// Line up the two sides. The player (character 1) always keeps team 1 and
// its own resting spot; the rest are spread evenly across the width of the
// field, team 1 on the player's half (+Z) and team 2 on the far half (-Z),
// one third of the way out from the middle so nobody starts inside a wall.
void WorldEditor::spawnSquads() {
  const u32 size = world_.profile.teamSize;
  physics_.character()->team = size > 1U ? 1U : 0U;
  if (size <= 1U) return;  // single-player profile: nothing to spawn

  const CharacterBody shape;  // default half extents
  const f64 rowZ = world_.halfLength() / 3.0;
  const f64 feet = playerRest().y;
  const f64 span = world_.halfWidth() - kPlayerMargin;
  for (u32 team = 1U; team <= 2U; ++team) {
    // Team 1 is a man short: the human player is already on the pitch.
    const u32 count = team == 1U ? size - 1U : size;
    for (u32 index = 0U; index < count; ++index) {
      CharacterBody body = shape;
      body.team = team;
      // Evenly spaced slots: (i + 1) / (count + 1) maps to -span..+span.
      const f64 t = static_cast<f64>(index + 1U) / static_cast<f64>(count + 1U);
      body.position.x = -span + 2.0 * span * t;
      body.position.y = feet;
      body.position.z = team == 1U ? rowZ : -rowZ;
      physics_.addCharacter(body);
    }
  }
}

u32 WorldEditor::teamScore(u32 team) const {
  if (team == 1U) return world_.scoreTeam1;
  if (team == 2U) return world_.scoreTeam2;
  return 0U;
}

u32 WorldEditor::matchWinner() const {
  if (world_.scoreTeam1 > world_.scoreTeam2) return 1U;
  if (world_.scoreTeam2 > world_.scoreTeam1) return 2U;
  return 0U;  // a draw
}

std::string WorldEditor::matchClockText() const {
  // Always mm:ss, rounded UP so the clock only shows 0:00 when time is gone.
  const f64 left = matchClock_ > 0.0 ? matchClock_ : 0.0;
  const i64 total = static_cast<i64>(std::ceil(left - 1e-9));
  const i64 minutes = total / 60;
  const i64 seconds = total % 60;
  std::ostringstream text;
  text << minutes << ':' << std::setfill('0') << std::setw(2) << seconds;
  return text.str();
}

std::string WorldEditor::matchScoreText() const {
  return "MA " + std::to_string(world_.scoreTeam1) + " - " + std::to_string(world_.scoreTeam2) + " ANHA";
}

// One goal for a side. «score» stays the plain goal counter every non-match
// world has always used, so nothing about a kickabout changed.
void WorldEditor::creditGoal(u32 team) {
  ++world_.score;
  if (!matchMode()) return;
  if (team == 1U) ++world_.scoreTeam1;
  if (team == 2U) ++world_.scoreTeam2;
}

// Kick-off: the ball on the center spot, both squads back in formation and
// the human on his resting mark. Used at the start and after every goal.
void WorldEditor::kickOff() {
  // A restart from the centre spot cancels whatever the whistle was for.
  stoppage_ = Stoppage::None;
  restartTimer_ = 0.0;
  restartTeam_ = 0U;
  resetBallToCenter();
  playerPos_ = playerRest();
  physics_.resetCharacter(playerPos_);
  // Drop everyone but the player, then lay the formation out again.
  for (const u32 id : physics_.characterIds()) {
    if (id != kPrimaryCharacter) physics_.removeCharacter(id);
  }
  spawnSquads();
}

void WorldEditor::enterPlay() {
  playerPos_ = playerRest();
  physics_.resetCharacter(playerPos_);  // feet on the ground, velocity zero
  jumpQueued_ = false;
  moveInput_ = Vec3{0.0, 0.0, 0.0};
  goalTimer_ = 0.0;
  aimYaw_ = 0.0;
  power_ = 0.0;
  charging_ = false;
  shootHeld_ = false;
  strokes_ = 0U;
  startRound();
  events_.clear();
  // Rebuild the physics world: the ball and every crate reset to their
  // placed spots and velocities.
  rebuildPhysics();
  spawnSquads();
  // A match starts 0-0 with a full clock; an endless kickabout has neither.
  matchOver_ = false;
  matchClock_ = world_.profile.matchSeconds;
  if (matchMode()) {
    world_.scoreTeam1 = 0U;
    world_.scoreTeam2 = 0U;
    world_.score = 0U;
  }
  lastError_.clear();
  screen_ = Screen::Play;
}

void WorldEditor::update(f64 hostSeconds) {
  const int screen = static_cast<int>(screen_);
  // The ghost and a live-moved object stay inside the field (same margin
  // as the inspector nudges) — on a 5-wide street court this matters.
  const f64 editBoundX = world_.halfWidth() - kEditMargin;
  const f64 editBoundZ = world_.halfLength() - kEditMargin;
  if (screen == 9) {  // Place: move the ghost with the arrows.
    const f64 speed = fine_ ? kWorldPlaceSpeedFine : kWorldPlaceSpeed;
    ghost_.x = std::min(editBoundX, std::max(-editBoundX, ghost_.x + moveInput_.x * speed * hostSeconds));
    ghost_.z = std::min(editBoundZ, std::max(-editBoundZ, ghost_.z + moveInput_.z * speed * hostSeconds));
    resetBallToCenter();
    return;
  }
  if (screen == 11) {  // Move: move the selected object live.
    if (managedIndex_ < managed_.size()) {
      EntityData* entity = world_.scene.get(managed_[managedIndex_]);
      if (entity != nullptr) {
        const f64 speed = fine_ ? kWorldPlaceSpeedFine : kWorldPlaceSpeed;
        Vec3& at = entity->transform.position;
        at.x = std::min(editBoundX, std::max(-editBoundX, at.x + moveInput_.x * speed * hostSeconds));
        at.z = std::min(editBoundZ, std::max(-editBoundZ, at.z + moveInput_.z * speed * hostSeconds));
      }
    }
    resetBallToCenter();
    return;
  }
  if (screen == 15 && shotMode()) {  // Play, shot mode (golf): aim / charge / roll
    SphereBody* ball = physics_.sphere(ballId_);
    const bool resting = ballAtRest();
    if (resting) {
      // Left/right turn the aim; the ball waits exactly where it stopped.
      aimYaw_ -= moveInput_.x * kWorldAimRate * hostSeconds;
      if (ball != nullptr) ball->velocity = Vec3{0.0, 0.0, 0.0};
      if (shootHeld_ && !charging_) {
        charging_ = true;
        power_ = 0.0;
      }
      if (charging_) {
        if (shootHeld_) {
          power_ += kWorldChargeRate * hostSeconds;
          while (power_ >= 1.0) power_ -= 1.0;
        } else {
          shoot(power_);
        }
      }
    } else {
      charging_ = false;  // a moving ball cannot be hit; the button is ignored
    }
    const Vec3 previous = ballPosition();
    physics_.advance(hostSeconds);
    ball = physics_.sphere(ballId_);
    if (ball != nullptr) {
      const f64 ballBoundX = world_.halfWidth() - world_.ball.radius;
      const f64 ballBoundZ = world_.halfLength() - world_.ball.radius;
      if (ball->position.x > ballBoundX) {
        ball->position.x = ballBoundX;
        if (ball->velocity.x > 0.0) ball->velocity.x = 0.0;
      } else if (ball->position.x < -ballBoundX) {
        ball->position.x = -ballBoundX;
        if (ball->velocity.x < 0.0) ball->velocity.x = 0.0;
      }
      if (ball->position.z > ballBoundZ) {
        ball->position.z = ballBoundZ;
        if (ball->velocity.z > 0.0) ball->velocity.z = 0.0;
      } else if (ball->position.z < -ballBoundZ) {
        ball->position.z = -ballBoundZ;
        if (ball->velocity.z < 0.0) ball->velocity.z = 0.0;
      }
      // Rolling is over below the stop speed: the ball rests for the next shot.
      if (!resting && ball->velocity.length() < kWorldShotStopSpeed && ball->position.y <= world_.ball.radius + 1e-3) {
        ball->velocity = Vec3{0.0, 0.0, 0.0};
      }
    }
    const Vec3 position = ballPosition();
    if (holeScoring()) {
      if (captureHole(position, ballVelocity().length())) {
        ++world_.score;
        screen_ = Screen::Goal;
        goalTimer_ = kGoalCelebration;
        events_.push_back(GameEvent::Holed);
      }
    } else {
      std::map<std::string, GoalGroup> goals;
      scanGoals(world_.scene, goals);
      for (const auto& entry : goals) {
        const GoalGroup& goal = entry.second;
        if (!goal.valid()) continue;
        // A ball crosses a goal line from either side: -Z through a far
        // goal, +Z through the player's own net (an own goal).
        const bool crossedToward = previous.z >= goal.z() && position.z < goal.z();
        const bool crossedBack = matchMode() && previous.z <= goal.z() && position.z > goal.z();
        if ((crossedToward || crossedBack) && std::abs(position.x - goal.x()) < goal.width() * 0.5 &&
            position.y < goal.height()) {
          creditGoal(scoringTeamForGoalZ(goal.z()));
          screen_ = Screen::Goal;
          goalTimer_ = kGoalCelebration;
          events_.push_back(GameEvent::Goal);
          if (ball != nullptr) ball->velocity = Vec3{0.0, 0.0, 0.0};
          break;
        }
      }
    }
    return;
  }
  if (screen == 15) {  // Play
    // The match clock. It only runs while the ball is in play, never during
    // the goal celebration, and full time waits for the ball to be dead.
    if (matchMode() && !matchOver_) {
      matchClock_ -= hostSeconds;
      if (matchClock_ <= 0.0) {
        matchClock_ = 0.0;
        matchOver_ = true;
        screen_ = Screen::RoundEnd;
        // The final whistle, then the end-of-round cue.
        events_.push_back(GameEvent::Whistle);
        events_.push_back(GameEvent::RoundOver);
        SphereBody* deadBall = physics_.sphere(ballId_);
        if (deadBall != nullptr) deadBall->velocity = Vec3{0.0, 0.0, 0.0};
        return;
      }
    }
    Vec3 direction = moveInput_;
    const f64 length = std::sqrt(direction.x * direction.x + direction.z * direction.z);
    if (length > 1.0) {
      direction.x /= length;
      direction.z /= length;
    }
    const bool moving = length > kMoveEpsilon;

    // Character controller: gravity, jumping and collisions live in the
    // physics module; the player shoves and kicks crates/ball as before.
    // A jump pressed in the air is buffered until the feet touch down.
    if (jumpQueued_ && world_.profile.jumpHeight > 0.0 && physics_.characterJump(world_.profile.jumpHeight)) {
      jumpQueued_ = false;
    }
    // Tiredness (stage 29) slows the legs; without a stamina profile this
    // is exactly world_.player.speed, so nothing else changes.
    updateStamina(hostSeconds, moving);
    physics_.moveCharacter(hostSeconds, direction * currentPlayerSpeed());
    playerPos_ = physics_.character()->position;
    const f64 boundX = world_.halfWidth() - kPlayerMargin;
    const f64 boundZ = world_.halfLength() - kPlayerMargin;
    playerPos_.x = std::min(boundX, std::max(-boundX, playerPos_.x));
    playerPos_.z = std::min(boundZ, std::max(-boundZ, playerPos_.z));

    const Vec3 previous = ballPosition();
    physics_.advance(hostSeconds);

    // The ball stays on the floor: clamp it inside the play area and stop
    // any outward motion so it can never roll away forever.
    SphereBody* ball = physics_.sphere(ballId_);
    const f64 ballBoundX = world_.halfWidth() - world_.ball.radius;
    const f64 ballBoundZ = world_.halfLength() - world_.ball.radius;
    if (ball != nullptr) {
      if (ball->position.x > ballBoundX) {
        ball->position.x = ballBoundX;
        if (ball->velocity.x > 0.0) ball->velocity.x = 0.0;
      } else if (ball->position.x < -ballBoundX) {
        ball->position.x = -ballBoundX;
        if (ball->velocity.x < 0.0) ball->velocity.x = 0.0;
      }
      if (ball->position.z > ballBoundZ) {
        ball->position.z = ballBoundZ;
        if (ball->velocity.z > 0.0) ball->velocity.z = 0.0;
      } else if (ball->position.z < -ballBoundZ) {
        ball->position.z = -ballBoundZ;
        if (ball->velocity.z < 0.0) ball->velocity.z = 0.0;
      }
    }

    // The player is solid: resolved after physics so the ball is never
    // rendered inside the player. Walking into the ball kicks it; a still
    // player deflects a rolling ball.
    if (ball != nullptr) {
      const f64 dx = ball->position.x - playerPos_.x;
      const f64 dz = ball->position.z - playerPos_.z;
      const f64 distance = std::sqrt(dx * dx + dz * dz);
      const f64 contact = world_.ball.radius + kWorldPlayerRadius;
      if (distance < contact) {
        Vec3 pushNormal{1.0, 0.0, 0.0};
        if (distance > kMoveEpsilon) {
          pushNormal = Vec3{dx / distance, 0.0, dz / distance};
        } else if (moving) {
          pushNormal = Vec3{direction.x, 0.0, direction.z};
        }
        ball->position.x = playerPos_.x + pushNormal.x * contact;
        ball->position.z = playerPos_.z + pushNormal.z * contact;
        const f64 velocityNormal = ball->velocity.x * pushNormal.x + ball->velocity.z * pushNormal.z;
        if (velocityNormal < 0.0) {
          ball->velocity.x -= (1.0 + kWorldPlayerRestitution) * velocityNormal * pushNormal.x;
          ball->velocity.z -= (1.0 + kWorldPlayerRestitution) * velocityNormal * pushNormal.z;
        }
      }
      // Dribbling (stage 23): a ball right at the feet of a walking player
      // is carried, not blasted. It is nudged to stay just ahead of the
      // player at no more than the player's own pace, so it stays under
      // control instead of running away on the first touch.
      const f64 dribbleDistance = world_.ball.radius + kWorldDribbleReach;
      const bool slowEnough = ball->velocity.length() < kKickMaxSpeed;
      // While a skill move is running it owns the ball: no ordinary dribble
      // touch and no accidental kick can interrupt the animation.
      dribbling_ = !trickActive() && dribbleHeld_ && moving && slowEnough && distance < dribbleDistance &&
                   ball->position.y <= world_.ball.radius + 1e-3;
      if (dribbling_) {
        const f64 pace = world_.player.speed * kWorldDribbleSpeed;
        const Vec3 ahead{playerPos_.x + direction.x * (contact + kWorldDribbleHold), ball->position.y,
                         playerPos_.z + direction.z * (contact + kWorldDribbleHold)};
        const f64 toX = ahead.x - ball->position.x;
        const f64 toZ = ahead.z - ball->position.z;
        const f64 toLength = std::sqrt(toX * toX + toZ * toZ);
        if (toLength > kMoveEpsilon) {
          const f64 push = std::min(pace, toLength / std::max(hostSeconds, 1e-4));
          ball->velocity.x = toX / toLength * push;
          ball->velocity.z = toZ / toLength * push;
        }
      } else if (!trickActive()) {
        const f64 kickDistance = world_.ball.radius + kWorldKickReach;
        if (moving && distance < kickDistance && slowEnough) {
          ball->velocity = direction * kickSpeed() + Vec3{0.0, world_.profile.kickUp, 0.0};
          ball->spin = takeCurlSpin();
          events_.push_back(GameEvent::Kick);
        }
      }
    }

    // The laws of the game (stage 29), checked against where the ball was
    // BEFORE physics so a ball that shot out and got clamped back is still
    // spotted. While play is stopped this holds the ball on the spot.
    updateRules(hostSeconds, previous);
    if (playStopped()) return;

    // Computer players move before the tricks resolve, so a defender who
    // arrives this frame can take the ball off a show-off in the same frame.
    updateAi(hostSeconds);

    // Skill moves run on the same clock as everything else, after the
    // ball has been moved and clamped, so a trick that finishes this frame
    // launches the ball from where it actually is.
    updateTrick(hostSeconds);

    // Dynamic crates: keep them on the floor area; the player can shove
    // them by walking into them and kick them like the ball. Crates collide
    // with the ball through the physics world, so they can push it around.
    const f64 crateHalf = kWorldCrateSize * 0.5;
    const f64 crateBoundX = world_.halfWidth() - crateHalf;
    const f64 crateBoundZ = world_.halfLength() - crateHalf;
    for (const u32 crateId : crateBodyIds_) {
      DynamicBox* crate = physics_.dynamicBox(crateId);
      if (crate == nullptr) continue;
      if (crate->position.x > crateBoundX) {
        crate->position.x = crateBoundX;
        if (crate->velocity.x > 0.0) crate->velocity.x = 0.0;
      } else if (crate->position.x < -crateBoundX) {
        crate->position.x = -crateBoundX;
        if (crate->velocity.x < 0.0) crate->velocity.x = 0.0;
      }
      if (crate->position.z > crateBoundZ) {
        crate->position.z = crateBoundZ;
        if (crate->velocity.z > 0.0) crate->velocity.z = 0.0;
      } else if (crate->position.z < -crateBoundZ) {
        crate->position.z = -crateBoundZ;
        if (crate->velocity.z < 0.0) crate->velocity.z = 0.0;
      }

      const f64 dx = crate->position.x - playerPos_.x;
      const f64 dz = crate->position.z - playerPos_.z;
      const f64 distance = std::sqrt(dx * dx + dz * dz);
      const f64 contact = crateHalf + kWorldPlayerRadius;
      if (distance < contact) {
        Vec3 pushNormal{1.0, 0.0, 0.0};
        if (distance > kMoveEpsilon) {
          pushNormal = Vec3{dx / distance, 0.0, dz / distance};
        } else if (moving) {
          pushNormal = Vec3{direction.x, 0.0, direction.z};
        }
        crate->position.x = playerPos_.x + pushNormal.x * contact;
        crate->position.z = playerPos_.z + pushNormal.z * contact;
        // Walking into the crate shoves it along.
        if (moving) {
          crate->velocity.x = direction.x * world_.player.speed;
          crate->velocity.z = direction.z * world_.player.speed;
        }
      }
      const f64 crateKickDistance = crateHalf + kWorldKickReach;
      if (moving && distance < crateKickDistance && crate->velocity.length() < kKickMaxSpeed) {
        crate->velocity = direction * (kickSpeed() * kWorldCrateKickScale) + Vec3{0.0, kWorldCrateKickUp, 0.0};
      }
    }

    const Vec3 position = ballPosition();
    if (holeScoring()) {
      // Hole scoring with a runner: kick the ball slowly into the cup.
      if (captureHole(position, ballVelocity().length())) {
        ++world_.score;
        screen_ = Screen::Goal;
        goalTimer_ = kGoalCelebration;
        events_.push_back(GameEvent::Holed);
      }
      return;
    }
    // Goal capture: the ball crosses a goal plane going -Z, inside the
    // posts and below the bar.
    std::map<std::string, GoalGroup> goals;
    scanGoals(world_.scene, goals);
    for (const auto& entry : goals) {
      const GoalGroup& goal = entry.second;
      if (!goal.valid()) continue;
      const bool crossedToward = previous.z >= goal.z() && position.z < goal.z();
      const bool crossedBack = matchMode() && previous.z <= goal.z() && position.z > goal.z();
      if ((crossedToward || crossedBack) && std::abs(position.x - goal.x()) < goal.width() * 0.5 &&
          position.y < goal.height()) {
        creditGoal(scoringTeamForGoalZ(goal.z()));
        screen_ = Screen::Goal;
        goalTimer_ = kGoalCelebration;
        events_.push_back(GameEvent::Goal);
        if (ball != nullptr) ball->velocity = Vec3{0.0, 0.0, 0.0};
        break;
      }
    }
    return;
  }
  if (screen == 16) {  // Goal celebration (a goal, or a holed ball)
    if (holeScoring()) {
      // The ball sits in the cup while we celebrate.
      SphereBody* ball = physics_.sphere(ballId_);
      if (ball != nullptr) ball->velocity = Vec3{0.0, 0.0, 0.0};
    } else {
      physics_.advance(hostSeconds);
    }
    goalTimer_ -= hostSeconds;
    if (goalTimer_ <= 0.0) {
      charging_ = false;
      power_ = 0.0;
      if (holeScoring()) {
        // The course goes on: the next cup is played from the cup just holed
        // (mini-golf style); after the last cup the round is over.
        scorecard_.push_back(strokes_);
        strokes_ = 0U;
        ++currentHole_;
        if (currentHole_ >= holeCount()) {
          // A complete round: every cup was holed, so the total counts as a
          // record attempt. Lower is better; the first finished round always
          // sets the record.
          const u32 total = totalStrokes();
          bestIsNew_ = total > 0U && (world_.bestRound == 0U || total < world_.bestRound);
          if (bestIsNew_) world_.bestRound = total;
          screen_ = Screen::RoundEnd;
          events_.push_back(GameEvent::RoundOver);
          return;
        }
        SphereBody* ball = physics_.sphere(ballId_);
        if (ball != nullptr) ball->velocity = Vec3{0.0, 0.0, 0.0};
        screen_ = Screen::Play;
        return;
      }
      // A match restarts from the center spot with the squads reset; an
      // endless kickabout just puts the ball back.
      if (matchMode()) {
        kickOff();
        if (matchOver_) {
          screen_ = Screen::RoundEnd;
          events_.push_back(GameEvent::RoundOver);
          return;
        }
      } else {
        resetBallToCenter();
      }
      screen_ = Screen::Play;
    }
    return;
  }
  if (screen == 21) {  // Round over: the scorecard waits for «دور جدید» / «منو»
    SphereBody* ball = physics_.sphere(ballId_);
    if (ball != nullptr) ball->velocity = Vec3{0.0, 0.0, 0.0};
    return;
  }
  // All menu screens: the ball waits at its spawn.
  resetBallToCenter();
}

void WorldEditor::resetBall() {
  resetBallToCenter();
  goalTimer_ = 0.0;
  charging_ = false;
  power_ = 0.0;
  if (holeScoring()) {
    // «توپ از نو» on a course restarts the round: back to the tee, cup 1,
    // a clean scorecard (a penalty-free mulligan of the whole round).
    strokes_ = 0U;
    startRound();
  }
  if (matchMode() && !matchOver_) {
    kickOff();
    events_.push_back(GameEvent::Whistle);  // the restart whistle
  }
  if (screen_ == Screen::Goal || (screen_ == Screen::RoundEnd && !matchOver_)) screen_ = Screen::Play;
}

// --- Shot mode ---

void WorldEditor::setShootHeld(bool held) { shootHeld_ = held; }

bool WorldEditor::ballAtRest() const {
  const SphereBody* ball = physics_.sphere(ballId_);
  if (ball == nullptr) return true;
  return ball->velocity.length() < kWorldShotStopSpeed && ball->position.y <= world_.ball.radius + 1e-3;
}

Vec3 WorldEditor::aimDirection() const { return Vec3{-std::sin(aimYaw_), 0.0, -std::cos(aimYaw_)}; }

f64 WorldEditor::shotSpeed(f64 power) const {
  return world_.profile.kickBase + std::min(1.0, std::max(0.0, power)) * world_.profile.kickSpeedScale;
}

void WorldEditor::setCurl(f64 curl) { curl_ = std::min(1.0, std::max(-1.0, curl)); }

Vec3 WorldEditor::ballSpin() const {
  const SphereBody* ball = physics_.sphere(ballId_);
  if (ball == nullptr) return Vec3{0.0, 0.0, 0.0};
  return ball->spin;
}

// The curl stick becomes spin about the vertical axis: positive curl bends
// the ball to the right of the aim. Taking the shot spends the stick.
Vec3 WorldEditor::takeCurlSpin() {
  const Vec3 spin{0.0, -curl_ * kWorldMaxCurl, 0.0};
  curl_ = 0.0;
  return spin;
}

void WorldEditor::shoot(f64 power) {
  SphereBody* ball = physics_.sphere(ballId_);
  charging_ = false;
  if (ball == nullptr) return;
  ball->velocity = aimDirection() * shotSpeed(power) + Vec3{0.0, world_.profile.kickUp, 0.0};
  ball->spin = takeCurlSpin();
  ++strokes_;
  power_ = 0.0;
  events_.push_back(GameEvent::Shot);
}

// Pick the team-mate a pass should find: on our side, ahead of the aim
// (within a generous cone) and nearest. 0 when there is nobody to pass to.
u32 WorldEditor::passTarget() const {
  const Vec3 aim = aimDirection();
  u32 best = 0U;
  f64 bestDistance = 0.0;
  for (const u32 id : physics_.characterIds()) {
    if (id == kPrimaryCharacter) continue;
    const CharacterBody* mate = physics_.characterById(id);
    if (mate == nullptr || mate->team != 1U) continue;  // only our own side
    const f64 dx = mate->position.x - playerPos_.x;
    const f64 dz = mate->position.z - playerPos_.z;
    const f64 distance = std::sqrt(dx * dx + dz * dz);
    if (distance < kMoveEpsilon) continue;
    // Ahead of the aim: at least 45 degrees off is behind us.
    if ((dx / distance) * aim.x + (dz / distance) * aim.z < 0.70710678) continue;
    if (best == 0U || distance < bestDistance) {
      best = id;
      bestDistance = distance;
    }
  }
  return best;
}

bool WorldEditor::pass() {
  SphereBody* ball = physics_.sphere(ballId_);
  if (ball == nullptr) return false;
  const u32 target = passTarget();
  if (target == 0U) return false;
  const CharacterBody* mate = physics_.characterById(target);
  if (mate == nullptr) return false;
  const f64 dx = mate->position.x - ball->position.x;
  const f64 dz = mate->position.z - ball->position.z;
  const f64 distance = std::sqrt(dx * dx + dz * dz);
  if (distance < kMoveEpsilon) return false;
  // A ground pass weighted to arrive: friction eats v^2 / (2*a) of range,
  // so aim for the speed that dies just past the receiver's feet.
  // The wet pitch is part of the sum: on a slick surface the ball keeps
  // running, so the same pass needs less weight on it.
  const f64 decel = (ball->friction + ball->rollingFriction) * kGravity * physics_.gripFactor();
  const f64 wanted = std::sqrt(std::max(2.0 * decel * distance * 1.15, 1e-6));
  ball->velocity = Vec3{dx / distance * wanted, 0.0, dz / distance * wanted};
  ball->spin = takeCurlSpin();
  events_.push_back(GameEvent::Kick);
  return true;
}

// --- Skill moves (stage 26) ---

const char* WorldEditor::trickName(Trick trick) {
  switch (trick) {
    case Trick::Nutmeg: return "NUTMEG";
    case Trick::Roulette: return "ROULETTE";
    case Trick::Juggle: return "JUGGLE";
    case Trick::None: break;
  }
  return "";
}

f64 WorldEditor::trickDuration(Trick trick) const {
  switch (trick) {
    case Trick::Nutmeg: return kTrickNutmegTime;
    case Trick::Roulette: return kTrickRouletteTime;
    case Trick::Juggle: return kTrickJuggleTime;
    case Trick::None: break;
  }
  return 0.0;
}

u32 WorldEditor::trickPoints(Trick trick) const {
  switch (trick) {
    case Trick::Nutmeg: return kTrickNutmegPoints;
    case Trick::Roulette: return kTrickRoulettePoints;
    case Trick::Juggle: return kTrickJugglePoints;
    case Trick::None: break;
  }
  return 0U;
}

// Is there an opponent close enough, and in front, to nutmeg? Without one
// there are no legs to put the ball through.
bool WorldEditor::opponentInFront(f64 range) const {
  const Vec3 aim = aimDirection();
  for (const u32 id : physics_.characterIds()) {
    if (id == kPrimaryCharacter) continue;
    const CharacterBody* other = physics_.characterById(id);
    if (other == nullptr || other->team == 1U) continue;  // only the other side
    const f64 dx = other->position.x - playerPos_.x;
    const f64 dz = other->position.z - playerPos_.z;
    const f64 distance = std::sqrt(dx * dx + dz * dz);
    if (distance < kMoveEpsilon || distance > range) continue;
    // Roughly ahead of us: past 45 degrees off the aim they are beside us,
    // and you cannot nutmeg someone you are not facing.
    if ((dx / distance) * aim.x + (dz / distance) * aim.z < 0.70710678) continue;
    return true;
  }
  return false;
}

bool WorldEditor::startTrick(Trick trick) {
  if (trick == Trick::None) return false;
  // A serious fixture has no time for showboating.
  if (!world_.profile.tricks) return false;
  if (!playing() || roundOver()) return false;
  // You cannot start a second trick to escape the first: committing is the
  // whole risk.
  if (trick_ != Trick::None) return false;
  const SphereBody* ball = physics_.sphere(ballId_);
  if (ball == nullptr) return false;
  // The ball has to be at your feet — you cannot nutmeg thin air.
  const f64 dx = ball->position.x - playerPos_.x;
  const f64 dz = ball->position.z - playerPos_.z;
  const f64 reach = world_.ball.radius + kWorldDribbleReach;
  if (std::sqrt(dx * dx + dz * dz) > reach) return false;
  // A nutmeg needs someone to nutmeg.
  if (trick == Trick::Nutmeg && !opponentInFront(kTrickNutmegRange)) return false;

  trick_ = trick;
  trickLength_ = trickDuration(trick);
  trickTimer_ = trickLength_;
  return true;
}

f64 WorldEditor::trickProgress() const {
  if (trick_ == Trick::None || trickLength_ <= 0.0) return 0.0;
  const f64 done = (trickLength_ - trickTimer_) / trickLength_;
  return done < 0.0 ? 0.0 : (done > 1.0 ? 1.0 : done);
}

// Runs the clock on the trick and pays out when it finishes. The payoff
// happens at the END: start one and lose the ball, and you get nothing.
void WorldEditor::updateTrick(f64 seconds) {
  if (trick_ == Trick::None) return;
  SphereBody* ball = physics_.sphere(ballId_);
  // Losing the ball mid-trick cancels it, with no points. This is the risk.
  if (ball != nullptr) {
    const f64 dx = ball->position.x - playerPos_.x;
    const f64 dz = ball->position.z - playerPos_.z;
    const f64 lost = world_.ball.radius + kWorldDribbleReach + kTrickLoseBall;
    if (std::sqrt(dx * dx + dz * dz) > lost) {
      trick_ = Trick::None;
      trickTimer_ = 0.0;
      trickLength_ = 0.0;
      return;
    }
  }

  trickTimer_ -= seconds;
  if (trickTimer_ > 0.0) return;

  // --- The trick lands ---
  const Trick finished = trick_;
  trick_ = Trick::None;
  trickTimer_ = 0.0;
  trickLength_ = 0.0;

  const Vec3 aim = aimDirection();
  if (ball != nullptr) {
    switch (finished) {
      case Trick::Nutmeg:
        // Knock it through and past them, along the aim, staying on the deck.
        ball->velocity = Vec3{aim.x * kTrickNutmegPush, 0.0, aim.z * kTrickNutmegPush};
        break;
      case Trick::Roulette:
        // Spin away: the player turns and takes the ball with them, so the
        // ball leaves along the NEW facing, gently, still under control.
        aimYaw_ += kTrickRouletteTurn;
        {
          const Vec3 turned = aimDirection();
          const f64 pace = world_.player.speed * kWorldDribbleSpeed;
          ball->velocity = Vec3{turned.x * pace, 0.0, turned.z * pace};
        }
        break;
      case Trick::Juggle:
        // Flick it up and keep it there — pure style, no ground gained.
        ball->velocity = Vec3{ball->velocity.x * 0.5, kTrickJuggleLift, ball->velocity.z * 0.5};
        break;
      case Trick::None: break;
    }
  }
  styleScore_ += trickPoints(finished);
  lastTrick_ = finished;
  events_.push_back(GameEvent::Trick);
}

// --- The laws of the game (stage 29) ---
//
// Only grass plays by these. An alley kickabout has no linesman, and
// stopping a street game for a throw-in would ruin it.

const char* WorldEditor::stoppageName(Stoppage stoppage) {
  switch (stoppage) {
    case Stoppage::ThrowIn: return "THROW IN";
    case Stoppage::Offside: return "OFFSIDE";
    case Stoppage::Foul: return "FOUL";
    case Stoppage::None: break;
  }
  return "";
}

std::string WorldEditor::rulesHudText() const {
  if (stoppage_ == Stoppage::None) return std::string();
  // Whose ball it is matters more than the decision itself.
  const std::string side = restartTeam_ == 1U ? "MA BALL" : "ANHA BALL";
  return std::string(stoppageName(stoppage_)) + "  " + side;
}

f64 WorldEditor::currentPlayerSpeed() const {
  // No stamina in the profile means the endless runner every other game
  // has always had: full pace, forever.
  if (world_.profile.stamina <= 0.0) return world_.player.speed;
  // A spent player is slower, never stopped: you tire, you do not seize up.
  const f64 pace = kRulesTiredPace + (1.0 - kRulesTiredPace) * stamina_;
  return world_.player.speed * pace;
}

void WorldEditor::updateStamina(f64 seconds, bool running) {
  // A profile without stamina keeps the endless runner. The rate below
  // would already be zero, so this is not what makes that true — it just
  // pins the value at exactly 1.0 and skips the arithmetic.
  if (world_.profile.stamina <= 0.0) {
    stamina_ = 1.0;
    return;
  }
  // Running drains, standing recovers — and recovery is slower than the
  // drain, so you cannot sprint the whole match.
  const f64 rate = running ? -kRulesStaminaDrain * world_.profile.stamina
                           : kRulesStaminaRecover * world_.profile.stamina;
  stamina_ += rate * seconds;
  if (stamina_ < 0.0) stamina_ = 0.0;
  if (stamina_ > 1.0) stamina_ = 1.0;
}

void WorldEditor::awardRestart(Stoppage reason, u32 team, const Vec3& spot) {
  stoppage_ = reason;
  restartTeam_ = team;
  restartSpot_ = spot;
  restartTimer_ = kRulesRestartPause;
  // Dead ball: park it on the restart spot and stop everything.
  SphereBody* ball = physics_.sphere(ballId_);
  if (ball != nullptr) {
    ball->position = Vec3{spot.x, world_.ball.radius, spot.z};
    ball->velocity = Vec3{0.0, 0.0, 0.0};
    ball->spin = Vec3{0.0, 0.0, 0.0};
  }
  // A trick in progress is over: the whistle has gone.
  trick_ = Trick::None;
  trickTimer_ = 0.0;
  trickLength_ = 0.0;
  events_.push_back(GameEvent::Whistle);
}

// Is a team-mate beyond the last defender, and therefore offside? Level is
// ONSIDE, which is the rule people always get wrong.
bool WorldEditor::offsideFor(u32 id) const {
  if (!world_.profile.rules) return false;
  const CharacterBody* mate = physics_.characterById(id);
  if (mate == nullptr || mate->team != 1U) return false;
  // Team 1 attacks -Z, so "further forward" means a smaller z.
  // Find the last defender: the opponent nearest their own goal line.
  f64 lastDefenderZ = -world_.halfLength();
  bool found = false;
  for (const u32 other : physics_.characterIds()) {
    const CharacterBody* body = physics_.characterById(other);
    if (body == nullptr || body->team != 2U) continue;
    // The offside line is the opposition's HIGHEST defender: team 1
    // attacks -Z, so that is the opponent with the largest z. Anyone in
    // front of that line has nobody left to play them onside.
    if (!found || body->position.z > lastDefenderZ) {
      lastDefenderZ = body->position.z;
      found = true;
    }
  }
  if (!found) return false;
  // Offside only in the opposition half.
  if (mate->position.z > 0.0) return false;
  return mate->position.z < lastDefenderZ - kRulesOffsideMargin;
}

// Watches for the ball leaving the pitch and for reckless challenges.
// `previousBall` is where the ball was before this frame's physics, so a
// ball that shot out and got clamped back is still caught.
void WorldEditor::updateRules(f64 seconds, const Vec3& previousBall) {
  if (!world_.profile.rules) return;

  // --- Serving a stoppage ---
  if (stoppage_ != Stoppage::None) {
    restartTimer_ -= seconds;
    // Hold the ball still on the spot while the whistle has gone.
    SphereBody* held = physics_.sphere(ballId_);
    if (held != nullptr) {
      held->position = Vec3{restartSpot_.x, world_.ball.radius, restartSpot_.z};
      held->velocity = Vec3{0.0, 0.0, 0.0};
    }
    if (restartTimer_ <= 0.0) {
      // Ball is live again.
      stoppage_ = Stoppage::None;
      restartTimer_ = 0.0;
      restartTeam_ = 0U;
    }
    return;
  }

  const SphereBody* ball = physics_.sphere(ballId_);
  if (ball == nullptr) return;

  // --- Out of play: a touchline throw-in ---
  // The ball is clamped inside the pitch by the play loop, so check where
  // it WANTED to go rather than where it ended up.
  const f64 touchline = world_.halfWidth() - world_.ball.radius;
  if (std::abs(previousBall.x) >= touchline - 1e-6 && std::abs(ball->position.x) >= touchline - 1e-6) {
    // Whoever did NOT put it out gets the throw. The player is team 1, so
    // if the player was the last to touch it, it is theirs.
    const f64 side = ball->position.x > 0.0 ? 1.0 : -1.0;
    const u32 team = 2U;  // conceded by the side in possession (the player)
    awardRestart(Stoppage::ThrowIn, team,
                 Vec3{side * (touchline - kRulesRestartInset), 0.0, ball->position.z});
    return;
  }

  // --- Offside ---
  // Flagged when the ball is played to a team-mate who was beyond the last
  // defender. Checked against the pass target, which is who the ball is
  // actually going to.
  const u32 target = passTarget();
  if (target != 0U && offsideFor(target) && ball->velocity.length() > 1.0) {
    const CharacterBody* mate = physics_.characterById(target);
    if (mate != nullptr) {
      awardRestart(Stoppage::Offside, 2U, Vec3{mate->position.x, 0.0, mate->position.z});
      return;
    }
  }

  // --- Fouls ---
  // Charging into an opponent at speed is a foul. A tackle for the ball is
  // fair; running through somebody is not.
  const Vec3 playerVelocity = physics_.character() != nullptr ? physics_.character()->velocity : Vec3{0.0, 0.0, 0.0};
  const f64 chargeSpeed = std::sqrt(playerVelocity.x * playerVelocity.x + playerVelocity.z * playerVelocity.z);
  if (chargeSpeed <= kRulesFoulSpeed) return;
  for (const u32 id : physics_.characterIds()) {
    if (id == kPrimaryCharacter) continue;
    const CharacterBody* other = physics_.characterById(id);
    if (other == nullptr || other->team == 1U) continue;
    const f64 dx = other->position.x - playerPos_.x;
    const f64 dz = other->position.z - playerPos_.z;
    const f64 distance = std::sqrt(dx * dx + dz * dz);
    if (distance > kWorldPlayerRadius * 2.0) continue;
    // Only a charge INTO them counts: running away from somebody is not a
    // foul however fast you do it.
    if (dx * playerVelocity.x + dz * playerVelocity.z <= 0.0) continue;
    awardRestart(Stoppage::Foul, 2U, Vec3{other->position.x, 0.0, other->position.z});
    return;
  }
}

// --- Camera director (stage 28) ---
//
// The app used to decide all of this inline, which meant none of it could
// be tested. It lives here now: the app just asks where to look and how
// far back to stand.

Vec3 WorldEditor::cameraTarget() const {
  if (placing() || movingObject()) return Vec3{ghost_.x, 0.2, ghost_.z};
  if (!playing()) {
    const EntityData* selected = selectedEntity();
    if (selectingObject() && selected != nullptr) return selected->transform.position;
    return Vec3{0.0, 0.2, 0.0};
  }
  const Vec3 ball = ballPosition();
  if (world_.profile.camera != CameraStyle::Broadcast) return ball;
  // Broadcast: frame the play between the ball and the player, weighted
  // toward the ball because that is what the viewer is actually watching.
  return Vec3{ball.x * kCameraBallBias + playerPos_.x * (1.0 - kCameraBallBias), ball.y,
              ball.z * kCameraBallBias + playerPos_.z * (1.0 - kCameraBallBias)};
}

f64 WorldEditor::cameraDistance(f64 restingDistance) const {
  if (!playing() || world_.profile.camera != CameraStyle::Broadcast) return restingDistance;
  // Pull back as the ball and the player separate, so a long ball never
  // leaves half the play off screen.
  const Vec3 ball = ballPosition();
  const f64 dx = ball.x - playerPos_.x;
  const f64 dz = ball.z - playerPos_.z;
  const f64 spread = std::sqrt(dx * dx + dz * dz);
  const f64 wanted = kCameraBroadcastNear + spread * kCameraBroadcastPerMeter;
  return std::min(kCameraBroadcastFar, std::max(kCameraBroadcastNear, wanted));
}

bool WorldEditor::cameraFollowsAim() const {
  if (!playing() || roundOver()) return false;
  // Only a chase camera swings around behind the aim. A broadcast camera
  // holds its side of the pitch, like a real touchline camera: swinging it
  // around behind the player every time they turn would be unwatchable.
  return world_.profile.camera == CameraStyle::Chase;
}

// --- Computer players (stage 27) ---
//
// The whole design is one idea: only ONE player per side goes for the
// ball. Everyone else holds a shape. Without that rule every character
// runs at the ball at once and a match becomes a scrum.

// Which way this team is attacking: team 1 defends +Z and shoots toward
// -Z, team 2 the other way round. Matches scoringTeamForGoalZ.
namespace {
f64 attackDirectionZ(u32 team) { return team == 1U ? -1.0 : 1.0; }
}  // namespace

u32 WorldEditor::aiChaser(u32 team) const {
  if (!aiActive()) return 0U;
  const SphereBody* ball = physics_.sphere(ballId_);
  if (ball == nullptr) return 0U;
  const u32 keeper = aiKeeper(team);
  u32 best = 0U;
  f64 bestDistance = 0.0;
  for (const u32 id : physics_.characterIds()) {
    // The human is never picked: the player chases their own ball.
    if (id == kPrimaryCharacter) continue;
    const CharacterBody* body = physics_.characterById(id);
    if (body == nullptr || body->team != team) continue;
    if (id == keeper) continue;  // the keeper minds the net, not the ball
    const f64 dx = ball->position.x - body->position.x;
    const f64 dz = ball->position.z - body->position.z;
    const f64 distance = std::sqrt(dx * dx + dz * dz);
    if (best == 0U || distance < bestDistance) {
      best = id;
      bestDistance = distance;
    }
  }
  return best;
}

u32 WorldEditor::aiKeeper(u32 team) const {
  if (!aiActive()) return 0U;
  // A side needs somebody to spare: with one outfield player, that player
  // goes for the ball rather than standing on the line.
  u32 count = 0U;
  for (const u32 id : physics_.characterIds()) {
    if (id == kPrimaryCharacter) continue;
    const CharacterBody* body = physics_.characterById(id);
    if (body != nullptr && body->team == team) ++count;
  }
  if (count < 2U) return 0U;

  // The keeper is whoever is deepest in their own half.
  const f64 ownGoalZ = -attackDirectionZ(team) * world_.halfLength();
  u32 best = 0U;
  f64 bestDistance = 0.0;
  for (const u32 id : physics_.characterIds()) {
    if (id == kPrimaryCharacter) continue;
    const CharacterBody* body = physics_.characterById(id);
    if (body == nullptr || body->team != team) continue;
    const f64 distance = std::abs(body->position.z - ownGoalZ);
    if (best == 0U || distance < bestDistance) {
      best = id;
      bestDistance = distance;
    }
  }
  return best;
}

Vec3 WorldEditor::aiTargetFor(u32 id) const {
  const CharacterBody* body = physics_.characterById(id);
  if (body == nullptr || id == kPrimaryCharacter) return Vec3{0.0, 0.0, 0.0};
  const SphereBody* ball = physics_.sphere(ballId_);
  if (ball == nullptr) return body->position;
  const u32 team = body->team;
  const f64 forward = attackDirectionZ(team);
  const f64 ownGoalZ = -forward * world_.halfLength();

  // --- Keeper: stay on the line, shuffle across to the ball ---
  if (id == aiKeeper(team)) {
    // A keeper tracks the ball sideways but never wanders far off the
    // line: an empty net is worse than a shot saved.
    const f64 postLimit = kWorldGoalLarge * 0.5;
    const f64 x = std::min(postLimit, std::max(-postLimit, ball->position.x));
    // Comes out a little when the ball is close, back on the line when not.
    const f64 ballDepth = std::abs(ball->position.z - ownGoalZ);
    const f64 comeOut = ballDepth < kAiKeeperRange * 2.0 ? kAiKeeperRange : kAiKeeperRange * 0.35;
    return Vec3{x, body->position.y, ownGoalZ + forward * comeOut};
  }

  // --- Chaser: go and get it ---
  if (id == aiChaser(team)) return Vec3{ball->position.x, body->position.y, ball->position.z};

  // --- Everyone else: hold a shape relative to the ball ---
  // Sit goal-side of the ball, spread across the width, so the side keeps
  // its shape and there is somebody to pass to.
  const f64 supportZ = ball->position.z - forward * kAiSupportGap;
  const f64 limit = world_.halfLength() - kPlayerMargin;
  // Fan out from the ball rather than all standing on the same spot: the
  // character id gives each one a stable slot.
  const f64 spread = (static_cast<f64>(id % 3U) - 1.0) * kAiSupportGap;
  const f64 boundX = world_.halfWidth() - kPlayerMargin;
  return Vec3{std::min(boundX, std::max(-boundX, ball->position.x + spread)), body->position.y,
              std::min(limit, std::max(-limit, supportZ))};
}

void WorldEditor::updateAi(f64 seconds) {
  if (!aiActive() || seconds <= 0.0) return;
  SphereBody* ball = physics_.sphere(ballId_);
  const f64 skill = world_.profile.aiSkill;
  // Skill scales how fast they close you down. Even a perfect one is a
  // shade slower than the human, so a good player can still beat them.
  const f64 speed = world_.player.speed * kAiMaxSpeedFactor * skill;
  const f64 boundX = world_.halfWidth() - kPlayerMargin;
  const f64 boundZ = world_.halfLength() - kPlayerMargin;

  for (const u32 id : physics_.characterIds()) {
    if (id == kPrimaryCharacter) continue;  // the human drives themself
    CharacterBody* body = physics_.characterById(id);
    if (body == nullptr) continue;
    const Vec3 target = aiTargetFor(id);
    const f64 dx = target.x - body->position.x;
    const f64 dz = target.z - body->position.z;
    const f64 distance = std::sqrt(dx * dx + dz * dz);
    Vec3 wanted{0.0, 0.0, 0.0};
    // Do not jitter on the spot once they have arrived.
    if (distance > kMoveEpsilon) {
      // Ease off over the last stride so they settle instead of overshooting.
      const f64 pace = std::min(speed, distance / std::max(seconds, 1e-4));
      wanted = Vec3{dx / distance * pace, 0.0, dz / distance * pace};
    }
    physics_.moveCharacter(id, seconds, wanted);
    // Keep them on the pitch, like the human.
    body->position.x = std::min(boundX, std::max(-boundX, body->position.x));
    body->position.z = std::min(boundZ, std::max(-boundZ, body->position.z));

    // --- Tackling ---
    // An opponent who reaches the ball knocks it away toward their own
    // attacking end. This is what makes a trick risky: start showing off
    // in front of a defender and they will take it off you.
    if (ball == nullptr || body->team == 1U) continue;
    const f64 ballDx = ball->position.x - body->position.x;
    const f64 ballDz = ball->position.z - body->position.z;
    if (std::sqrt(ballDx * ballDx + ballDz * ballDz) > world_.ball.radius + kAiTackleReach) continue;
    const f64 forward = attackDirectionZ(body->team);
    ball->velocity.x = ballDx * skill;
    ball->velocity.z = forward * kAiTacklePush * skill;
    events_.push_back(GameEvent::Tackle);
  }
}

std::string WorldEditor::trickHudText() const {
  if (!world_.profile.tricks) return std::string();
  // A trick in progress is the more urgent thing to show.
  if (trick_ != Trick::None) return std::string(trickName(trick_)) + "!";
  if (styleScore_ == 0U) return std::string();
  return "STYLE " + std::to_string(styleScore_);
}

// Hole capture: within kWorldHoleCapture of a cup centre (horizontally) and
// slower than kWorldHoleCaptureSpeed — a fast ball rolls over the cup. The
// ball is parked in the cup so the render shows it there.
bool WorldEditor::captureHole(const Vec3& position, f64 speed) {
  if (speed >= kWorldHoleCaptureSpeed) return false;
  // Only the cup being played captures: the others are just marks on the
  // course until their turn comes (a ball rolling over Hole_2 while playing
  // Hole_1 keeps rolling).
  const EntityData* hole = world_.scene.get(world_.scene.find(currentHoleName()));
  if (hole == nullptr) return false;
  const f64 dx = position.x - hole->transform.position.x;
  const f64 dz = position.z - hole->transform.position.z;
  if (std::sqrt(dx * dx + dz * dz) >= kWorldHoleCapture) return false;
  SphereBody* ball = physics_.sphere(ballId_);
  if (ball != nullptr) {
    ball->position = Vec3{hole->transform.position.x, world_.ball.radius, hole->transform.position.z};
    ball->velocity = Vec3{0.0, 0.0, 0.0};
  }
  return true;
}

// --- The course: cups in name order ---

std::vector<std::string> WorldEditor::sortedHoleNames() const {
  std::vector<std::pair<u32, std::string>> cups;
  world_.scene.forEach([&cups](EntityHandle, const EntityData& entity) {
    if (objectKindForName(entity.name) != ObjectKind::Hole) return;
    u32 number = 0U;
    for (usize i = 5U; i < entity.name.size(); ++i) {  // after "Hole_"
      const char c = entity.name[i];
      if (c < '0' || c > '9') {
        number = 0U;
        break;
      }
      number = number * 10U + static_cast<u32>(c - '0');
    }
    cups.emplace_back(number, entity.name);
  });
  std::sort(cups.begin(), cups.end());
  std::vector<std::string> names;
  names.reserve(cups.size());
  for (const auto& cup : cups) names.push_back(cup.second);
  return names;
}

std::string WorldEditor::currentHoleName() const {
  const std::vector<std::string> cups = sortedHoleNames();
  return currentHole_ < cups.size() ? cups[currentHole_] : std::string{};
}

void WorldEditor::startRound() {
  currentHole_ = 0U;
  scorecard_.clear();
  bestIsNew_ = false;  // the new round has not beaten anything yet
}

// --- Wind ---

Vec3 WorldEditor::windVector() const {
  if (!windActive()) return Vec3{0.0, 0.0, 0.0};
  return Vec3{-std::sin(world_.profile.windDirection), 0.0, -std::cos(world_.profile.windDirection)};
}

// Rain wets the pitch by itself, but a profile can also start it wet (a
// pitch soaked before kick-off) — so the slickness is whichever is greater.
f64 WorldEditor::pitchWetness() const {
  const f64 fromRain = world_.profile.rain;
  const f64 fromProfile = world_.profile.wetness;
  return fromRain > fromProfile ? fromRain : fromProfile;
}

bool WorldEditor::raining() const { return world_.profile.rain > 0.0; }

// Night is before sunrise or after sunset. The hours are deliberately plain
// numbers rather than a solar model: this is a game, not an almanac.
bool WorldEditor::night() const {
  const f64 hour = world_.profile.hour;
  return hour < kWorldSunrise || hour >= kWorldSunset;
}

// How high the sun sits, -1 (deep night) .. 1 (noon). It follows a simple
// cosine over the day so dawn and dusk are gentle rather than a switch.
f64 WorldEditor::sunHeight() const {
  const f64 dayFraction = world_.profile.hour / 24.0;
  return -std::cos(dayFraction * 2.0 * 3.14159265358979323846);
}

// Daylight, 0 (pitch dark) .. 1 (full noon sun). Floodlights mean a night
// match is never truly black, so it floors at kWorldNightLight.
f64 WorldEditor::daylight() const {
  const f64 height = sunHeight();
  const f64 lit = height <= 0.0 ? 0.0 : height;
  const f64 clouded = lit * (1.0 - 0.6 * world_.profile.rain);  // rain dims the sky
  return clouded < kWorldNightLight ? kWorldNightLight : clouded;
}

std::string WorldEditor::skyHudText() const {
  // Only worth a line when the weather or the hour is actually notable.
  const bool wet = pitchWetness() > 0.0;
  if (!raining() && !wet && !night()) return std::string{};
  std::ostringstream out;
  const i32 hourPart = static_cast<i32>(world_.profile.hour);
  const i32 minutePart = static_cast<i32>((world_.profile.hour - static_cast<f64>(hourPart)) * 60.0 + 0.5);
  out << (hourPart < 10 ? "0" : "") << hourPart << ':' << (minutePart < 10 ? "0" : "") << minutePart;
  if (night()) out << " NIGHT";
  if (raining()) out << " RAIN";
  if (wet) out << " WET";
  return out.str();
}

std::string WorldEditor::windHudText() const {
  if (!windActive()) return std::string{};
  // Turn the wind into the player's frame: the camera looks along the aim,
  // so a wind blowing across the aim reads as left/right and one blowing
  // along it reads as head/tail.
  const f64 relative = world_.profile.windDirection - aimYaw_;
  const f64 forward = std::cos(relative);   // +1 = blowing where I aim (tail)
  const f64 side = std::sin(relative);      // +1 = blowing to my left
  const char* arrow = "^";                  // tailwind: pushes the ball on
  if (std::abs(side) > std::abs(forward)) {
    arrow = side > 0.0 ? "<-" : "->";
  } else if (forward < 0.0) {
    arrow = "v";  // headwind: holds the ball back
  }
  // The strength is shown rounded to the nearest whole m/s^2 — the player
  // needs "how much", not six decimals.
  const i64 strength = static_cast<i64>(std::llround(world_.profile.windSpeed));
  return "WIND " + std::to_string(strength) + " " + arrow;
}

u32 WorldEditor::totalStrokes() const {
  u32 total = 0U;
  for (const u32 strokes : scorecard_) total += strokes;
  return total;
}

i32 WorldEditor::scoreToPar() const {
  return static_cast<i32>(totalStrokes()) - static_cast<i32>(par() * static_cast<u32>(scorecard_.size()));
}

usize WorldEditor::holeCount() const {
  usize count = 0U;
  world_.scene.forEach([&count](EntityHandle, const EntityData& entity) {
    if (objectKindForName(entity.name) == ObjectKind::Hole) ++count;
  });
  return count;
}

void WorldEditor::backToMenu() {
  if (playing()) {
    resetBallToCenter();
    goalTimer_ = 0.0;
    screen_ = Screen::Builder;
  }
}

usize WorldEditor::goalCount() const {
  std::map<std::string, GoalGroup> goals;
  scanGoals(world_.scene, goals);
  usize count = 0U;
  for (const auto& entry : goals) {
    if (entry.second.valid()) ++count;
  }
  return count;
}

usize WorldEditor::objectCount() const {
  usize count = 0U;
  world_.scene.forEach([&count](EntityHandle, const EntityData& entity) {
    if (entity.name != "Ground") ++count;
  });
  return count;
}

f64 WorldEditor::kickSpeed() const {
  return world_.profile.kickBase + world_.player.speed * world_.profile.kickSpeedScale;
}

// --- HUD / events / camera ---

std::vector<std::string> WorldEditor::hudLines() const {
  std::vector<std::string> lines;
  if (!playing()) return lines;
  if (holeScoring()) {
    const usize cups = holeCount();
    if (screen_ == Screen::RoundEnd) {
      const u32 coursePar = par() * static_cast<u32>(scorecard_.size());
      const i32 diff = scoreToPar();
      std::string verdict = "EVEN";
      if (diff < 0) verdict = std::to_string(-diff) + " UNDER";
      if (diff > 0) verdict = std::to_string(diff) + " OVER";
      lines.push_back("ROUND OVER  " + std::to_string(totalStrokes()) + " (PAR " + std::to_string(coursePar) + ")  " +
                      verdict);
      std::string card = "CARD";
      for (const u32 strokes : scorecard_) card += " " + std::to_string(strokes);
      lines.push_back(card);
      // The personal record: shouted when this round just set it, quiet
      // otherwise. Nothing at all until a round has ever been finished.
      if (bestIsNew_) {
        lines.push_back("NEW BEST " + std::to_string(world_.bestRound));
      } else if (world_.bestRound > 0U) {
        lines.push_back("BEST " + std::to_string(world_.bestRound));
      }
      return lines;
    }
    lines.push_back("HOLE " + std::to_string(std::min(currentHole_ + 1U, cups)) + "/" + std::to_string(cups) +
                    "  PAR " + std::to_string(par()));
    if (screen_ == Screen::Goal) {
      lines.push_back("IN! " + std::to_string(strokes_) + (strokes_ == 1U ? " STROKE" : " STROKES"));
    } else {
      lines.push_back("STROKE " + std::to_string(strokes_) + "  TOTAL " + std::to_string(totalStrokes() + strokes_));
    }
    const std::string wind = windHudText();
    if (!wind.empty()) lines.push_back(wind);
    {
      const std::string sky = skyHudText();
      if (!sky.empty()) lines.push_back(sky);
      const std::string trick = trickHudText();
      if (!trick.empty()) lines.push_back(trick);
    }
    return lines;
  }
  if (matchMode()) {
    if (screen_ == Screen::RoundEnd) {
      const u32 winner = matchWinner();
      lines.push_back("FULL TIME  " + matchScoreText());
      lines.push_back(winner == 0U ? "DRAW" : (winner == 1U ? "MA BORDIM" : "ANHA BORDAND"));
      return lines;
    }
    lines.push_back(matchScoreText() + "  " + matchClockText());
    if (screen_ == Screen::Goal) lines.push_back("GOAL!");
    // A stoppage is the most urgent thing on the screen: the player needs
    // to know why the ball has stopped.
    const std::string rules = rulesHudText();
    if (!rules.empty()) lines.push_back(rules);
    // The closing stretch: tell the player the clock is nearly gone, the
    // way a stadium clock turns red.
    if (!matchOver_ && matchClock_ > 0.0 && matchClock_ <= kMatchFinalWhistleWarning) {
      lines.push_back("LAST " + std::to_string(static_cast<i64>(std::ceil(matchClock_ - 1e-9))) + "S");
    }
    const std::string matchWind = windHudText();
    if (!matchWind.empty()) lines.push_back(matchWind);
    {
      const std::string sky = skyHudText();
      if (!sky.empty()) lines.push_back(sky);
      const std::string trick = trickHudText();
      if (!trick.empty()) lines.push_back(trick);
    }
    return lines;
  }
  lines.push_back("SCORE " + std::to_string(world_.score));
  if (screen_ == Screen::Goal) lines.push_back("GOAL!");
  const std::string wind = windHudText();
  if (!wind.empty()) lines.push_back(wind);
  {
    const std::string sky = skyHudText();
    if (!sky.empty()) lines.push_back(sky);
    const std::string trick = trickHudText();
    if (!trick.empty()) lines.push_back(trick);
  }
  return lines;
}

std::vector<WorldEditor::GameEvent> WorldEditor::drainEvents() {
  std::vector<GameEvent> drained;
  drained.swap(events_);
  return drained;
}

std::string WorldEditor::statsLine() const {
  std::ostringstream line;
  line << "KIMIA WORLD | " << screenName(static_cast<int>(screen_)) << " | world " << world_.name
       << " | game " << world_.profile.name << " | player " << playerSpeedName(world_.player.speed)
       << " | ball " << ballTypeName(world_.ball.type) << " | env " << environmentName(world_.environment)
       << " | score " << world_.score << " | objects " << objectCount();
  if (shotMode()) {
    line << " | stroke " << strokes_ << " | power " << static_cast<i32>(power_ * 100.0) << "%";
  }
  if (holeScoring()) {
    const usize cups = holeCount();
    line << " | hole " << std::min(currentHole_ + 1U, cups) << "/" << cups << " | total " << totalStrokes()
         << " | par " << par() << " | best " << world_.bestRound;
  }
  if (matchMode()) {
    line << " | match " << world_.scoreTeam1 << "-" << world_.scoreTeam2 << " | clock " << matchClockText();
  }
  if (windActive()) {
    line << " | wind " << static_cast<i32>(std::llround(world_.profile.windSpeed));
  }
  if (!lastError_.empty()) line << " | note " << lastError_;
  return line.str();
}

}  // namespace kimia
