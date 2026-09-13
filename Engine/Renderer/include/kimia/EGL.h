#pragma once

#include <kimia/Types.h>

namespace kimia {

// EGL context via dlopen'd libEGL (no build-time dependency on EGL headers
// or libraries). Two surface kinds:
//   create()         — a headless pbuffer, used when no window system exists
//                      or when the display route is the WebViewer.
//   createWindow()   — a real on-screen window surface (ANativeWindow on
//                      Android); the native Android GPU path renders into it
//                      and swapBuffers() presents the frame.
class EGLContext {
public:
  EGLContext() = default;
  ~EGLContext();
  EGLContext(const EGLContext&) = delete;
  EGLContext& operator=(const EGLContext&) = delete;

  bool create(i32 width, i32 height);
  // `nativeWindow` is an ANativeWindow* on Android (any EGLNativeWindowType
  // elsewhere). `msaa` requests a 4x multisampled config where the driver
  // offers one. Call swapBuffers() after each rendered frame.
  bool createWindow(void* nativeWindow, i32 width, i32 height, bool msaa = false);
  bool swapBuffers();
  void destroy();
  bool valid() const { return valid_; }

private:
  void* library_ = nullptr;
  void* display_ = nullptr;
  void* surface_ = nullptr;
  void* context_ = nullptr;
  bool valid_ = false;
};

}  // namespace kimia
