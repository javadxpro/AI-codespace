#pragma once

#include <kimia/Scene.h>

#include <string>

namespace kimia {

// KIMIA scene text format v1.
//
//   # KIMIA scene v1
//   e "Green" mesh plane pos 0 0 0 scale 1 1 1 color 0.22 0.45 0.24 rough 0.95
//   e "Ball" mesh sphere pos 0 0 0 scale 1 1 1 color 0.95 0.95 0.92 rough 0.3
//   # demo 0.000000 0.610000
//
// Rules:
// - `#` lines are comments, EXCEPT `# demo <aim> <power>` which stores the
//   player-authored demo shot.
// - mesh is one of: cube, plane, sphere.
// - entity names are double-quoted (`\"` and `\\` are the escapes); a bare
//   token is also accepted on load.
// - Unknown keywords/tokens are skipped and partial entity lines are ignored
//   (tolerant load).
class SceneIO {
public:
  static bool save(const Scene& scene, std::string& out);
  static bool saveToFile(const Scene& scene, const std::string& path);

  // Always succeeds for well-formed text; tolerates anything it does not
  // understand. `error` is only set by loadFromFile for IO failures.
  static bool load(const std::string& text, Scene& out, std::string& error);
  static bool loadFromFile(const std::string& path, Scene& out, std::string& error);
};

}  // namespace kimia
