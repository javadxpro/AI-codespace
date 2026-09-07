#pragma once

#include <kimia/D3D11Renderer.h>
#include <kimia/EGL.h>
#include <kimia/GLFunctions.h>
#include <kimia/InputState.h>
#include <kimia/Types.h>
#include <kimia/WebViewer.h>
#include <kimia/Window.h>

#include <memory>
#include <string>

namespace kimia {

struct EngineOptions {
  bool headless = false;    // never create a window
  bool windowHidden = false; // headless/remote callers can hide the native window explicitly
  bool preferD3D11 = false;  // Windows PC path; false keeps legacy GL/software behavior
  bool enableWeb = false;   // start the WebViewer server
  u16 webPort = 8080;
  std::string webBindAddress = "127.0.0.1";
  std::string webAuthToken;
  std::string windowTitle = "KIMIA";
  i32 windowWidth = 640;
  i32 windowHeight = 480;
};

// Bootstrap: environment setup (Termux/XDG conventions from the engine
// handoff), runtime GL loading, headless EGL context, optional window and
// optional WebViewer. Owns all of them; shutdown() releases everything.
class Engine {
public:
  Engine() = default;
  ~Engine();
  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;

  bool initialize(const EngineOptions& options);
  void shutdown();

  // Polls window + web input into input(); returns false when quitting.
  // Call endFrame() after reading the input each frame.
  bool poll();
  void endFrame() { input_.endFrame(); }

  bool glAvailable() const { return GLFunctions::instance().loaded() && eglContext_.valid(); }
  bool d3d11Available() const { return d3d11_.ready(); }
  D3D11Renderer& d3d11() { return d3d11_; }
  InputState& input() { return input_; }
  web::Server* server() { return server_.get(); }
  Window* window() { return window_.get(); }

private:
  EngineOptions options_;
  std::unique_ptr<Window> window_;
  std::unique_ptr<web::Server> server_;
  D3D11Renderer d3d11_;
  EGLContext eglContext_;
  InputState input_;
  i32 lastWindowWidth_ = 0;
  i32 lastWindowHeight_ = 0;
};

}  // namespace kimia
