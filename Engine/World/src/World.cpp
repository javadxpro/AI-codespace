#include <kimia/World.h>
#include <kimia/WorldIO.h>

#include <kimia/Golf.h>  // the "accurate" ball IS the golf tuning

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <sstream>

namespace kimia {

namespace {

constexpr f64 kGoalCelebration = 2.0;  // seconds after a goal
constexpr f64 kKickMaxSpeed = 8.0;     // a ball rolling faster is not re-kicked
constexpr f64 kMoveEpsilon = 1e-6;
constexpr f64 kPlayerMargin = 0.6;  // keep the player inside the floor

const char* playerSpeedName(f64 speed) {
  if (speed >= kWorldPlayerFast - 0.5) return "fast";
  if (speed <= kWorldPlayerSlow + 0.5) return "slow";
  return "normal";
}

const char* ballTypeName(BallType type) { return type == BallType::Fantasy ? "fantasy" : "accurate"; }

const char* environmentName(EnvironmentKind kind) {
  switch (kind) {
    case EnvironmentKind::Sand:
      return "sand";
    case EnvironmentKind::Night:
      return "night";
    default:
      return "grass";
  }
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
    default: return "MODELSIZE";
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
    ball.radius = kGolfBallRadius;
    ball.restitution = kGolfBallRestitution;
    ball.friction = kGolfBallFriction;
    ball.rollingFriction = kGolfBallRollingFriction;
    ball.color = Vec3{0.95, 0.95, 0.92};
  }
}

EnvironmentColors environmentColors(EnvironmentKind kind) {
  switch (kind) {
    case EnvironmentKind::Sand:
      return EnvironmentColors{Vec3{0.76, 0.70, 0.50}, Vec3{0.78, 0.60, 0.38}};
    case EnvironmentKind::Night:
      return EnvironmentColors{Vec3{0.16, 0.26, 0.20}, Vec3{0.03, 0.04, 0.10}};
    default:
      return EnvironmentColors{Vec3{0.22, 0.45, 0.24}, Vec3{0.40, 0.62, 0.88}};
  }
}

void buildEmptyWorldScene(WorldData& world) {
  world.scene.clear();
  EntityData ground;
  ground.name = "Ground";
  ground.mesh = MeshKind::plane;
  ground.transform.scale = Vec3{kWorldFloorHalf * 2.0, 1.0, kWorldFloorHalf * 2.0};
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

WorldEditor::WorldEditor() { rebuildPhysics(); }

void WorldEditor::rebuildPhysics() {
  physics_.clear();
  physics_.addPlane(0.0);
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
  const f64 raised = physics_.resolveSpawnHeight(ball.position, ball.radius, kWorldFloorHalf);
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

void WorldEditor::createWorld() {
  world_ = WorldData{};
  buildEmptyWorldScene(world_);
  applyBallType(world_.ball, BallType::Accurate);
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

void WorldEditor::enterPlay() {
  playerPos_ = playerRest();
  physics_.resetCharacter(playerPos_);  // feet on the ground, velocity zero
  jumpQueued_ = false;
  moveInput_ = Vec3{0.0, 0.0, 0.0};
  goalTimer_ = 0.0;
  // Rebuild the physics world: the ball and every crate reset to their
  // placed spots and velocities.
  rebuildPhysics();
  lastError_.clear();
  screen_ = Screen::Play;
}

void WorldEditor::update(f64 hostSeconds) {
  const int screen = static_cast<int>(screen_);
  if (screen == 9) {  // Place: move the ghost with the arrows.
    const f64 speed = fine_ ? kWorldPlaceSpeedFine : kWorldPlaceSpeed;
    ghost_.x += moveInput_.x * speed * hostSeconds;
    ghost_.z += moveInput_.z * speed * hostSeconds;
    resetBallToCenter();
    return;
  }
  if (screen == 11) {  // Move: move the selected object live.
    if (managedIndex_ < managed_.size()) {
      EntityData* entity = world_.scene.get(managed_[managedIndex_]);
      if (entity != nullptr) {
        const f64 speed = fine_ ? kWorldPlaceSpeedFine : kWorldPlaceSpeed;
        entity->transform.position.x += moveInput_.x * speed * hostSeconds;
        entity->transform.position.z += moveInput_.z * speed * hostSeconds;
      }
    }
    resetBallToCenter();
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
    if (jumpQueued_ && physics_.characterJump(kWorldJumpHeight)) {
      jumpQueued_ = false;
    }
    physics_.moveCharacter(hostSeconds, direction * world_.player.speed);
    playerPos_ = physics_.character()->position;
    const f64 bound = kWorldFloorHalf - kPlayerMargin;
    playerPos_.x = std::min(bound, std::max(-bound, playerPos_.x));
    playerPos_.z = std::min(bound, std::max(-bound, playerPos_.z));

    const Vec3 previous = ballPosition();
    physics_.advance(hostSeconds);

    // The ball stays on the floor: clamp it inside the play area and stop
    // any outward motion so it can never roll away forever.
    SphereBody* ball = physics_.sphere(ballId_);
    const f64 ballBound = kWorldFloorHalf - world_.ball.radius;
    if (ball != nullptr) {
      if (ball->position.x > ballBound) {
        ball->position.x = ballBound;
        if (ball->velocity.x > 0.0) ball->velocity.x = 0.0;
      } else if (ball->position.x < -ballBound) {
        ball->position.x = -ballBound;
        if (ball->velocity.x < 0.0) ball->velocity.x = 0.0;
      }
      if (ball->position.z > ballBound) {
        ball->position.z = ballBound;
        if (ball->velocity.z > 0.0) ball->velocity.z = 0.0;
      } else if (ball->position.z < -ballBound) {
        ball->position.z = -ballBound;
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
        ball->velocity = direction * (kWorldKickBase + world_.player.speed * kWorldKickSpeedScale) +
                         Vec3{0.0, kWorldKickUp, 0.0};
      }
    }

    // Dynamic crates: keep them on the floor area; the player can shove
    // them by walking into them and kick them like the ball. Crates collide
    // with the ball through the physics world, so they can push it around.
    const f64 crateHalf = kWorldCrateSize * 0.5;
    const f64 crateBound = kWorldFloorHalf - crateHalf;
    for (const u32 crateId : crateBodyIds_) {
      DynamicBox* crate = physics_.dynamicBox(crateId);
      if (crate == nullptr) continue;
      if (crate->position.x > crateBound) {
        crate->position.x = crateBound;
        if (crate->velocity.x > 0.0) crate->velocity.x = 0.0;
      } else if (crate->position.x < -crateBound) {
        crate->position.x = -crateBound;
        if (crate->velocity.x < 0.0) crate->velocity.x = 0.0;
      }
      if (crate->position.z > crateBound) {
        crate->position.z = crateBound;
        if (crate->velocity.z > 0.0) crate->velocity.z = 0.0;
      } else if (crate->position.z < -crateBound) {
        crate->position.z = -crateBound;
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
        const f64 kickSpeed = kWorldKickBase + world_.player.speed * kWorldKickSpeedScale;
        crate->velocity = direction * (kickSpeed * kWorldCrateKickScale) + Vec3{0.0, kWorldCrateKickUp, 0.0};
      }
    }

    const Vec3 position = ballPosition();
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
        if (ball != nullptr) ball->velocity = Vec3{0.0, 0.0, 0.0};
        break;
      }
    }
    return;
  }
  if (screen == 16) {  // Goal celebration
    physics_.advance(hostSeconds);
    goalTimer_ -= hostSeconds;
    if (goalTimer_ <= 0.0) {
      resetBallToCenter();
      screen_ = Screen::Play;
    }
    return;
  }
  // All menu screens: the ball waits at its spawn.
  resetBallToCenter();
}

void WorldEditor::resetBall() {
  resetBallToCenter();
  goalTimer_ = 0.0;
  if (screen_ == Screen::Goal) screen_ = Screen::Play;
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

std::string WorldEditor::statsLine() const {
  std::ostringstream line;
  line << "KIMIA WORLD | " << screenName(static_cast<int>(screen_)) << " | world " << world_.name
       << " | player " << playerSpeedName(world_.player.speed) << " | ball " << ballTypeName(world_.ball.type)
       << " | env " << environmentName(world_.environment) << " | score " << world_.score
       << " | objects " << objectCount();
  if (!lastError_.empty()) line << " | note " << lastError_;
  return line.str();
}

}  // namespace kimia
