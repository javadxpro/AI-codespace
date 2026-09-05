#pragma once

#include <kimia/Image.h>
#include <kimia/InputState.h>
#include <kimia/Types.h>

#include <memory>
#include <string>

namespace kimia {

// Display/input surface. The display route is a CPU blit (present); GL
// rendering happens offscreen (EGL pbuffer) and is captured to PNG for the
// WebViewer, so the window itself never needs a GL context.
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
  virtual void setTitle(const std::string& title) = 0;
};

}  // namespace kimia
