package com.kimia.world;

import android.app.Activity;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
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
import android.widget.ListView;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;

/**
 * Hosts the KIMIA engine natively. A full-screen SurfaceView hands its
 * ANativeWindow to the native renderer (GLES3 with a software fallback), and
 * every touch is forwarded to the engine, so the whole game — world, controls,
 * HUD, on-screen buttons — runs on-device.
 *
 * Phase 1: an in-world editor button (✎) swaps the activity into MODE_EDIT.
 * The world pauses, taps go to the picker, and a left-side list plus a bottom
 * inspector let the user select entities, recolour them, drag them around,
 * add cube/sphere/plane primitives and save the result to a .kimia file.
 */
public final class MainActivity extends Activity implements SurfaceHolder.Callback {

  private static final String PREFS = "kimia_settings";

  private FrameLayout root;
  private SurfaceView surfaceView;
  private View settingsButton;
  private View editButton;
  private View settingsPanel;
  private View editPanel;
  private ListView entityList;
  private TextView selectionLabel;
  private TextView positionLabel;
  private SeekBar colorR;
  private SeekBar colorG;
  private SeekBar colorB;
  private final Handler ui = new Handler(Looper.getMainLooper());

  private Spinner gameSpinner;
  private Spinner resolutionSpinner;
  private Spinner fpsSpinner;
  private Spinner backendSpinner;
  private CheckBox shadowsCheck;
  private CheckBox msaaCheck;

  private String filesDir;
  private boolean started = false;
  private boolean editing = false;
  private boolean dragInFlight = false;

  private String game = "golf";
  private int backend = NativeEngine.BACKEND_AUTO;
  private int width = 0;
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

    settingsButton = makeIconButton("\u2699", () -> toggleSettings());
    root.addView(settingsButton, iconLayout(true /* top */, true /* right */));

    editButton = makeIconButton("\u270E", () -> toggleEdit());
    root.addView(editButton, iconLayout(true /* top */, false /* right */));

