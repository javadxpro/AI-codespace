// KIMIA Android JNI bridge.
//
// Runs the KIMIA World engine + WebViewer server (headless software renderer)
// inside the app process. The Java side (MainActivity) then points a WebView
// at http://127.0.0.1:<port> — the exact same dependable path the PS4 and
// Termux builds use. Graphics/physics quality is identical to those builds;
// the only difference is that the browser is embedded in the app instead of
// opened manually.
#include <jni.h>

#include <thread>

#include <kimia/WorldServer.h>

namespace {

std::thread gServerThread;

}  // namespace

extern "C" JNIEXPORT void JNICALL
Java_com_kimia_world_NativeEngine_nativeStart(JNIEnv* /*env*/, jobject /*thiz*/, jint port) {
  if (gServerThread.joinable()) return;  // already running

  WorldServerOptions options;
  options.port = static_cast<int>(port);
  options.bindAddress = "127.0.0.1";  // loopback only; the WebView reads it locally
  // Lighter than desktop defaults so a phone SoC stays cool while the
  // software rasteriser draws every frame on the CPU.
  options.frameWidth = 480;
  options.frameHeight = 360;
  options.maxFps = 20;
  options.jpegQuality = 70;

  gServerThread = std::thread(
      [](WorldServerOptions o) { runWorldServer(o); },
      options);
}

extern "C" JNIEXPORT void JNICALL
Java_com_kimia_world_NativeEngine_nativeStop(JNIEnv* /*env*/, jobject /*thiz*/) {
  requestWorldServerShutdown();
  if (gServerThread.joinable()) gServerThread.join();
}
