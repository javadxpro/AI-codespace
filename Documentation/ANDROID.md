# KIMIA روی اندروید (APK)

موتور به‌صورت یک APK بومی ارائه می‌شود. معماری آن همان مسیر اثبات‌شدهٔ
PS4/Termux است: کتابخانهٔ بومی موتور (`libkimia_jni.so`) سرور WebViewer را
داخل خودِ برنامه روی `127.0.0.1` بالا می‌آورد و یک WebView تمام‌صفحه همان
صفحهٔ بازی/ادیتور را نمایش می‌دهد. فیزیک، منطق و رندر نرم‌افزاری دقیقاً همان
کدی است که روی PS4 و Termux اجرا می‌شود؛ گرافیک GPU در این نسخهٔ اول از مسیر
نرم‌افزاری می‌آید (دقیقاً مثل PS4 headless) و کیفیتش با آن یکی است.

## دو راه برای گرفتن APK

### ۱) GitHub Actions (بدون هیچ ابزار محلی — پیشنهادی)
بعد از push، workflow `.github/workflows/android-apk.yml` به‌صورت خودکار APK
را روی سرورهای گیتهاب می‌سازد:

1. در ریپو `javadxpro/AI-codespace` به تب **Actions** برو.
2. روی اجرای **Build Android APK** کلیک کن (یا اولین اجرای خودکار بعد از push).
3. در پایین صفحه، بخش **Artifacts** → `kimia-world-debug-apk` را دانلود کن.
4. ZIP را باز کن و `app-debug.apk` را روی گوشی نصب کن (اجازهٔ «نصب از
   منابع ناشناس» را بده).

### ۲) ساخت محلی با Android Studio
1. Android Studio (نسخهٔ جدید) را نصب کن.
2. ریپو را کلون کن (فقط شاخهٔ کار):
   ```sh
   git clone --depth 1 -b arena/01a080a4-ai-codespace \
     https://github.com/javadxpro/AI-codespace.git
   ```
3. پوشهٔ `Android/` را به‌عنوان پروژه باز کن. Android Studio به‌صورت خودکار
   NDK و CMake لازم را دانلود می‌کند.
4. `Build → Build Bundle(s) / APK(s) → Build APK(s)`.

یا از خط فرمان (اگر JDK 17 و Android SDK و NDK 26.3 داری):
```sh
cd Android
gradle assembleDebug
# خروجی: app/build/outputs/apk/debug/app-debug.apk
```

- ABIها: `arm64-v8a` (گوشی‌های مدرن ازجمله Poco X3 Pro)، `armeabi-v7a` و
  `x86_64` (شبیه‌ساز). هر سه در APK قرار می‌گیرند.
- **تک‌فایلی:** بیلد APK با `KIMIA_EMBED_ASSETS=ON` انجام می‌شود، یعنی
  `Profiles/`، `Worlds/` و `Branding/` داخل `libkimia_jni.so` جاسازی و در
  اولین اجرا در پوشهٔ files برنامه extract می‌شوند — APK هیچ فایل کناری
  لازم ندارد. جزئیات: `Documentation/ONE_FILE_BUILDS.md`.
- رزولوشن/فریم پیش‌فرض در APK سبک‌تر از دسکتاپ تنظیم شده است
  (۴۸۰×۳۶۰ @ ۲۰fps، JPEG کیفیت ۷۰) تا CPU گوشی داغ نکند. می‌توانی همین
  مقادیر را در `jni_glue.cpp` تغییر دهی.

## ساختار

| فایل | نقش |
| --- | --- |
| `Android/app/src/main/cpp/jni_glue.cpp` | پل JNI؛ `runWorldServer` را روی thread اجرا می‌کند. |
| `Android/app/src/main/java/com/kimia/world/NativeEngine.java` | `System.loadLibrary("kimia_jni")` + متدهای native. |
| `Android/app/src/main/java/com/kimia/world/MainActivity.java` | WebView تمام‌صفحه که `http://127.0.0.1:8080` را نمایش می‌دهد. |
| `Android/app/build.gradle` | `externalNativeBuild` به `CMakeLists.txt` اصلی ریپو اشاره می‌کند؛ فقط target `kimia_jni` را می‌سازد. |
| `Engine/App/include/kimia/WorldServer.h` | نقطهٔ ورود مشترک CLI و اندروید. |

## نکته‌ها

- APK اشکال‌زدایی (debug) است و با کلید debug امضا می‌شود؛ برای انتشار
  عمومی باید signing رسمی در `Android/app/build.gradle` اضافه شود.
- قدم بعدی برای گرافیک GPU بومی: پورت رندرر GL به GLES 3 (کار WebGL2/GLES3
  که برای Emscripten انجام شد مستقیماً قابل استفاده است) روی یک
  `ANativeWindow`/`SurfaceView` به‌جای WebView.
