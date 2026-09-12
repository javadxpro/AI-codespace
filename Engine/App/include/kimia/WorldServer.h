#pragma once

#include <string>

// Reusable entry point for the KIMIA World game/editor behind the WebViewer
// HTTP server. Shared by the CLI (Examples/WorldEditorApp.cpp) and the
// Android JNI wrapper (Android/app/src/main/cpp/jni_glue.cpp).
//
// Deliberately at global scope: the app translation unit relies on global
// `using kimia::X;` declarations, so a namespace would force hundreds of
// qualifications for no benefit here.
struct WorldServerOptions {
  int port = 8080;
  std::string bindAddress = "127.0.0.1";
  std::string authToken;      // empty = loopback only
  std::string worldPath = "my_world.kimia";
  std::string assetsDir = "assets";
  std::string profilesDir = "profiles";
  std::string brandingDir;    // empty = auto-detect Branding/; "-" = skip intro
  std::string playWorld;      // non-empty = a published game, not the editor
  std::string unpackDir;      // embedded-asset unpack root (empty = system temp)
  bool desktopMode = false;   // native window + D3D11 (PC only)
  int frameWidth = 640;       // software-capture size (lower = cooler phone)
  int frameHeight = 480;
  int maxFps = 30;            // web frame cap (lower = cooler phone)
  int jpegQuality = 80;       // stream JPEG quality 10..100
};

// Runs the engine + WebViewer server and blocks until the app quits
// (SIGINT, a quit request, or the editor asking to quit). Returns the
// process exit code.
int runWorldServer(const WorldServerOptions& options);

// Thread-safe quit request. On desktop this is what SIGINT does; the Android
// wrapper calls it from Activity.onDestroy().
void requestWorldServerShutdown();
