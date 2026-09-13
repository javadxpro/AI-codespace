package com.kimia.world;

import android.view.Surface;

/**
 * JNI binding to the KIMIA engine's shared library (libkimia_jni.so).
 *
 * <p>The native side boots the engine in-process and renders straight onto an
 * ANativeWindow (the Activity's SurfaceView) with GLES3 — Cook-Torrance PBR,
 * shadows, fog and the filmic tone map — falling back to the software
 * rasteriser when GLES3/EGL cannot come up. There is no WebView and no HTTP
 * server on 127.0.0.1: the whole game (world, touch controls, HUD, on-screen
 * buttons) runs inside the engine.
 */
public final class NativeEngine {

  /** Renderer choice for {@link #nativeStart}. */
  public static final int BACKEND_AUTO = 0;      // GLES3, then software
  public static final int BACKEND_GLES3 = 1;     // GLES3 only
  public static final int BACKEND_SOFTWARE = 2;  // CPU rasteriser only

  static {
    System.loadLibrary("kimia_jni");
  }

  private NativeEngine() {}

  /** Boots the game thread with the chosen flags. Safe to call once. */
  public static native void nativeStart(String filesDir, String game, int backend,
                                        int width, int height, int fps,
                                        boolean shadows, boolean msaa);

  /** Hands the Activity's surface (or null) to the renderer. */
  public static native void nativeSetSurface(Surface surface);

  /** Reports the surface size; recreates the EGL surface at the new size. */
  public static native void nativeSurfaceChanged(int width, int height);

  /** The surface is going away; the native side drops its ANativeWindow. */
  public static native void nativeSurfaceDestroyed();

  /** Forwards a MotionEvent: action = getActionMasked(), pointerId = getActionIndex(). */
  public static native void nativeTouch(int action, int pointerId, float x, float y);

  /** Stops the game thread and releases everything. */
  public static native void nativeStop();
}
