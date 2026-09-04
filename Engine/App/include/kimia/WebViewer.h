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

private:
  std::unique_ptr<Impl> impl_;
};

}  // namespace web
}  // namespace kimia
