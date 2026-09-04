#pragma once

#include <kimia/Types.h>

namespace kimia {

// Headless OpenGL context via EGL pbuffer surfaces (dlopen'd at runtime,
// no build-time dependency on EGL headers or libraries). Used when no
// window system is available or when the display route is the WebViewer.
class EGLContext {
public:
  EGLContext() = default;
  ~EGLContext();
  EGLContext(const EGLContext&) = delete;
  EGLContext& operator=(const EGLContext&) = delete;

  bool create(i32 width, i32 height);
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
