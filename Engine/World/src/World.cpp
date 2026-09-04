#include <kimia/World.h>
#include <kimia/WorldIO.h>

#include <kimia/Golf.h>  // the "accurate" ball IS the golf tuning

#include <algorithm>
#include <cmath>
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
    case 0:
      return "MAIN";
    case 1:
      return "MENU";
    case 2:
      return "PLAYER";
    case 3:
      return "BALL";
    case 4:
      return "ENV";
    case 5:
      return "PLAY";
    default:
      return "GOAL";
  }
}

}  // namespace

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

void buildDefaultWorldScene(WorldData& world) {
  world.scene.clear();
  const EnvironmentColors colors = environmentColors(world.environment);
  {
    EntityData ground;
    ground.name = "Ground";
    ground.mesh = MeshKind::plane;
    ground.transform.scale = Vec3{kWorldFloorHalf * 2.0, 1.0, kWorldFloorHalf * 2.0};
    ground.color = colors.floor;
    ground.roughness = 0.95;
    world.scene.create(ground);
  }
  const auto addPost = [&world](const std::string& name, f64 x, f64 z) {
    EntityData post;
    post.name = name;
    post.mesh = MeshKind::cube;
    post.transform.position = Vec3{x, 1.0, z};
    post.transform.scale = Vec3{0.12, 2.0, 0.12};
    post.color = Vec3{0.9, 0.9, 0.9};
    post.roughness = 0.4;
    world.scene.create(post);
  };
  addPost("GoalPostLeft", -kWorldGoalHalfWidth, kWorldGoalZ);
  addPost("GoalPostRight", kWorldGoalHalfWidth, kWorldGoalZ);
  {
    EntityData bar;
    bar.name = "GoalBar";
    bar.mesh = MeshKind::cube;
    bar.transform.position = Vec3{0.0, 2.02, kWorldGoalZ};
    bar.transform.scale = Vec3{kWorldGoalHalfWidth * 2.0 + 0.12, 0.12, 0.12};
    bar.color = Vec3{0.9, 0.9, 0.9};
    bar.roughness = 0.4;
    world.scene.create(bar);
  }
}

WorldEditor::WorldEditor() { rebuildPhysics(); }

void WorldEditor::rebuildPhysics() {
  physics_.clear();
  physics_.addPlane(0.0);
  world_.scene.forEach([this](EntityHandle, const EntityData& entity) {
    if (entity.name.rfind("GoalPost", 0) == 0 || entity.name == "GoalBar") {
      physics_.addBox(entity.transform.position, entity.transform.scale * 0.5);
    }
  });
  SphereBody ball;
  ball.position = ballRest();
  ball.radius = world_.ball.radius;
  ball.restitution = world_.ball.restitution;
  ball.friction = world_.ball.friction;
  ball.rollingFriction = world_.ball.rollingFriction;
  ballId_ = physics_.addSphere(ball);
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
  const EntityHandle ground = world_.scene.find("Ground");
  EntityData* data = world_.scene.get(ground);
  if (data != nullptr) data->color = environmentColors(world_.environment).floor;
}

void WorldEditor::createWorld() {
  world_ = WorldData{};
  buildDefaultWorldScene(world_);
  applyBallType(world_.ball, BallType::Accurate);
  hasWorld_ = true;
  lastError_.clear();
  screen_ = Screen::Menu;
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
  screen_ = Screen::Menu;
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
  moveInput_ = Vec3{0.0, 0.0, 0.0};
  goalTimer_ = 0.0;
  resetBallToCenter();
  lastError_.clear();
  screen_ = Screen::Play;
}

std::string WorldEditor::menuTitle() const {
  switch (screen_) {
    case Screen::Main:
      return "KIMIA World \xE2\x80\x94 \xD9\x85\xD9\x86\xD9\x88\xDB\x8C \xD8\xA7\xD8\xB5\xD9\x84\xDB\x8C";
    case Screen::Menu:
      return world_.name +
             " \xE2\x80\x94 \xDA\x86\xD9\x87 \xDA\xA9\xD8\xA7\xD8\xB1 \xDA\xA9\xD9\x86\xDB\x8C\xD9\x85\xD8\x9F";
    case Screen::AskPlayer:
      return "\xD8\xA8\xD8\xA7\xD8\xB2\xDB\x8C\xDA\xA9\xD9\x86\x3A \xDA\x86\xD9\x87 \xD8\xB3\xD8\xB1\xD8\xB9\xD8\xAA\xDB\x8C\xD8\x9F";
    case Screen::AskBall:
      return "\xD8\xAA\xD9\x88\xD9\xBE\x3A \xD8\xAF\xD9\x82\xDB\x8C\xD9\x82 \xD8\xA8\xD8\xA7\xD8\xB4\xD9\x87 \xDB\x8C\xD8\xA7 \xD9\x81\xD8\xA7\xD9\x86\xD8\xAA\xD8\xB2\xDB\x8C\xD8\x9F";
    case Screen::AskEnvironment:
      return "\xD9\x85\xD8\xAD\xDB\x8C\xD8\xB7\x3A \xDA\x86\xD9\x87 \xD9\x81\xD8\xB6\xD8\xA7\xDB\x8C\xDB\x8C\xD8\x9F";
    case Screen::Play:
      return "\xD8\xAF\xD8\xB1 \xD8\xAD\xD8\xA7\xD9\x84 \xD8\xA8\xD8\xA7\xD8\xB2\xDB\x8C \xE2\x80\x94 \xD8\xA8\xD8\xA7 \xD8\xAC\xD9\x87\xD8\xAA\xD9\x87\xD8\xA7 \xD8\xAD\xD8\xB1\xDA\xA9\xD8\xAA \xDA\xA9\xD9\x86";
    default:
      return "\xDA\xAF\xD9\x84 \xD8\xB4\xD8\xAF!";
  }
}

