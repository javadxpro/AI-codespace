#include <kimia/AssetPipeline.h>
#include <kimia/Skeleton.h>
#include <kimia/World.h>
#include <fstream>
#include <filesystem>
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
    // A body COMPONENT wins over the name (stage 31). Until now what an
    // object did was decided entirely by what it was called: "Crate_*" was
    // pushable, "Block_*" was solid, and an imported model was scenery you
    // could walk through. A component lets any entity — including one you
    // imported yourself — be solid, pushable or a rolling ball.
    if (entity.body.has_value()) {
      const BodyComponent& body = *entity.body;
      const Vec3 half = entity.transform.scale * 0.5;
      switch (body.kind) {
        case BodyKind::Static:
          physics_.addBox(entity.transform.position, half);
          return;
        case BodyKind::Dynamic: {
          DynamicBox box;
          box.position = entity.transform.position;
          box.halfExtents = half;
          box.mass = body.mass;
          box.restitution = body.restitution;
          box.friction = body.friction;
          box.rollingFriction = body.friction;
          const u32 id = physics_.addDynamicBox(box);
          crateIds_[entity.name] = id;
          crateBodyIds_.push_back(id);
          return;
        }
        case BodyKind::Sphere: {
          SphereBody sphere;
          sphere.position = entity.transform.position;
          // A radius of 0 means "work it out from the transform", so the
          // editor does not have to keep two numbers in step.
          sphere.radius = body.radius > 0.0 ? body.radius : std::max(half.x, std::max(half.y, half.z));
          sphere.mass = body.mass;
          sphere.restitution = body.restitution;
          sphere.friction = body.friction;
          sphere.rollingFriction = body.friction;
          physics_.addSphere(sphere);
          return;
        }
        case BodyKind::None:
          return;  // explicitly decoration: drawn, never collided with
      }
    }
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
  // An arena needs COVER. Without it every fighter can see every other
  // fighter from anywhere and the match is decided in seconds — the first
  // build produced 183 kills a minute in an empty box. These blocks are
  // real static bodies, so they stop bullets as well as bodies.
  if (world_.profile.arena) {
    const f64 spanX = world_.halfWidth() * 0.55;
    const f64 spanZ = world_.halfLength() * 0.55;
    const Vec3 spots[] = {
        {0.0, 0.0, 0.0},        {spanX, 0.0, spanZ},   {-spanX, 0.0, spanZ},
        {spanX, 0.0, -spanZ},   {-spanX, 0.0, -spanZ}, {0.0, 0.0, spanZ * 1.4},
        {0.0, 0.0, -spanZ * 1.4}, {spanX * 1.3, 0.0, 0.0}, {-spanX * 1.3, 0.0, 0.0},
    };
    i32 index = 1;
    for (const Vec3& spot : spots) {
      EntityData block;
      block.name = "Block_" + std::to_string(index++);
      block.transform.position = Vec3{spot.x, kWorldBlockMedium * 0.5, spot.z};
      block.transform.scale = Vec3{kWorldBlockMedium * 1.6, kWorldBlockMedium, kWorldBlockMedium * 1.6};
      block.mesh = MeshKind::cube;
      block.color = Vec3{0.55, 0.48, 0.38};
      world_.scene.create(block);
    }
  }
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

f64 WorldEditor::squadSpeed(u32 id) const {
  const CharacterBody* body = physics_.characterById(id);
  if (body == nullptr) return 0.0;
  return std::sqrt(body->velocity.x * body->velocity.x + body->velocity.z * body->velocity.z);
}

f64 WorldEditor::squadFacing(u32 id) const {
  const CharacterBody* body = physics_.characterById(id);
  if (body == nullptr) return 0.0;
  // Face the way you are running. Standing still, the human keeps its aim
  // and everyone else faces up the pitch, so nobody stares at their feet.
  const f64 speed = std::sqrt(body->velocity.x * body->velocity.x + body->velocity.z * body->velocity.z);
  if (speed > 0.15) return std::atan2(body->velocity.x, -body->velocity.z);
  if (id == kPrimaryCharacter) return aimYaw_;
  return body->team == 1U ? 3.14159265358979323846 : 0.0;
}

