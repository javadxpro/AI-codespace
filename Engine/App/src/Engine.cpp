#include <kimia/Engine.h>

#include <sys/stat.h>

#include <cstdlib>
#include <vector>

namespace kimia {

Engine::~Engine() { shutdown(); }

bool Engine::initialize(const EngineOptions& options) {
  shutdown();
  options_ = options;

  // Engine handoff conventions: a private XDG_RUNTIME_DIR for headless
  // machines (created 0700, never overwritten) and software GL on Android
  // (never overwritten either).
  if (std::getenv("XDG_RUNTIME_DIR") == nullptr) {
    static const char* kRuntimeDir = "/tmp/kimia-xdg";
    ::mkdir(kRuntimeDir, 0700);
    ::setenv("XDG_RUNTIME_DIR", kRuntimeDir, 1);
  }
#ifdef __ANDROID__
  if (std::getenv("LIBGL_ALWAYS_SOFTWARE") == nullptr) {
    ::setenv("LIBGL_ALWAYS_SOFTWARE", "1", 1);
  }
#endif

  // Runtime GL loading: try the window's proc resolver first (SDL GL), then
  // plain dlopen; then a headless EGL context. Any failure is fine — the
  // software renderer covers it.
  if (!options.headless) {
    window_ = Window::create(options.windowTitle, options.windowWidth, options.windowHeight, true);
    if (window_ != nullptr && !window_->valid()) window_.reset();
  }
  if (!GLFunctions::instance().loaded()) {
    GLFunctions::instance().load();
  }
  if (GLFunctions::instance().loaded() && !eglContext_.valid()) {
    eglContext_.create(options.windowWidth, options.windowHeight);
  }
  if (options.enableWeb) {
    server_ = std::unique_ptr<web::Server>(new web::Server());
    std::vector<web::PadButton> pad;
    web::Server* server = server_.get();
    if (!server->start(options.webPort, web::makePageHtml(options.windowTitle, pad, "", ""))) {
      server_.reset();
    }
  }
  return true;
}

void Engine::shutdown() {
  if (server_ != nullptr) server_->stop();
  server_.reset();
  window_.reset();
  eglContext_.destroy();
  GLFunctions::instance().unload();
}

bool Engine::poll() {
  bool running = true;
  if (window_ != nullptr) running = window_->poll(input_);
  if (server_ != nullptr) {
    const web::DrainedInput drained = server_->drain();
    for (const auto& [name, isDown] : drained.held) {
      const auto key = keyFromName(name);
      if (key.has_value()) input_.setKeyDown(*key, isDown);
    }
    for (const std::string& tap : drained.taps) {
      const auto key = keyFromName(tap);
      if (key.has_value()) input_.tap(*key);
    }
    input_.lookX += drained.lookX;
    input_.lookY += drained.lookY;
    input_.zoom += drained.zoom;
  }
  return running;
}

}  // namespace kimia