std::vector<std::string> WorldEditor::optionLabels() const {
  switch (screen_) {
    case Screen::Main:
      return {"\xD8\xAF\xD9\x86\xDB\x8C\xD8\xA7\xDB\x8C \xD8\xAC\xD8\xAF\xDB\x8C\xD8\xAF",
              "\xD8\xA8\xD8\xA7\xD8\xB2 \xDA\xA9\xD8\xB1\xD8\xAF\xD9\x86 \xD8\xAF\xD9\x86\xDB\x8C\xD8\xA7",
              "\xD8\xAE\xD8\xB1\xD9\x88\xD8\xAC"};
    case Screen::Menu:
      return {"\xD8\xA7\xD9\x81\xD8\xB2\xD9\x88\xD8\xAF\xD9\x86 \xD8\xA8\xD8\xA7\xD8\xB2\xDB\x8C\xDA\xA9\xD9\x86",
              "\xD8\xA7\xD9\x81\xD8\xB2\xD9\x88\xD8\xAF\xD9\x86 \xD8\xAA\xD9\x88\xD9\xBE",
              "\xD8\xA7\xD9\x81\xD8\xB2\xD9\x88\xD8\xAF\xD9\x86 \xD9\x85\xD8\xAD\xDB\x8C\xD8\xB7",
              "\xD8\xA8\xD8\xA7\xD8\xB2\xDB\x8C \x28PLAY\x29", "\xD8\xB0\xD8\xAE\xDB\x8C\xD8\xB1\xD9\x87",
              "\xD9\x85\xD9\x86\xD9\x88\xDB\x8C \xD8\xA7\xD8\xB5\xD9\x84\xDB\x8C"};
    case Screen::AskPlayer:
      return {"\xD8\xB3\xD8\xB1\xDB\x8C\xD8\xB9", "\xD8\xB9\xD8\xA7\xD8\xAF\xDB\x8C",
              "\xD8\xA2\xD8\xB1\xD8\xA7\xD9\x85", "\xD8\xA8\xD8\xA7\xD8\xB2\xDA\xAF\xD8\xB4\xD8\xAA"};
    case Screen::AskBall:
      return {"\xD8\xAF\xD9\x82\xDB\x8C\xD9\x82 \x28\xD9\x85\xD8\xAB\xD9\x84 \xDA\xAF\xD9\x84\xD9\x81\x29",
              "\xD9\x81\xD8\xA7\xD9\x86\xD8\xAA\xD8\xB2\xDB\x8C \x28\xD9\xBE\xD8\xB1\xD8\xA7\xD9\x86 \xD9\x88 \xD9\x84\xD8\xBA\xD8\xB2\xD9\x86\xD8\xAF\xD9\x87\x29",
              "\xD8\xA8\xD8\xA7\xD8\xB2\xDA\xAF\xD8\xB4\xD8\xAA"};
    case Screen::AskEnvironment:
      return {"\xDA\x86\xD9\x85\xD9\x86\xD8\xB2\xD8\xA7\xD8\xB1", "\xD8\xB4\xD9\x86\xDB\x8C \x28\xDA\xA9\xD9\x88\xDB\x8C\xD8\xB1\x29",
              "\xD8\xB4\xD8\xA8", "\xD8\xA8\xD8\xA7\xD8\xB2\xDA\xAF\xD8\xB4\xD8\xAA"};
    default:
      return {};
  }
}

std::vector<std::pair<std::string, std::string>> WorldEditor::holdPad() const {
  if (!playing()) return {};
  return {{"\xE2\x86\x91", "up"}, {"\xE2\x86\x90", "left"}, {"\xE2\x86\x93", "down"},
          {"\xE2\x86\x92", "right"}};
}

std::vector<std::pair<std::string, std::string>> WorldEditor::tapPad() const {
  if (!playing()) return {};
  return {{"\xD8\xAA\xD9\x88\xD9\xBE \xD8\xA7\xD8\xB2 \xD9\x86\xD9\x88", "r"},
          {"\xD9\x85\xD9\x86\xD9\x88", "b"}};
}

