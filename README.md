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
- [ ] GolfGame
- [ ] KIMIA World، ویرایشگر گزینه‌محور و PLAY در مرورگر

## ساخت و تست

```bash
cmake -B build -DKIMIA_WERROR=ON
cmake --build build -j4
./build/kimia_tests          # 85/85 tests passed
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
./build-nosdl/kimia_tests    # 85/85 tests passed
```

## نمایش در مرورگر (WebViewer)

```bash
./build/kimia_remote --port 8080
# سپس در مرورگر: http://localhost:8080
```

بدون GL هم کار می‌کند (رسترایزر نرم‌افزاری) — مناسب Termux. نمونهٔ تک‌فریم:

```bash
./build/kimia_first3d --frames 1 --capture frame.png
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
Examples/        # HelloWindow، First3DScene، RemoteView
Tools/           # kimia_assets (CLI)، kimia_asset_gen (دادهٔ تست)
Tests/           # سوئیت تست + دادهٔ تست
Documentation/   # مستندات هر زیرسیستم
ThirdParty/      # stb، dr_libs، ufbx (vendored)
```

مستندات در `Documentation/` و نقشهٔ راه در `ROADMAP.md`.
