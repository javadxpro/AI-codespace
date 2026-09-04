#include <kimia/Window.h>

#ifdef KIMIA_HAS_SDL2
#include <kimia/SDLWindow.h>
#endif

namespace kimia {

std::unique_ptr<Window> Window::create(const std::string& title, i32 width, i32 height, bool hidden) {
#ifdef KIMIA_HAS_SDL2
  return SDLWindow::create(title, width, height, hidden);
#else
  static_cast<void>(title);
  static_cast<void>(width);
  static_cast<void>(height);
  static_cast<void>(hidden);
  return nullptr;
#endif
}

}  // namespace kimia
