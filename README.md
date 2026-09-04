# KIMIA

موتور بازی C++17 و ویرایشگر گزینه‌محور KIMIA World — ساخته‌شده از صفر، بدون وابستگی غیرآزاد.

## وضعیت

- [x] هستهٔ مستقل (Types، Log، Profiler، FixedTimeStep)
- [x] ریاضیات (Vec2/3/4، Mat4، Quat، Camera)
- [x] گرافیک و خط لولهٔ دارایی (Primitive Meshes، OBJ، FBX، PNG/JPG، WAV/MP3)
- [x] Scene و SceneIO (هندل‌های ۱-مبنا + فرمت متنی v1)
- [x] فیزیک (گام ثابت ۱/۱۲۰، کره/صفحه/AABB، استرداد و اصطکاک)
- [x] Renderer (GL 3.3 با لودر dlopen + EGL، سایهٔ PCF، رسترایزر نرم‌افزاری)
- [x] Platform (ورودی LEVEL/EDGE + پنجرهٔ SDL2 اختیاری)
- [x] App — WebViewer (فریم‌ها در مرورگر، localhost:8080) و بوت‌استرپ Engine
- [x] GolfGame — بازی مرجع با نشانه‌گیری/شارژ/شوت، سازندهٔ زمین و `# demo`
- [x] KIMIA World — سازندهٔ بازی: زمین خالی، اجسام با سؤال و پاسخ، مدیریت و رنگ، PLAY در مرورگر
- [x] فرمت‌ها — OBJ+MTL (متریال/تکسچر و زیر-مش)، متریال و تکسچر FBX، OGG/FLAC، راهنمای .blend
- [x] فیزیک پیشرفته ۱ — جسم‌های دینامیک: جعبه‌هایی که می‌افتند، روی هم چیده می‌شوند، هل داده و شوت می‌شوند و با توپ برخورد می‌کنند
- [x] فایل در صحنه — «مدل از فایل» در کاتالوگ: فایل‌های OBJ/FBX پوشهٔ assets را در صحنه می‌گذاری (اندازهٔ دلخواه، نرمال‌سازی خودکار ابعاد، ذخیره/بارگذاری) + فیکس اسپاون توپ داخل جسم‌ها
- [x] ردیاب پرتو آفلاین — رندر هر دنیای `.kimia` به PNG با path tracing: PBR Cook-Torrance، روشنایی سراسری تا N جهش، آسمان + خورشید با سایهٔ نرم، BVH، خروجی دترمینیستیک (بایت-به-بایت یکسان)

## ساخت و تست

```bash
cmake -B build -DKIMIA_WERROR=ON
cmake --build build -j4
./build/bin/kimia_tests      # 161/161 tests passed
ctest --test-dir build --output-on-failure
```

بیلد دوم بدون `-Werror` (شمارش اخطار باید ۰ باشد):

```bash
cmake -B build-warn -DKIMIA_WERROR=OFF
cmake --build build-warn -j4 2>&1 | grep -ci warning   # 0
```

بیلد headless (بدون SDL2) — WebViewer به‌جای پنجره:

```bash
cmake -B build-nosdl -DKIMIA_ENABLE_SDL2=OFF -DKIMIA_WERROR=ON
cmake --build build-nosdl -j4
./build-nosdl/bin/kimia_tests    # 161/161 tests passed
```

## KIMIA World — ویرایشگر گزینه‌محور

```bash
./build/bin/kimia_world --port 8080
# سپس در مرورگر: http://localhost:8080
```

همه‌چیز با دکمه‌ها: Create World (زمین خالی) → افزودن جسم (بازیکن، توپ،
بلوک، دیوار، دروازه — هر کدام با سؤال فارسی مثل «توپ: دقیق باشه یا
فانتزی؟») → مدیریت اجسام (جابه‌جایی/حذف/رنگ) → محیط → PLAY.
دنیاها در فرمت SceneIO-v1 ذخیره می‌شوند و فایل‌های قدیمی `.kimia` هم
بار می‌شوند. جزئیات در [Documentation/World.md](Documentation/World.md).

## ردیاب پرتو — رندر آفلاین (path tracer)

هر دنیایی که در KIMIA World ساخته و ذخیره کرده‌ای را می‌توانی با
ردیاب پرتو به PNG رندر کنی (تمام CPU؛ بدون نیاز به GPU — روی Termux
هم اجرا می‌شود):

