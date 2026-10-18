package com.termail.spical;

import android.app.Activity;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

public class GameWebView extends Activity {
    private WebView webView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(
            WindowManager.LayoutParams.FLAG_FULLSCREEN,
            WindowManager.LayoutParams.FLAG_FULLSCREEN
        );
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        try {
            if (android.os.Build.VERSION.SDK_INT >= 26) {
                if (android.webkit.WebView.getCurrentWebViewPackage() == null) {
                    showError("设备缺少 WebView 组件\n请安装 Android System WebView");
                    return;
                }
            }
        } catch (Throwable t) {
            showError("WebView 检查失败\n" + t.getMessage());
            return;
        }

        try {
            webView = new WebView(this);
        } catch (Throwable t) {
            showError("无法创建 WebView\n" + t.getMessage());
            return;
        }

        try {
            webView.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
            ));

            WebSettings settings = webView.getSettings();
            settings.setJavaScriptEnabled(true);
            settings.setDomStorageEnabled(true);
            settings.setAllowFileAccess(true);
            settings.setAllowContentAccess(true);
            settings.setAllowFileAccessFromFileURLs(true);
            settings.setAllowUniversalAccessFromFileURLs(true);
            settings.setMediaPlaybackRequiresUserGesture(false);
            settings.setLoadWithOverviewMode(true);
            settings.setUseWideViewPort(true);
            settings.setCacheMode(WebSettings.LOAD_DEFAULT);

            webView.setWebViewClient(new WebViewClient() {
                @Override
                public boolean shouldOverrideUrlLoading(WebView view, String url) {
                    view.loadUrl(url);
                    return true;
                }
            });

            String gameName = getIntent().getStringExtra("game");
            if (gameName == null || gameName.isEmpty()) {
                gameName = "2048";
            }

            String gameDir = copyGameToFiles(gameName);
            if (gameDir != null) {
                String url = "file://" + gameDir + "/index.html";
                webView.loadUrl(url);
            } else {
                String url = "file:///android_asset/games/" + gameName + "/index.html";
                webView.loadUrl(url);
            }

            setContentView(webView);
        } catch (Throwable t) {
            showError("加载游戏失败\n" + t.getMessage());
        }
    }

    private void showError(String msg) {
        android.widget.TextView tv = new android.widget.TextView(this);
        tv.setText("❌ 错误\n\n" + msg + "\n\n按返回键退出");
        tv.setTextColor(0xffffffff);
        tv.setBackgroundColor(0xff000000);
        tv.setPadding(60, 60, 60, 60);
        tv.setTextSize(16);
        tv.setGravity(android.view.Gravity.CENTER);
        setContentView(tv);
    }

    private String copyGameToFiles(String gameName) {
        try {
            java.io.File targetDir = new java.io.File(getFilesDir(), "games/" + gameName);
            if (targetDir.exists() && new java.io.File(targetDir, "index.html").exists()) {
                return targetDir.getAbsolutePath();
            }
            targetDir.mkdirs();
            copyAssetDir("games/" + gameName, targetDir);
            return targetDir.getAbsolutePath();
        } catch (Exception e) {
            return null;
        }
    }

    private void copyAssetDir(String assetPath, java.io.File target) throws Exception {
        String[] files = getAssets().list(assetPath);
        if (files == null || files.length == 0) {
            java.io.InputStream is = getAssets().open(assetPath);
            java.io.FileOutputStream fos = new java.io.FileOutputStream(target);
            byte[] buf = new byte[8192];
            int len;
            while ((len = is.read(buf)) > 0) {
                fos.write(buf, 0, len);
            }
            fos.close();
            is.close();
            return;
        }
        target.mkdirs();
        for (String f : files) {
            copyAssetDir(assetPath + "/" + f, new java.io.File(target, f));
        }
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK && webView != null && webView.canGoBack()) {
            webView.goBack();
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (webView != null) webView.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (webView != null) webView.onResume();
    }

    @Override
    protected void onDestroy() {
        if (webView != null) {
            webView.loadUrl("about:blank");
            webView.stopLoading();
            webView.destroy();
            webView = null;
        }
        super.onDestroy();
    }
}