bool WorldEditor::squadAirborne(u32 id) const {
  const CharacterBody* body = physics_.characterById(id);
  return body != nullptr && !body->onGround;
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
  arenaKills1_ = 0U;
  arenaKills2_ = 0U;
  fireHeld_ = false;
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
  // Everyone starts the round whole and loaded (no-op outside arena mode).
  arenaReset();
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

    // The walk cycle only runs while the game does.
    figureClock_ += hostSeconds;
    particles_.step(hostSeconds);

    // The rules that make this world a game.
    runLogic(hostSeconds);

    // Triggered clips age out (stage 31).
    updateTriggers(hostSeconds);

    // Arena mode (stage 30): weapons, reloads and respawns. The football
    // rules below still run — the pitch, the walls and the characters are
    // shared — but nothing here depends on the ball.
    updateArena(hostSeconds);

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
  // The linesman only watches balls the player actually plays.
  humanPassedBall_ = true;
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

// --- Publishing ---

bool WorldEditor::startPublished(const std::string& path, std::string& error) {
  if (!loadWorld(path, error)) return false;
  playOnly_ = true;
  enterPlay();
  return true;
}

std::string WorldEditor::publish(const std::string& folder, std::string& error) {
  if (!hasWorld_) {
    error = "there is no world to publish";
    return std::string();
  }
  const std::string out = folder.empty() ? std::string("published") : folder;
  // The engine only writes files it was asked to write, and only inside
  // the folder it was given.
  std::error_code failed;
  std::filesystem::create_directories(out, failed);
  if (failed) {
    error = "cannot make the folder '" + out + "'";
    return std::string();
  }

  // The world carries EVERYTHING — scene, rules, panels, blueprints of
  // the objects in it — so a published game is one file plus a runner.
  const std::string worldFile = out + "/game.kimia";
  std::string saveError;
  if (!saveWorld(worldFile, saveError)) {
    error = saveError.empty() ? std::string("could not write the world") : saveError;
    return std::string();
  }

  // A player should not have to know any of the engine's options.
  std::ofstream runner(out + "/play.sh", std::ios::binary);
  if (!runner) {
    error = "cannot write the start script";
    return std::string();
  }
  runner << "#!/usr/bin/env bash\n";
  runner << "# " << world_.name << " — made with KIMIA\n";
  runner << "# Start the game, then open http://127.0.0.1:8080 in a browser.\n";
  runner << "cd \"$(dirname \"$0\")\"\n";
  runner << "exec ./kimia_world --play game.kimia --port \"${1:-8080}\"\n";
  runner.close();

  std::ofstream readme(out + "/README.txt", std::ios::binary);
  if (readme) {
    readme << world_.name << "\n\n";
    readme << "To play:\n";
    readme << "  1. copy the kimia_world program into this folder\n";
    readme << "  2. bash play.sh\n";
    readme << "  3. open http://127.0.0.1:8080\n\n";
    readme << "There is no editor in a published game: it opens straight into play.\n";
  }
  return out;
}

// --- Particles ---

bool WorldEditor::setEmitter(const Emitter& emitter) {
  if (emitter.name.empty()) return false;
  world_.emitters.set(emitter);
  return true;
}

bool WorldEditor::removeEmitter(const std::string& name) { return world_.emitters.remove(name); }

bool WorldEditor::playEffect(const std::string& name, const Vec3& at) {
  const Emitter* emitter = world_.emitters.find(name);
  if (emitter == nullptr) return false;
  // A rising seed makes each burst scatter differently, while a single
  // burst stays repeatable for a given seed.
  particles_.burst(*emitter, at, effectSeed_++);
  return true;
}

// --- The game's own interface ---

bool WorldEditor::setPanel(const Panel& panel) {
  if (panel.name.empty()) return false;
  world_.hud.set(panel);
  return true;
}

bool WorldEditor::removePanel(const std::string& name) { return world_.hud.remove(name); }

std::string WorldEditor::pressHudAt(i32 imageWidth, i32 imageHeight, f64 pixelX, f64 pixelY) {
  const std::string name = buttonAt(world_.hud, imageWidth, imageHeight, pixelX, pixelY);
  if (name.empty()) return name;
  const Panel* panel = world_.hud.find(name);
  // A button with no event still counts as pressed — it swallows the tap
  // rather than letting it fall through and select whatever is behind it.
  if (panel != nullptr && !panel->event.empty()) hudEvents_.push_back(panel->event);
  return name;
}

std::vector<std::string> WorldEditor::drainHudEvents() {
  std::vector<std::string> drained;
  drained.swap(hudEvents_);
  return drained;
}

// --- Blueprints and stages ---

bool WorldEditor::keepBlueprint(const std::string& entityName, const std::string& blueprintName) {
  const EntityData* source = entity(entityName);
  if (source == nullptr || blueprintName.empty()) return false;
  library_.keep(blueprintName, *source);
  return true;
}

bool WorldEditor::forgetBlueprint(const std::string& blueprintName) {
  return library_.forget(blueprintName);
}

std::string WorldEditor::stampBlueprint(const std::string& blueprintName, const Vec3& at) {
  const std::string name = library_.stamp(blueprintName, world_.scene, at);
  if (name.empty()) return name;
  rebuildPhysics();  // a stamped object is solid straight away
  refreshManaged();
  return name;
}

std::vector<std::string> WorldEditor::blueprintNames() const {
  std::vector<std::string> names;
  names.reserve(library_.blueprints.size());
  for (const Blueprint& blueprint : library_.blueprints) names.push_back(blueprint.name);
  return names;
}

std::vector<std::string> WorldEditor::stageNames() const {
  // The stage being edited lives in world_.scene, not in the library, so
  // it is listed here rather than being absent from its own project.
  std::vector<std::string> names;
  names.push_back(currentStage_);
  for (const Stage& stage : library_.stages) {
    if (stage.name != currentStage_) names.push_back(stage.name);
  }
  return names;
}

bool WorldEditor::addStage(const std::string& name) {
  if (name.empty() || name == currentStage_) return false;
  if (library_.findStage(name) != nullptr) return false;
  Stage stage;
  stage.name = name;
  // A new stage starts as an empty room with a floor, the same as a new
  // world does — an editor should never open on nothing at all.
  WorldData scratch;
  scratch.profile = world_.profile;
  buildEmptyWorldScene(scratch);
  stage.scene = std::move(scratch.scene);
  library_.stages.push_back(std::move(stage));
  return true;
}

bool WorldEditor::goToStage(const std::string& name) {
  if (name == currentStage_) return true;
  Stage* target = library_.findStage(name);
  if (target == nullptr) return false;

  // Stash the scene being edited before bringing the other one in, or
  // switching away would throw the work away.
  Stage* here = library_.findStage(currentStage_);
  if (here == nullptr) {
    Stage saved;
    saved.name = currentStage_;
    saved.scene = world_.scene.clone();
    library_.stages.push_back(std::move(saved));
    target = library_.findStage(name);  // the vector may have moved
    if (target == nullptr) return false;
  } else {
    here->scene = world_.scene.clone();
  }

  world_.scene = target->scene.clone();
  currentStage_ = name;
  selected_.clear();
  rebuildPhysics();
  refreshManaged();
  return true;
}

bool WorldEditor::removeStage(const std::string& name) {
  // The stage you are standing on cannot be deleted: there would be
  // nothing to show.
  if (name == currentStage_) return false;
  for (usize i = 0; i < library_.stages.size(); ++i) {
    if (library_.stages[i].name != name) continue;
    library_.stages.erase(library_.stages.begin() + static_cast<std::ptrdiff_t>(i));
    return true;
  }
  return false;
}

// --- The live viewport ---

std::string WorldEditor::pickEntityAt(f64 pixelX, f64 pixelY) const {
  const pick::Hit hit = pick::pickAt(viewport_, pick::targetsFromScene(world_.scene), pixelX, pixelY);
  return hit.hit ? hit.name : std::string();
}

bool WorldEditor::dragEntity(const std::string& name, f64 fromX, f64 fromY, f64 toX, f64 toY, f64 grid) {
  EntityData* target = world_.scene.get(world_.scene.find(name));
  if (target == nullptr) return false;
  Vec3 delta;
  // Drag along the plane at the OBJECT'S OWN height, so something on a
  // table slides across the table rather than jumping to the floor.
  if (!pick::dragDelta(viewport_, fromX, fromY, toX, toY, target->transform.position.y, delta)) {
    return false;
  }
  target->transform.position = pick::snapTo(target->transform.position + delta, grid);
  rebuildPhysics();  // an editor shows the result now, not after a reload
  return true;
}

// --- Visual logic: the rules that make a scene into a game ---

usize WorldEditor::addRule(const Rule& rule) {
  world_.logic.rules.push_back(rule);
  return world_.logic.rules.size() - 1U;
}

bool WorldEditor::replaceRule(usize index, const Rule& rule) {
  if (index >= world_.logic.rules.size()) return false;
  world_.logic.rules[index] = rule;
  return true;
}

bool WorldEditor::removeRule(usize index) {
  if (index >= world_.logic.rules.size()) return false;
  world_.logic.rules.erase(world_.logic.rules.begin() + static_cast<std::ptrdiff_t>(index));
  return true;
}

bool WorldEditor::enableRule(usize index, bool enabled) {
  if (index >= world_.logic.rules.size()) return false;
  world_.logic.rules[index].enabled = enabled;
  return true;
}

bool WorldEditor::moveRule(usize index, bool up) {
  if (index >= world_.logic.rules.size()) return false;
  if (up && index == 0U) return false;
  if (!up && index + 1U >= world_.logic.rules.size()) return false;
  const usize other = up ? index - 1U : index + 1U;
  const Rule swapped = world_.logic.rules[index];
  world_.logic.rules[index] = world_.logic.rules[other];
  world_.logic.rules[other] = swapped;
  return true;
}

void WorldEditor::setVariable(const std::string& name, f64 value) { world_.logic.setNumber(name, value); }

void WorldEditor::setVariableText(const std::string& name, const std::string& text) {
  world_.logic.setText(name, text);
}

bool WorldEditor::removeVariable(const std::string& name) {
  for (usize i = 0; i < world_.logic.variables.size(); ++i) {
    if (world_.logic.variables[i].name != name) continue;
    world_.logic.variables.erase(world_.logic.variables.begin() + static_cast<std::ptrdiff_t>(i));
    return true;
  }
  return false;
}

void WorldEditor::setLogicKeys(const std::vector<std::string>& pressed,
                               const std::vector<std::string>& held) {
  logicKeysPressed_ = pressed;
  logicKeysHeld_ = held;
}

void WorldEditor::runLogic(f64 seconds) {
  if (world_.logic.rules.empty()) return;

  LogicInput input;
  input.seconds = seconds;
  input.keysPressed = logicKeysPressed_;
  input.keysHeld = logicKeysHeld_;

  // Built-in game events are logic events too, so a rule can listen for
  // "goal" without the engine knowing that rule exists.
  for (const GameEvent event : events_) input.events.push_back(eventTriggerName(event));
  // So are HUD button presses: tapping a button the user drew is exactly
  // as good a trigger as pressing a key.
  for (const std::string& pressed : hudEvents_) input.events.push_back(pressed);
  hudEvents_.clear();

  // Which characters are standing inside which entity's area. The radius
  // is the rule's own `number`, so "near the goal" is the user's call.
  for (const Rule& rule : world_.logic.rules) {
    if (rule.trigger != Trigger::AreaEnter && rule.trigger != Trigger::AreaExit) continue;
    const EntityData* area = entity(rule.other);
    if (area == nullptr) continue;
    const f64 radius = rule.number > 0.0 ? rule.number : 1.0;
    const Vec3 centre = area->transform.position;
    const EntityData* who = entity(rule.subject);
    // "Player" means the character the person is driving, not a scene
    // entity that happens to share the name.
    const Vec3 at = rule.subject == "Player" ? playerPos_
                                             : (who != nullptr ? who->transform.position : Vec3{1e9, 1e9, 1e9});
    const f64 dx = at.x - centre.x;
    const f64 dz = at.z - centre.z;
    if (std::sqrt(dx * dx + dz * dz) <= radius) {
      input.areaPairs.push_back(rule.subject + "|" + rule.other);
    }
  }

  std::vector<Effect> effects;
  logicRuntime_.step(world_.logic, input, effects);
  // "Pressed" lasts one frame. Held keys persist until the app says
  // otherwise, so they are left alone.
  logicKeysPressed_.clear();

  // Carry out what the rules decided. The runtime never touches the world
  // itself, so everything the engine does is in one place.
  for (const Effect& effect : effects) {
    switch (effect.act) {
      case Act::Move: {
        EntityData* target = world_.scene.get(world_.scene.find(effect.target));
        if (target != nullptr) {
          // A nudge per second, so a rule reads in units a person expects.
          target->transform.position += effect.amount * seconds;
        } else if (effect.target == "Player") {
          setPlayerPosition(playerPos_ + effect.amount * seconds);
        }
        break;
      }
      case Act::MoveTo: {
        EntityData* target = world_.scene.get(world_.scene.find(effect.target));
        if (target != nullptr) {
          target->transform.position = effect.amount;
        } else if (effect.target == "Player") {
          setPlayerPosition(effect.amount);
        }
        break;
      }
      case Act::Rotate: {
        EntityData* target = world_.scene.get(world_.scene.find(effect.target));
        if (target != nullptr) {
          const f64 radians = effect.number * seconds * 3.14159265358979323846 / 180.0;
          target->transform.rotation =
              target->transform.rotation * Quat::fromAxisAngle(Vec3{0.0, 1.0, 0.0}, radians);
        }
        break;
      }
      case Act::Spawn: {
        const EntityData* source = entity(effect.target);
        if (source != nullptr) {
          EntityData copy = *source;
          // A unique name, so spawning twice gives two things.
          u32 index = 1U;
          std::string name = source->name + "_" + std::to_string(index);
          while (world_.scene.find(name) != kNullEntity) {
            ++index;
            name = source->name + "_" + std::to_string(index);
          }
          copy.name = name;
          copy.transform.position = effect.amount;
          world_.scene.create(copy);
          rebuildPhysics();
        }
        break;
      }
      case Act::Destroy:
        deleteEntity(effect.target);
        break;
      case Act::PlaySound:
        triggeredSounds_.push_back(effect.text);
        break;
      case Act::PlayAnimation:
        fireTrigger(effect.text);
        break;
      case Act::ShowMessage:
        logicMessage_ = effect.text;
        break;
      case Act::Effect_: {
        // At the named object if there is one, otherwise at the spot the
        // rule gave — so "explode at the barrel" and "explode here" both
        // read naturally.
        const EntityData* where = entity(effect.target);
        playEffect(effect.text, where != nullptr ? where->transform.position : effect.amount);
        break;
      }
      case Act::GoToScene:
        // A rule can send the player from a menu to a level, which is
        // what makes several stages worth having.
        goToStage(effect.text);
        break;
      default:
        break;  // variables and events are the runtime's own business
    }
  }
}

// --- Components, tags and triggers (stage 31) ---
//
// This is the layer the editor talks to. Everything is addressed by entity
// NAME so a user interface can stay stringly-typed and never needs to know
// about handles, physics ids or the scene's internals.

std::vector<std::string> WorldEditor::entityNames() const {
  std::vector<std::string> names;
  world_.scene.forEach([&names](EntityHandle, const EntityData& entity) { names.push_back(entity.name); });
  return names;
}

std::vector<std::string> WorldEditor::entitiesWithTag(const std::string& tag) const {
  std::vector<std::string> names;
  if (tag.empty()) return names;
  world_.scene.forEach([&names, &tag](EntityHandle, const EntityData& entity) {
    if (entity.hasTag(tag)) names.push_back(entity.name);
  });
  return names;
}

std::vector<std::string> WorldEditor::allTags() const {
  std::vector<std::string> tags;
  world_.scene.forEach([&tags](EntityHandle, const EntityData& entity) {
    for (const std::string& tag : entity.tags) {
      bool seen = false;
      for (const std::string& known : tags) {
        if (known == tag) seen = true;
      }
      if (!seen) tags.push_back(tag);
    }
  });
  std::sort(tags.begin(), tags.end());
  return tags;
}

const EntityData* WorldEditor::entity(const std::string& name) const {
  return world_.scene.get(world_.scene.find(name));
}

bool WorldEditor::setEntityTransform(const std::string& name, const Vec3& position, const Vec3& scale) {
  EntityData* target = world_.scene.get(world_.scene.find(name));
  if (target == nullptr) return false;
  target->transform.position = position;
  target->transform.scale = scale;
  rebuildPhysics();  // an editor shows you the result, it does not ask you to restart
  return true;
}

bool WorldEditor::setEntityColor(const std::string& name, const Vec3& color) {
  EntityData* target = world_.scene.get(world_.scene.find(name));
  if (target == nullptr) return false;
  target->color = color;
  return true;
}

bool WorldEditor::addEntityTag(const std::string& name, const std::string& tag) {
  EntityData* target = world_.scene.get(world_.scene.find(name));
  if (target == nullptr || tag.empty()) return false;
  target->addTag(tag);
  return true;
}

bool WorldEditor::removeEntityTag(const std::string& name, const std::string& tag) {
  EntityData* target = world_.scene.get(world_.scene.find(name));
  if (target == nullptr) return false;
  return target->removeTag(tag);
}

bool WorldEditor::setEntityBody(const std::string& name, const BodyComponent& body) {
  EntityData* target = world_.scene.get(world_.scene.find(name));
  if (target == nullptr) return false;
  target->body = body;
  rebuildPhysics();  // solid immediately, not after a reload
  return true;
}

bool WorldEditor::clearEntityBody(const std::string& name) {
  EntityData* target = world_.scene.get(world_.scene.find(name));
  if (target == nullptr || !target->body.has_value()) return false;
  target->body.reset();
  rebuildPhysics();
  return true;
}

bool WorldEditor::addEntityAnimation(const std::string& name, const AnimationComponent& clip) {
  EntityData* target = world_.scene.get(world_.scene.find(name));
  if (target == nullptr || clip.clip.empty()) return false;
  target->animations.push_back(clip);
  return true;
}

bool WorldEditor::addEntitySound(const std::string& name, const SoundComponent& sound) {
  EntityData* target = world_.scene.get(world_.scene.find(name));
  if (target == nullptr || sound.sound.empty()) return false;
  target->sounds.push_back(sound);
  return true;
}

bool WorldEditor::setEntityBone(const std::string& name, const RigBone& bone) {
  EntityData* target = world_.scene.get(world_.scene.find(name));
  if (target == nullptr || bone.name.empty()) return false;
  for (RigBone& existing : target->rig) {
    if (existing.name != bone.name) continue;
    existing = bone;  // moving a bone is a replace, not a second bone
    return true;
  }
  target->rig.push_back(bone);
  return true;
}

bool WorldEditor::removeEntityBone(const std::string& name, const std::string& bone) {
  EntityData* target = world_.scene.get(world_.scene.find(name));
  if (target == nullptr) return false;
  for (usize i = 0; i < target->rig.size(); ++i) {
    if (target->rig[i].name != bone) continue;
    // Anything parented to it becomes a root rather than vanishing.
    for (RigBone& child : target->rig) {
      if (child.parent == bone) child.parent.clear();
    }
    target->rig.erase(target->rig.begin() + static_cast<std::ptrdiff_t>(i));
    return true;
  }
  return false;
}

bool WorldEditor::clearEntityRig(const std::string& name) {
  EntityData* target = world_.scene.get(world_.scene.find(name));
  if (target == nullptr) return false;
  target->rig.clear();
  return true;
}

bool WorldEditor::fitDefaultRig(const std::string& name, f64 height) {
  EntityData* target = world_.scene.get(world_.scene.find(name));
  if (target == nullptr) return false;
  if (height <= 0.0) height = 1.7;
  // Build the engine's figure, then write it out as editable bones. The
  // point is to give the user something real to drag, not to hide it.
  const Skeleton figure = makeFigureRig(height);
  std::vector<Transform3D> pose;
  poseFigure(figure, FigureMotion{}, pose);
  std::vector<FigureLimb> limbs;
  figureLimbs(figure, pose, Vec3{0.0, 0.0, 0.0}, 0.0, limbs);

  // Same order as figureLimbs builds them.
  static const char* kNames[] = {"Torso", "Neck",  "LeftArm",  "LeftHand", "RightArm", "RightHand",
                                 "LeftLeg", "LeftFoot", "RightLeg", "RightFoot", "Head"};
  static const char* kParents[] = {"",        "Torso",   "Torso",    "LeftArm", "Torso", "RightArm",
                                   "",        "LeftLeg", "",         "RightLeg", "Neck"};
  // Arms swing against the legs, the body does not swing at all.
  static const f64 kSwing[] = {0.0, 0.0, -0.8, -0.8, 0.8, 0.8, 1.0, 0.9, -1.0, -0.9, 0.0};

  target->rig.clear();
  for (usize i = 0; i < limbs.size() && i < sizeof(kNames) / sizeof(kNames[0]); ++i) {
    RigBone bone;
    bone.name = kNames[i];
    bone.parent = kParents[i];
    bone.from = limbs[i].from;
    bone.to = limbs[i].to;
    bone.thickness = limbs[i].thickness;
    bone.swing = kSwing[i];
    target->rig.push_back(bone);
  }
  return true;
}

bool WorldEditor::clearEntityAnimations(const std::string& name) {
  EntityData* target = world_.scene.get(world_.scene.find(name));
  if (target == nullptr) return false;
  target->animations.clear();
  return true;
}

bool WorldEditor::clearEntitySounds(const std::string& name) {
  EntityData* target = world_.scene.get(world_.scene.find(name));
  if (target == nullptr) return false;
  target->sounds.clear();
  return true;
}

std::string WorldEditor::importModel(const std::string& file, f64 size, std::string& error) {
  if (!hasWorld_) {
    error = "no world open";
    return std::string();
  }
  if (file.empty()) {
    error = "no file given";
    return std::string();
  }
  std::string loadError;
  auto loaded = assets::loadMesh(file, loadError);
  if (!loaded.has_value()) {
    error = loadError;
    return std::string();
  }

  EntityData model;
  // A unique name, so importing the same file twice gives two objects.
  u32 index = 1U;
  std::string name = "Model_1";
  while (world_.scene.find(name) != kNullEntity) {
    ++index;
    name = "Model_" + std::to_string(index);
  }
  model.name = name;
  model.meshFile = file;
  model.mesh = MeshKind::cube;  // fallback shape while the mesh loads
  model.transform.position = Vec3{0.0, 0.0, 0.0};

  // Fit the model's largest dimension to the requested size, so any file
  // becomes a prop of a predictable size whatever units it was authored in.
  if (!loaded->mesh.positions.empty()) {
    Vec3 lo = loaded->mesh.positions[0];
    Vec3 hi = loaded->mesh.positions[0];
    for (const Vec3& point : loaded->mesh.positions) {
      lo.x = std::min(lo.x, point.x);
      lo.y = std::min(lo.y, point.y);
      lo.z = std::min(lo.z, point.z);
      hi.x = std::max(hi.x, point.x);
      hi.y = std::max(hi.y, point.y);
      hi.z = std::max(hi.z, point.z);
    }
    const f64 largest = std::max(hi.x - lo.x, std::max(hi.y - lo.y, hi.z - lo.z));
    if (largest > 1e-6 && size > 0.0) {
      const f64 fit = size / largest;
      model.transform.scale = Vec3{fit, fit, fit};
    }
  }
  world_.scene.create(model);
  rebuildPhysics();
  return name;
}

bool WorldEditor::deleteEntity(const std::string& name) {
  const EntityHandle handle = world_.scene.find(name);
  if (handle == kNullEntity) return false;
  world_.scene.destroy(handle);
  rebuildPhysics();
  refreshManaged();
  return true;
}

u32 WorldEditor::fireTrigger(const std::string& trigger) {
  if (trigger.empty()) return 0U;
  u32 fired = 0U;
  world_.scene.forEach([this, &trigger, &fired](EntityHandle, const EntityData& entity) {
    for (const AnimationComponent& clip : entity.animations) {
      if (clip.trigger != trigger) continue;
      // Restart rather than stack: pressing a button twice should replay
      // the move, not run two copies of it on the same model.
      bool replaced = false;
      for (PlayingClip& playing : playingClips_) {
        if (playing.entity != entity.name || playing.clip != clip.clip) continue;
        playing.timeLeft = kTriggerClipSeconds / std::max(clip.speed, 0.01);
        playing.loop = clip.loop;
        replaced = true;
        break;
      }
      if (!replaced) {
        PlayingClip playing;
        playing.entity = entity.name;
        playing.clip = clip.clip;
        playing.timeLeft = kTriggerClipSeconds / std::max(clip.speed, 0.01);
        playing.loop = clip.loop;
        playingClips_.push_back(playing);
      }
      ++fired;
    }
    for (const SoundComponent& sound : entity.sounds) {
      if (sound.trigger != trigger) continue;
      triggeredSounds_.push_back(sound.sound);
      ++fired;
    }
  });
  return fired;
}

std::vector<std::string> WorldEditor::drainTriggeredSounds() {
  std::vector<std::string> drained;
  drained.swap(triggeredSounds_);
  return drained;
}

std::vector<std::string> WorldEditor::playingAnimations() const {
  std::vector<std::string> playing;
  playing.reserve(playingClips_.size());
  for (const PlayingClip& clip : playingClips_) {
    playing.push_back(clip.entity + ":" + clip.clip);
  }
  return playing;
}

void WorldEditor::updateTriggers(f64 seconds) {
  if (seconds <= 0.0 || playingClips_.empty()) return;
  for (usize i = playingClips_.size(); i > 0U; --i) {
    PlayingClip& clip = playingClips_[i - 1U];
    clip.timeLeft -= seconds;
    if (clip.timeLeft > 0.0) continue;
    if (clip.loop) {
      clip.timeLeft = kTriggerClipSeconds;  // a looping clip runs until stopped
      continue;
    }
    playingClips_.erase(playingClips_.begin() + static_cast<std::ptrdiff_t>(i - 1U));
  }
}

// --- Arena mode (stage 30) ---
//
// The same engine, the same characters, the same physics — but the ball is
// replaced by a rifle. A shot is a RAYCAST, not a projectile: at rifle
// speed a bullet crosses this arena in a few milliseconds, so simulating
// its flight would be an expensive way to draw a straight line.

u32 WorldEditor::health(u32 id) const {
  if (!arenaMode()) return 0U;
  const auto found = arenaHealth_.find(id);
  return found == arenaHealth_.end() ? 0U : found->second;
}

u32 WorldEditor::ammo(u32 id) const {
  if (!arenaMode()) return 0U;
  const auto found = arenaAmmo_.find(id);
  return found == arenaAmmo_.end() ? 0U : found->second;
}

bool WorldEditor::reloading(u32 id) const {
  if (!arenaMode()) return false;
  const auto found = arenaReload_.find(id);
  return found != arenaReload_.end() && found->second > 0.0;
}

u32 WorldEditor::arenaScore(u32 team) const {
  if (team == 1U) return arenaKills1_;
  if (team == 2U) return arenaKills2_;
  return 0U;
}

void WorldEditor::arenaReset() {
  arenaHealth_.clear();
  arenaAmmo_.clear();
  arenaReload_.clear();
  arenaCooldown_.clear();
  arenaRespawn_.clear();
  if (!arenaMode()) return;
  for (const u32 id : physics_.characterIds()) {
    arenaHealth_[id] = world_.profile.health;
    arenaAmmo_[id] = world_.profile.magazine;
  }
}

std::string WorldEditor::arenaHudText() const {
  if (!arenaMode()) return std::string();
  const u32 hp = health(kPrimaryCharacter);
  if (hp == 0U) return "DOWN";
  if (reloading(kPrimaryCharacter)) return "HP " + std::to_string(hp) + "  RELOADING";
  return "HP " + std::to_string(hp) + "  AMMO " + std::to_string(ammo(kPrimaryCharacter)) + "/" +
         std::to_string(world_.profile.magazine);
}

// One fighter pulls the trigger along `aim`. Everything a shot needs to be
// fair is checked here, so the human and the computer use the identical
// path — no special cases that quietly favour one of them.
bool WorldEditor::arenaShoot(u32 id, const Vec3& aim) {
  if (!arenaMode() || !playing() || roundOver()) return false;
  if (health(id) == 0U) return false;      // downed fighters do not shoot
  if (reloading(id)) return false;         // nor mid-reload
  if (arenaAmmo_[id] == 0U) return false;  // nor with an empty magazine
  const auto cooldown = arenaCooldown_.find(id);
  if (cooldown != arenaCooldown_.end() && cooldown->second > 0.0) return false;  // rate of fire

  const CharacterBody* body = physics_.characterById(id);
  if (body == nullptr) return false;
  const f64 length = std::sqrt(aim.x * aim.x + aim.y * aim.y + aim.z * aim.z);
  if (length < kMoveEpsilon) return false;
  const Vec3 direction{aim.x / length, aim.y / length, aim.z / length};
  const Vec3 muzzle{body->position.x, body->position.y + kArenaMuzzleHeight, body->position.z};

  --arenaAmmo_[id];
  arenaCooldown_[id] = 1.0 / world_.profile.fireRate;

  const PhysicsWorld::RayHit shot = physics_.raycast(muzzle, direction, world_.profile.range, id);
  lastShotFrom_ = muzzle;
  lastShotTo_ = shot.hit ? shot.point : muzzle + direction * world_.profile.range;
  lastShotHit_ = false;

  if (shot.hit && shot.character != 0U) {
    const CharacterBody* target = physics_.characterById(shot.character);
    // Friendly fire does no damage: teams would shred each other in the
    // scramble and the match would be decided by accidents.
    if (target != nullptr && target->team != body->team) {
      u32& hp = arenaHealth_[shot.character];
      hp = hp > world_.profile.damage ? hp - world_.profile.damage : 0U;
      lastShotHit_ = true;
      if (hp == 0U) {
        arenaRespawn_[shot.character] = kArenaRespawnTime;
        if (body->team == 1U) {
          ++arenaKills1_;
        } else {
          ++arenaKills2_;
        }
        events_.push_back(GameEvent::Goal);  // a downed opponent is the score here
      } else {
        events_.push_back(GameEvent::Tackle);  // the hit marker
      }
    }
  }
  events_.push_back(GameEvent::Shot);
  return true;
}

bool WorldEditor::fire() { return arenaShoot(kPrimaryCharacter, aimDirection()); }

bool WorldEditor::reload() {
  if (!arenaMode() || health(kPrimaryCharacter) == 0U) return false;
  if (reloading(kPrimaryCharacter)) return false;
  if (arenaAmmo_[kPrimaryCharacter] >= world_.profile.magazine) return false;  // already full
  arenaReload_[kPrimaryCharacter] = world_.profile.reloadTime;
  return true;
}

void WorldEditor::updateArena(f64 seconds) {
  if (!arenaMode() || seconds <= 0.0) return;
  // Late joiners (a character spawned after the reset) start whole.
  for (const u32 id : physics_.characterIds()) {
    if (arenaHealth_.find(id) == arenaHealth_.end()) {
      arenaHealth_[id] = world_.profile.health;
      arenaAmmo_[id] = world_.profile.magazine;
    }
  }

  for (const u32 id : physics_.characterIds()) {
    // Rate of fire.
    f64& cooldown = arenaCooldown_[id];
    if (cooldown > 0.0) cooldown = cooldown > seconds ? cooldown - seconds : 0.0;

    // Reloads.
    f64& reloadLeft = arenaReload_[id];
    if (reloadLeft > 0.0) {
      reloadLeft -= seconds;
      if (reloadLeft <= 0.0) {
        reloadLeft = 0.0;
        arenaAmmo_[id] = world_.profile.magazine;
      }
    }

    // Respawns: a downed fighter comes back at their own end, whole again.
    if (arenaHealth_[id] == 0U) {
      f64& respawn = arenaRespawn_[id];
      respawn -= seconds;
      if (respawn <= 0.0) {
        respawn = 0.0;
        arenaHealth_[id] = world_.profile.health;
        arenaAmmo_[id] = world_.profile.magazine;
        CharacterBody* body = physics_.characterById(id);
        if (body != nullptr) {
          const f64 ownEnd = body->team == 1U ? world_.halfLength() - kPlayerMargin
                                              : -(world_.halfLength() - kPlayerMargin);
          body->position.z = ownEnd;
          body->velocity = Vec3{0.0, 0.0, 0.0};
          if (id == kPrimaryCharacter) playerPos_ = body->position;
        }
      }
      continue;  // no shooting while down
    }

    // An empty magazine reloads itself: nobody stands in a firefight
    // holding an empty rifle waiting to be told.
    if (arenaAmmo_[id] == 0U && reloadLeft <= 0.0) {
      arenaReload_[id] = world_.profile.reloadTime;
      continue;
    }

    if (id == kPrimaryCharacter) {
      // The human's held trigger fires at the weapon's rate.
      if (fireHeld_) arenaShoot(id, aimDirection());
      continue;
    }

    // --- Computer fighters ---
    if (!aiActive()) continue;
    const CharacterBody* body = physics_.characterById(id);
    if (body == nullptr) continue;
    // Shoot at the nearest standing opponent that is roughly in front.
    u32 target = 0U;
    f64 bestDistance = 0.0;
    for (const u32 other : physics_.characterIds()) {
      const CharacterBody* enemy = physics_.characterById(other);
      if (enemy == nullptr || enemy->team == body->team) continue;
      if (arenaHealth_[other] == 0U) continue;  // do not shoot the downed
      const f64 dx = enemy->position.x - body->position.x;
      const f64 dz = enemy->position.z - body->position.z;
      const f64 distance = std::sqrt(dx * dx + dz * dz);
      if (distance > world_.profile.range) continue;
      if (target == 0U || distance < bestDistance) {
        target = other;
        bestDistance = distance;
      }
    }
    if (target == 0U) continue;
    const CharacterBody* enemy = physics_.characterById(target);
    if (enemy == nullptr) continue;
    Vec3 aim{enemy->position.x - body->position.x, 0.0, enemy->position.z - body->position.z};
    const f64 aimLength = std::sqrt(aim.x * aim.x + aim.z * aim.z);
    if (aimLength < kMoveEpsilon) continue;
    aim.x /= aimLength;
    aim.z /= aimLength;
    // Skill spoils the aim: a perfect AI would be a dead shot at any range
    // and no human could ever win. The wobble is deterministic — derived
    // from the ids — so a replay of the same match plays out the same way.
    const f64 spread = kArenaAiSpread * (1.0 - world_.profile.aiSkill);
    const f64 wobble = spread * (static_cast<f64>((id * 7U + target * 13U) % 11U) / 5.0 - 1.0);
    const f64 sin = std::sin(wobble);
    const f64 cos = std::cos(wobble);
    const Vec3 spread_aim{aim.x * cos - aim.z * sin, 0.0, aim.x * sin + aim.z * cos};
    arenaShoot(id, spread_aim);
  }
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
  // Only when the HUMAN actually plays the ball. This used to run every
  // frame against whoever happened to be furthest forward, so once the
  // computer players started making runs the flag went up 44 times a
  // minute and the match was stopped 95% of the time.
  if (humanPassedBall_) {
    humanPassedBall_ = false;
    const u32 target = passTarget();
    if (target != 0U && offsideFor(target)) {
      const CharacterBody* mate = physics_.characterById(target);
      if (mate != nullptr) {
        awardRestart(Stoppage::Offside, 2U, Vec3{mate->position.x, 0.0, mate->position.z});
        return;
      }
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

const char* WorldEditor::aiRoleName(AiRole role) {
  switch (role) {
    case AiRole::Keeper: return "KEEPER";
    case AiRole::Attack: return "ATTACK";
    case AiRole::Defend: return "DEFEND";
    case AiRole::Support: return "SUPPORT";
    case AiRole::Idle: break;
  }
  return "IDLE";
}

// A side has the ball when its chaser is on it AND is closer to it than
// anybody from the other side. Possession has to be EXCLUSIVE: when both
// sides thought they had it, both attacked, each dragged the ball the
// opposite way, and the two of them stood there cancelling out forever.
bool WorldEditor::aiHasPossession(u32 team) const {
  const SphereBody* ball = physics_.sphere(ballId_);
  if (ball == nullptr) return false;
  const u32 chaser = aiChaser(team);
  if (chaser == 0U) return false;
  const CharacterBody* body = physics_.characterById(chaser);
  if (body == nullptr) return false;
  const f64 dx = ball->position.x - body->position.x;
  const f64 dz = ball->position.z - body->position.z;
  const f64 mine = std::sqrt(dx * dx + dz * dz);
  if (mine >= world_.ball.radius + kAiTackleReach + kAiApproachOffset) return false;

  // Anybody closer — including the human — takes it off us.
  for (const u32 other : physics_.characterIds()) {
    const CharacterBody* rival = physics_.characterById(other);
    if (rival == nullptr || rival->team == team) continue;
    const f64 rx = ball->position.x - rival->position.x;
    const f64 rz = ball->position.z - rival->position.z;
    if (std::sqrt(rx * rx + rz * rz) < mine) return false;
  }
  return true;
}

// The net this side is attacking. Team 1 shoots at the -Z goal, team 2 at
// the +Z one (matching scoringTeamForGoalZ).
Vec3 WorldEditor::aiGoalMouth(u32 team) const {
  const f64 forward = attackDirectionZ(team);
  // Default: the middle of the far goal line, in case no goal was built.
  Vec3 mouth{0.0, 0.0, forward * world_.halfLength()};
  f64 half = kWorldGoalMedium * 0.5;
  std::map<std::string, GoalGroup> goals;
  scanGoals(world_.scene, goals);
  for (const auto& entry : goals) {
    const GoalGroup& goal = entry.second;
    if (!goal.valid()) continue;
    // The one we are shooting AT is on their side of halfway.
    if (forward > 0.0 ? goal.z() <= 0.0 : goal.z() > 0.0) continue;
    mouth = Vec3{goal.x(), 0.0, goal.z()};
    half = goal.width() * 0.5;
    break;
  }
  // Aim at a POST, not the middle. The keeper stands on the centre of its
  // line, so a side that always shoots down the middle is saved every
  // time — which is exactly why one team could never score. Each side
  // favours a different corner so the two are not mirror images.
  const SphereBody* ball = physics_.sphere(ballId_);
  const f64 side = ball != nullptr && ball->position.x > mouth.x ? 1.0 : -1.0;
  mouth.x += side * half * kAiGoalCorner;
  return mouth;
}

WorldEditor::AiRole WorldEditor::aiRole(u32 id) const {
  if (!aiActive() || id == kPrimaryCharacter) return AiRole::Idle;
  const CharacterBody* body = physics_.characterById(id);
  if (body == nullptr) return AiRole::Idle;
  const u32 team = body->team;
  if (id == aiKeeper(team)) return AiRole::Keeper;
  // The chaser attacks when its side has the ball and defends when it does
  // not: the same player, two different jobs.
  if (id == aiChaser(team)) return aiHasPossession(team) ? AiRole::Attack : AiRole::Defend;
  return AiRole::Support;
}

// Push away from anyone standing too close. This is what stops two players
// occupying the same spot and deadlocking — the bug where a match froze
// with two opponents stacked on the ball.
Vec3 WorldEditor::aiSeparation(u32 id) const {
  const CharacterBody* self = physics_.characterById(id);
  if (self == nullptr) return Vec3{0.0, 0.0, 0.0};
  Vec3 push{0.0, 0.0, 0.0};
  for (const u32 other : physics_.characterIds()) {
    if (other == id) continue;
    const CharacterBody* body = physics_.characterById(other);
    if (body == nullptr) continue;
    const f64 dx = self->position.x - body->position.x;
    const f64 dz = self->position.z - body->position.z;
    const f64 distance = std::sqrt(dx * dx + dz * dz);
    if (distance >= kAiPersonalSpace) continue;
    if (distance < kMoveEpsilon) {
      // Exactly on top of each other: break the tie with the id so the two
      // of them pick opposite directions instead of both waiting.
      const f64 nudge = id % 2U == 0U ? 1.0 : -1.0;
      push.x += nudge;
      continue;
    }
    // The closer they are, the harder the shove.
    const f64 strength = (kAiPersonalSpace - distance) / kAiPersonalSpace;
    push.x += dx / distance * strength;
    push.z += dz / distance * strength;
  }
  return push;
}

Vec3 WorldEditor::aiTargetFor(u32 id) const {
  const CharacterBody* body = physics_.characterById(id);
  if (body == nullptr || id == kPrimaryCharacter) return Vec3{0.0, 0.0, 0.0};

  // --- Arena fighters (stage 30) ---
  // A shooter has no ball to chase and no net to mind, so the football
  // roles are meaningless here: a "keeper" standing on a goal line in a
  // firefight is just an easy target.
  if (arenaMode()) {
    const f64 boundX = world_.halfWidth() - kPlayerMargin;
    const f64 boundZ = world_.halfLength() - kPlayerMargin;
    // Close to the nearest standing enemy, but stop at a fighting distance
    // rather than walking into their muzzle.
    u32 target = 0U;
    f64 bestDistance = 0.0;
    for (const u32 other : physics_.characterIds()) {
      const CharacterBody* enemy = physics_.characterById(other);
      if (enemy == nullptr || enemy->team == body->team) continue;
      const auto hp = arenaHealth_.find(other);
      if (hp != arenaHealth_.end() && hp->second == 0U) continue;  // ignore the downed
      const f64 dx = enemy->position.x - body->position.x;
      const f64 dz = enemy->position.z - body->position.z;
      const f64 distance = std::sqrt(dx * dx + dz * dz);
      if (target == 0U || distance < bestDistance) {
        target = other;
        bestDistance = distance;
      }
    }
    if (target == 0U) return body->position;  // nobody left standing: hold
    const CharacterBody* enemy = physics_.characterById(target);
    if (enemy == nullptr) return body->position;
    // Hold at engagement range: close enough to shoot, far enough that a
    // firefight is not decided by who bumped into whom.
    const f64 dx = enemy->position.x - body->position.x;
    const f64 dz = enemy->position.z - body->position.z;
    const f64 distance = std::sqrt(dx * dx + dz * dz);
    if (distance < kMoveEpsilon) return body->position;
    const f64 want = distance - kArenaEngageRange;
    // Fan out sideways so a squad does not advance in single file.
    const f64 lane = (static_cast<f64>(id % 3U) - 1.0) * kArenaSpreadOut;
    return Vec3{std::min(boundX, std::max(-boundX, body->position.x + dx / distance * want - dz / distance * lane)),
                body->position.y,
                std::min(boundZ, std::max(-boundZ, body->position.z + dz / distance * want + dx / distance * lane))};
  }

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

  const f64 boundX = world_.halfWidth() - kPlayerMargin;
  const f64 limit = world_.halfLength() - kPlayerMargin;

  // --- The player on the ball ---
  if (id == aiChaser(team)) {
    const f64 toBallX = ball->position.x - body->position.x;
    const f64 toBallZ = ball->position.z - body->position.z;
    const f64 range = std::sqrt(toBallX * toBallX + toBallZ * toBallZ);

    // ATTACK: already on the ball, so carry it at their goal rather than
    // standing over it. This is the difference between a game and a scrum:
    // somebody has to actually take the ball somewhere.
    if (range < world_.ball.radius + kAiTackleReach + kAiApproachOffset) {
      // Run AT the goal. This used to stop short: the "trapped ball" rule
      // counted the END wall as trouble, but that is exactly where the net
      // is, so the attacker turned back every time it got close and the
      // score stayed 0-0 forever.
      //
      // Only the SIDE walls trap a ball. A ball in the corner gets taken
      // back infield; a ball near the goal line gets put in the net.
      const f64 wallX = world_.halfWidth() - world_.ball.radius;
      if (std::abs(ball->position.x) > wallX - kAiWallEscape) {
        return Vec3{0.0, body->position.y, ball->position.z};
      }
      const Vec3 mouth = aiGoalMouth(team);
      // Run THROUGH the ball toward the goal, not at the goal directly.
      // Heading straight for the net meant the player could end up on the
      // wrong side of the ball and drag it backwards, which made it
      // judder on the spot instead of advancing.
      const f64 toGoalX = mouth.x - ball->position.x;
      const f64 toGoalZ = mouth.z - ball->position.z;
      const f64 toGoal = std::sqrt(toGoalX * toGoalX + toGoalZ * toGoalZ);
      if (toGoal < kMoveEpsilon) return Vec3{mouth.x, body->position.y, mouth.z};
      // A point a stride beyond the ball, on the line from ball to goal.
      const f64 stepX = ball->position.x + toGoalX / toGoal * kAiGoalAim;
      const f64 stepZ = ball->position.z + toGoalZ / toGoal * kAiGoalAim;
      return Vec3{std::min(boundX, std::max(-boundX, stepX)), body->position.y,
                  std::min(limit, std::max(-limit, stepZ))};
    }

    // DEFEND / close down: approach the ball from our OWN goal side, not
    // its centre. Two chasers aiming at the exact same point met head on
    // and both stopped; arriving from behind it means they end up facing
    // the right way and the ball squirts free instead of jamming.
    return Vec3{ball->position.x, body->position.y,
                std::min(limit, std::max(-limit, ball->position.z - forward * kAiApproachOffset))};
  }

  // --- Everyone else: hold a shape relative to the ball ---
  // Each supporting player gets its OWN slot. The old code used id % 3, so on a
  // five-a-side team ids 7 and 10 were handed the identical spot and piled
  // up on each other. Numbering within the team fixes that.
  u32 slot = 0U;
  u32 mates = 0U;
  for (const u32 other : physics_.characterIds()) {
    if (other == kPrimaryCharacter) continue;
    const CharacterBody* mate = physics_.characterById(other);
    if (mate == nullptr || mate->team != team) continue;
    if (other == aiKeeper(team) || other == aiChaser(team)) continue;
    if (other == id) slot = mates;
    ++mates;
  }
  if (mates == 0U) mates = 1U;
  // Spread the supporting players evenly across the width instead of
  // stacking them: (slot + 1) / (mates + 1) maps to the whole pitch.
  const f64 t = static_cast<f64>(slot + 1U) / static_cast<f64>(mates + 1U);
  f64 lane = -boundX + 2.0 * boundX * t;
  // Keep out of the ball carrier's way. A lane that happens to run through
  // the ball turns the support player into a second attacker and they end
  // up jostling over it, which is the pile-up all over again.
  if (std::abs(lane - ball->position.x) < kAiPersonalSpace) {
    lane += lane < ball->position.x ? -kAiPersonalSpace : kAiPersonalSpace;
  }

  // With the ball, push UP in support so there is an option ahead; without
  // it, drop goal-side and defend. A team that only ever sits behind the
  // ball never attacks.
  const f64 gap = aiHasPossession(team) ? -kAiSupportGap * 0.5 : kAiSupportGap;
  const f64 supportZ = ball->position.z - forward * gap;
  return Vec3{std::min(boundX, std::max(-boundX, lane)), body->position.y,
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

    // --- Personal space ---
    // Steer away from anyone crowding this player. Without it two of them
    // walk into each other, each keeps pressing forward, and neither ever
    // moves again.
    const Vec3 apart = aiSeparation(id);
    wanted.x += apart.x * speed * kAiSeparationForce;
    wanted.z += apart.z * speed * kAiSeparationForce;

    // --- Getting unjammed ---
    // Wanting to move but going nowhere means something is in the way.
    // After kAiStuckTime of that, sidestep for a moment to walk around it.
    const bool tryingToMove = distance > kAiStuckDistance;
    const Vec3 previous = aiLastPos_.count(id) > 0U ? aiLastPos_[id] : body->position;
    const f64 travelled =
        std::sqrt(std::pow(body->position.x - previous.x, 2.0) + std::pow(body->position.z - previous.z, 2.0));
    f64& stuckFor = aiStuckFor_[id];
    f64& unstickFor = aiUnstickFor_[id];
    // Moving slower than a crawl while trying to run = something is in the way.
    if (tryingToMove && travelled < kAiStuckDistance * seconds) {
      stuckFor += seconds;
    } else {
      stuckFor = 0.0;
    }
    if (stuckFor > kAiStuckTime) {
      unstickFor = kAiUnstickTime;
      stuckFor = 0.0;
    }
    if (unstickFor > 0.0) {
      unstickFor -= seconds;
      // Strafe across the blocked direction. Odd and even ids go opposite
      // ways, so two players jammed together never pick the same escape.
      const f64 side = id % 2U == 0U ? 1.0 : -1.0;
      const f64 length = std::sqrt(wanted.x * wanted.x + wanted.z * wanted.z);
      if (length > kMoveEpsilon) {
        // Take a copy first: rotating in place would feed the already
        // updated x back into z and bend the sidestep off course.
        const f64 wx = wanted.x;
        const f64 wz = wanted.z;
        wanted.x += -wz / length * speed * side;
        wanted.z += wx / length * speed * side;
      } else {
        wanted.x += speed * side;
      }
    }
    aiLastPos_[id] = body->position;

    // Never ask for more than a run: the pushes above can stack up.
    const f64 wantedSpeed = std::sqrt(wanted.x * wanted.x + wanted.z * wanted.z);
    if (wantedSpeed > speed) {
      wanted.x = wanted.x / wantedSpeed * speed;
      wanted.z = wanted.z / wantedSpeed * speed;
    }
    physics_.moveCharacter(id, seconds, wanted);
    // Keep them on the pitch, like the human.
    body->position.x = std::min(boundX, std::max(-boundX, body->position.x));
    body->position.z = std::min(boundZ, std::max(-boundZ, body->position.z));

    // --- Carrying the ball ---
    // The player on the ball nudges it toward where it is running. Without
    // this an "attacking" player just stands over a stationary ball.
    if (ball != nullptr && aiRole(id) == AiRole::Attack) {
      const f64 ballDx = ball->position.x - body->position.x;
      const f64 ballDz = ball->position.z - body->position.z;
      if (std::sqrt(ballDx * ballDx + ballDz * ballDz) < world_.ball.radius + kAiTackleReach + kAiApproachOffset) {
        // Push it at the NET. Pushing it toward the player's own waypoint
        // sent it sideways or backwards, because that waypoint is a spot
        // beside the ball rather than somewhere to take it.
        const Vec3 want = aiGoalMouth(body->team);
        const f64 towardX = want.x - ball->position.x;
        const f64 towardZ = want.z - ball->position.z;
        const f64 towardLength = std::sqrt(towardX * towardX + towardZ * towardZ);
        if (towardLength > kMoveEpsilon) {
          // Close to the net: hit it. Dribbling all the way in gave the
          // defence time to get back every single time.
          const bool shooting = towardLength < kAiShootFrom;
          const f64 push = shooting ? kAiShootSpeed : kAiDribblePush;
          ball->velocity.x = towardX / towardLength * push * skill;
          ball->velocity.z = towardZ / towardLength * push * skill;
          if (shooting) events_.push_back(GameEvent::Kick);
        }
      }
    }

    // --- Tackling ---
    // An opponent who reaches the ball knocks it away toward their own
    // attacking end. This is what makes a trick risky: start showing off
    // in front of a defender and they will take it off you.
    //
    // Only a DEFENDER tackles. This used to fire for the attacker too, so
    // the side in possession booted its own ball goal-ward every frame —
    // straight into the end wall, where it stuck and the match froze.
    //
    // BOTH sides tackle. This was once limited to team 2, back when team 2
    // meant "the human's opponents" — but the player's own side is run by
    // the computer too, so that quietly made every match one-way: team 1
    // could hold the ball all game and never be able to win it back or
    // clear it, and finished 0-22.
    if (ball == nullptr) continue;
    if (aiRole(id) != AiRole::Defend) continue;
    const f64 ballDx = ball->position.x - body->position.x;
    const f64 ballDz = ball->position.z - body->position.z;
    if (std::sqrt(ballDx * ballDx + ballDz * ballDz) > world_.ball.radius + kAiTackleReach) continue;
    // A tackle CLEARS the ball away from the tackler, it does not fire it
    // up the pitch. Sending it toward the tackler's attacking end meant
    // every challenge nudged the ball the same way; with one side holding
    // an extra body, that bias piled up until the ball spent 92% of the
    // match in one half and only one team could ever score.
    const f64 awayLength = std::sqrt(ballDx * ballDx + ballDz * ballDz);
    if (awayLength > kMoveEpsilon) {
      // Clear it away, but bend the clearance back INFIELD. A defender on
      // the touchline otherwise hammers the ball straight out for a
      // throw-in every time, which on a narrow pitch stopped play almost
      // continuously.
      const f64 infield = -ball->position.x / std::max(world_.halfWidth(), 1e-6);
      f64 outX = ballDx / awayLength + infield;
      f64 outZ = ballDz / awayLength;
      const f64 outLength = std::sqrt(outX * outX + outZ * outZ);
      if (outLength > kMoveEpsilon) {
        outX /= outLength;
        outZ /= outLength;
      }
      ball->velocity.x = outX * kAiTacklePush * skill;
      ball->velocity.z = outZ * kAiTacklePush * skill;
    } else {
      // Dead on top of it: clear it toward the tackler's own attacking end.
      ball->velocity.z = attackDirectionZ(body->team) * kAiTacklePush * skill;
    }
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
  // Whatever the user's rules asked to say goes FIRST, on every kind of
  // game. Putting it inside one branch meant a win message was invisible
  // in a match, which is exactly where somebody would use it.
  if (!logicMessage_.empty()) lines.push_back(logicMessage_);
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
  if (arenaMode()) {
    // A shooter's HUD is health and ammo, not a football score.
    lines.push_back("MA " + std::to_string(arenaKills1_) + " - " + std::to_string(arenaKills2_) + " ANHA  " +
                    matchClockText());
    const std::string arena = arenaHudText();
    if (!arena.empty()) lines.push_back(arena);
    if (screen_ == Screen::RoundEnd) {
      const u32 winner = arenaKills1_ > arenaKills2_ ? 1U : (arenaKills2_ > arenaKills1_ ? 2U : 0U);
      lines.push_back(winner == 0U ? "DRAW" : (winner == 1U ? "MA BORDIM" : "ANHA BORDAND"));
    }
    const std::string sky = skyHudText();
    if (!sky.empty()) lines.push_back(sky);
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

// Built-in events are trigger names too (stage 31): attach an animation to
// "goal" in the editor and it plays when a goal is scored, with no code.
const char* WorldEditor::eventTriggerName(GameEvent event) {
  switch (event) {
    case GameEvent::Shot: return "shot";
    case GameEvent::Kick: return "kick";
    case GameEvent::Holed: return "holed";
    case GameEvent::Goal: return "goal";
    case GameEvent::RoundOver: return "roundover";
    case GameEvent::Whistle: return "whistle";
    case GameEvent::Tackle: return "tackle";
    case GameEvent::Trick: return "trick";
  }
  return "";
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