void WorldEditor::choose(i32 optionIndex) {
  switch (screen_) {
    case Screen::Main: {
      if (optionIndex == 0) {
        createWorld();
      } else if (optionIndex == 1) {
        std::string error;
        if (!loadWorld(worldPath_, error)) {
          // Stay on the main menu; the error is visible in the stats line.
          lastError_ = error;
        }
      } else if (optionIndex == 2) {
        quitRequested_ = true;
      }
      break;
    }
    case Screen::Menu: {
      if (optionIndex == 0) {
        screen_ = Screen::AskPlayer;
      } else if (optionIndex == 1) {
        screen_ = Screen::AskBall;
      } else if (optionIndex == 2) {
        screen_ = Screen::AskEnvironment;
      } else if (optionIndex == 3) {
        enterPlay();
      } else if (optionIndex == 4) {
        std::string error;
        saveWorld(worldPath_, error);
      } else if (optionIndex == 5) {
        screen_ = Screen::Main;
      }
      break;
    }
    case Screen::AskPlayer: {
      if (optionIndex == 0) {
        world_.player.speed = kWorldPlayerFast;
      } else if (optionIndex == 1) {
        world_.player.speed = kWorldPlayerNormal;
      } else if (optionIndex == 2) {
        world_.player.speed = kWorldPlayerSlow;
      }
      screen_ = Screen::Menu;
      break;
    }
    case Screen::AskBall: {
      if (optionIndex == 0) {
        applyBallType(world_.ball, BallType::Accurate);
      } else if (optionIndex == 1) {
        applyBallType(world_.ball, BallType::Fantasy);
      }
      rebuildPhysics();
      resetBallToCenter();
      screen_ = Screen::Menu;
      break;
    }
    case Screen::AskEnvironment: {
      if (optionIndex == 0) {
        world_.environment = EnvironmentKind::Grass;
      } else if (optionIndex == 1) {
        world_.environment = EnvironmentKind::Sand;
      } else if (optionIndex == 2) {
        world_.environment = EnvironmentKind::Night;
      }
      applyEnvironmentToScene();
      screen_ = Screen::Menu;
      break;
    }
    default:
      break;  // play screens have no options; a tap is not a silent no-op
  }
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
    screen_ = Screen::Menu;
  }
}

void WorldEditor::update(f64 hostSeconds) {
  if (screen_ == Screen::Play) {
    Vec3 direction = moveInput_;
    const f64 lengthSquared = direction.x * direction.x + direction.z * direction.z;
    const f64 length = std::sqrt(lengthSquared);
    if (length > 1.0) {
      direction.x /= length;
      direction.z /= length;
    }
    const bool moving = length > kMoveEpsilon;

    playerPos_.x += direction.x * world_.player.speed * hostSeconds;
    playerPos_.z += direction.z * world_.player.speed * hostSeconds;
    const f64 bound = kWorldFloorHalf - kPlayerMargin;
    playerPos_.x = std::min(bound, std::max(-bound, playerPos_.x));
    playerPos_.z = std::min(bound, std::max(-bound, playerPos_.z));

    SphereBody* ball = physics_.sphere(ballId_);
    if (ball != nullptr && moving) {
      const f64 dx = ball->position.x - playerPos_.x;
      const f64 dz = ball->position.z - playerPos_.z;
      const f64 distance = std::sqrt(dx * dx + dz * dz);
      if (distance < world_.ball.radius + kWorldKickReach && ball->velocity.length() < kKickMaxSpeed) {
        ball->velocity = direction * (kWorldKickBase + world_.player.speed * kWorldKickSpeedScale) +
                         Vec3{0.0, kWorldKickUp, 0.0};
      }
    }

    physics_.advance(hostSeconds);

    const Vec3 position = ballPosition();
    if (position.z < kWorldGoalZ && std::abs(position.x) < kWorldGoalHalfWidth &&
        position.y < kWorldGoalBarHeight) {
      ++world_.score;
      screen_ = Screen::Goal;
      goalTimer_ = kGoalCelebration;
      if (ball != nullptr) ball->velocity = Vec3{0.0, 0.0, 0.0};
    }
  } else if (screen_ == Screen::Goal) {
    physics_.advance(hostSeconds);
    goalTimer_ -= hostSeconds;
    if (goalTimer_ <= 0.0) {
      resetBallToCenter();
      screen_ = Screen::Play;
    }
  } else {
    // Menu screens: the ball waits at the center.
    resetBallToCenter();
  }
}

std::string WorldEditor::statsLine() const {
  std::ostringstream line;
  line << "KIMIA WORLD | " << screenName(static_cast<int>(screen_)) << " | world " << world_.name
       << " | player " << playerSpeedName(world_.player.speed) << " | ball " << ballTypeName(world_.ball.type)
       << " | env " << environmentName(world_.environment) << " | score " << world_.score;
  if (!lastError_.empty()) line << " | note " << lastError_;
  return line.str();
}

}  // namespace kimia
