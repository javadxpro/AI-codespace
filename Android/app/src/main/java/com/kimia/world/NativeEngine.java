package com.kimia.world;

public final class NativeEngine {
  static {
    System.loadLibrary("kimia_engine");
  }

  // Backends (kept in sync with the JNI's NativeConfig).
  public static final int BACKEND_AUTO = 0;
  public static final int BACKEND_GLES3 = 1;
  public static final int BACKEND_SOFTWARE = 2;

  // Application modes.
  public static final int MODE_PLAY = 0;
  public static final int MODE_EDIT = 1;

  // Edit-mode primitive kinds.
  public static final int EDIT_NEW_CUBE = 0;
  public static final int EDIT_NEW_SPHERE = 1;
  public static final int EDIT_NEW_PLANE = 2;

  public static native void nativeStart(String filesDir, String game, int backend,
                                        int width, int height, int fps, boolean shadows, boolean msaa);
  public static native void nativeStop();
  public static native void nativeSetSurface(android.view.Surface surface);
  public static native void nativeSurfaceChanged(int width, int height);
  public static native void nativeSurfaceDestroyed();
  public static native void nativeTouch(int action, int pointerId, float x, float y);

  // Phase 1: in-world editor bridge (UI thread → render thread).
  public static native void nativeSetMode(int mode);
  public static native void nativeEditSelect(float x, float y);
  public static native void nativeEditBeginDrag(float x, float y);
  public static native void nativeEditDragTo(float fromX, float fromY, float toX, float toY);
  public static native void nativeEditEndDrag();
  public static native void nativeEditSetColor(float r, float g, float b);
  public static native void nativeEditDelete();
  public static native void nativeEditNew(int kind);
  public static native void nativeEditSave(String path);

  // Phase 1: snapshot readers (UI thread ← render thread).
  public static native int nativeEditRefresh();
  public static native void nativeEditGetNames(String[] out);
  public static native String nativeEditGetSelected();
  public static native float nativeEditGetColorR();
  public static native float nativeEditGetColorG();
  public static native float nativeEditGetColorB();
  public static native float nativeEditGetPosX();
  public static native float nativeEditGetPosY();
  public static native float nativeEditGetPosZ();
  public static native int nativeEditSelectionChanged();
}
