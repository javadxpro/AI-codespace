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

KIMIA_TEST(profile_builtins_are_the_four_games_plus_sandbox) {
  // Menu order = build order: golf (learn the engine), street, grass,
  // battleground, then the free sandbox.
  const std::vector<GameProfile> profiles = kimia::builtinProfiles();
  KIMIA_REQUIRE(profiles.size() == 5U);
  KIMIA_REQUIRE(profiles[0].name == "golf");
  KIMIA_REQUIRE(profiles[1].name == "street");
  KIMIA_REQUIRE(profiles[2].name == "grass");
  KIMIA_REQUIRE(profiles[3].name == "battleground");
  KIMIA_REQUIRE(profiles[4].name == "sandbox");

  // Golf: no runner (shot mode), cup scoring, the reference launch numbers.
  const GameProfile& golf = profiles[0];
  KIMIA_REQUIRE(golf.title == "گلف کیمیا");
  KIMIA_REQUIRE(near(golf.fieldLength, 24.0));
  KIMIA_REQUIRE(near(golf.fieldWidth, 10.0));
  KIMIA_REQUIRE(golf.mode == kimia::PlayMode::Shot);
  KIMIA_REQUIRE(golf.scoring == kimia::Scoring::Hole);
  KIMIA_REQUIRE(golf.ballDefault == BallType::Accurate);
  KIMIA_REQUIRE(!golf.ballChoice);
  KIMIA_REQUIRE(near(golf.jumpHeight, 0.0));
  KIMIA_REQUIRE(near(golf.kickBase, kimia::kGolfLaunchBaseSpeed));
  KIMIA_REQUIRE(near(golf.kickSpeedScale, kimia::kGolfLaunchPowerScale));
  KIMIA_REQUIRE(near(golf.kickUp, 0.0));

  const GameProfile& street = profiles[1];
  KIMIA_REQUIRE(street.title == "فوتبال خیابونی ایران: کوی ابوذر");
  KIMIA_REQUIRE(near(street.fieldLength, 16.0));
  KIMIA_REQUIRE(near(street.fieldWidth, 5.0));
  KIMIA_REQUIRE(street.environment == EnvironmentKind::Asphalt);
  KIMIA_REQUIRE(street.ballDefault == BallType::Fantasy);
  KIMIA_REQUIRE(!street.ballChoice);
  KIMIA_REQUIRE(near(street.jumpHeight, 1.8));
  KIMIA_REQUIRE(street.mode == kimia::PlayMode::Kick);
  KIMIA_REQUIRE(street.scoring == kimia::Scoring::Gate);

  const GameProfile& grass = profiles[2];
  KIMIA_REQUIRE(grass.title == "زمین چمن: کوی ابوذر");
  KIMIA_REQUIRE(near(grass.fieldLength, 40.0));
  KIMIA_REQUIRE(near(grass.fieldWidth, 25.0));
  KIMIA_REQUIRE(grass.ballDefault == BallType::Accurate);
  KIMIA_REQUIRE(!grass.ballChoice);

  const GameProfile& battleground = profiles[3];
  KIMIA_REQUIRE(battleground.title == "مسابقه واقعی: بتل گراند");
  KIMIA_REQUIRE(near(battleground.fieldLength, 40.0));
  KIMIA_REQUIRE(near(battleground.fieldWidth, 40.0));

  // The sandbox IS the pre-profile editor: 20 x 20, golf ball, asks the question.
  const GameProfile& sandbox = profiles[4];
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
  KIMIA_REQUIRE(sandbox.mode == kimia::PlayMode::Kick);
  KIMIA_REQUIRE(sandbox.scoring == kimia::Scoring::Gate);
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
    KIMIA_REQUIRE(loaded.mode == profile.mode);
    KIMIA_REQUIRE(loaded.scoring == profile.scoring);
    KIMIA_REQUIRE(ProfileIO::save(loaded) == first);
  }
  // The exact street text (this is also what Profiles/street.kimiaprofile says).
  const std::string street = ProfileIO::save(kimia::builtinProfiles()[1]);
  KIMIA_REQUIRE(street ==
                "# KIMIA profile v1\n"
                "name street\n"
                "title فوتبال خیابونی ایران: کوی ابوذر\n"
                "field 16.000000 5.000000\n"
                "environment asphalt\n"
                "player speed 5.000000 jump 1.800000\n"
                "ball fantasy choice off\n"
                "kick 3.000000 0.600000 2.000000\n"
                "mode kick\n"
                "scoring gate\n"
                "par 3\n"
                "wind 0.000000 0.000000\n"
                "team 5\n"
                "match 300.000000\n"
                "weather 0.000000 0.250000\n"
                "time 17.000000\n"
                "tricks on\n"
                "ai 0.600000\n"
                "camera broadcast\n");
  // And the golf text.
  const std::string golf = ProfileIO::save(kimia::builtinProfiles()[0]);
  KIMIA_REQUIRE(golf ==
                "# KIMIA profile v1\n"
                "name golf\n"
                "title گلف کیمیا\n"
                "field 24.000000 10.000000\n"
                "environment grass\n"
                "player speed 4.000000 jump 0.000000\n"
                "ball accurate choice off\n"
                "kick 2.500000 13.500000 0.000000\n"
                "mode shot\n"
                "scoring hole\n"
                "par 3\n"
                "wind 0.000000 0.000000\n"
                "team 1\n"
                "match 0.000000\n"
                "weather 0.000000 0.000000\n"
                "time 9.000000\n"
                "tricks off\n"
                "ai 0.000000\n"
                "camera chase\n");
}

