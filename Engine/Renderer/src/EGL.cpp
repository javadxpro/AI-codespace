#include <kimia/EGL.h>

#if defined(__EMSCRIPTEN__)

#include <emscripten/html5.h>

namespace kimia {

// On the web there is no libEGL.so to dlopen: the context is a WebGL2
// context on the page's canvas, created through the Emscripten HTML5 API.
// The engine's GLFunctions already wire the GLES3 entry points directly, so
// this is the only platform-specific context step.
EGLContext::~EGLContext() { destroy(); }

bool EGLContext::create(i32 width, i32 height) {
  destroy();
  // The page's shell (Web/webgl-shell.html) owns the canvas size and keeps
  // it matched to the window and device-pixel ratio; the render loop reads
  // the live size every frame.
  static_cast<void>(width);
  static_cast<void>(height);
  EmscriptenWebGLContextAttributes attrs;
  emscripten_webgl_init_context_attributes(&attrs);
  attrs.majorVersion = 2;  // WebGL2: GLES3 core, the engine's GL 3.3 feature set
  attrs.depth = true;
  attrs.stencil = false;
  attrs.antialias = false;
  attrs.alpha = false;
  const EMSCRIPTEN_WEBGL_CONTEXT_HANDLE handle = emscripten_webgl_create_context("canvas", &attrs);
  if (handle <= 0) return false;
  if (emscripten_webgl_make_context_current(handle) != EMSCRIPTEN_RESULT_SUCCESS) {
    emscripten_webgl_destroy_context(handle);
    return false;
  }
  context_ = reinterpret_cast<void*>(static_cast<i64>(handle));
  valid_ = true;
  return true;
}

bool EGLContext::createWindow(void*, i32, i32, bool) { return false; }
bool EGLContext::swapBuffers() { return false; }

void EGLContext::destroy() {
  if (context_ != nullptr) {
    emscripten_webgl_destroy_context(reinterpret_cast<EMSCRIPTEN_WEBGL_CONTEXT_HANDLE>(context_));
  }
  library_ = nullptr;
  display_ = nullptr;
  surface_ = nullptr;
  context_ = nullptr;
  valid_ = false;
}

}  // namespace kimia

#elif defined(_WIN32)

namespace kimia {

EGLContext::~EGLContext() { destroy(); }
bool EGLContext::create(i32, i32) { return false; }
bool EGLContext::createWindow(void*, i32, i32, bool) { return false; }
bool EGLContext::swapBuffers() { return false; }
void EGLContext::destroy() {
  library_ = nullptr;
  display_ = nullptr;
  surface_ = nullptr;
  context_ = nullptr;
  valid_ = false;
}

}  // namespace kimia

#else

#include <dlfcn.h>

#include <vector>

