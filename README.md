# KIMIA

موتور بازی C++17 و ویرایشگر گزینه‌محور **KIMIA World** — از صفر، بدون وابستگی اجباری و بدون کتابخانهٔ غیرآزاد.

نسخهٔ فعلی موتور: **`0.29.0`** (در `Engine/Core/include/kimia/Version.h`؛ تاریخچهٔ کامل در [CHANGELOG.md](CHANGELOG.md)).

> انشعاب کاری: همهٔ کار روی `arena/01a080a4-ai-codespace` است؛ `main` فقط اسکلت ابتدایی دارد.

---

## در یک نگاه

| قابلیت | خلاصه |
| --- | --- |
| هسته | انواع عددی، گزارش، پروفایلر، گام ثابت |
| ریاضی | `Vec2/3/4`، `Mat4`، `Quat`، دوربین — همگی header-only |
| صحنه | موجودیت‌ها با هندل ۱‌مبنا + فرمت متنی `SceneIO` v1 |
| فیزیک | گام ثابت ۱/۱۲۰، کره/جعبه/صفحه، برخورد دینامیک-دینامیک، باد، آب‌وهوا، کنترل‌گر کاراکتر |
| رندر | GL 3.3 / GLES3 / WebGL2 / D3D11 + رسترایزر نرم‌افزاری تضمینی؛ PBR، سایه، تون‌مپ |
| دارایی | OBJ، FBX، PNG/JPG، WAV/MP3/OGG/FLAC + ابزار خط فرمان |
| بازی | چهار بازی داخلی (گلف، فوتبال خیابانی، چمن، بتل گراند) فقط با فایل پروفایل |
| ویرایشگر | KIMIA Editor به سبک Unity (WebWorkbench) + منطق بصری بدون کدنویسی |
| خروجی | EXE و APK تک‌فایلی با دارایی‌های جاسازی‌شده؛ رندر آفلاین path tracer به PNG |

## خروجی‌های آماده

- **Windows** — یک `kimia_world.exe` خودکفا (D3D11 روی کارت گرافیک، fallback نرم‌افزاری).
- **Android** — یک APK بومی که روی `SurfaceView` با **GLES3** رندر می‌گیرد (fallback نرم‌افزاری)؛ بدون WebView.
- هر دو روی **GitHub Actions** ساخته می‌شوند (آرتیفکت‌ها: `kimia-world-windows-x64` و `kimia-world-debug-apk`).

جزئیات نصب و اجرا: [GettingStarted](Documentation/GettingStarted.md).

---

## ساخت و تست

```bash
cmake -B build -DKIMIA_WERROR=ON      # بدون build type = Release
cmake --build build -j4
./build/bin/kimia_tests               # 475/475 tests passed
ctest --test-dir build --output-on-failure
```

بیلد headless (بدون SDL2، مسیر WebViewer):

```bash
cmake -B build-nosdl -DKIMIA_ENABLE_SDL2=OFF -DKIMIA_WERROR=ON
cmake --build build-nosdl -j4
./build-nosdl/bin/kimia_world --port 8080   # سپس http://localhost:8080
```

## چهار بازی، یک موتور

همهٔ قابلیت‌ها در خود موتورند؛ هر بازی فقط یک **پروفایل** است (فایل متنی `Profiles/*.kimiaprofile` که بدون C++ قابل ویرایش است):

| بازی | پروفایل | زمین (X×Z) | توپ | ویژگی |
| --- | --- | --- | --- | --- |
| گلف کیمیا | `golf` | ۱۰×۲۴ | دقیق | حالت شوت، چند سوراخ، پار |
| فوتبال خیابانی: کوی ابوذر | `street` | ۱۶×۵ | فانتزی | ۵v۵، حرکات نمایشی |
| زمین چمن: کوی ابوذر | `grass` | ۴۰×۲۵ | دقیق | ۱۱v۱۱، قوانین، استقامت |
| بتل گراند | `battleground` | ۴۰×۴۰ | دقیق | آرنا ۴v۴، تیراندازی |

## ساختار

```
Engine/Core       هسته: انواع، گزارش، پروفایلر، گام ثابت
Engine/Math       بردار، ماتریس، کواترنیون، دوربین (header-only)
Engine/Graphics   مش و پریمیتیوها، بیت‌مپ‌فونت، اسکلت و انیمیشن، Image
Engine/Assets     خط لولهٔ دارایی: OBJ/FBX، تصویر، صدا
Engine/Scene      صحنه و SceneIO (فرمت متنی v1)
Engine/Physics    فیزیک گام‌ثابت + کنترل‌گر کاراکتر
Engine/Renderer   GL/GLES3/WebGL2/D3D11 + رسترایزر نرم‌افزاری
Engine/Platform   ورودی + پنجرهٔ اختیاری SDL2
Engine/App        بوت‌استرپ Engine + WebViewer + دارایی‌های جاسازی‌شده
Engine/Golf       بازی مرجع گلف
Engine/World      ویرایشگر گزینه‌محور و شبیه‌سازی بازی
Engine/Profile    پروفایل بازی (*.kimiaprofile)
Engine/Raytracer  ردیاب پرتو آفلاین (PBR + GI + BVH)
Profiles/         فایل‌های بازی‌ها (قابل ویرایش)
Worlds/           صحنه‌های نمونه
Examples/         برنامه‌های نمونه و ورودی‌های CLI
Tests/            سوئیت تست
Tools/            ابزارهای بیلد/بسته‌بندی/تولید دارایی
Android/          پروژهٔ Gradle برای APK
Web/              پوستهٔ WebGL (Emscripten)
```

---

## مستندات

| سند | موضوع |
| --- | --- |
| [GettingStarted](Documentation/GettingStarted.md) | نصب، بیلد و اجرا روی هر پلتفرم + خروجی‌های CI |
| [Architecture](Documentation/Architecture.md) | لایه‌های موتور و جریان داده |
| [Rendering](Documentation/Rendering.md) | مسیرهای رندر و خط لولهٔ رنگ |
| [Physics](Documentation/Physics.md) | موتور فیزیک |
| [Scene](Documentation/Scene.md) | صحنه، SceneIO و قراردادهای ریاضی |
| [WorldEditor](Documentation/WorldEditor.md) | ویرایشگر و رابط کاربری |
| [Games](Documentation/Games.md) | پروفایل‌ها، گیم‌پلی و کنترل‌ها |
| [Logic](Documentation/Logic.md) | منطق بصری، HUD، ورودی، ذرات، انتشار |
| [Animation](Documentation/Animation.md) | اسکلت، retarget و آدمک |
| [Assets](Documentation/Assets.md) | خط لولهٔ دارایی |
| [Android](Documentation/Android.md) | APK بومی (GLES3) |
| [WebGL](Documentation/WebGL.md) | Emscripten / WebGL2 |
| [Windows](Documentation/Windows.md) | بیلد PC (D3D11) |
| [PS4](Documentation/PS4.md) | لینوکس روی PS4 |

نقشهٔ راه: [ROADMAP.md](ROADMAP.md) — تاریخچه: [CHANGELOG.md](CHANGELOG.md).
