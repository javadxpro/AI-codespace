#include <kimia/World.h>
#include <kimia/WorldIO.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <sstream>

namespace kimia {

namespace {

constexpr f64 kGoalCelebration = 2.0;  // seconds after a goal (or a holed ball)
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
        if (previous.z >= goal.z() && position.z < goal.z() && std::abs(position.x - goal.x()) < goal.width() * 0.5 &&
            position.y < goal.height()) {
          ++world_.score;
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
    physics_.moveCharacter(hostSeconds, direction * world_.player.speed);
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
      const f64 kickDistance = world_.ball.radius + kWorldKickReach;
      if (moving && distance < kickDistance && ball->velocity.length() < kKickMaxSpeed) {
        ball->velocity = direction * kickSpeed() + Vec3{0.0, world_.profile.kickUp, 0.0};
        events_.push_back(GameEvent::Kick);
      }
    }

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
      if (previous.z >= goal.z() && position.z < goal.z() && std::abs(position.x - goal.x()) < goal.width() * 0.5 &&
          position.y < goal.height()) {
        ++world_.score;
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
      resetBallToCenter();
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
  if (screen_ == Screen::Goal || screen_ == Screen::RoundEnd) screen_ = Screen::Play;
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

void WorldEditor::shoot(f64 power) {
  SphereBody* ball = physics_.sphere(ballId_);
  charging_ = false;
  if (ball == nullptr) return;
  ball->velocity = aimDirection() * shotSpeed(power) + Vec3{0.0, world_.profile.kickUp, 0.0};
  ++strokes_;
  power_ = 0.0;
  events_.push_back(GameEvent::Shot);
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
    return lines;
  }
  lines.push_back("SCORE " + std::to_string(world_.score));
  if (screen_ == Screen::Goal) lines.push_back("GOAL!");
  const std::string wind = windHudText();
  if (!wind.empty()) lines.push_back(wind);
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
  if (windActive()) {
    line << " | wind " << static_cast<i32>(std::llround(world_.profile.windSpeed));
  }
  if (!lastError_.empty()) line << " | note " << lastError_;
  return line.str();
}

}  // namespace kimia
