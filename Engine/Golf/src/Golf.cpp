#include <kimia/Golf.h>
#include <kimia/SceneIO.h>

#include <algorithm>
#include <cmath>
#include <sstream>

namespace kimia {

namespace {

constexpr f64 kChargeRate = 0.9;   // power units per second (wraps 1 -> 0)
constexpr f64 kSunkTimer = 2.5;    // celebration before the reset
constexpr f64 kOutTimer = 1.5;     // pause before the ball returns to the tee
constexpr f64 kStopSpeed = 0.05;   // rolling is over below this
constexpr usize kUndoCap = 64U;

bool wallName(const std::string& name) { return name.rfind("Wall_", 0) == 0; }

u32 nameNumber(const std::string& name) {
  if (!wallName(name)) return 0U;
  u32 number = 0U;
  for (usize i = 5U; i < name.size(); ++i) {
    const char c = name[i];
    if (c < '0' || c > '9') return 0U;
    number = number * 10U + static_cast<u32>(c - '0');
  }
  return number;
}

}  // namespace

GolfGame::GolfGame() { resetToDefaultCourse(); }

void GolfGame::resetToDefaultCourse() {
  scene_.clear();
  auto add = [this](const EntityData& data) { return scene_.create(data); };
  {
    EntityData green;
    green.name = "Green";
    green.mesh = MeshKind::plane;
    green.transform.scale = Vec3{30.0, 1.0, 30.0};
    green.color = Vec3{0.22, 0.45, 0.24};
    green.roughness = 0.95;
    add(green);
  }
  {
    EntityData wall;
    wall.name = "Wall_1";
    wall.mesh = MeshKind::cube;
    wall.transform.position = Vec3{2.4, 0.5, 0.0};
    wall.transform.scale = Vec3{0.5, 1.0, 4.4};
    wall.color = Vec3{0.7, 0.68, 0.62};
    wall.roughness = 0.5;
    add(wall);
  }
  {
    EntityData tee;
    tee.name = "Tee";
    tee.mesh = MeshKind::cube;
    tee.transform.position = Vec3{0.0, 0.01, 7.0};
    tee.transform.scale = Vec3{0.4, 0.02, 0.4};
    tee.color = Vec3{0.9, 0.85, 0.3};
    tee.roughness = 0.8;
    add(tee);
  }
  {
    EntityData hole;
    hole.name = "Hole";
    hole.mesh = MeshKind::cube;
    hole.transform.position = Vec3{0.0, 0.01, -7.0};
    hole.transform.scale = Vec3{0.56, 0.02, 0.56};
    hole.color = Vec3{0.05, 0.05, 0.05};
    hole.roughness = 0.8;
    add(hole);
  }
  {
    EntityData pole;
    pole.name = "FlagPole";
    pole.mesh = MeshKind::cube;
    pole.transform.position = Vec3{0.0, 0.9, -7.0};
    pole.transform.scale = Vec3{0.03, 1.8, 0.03};
    pole.color = Vec3{0.9, 0.9, 0.9};
    pole.roughness = 0.4;
    add(pole);
  }
  {
    EntityData cloth;
    cloth.name = "FlagCloth";
    cloth.mesh = MeshKind::cube;
    cloth.transform.position = Vec3{0.31, 1.62, -7.0};
    cloth.transform.scale = Vec3{0.6, 0.35, 0.02};
    cloth.color = Vec3{0.85, 0.15, 0.15};
    cloth.roughness = 0.6;
    add(cloth);
  }
  {
    EntityData ball;
    ball.name = "Ball";
    ball.mesh = MeshKind::sphere;
    ball.transform.position = Vec3{0.0, 0.0, 0.0};
    ball.color = Vec3{0.95, 0.95, 0.92};
    ball.roughness = 0.3;
    add(ball);
  }
  scene_.demoShot = DemoShot{0.0, 0.61};
  lastShot_ = scene_.demoShot;
  strokes_ = 0U;
  sunk_ = false;
  mode_ = GolfMode::Aim;
  modeTimer_ = 0.0;
  power_ = 0.5;
  aimYaw_ = 0.0;
  undoStack_.clear();
  ghost_ = Vec3{0.0, 0.0, 0.0};
  tool_ = BuilderTool::Wall;
  wallLength_ = 4.0;
  wallAxisZ_ = true;
  teeHandle_ = findNamed("Tee");
  holeHandle_ = findNamed("Hole");
  const EntityData* tee = scene_.get(teeHandle_);
  if (tee != nullptr) {
    teeRest_ = Vec3{tee->transform.position.x, tee->transform.position.y + tee->transform.scale.y * 0.5 + kGolfBallRadius,
                    tee->transform.position.z};
  }
  const EntityData* hole = scene_.get(holeHandle_);
  if (hole != nullptr) holePos_ = hole->transform.position;
  rebuildPhysics();
}

EntityHandle GolfGame::findNamed(const std::string& name) const {
  EntityHandle found = kNullEntity;
  scene_.forEach([&found, &name](EntityHandle handle, const EntityData& entity) {
    if (found == kNullEntity && entity.name == name) found = handle;
  });
  return found;
}

u32 GolfGame::nextWallNumber() const {
  u32 highest = 0U;
  scene_.forEach([&highest](EntityHandle, const EntityData& entity) {
    highest = std::max(highest, nameNumber(entity.name));
  });
  return highest + 1U;
}

usize GolfGame::wallCount() const {
  usize count = 0U;
  scene_.forEach([&count](EntityHandle, const EntityData& entity) {
    if (wallName(entity.name)) ++count;
  });
  return count;
}

void GolfGame::rebuildPhysics() {
  world_.clear();
  f64 greenY = 0.0;
  const EntityHandle green = findNamed("Green");
  const EntityData* greenData = scene_.get(green);
  if (greenData != nullptr) greenY = greenData->transform.position.y;
  world_.addPlane(greenY);
  scene_.forEach([this](EntityHandle, const EntityData& entity) {
    if (wallName(entity.name)) {
      const Vec3 half = entity.transform.scale * 0.5;
      world_.addBox(entity.transform.position, half);
    }
  });
  SphereBody ball;
  ball.position = teeRest_;
  ball.radius = kGolfBallRadius;
  ball.restitution = kGolfBallRestitution;
  ball.friction = kGolfBallFriction;
  ball.rollingFriction = kGolfBallRollingFriction;
  ballId_ = world_.addSphere(ball);
}

void GolfGame::resetBallToTee() {
  SphereBody* ball = world_.sphere(ballId_);
  if (ball == nullptr) return;
  ball->position = teeRest_;
  ball->velocity = Vec3{0.0, 0.0, 0.0};
}

Vec3 GolfGame::ballPosition() const {
  const SphereBody* ball = world_.sphere(ballId_);
  return ball != nullptr ? ball->position : teeRest_;
}

Vec3 GolfGame::ballVelocity() const {
  const SphereBody* ball = world_.sphere(ballId_);
  return ball != nullptr ? ball->velocity : Vec3{0.0, 0.0, 0.0};
}

f64 GolfGame::ballSpeed() const { return ballVelocity().length(); }

Vec3 GolfGame::aimDirection() const {
  return Vec3{-std::sin(aimYaw_), 0.0, -std::cos(aimYaw_)};
}

void GolfGame::update(f64 hostSeconds) {
  switch (mode_) {
    case GolfMode::Aim:
    case GolfMode::Edit: {
      // The ball waits at the tee.
      SphereBody* ball = world_.sphere(ballId_);
      if (ball != nullptr) {
        ball->position = teeRest_;
        ball->velocity = Vec3{0.0, 0.0, 0.0};
      }
      break;
    }
    case GolfMode::Charge: {
      power_ += kChargeRate * hostSeconds;
      while (power_ >= 1.0) power_ -= 1.0;
      resetBallToTee();
      break;
    }
    case GolfMode::Roll: {
      world_.advance(hostSeconds);
      const Vec3 ball = ballPosition();
      const f64 dx = ball.x - holePos_.x;
      const f64 dz = ball.z - holePos_.z;
      const f64 horizontal = std::sqrt(dx * dx + dz * dz);
      if (horizontal < kGolfCupCaptureDistance && ballSpeed() < kGolfCupCaptureSpeed) {
        sunk_ = true;
        mode_ = GolfMode::Sunk;
        modeTimer_ = kSunkTimer;
        resetBallToTee();  // the render draws the ball resting in the cup
      } else if (ballSpeed() < kStopSpeed) {
        mode_ = GolfMode::Out;
        modeTimer_ = kOutTimer;
      }
      break;
    }
    case GolfMode::Sunk: {
      modeTimer_ -= hostSeconds;
      if (modeTimer_ <= 0.0) {
        sunk_ = false;
        strokes_ = 0U;
        resetBallToTee();
        mode_ = GolfMode::Aim;
      }
      break;
    }
    case GolfMode::Out: {
      modeTimer_ -= hostSeconds;
      if (modeTimer_ <= 0.0) {
        resetBallToTee();
        mode_ = GolfMode::Aim;
      }
      break;
    }
  }
}

void GolfGame::chargeBegin() {
  if (mode_ == GolfMode::Aim) {
    mode_ = GolfMode::Charge;
    power_ = 0.0;
  }
}

void GolfGame::chargeEnd() {
  if (mode_ == GolfMode::Charge) launch(power_);
}

void GolfGame::launch(f64 shotPower) {
  const f64 speed = kGolfLaunchBaseSpeed + shotPower * kGolfLaunchPowerScale;
  SphereBody* ball = world_.sphere(ballId_);
  if (ball == nullptr) return;
  ball->position = teeRest_;
  ball->velocity = aimDirection() * speed;
  ++strokes_;
  lastShot_ = DemoShot{aimYaw_, shotPower};
  sunk_ = false;
  mode_ = GolfMode::Roll;
}

void GolfGame::launchDemoShot() {
  const std::optional<DemoShot> demo = scene_.demoShot;
  if (!demo.has_value()) return;
  if (mode_ != GolfMode::Aim) {
    sunk_ = false;
    mode_ = GolfMode::Aim;
    modeTimer_ = 0.0;
  }
  aimYaw_ = demo->aim;
  power_ = demo->power;
  launch(power_);
}

void GolfGame::togglePlayEdit() {
  if (mode_ == GolfMode::Edit) {
    resetBallToTee();
    mode_ = GolfMode::Aim;
    modeTimer_ = 0.0;
  } else {
    resetBallToTee();
    mode_ = GolfMode::Edit;
    modeTimer_ = 0.0;
  }
}

void GolfGame::setTool(BuilderTool tool) {
  if (mode_ != GolfMode::Edit) togglePlayEdit();
  tool_ = tool;
}

void GolfGame::adjustWallLength(f64 delta) {
  if (mode_ != GolfMode::Edit) togglePlayEdit();
  wallLength_ = std::min(kGolfWallMaxLength, std::max(kGolfWallMinLength, wallLength_ + delta));
}

void GolfGame::toggleWallAxis() {
  if (mode_ != GolfMode::Edit) togglePlayEdit();
  wallAxisZ_ = !wallAxisZ_;
}

void GolfGame::moveGhost(f64 dx, f64 dz, bool fine) {
  if (mode_ != GolfMode::Edit) togglePlayEdit();
  const f64 step = fine ? 0.1 : 1.0;
  ghost_.x += dx * step;
  ghost_.z += dz * step;
}

Vec3 GolfGame::wallScale() const {
  return wallAxisZ_ ? Vec3{0.5, 1.0, wallLength_} : Vec3{wallLength_, 1.0, 0.5};
}

bool GolfGame::place() {
  if (mode_ != GolfMode::Edit) togglePlayEdit();
  if (tool_ == BuilderTool::Wall) {
    EntityData wall;
    wall.name = "Wall_" + std::to_string(nextWallNumber());
    wall.mesh = MeshKind::cube;
    wall.transform.position = Vec3{ghost_.x, 0.5, ghost_.z};
    wall.transform.scale = wallScale();
    wall.color = Vec3{0.7, 0.68, 0.62};
    wall.roughness = 0.5;
    const EntityHandle handle = scene_.create(wall);
    rebuildPhysics();
    UndoAction action;
    action.kind = UndoAction::PlacedWall;
    action.wallHandle = handle;
    undoStack_.push_back(action);
    if (undoStack_.size() > kUndoCap) undoStack_.erase(undoStack_.begin());
    return true;
  }
  const bool tee = tool_ == BuilderTool::Tee;
  const EntityHandle oldHandle = tee ? teeHandle_ : holeHandle_;
  const EntityData* old = scene_.get(oldHandle);
  if (old == nullptr) return false;
  const Vec3 before = old->transform.position;
  EntityData moved = *old;
  moved.transform.position = Vec3{ghost_.x, before.y, ghost_.z};
  scene_.destroy(oldHandle);
  const EntityHandle fresh = scene_.create(moved);
  if (tee) {
    teeHandle_ = fresh;
    teeRest_ = Vec3{ghost_.x, before.y + moved.transform.scale.y * 0.5 + kGolfBallRadius, ghost_.z};
    resetBallToTee();
  } else {
    holeHandle_ = fresh;
    holePos_ = moved.transform.position;
  }
  UndoAction action;
  action.kind = tee ? UndoAction::MovedTee : UndoAction::MovedHole;
  action.before = before;
  undoStack_.push_back(action);
  if (undoStack_.size() > kUndoCap) undoStack_.erase(undoStack_.begin());
  return true;
}

bool GolfGame::undo() {
  if (mode_ != GolfMode::Edit) togglePlayEdit();
  if (undoStack_.empty()) return false;
  const UndoAction action = undoStack_.back();
  undoStack_.pop_back();
  if (action.kind == UndoAction::PlacedWall) {
    scene_.destroy(action.wallHandle);
    rebuildPhysics();
    return true;
  }
  const bool tee = action.kind == UndoAction::MovedTee;
  const EntityHandle oldHandle = tee ? teeHandle_ : holeHandle_;
  const EntityData* old = scene_.get(oldHandle);
  if (old == nullptr) return false;
  EntityData restored = *old;
  restored.transform.position = action.before;
  scene_.destroy(oldHandle);
  const EntityHandle fresh = scene_.create(restored);
  if (tee) {
    teeHandle_ = fresh;
    teeRest_ = Vec3{action.before.x, action.before.y + restored.transform.scale.y * 0.5 + kGolfBallRadius,
                    action.before.z};
    resetBallToTee();
  } else {
    holeHandle_ = fresh;
    holePos_ = restored.transform.position;
  }
  return true;
}

bool GolfGame::loadCourseText(const std::string& text, std::string& error) {
  Scene loaded;
  if (!SceneIO::load(text, loaded, error)) return false;
  scene_ = std::move(loaded);
  teeHandle_ = findNamed("Tee");
  holeHandle_ = findNamed("Hole");
  teeRest_ = Vec3{0.0, 0.14, 7.0};
  holePos_ = Vec3{0.0, 0.01, -7.0};
  const EntityData* tee = scene_.get(teeHandle_);
  if (tee != nullptr) {
    teeRest_ = Vec3{tee->transform.position.x, tee->transform.position.y + tee->transform.scale.y * 0.5 + kGolfBallRadius,
                    tee->transform.position.z};
  }
  const EntityData* hole = scene_.get(holeHandle_);
  if (hole != nullptr) holePos_ = hole->transform.position;
  undoStack_.clear();
  strokes_ = 0U;
  sunk_ = false;
  modeTimer_ = 0.0;
  power_ = 0.5;
  aimYaw_ = 0.0;
  lastShot_ = scene_.demoShot;
  mode_ = GolfMode::Edit;  // loading is a builder action
  rebuildPhysics();
  return true;
}

bool GolfGame::loadCourse(const std::string& path, std::string& error) {
  Scene loaded;
  if (!SceneIO::loadFromFile(path, loaded, error)) return false;
  scene_ = std::move(loaded);
  teeHandle_ = findNamed("Tee");
  holeHandle_ = findNamed("Hole");
  teeRest_ = Vec3{0.0, 0.14, 7.0};
  holePos_ = Vec3{0.0, 0.01, -7.0};
  const EntityData* tee = scene_.get(teeHandle_);
  if (tee != nullptr) {
    teeRest_ = Vec3{tee->transform.position.x, tee->transform.position.y + tee->transform.scale.y * 0.5 + kGolfBallRadius,
                    tee->transform.position.z};
  }
  const EntityData* hole = scene_.get(holeHandle_);
  if (hole != nullptr) holePos_ = hole->transform.position;
  undoStack_.clear();
  strokes_ = 0U;
  sunk_ = false;
  modeTimer_ = 0.0;
  power_ = 0.5;
  aimYaw_ = 0.0;
  lastShot_ = scene_.demoShot;
  mode_ = GolfMode::Edit;
  rebuildPhysics();
  return true;
}

bool GolfGame::saveCourse(const std::string& path, std::string& error) {
  const EntityHandle ballEntity = findNamed("Ball");
  EntityData* ballData = scene_.get(ballEntity);
  if (ballData != nullptr) {
    ballData->transform.position = teeRest_;
  } else {
    EntityData ball;
    ball.name = "Ball";
    ball.mesh = MeshKind::sphere;
    ball.transform.position = teeRest_;
    ball.color = Vec3{0.95, 0.95, 0.92};
    ball.roughness = 0.3;
    scene_.create(ball);
  }
  if (lastShot_.has_value()) scene_.demoShot = lastShot_;
  const bool ok = SceneIO::saveToFile(scene_, path);
  if (!ok) error = "failed to write course file: " + path;
  return ok;
}

void GolfGame::placeBall(const Vec3& position, const Vec3& velocity) {
  SphereBody* ball = world_.sphere(ballId_);
  if (ball == nullptr) return;
  ball->position = position;
  ball->velocity = velocity;
  sunk_ = false;
  mode_ = GolfMode::Roll;
  modeTimer_ = 0.0;
}

std::string GolfGame::statsLine() const {
  const char* modeName = "EDIT";
  switch (mode_) {
    case GolfMode::Edit: modeName = "EDIT"; break;
    case GolfMode::Aim: modeName = "AIM"; break;
    case GolfMode::Charge: modeName = "CHARGE"; break;
    case GolfMode::Roll: modeName = "ROLL"; break;
    case GolfMode::Sunk: modeName = "SUNK"; break;
    case GolfMode::Out: modeName = "OUT"; break;
  }
  const char* toolName = "wall";
  switch (tool_) {
    case BuilderTool::Wall: toolName = "wall"; break;
    case BuilderTool::Tee: toolName = "tee"; break;
    case BuilderTool::Hole: toolName = "hole"; break;
  }
  std::ostringstream line;
  line << "KIMIA GOLF | " << modeName << " | stroke " << strokes_ << " | power "
       << static_cast<i32>(power_ * 100.0) << "% | tool " << toolName << " len " << wallLength_ << " "
       << (wallAxisZ_ ? "Z" : "X") << " | walls " << wallCount();
  return line.str();
}

}  // namespace kimia
