#include <kimia/Engine.h>

#ifndef _WIN32
#include <sys/stat.h>
#endif

#include <cstdlib>
#include <vector>

namespace kimia {

Engine::~Engine() { shutdown(); }

bool Engine::initialize(const EngineOptions& options) {
  shutdown();
  options_ = options;
  lastWindowWidth_ = options.windowWidth;
  lastWindowHeight_ = options.windowHeight;

  // Engine handoff conventions on Unix/Termux: a private XDG_RUNTIME_DIR
  // for headless machines (created 0700, never overwritten) and software GL
  // on Android. Windows owns its window/driver setup and does not need these
  // environment variables.
#ifndef _WIN32
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
#endif

  // Runtime GL loading: try the window's proc resolver first (SDL GL), then
  // plain dlopen; then a headless EGL context. Any failure is fine — the
  // software renderer covers it.
  if (!options.headless) {
    window_ = Window::create(options.windowTitle, options.windowWidth, options.windowHeight, options.windowHidden);
    if (window_ != nullptr && !window_->valid()) window_.reset();
  }
  bool wantD3D11 = options.preferD3D11;
#ifdef KIMIA_PRIMARY_RENDERER_D3D11
  wantD3D11 = true;
#elif defined(KIMIA_PRIMARY_RENDERER_OPENGL) || defined(KIMIA_PRIMARY_RENDERER_SOFTWARE)
  wantD3D11 = false;
#endif
  if (wantD3D11 && window_ != nullptr && window_->nativeHandle() != nullptr) {
    D3D11Options d3dOptions;
    d3dOptions.nativeWindow = window_->nativeHandle();
    d3dOptions.width = options.windowWidth;
    d3dOptions.height = options.windowHeight;
    d3dOptions.vsync = true;
    std::string d3dError;
    d3d11_.initialize(d3dOptions, d3dError);
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
    web::ServerOptions serverOptions;
    serverOptions.bindAddress = options.webBindAddress;
    serverOptions.authToken = options.webAuthToken;
    if (!server->start(options.webPort, web::makePageHtml(options.windowTitle, pad, "", ""), serverOptions)) {
      server_.reset();
    }
  }
  return true;
}

void Engine::shutdown() {
  if (server_ != nullptr) server_->stop();
  server_.reset();
  d3d11_.shutdown();
  window_.reset();
  lastWindowWidth_ = 0;
  lastWindowHeight_ = 0;
  input_ = InputState{};
  eglContext_.destroy();
  GLFunctions::instance().unload();
}

bool Engine::poll() {
  bool running = true;
  if (window_ != nullptr) {
    running = window_->poll(input_);
    const i32 windowWidth = window_->width();
    const i32 windowHeight = window_->height();
    if (d3d11_.ready() && (windowWidth != lastWindowWidth_ || windowHeight != lastWindowHeight_)) {
      std::string resizeError;
      if (d3d11_.resize(windowWidth, windowHeight, resizeError)) {
        lastWindowWidth_ = windowWidth;
        lastWindowHeight_ = windowHeight;
      } else {
        // Device loss/invalid resize must not take down the editor. The
        // caller will see D3D11 unavailable and use its software fallback.
        d3d11_.shutdown();
      }
    }
  }
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
