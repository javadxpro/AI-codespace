#pragma once

#include <kimia/Renderer.h>
#include <kimia/Types.h>

#include <memory>
#include <string>

namespace kimia {

// The first desktop backend contract. `nativeWindow` is an HWND on Windows;
// it stays opaque here so the rest of the engine does not include Windows.h.
struct D3D11Options {
  void* nativeWindow = nullptr;
  i32 width = 1280;
  i32 height = 720;
  bool vsync = true;
  bool debugLayer = false;
};

// Direct3D 11 renderer for the Windows PC build. The class is deliberately
// independent from the legacy OpenGL Renderer: the PC runtime can select it
// without forcing the WebWorkbench/software path to create a GL context.
// On non-Windows platforms it is a safe unavailable backend.
class D3D11Renderer {
public:
  D3D11Renderer();
  ~D3D11Renderer();
  D3D11Renderer(const D3D11Renderer&) = delete;
  D3D11Renderer& operator=(const D3D11Renderer&) = delete;

  bool initialize(const D3D11Options& options, std::string& error);
  void shutdown();
  bool ready() const;

  // Resizes the swap chain and depth buffer after a native window resize.
  bool resize(i32 width, i32 height, std::string& error);

  // Draws the current RenderScene and presents it. This first backend keeps
  // the same scene contract as the software/GL paths so game migration can be
  // incremental; base-color texture upload and sampler caching are included,
  // while the full multi-map material graph remains a later phase.
  bool render(const RenderScene& scene, i32 width, i32 height, std::string& error);

  void setVsync(bool enabled);
  bool vsync() const;
  const std::string& adapterName() const;
  const std::string& featureLevelName() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace kimia
