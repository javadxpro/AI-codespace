#include <kimia_test.h>
#include <kimia/GameProfile.h>
#include <kimia/Golf.h>

#include <dirent.h>
#include <sys/stat.h>

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <string>

namespace {

using kimia::BallType;
using kimia::EnvironmentKind;
using kimia::GameProfile;
using kimia::ProfileIO;
using kimia::f64;
using kimia::usize;

bool near(f64 a, f64 b, f64 eps = 1e-9) { return std::abs(a - b) <= eps; }

// A fresh, EMPTY directory under the test tmp root (a previous run may have
// left files behind: they are removed so every run starts from the same state).
std::string tmpDir(const std::string& name) {
  const int base = ::mkdir(KIMIA_TEST_TMP, 0755);
  static_cast<void>(base == 0 || errno == EEXIST);
  const std::string dir = std::string(KIMIA_TEST_TMP) + "/" + name;
  if (DIR* handle = ::opendir(dir.c_str())) {
    while (dirent* entry = ::readdir(handle)) {
      const std::string file = entry->d_name;
      if (file == "." || file == "..") continue;
      std::remove((dir + "/" + file).c_str());
    }
    ::closedir(handle);
  }
  const int created = ::mkdir(dir.c_str(), 0755);
  static_cast<void>(created == 0 || errno == EEXIST);
  return dir;
}

void writeText(const std::string& path, const std::string& text) {
  std::FILE* file = std::fopen(path.c_str(), "wb");
  KIMIA_REQUIRE(file != nullptr);
  std::fwrite(text.data(), 1U, text.size(), file);
  std::fclose(file);
}

const GameProfile* findProfile(const std::vector<GameProfile>& profiles, const char* name) {
  for (const GameProfile& profile : profiles) {
    if (profile.name == name) return &profile;
  }
  return nullptr;
}

}  // namespace

KIMIA_TEST(profile_builtins_are_the_three_games_plus_sandbox) {
  const std::vector<GameProfile> profiles = kimia::builtinProfiles();
  KIMIA_REQUIRE(profiles.size() == 4U);
  KIMIA_REQUIRE(profiles[0].name == "street");
  KIMIA_REQUIRE(profiles[1].name == "grass");
  KIMIA_REQUIRE(profiles[2].name == "battleground");
  KIMIA_REQUIRE(profiles[3].name == "sandbox");

  const GameProfile& street = profiles[0];
  KIMIA_REQUIRE(street.title == "فوتبال خیابونی ایران: کوی ابوذر");
  KIMIA_REQUIRE(near(street.fieldLength, 16.0));
  KIMIA_REQUIRE(near(street.fieldWidth, 5.0));
  KIMIA_REQUIRE(street.environment == EnvironmentKind::Asphalt);
  KIMIA_REQUIRE(street.ballDefault == BallType::Fantasy);
  KIMIA_REQUIRE(!street.ballChoice);
  KIMIA_REQUIRE(near(street.jumpHeight, 1.8));

  const GameProfile& grass = profiles[1];
  KIMIA_REQUIRE(grass.title == "زمین چمن: کوی ابوذر");
  KIMIA_REQUIRE(near(grass.fieldLength, 40.0));
  KIMIA_REQUIRE(near(grass.fieldWidth, 25.0));
  KIMIA_REQUIRE(grass.ballDefault == BallType::Accurate);
  KIMIA_REQUIRE(!grass.ballChoice);

  const GameProfile& battleground = profiles[2];
  KIMIA_REQUIRE(battleground.title == "مسابقه واقعی: بتل گراند");
  KIMIA_REQUIRE(near(battleground.fieldLength, 40.0));
  KIMIA_REQUIRE(near(battleground.fieldWidth, 40.0));

  // The sandbox IS the pre-profile editor: 20 x 20, golf ball, asks the question.
  const GameProfile& sandbox = profiles[3];
  KIMIA_REQUIRE(sandbox.title == "زمین آزاد");
  KIMIA_REQUIRE(near(sandbox.fieldLength, 20.0));
  KIMIA_REQUIRE(near(sandbox.fieldWidth, 20.0));
  KIMIA_REQUIRE(near(sandbox.halfLength(), kimia::kWorldFloorHalf));
  KIMIA_REQUIRE(sandbox.environment == EnvironmentKind::Grass);
  KIMIA_REQUIRE(near(sandbox.playerSpeed, kimia::kWorldPlayerNormal));
  KIMIA_REQUIRE(near(sandbox.jumpHeight, kimia::kWorldJumpHeight));
  KIMIA_REQUIRE(sandbox.ballDefault == BallType::Accurate);
  KIMIA_REQUIRE(sandbox.ballChoice);
  KIMIA_REQUIRE(near(sandbox.kickBase, kimia::kWorldKickBase));
  KIMIA_REQUIRE(near(sandbox.kickSpeedScale, kimia::kWorldKickSpeedScale));
  KIMIA_REQUIRE(near(sandbox.kickUp, kimia::kWorldKickUp));
}