namespace kimia {

namespace {

// EGL constants (defined locally: no build-time dependency on EGL headers).
constexpr i32 kDefaultDisplay = 0;
constexpr i32 kPbufferBit = 0x0001;
constexpr i32 kWindowBit = 0x0004;
constexpr i32 kOpenGLBit = 0x0008;
constexpr i32 kRenderableType = 0x3040;
constexpr i32 kSurfaceType = 0x3033;
constexpr i32 kRedSize = 0x3024;
constexpr i32 kGreenSize = 0x3023;
constexpr i32 kBlueSize = 0x3022;
constexpr i32 kDepthSize = 0x3025;
constexpr i32 kNone = 0x3038;
constexpr i32 kOpenGLApi = 0x30A2;
constexpr i32 kOpenGLEsApi = 0x30A0;
constexpr i32 kContextMajorVersion = 0x3098;
constexpr i32 kContextMinorVersion = 0x3097;
constexpr i32 kWidth = 0x3057;
constexpr i32 kHeight = 0x3056;
constexpr i32 kSampleBuffers = 0x3031;
constexpr i32 kSamples = 0x3032;

using EglDisplay = void*;
using EglConfig = void*;
using EglContextHandle = void*;
using EglSurface = void*;
using EglBoolean = i32;
using EglInt = i32;

using PFNGetDisplay = EglDisplay (*)(EglInt);
using PFNInitialize = EglBoolean (*)(EglDisplay, EglInt*, EglInt*);
using PFNBindAPI = EglBoolean (*)(EglInt);
using PFNChooseConfig = EglBoolean (*)(EglDisplay, const EglInt*, EglConfig*, EglInt, EglInt*);
using PFNCreateContext = EglContextHandle (*)(EglDisplay, EglConfig, EglContextHandle, const EglInt*);
using PFNCreatePbufferSurface = EglSurface (*)(EglDisplay, EglConfig, const EglInt*);
using PFNCreateWindowSurface = EglSurface (*)(EglDisplay, EglConfig, void*, const EglInt*);
using PFNMakeCurrent = EglBoolean (*)(EglDisplay, EglSurface, EglSurface, EglContextHandle);
using PFNSwapBuffers = EglBoolean (*)(EglDisplay, EglSurface);
using PFNDestroyContext = EglBoolean (*)(EglDisplay, EglContextHandle);
using PFNDestroySurface = EglBoolean (*)(EglDisplay, EglSurface);
using PFNTerminate = EglBoolean (*)(EglDisplay);

// Everything resolved from one dlopen'd libEGL.
struct EglApi {
  PFNGetDisplay getDisplay = nullptr;
  PFNInitialize initialize = nullptr;
  PFNBindAPI bindAPI = nullptr;
  PFNChooseConfig chooseConfig = nullptr;
  PFNCreateContext createContext = nullptr;
  PFNCreatePbufferSurface createPbuffer = nullptr;
  PFNCreateWindowSurface createWindowSurface = nullptr;
  PFNMakeCurrent makeCurrent = nullptr;
  PFNSwapBuffers swapBuffers = nullptr;
  PFNDestroyContext destroyContext = nullptr;
  PFNDestroySurface destroySurface = nullptr;
  PFNTerminate terminate = nullptr;
};

// dlopens the platform EGL and resolves the shared entry points, or returns
// null. Android's bionic ships libEGL.so with no .1 suffix; Mesa on Linux
// has both, so the .1 name is tried first exactly as before.
void* openEgl(EglApi& api) {
  void* library = dlopen("libEGL.so.1", RTLD_NOW | RTLD_LOCAL);
  if (library == nullptr) library = dlopen("libEGL.so", RTLD_NOW | RTLD_LOCAL);
  if (library == nullptr) return nullptr;
  api.getDisplay = reinterpret_cast<PFNGetDisplay>(dlsym(library, "eglGetDisplay"));
  api.initialize = reinterpret_cast<PFNInitialize>(dlsym(library, "eglInitialize"));
  api.bindAPI = reinterpret_cast<PFNBindAPI>(dlsym(library, "eglBindAPI"));
  api.chooseConfig = reinterpret_cast<PFNChooseConfig>(dlsym(library, "eglChooseConfig"));
  api.createContext = reinterpret_cast<PFNCreateContext>(dlsym(library, "eglCreateContext"));
  api.createPbuffer = reinterpret_cast<PFNCreatePbufferSurface>(dlsym(library, "eglCreatePbufferSurface"));
  api.createWindowSurface = reinterpret_cast<PFNCreateWindowSurface>(dlsym(library, "eglCreateWindowSurface"));
  api.makeCurrent = reinterpret_cast<PFNMakeCurrent>(dlsym(library, "eglMakeCurrent"));
  api.swapBuffers = reinterpret_cast<PFNSwapBuffers>(dlsym(library, "eglSwapBuffers"));
  api.destroyContext = reinterpret_cast<PFNDestroyContext>(dlsym(library, "eglDestroyContext"));
  api.destroySurface = reinterpret_cast<PFNDestroySurface>(dlsym(library, "eglDestroySurface"));
  api.terminate = reinterpret_cast<PFNTerminate>(dlsym(library, "eglTerminate"));
  if (api.getDisplay == nullptr || api.initialize == nullptr || api.bindAPI == nullptr ||
      api.chooseConfig == nullptr || api.createContext == nullptr || api.createWindowSurface == nullptr ||
      api.makeCurrent == nullptr || api.swapBuffers == nullptr) {
    dlclose(library);
    return nullptr;
  }
  return library;
}

}  // namespace

EGLContext::~EGLContext() { destroy(); }

bool EGLContext::create(i32 width, i32 height) {
  destroy();
  EglApi api;
  library_ = openEgl(api);
  if (library_ == nullptr) return false;

  display_ = api.getDisplay(kDefaultDisplay);
  if (display_ == nullptr) {
    destroy();
    return false;
  }
  EglInt major = 0;
  EglInt minor = 0;
  if (api.initialize(display_, &major, &minor) == 0 || api.bindAPI(kOpenGLApi) == 0) {
    destroy();
    return false;
  }

  const EglInt configAttribs[] = {kSurfaceType, kPbufferBit, kRenderableType, kOpenGLBit, kRedSize,    8,
                                  kGreenSize,   8,           kBlueSize,      8,          kDepthSize,  24,
                                  kNone};
  EglConfig config = nullptr;
  EglInt configCount = 0;
  if (api.chooseConfig(display_, configAttribs, &config, 1, &configCount) == 0 || configCount < 1) {
    destroy();
    return false;
  }

  // Prefer a 3.3 core context; fall back to 3.0 and 2.1 (shader compile will
  // report the final story).
  const EglInt majorVersions[] = {3, 3, 2};
  const EglInt minorVersions[] = {3, 0, 1};
  for (i32 attempt = 0; attempt < 3; ++attempt) {
    const EglInt contextAttribs[] = {kContextMajorVersion, majorVersions[attempt],
                                     kContextMinorVersion, minorVersions[attempt], kNone};
    context_ = api.createContext(display_, config, nullptr, contextAttribs);
    if (context_ != nullptr) break;
  }
  if (context_ == nullptr) {
    destroy();
    return false;
  }

  const EglInt surfaceAttribs[] = {kWidth, width, kHeight, height, kNone};
  surface_ = api.createPbuffer(display_, config, surfaceAttribs);
  if (surface_ == nullptr || api.makeCurrent(display_, surface_, surface_, context_) == 0) {
    destroy();
    return false;
  }
  valid_ = true;
  return true;
}

bool EGLContext::createWindow(void* nativeWindow, i32 width, i32 height, bool msaa) {
  destroy();
  if (nativeWindow == nullptr || width <= 0 || height <= 0) return false;
  EglApi api;
  library_ = openEgl(api);
  if (library_ == nullptr) return false;

  display_ = api.getDisplay(kDefaultDisplay);
  if (display_ == nullptr) {
    destroy();
    return false;
  }
  EglInt major = 0;
  EglInt minor = 0;
  if (api.initialize(display_, &major, &minor) == 0) {
    destroy();
    return false;
  }

#ifdef __ANDROID__
  // Android has no desktop GL: bind GLES and ask for an ES 3 context, which
  // is the engine's GL 3.3 feature set (the Emscripten/WebGL2 path already
  // proves the renderer works on ES 3.00).
  if (api.bindAPI(kOpenGLEsApi) == 0) {
    destroy();
    return false;
  }
  const i32 renderableBit = kOpenGLEsApi;
#else
  if (api.bindAPI(kOpenGLApi) == 0) {
    destroy();
    return false;
  }
  const i32 renderableBit = kOpenGLBit;
#endif

  std::vector<EglInt> configAttribs = {kSurfaceType,    kWindowBit, kRenderableType, renderableBit,
                                       kRedSize,        8,          kGreenSize,      8,
                                       kBlueSize,       8,          kDepthSize,      24};
  if (msaa) {
    configAttribs.push_back(kSampleBuffers);
    configAttribs.push_back(1);
    configAttribs.push_back(kSamples);
    configAttribs.push_back(4);
  }
  configAttribs.push_back(kNone);
  EglConfig config = nullptr;
  EglInt configCount = 0;
  if (api.chooseConfig(display_, configAttribs.data(), &config, 1, &configCount) == 0 || configCount < 1) {
    destroy();
    return false;
  }

#ifdef __ANDROID__
  // kContextMajorVersion == EGL_CONTEXT_CLIENT_VERSION (0x3098): the ES 3
  // context request. Fall back to ES 2 so an old driver still gets a context
  // (the 3.00 shaders report their own failure at compile time).
  const EglInt contextAttribs[] = {kContextMajorVersion, 3, kNone};
  context_ = api.createContext(display_, config, nullptr, contextAttribs);
  if (context_ == nullptr) {
    const EglInt fallbackAttribs[] = {kContextMajorVersion, 2, kNone};
    context_ = api.createContext(display_, config, nullptr, fallbackAttribs);
  }
#else
  const EglInt majorVersions[] = {3, 3, 2};
  const EglInt minorVersions[] = {3, 0, 1};
  for (i32 attempt = 0; attempt < 3; ++attempt) {
    const EglInt contextAttribs[] = {kContextMajorVersion, majorVersions[attempt],
                                     kContextMinorVersion, minorVersions[attempt], kNone};
    context_ = api.createContext(display_, config, nullptr, contextAttribs);
    if (context_ != nullptr) break;
  }
#endif
  if (context_ == nullptr) {
    destroy();
    return false;
  }

  surface_ = api.createWindowSurface(display_, config, nativeWindow, nullptr);
  if (surface_ == nullptr || api.makeCurrent(display_, surface_, surface_, context_) == 0) {
    destroy();
    return false;
  }
  valid_ = true;
  return true;
}

bool EGLContext::swapBuffers() {
  if (!valid_ || library_ == nullptr || display_ == nullptr || surface_ == nullptr) return false;
  const auto swap = reinterpret_cast<PFNSwapBuffers>(dlsym(library_, "eglSwapBuffers"));
  if (swap == nullptr) return false;
  return swap(display_, surface_) != 0;
}

void EGLContext::destroy() {
  if (library_ == nullptr) return;
  const auto makeCurrent = reinterpret_cast<PFNMakeCurrent>(dlsym(library_, "eglMakeCurrent"));
  const auto destroyContext = reinterpret_cast<PFNDestroyContext>(dlsym(library_, "eglDestroyContext"));
  const auto destroySurface = reinterpret_cast<PFNDestroySurface>(dlsym(library_, "eglDestroySurface"));
  const auto terminate = reinterpret_cast<PFNTerminate>(dlsym(library_, "eglTerminate"));
  if (display_ != nullptr) {
    if (makeCurrent != nullptr) makeCurrent(display_, nullptr, nullptr, nullptr);
    if (context_ != nullptr && destroyContext != nullptr) destroyContext(display_, context_);
    if (surface_ != nullptr && destroySurface != nullptr) destroySurface(display_, surface_);
    if (terminate != nullptr) terminate(display_);
  }
  dlclose(library_);
  library_ = nullptr;
  display_ = nullptr;
  surface_ = nullptr;
  context_ = nullptr;
  valid_ = false;
}

}  // namespace kimia

#endif  // __EMSCRIPTEN__ / _WIN32 / dlopen-EGL
