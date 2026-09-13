# اندروید (APK بومی)

موتور به‌صورت یک APK **بومی** ارائه می‌شود: کتابخانهٔ `libkimia_jni.so` بازی را داخل پروسهٔ برنامه بوت می‌کند و مستقیماً روی `SurfaceView` با **GLES3** رندر می‌گیرد — بدون WebView و بدون سرور `127.0.0.1`.

اگر GLES3/EGL روی دستگاهی بالا نیامد، رسترایزر نرم‌افزاری به‌صورت خودکار جایگزین می‌شود؛ پس APK روی هر دستگاهی اجرا می‌شود.

---

## ساختار

| فایل | نقش |
| --- | --- |
| `Android/app/src/main/cpp/jni_glue.cpp` | پل JNI: بوت بازی، رندر EGL/GLES3 + fallback نرم‌افزاری، لمس، HUD و کنترل‌های روی صفحه |
| `.../java/com/kimia/world/MainActivity.java` | میزبان `SurfaceView` + صفحهٔ تنظیمات |
| `.../java/com/kimia/world/NativeEngine.java` | `System.loadLibrary("kimia_jni")` + متدهای native |
| `Android/app/build.gradle` | `externalNativeBuild` به `CMakeLists.txt` اصلی؛ فقط target `kimia_jni` |
| `Android/app/src/main/AndroidManifest.xml` | اپلیکیشن تمام‌صفحه، landscape؛ بدون مجوز اینترنت |

## چطور کار می‌کند

1. Java یک `SurfaceView` می‌سازد و `Surface` را با `nativeSetSurface` می‌دهد؛ اندازه با `nativeSurfaceChanged`.
2. `nativeStart` بازی انتخاب‌شده را می‌سازد (`golf/street/grass/battleground`) و حلقهٔ رندر را روی thread جدا می‌راند.
3. هر فریم: ورودی لمس → `WorldEditor::update` (گام ثابت) → `RenderScene` → رندر GLES3 → HUD/کنترل‌ها → `eglSwapBuffers`. در نبود GLES3، همان فریم با CPU رسم و روی بافر پنجره blit می‌شود.
4. `nativeTouch` لمس را نگاشت می‌کند؛ `nativeStop` حلقه را می‌بندد.

## لمس

| ناحیه | عمل |
| --- | --- |
| پایین-چپ (جوی‌استیک) | حرکت |
| پایین-راست (دکمه) | شوت (گلف، نگه‌داشتن = شارژ) / پرش (فوتبال) / آتش (آرنا) |
| وسط-راست (فقط آرنا) | پر کردن خشاب (Reload) |
| بقیهٔ صفحه | درگ = چرخش دوربین |
| دو انگشت | پینچ = زوم |

## تنظیمات داخل برنامه

دکمهٔ چرخ‌دندهٔ بالای صفحه یک پنل باز می‌کند: **بازی** (golf/street/grass/battleground)، **رزولوشن** (native/1280×720/960×540/640×360)، **نرخ فریم** (۳۰/۶۰)، **رندرر** (خودکار/GLES3/نرم‌افزاری)، **سایه**، **MSAA**. تنظیمات در `SharedPreferences` ذخیره می‌شوند و با «Apply & Restart» اعمال می‌شوند.

## دارایی‌های جاسازی‌شده

بیلد با `KIMIA_EMBED_ASSETS=ON` انجام می‌شود؛ `Profiles/`، `Worlds/` و `Branding/` داخل `libkimia_jni.so` هستند و در اولین اجرا به `files/kimia_engine/<version>/` استخراج می‌شوند — APK هیچ فایل کناری لازم ندارد.

## گرفتن APK

### GitHub Actions (پیشنهادی)

تب **Actions** → اجرای `Build Android APK` → **Artifacts** → `kimia-world-debug-apk` → نصب `app-debug.apk` (با اجازهٔ «نصب از منابع ناشناس»).

### ساخت محلی

```sh
# پیش‌نیاز: JDK 17، Android SDK (platform 34، build-tools 34، NDK 26.3، CMake 3.22)
cd Android
gradle assembleDebug
# خروجی: app/build/outputs/apk/debug/app-debug.apk
```

- ABIها: `arm64-v8a`، `armeabi-v7a`، `x86_64` (شبیه‌ساز).
- بیلد debug با کلید debug امضا می‌شود؛ برای انتشار رسمی signing لازم است.

## مهندسی (برای توسعه‌دهنده)

- کانتکست GLES3 با `EGLContext::createWindow(ANativeWindow*, w, h, msaa)` ساخته می‌شود (ES3 با fallback ES2)؛ توابع GL با `dlopen("libGLESv3.so")` بار می‌شوند.
- شیدرها `#version 300 es` + precision دارند؛ `glBufferData` و `glClearDepthf` shim شده‌اند (اندازهٔ pointer روی ABIهای ۳۲/۶۴ بیتی).
- CMake برای `kimia_jni` به `liblog` و `libandroid` لینک می‌شود؛ EGL/GLES در اجرا باز می‌مانند (وابستگی build-time به GPU نیست).

---
