#pragma once

#ifdef KIMIA_HAS_SDL2

#include <kimia/Types.h>
#include <kimia/Window.h>

#include <memory>
#include <string>

struct SDL_Window;

namespace kimia {

// SDL2-backed window: event polling (keyboard/quit) and CPU-blit display via
// a window surface (no GL context required). Hidden-window mode supported
// (the WebViewer is the real display on Termux).
class SDLWindow final : public Window {
public:
  ~SDLWindow() override;

  static std::unique_ptr<Window> create(const std::string& title, i32 width, i32 height, bool hidden);

  bool valid() const override;
  bool poll(InputState& input) override;
  void present(const Image& image) override;
  i32 width() const override;
  i32 height() const override;
  void setTitle(const std::string& title) override;

private:
  SDLWindow() = default;
  bool init(const std::string& title, i32 width, i32 height, bool hidden);

  SDL_Window* window_ = nullptr;
  i32 width_ = 0;
  i32 height_ = 0;
  bool hidden_ = false;
  bool quitRequested_ = false;
};

}  // namespace kimia

#endif  // KIMIA_HAS_SDL2