```bash
./build/bin/kimia_raytrace دنیای_من.kimia out.png
./build/bin/kimia_raytrace دنیای_من.kimia big.png \
    --width 960 --height 720 --spp 64 --bounces 4 \
    --eye 0 4.5 10 --target 0 0.5 -1 \
    --sun 0.45 0.5 0.45 --intensity 2.8 --sky 0.7 --exposure 1.0
# خروجی نمونه:
# RENDER دنیای_من.kimia -> big.png | 960x720 | 64 spp | 4 bounces | T s | 1662 triangles
```

- **PBR واقعی**: Cook-Torrance GGX + Fresnel شلیک برای خورشید، بازتاب آسمان
  به‌عنوان محیط، روشنایی سراسری (GI) با جهش‌های مسیر تا `--bounces`.
- **سایهٔ نرم خورشید**: خورشید یک قرص با مخروط نمونه‌برداری است (نیم‌سایهٔ واقعی).
- **BVH**: شتاب‌دهی درخت جعبه‌ها؛ `--brute` همان تصویر را بدون BVH می‌دهد
  (بایت-به-بایت یکسان — تست دارد).
- **دترمینیستیک**: RNG هر پیکسل seed ثابت دارد؛ رندر یک دنیا دو بار،
  همیشه فایل خروجی بایت-به-بایت یکسانی می‌دهد.
- مدل‌های «مدل از فایل» (OBJ/FBX) با رنگ/زبری موجودیت رندر می‌شوند
  (نمونه‌برداری تکسچر diffuse در ردیاب پرتو، مورد بعدی نقشهٔ راه است).

جزئیات کامل در [Documentation/TechnicalSpec.md](Documentation/TechnicalSpec.md) (بخش ردیاب پرتو).

## بازی گلف در مرورگر (WebViewer)

```bash
./build/bin/kimia_golf --web --port 8080
# سپس در مرورگر: http://localhost:8080
```

a/d نشانه‌گیری، نگه‌داشتن SPACE شارژ قدرت، رها کردن شوت. سازندهٔ زمین با
`--edit`: 1/2/3 ابزار، WASD حرکت، Enter قرار دادن، S ذخیره، L بارگذاری،
F بازی. جزئیات در [Documentation/Golf.md](Documentation/Golf.md).

نمای نمونهٔ RemoteView و First3DScene:

```bash
./build/bin/kimia_remote --port 8080
./build/bin/kimia_first3d --frames 1 --capture frame.png
```

## خط لولهٔ دارایی

```bash
./build/kimia_assets_cli model.fbx texture.png sound.mp3
```

فرمت‌های پشتیبانی‌شده: **OBJ، FBX** (مش) — **PNG، JPG** (تصویر) — **WAV، MP3**
(صدا). جزئیات در [Documentation/Assets.md](Documentation/Assets.md).

## ساختار

```
Engine/Core      # انواع، گزارش‌گیری، پروفایلر، گام ثابت
Engine/Math      # بردارها، ماتریس‌ها، کواترنیون، دوربین (header-only)
Engine/Graphics  # MeshData، پریمیتیوها، OBJ، Image (PNG/JPG)
Engine/Assets    # AssetPipeline، Audio (WAV/MP3)، FBX
Engine/Scene     # Scene (هندل‌های ۱-مبنا) و SceneIO (فرمت متنی v1)
Engine/Physics   # PhysicsWorld: گام ثابت، کره/صفحه/AABB، استرداد و اصطکاک
Engine/Renderer  # GL/EGL (dlopen) + سایه + رسترایزر نرم‌افزاری
Engine/Platform  # ورودی + پنجرهٔ SDL2 اختیاری
Engine/App       # WebViewer (HTTP) و بوت‌استرپ Engine
Engine/Golf      # بازی مرجع: حالت‌ها، سازندهٔ زمین، شات نمایشی
Engine/World     # ویرایشگر گزینه‌محور: WorldData، WorldIO، شبیه‌سازی بازی
Engine/Raytracer # ردیاب پرتو آفلاین: PBR + GI + BVH + خروجی PNG دترمینیستیک
Examples/        # HelloWindow، First3DScene، RemoteView، GolfGame، WorldEditorApp
Tools/           # kimia_assets (CLI)، kimia_raytrace (رندر آفلاین)، kimia_asset_gen (دادهٔ تست)
Tests/           # سوئیت تست + دادهٔ تست
Documentation/   # مستندات هر زیرسیستم
ThirdParty/      # stb، dr_libs، ufbx (vendored)
```

مستندات در `Documentation/` و نقشهٔ راه در `ROADMAP.md`.
