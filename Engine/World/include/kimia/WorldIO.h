#pragma once

#include <kimia/World.h>

#include <string>

namespace kimia {

// World serialization on top of the SceneIO-v1 text format. The file is a
// valid v1 scene: all world metadata lives in `#` comment lines that the v1
// loader ignores, so old .kimia files (and files written by other tools)
// still load — missing metadata simply falls back to defaults.
//
//   # KIMIA scene v1
//   # world name MyWorld
//   # profile name sandbox            <- one ProfileIO body line per `# profile`
//   # profile title زمین آزاد
//   # profile field 20.000000 20.000000
//   # profile environment grass
//   # profile player speed 4.000000 jump 1.200000
//   # profile ball accurate choice on
//   # profile kick 2.000000 0.500000 1.200000
//   # player speed 4.000000 color 0.200000 0.500000 0.900000
//   # ball type accurate
//   # env grass
//   # score 0
//   e "Ground" mesh plane pos 0 0 0 scale 20 1 20 color 0.22 0.45 0.24 rough 0.95
//   ...
//
// A world file is self-contained: it carries a copy of the game profile it
// was made with, so it plays the same even when the profile file changes
// later. Files without `# profile` lines (stage-10..17 worlds) load as the
// sandbox game with the field sized from their ground plane.
class WorldIO {
public:
  static bool save(const WorldData& world, std::string& out);
  static bool saveToFile(const WorldData& world, const std::string& path, std::string& error);
  static bool load(const std::string& text, WorldData& out, std::string& error);
  static bool loadFromFile(const std::string& path, WorldData& out, std::string& error);
};

}  // namespace kimia
