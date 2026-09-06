#pragma once

#include <kimia/World.h>

#include <map>
#include <string>

namespace kimia {
namespace studio {

// --- KIMIA Workbench: the browser-side world editor (stage 32) ---
//
// Deliberately its own thing rather than a copy of anybody else's editor.
// The vocabulary is the workshop, not the film set:
//
//   Bench      the whole editor page
//   Rack       the list of everything in the world  (hierarchy)
//   Dossier    the panel describing one object      (inspector)
//   Fittings   the components bolted onto an object (physics/anim/sound)
//   Labels     the tags used to address groups
//   Wiring     what connects a fitting to a button or a game event
//
// The engine answers questions and takes orders as JSON; the page draws
// them. Every decision stays here where it can be tested, and nothing in
// the HTML knows how the engine works.

// Handles one /api/... request and returns a JSON body. `params` is the
// already-parsed query string. Unknown paths return an {"error": ...}
// object rather than throwing, so a stale page can never wedge the server.
std::string handleApi(WorldEditor& editor, const std::string& path,
                      const std::map<std::string, std::string>& params);

// The Workbench page itself: one self-contained HTML document with no
// external files, so it works offline on a phone exactly as on a desktop.
std::string benchPage();

}  // namespace studio
}  // namespace kimia
