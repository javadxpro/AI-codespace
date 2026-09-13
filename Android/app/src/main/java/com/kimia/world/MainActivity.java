package com.kimia.world;

import android.app.Activity;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.Bundle;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;

/**
 * Hosts the KIMIA engine natively. A full-screen SurfaceView hands its
 * ANativeWindow to the native renderer (GLES3 with a software fallback), and
 * every touch is forwarded to the engine, so the whole game — world, controls,
 * HUD, on-screen buttons — runs on-device. A settings panel (toggled by the
 * gear button) edits the flags: game, resolution, FPS, GPU vs software and
 * shadows.
 */
public final class MainActivity extends Activity implements SurfaceHolder.Callback {

  private static final String PREFS = "kimia_settings";

  private FrameLayout root;
  private SurfaceView surfaceView;
  private View settingsButton;
  private View settingsPanel;

  private Spinner gameSpinner;
  private Spinner resolutionSpinner;
  private Spinner fpsSpinner;
  private Spinner backendSpinner;
  private CheckBox shadowsCheck;
  private CheckBox msaaCheck;

  private String filesDir;
  private boolean started = false;

  // Current flags (persisted, and what a restart applies).
  private String game = "golf";
  private int backend = NativeEngine.BACKEND_AUTO;
  private int width = 0;   // 0 = native surface size
  private int height = 0;
  private int fps = 60;
  private boolean shadows = true;
  private boolean msaa = false;

  private static final String[] GAME_LABELS = {"Golf", "Street", "Grass", "Battleground"};
  private static final String[] GAME_VALUES = {"golf", "street", "grass", "battleground"};

  private static final String[] RESOLUTION_LABELS = {
      "Screen (native)", "1280 x 720", "960 x 540", "640 x 360"};
  private static final int[][] RESOLUTION_VALUES = {
      {0, 0}, {1280, 720}, {960, 540}, {640, 360}};

  private static final String[] FPS_LABELS = {"30 FPS", "60 FPS"};
  private static final int[] FPS_VALUES = {30, 60};