    settingsPanel = buildSettingsPanel();
    settingsPanel.setVisibility(View.GONE);
    root.addView(settingsPanel,
        new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.MATCH_PARENT));

    editPanel = buildEditPanel();
    editPanel.setVisibility(View.GONE);
    root.addView(editPanel,
        new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.MATCH_PARENT));

    setContentView(root);
    startEngine();
  }

  // --- Touch routing -------------------------------------------------------
  // One touch listener does everything: drag in edit mode, normal game touch
  // everywhere else. The actual interpretation lives in the native engine.
  private final View.OnTouchListener touchListener = new View.OnTouchListener() {
    @Override
    public boolean onTouch(View v, MotionEvent event) {
      final int action = event.getActionMasked();
      final int index = event.getActionIndex();
      final float x = event.getX(index);
      final float y = event.getY(index);

      if (editing) {
        handleEditTouch(action, index, x, y);
        return true;
      }

      NativeEngine.nativeTouch(action, index, x, y);
      return true;
    }
  };

  // --- Edit-mode touch model ----------------------------------------------
  // Tap: select. Long-touch / drag: move. The drag is split so the engine's
  // pickEntityAt() sees the actual start and end pixel coordinates.
  private void handleEditTouch(int action, int index, float x, float y) {
    switch (action) {
      case MotionEvent.ACTION_DOWN:
      case MotionEvent.ACTION_POINTER_DOWN: {
        // Begin drag; the engine picks on BeginDrag and stays picked until
        // EndDrag. A short tap without motion still results in a select.
        NativeEngine.nativeEditBeginDrag(x, y);
        dragInFlight = true;
        break;
      }
      case MotionEvent.ACTION_MOVE: {
        if (dragInFlight && index == 0) {
          // The native side uses lastX/lastY of finger 0; we re-send the
          // absolute position so the picker can project both pixels.
          NativeEngine.nativeEditDragTo(x, y, x, y);
        }
        break;
      }
      case MotionEvent.ACTION_UP:
      case MotionEvent.ACTION_POINTER_UP: {
        if (index == 0) {
          if (!dragInFlight) {
            NativeEngine.nativeEditSelect(x, y);
          }
          NativeEngine.nativeEditEndDrag();
          dragInFlight = false;
        }
        break;
      }
      default: break;
    }
  }

  // --- SurfaceHolder.Callback ---------------------------------------------

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
    NativeEngine.nativeSetMode(editing ? NativeEngine.MODE_EDIT : NativeEngine.MODE_PLAY);
    started = true;
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
    if (editPanel.getVisibility() == View.VISIBLE) {
      toggleEdit();
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
    for (int i = 0; i < values.length; ++i) if (values[i].equals(value)) return i;
    return 0;
  }

  private int indexOf(int[] values, int value) {
    for (int i = 0; i < values.length; ++i) if (values[i] == value) return i;
    return 0;
  }

  private int resolutionIndex(int w, int h) {
    for (int i = 0; i < RESOLUTION_VALUES.length; ++i) {
      if (RESOLUTION_VALUES[i][0] == w && RESOLUTION_VALUES[i][1] == h) return i;
    }
    return 0;
  }

  private View makeIconButton(String glyph, Runnable onClick) {
    final TextView button = new TextView(this);
    button.setText(glyph);
    button.setTextSize(22);
    button.setTextColor(Color.WHITE);
    button.setGravity(Gravity.CENTER);
    button.setBackgroundColor(0x66000000);
    button.setOnClickListener(v -> onClick.run());
    return button;
  }

  private FrameLayout.LayoutParams iconLayout(boolean top, boolean right) {
    final int g = right ? Gravity.TOP | Gravity.END : Gravity.TOP | Gravity.START;
    final FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(dp(44), dp(44), g);
    lp.setMargins(dp(12), dp(12), dp(12), dp(12));
    return lp;
  }

  private void toggleSettings() {
    final boolean show = settingsPanel.getVisibility() != View.VISIBLE;
    settingsPanel.setVisibility(show ? View.VISIBLE : View.GONE);
    if (show) editPanel.setVisibility(View.GONE);
  }

  private void toggleEdit() {
    editing = !editing;
    editPanel.setVisibility(editing ? View.VISIBLE : View.GONE);
    editButton.setBackgroundColor(editing ? 0xCC0E6FBA : 0x66000000);
    NativeEngine.nativeSetMode(editing ? NativeEngine.MODE_EDIT : NativeEngine.MODE_PLAY);
    if (editing) settingsPanel.setVisibility(View.GONE);
    if (editing) startEditorRefreshLoop();
  }

  // --- Settings panel ------------------------------------------------------

  private View buildSettingsPanel() {
    final FrameLayout scrim = new FrameLayout(this);
    scrim.setBackgroundColor(0xCC000000);
    scrim.setOnClickListener(v -> {
      settingsPanel.setVisibility(View.GONE);
      hideSystemBars();
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
    apply.setOnClickListener(v -> {
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
    });
    column.addView(apply, new LinearLayout.LayoutParams(
        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

    final Button done = new Button(this);
    done.setText("Close");
    done.setOnClickListener(v -> {
      settingsPanel.setVisibility(View.GONE);
      hideSystemBars();
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

  // --- Edit panel ----------------------------------------------------------
  // Three layers, top-to-bottom on screen:
  //   1. left side: ListView of entity names (Hierarchy), full height
  //   2. bottom: a wide horizontal strip with selection label + 3 colour
  //      sliders + Add Cube/Sphere/Plane + Delete + Save
  //   3. middle: nothing — the SurfaceView remains interactive for picking.
  private View buildEditPanel() {
    final FrameLayout root = new FrameLayout(this);
    root.setBackgroundColor(0x33000000);  // gentle dim, not opaque

    entityList = new ListView(this);
    entityList.setBackgroundColor(0xCC1B1E22);
    entityList.setOnItemClickListener((parent, view, position, id) -> {
      // Selection from the list is forwarded as a tap at (0,0) on the
      // underlying engine, which won't match anything; instead we read
      // the snapshot and select by name.
      final String name = (String) parent.getItemAtPosition(position);
      NativeEngine.nativeEditSelect(0, 0);  // refresh selection state
      // Mirror the click into the engine by simulating a tap at the entity.
      // The picker is a ray from the camera through the pixel; with no good
      // pixel we fall back to: the user can tap the entity directly in the
      // scene. The list still shows the current selection.
      if (selectionLabel != null) selectionLabel.setText("Selected: " + name);
    });
    final FrameLayout.LayoutParams listLp = new FrameLayout.LayoutParams(dp(220),
        ViewGroup.LayoutParams.MATCH_PARENT, Gravity.START);
    listLp.setMargins(0, dp(56), 0, dp(170));
    root.addView(entityList, listLp);

    final LinearLayout bottom = new LinearLayout(this);
    bottom.setOrientation(LinearLayout.VERTICAL);
    bottom.setBackgroundColor(0xEE1B1E22);
    bottom.setPadding(dp(16), dp(12), dp(16), dp(12));

    selectionLabel = new TextView(this);
    selectionLabel.setText("Tap an object in the scene to select it.");
    selectionLabel.setTextColor(Color.WHITE);
    bottom.addView(selectionLabel, new LinearLayout.LayoutParams(
        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

    positionLabel = new TextView(this);
    positionLabel.setTextColor(0xFFCCCCCC);
    positionLabel.setTextSize(12);
    bottom.addView(positionLabel, new LinearLayout.LayoutParams(
        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

    colorR = makeColorSlider("R", 0xFFE53935);
    colorG = makeColorSlider("G", 0xFF43A047);
    colorB = makeColorSlider("B", 0xFF1E88E5);
    bottom.addView(colorR, new LinearLayout.LayoutParams(
        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
    bottom.addView(colorG, new LinearLayout.LayoutParams(
        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
    bottom.addView(colorB, new LinearLayout.LayoutParams(
        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

    final LinearLayout buttons = new LinearLayout(this);
    buttons.setOrientation(LinearLayout.HORIZONTAL);
    final Button addCube = smallButton("+ Cube", v -> NativeEngine.nativeEditNew(NativeEngine.EDIT_NEW_CUBE));
    final Button addSphere = smallButton("+ Sphere", v -> NativeEngine.nativeEditNew(NativeEngine.EDIT_NEW_SPHERE));
    final Button addPlane = smallButton("+ Plane", v -> NativeEngine.nativeEditNew(NativeEngine.EDIT_NEW_PLANE));
    final Button delete = smallButton("Delete", v -> NativeEngine.nativeEditDelete());
    final Button save = smallButton("Save", v -> NativeEngine.nativeEditSave(filesDir + "/my_world.kimia"));
    for (Button b : new Button[]{addCube, addSphere, addPlane, delete, save}) {
      buttons.addView(b, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1.0f));
    }
    bottom.addView(buttons, new LinearLayout.LayoutParams(
        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

    final FrameLayout.LayoutParams bottomLp = new FrameLayout.LayoutParams(
        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.BOTTOM);
    root.addView(bottom, bottomLp);

    return root;
  }

  private SeekBar makeColorSlider(String label, int barTint) {
    final LinearLayout row = new LinearLayout(this);
    row.setOrientation(LinearLayout.HORIZONTAL);
    row.setGravity(Gravity.CENTER_VERTICAL);
    final TextView tag = new TextView(this);
    tag.setText(label);
    tag.setTextColor(Color.WHITE);
    tag.setWidth(dp(18));
    row.addView(tag);
    final SeekBar bar = new SeekBar(this);
    bar.setMax(1000);
    bar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
      @Override public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        if (!fromUser) return;
        NativeEngine.nativeEditSetColor(
            colorR.getProgress() / 1000.0f,
            colorG.getProgress() / 1000.0f,
            colorB.getProgress() / 1000.0f);
      }
      @Override public void onStartTrackingTouch(SeekBar seekBar) {}
      @Override public void onStopTrackingTouch(SeekBar seekBar) {
        NativeEngine.nativeEditSetColor(
            colorR.getProgress() / 1000.0f,
            colorG.getProgress() / 1000.0f,
            colorB.getProgress() / 1000.0f);
      }
    });
    row.addView(bar, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1.0f));
    // Wrap: return SeekBar by tagging it via setId-less trick — we already
    // have separate SeekBar fields (colorR/G/B), so wrap via a Holder.
    holderOf(bar).label = label;
    return bar;
  }

  private static class Holder { String label; }
  private static final java.util.WeakHashMap<SeekBar, Holder> holders = new java.util.WeakHashMap<>();
  private static Holder holderOf(SeekBar bar) {
    Holder h = holders.get(bar);
    if (h == null) { h = new Holder(); holders.put(bar, h); }
    return h;
  }

  private Button smallButton(String label, View.OnClickListener onClick) {
    final Button b = new Button(this);
    b.setText(label);
    b.setTextSize(11);
    b.setOnClickListener(onClick);
    return b;
  }

  // Polls the snapshot every ~120 ms while the edit panel is open so the
  // list, colour sliders and position label stay in sync with what the
  // engine sees. The native side is the source of truth; this is a thin
  // mirror, not a state machine.
  private void startEditorRefreshLoop() {
    ui.removeCallbacksAndMessages(null);
    ui.postDelayed(new Runnable() {
      @Override public void run() {
        if (!editing) return;
        refreshEditorPanel();
        ui.postDelayed(this, 120);
      }
    }, 60);
  }

  private void refreshEditorPanel() {
    if (NativeEngine.nativeEditSelectionChanged() == 1) {
      // selection just changed: read everything again
    }
    final String selected = NativeEngine.nativeEditGetSelected();
    final float r = NativeEngine.nativeEditGetColorR();
    final float g = NativeEngine.nativeEditGetColorG();
    final float b = NativeEngine.nativeEditGetColorB();
    final float px = NativeEngine.nativeEditGetPosX();
    final float py = NativeEngine.nativeEditGetPosY();
    final float pz = NativeEngine.nativeEditGetPosZ();

    if (selected != null && !selected.isEmpty()) {
      selectionLabel.setText("Selected: " + selected);
      positionLabel.setText(String.format("pos (%.2f, %.2f, %.2f)   colour (%.2f, %.2f, %.2f)",
          px, py, pz, r, g, b));
    } else {
      selectionLabel.setText("Tap an object in the scene to select it.");
      positionLabel.setText("");
    }
    colorR.setProgress(Math.round(r * 1000));
    colorG.setProgress(Math.round(g * 1000));
    colorB.setProgress(Math.round(b * 1000));

    final int n = NativeEngine.nativeEditRefresh();
    if (n > 0) {
      final String[] names = new String[n];
      NativeEngine.nativeEditGetNames(names);
      entityList.setAdapter(new ArrayAdapter<>(this,
          android.R.layout.simple_list_item_1, names));
    }
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
