#include <kimia/AssetPipeline.h>
#include <kimia/World.h>

#include <dirent.h>

#include <algorithm>
#include <cstring>
#include <sstream>

namespace kimia {

namespace {

constexpr f64 kBlockColor = 0.62;
constexpr f64 kWallColorR = 0.7;
constexpr f64 kWallColorG = 0.68;
constexpr f64 kWallColorB = 0.62;
constexpr f64 kCrateColorR = 0.55;  // wooden crate brown
constexpr f64 kCrateColorG = 0.40;
constexpr f64 kCrateColorB = 0.25;

u32 nameNumber(const std::string& name, const char* prefix) {
  const usize prefixLength = std::strlen(prefix);
  if (name.size() <= prefixLength || name.compare(0, prefixLength, prefix) != 0) return 0U;
  u32 number = 0U;
  for (usize i = prefixLength; i < name.size(); ++i) {
    const char c = name[i];
    if (c < '0' || c > '9') return 0U;
    number = number * 10U + static_cast<u32>(c - '0');
  }
  return number;
}

u32 nextNumber(const Scene& scene, const char* prefix) {
  u32 highest = 0U;
  scene.forEach([&highest, prefix](EntityHandle, const EntityData& entity) {
    highest = std::max(highest, nameNumber(entity.name, prefix));
  });
  return highest + 1U;
}

bool hasExtension(const std::string& name, const char* ext) {
  const usize extLength = std::strlen(ext);
  if (name.size() <= extLength) return false;
  for (usize i = 0; i < extLength; ++i) {
    char c = name[name.size() - extLength + i];
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    if (c != ext[i]) return false;
  }
  return true;
}

}  // namespace

void WorldEditor::refreshImportFiles() {
  importFiles_.clear();
  DIR* dir = ::opendir(importDir_.c_str());
  if (dir != nullptr) {
    while (dirent* entry = ::readdir(dir)) {
      const std::string name = entry->d_name;
      if (!hasExtension(name, ".obj") && !hasExtension(name, ".fbx")) continue;
      importFiles_.push_back(name);
    }
    ::closedir(dir);
  }
  std::sort(importFiles_.begin(), importFiles_.end());
  if (importPage_ * 5U >= importFiles_.size()) importPage_ = 0U;
}

// --- Management accessors ---

const EntityData* WorldEditor::selectedEntity() const {
  if (managedIndex_ >= managed_.size()) return nullptr;
  return world_.scene.get(managed_[managedIndex_]);
}

std::string WorldEditor::managedName() const {
  const EntityData* entity = selectedEntity();
  return entity != nullptr ? entity->name : std::string{};
}

std::string WorldEditor::managedKindName() const {
  const EntityData* entity = selectedEntity();
  if (entity == nullptr) return std::string{};
  switch (objectKindForName(entity->name)) {
    case ObjectKind::Player:
      return "player";
    case ObjectKind::Ball:
      return "ball";
    case ObjectKind::Block:
      return "block";
    case ObjectKind::Wall:
      return "wall";
    case ObjectKind::Goal:
      return "goal";
    case ObjectKind::Crate:
      return "crate";
    case ObjectKind::Model:
      return "model";
    default:
      return "other";
  }
}

void WorldEditor::refreshManaged() {
  managed_.clear();
  world_.scene.forEach([this](EntityHandle handle, const EntityData& entity) {
    if (entity.name != "Ground") managed_.push_back(handle);
  });
  if (managedIndex_ >= managed_.size() && !managed_.empty()) managedIndex_ = 0U;
  if (managed_.empty()) managedIndex_ = 0U;
}

void WorldEditor::beginPlace() {
  ghost_ = Vec3{0.0, 0.0, 0.0};
  screen_ = Screen::Place;
}

void WorldEditor::confirmPlace() {
  switch (pendingKind_) {
    case ObjectKind::Player: {
      EntityData player;
      player.name = "Player";
      player.mesh = MeshKind::cube;
      player.transform.position = Vec3{ghost_.x, 0.5, ghost_.z};
      player.transform.scale = Vec3{0.6, 1.0, 0.6};
      player.color = world_.player.color;
      player.roughness = 0.5;
      const EntityHandle existing = playerEntity();
      if (existing != kNullEntity) {
        EntityData* current = world_.scene.get(existing);
        if (current != nullptr) current->transform.position = player.transform.position;
      } else {
        world_.scene.create(player);
      }
      break;
    }
    case ObjectKind::Ball: {
      const f64 radius = world_.ball.radius;
      EntityData ball;
      ball.name = "Ball";
      ball.mesh = MeshKind::sphere;
      ball.transform.position = Vec3{ghost_.x, radius, ghost_.z};
      ball.transform.scale = Vec3{radius * 2.0, radius * 2.0, radius * 2.0};
      ball.color = world_.ball.color;
      ball.roughness = 0.3;
      const EntityHandle existing = ballEntity();
      if (existing != kNullEntity) {
        EntityData* current = world_.scene.get(existing);
        if (current != nullptr) *current = ball;
      } else {
        world_.scene.create(ball);
      }
      rebuildPhysics();
      break;
    }
    case ObjectKind::Block: {
      EntityData block;
      block.name = "Block_" + std::to_string(nextNumber(world_.scene, "Block_"));
      block.mesh = MeshKind::cube;
      block.transform.position = Vec3{ghost_.x, pendingSize_ * 0.5, ghost_.z};
      block.transform.scale = Vec3{pendingSize_, pendingSize_, pendingSize_};
      block.color = Vec3{kBlockColor, kBlockColor, kBlockColor};
      block.roughness = 0.5;
      world_.scene.create(block);
      rebuildPhysics();
      break;
    }
    case ObjectKind::Wall: {
      EntityData wall;
      wall.name = "Wall_" + std::to_string(nextNumber(world_.scene, "Wall_"));
      wall.mesh = MeshKind::cube;
      wall.transform.position = Vec3{ghost_.x, 0.5, ghost_.z};
      wall.transform.scale = pendingAxisZ_ ? Vec3{0.5, 1.0, pendingSize_} : Vec3{pendingSize_, 1.0, 0.5};
      wall.color = Vec3{kWallColorR, kWallColorG, kWallColorB};
      wall.roughness = 0.5;
      world_.scene.create(wall);
      rebuildPhysics();
      break;
    }
    case ObjectKind::Goal: {
      EntityData goal;
      goal.name = "Goal_" + std::to_string(nextNumber(world_.scene, "Goal_"));
      goal.mesh = MeshKind::cube;
      goal.transform.position = Vec3{ghost_.x, kWorldGoalHeight * 0.5, ghost_.z};
      goal.transform.scale = Vec3{pendingSize_, kWorldGoalHeight, 0.12};
      goal.color = Vec3{0.9, 0.9, 0.9};
      goal.roughness = 0.4;
      world_.scene.create(goal);
      rebuildPhysics();
      break;
    }
    case ObjectKind::Crate: {
      EntityData crate;
      crate.name = "Crate_" + std::to_string(nextNumber(world_.scene, "Crate_"));
      crate.mesh = MeshKind::cube;
      crate.transform.position = Vec3{ghost_.x, kWorldCrateSize * 0.5, ghost_.z};
      crate.transform.scale = Vec3{kWorldCrateSize, kWorldCrateSize, kWorldCrateSize};
      crate.color = Vec3{kCrateColorR, kCrateColorG, kCrateColorB};
      crate.roughness = 0.6;
      world_.scene.create(crate);
      rebuildPhysics();
      break;
    }
    case ObjectKind::Model: {
      EntityData model;
      model.name = "Model_" + std::to_string(nextNumber(world_.scene, "Model_"));
      model.mesh = MeshKind::cube;  // fallback shape; meshFile drives rendering
      model.transform.position = Vec3{ghost_.x, ghost_.y, ghost_.z};
      model.transform.scale = Vec3{pendingSize_, pendingSize_, pendingSize_};
      model.color = Vec3{1.0, 1.0, 1.0};
      model.roughness = 0.5;
      if (!pendingFile_.empty()) {
        const std::string& dir = importDir_;
        model.meshFile = (!dir.empty() && dir.back() != '/') ? dir + "/" + pendingFile_ : dir + pendingFile_;
        // Unity-style import normalization: fit the model's largest bounding
        // dimension to the chosen size, so any OBJ/FBX becomes a prop of the
        // requested size regardless of the source file's units.
        std::string loadError;
        auto loaded = kimia::assets::loadMesh(model.meshFile, loadError);
        if (loaded.has_value() && !loaded->mesh.positions.empty()) {
          Vec3 lo = loaded->mesh.positions[0];
          Vec3 hi = loaded->mesh.positions[0];
          for (const Vec3& p : loaded->mesh.positions) {
            lo.x = std::min(lo.x, p.x);
            lo.y = std::min(lo.y, p.y);
            lo.z = std::min(lo.z, p.z);
            hi.x = std::max(hi.x, p.x);
            hi.y = std::max(hi.y, p.y);
            hi.z = std::max(hi.z, p.z);
          }
          const f64 size = std::max(hi.x - lo.x, std::max(hi.y - lo.y, hi.z - lo.z));
          if (size > 1e-6) {
            const f64 fit = pendingSize_ / size;
            model.transform.scale = Vec3{fit, fit, fit};
          }
        }
      }
      world_.scene.create(model);
      pendingFile_.clear();
      break;
    }
    default:
      break;
  }
  lastError_.clear();
  // Stay in Place so several objects can be placed in a row.
}

void WorldEditor::deleteManaged() {
  if (managedIndex_ >= managed_.size()) return;
  const EntityHandle handle = managed_[managedIndex_];
  world_.scene.destroy(handle);
  refreshManaged();
  rebuildPhysics();
  resetBallToCenter();
}

void WorldEditor::applyManagedColor(const Vec3& color) {
  if (managedIndex_ >= managed_.size()) return;
  EntityData* entity = world_.scene.get(managed_[managedIndex_]);
  if (entity == nullptr) return;
  entity->color = color;
  if (objectKindForName(entity->name) == ObjectKind::Player) world_.player.color = color;
  if (objectKindForName(entity->name) == ObjectKind::Ball) world_.ball.color = color;
}

// --- Menu model ---

std::string WorldEditor::menuTitle() const {
  switch (screen_) {
    case Screen::Main:
      return "KIMIA World — منوی اصلی";
    case Screen::Builder:
      return world_.name + " — سازنده";
    case Screen::Catalog:
      return "افزودن جسم: چی اضافه کنیم؟";
    case Screen::AskPlayer:
      return "بازیکن: چه سرعتی؟";
    case Screen::AskBall:
      return "توپ: دقیق باشه یا فانتزی؟";
    case Screen::AskBlock:
      return "بلوک: چه اندازه‌ای؟";
    case Screen::AskWallLen:
      return "دیوار: چه طولی؟";
    case Screen::AskWallAxis:
      return "دیوار: در چه جهتی؟";
    case Screen::AskGoal:
      return "دروازه: چه عرضی؟";
    case Screen::Place:
      return "جای‌گذاری: با جهت‌ها حرکت کن";
    case Screen::Manage:
      return "مدیریت اجسام";
    case Screen::Move:
      return "جابه‌جایی: با جهت‌ها حرکت کن";
    case Screen::ConfirmDelete:
      return "حذف شود؟";
    case Screen::AskColor:
      return "چه رنگی؟";
    case Screen::AskEnvironment:
      return "محیط: چه فضایی؟";
    case Screen::Play:
      return "در حال بازی — با جهت‌ها حرکت کن";
    case Screen::AskModelFile:
      return "مدل از فایل: کدام فایل؟";
    case Screen::AskModelSize:
      return "مدل: چه اندازه‌ای؟";
    default:
      return "گل شد!";
  }
}

std::vector<std::string> WorldEditor::optionLabels() const {
  switch (screen_) {
    case Screen::Main:
      return {"دنیای جدید", "باز کردن دنیا", "خروج"};
    case Screen::Builder:
      return {"افزودن جسم", "مدیریت اجسام", "محیط", "بازی (PLAY)", "ذخیره", "منوی اصلی"};
    case Screen::Catalog:
      return {"بازیکن", "توپ", "بلوک", "دیوار", "دروازه", "جعبه", "مدل از فایل", "بازگشت"};
    case Screen::AskModelFile: {
      std::vector<std::string> labels;
      const usize begin = importPage_ * 5U;
      const usize end = begin + 5U < importFiles_.size() ? begin + 5U : importFiles_.size();
      for (usize i = begin; i < end; ++i) labels.push_back(importFiles_[i]);
      if (end < importFiles_.size()) {
        labels.push_back("بیشتر…");
      } else {
        labels.push_back("بازگشت");
      }
      return labels;
    }
    case Screen::AskModelSize:
      return {"کوچک", "متوسط", "بزرگ", "بازگشت"};
    case Screen::AskPlayer:
      return {"سریع", "عادی", "آرام", "بازگشت"};
    case Screen::AskBall:
      return {"دقیق (مثل گلف)", "فانتزی (پران و لغزنده)", "بازگشت"};
    case Screen::AskBlock:
      return {"کوچک", "متوسط", "بزرگ", "بازگشت"};
    case Screen::AskWallLen:
      return {"کوتاه", "متوسط", "بلند", "بازگشت"};
    case Screen::AskWallAxis:
      return {"در امتداد Z (عمق)", "در امتداد X (عرض)", "بازگشت"};
    case Screen::AskGoal:
      return {"کوچک", "متوسط", "بزرگ", "بازگشت"};
    case Screen::Place:
      return {"قرار دادن", "بازگشت"};
    case Screen::Manage: {
      if (managed_.empty()) return {"بازگشت"};
      return {"قبلی", "بعدی", "جابه‌جایی", "حذف", "رنگ", "بازگشت"};
    }
    case Screen::Move:
      return {"پایان"};
    case Screen::ConfirmDelete:
      return {"بله", "نه"};
    case Screen::AskColor:
      return {"قرمز", "سبز", "آبی", "زرد", "سفید", "بازگشت"};
    case Screen::AskEnvironment:
      return {"چمنزار", "شنی (کویر)", "شب", "بازگشت"};
    default:
      return {};
  }
}

std::vector<std::pair<std::string, std::string>> WorldEditor::holdPad() const {
  if (!playing() && screen_ != Screen::Place && screen_ != Screen::Move) return {};
  if (playing()) {
    return {{"↑", "up"}, {"←", "left"}, {"↓", "down"}, {"→", "right"}};
  }
  return {{"↑", "up"}, {"←", "left"}, {"↓", "down"}, {"→", "right"}, {"ریز", "shift"}};
}

std::vector<std::pair<std::string, std::string>> WorldEditor::tapPad() const {
  if (!playing()) return {};
  return {{"توپ از نو", "r"}, {"منو", "b"}};
}

// --- Option handling (the builder state machine) ---

void WorldEditor::choose(i32 optionIndex) {
  switch (screen_) {
    case Screen::Main: {
      if (optionIndex == 0) {
        createWorld();
      } else if (optionIndex == 1) {
        std::string error;
        if (!loadWorld(worldPath_, error)) lastError_ = error;  // shown in stats
      } else if (optionIndex == 2) {
        quitRequested_ = true;
      }
      break;
    }
    case Screen::Builder: {
      if (optionIndex == 0) {
        screen_ = Screen::Catalog;
      } else if (optionIndex == 1) {
        refreshManaged();
        screen_ = Screen::Manage;
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
    case Screen::Catalog: {
      if (optionIndex == 0) {
        screen_ = Screen::AskPlayer;
      } else if (optionIndex == 1) {
        screen_ = Screen::AskBall;
      } else if (optionIndex == 2) {
        screen_ = Screen::AskBlock;
      } else if (optionIndex == 3) {
        screen_ = Screen::AskWallLen;
      } else if (optionIndex == 4) {
        screen_ = Screen::AskGoal;
      } else if (optionIndex == 5) {
        // جعبه: a dynamic crate, fixed 1x1x1 — no size question.
        pendingKind_ = ObjectKind::Crate;
        pendingSize_ = kWorldCrateSize;
        beginPlace();
      } else if (optionIndex == 6) {
        // مدل از فایل: list the OBJ/FBX files of the import directory.
        refreshImportFiles();
        screen_ = Screen::AskModelFile;
      } else if (optionIndex == 7) {
        screen_ = Screen::Builder;
      }
      break;
    }
    case Screen::AskModelFile: {
      const usize begin = importPage_ * 5U;
      const usize shown = begin + 5U < importFiles_.size() ? 5U : importFiles_.size() - begin;
      if (importFiles_.empty()) {
        screen_ = Screen::Catalog;
        break;
      }
      if (optionIndex >= 0 && static_cast<usize>(optionIndex) < shown) {
        pendingFile_ = importFiles_[begin + static_cast<usize>(optionIndex)];
        importPage_ = 0U;
        screen_ = Screen::AskModelSize;
      } else if (optionIndex == static_cast<i32>(shown)) {
        if (begin + 5U < importFiles_.size()) {
          ++importPage_;
        } else {
          screen_ = Screen::Catalog;
        }
      } else {
        screen_ = Screen::Catalog;
      }
      break;
    }
    case Screen::AskModelSize: {
      if (optionIndex == 0) {
        pendingSize_ = kWorldModelSmall;
      } else if (optionIndex == 1) {
        pendingSize_ = kWorldModelMedium;
      } else if (optionIndex == 2) {
        pendingSize_ = kWorldModelLarge;
      } else {
        screen_ = Screen::AskModelFile;
        break;
      }
      pendingKind_ = ObjectKind::Model;
      beginPlace();
      break;
    }
    case Screen::AskPlayer: {
      if (optionIndex == 0) {
        world_.player.speed = kWorldPlayerFast;
      } else if (optionIndex == 1) {
        world_.player.speed = kWorldPlayerNormal;
      } else if (optionIndex == 2) {
        world_.player.speed = kWorldPlayerSlow;
      } else {
        screen_ = Screen::Catalog;
        break;
      }
      pendingKind_ = ObjectKind::Player;
      beginPlace();
      break;
    }
    case Screen::AskBall: {
      if (optionIndex == 0) {
        applyBallType(world_.ball, BallType::Accurate);
      } else if (optionIndex == 1) {
        applyBallType(world_.ball, BallType::Fantasy);
      } else {
        screen_ = Screen::Catalog;
        break;
      }
      pendingKind_ = ObjectKind::Ball;
      beginPlace();
      break;
    }
    case Screen::AskBlock: {
      if (optionIndex == 0) {
        pendingSize_ = kWorldBlockSmall;
      } else if (optionIndex == 1) {
        pendingSize_ = kWorldBlockMedium;
      } else if (optionIndex == 2) {
        pendingSize_ = kWorldBlockLarge;
      } else {
        screen_ = Screen::Catalog;
        break;
      }
      pendingKind_ = ObjectKind::Block;
      beginPlace();
      break;
    }
    case Screen::AskWallLen: {
      if (optionIndex == 0) {
        pendingSize_ = kWorldWallShort;
      } else if (optionIndex == 1) {
        pendingSize_ = kWorldWallMedium;
      } else if (optionIndex == 2) {
        pendingSize_ = kWorldWallLong;
      } else {
        screen_ = Screen::Catalog;
        break;
      }
      pendingKind_ = ObjectKind::Wall;
      screen_ = Screen::AskWallAxis;
      break;
    }
    case Screen::AskWallAxis: {
      if (optionIndex == 0) {
        pendingAxisZ_ = true;
      } else if (optionIndex == 1) {
        pendingAxisZ_ = false;
      } else {
        screen_ = Screen::AskWallLen;
        break;
      }
      beginPlace();
      break;
    }
    case Screen::AskGoal: {
      if (optionIndex == 0) {
        pendingSize_ = kWorldGoalSmall;
      } else if (optionIndex == 1) {
        pendingSize_ = kWorldGoalMedium;
      } else if (optionIndex == 2) {
        pendingSize_ = kWorldGoalLarge;
      } else {
        screen_ = Screen::Catalog;
        break;
      }
      pendingKind_ = ObjectKind::Goal;
      beginPlace();
      break;
    }
    case Screen::Place: {
      if (optionIndex == 0) {
        confirmPlace();
      } else {
        screen_ = Screen::Builder;
      }
      break;
    }
    case Screen::Manage: {
      if (managed_.empty()) {
        if (optionIndex == 0) screen_ = Screen::Builder;
        break;
      }
      if (optionIndex == 0) {
        managedIndex_ = managedIndex_ == 0U ? managed_.size() - 1U : managedIndex_ - 1U;
      } else if (optionIndex == 1) {
        managedIndex_ = (managedIndex_ + 1U) % managed_.size();
      } else if (optionIndex == 2) {
        screen_ = Screen::Move;
      } else if (optionIndex == 3) {
        screen_ = Screen::ConfirmDelete;
      } else if (optionIndex == 4) {
        screen_ = Screen::AskColor;
      } else if (optionIndex == 5) {
        screen_ = Screen::Builder;
      }
      break;
    }
    case Screen::Move: {
      if (optionIndex == 0) {
        rebuildPhysics();
        screen_ = Screen::Manage;
      }
      break;
    }
    case Screen::ConfirmDelete: {
      if (optionIndex == 0) {
        deleteManaged();
        screen_ = Screen::Manage;
      } else {
        screen_ = Screen::Manage;
      }
      break;
    }
    case Screen::AskColor: {
      if (optionIndex == 0) {
        applyManagedColor(Vec3{0.85, 0.15, 0.15});
      } else if (optionIndex == 1) {
        applyManagedColor(Vec3{0.2, 0.75, 0.3});
      } else if (optionIndex == 2) {
        applyManagedColor(Vec3{0.2, 0.5, 0.9});
      } else if (optionIndex == 3) {
        applyManagedColor(Vec3{0.9, 0.85, 0.3});
      } else if (optionIndex == 4) {
        applyManagedColor(Vec3{0.92, 0.92, 0.92});
      }
      screen_ = Screen::Manage;
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
      if (optionIndex <= 2) {
        applyEnvironmentToScene();
        screen_ = Screen::Builder;
      } else {
        screen_ = Screen::Builder;
      }
      break;
    }
    default:
      break;  // play screens have no options; taps are handled by the app
  }
}

}  // namespace kimia