KIMIA_TEST(profile_shipped_files_match_the_builtins) {
  // Profiles/*.kimiaprofile are the user-editable copies of the built-ins:
  // they must load to exactly the same values (so editing one really is a
  // retune, not a divergence).
  const char* names[] = {"golf", "street", "grass", "battleground"};
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
      "mode flying\n"                      // unknown mode: ignored, stays kick
      "scoring hole\n"
      "par 99.7\n"                         // clamped to 20 (whole strokes)
      "par x\n"                            // not a number: ignored
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
  KIMIA_REQUIRE(loaded.mode == kimia::PlayMode::Kick);
  KIMIA_REQUIRE(loaded.scoring == kimia::Scoring::Hole);
  KIMIA_REQUIRE(loaded.par == 20U);
  GameProfile lowPar;
  KIMIA_REQUIRE(ProfileIO::load("name p\npar 0\n", lowPar, error));
  KIMIA_REQUIRE(lowPar.par == 1U);  // at least one stroke
  GameProfile noPar;
  KIMIA_REQUIRE(ProfileIO::load("name q\n", noPar, error));
  KIMIA_REQUIRE(noPar.par == 3U);   // default
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
  KIMIA_REQUIRE(kimia::loadProfiles(dir + "/does_not_exist").size() == 5U);
  // Empty directory -> exactly the built-ins.
  KIMIA_REQUIRE(kimia::loadProfiles(dir).size() == 5U);

  // z_ sorts last, a_ sorts first: order is by filename, built-ins stay in front.
  writeText(dir + "/z_extra.kimiaprofile", "name zeta\ntitle زتا\nfield 8 8\n");
  writeText(dir + "/a_extra.KIMIAPROFILE", "name alpha\ntitle آلفا\n");  // extension is case-insensitive
  writeText(dir + "/grass_retune.kimiaprofile", "name grass\ntitle چمن من\nfield 30 20\nkick 9 1 1\n");
  writeText(dir + "/broken.kimiaprofile", "title no name here\n");  // skipped
  writeText(dir + "/notes.txt", "name ignored\n");                  // wrong extension
  const std::vector<GameProfile> profiles = kimia::loadProfiles(dir);
  KIMIA_REQUIRE(profiles.size() == 7U);
  KIMIA_REQUIRE(profiles[0].name == "golf");
  KIMIA_REQUIRE(profiles[1].name == "street");
  KIMIA_REQUIRE(profiles[2].name == "grass");
  KIMIA_REQUIRE(profiles[2].title == "چمن من");  // replaced in place
  KIMIA_REQUIRE(near(profiles[2].fieldLength, 30.0));
  KIMIA_REQUIRE(near(profiles[2].kickBase, 9.0));
  KIMIA_REQUIRE(profiles[2].environment == EnvironmentKind::Grass);  // unspecified keys: struct defaults
  KIMIA_REQUIRE(profiles[3].name == "battleground");
  KIMIA_REQUIRE(profiles[4].name == "sandbox");
  KIMIA_REQUIRE(profiles[5].name == "alpha");
  KIMIA_REQUIRE(profiles[6].name == "zeta");
  KIMIA_REQUIRE(near(profiles[6].fieldWidth, 8.0));
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
  kimia::PlayMode mode = kimia::PlayMode::Kick;
  KIMIA_REQUIRE(kimia::playModeFromName("shot", mode) && mode == kimia::PlayMode::Shot);
  KIMIA_REQUIRE(kimia::playModeFromName("kick", mode) && mode == kimia::PlayMode::Kick);
  KIMIA_REQUIRE(!kimia::playModeFromName("fly", mode));
  KIMIA_REQUIRE(std::string(kimia::playModeName(kimia::PlayMode::Shot)) == "shot");
  kimia::Scoring scoring = kimia::Scoring::Gate;
  KIMIA_REQUIRE(kimia::scoringFromName("hole", scoring) && scoring == kimia::Scoring::Hole);
  KIMIA_REQUIRE(kimia::scoringFromName("gate", scoring) && scoring == kimia::Scoring::Gate);
  KIMIA_REQUIRE(!kimia::scoringFromName("points", scoring));
  KIMIA_REQUIRE(std::string(kimia::scoringName(kimia::Scoring::Hole)) == "hole");
}

