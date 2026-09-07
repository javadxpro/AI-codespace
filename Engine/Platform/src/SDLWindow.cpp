#include <kimia/SDLWindow.h>

#ifdef KIMIA_HAS_SDL2

#include <SDL.h>
#ifdef _WIN32
#include <SDL_syswm.h>
#endif

#include <algorithm>
#include <cmath>
#include <optional>

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

std::optional<GamepadButton> gamepadButtonFromSDL(Uint8 button) {
  switch (button) {
    case SDL_CONTROLLER_BUTTON_A:
      return GamepadButton::A;
    case SDL_CONTROLLER_BUTTON_B:
      return GamepadButton::B;
    case SDL_CONTROLLER_BUTTON_X:
      return GamepadButton::X;
    case SDL_CONTROLLER_BUTTON_Y:
      return GamepadButton::Y;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
      return GamepadButton::LeftShoulder;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
      return GamepadButton::RightShoulder;
    case SDL_CONTROLLER_BUTTON_BACK:
      return GamepadButton::Back;
    case SDL_CONTROLLER_BUTTON_START:
      return GamepadButton::Start;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:
      return GamepadButton::DpadUp;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
      return GamepadButton::DpadDown;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
      return GamepadButton::DpadLeft;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
      return GamepadButton::DpadRight;
    default:
      return std::nullopt;
  }
}

std::optional<GamepadAxis> gamepadAxisFromSDL(Uint8 axis) {
  switch (axis) {
    case SDL_CONTROLLER_AXIS_LEFTX:
      return GamepadAxis::LeftX;
    case SDL_CONTROLLER_AXIS_LEFTY:
      return GamepadAxis::LeftY;
    case SDL_CONTROLLER_AXIS_RIGHTX:
      return GamepadAxis::RightX;
    case SDL_CONTROLLER_AXIS_RIGHTY:
      return GamepadAxis::RightY;
    case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
      return GamepadAxis::LeftTrigger;
    case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
      return GamepadAxis::RightTrigger;
    default:
      return std::nullopt;
  }
}

f64 normaliseStick(Sint16 value) {
  const f64 raw = value < 0 ? static_cast<f64>(value) / 32768.0 : static_cast<f64>(value) / 32767.0;
  const f64 magnitude = std::abs(raw);
  constexpr f64 kDeadZone = 0.15;
  if (magnitude <= kDeadZone) return 0.0;
  const f64 scaled = (magnitude - kDeadZone) / (1.0 - kDeadZone);
  return raw < 0.0 ? -scaled : scaled;
}

f64 normaliseTrigger(Sint16 value) {
  return std::clamp(static_cast<f64>(value) / 32767.0, 0.0, 1.0);
}

}  // namespace

SDLWindow::~SDLWindow() {
  if (controller_ != nullptr) {
    SDL_GameControllerClose(controller_);
    controller_ = nullptr;
  }
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
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) return false;
  u32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
  if (hidden) flags = SDL_WINDOW_HIDDEN;
  window_ = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags);
  if (window_ == nullptr) {
    SDL_Quit();
    return false;
  }
  width_ = width;
  height_ = height;
  hidden_ = hidden;
  for (i32 index = 0; index < SDL_NumJoysticks(); ++index) {
    if (SDL_IsGameController(index) == SDL_TRUE) {
      controller_ = SDL_GameControllerOpen(index);
      if (controller_ != nullptr) break;
    }
  }
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
    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
      width_ = static_cast<i32>(event.window.data1);
      height_ = static_cast<i32>(event.window.data2);
      continue;
    }
    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
      input.clearHeld();
      continue;
    }
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
      const bool down = event.type == SDL_KEYDOWN;
      const auto key = keyFromScancode(event.key.keysym.scancode);
      if (key.has_value()) input.setKeyDown(*key, down);
      if (down && event.key.keysym.sym == SDLK_ESCAPE) quitRequested_ = true;
      continue;
    }
    if (event.type == SDL_MOUSEMOTION) {
      if (input.mouseDown(MouseButton::Right)) {
        input.addLook(static_cast<f64>(event.motion.xrel), static_cast<f64>(event.motion.yrel));
      }
      continue;
    }
    if (event.type == SDL_MOUSEWHEEL) {
      input.addZoom(static_cast<f64>(event.wheel.y));
      continue;
    }
    if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
      const bool down = event.type == SDL_MOUSEBUTTONDOWN;
      MouseButton button = MouseButton::Left;
      bool known = true;
      switch (event.button.button) {
        case SDL_BUTTON_LEFT:
          button = MouseButton::Left;
          break;
        case SDL_BUTTON_RIGHT:
          button = MouseButton::Right;
          break;
        case SDL_BUTTON_MIDDLE:
          button = MouseButton::Middle;
          break;
        default:
          known = false;
          break;
      }
      if (known) input.setMouseButton(button, down);
      continue;
    }
    if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
      const auto button = gamepadButtonFromSDL(event.cbutton.button);
      if (button.has_value()) input.setGamepadButton(*button, event.type == SDL_CONTROLLERBUTTONDOWN);
      continue;
    }
    if (event.type == SDL_CONTROLLERAXISMOTION) {
      const auto axis = gamepadAxisFromSDL(event.caxis.axis);
      if (axis.has_value()) {
        const bool trigger = *axis == GamepadAxis::LeftTrigger || *axis == GamepadAxis::RightTrigger;
        input.setGamepadAxis(*axis, trigger ? normaliseTrigger(event.caxis.value)
                                             : normaliseStick(event.caxis.value));
      }
      continue;
    }
    if (event.type == SDL_CONTROLLERDEVICEADDED && controller_ == nullptr &&
        SDL_IsGameController(event.cdevice.which) == SDL_TRUE) {
      controller_ = SDL_GameControllerOpen(event.cdevice.which);
    }
    if (event.type == SDL_CONTROLLERDEVICEREMOVED && controller_ != nullptr) {
      SDL_Joystick* joystick = SDL_GameControllerGetJoystick(controller_);
      if (joystick != nullptr && SDL_JoystickInstanceID(joystick) == event.cdevice.which) {
        SDL_GameControllerClose(controller_);
        controller_ = nullptr;
      }
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

void* SDLWindow::nativeHandle() const {
#ifdef _WIN32
  if (window_ == nullptr) return nullptr;
  SDL_SysWMinfo info{};
  SDL_VERSION(&info.version);
  if (SDL_GetWindowWMInfo(window_, &info) == SDL_TRUE) return info.info.win.window;
#endif
  return nullptr;
}

void SDLWindow::setTitle(const std::string& title) {
  if (window_ != nullptr) SDL_SetWindowTitle(window_, title.c_str());
}

}  // namespace kimia

#endif  // KIMIA_HAS_SDL2
