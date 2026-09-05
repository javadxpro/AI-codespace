#include <kimia/SDLWindow.h>

#ifdef KIMIA_HAS_SDL2

#include <SDL.h>

#include <vector>

namespace kimia {

namespace {

std::optional<Key> keyFromScancode(SDL_Scancode scancode) {
  if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z) {
    return static_cast<Key>(static_cast<i32>(Key::A) + (scancode - SDL_SCANCODE_A));
  }
  if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9) {
    return static_cast<Key>(static_cast<i32>(Key::Num1) + (scancode - SDL_SCANCODE_1));
  }
  if (scancode == SDL_SCANCODE_0) return Key::Num0;
  if (scancode == SDL_SCANCODE_UP) return Key::Up;
  if (scancode == SDL_SCANCODE_DOWN) return Key::Down;
  if (scancode == SDL_SCANCODE_LEFT) return Key::Left;
  if (scancode == SDL_SCANCODE_RIGHT) return Key::Right;
  if (scancode == SDL_SCANCODE_RETURN) return Key::Return;
  if (scancode == SDL_SCANCODE_SPACE) return Key::Space;
  if (scancode == SDL_SCANCODE_LSHIFT || scancode == SDL_SCANCODE_RSHIFT) return Key::Shift;
  if (scancode == SDL_SCANCODE_ESCAPE) return Key::Escape;
  if (scancode == SDL_SCANCODE_TAB) return Key::Tab;
  if (scancode == SDL_SCANCODE_BACKSPACE) return Key::Backspace;
  return std::nullopt;
}

}  // namespace

SDLWindow::~SDLWindow() {
  if (window_ != nullptr) {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
  }
  SDL_Quit();
}

std::unique_ptr<Window> SDLWindow::create(const std::string& title, i32 width, i32 height, bool hidden) {
  std::unique_ptr<SDLWindow> window(new SDLWindow());
  if (!window->init(title, width, height, hidden)) return nullptr;
  return window;
}

bool SDLWindow::init(const std::string& title, i32 width, i32 height, bool hidden) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) return false;
  u32 flags = SDL_WINDOW_SHOWN;
  if (hidden) flags = SDL_WINDOW_HIDDEN;
  window_ = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags);
  if (window_ == nullptr) {
    SDL_Quit();
    return false;
  }
  width_ = width;
  height_ = height;
  hidden_ = hidden;
  return true;
}

bool SDLWindow::valid() const { return window_ != nullptr; }

bool SDLWindow::poll(InputState& input) {
  SDL_Event event;
  while (SDL_PollEvent(&event) != 0) {
    if (event.type == SDL_QUIT) {
      quitRequested_ = true;
      continue;
    }
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
      const bool down = event.type == SDL_KEYDOWN;
      const auto key = keyFromScancode(event.key.keysym.scancode);
      if (key.has_value()) input.setKeyDown(*key, down);
      if (down && event.key.keysym.sym == SDLK_ESCAPE) quitRequested_ = true;
    }
  }
  return !quitRequested_;
}

void SDLWindow::present(const Image& image) {
  if (window_ == nullptr || hidden_ || image.isEmpty()) return;
  SDL_Surface* surface = SDL_GetWindowSurface(window_);
  if (surface == nullptr) return;
  u32 format = SDL_PIXELFORMAT_RGB24;
  if (image.channels == 4) format = SDL_PIXELFORMAT_RGBA32;
  SDL_Surface* frame = SDL_CreateRGBSurfaceWithFormatFrom(
      const_cast<void*>(static_cast<const void*>(image.pixels.data())), image.width, image.height,
      static_cast<int>(image.channels * 8), static_cast<int>(image.width * image.channels), format);
  if (frame == nullptr) return;
  SDL_BlitScaled(frame, nullptr, surface, nullptr);
  SDL_UpdateWindowSurface(window_);
  SDL_FreeSurface(frame);
}

i32 SDLWindow::width() const { return width_; }
i32 SDLWindow::height() const { return height_; }

void SDLWindow::setTitle(const std::string& title) {
  if (window_ != nullptr) SDL_SetWindowTitle(window_, title.c_str());
}

}  // namespace kimia

#endif  // KIMIA_HAS_SDL2