KIMIA_TEST(profile_accurate_ball_is_the_golf_tuning) {
  // The world module no longer links golf; this pins the numbers together.
  KIMIA_REQUIRE(near(kimia::kWorldAccurateRadius, kimia::kGolfBallRadius));
  KIMIA_REQUIRE(near(kimia::kWorldAccurateRestitution, kimia::kGolfBallRestitution));
  KIMIA_REQUIRE(near(kimia::kWorldAccurateFriction, kimia::kGolfBallFriction));
  KIMIA_REQUIRE(near(kimia::kWorldAccurateRollingFriction, kimia::kGolfBallRollingFriction));
}

KIMIA_TEST(profile_save_load_save_is_byte_identical) {
  for (const GameProfile& profile : kimia::builtinProfiles()) {
    const std::string first = ProfileIO::save(profile);
    KIMIA_REQUIRE(first.rfind("# KIMIA profile v1\n", 0) == 0);
    GameProfile loaded;
    std::string error;
    KIMIA_REQUIRE(ProfileIO::load(first, loaded, error));
    KIMIA_REQUIRE(loaded.name == profile.name);
    KIMIA_REQUIRE(loaded.title == profile.title);
    KIMIA_REQUIRE(near(loaded.fieldLength, profile.fieldLength));
    KIMIA_REQUIRE(near(loaded.fieldWidth, profile.fieldWidth));
    KIMIA_REQUIRE(loaded.environment == profile.environment);
    KIMIA_REQUIRE(near(loaded.playerSpeed, profile.playerSpeed));
    KIMIA_REQUIRE(near(loaded.jumpHeight, profile.jumpHeight));
    KIMIA_REQUIRE(loaded.ballDefault == profile.ballDefault);
    KIMIA_REQUIRE(loaded.ballChoice == profile.ballChoice);
    KIMIA_REQUIRE(near(loaded.kickBase, profile.kickBase));
    KIMIA_REQUIRE(near(loaded.kickSpeedScale, profile.kickSpeedScale));
    KIMIA_REQUIRE(near(loaded.kickUp, profile.kickUp));
    KIMIA_REQUIRE(ProfileIO::save(loaded) == first);
  }
  // The exact street text (this is also what Profiles/street.kimiaprofile says).
  const std::string street = ProfileIO::save(kimia::builtinProfiles()[0]);
  KIMIA_REQUIRE(street ==
                "# KIMIA profile v1\n"
                "name street\n"
                "title فوتبال خیابونی ایران: کوی ابوذر\n"
                "field 16.000000 5.000000\n"
                "environment asphalt\n"
                "player speed 5.000000 jump 1.800000\n"
                "ball fantasy choice off\n"
                "kick 3.000000 0.600000 2.000000\n");
}

KIMIA_TEST(profile_shipped_files_match_the_builtins) {
  // Profiles/*.kimiaprofile are the user-editable copies of the built-ins:
  // they must load to exactly the same values (so editing one really is a
  // retune, not a divergence).
  const char* names[] = {"street", "grass", "battleground"};
  const std::vector<GameProfile> builtins = kimia::builtinProfiles();
  for (const char* name : names) {
    GameProfile fromFile;
    std::string error;
    KIMIA_REQUIRE(ProfileIO::loadFromFile(std::string(KIMIA_PROFILE_DIR) + "/" + name + ".kimiaprofile", fromFile,
                                          error));
    const GameProfile* builtin = findProfile(builtins, name);
    KIMIA_REQUIRE(builtin != nullptr);
    KIMIA_REQUIRE(ProfileIO::save(fromFile) == ProfileIO::save(*builtin));
  }
}

