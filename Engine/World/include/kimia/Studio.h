#pragma once

#include <kimia/World.h>

#include <map>
#include <string>

namespace kimia {
namespace studio {

// --- KIMIA Editor: the browser-side world editor (stage 32) ---
//
// A Unity-style layout with Unity terms, written from scratch for this
// engine (no Unity code, assets or stylesheets):
//
//   Hierarchy   the list of everything in the world (left)
//   Inspector   the panel describing one object     (middle)
//   Game        the live play view                  (right, large)
//   Project     files, prefabs and scenes           (bottom)
//   Console     what the editor just did            (bottom)
//
// The older /api/rack and /api/dossier routes keep answering under their
// names: the page above is new, but no route was renamed or removed. The
// engine answers questions and takes orders as JSON; the page draws them.
// Every decision stays here where it can be tested, and nothing in the
// HTML knows how the engine works.

// Handles one /api/... request and returns a JSON body. `params` is the
// already-parsed query string. Unknown paths return an {"error": ...}
// object rather than throwing, so a stale page can never wedge the server.
std::string handleApi(WorldEditor& editor, const std::string& path,
                      const std::map<std::string, std::string>& params);

// Writes an uploaded file into the import folder (the Project panel's
// Upload button posts the raw bytes; the server hands them over here).
// Returns a JSON body like handleApi. The name must be a bare file name;
// anything with a folder in it is refused, so a browser can never write
// outside the assets.
std::string saveAssetFile(WorldEditor& editor, const std::map<std::string, std::string>& params,
                          const std::string& bytes);

// The Workbench page itself: one self-contained HTML document with no
// external files, so it works offline on a phone exactly as on a desktop.
std::string benchPage();

}  // namespace studio
}  // namespace kimia
