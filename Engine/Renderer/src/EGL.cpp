#include <kimia/EGL.h>

#include <dlfcn.h>

namespace kimia {

namespace {

// EGL constants (defined locally: no build-time dependency on EGL headers).
constexpr i32 kDefaultDisplay = 0;
constexpr i32 kPbufferBit = 0x0001;
constexpr i32 kOpenGLBit = 0x0008;
constexpr i32 kRenderableType = 0x3040;
constexpr i32 kSurfaceType = 0x3033;
constexpr i32 kRedSize = 0x3024;
constexpr i32 kGreenSize = 0x3023;
constexpr i32 kBlueSize = 0x3022;
constexpr i32 kDepthSize = 0x3025;
constexpr i32 kNone = 0x3038;
constexpr i32 kOpenGLApi = 0x30A2;
constexpr i32 kContextMajorVersion = 0x3098;
constexpr i32 kContextMinorVersion = 0x3097;
constexpr i32 kWidth = 0x3057;
constexpr i32 kHeight = 0x3056;

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
using PFNMakeCurrent = EglBoolean (*)(EglDisplay, EglSurface, EglSurface, EglContextHandle);
using PFNDestroyContext = EglBoolean (*)(EglDisplay, EglContextHandle);
using PFNDestroySurface = EglBoolean (*)(EglDisplay, EglSurface);
using PFNTerminate = EglBoolean (*)(EglDisplay);
using PFNGetError = EglInt (*)();

}  // namespace

EGLContext::~EGLContext() { destroy(); }

bool EGLContext::create(i32 width, i32 height) {
  destroy();
  library_ = dlopen("libEGL.so.1", RTLD_NOW | RTLD_LOCAL);
  if (library_ == nullptr) library_ = dlopen("libEGL.so", RTLD_NOW | RTLD_LOCAL);
  if (library_ == nullptr) return false;
  const auto getDisplay = reinterpret_cast<PFNGetDisplay>(dlsym(library_, "eglGetDisplay"));
  const auto initialize = reinterpret_cast<PFNInitialize>(dlsym(library_, "eglInitialize"));
  const auto bindAPI = reinterpret_cast<PFNBindAPI>(dlsym(library_, "eglBindAPI"));
  const auto chooseConfig = reinterpret_cast<PFNChooseConfig>(dlsym(library_, "eglChooseConfig"));
  const auto createContext = reinterpret_cast<PFNCreateContext>(dlsym(library_, "eglCreateContext"));
  const auto createPbuffer = reinterpret_cast<PFNCreatePbufferSurface>(dlsym(library_, "eglCreatePbufferSurface"));
  const auto makeCurrent = reinterpret_cast<PFNMakeCurrent>(dlsym(library_, "eglMakeCurrent"));
  const auto destroyContext = reinterpret_cast<PFNDestroyContext>(dlsym(library_, "eglDestroyContext"));
  const auto destroySurface = reinterpret_cast<PFNDestroySurface>(dlsym(library_, "eglDestroySurface"));
  const auto terminate = reinterpret_cast<PFNTerminate>(dlsym(library_, "eglTerminate"));
  if (getDisplay == nullptr || initialize == nullptr || bindAPI == nullptr || chooseConfig == nullptr ||
      createContext == nullptr || createPbuffer == nullptr || makeCurrent == nullptr) {
    destroy();
    return false;
  }

  display_ = getDisplay(kDefaultDisplay);
  if (display_ == nullptr) {
    destroy();
    return false;
  }
  EglInt major = 0;
  EglInt minor = 0;
  if (initialize(display_, &major, &minor) == 0 || bindAPI(kOpenGLApi) == 0) {
    destroy();
    return false;
  }

  const EglInt configAttribs[] = {kSurfaceType, kPbufferBit, kRenderableType, kOpenGLBit, kRedSize,    8,
                                  kGreenSize,   8,           kBlueSize,      8,          kDepthSize,  24,
                                  kNone};
  EglConfig config = nullptr;
  EglInt configCount = 0;
  if (chooseConfig(display_, configAttribs, &config, 1, &configCount) == 0 || configCount < 1) {
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
    context_ = createContext(display_, config, nullptr, contextAttribs);
    if (context_ != nullptr) break;
  }
  if (context_ == nullptr) {
    destroy();
    return false;
  }

  const EglInt surfaceAttribs[] = {kWidth, width, kHeight, height, kNone};
  surface_ = createPbuffer(display_, config, surfaceAttribs);
  if (surface_ == nullptr || makeCurrent(display_, surface_, surface_, context_) == 0) {
    destroy();
    return false;
  }
  static_cast<void>(destroyContext);
  static_cast<void>(destroySurface);
  static_cast<void>(terminate);
  valid_ = true;
  return true;
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
