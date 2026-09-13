# شروع کار

نصب، بیلد و اجرای KIMIA روی هر پلتفرم، و گرفتن خروجی‌های آماده.

---

## خروجی‌های آماده (بدون ابزار محلی)

هر دو خروجی روی **GitHub Actions** ساخته می‌شوند و از تب **Actions** قابل دانلودند:

| آرتیفکت | محتوا |
| --- | --- |
| `kimia-world-windows-x64` | `kimia_world.exe` تک‌فایلی (D3D11 + fallback نرم‌افزاری) |
| `kimia-world-debug-apk` | `app-debug.apk` بومی (GLES3 + fallback نرم‌افزاری) |

مسیر: ریپو `javadxpro/AI-codespace` → تب **Actions** → اجرای `Build Windows EXE` یا `Build Android APK` → بخش **Artifacts**.

---

## لینوکس / دسکتاپ

پیش‌نیاز: CMake ۳٫۲۵+، یک کامپایلر C++17، و (اختیاری) SDL2 برای پنجرهٔ بومی.

```bash
cmake -B build -DKIMIA_WERROR=ON
cmake --build build -j4
./build/bin/kimia_tests          # 475/475 tests passed
```

اجرای ویرایشگر/بازی (WebViewer headless — بدون نیاز به SDL2):

```bash
cmake -B build-nosdl -DKIMIA_ENABLE_SDL2=OFF -DKIMIA_WERROR=ON
cmake --build build-nosdl -j4
./build-nosdl/bin/kimia_world --port 8080 --profiles build-nosdl/bin/profiles
# سپس در مرورگر: http://localhost:8080
# ویرایشگر:      http://localhost:8080/bench
```

گزینه‌های `kimia_world`: `--port N`، `--world FILE.kimia`، `--assets DIR`، `--profiles DIR`، `--version`، `--help`.

---

## گوشی اندروید (Termux)

```bash
pkg install -y git clang cmake ninja
git clone --branch arena/01a080a4-ai-codespace https://github.com/javadxpro/AI-codespace.git
cd AI-codespace
bash Tools/termux_build.sh         # ابزار → cmake → build → تست → دستور بعدی
./build/bin/kimia_world --port 8080 --profiles build/bin/profiles
```

اسکریپت شاخهٔ درست را تشخیص می‌دهد، ابزار گم‌شده را نصب می‌کند و با `-DKIMIA_WERROR=ON` در حالت Release می‌سازد. گزینه‌ها: `--run`، `--clean`، `--port=N`.

---

## اندروید (APK بومی)

APK از همان `CMakeLists.txt` ریپو ساخته می‌شود و موتور را **بومی** اجرا می‌کند — `SurfaceView` + EGL/GLES3، بدون WebView. جزئیات و ساخت محلی: [Android](Android.md).

---

## Windows (PC)

بیلد PC با Visual Studio 2026 و رندر D3D11. جزئیات، presetها و تنظیمات: [Windows](Windows.md).

---

## Web (WebGL2 / WebAssembly)

بیلد Emscripten که موتور را به WASM کامپایل می‌کند و مستقیم روی canvas می‌راند. جزئیات: [WebGL](WebGL.md).

---

## PS4 (لینوکس)

بازی headless (رندر نرم‌افزاری + WebViewer) روی لینوکسِ PS4. جزئیات: [PS4](PS4.md).

---

## بیلد تک‌فایلی (دارایی‌های جاسازی‌شده)

گزینهٔ `KIMIA_EMBED_ASSETS=ON` محتوای `Profiles/`، `Worlds/` و `Branding/` را هنگام بیلد به یک آرایهٔ بایت C++ تبدیل می‌کند و داخل باینری می‌گذارد؛ در اولین اجرا به پوشهٔ قابل‌نوشتن (`kimia_engine/<version>/`) استخراج می‌شوند. نتیجه یک `.exe` یا `.apk` است که هیچ فایل کناری لازم ندارد.

```bash
cmake -B build-embed -DKIMIA_EMBED_ASSETS=ON -DKIMIA_ENABLE_SDL2=OFF -DKIMIA_WERROR=ON
cmake --build build-embed -j4
```

همین مسیر در CI برای هر دو خروجی استفاده می‌شود.

---

## انتشار به بازیکن

بستهٔ آفلاین خودکفا (باینری + پروفایل‌ها + `play.sh` + لایسنس‌ها):

```bash
bash Tools/package_release.sh              # همهٔ بازی‌ها
bash Tools/package_release.sh --game=golf  # فقط گلف
```

اسکریپت فقط وقتی بسته می‌سازد که بیلد صفر اخطار باشد و کل سوئیت تست سبز باشد، و پیش از آرشیو، خودِ بسته را روی یک پورت یدکی اجرا و دودآزمایی می‌کند.

---