// --- Wind (stage 20.5-b2) ---

KIMIA_TEST(profile_wind_round_trips_clamps_and_defaults_to_calm) {
  // Every shipped game is calm: wind is opt-in, so no existing game changed.
  for (const GameProfile& profile : kimia::builtinProfiles()) {
    KIMIA_REQUIRE(near(profile.windSpeed, 0.0));
    KIMIA_REQUIRE(near(profile.windDirection, 0.0));
  }
  // A windy retune loads exactly and survives save -> load -> save.
  GameProfile windy;
  std::string error;
  KIMIA_REQUIRE(ProfileIO::load("# KIMIA profile v1\nname windy\nwind 3.500000 1.570796\n", windy, error));
  KIMIA_REQUIRE(near(windy.windSpeed, 3.5));
  KIMIA_REQUIRE(near(windy.windDirection, 1.570796));
  const std::string once = ProfileIO::save(windy);
  GameProfile again;
  KIMIA_REQUIRE(ProfileIO::load(once, again, error));
  KIMIA_REQUIRE(ProfileIO::save(again) == once);
  // Tolerant like every other key: out of range clamps, a half line is
  // ignored as a whole, an unparsable value changes nothing.
  GameProfile clamped;
  KIMIA_REQUIRE(ProfileIO::load("# KIMIA profile v1\nname w\nwind 999 0\n", clamped, error));
  KIMIA_REQUIRE(near(clamped.windSpeed, kimia::kProfileWindMax));
  GameProfile negative;
  KIMIA_REQUIRE(ProfileIO::load("# KIMIA profile v1\nname w\nwind -4 0\n", negative, error));
  KIMIA_REQUIRE(near(negative.windSpeed, 0.0));
  GameProfile partial;
  KIMIA_REQUIRE(ProfileIO::load("# KIMIA profile v1\nname w\nwind 5\nwind x y\n", partial, error));
  KIMIA_REQUIRE(near(partial.windSpeed, 0.0));      // both lines refused whole
  KIMIA_REQUIRE(near(partial.windDirection, 0.0));
}

// --- Offline release packaging (stage 21) ---

