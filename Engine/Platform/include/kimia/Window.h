#pragma once

#include <kimia/Image.h>
#include <kimia/InputState.h>
#include <kimia/Types.h>

#include <memory>
#include <string>

namespace kimia {

// Display/input surface. The software/remote path uses present() as a CPU
// blit; the Windows PC path can use nativeHandle() for D3D11 and keeps the
// WebViewer capture independent from the native swap chain.
class Window {
public:
  virtual ~Window() = default;

  // Returns null when no window backend is available (headless build/run).
  static std::unique_ptr<Window> create(const std::string& title, i32 width, i32 height, bool hidden);

  virtual bool valid() const = 0;
  // Polls events into `input`. Returns false when the user asked to quit.
  virtual bool poll(InputState& input) = 0;
  // Shows an image on the window (no-op when hidden or unsupported).
  virtual void present(const Image& image) = 0;
  virtual i32 width() const = 0;
  virtual i32 height() const = 0;
  // Opaque native handle for a platform renderer. On Windows this is an HWND
  // when the SDL backend is active; other backends may return nullptr.
  virtual void* nativeHandle() const { return nullptr; }
  virtual void setTitle(const std::string& title) = 0;
};

}  // namespace kimia
