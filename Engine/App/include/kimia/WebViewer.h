#pragma once

#include <kimia/Types.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace kimia {
namespace web {

struct PadButton {
  std::string label;
  std::string key;
  bool hold = false;  // true: level (down/up); false: edge (tap)
};

// A dynamic menu served at GET /menu as JSON. The control page polls it and
// rebuilds its buttons, which is how the option-driven editor presents
// questions. An empty title hides the menu (static pad shown instead).
struct Menu {
  std::string title;
  std::vector<PadButton> holds;
  std::vector<PadButton> taps;
};

// Input drained from the web page. `held` is LEVEL state (server keeps it),
// `taps`/`lookX`/`lookY`/`zoom` are EDGES cleared by drain().
struct DrainedInput {
  std::map<std::string, bool> held;
  std::vector<std::string> taps;
  f64 lookX = 0.0;
  f64 lookY = 0.0;
  f64 zoom = 0.0;
};

// Generates the touch-pad control page served on "/".
std::string makePageHtml(const std::string& title, const std::vector<PadButton>& padButtons,
                         const std::string& keymapJs, const std::string& hint);

// Tiny HTTP server on plain POSIX sockets + threads (no external HTTP lib).
// Routes:
//   GET  /          -> 200 text/html (the control page)
//   GET  /frame.png -> 200 image/png (latest published frame) or 503 if none
//   GET  /stats     -> 200 text/plain (last stats line)
//   GET  /menu      -> 200 application/json (the dynamic menu; empty by default)
//   POST /input?key=<k>&down=0|1&tap=<k>&lookX=<dx>&lookY=<dy>&zoom=<dz> -> 200
//   GET  /sound     -> 200 text/plain "<seq> <name>" (the latest sound cue; seq
//                      grows by one per playSound, so the page plays each cue once)
//   GET  /sfx/<n>   -> 200 audio/wav (a registered sound) or 404
//   GET  /intro.mp4 -> 200 video/mp4 (the intro film) or 404 if none was set
//   GET  /logo.png  -> 200 image/png (the splash logo) or 404 if none was set
//   anything else   -> 404
class Server {
public:
  struct Impl;  // opaque; definition lives in the .cpp

  Server();
  ~Server();
  Server(const Server&) = delete;
  Server& operator=(const Server&) = delete;

  // Binds 0.0.0.0:port (port 0 = ephemeral) and starts the accept thread.
  bool start(u16 port, const std::string& pageHtml);
  u16 port() const;
  bool running() const;
  void stop();

  void publishFrame(std::vector<u8> pngBytes, const std::string& statsLine);
  void setMenu(const Menu& menu);
  DrainedInput drain();

  // Sound: register WAV bytes under a name once, then cue it by name. The
  // page polls /sound and plays /sfx/<name> when the sequence number moves.
  // Branding: the intro film plays full-screen over the page once, before
  // the first frame, and the logo is the poster shown while it loads. Both
  // are optional — without them the page opens straight into the game, so a
  // build with no branding files behaves exactly as it always did.
  void setIntro(std::vector<u8> mp4Bytes, std::vector<u8> logoPngBytes);
  bool hasIntro() const;

  void registerSound(const std::string& name, std::vector<u8> wavBytes);
  void playSound(const std::string& name);
  u64 soundSequence() const;

private:
  std::unique_ptr<Impl> impl_;
};
// Reads the branding files (kimia-intro.mp4 / kimia-logo.png) from a folder
// and hands them to the server. Looks in `folder`, then ./Branding, then
// ../Branding, so it works from a build tree and from a release package
// alike. Returns false when nothing was found — which is not an error.
bool loadIntroFrom(Server& server, const std::string& folder);

}  // namespace web
}  // namespace kimia
