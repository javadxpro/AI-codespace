package com.kimia.world;

/**
 * JNI binding to the KIMIA engine's shared library (libkimia_jni.so).
 * The native side hosts the WebViewer HTTP server on 127.0.0.1.
 */
public final class NativeEngine {
  static {
    System.loadLibrary("kimia_jni");
  }

  private NativeEngine() {}

  public static native void nativeStart(int port, String filesDir);
  public static native void nativeStop();

  public static void start(int port, String filesDir) {
    nativeStart(port, filesDir);
  }

  public static void stop() {
    nativeStop();
  }
}