KIMIA_TEST(profile_load_is_tolerant_and_clamps) {
  const std::string text =
      "# KIMIA profile v1\r\n"
      "name my_game\r\n"
      "title  بازی من \\n خط دوم\r\n"
      "field 200 1\n"                      // clamped to [4, 80]
      "environment moon\n"                 // unknown value: ignored, stays grass
      "player speed 99 jump -2\n"          // clamped: speed 20, jump 0
      "ball fantasy choice on\n"
      "ball accurate\n"                    // incomplete: ignored as a whole
      "kick 1 2\n"                         // incomplete: ignored as a whole
      "gravity 3.7\n"                      // unknown key
      "\n"
      "kick 5 0.25 1.5\n";
  GameProfile loaded;
  std::string error;
  KIMIA_REQUIRE(ProfileIO::load(text, loaded, error));
  KIMIA_REQUIRE(loaded.name == "my_game");
  KIMIA_REQUIRE(loaded.title == " بازی من \n خط دوم");  // escape decoded, CR stripped
  KIMIA_REQUIRE(near(loaded.fieldLength, kimia::kProfileFieldMax));
  KIMIA_REQUIRE(near(loaded.fieldWidth, kimia::kProfileFieldMin));
  KIMIA_REQUIRE(loaded.environment == EnvironmentKind::Grass);
  KIMIA_REQUIRE(near(loaded.playerSpeed, 20.0));
  KIMIA_REQUIRE(near(loaded.jumpHeight, 0.0));
  KIMIA_REQUIRE(loaded.ballDefault == BallType::Fantasy);
  KIMIA_REQUIRE(loaded.ballChoice);
  KIMIA_REQUIRE(near(loaded.kickBase, 5.0));
  KIMIA_REQUIRE(near(loaded.kickSpeedScale, 0.25));
  KIMIA_REQUIRE(near(loaded.kickUp, 1.5));
  // The title round-trips through the escape.
  GameProfile again;
  KIMIA_REQUIRE(ProfileIO::load(ProfileIO::save(loaded), again, error));
  KIMIA_REQUIRE(again.title == loaded.title);

  // No name -> not a profile.
  GameProfile unnamed;
  KIMIA_REQUIRE(!ProfileIO::load("# KIMIA profile v1\ntitle x\n", unnamed, error));
  KIMIA_REQUIRE(!error.empty());
  // A name with spaces/unicode is not an identifier -> rejected line -> no name.
  KIMIA_REQUIRE(!ProfileIO::load("name بازی\n", unnamed, error));
  KIMIA_REQUIRE(!ProfileIO::parseLine("name two words", unnamed));
  KIMIA_REQUIRE(ProfileIO::parseLine("name ok-name_2", unnamed));
}

KIMIA_TEST(profile_directory_overrides_and_extends_builtins) {
  const std::string dir = tmpDir("profile_dir");
  // Missing directory -> exactly the built-ins.
  KIMIA_REQUIRE(kimia::loadProfiles(dir + "/does_not_exist").size() == 4U);
  // Empty directory -> exactly the built-ins.
  KIMIA_REQUIRE(kimia::loadProfiles(dir).size() == 4U);

  // z_ sorts last, a_ sorts first: order is by filename, built-ins stay in front.
  writeText(dir + "/z_extra.kimiaprofile", "name zeta\ntitle زتا\nfield 8 8\n");
  writeText(dir + "/a_extra.KIMIAPROFILE", "name alpha\ntitle آلفا\n");  // extension is case-insensitive
  writeText(dir + "/grass_retune.kimiaprofile", "name grass\ntitle چمن من\nfield 30 20\nkick 9 1 1\n");
  writeText(dir + "/broken.kimiaprofile", "title no name here\n");  // skipped
  writeText(dir + "/notes.txt", "name ignored\n");                  // wrong extension
  const std::vector<GameProfile> profiles = kimia::loadProfiles(dir);
  KIMIA_REQUIRE(profiles.size() == 6U);
  KIMIA_REQUIRE(profiles[0].name == "street");
  KIMIA_REQUIRE(profiles[1].name == "grass");
  KIMIA_REQUIRE(profiles[1].title == "چمن من");  // replaced in place
  KIMIA_REQUIRE(near(profiles[1].fieldLength, 30.0));
  KIMIA_REQUIRE(near(profiles[1].kickBase, 9.0));
  KIMIA_REQUIRE(profiles[1].environment == EnvironmentKind::Grass);  // unspecified keys: struct defaults
  KIMIA_REQUIRE(profiles[2].name == "battleground");
  KIMIA_REQUIRE(profiles[3].name == "sandbox");
  KIMIA_REQUIRE(profiles[4].name == "alpha");
  KIMIA_REQUIRE(profiles[5].name == "zeta");
  KIMIA_REQUIRE(near(profiles[5].fieldWidth, 8.0));
}

KIMIA_TEST(profile_enum_names_round_trip) {
  BallType ball = BallType::Accurate;
  KIMIA_REQUIRE(kimia::ballTypeFromName("fantasy", ball) && ball == BallType::Fantasy);
  KIMIA_REQUIRE(kimia::ballTypeFromName("accurate", ball) && ball == BallType::Accurate);
  KIMIA_REQUIRE(!kimia::ballTypeFromName("magic", ball));
  KIMIA_REQUIRE(std::string(kimia::ballTypeName(BallType::Fantasy)) == "fantasy");
  const EnvironmentKind kinds[] = {EnvironmentKind::Grass, EnvironmentKind::Sand, EnvironmentKind::Night,
                                   EnvironmentKind::Asphalt};
  for (const EnvironmentKind kind : kinds) {
    EnvironmentKind parsed = EnvironmentKind::Grass;
    KIMIA_REQUIRE(kimia::environmentFromName(kimia::environmentName(kind), parsed));
    KIMIA_REQUIRE(parsed == kind);
  }
  EnvironmentKind unknown = EnvironmentKind::Night;
  KIMIA_REQUIRE(!kimia::environmentFromName("lava", unknown));
  KIMIA_REQUIRE(unknown == EnvironmentKind::Night);  // untouched on failure
}