KIMIA_TEST(profile_shipped_directory_loads_every_game_for_a_release) {
  // What a release package ships in profiles/ must all load: a profile file
  // that is silently dropped would ship a game the player cannot pick.
  const std::vector<GameProfile> fromDir = kimia::loadProfiles(KIMIA_PROFILE_DIR);
  const std::vector<GameProfile> builtins = kimia::builtinProfiles();
  // Every shipped file overrides a built-in (it never appends an unknown
  // game), so the menu keeps exactly the built-in count and order.
  KIMIA_REQUIRE(fromDir.size() == builtins.size());
  for (kimia::usize i = 0; i < builtins.size(); ++i) {
    KIMIA_REQUIRE(fromDir[i].name == builtins[i].name);
  }
  // The golf profile carries commented-out documentation (the wind recipe)
  // after its last real key: trailing comments must not change a value.
  const GameProfile* golf = findProfile(fromDir, "golf");
  KIMIA_REQUIRE(golf != nullptr);
  KIMIA_REQUIRE(golf->mode == kimia::PlayMode::Shot);
  KIMIA_REQUIRE(golf->scoring == kimia::Scoring::Hole);
  KIMIA_REQUIRE(near(golf->windSpeed, 0.0));  // the wind line is a comment
  KIMIA_REQUIRE(ProfileIO::save(*golf) == ProfileIO::save(*findProfile(builtins, "golf")));
}

// --- Stage 24: weather and the clock ---

KIMIA_TEST(profile_weather_and_time_round_trip_and_clamp) {
  GameProfile out;
  std::string error;
  KIMIA_REQUIRE(ProfileIO::load("# KIMIA profile v1\nname x\nweather 0.4 0.7\ntime 21.25\n", out, error));
  KIMIA_REQUIRE(near(out.rain, 0.4));
  KIMIA_REQUIRE(near(out.wetness, 0.7));
  KIMIA_REQUIRE(near(out.hour, 21.25));
  // Both values clamp into range.
  KIMIA_REQUIRE(ProfileIO::load("# KIMIA profile v1\nname x\nweather 9 -3\ntime 99\n", out, error));
  KIMIA_REQUIRE(near(out.rain, 1.0));
  KIMIA_REQUIRE(near(out.wetness, 0.0));
  KIMIA_REQUIRE(near(out.hour, 24.0));
  // A half-written weather line is ignored whole, never half applied.
  GameProfile partial;
  KIMIA_REQUIRE(ProfileIO::load("# KIMIA profile v1\nname x\nweather 0.5\n", partial, error));
  KIMIA_REQUIRE(near(partial.rain, 0.0));
  KIMIA_REQUIRE(near(partial.wetness, 0.0));
  // Defaults with no lines at all: dry, and midday.
  GameProfile plain;
  KIMIA_REQUIRE(ProfileIO::load("# KIMIA profile v1\nname x\n", plain, error));
  KIMIA_REQUIRE(near(plain.rain, 0.0));
  KIMIA_REQUIRE(near(plain.wetness, 0.0));
  KIMIA_REQUIRE(near(plain.hour, 12.0));
}

KIMIA_TEST(profile_each_game_has_its_own_weather) {
  const std::vector<GameProfile> games = kimia::builtinProfiles();
  // golf: a calm dry morning round.
  KIMIA_REQUIRE(near(games[0].hour, 9.0));
  KIMIA_REQUIRE(near(games[0].rain, 0.0));
  KIMIA_REQUIRE(near(games[0].wetness, 0.0));
  // street: after school, alley still damp.
  KIMIA_REQUIRE(near(games[1].hour, 17.0));
  KIMIA_REQUIRE(near(games[1].rain, 0.0));
  KIMIA_REQUIRE(near(games[1].wetness, 0.25));
  // grass: a floodlit evening fixture in drizzle.
  KIMIA_REQUIRE(near(games[2].hour, 19.5));
  KIMIA_REQUIRE(near(games[2].rain, 0.35));
  KIMIA_REQUIRE(near(games[2].wetness, 0.5));
  // battleground: a dusk raid.
  KIMIA_REQUIRE(near(games[3].hour, 20.5));
}
