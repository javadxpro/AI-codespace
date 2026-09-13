# معماری

لایه‌های موتور KIMIA و جریان داده میان آن‌ها.

---

## لایه‌ها (از پایین به بالا)

| لایه | مسئولیت |
| --- | --- |
| `Engine/Core` | انواع عددی، گزارش، پروفایلر، گام ثابت |
| `Engine/Math` | بردار، ماتریس، کواترنیون، دوربین — header-only، بدون وابستگی |
| `Engine/Graphics` | `MeshData` و پریمیتیوها، بیت‌مپ‌فونت، اسکلت/انیمیشن، `Image` |
| `Engine/Assets` | خط لولهٔ دارایی: OBJ/FBX، تصویر PNG/JPG، صدا WAV/MP3/OGG/FLAC |
| `Engine/Scene` | موجودیت‌ها (هندل ۱‌مبنا) و `SceneIO` (فرمت متنی v1) |
| `Engine/Physics` | فیزیک گام‌ثابت + کنترل‌گر کاراکتر |
| `Engine/Renderer` | GL/GLES3/WebGL2/D3D11 + رسترایزر نرم‌افزاری |
| `Engine/Platform` | ورودی (LEVEL/EDGE) + پنجرهٔ اختیاری SDL2 |
| `Engine/App` | بوت‌استرپ `Engine`، WebViewer، دارایی‌های جاسازی‌شده |
| `Engine/Golf` | بازی مرجع گلف (`GolfGame` + سازندهٔ زمین) |
| `Engine/World` | `WorldEditor` — شبیه‌سازی بازی و پرسش/پاسخ سازنده |
| `Engine/Profile` | `GameProfile` + فرمت `*.kimiaprofile` |
| `Engine/Raytracer` | ردیاب پرتو آفلاین (PBR + GI + BVH) |

## وابستگی‌ها

فقط رو به بالا است؛ لایه‌های پایین چیزی از بالایی نمی‌دانند:

```
Core → Math → Graphics/Assets/Scene → Physics/Renderer → World/Profile → App
```

دو نقطهٔ عمدیِ جداسازی:

- **رندر** پشت `Renderer`/`renderSoftware` پنهان است؛ `World`/`App` فقط یک `RenderScene` می‌سازند و نمی‌دانند با GL، GLES3، D3D11 یا CPU رسم می‌شود.
- **منطق بازی** در `LogicRuntime` می‌گوید *چه* باید بشود؛ دنیا آن را انجام می‌دهد. منطق بدون رندر، پنجره یا فیزیک تست می‌شود.

## جریان یک فریم (Play)

```
poll ورودی (پنجره/وب/لمس) → editor.update(گام ثابت) → buildScene → render → HUD → present
```

حلقهٔ مرجع در `Examples/WorldEditorApp.cpp` است؛ نسخهٔ اندروید همان گام‌ها را در `Android/app/src/main/cpp/jni_glue.cpp` اجرا می‌کند (لمس به `WorldEditor` + `OrbitCamera` نگاشت می‌شود).

## قراردادها

- C++17، بدون استثنا در مسیر داغ، `-Werror` (کلنگ/GCC) و `/W4 /WX` (MSVC).
- شماره‌گذاری معنایی نسخه در `Version.h`؛ دفترچهٔ `CHANGELOG.md` با نسخهٔ فعلی هم‌خوان است (تست دارد).
- هر زیرسیستم پیش از رفتن به مرحلهٔ بعد، تست مستقل دارد.

---