  private static final String[] BACKEND_LABELS = {
      "Auto (GPU -> Software)", "GPU (GLES 3)", "Software (CPU)"};
  private static final int[] BACKEND_VALUES = {
      NativeEngine.BACKEND_AUTO, NativeEngine.BACKEND_GLES3, NativeEngine.BACKEND_SOFTWARE};

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    requestWindowFeature(Window.FEATURE_NO_TITLE);
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON
        | WindowManager.LayoutParams.FLAG_FULLSCREEN);
    hideSystemBars();

    filesDir = getFilesDir().getAbsolutePath();
    loadSettings();

    root = new FrameLayout(this);
    root.setBackgroundColor(Color.BLACK);

    surfaceView = new SurfaceView(this);
    surfaceView.getHolder().addCallback(this);
    surfaceView.setOnTouchListener(touchListener);
    root.addView(surfaceView,
        new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.MATCH_PARENT));

    settingsButton = makeSettingsButton();
    root.addView(settingsButton, settingsButtonLayout());

    settingsPanel = buildSettingsPanel();
    settingsPanel.setVisibility(View.GONE);
    root.addView(settingsPanel,
        new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.MATCH_PARENT));

    setContentView(root);
    startEngine();
  }

  private final View.OnTouchListener touchListener = new View.OnTouchListener() {
    @Override
    public boolean onTouch(View v, MotionEvent event) {
      final int action = event.getActionMasked();
      final int index = event.getActionIndex();
      final float x = event.getX(index);
      final float y = event.getY(index);
      NativeEngine.nativeTouch(action, index, x, y);
      return true;
    }
  };

  // --- SurfaceHolder.Callback ----------------------------------------------

  @Override
  public void surfaceCreated(SurfaceHolder holder) {
    NativeEngine.nativeSetSurface(holder.getSurface());
    final Rect frame = holder.getSurfaceFrame();
    NativeEngine.nativeSurfaceChanged(frame.width(), frame.height());
  }

  @Override
  public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
    NativeEngine.nativeSurfaceChanged(width, height);
  }

  @Override
  public void surfaceDestroyed(SurfaceHolder holder) {
    NativeEngine.nativeSurfaceDestroyed();
  }

  // --- Engine lifecycle ----------------------------------------------------

  private void startEngine() {
    NativeEngine.nativeStart(filesDir, game, backend, width, height, fps, shadows, msaa);
    started = true;
    // If the surface is already live (an in-place restart), re-attach it so
    // the fresh game thread renders immediately.
    final SurfaceHolder holder = surfaceView.getHolder();
    if (holder.getSurface() != null && holder.getSurface().isValid()) {
      NativeEngine.nativeSetSurface(holder.getSurface());
      final Rect frame = holder.getSurfaceFrame();
      NativeEngine.nativeSurfaceChanged(frame.width(), frame.height());
    }
  }

  private void restartEngine() {
    if (started) {
      NativeEngine.nativeStop();
    }
    startEngine();
  }

  @Override
  protected void onDestroy() {
    if (started) {
      NativeEngine.nativeStop();
      started = false;
    }
    super.onDestroy();
  }

  @Override
  public void onBackPressed() {
    if (settingsPanel.getVisibility() == View.VISIBLE) {
      settingsPanel.setVisibility(View.GONE);
      hideSystemBars();
      return;
    }
    super.onBackPressed();
  }

  @Override
  public void onWindowFocusChanged(boolean hasFocus) {
    super.onWindowFocusChanged(hasFocus);
    if (hasFocus) {
      hideSystemBars();
    }
  }

  // --- Settings ------------------------------------------------------------

  private void loadSettings() {
    final SharedPreferences p = getSharedPreferences(PREFS, MODE_PRIVATE);
    game = p.getString("game", "golf");
    backend = p.getInt("backend", NativeEngine.BACKEND_AUTO);
    width = p.getInt("width", 0);
    height = p.getInt("height", 0);
    fps = p.getInt("fps", 60);
    shadows = p.getBoolean("shadows", true);
    msaa = p.getBoolean("msaa", false);
  }

  private void saveSettings() {
    final SharedPreferences p = getSharedPreferences(PREFS, MODE_PRIVATE);
    p.edit()
        .putString("game", game)
        .putInt("backend", backend)
        .putInt("width", width)
        .putInt("height", height)
        .putInt("fps", fps)
        .putBoolean("shadows", shadows)
        .putBoolean("msaa", msaa)
        .apply();
  }

  private int indexOf(String[] values, String value) {
    for (int i = 0; i < values.length; i++) {
      if (values[i].equals(value)) {
        return i;
      }
    }
    return 0;
  }

  private int indexOf(int[] values, int value) {
    for (int i = 0; i < values.length; i++) {
      if (values[i] == value) {
        return i;
      }
    }
    return 0;
  }

  private int resolutionIndex(int w, int h) {
    for (int i = 0; i < RESOLUTION_VALUES.length; i++) {
      if (RESOLUTION_VALUES[i][0] == w && RESOLUTION_VALUES[i][1] == h) {
        return i;
      }
    }
    return 0;
  }

  private View makeSettingsButton() {
    final TextView button = new TextView(this);
    button.setText("\u2699");          // gear
    button.setTextSize(22);
    button.setTextColor(Color.WHITE);
    button.setGravity(Gravity.CENTER);
    button.setBackgroundColor(0x66000000);
    button.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        final boolean show = settingsPanel.getVisibility() != View.VISIBLE;
        settingsPanel.setVisibility(show ? View.VISIBLE : View.GONE);
      }
    });
    return button;
  }

  private FrameLayout.LayoutParams settingsButtonLayout() {
    final FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(dp(44), dp(44),
        Gravity.TOP | Gravity.END);
    lp.setMargins(dp(12), dp(12), dp(12), dp(12));
    return lp;
  }

  private View buildSettingsPanel() {
    final FrameLayout scrim = new FrameLayout(this);
    scrim.setBackgroundColor(0xCC000000);
    scrim.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        // Tapping the dim area closes the panel.
        settingsPanel.setVisibility(View.GONE);
      }
    });

    final LinearLayout column = new LinearLayout(this);
    column.setOrientation(LinearLayout.VERTICAL);
    column.setPadding(dp(20), dp(24), dp(20), dp(24));
    column.setBackgroundColor(0xF0202228);

    final TextView title = new TextView(this);
    title.setText("KIMIA Settings");
    title.setTextColor(Color.WHITE);
    title.setTextSize(20);
    title.setGravity(Gravity.CENTER);
    column.addView(title, new LinearLayout.LayoutParams(
        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

    gameSpinner = makeSpinner(GAME_LABELS, indexOf(GAME_VALUES, game));
    resolutionSpinner = makeSpinner(RESOLUTION_LABELS, resolutionIndex(width, height));
    fpsSpinner = makeSpinner(FPS_LABELS, indexOf(FPS_VALUES, fps));
    backendSpinner = makeSpinner(BACKEND_LABELS, indexOf(BACKEND_VALUES, backend));

    addRow(column, "Game", gameSpinner);
    addRow(column, "Resolution", resolutionSpinner);
    addRow(column, "Frame rate", fpsSpinner);
    addRow(column, "Renderer", backendSpinner);

    shadowsCheck = new CheckBox(this);
    shadowsCheck.setText("Shadows");
    shadowsCheck.setTextColor(Color.WHITE);
    shadowsCheck.setChecked(shadows);
    addRow(column, "Quality", shadowsCheck);

    msaaCheck = new CheckBox(this);
    msaaCheck.setText("Anti-aliasing (MSAA)");
    msaaCheck.setTextColor(Color.WHITE);
    msaaCheck.setChecked(msaa);
    addRow(column, "", msaaCheck);

    final Button apply = new Button(this);
    apply.setText("Apply & Restart");
    apply.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        game = GAME_VALUES[gameSpinner.getSelectedItemPosition()];
        final int[] res = RESOLUTION_VALUES[resolutionSpinner.getSelectedItemPosition()];
        width = res[0];
        height = res[1];
        fps = FPS_VALUES[fpsSpinner.getSelectedItemPosition()];
        backend = BACKEND_VALUES[backendSpinner.getSelectedItemPosition()];
        shadows = shadowsCheck.isChecked();
        msaa = msaaCheck.isChecked();
        saveSettings();
        settingsPanel.setVisibility(View.GONE);
        hideSystemBars();
        restartEngine();
      }
    });
    column.addView(apply, new LinearLayout.LayoutParams(
        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

    final Button done = new Button(this);
    done.setText("Close");
    done.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        settingsPanel.setVisibility(View.GONE);
        hideSystemBars();
      }
    });
    column.addView(done, new LinearLayout.LayoutParams(
        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

    final ScrollView scroll = new ScrollView(this);
    scroll.addView(column);

    final int panelWidth = dp(360);
    final FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(panelWidth,
        ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.CENTER);
    scrim.addView(scroll, lp);
    return scrim;
  }

  private Spinner makeSpinner(String[] labels, int selection) {
    final Spinner spinner = new Spinner(this);
    final ArrayAdapter<String> adapter = new ArrayAdapter<>(this,
        android.R.layout.simple_spinner_item, labels);
    adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
    spinner.setAdapter(adapter);
    spinner.setSelection(selection);
    return spinner;
  }

  private void addRow(LinearLayout column, String label, View control) {
    final LinearLayout row = new LinearLayout(this);
    row.setOrientation(LinearLayout.HORIZONTAL);
    row.setGravity(Gravity.CENTER_VERTICAL);
    if (label != null && !label.isEmpty()) {
      final TextView text = new TextView(this);
      text.setText(label);
      text.setTextColor(Color.LTGRAY);
      row.addView(text, new LinearLayout.LayoutParams(dp(110),
          ViewGroup.LayoutParams.WRAP_CONTENT));
    }
    row.addView(control, new LinearLayout.LayoutParams(0,
        ViewGroup.LayoutParams.WRAP_CONTENT, 1.0f));
    column.addView(row, new LinearLayout.LayoutParams(
        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
  }

  private void hideSystemBars() {
    getWindow().getDecorView().setSystemUiVisibility(
        View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            | View.SYSTEM_UI_FLAG_FULLSCREEN
            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION);
  }

  private int dp(int value) {
    return Math.round(value * getResources().getDisplayMetrics().density);
  }
}
