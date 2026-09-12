# بیلدهای تک‌فایلی (self-contained) — مثل یونیتی

هدف: یک فایل `.exe` برای ویندوز و یک فایل `.apk` برای اندروید که **همه‌چیز
داخلشان باشد** — بدون DLL جدا، بدون پوشهٔ داده کنار فایل، بدون نیاز به نصب
runtime. مثل یونیتی که assets را داخل خروجی می‌گذارد.

## چطور کار می‌کند

| لایه | روش |
| --- | --- |
| **جاسازی assets** | `Tools/make_embedded_assets.py` محتوای `Profiles/`، `Worlds/` و `Branding/` را موقع بیلد به یک فایل C++ (آرایهٔ بایت) تبدیل می‌کند و داخل باینری کامپایل می‌شود. |
| **استخراج در اجرا** | در اولین اجرا، `runWorldServer` (با `KIMIA_EMBEDDED_ASSETS`) assets را یک‌بار داخل یک پوشهٔ قابل‌نوشتن extract می‌کند (`kimia_engine/<version>/`) و مسیرهای پروفایل/دنیا/برندینگ را آنجا تنظیم می‌کند. |
| **runtime استاتیک (exe)** | CRT به‌صورت استاتیک (`/MT`) و SDL2 استاتیک (vcpkg triplet `x64-windows-static`) لینک می‌شود → هیچ DLL همراه لازم نیست. |
| **بستهٔ اندروید** | APK ذاتاً یک بستهٔ فشرده است؛ `.so` موتور + assets جاسازی‌شده داخلش است و در اجرا در پوشهٔ files app باز می‌شود. |

## گرفتن خروجی‌ها (بدون هیچ ابزار محلی)

هر دو از **GitHub Actions** ساخته می‌شوند؛ نتیجه را از تب Actions دانلود کن:

- **`kimia-world-windows-x64`** → یک فایل `kimia_world.exe` (خودکفا؛ D3D11 روی
  RTX 3060 با SDL2، و اگر GPU نبود به‌صورت خودکار software).
- **`kimia-world-debug-apk`** → `app-debug.apk`.

## ساخت محلی

```sh
# Windows: VS 2026 (Desktop C++ workload) + vcpkg با SDL2 استاتیک
vcpkg install sdl2:x64-windows-static
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 `
  -DKIMIA_EMBED_ASSETS=ON -DKIMIA_ENABLE_SDL2=ON -DKIMIA_BUILD_PC=ON `
  -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static `
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded
cmake --build build --config Release
```

خروجی: `build/bin/Release/kimia_world.exe`.

## نکته‌ها

- گزینهٔ `KIMIA_EMBED_ASSETS` پیش‌فرض خاموش است؛ بیلدهای Termux/PS4/Linux
  دست‌نخورده می‌مانند.
- در اجرای exe، اگر کاربر مسیر پروفایل/برندینگ را صریح با `--profiles` و
  `--branding` ندهد، مقادیر جاسازی‌شده استفاده می‌شوند.
- دنیای ساخته‌شده توسط کاربر (`my_world.kimia`) مثل قبل کنار فایل ذخیره
  می‌شود؛ خود باینری فقط assets پایه را حمل می‌کند.
- در آینده می‌توان همین مسیر را با فشرده‌سازی واقعی (zlib/miniz) و خروجی
  امضاشدهٔ release (signing) کامل کرد.
