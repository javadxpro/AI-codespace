// PCD3D11Smoke — Windows-only Phase 0 renderer validation.
//
// This is intentionally a small native smoke executable. It proves that the
// PC build can create a real HWND, a hardware D3D11 device and a swap chain
// before the legacy WorldEditor is migrated onto the new backend.

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <kimia/D3D11Renderer.h>
#include <kimia/MathUtils.h>
#include <kimia/Mesh.h>
#include <kimia/Renderer.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace {

constexpr char kWindowClass[] = "KIMIA_PC_D3D11_SMOKE";
HWND gWindow = nullptr;

LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
  static_cast<void>(lParam);
  switch (message) {
    case WM_CLOSE:
      ::DestroyWindow(window);
      return 0;
    case WM_DESTROY:
      ::PostQuitMessage(0);
      return 0;
    case WM_KEYDOWN:
      if (wParam == VK_ESCAPE) ::DestroyWindow(window);
      return 0;
    default:
      return ::DefWindowProcA(window, message, wParam, lParam);
  }
}

bool createNativeWindow(HINSTANCE instance, kimia::i32 width, kimia::i32 height) {
  WNDCLASSA windowClass{};
  windowClass.hInstance = instance;
  windowClass.lpfnWndProc = windowProcedure;
  windowClass.lpszClassName = kWindowClass;
  windowClass.hCursor = ::LoadCursorA(nullptr, IDC_ARROW);
  if (::RegisterClassA(&windowClass) == 0 && ::GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;

  RECT rectangle{0, 0, width, height};
  ::AdjustWindowRect(&rectangle, WS_OVERLAPPEDWINDOW, FALSE);
  gWindow = ::CreateWindowExA(0, kWindowClass, "KIMIA PC — Direct3D 11 smoke", WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, rectangle.right - rectangle.left,
                              rectangle.bottom - rectangle.top, nullptr, nullptr, instance, nullptr);
  if (gWindow == nullptr) return false;
  ::ShowWindow(gWindow, SW_SHOW);
  ::UpdateWindow(gWindow);
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  int maxFrames = 0;  // 0 = run until the window closes
  bool debugLayer = false;
  bool vsync = true;
  for (int i = 1; i < argc; ++i) {
    const std::string argument = argv[i];
    if (argument == "--frames" && i + 1 < argc) maxFrames = std::atoi(argv[++i]);
    if (argument == "--debug") debugLayer = true;
    if (argument == "--no-vsync") vsync = false;
  }

  constexpr kimia::i32 width = 1280;
  constexpr kimia::i32 height = 720;
  if (!createNativeWindow(::GetModuleHandleA(nullptr), width, height)) {
    std::fprintf(stderr, "cannot create native Windows window\n");
    return 1;
  }

  kimia::D3D11Renderer renderer;
  kimia::D3D11Options options;
  options.nativeWindow = gWindow;
  options.width = width;
  options.height = height;
  options.vsync = vsync;
  options.debugLayer = debugLayer;
  std::string error;
  if (!renderer.initialize(options, error)) {
    std::fprintf(stderr, "D3D11 initialization failed: %s\n", error.c_str());
    ::DestroyWindow(gWindow);
    return 2;
  }
  const double memoryGiB = static_cast<double>(renderer.dedicatedVideoMemoryBytes()) / (1024.0 * 1024.0 * 1024.0);
  std::printf("KIMIA PC D3D11 | adapter: %s | feature level: %s | VRAM: %.2f GiB | %dx%d | vsync: %s | debug: %s\n",
              renderer.adapterName().c_str(), renderer.featureLevelName().c_str(), memoryGiB, width, height,
              vsync ? "on" : "off", debugLayer ? "on" : "off");

  const kimia::MeshData cube = kimia::makeCube(1.5);
  kimia::RenderScene scene;
  scene.cameraPosition = kimia::Vec3{0.0, 1.2, 4.5};
  scene.projection = kimia::Mat4::perspective(kimia::radians(60.0), static_cast<kimia::f64>(width) / height, 0.1, 100.0);
  scene.view = kimia::Mat4::lookAt(scene.cameraPosition, kimia::Vec3{0.0, 0.0, 0.0}, kimia::Vec3{0.0, 1.0, 0.0});
  scene.lightDirection = kimia::Vec3{-0.4, -0.8, -0.5};
  scene.ambient = 0.25;
  scene.objects.push_back({&cube, kimia::Mat4{}, kimia::Vec3{0.12, 0.48, 0.95}, 0.35, 0.0, nullptr});

  bool running = true;
  int frame = 0;
  while (running && (maxFrames <= 0 || frame < maxFrames)) {
    MSG message{};
    while (::PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE) != 0) {
      if (message.message == WM_QUIT) running = false;
      ::TranslateMessage(&message);
      ::DispatchMessageA(&message);
    }
    if (!running) break;

    const kimia::f64 time = static_cast<kimia::f64>(frame) / 60.0;
    scene.objects[0].model = kimia::Mat4::rotationY(time) * kimia::Mat4::rotationX(time * 0.35);
    if (!renderer.render(scene, width, height, error)) {
      std::fprintf(stderr, "D3D11 render failed: %s\n", error.c_str());
      renderer.shutdown();
      ::DestroyWindow(gWindow);
      return 3;
    }
    ++frame;
  }

  renderer.shutdown();
  if (gWindow != nullptr) ::DestroyWindow(gWindow);
  std::printf("D3D11 smoke completed: %d frame(s)\n", frame);
  return 0;
}

#else

int main() { return 0; }

#endif
