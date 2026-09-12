package com.kimia.world;

import android.app.Activity;
import android.os.Bundle;
import android.view.Window;
import android.view.WindowManager;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

/**
 * Hosts the engine. The native library starts the KIMIA World server on
 * 127.0.0.1 and this activity shows it in a full-screen WebView, so the
 * whole editor/game — menus, builder, PLAY mode — runs on-device.
 */
public final class MainActivity extends Activity {
  private static final int PORT = 8080;

  private WebView webView;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    requestWindowFeature(Window.FEATURE_NO_TITLE);
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

    webView = new WebView(this);
    WebSettings settings = webView.getSettings();
    settings.setJavaScriptEnabled(true);
    settings.setDomStorageEnabled(true);          // intro "seen" flag + menu state
    settings.setMediaPlaybackRequiresUserGesture(false);
    settings.setCacheMode(WebSettings.LOAD_NO_CACHE);  // always fetch fresh frames
    webView.setWebViewClient(new WebViewClient());     // stay in-app, no browser
    setContentView(webView);

    NativeEngine.start(PORT, getFilesDir().getAbsolutePath());
    webView.loadUrl("http://127.0.0.1:" + PORT + "/");
  }

  @Override
  protected void onDestroy() {
    NativeEngine.stop();
    if (webView != null) {
      webView.destroy();
      webView = null;
    }
    super.onDestroy();
  }

  @Override
  public void onBackPressed() {
    if (webView != null && webView.canGoBack()) {
      webView.goBack();
      return;
    }
    super.onBackPressed();
  }
}
