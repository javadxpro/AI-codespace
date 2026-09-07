# KIMIA PC Rebuild

این سند تصمیم‌های بازسازی نسخهٔ اختصاصی KIMIA برای Windows PC را ثبت می‌کند.
نسخهٔ PC برای استفادهٔ شخصی صاحب پروژه طراحی می‌شود و هدف آن compatibility عمومی
با همهٔ دستگاه‌ها نیست.

## هدف سخت‌افزاری

- GPU اصلی: NVIDIA RTX 3060
- حافظه: 16 GB RAM
- سیستم‌عامل: Windows X Lite، مشروط به نصب بودن driver رسمی NVIDIA و runtimeهای
  معمول DirectX 11
- toolchain: MSVC + Visual Studio + CMake
- renderer اصلی: Direct3D 11
- renderer ثانویه: software/remote path برای Workbench وب و diagnostic
- editor: hybrid؛ native desktop در آینده، WebWorkbench به‌عنوان remote/editor سبک

RTX 3060 برای D3D11 کاملاً کافی است. در این پروژه فعلاً ray tracing سخت‌افزاری
هدف اصلی نیست؛ باید ابتدا frame pacing، asset streaming، material، input و publish
درست شوند. بعد از پایدار شدن D3D11 می‌توان DXR را به‌صورت feature اختیاری اضافه کرد.

## تصمیم معماری

کد فعلی دو نقش را هم‌زمان دارد:

1. هسته و runtime موتور
2. بازی‌های مرجع فوتبال، گلف و arena و منطق Workbench

در بازسازی، این دو به‌تدریج جدا می‌شوند:

```text
KIMIA Core
├── Core / Math / Time / Diagnostics
├── Scene / Assets / Animation
├── Physics
├── Runtime / Input / Audio / Save
└── Render abstraction
    └── D3D11 backend (PC)

KIMIA Tools
├── Desktop editor
├── WebWorkbench / remote inspector
└── Import / package / validation

Game modules
├── Street football
├── Golf
└── Arena
```

`WorldEditor` فعلی در مرحلهٔ مهاجرت به‌عنوان legacy game/editor باقی می‌ماند و
تا زمانی که runtime جدید قابلیت‌هایش را پوشش نداده حذف نمی‌شود.

## چرا Direct3D 11؟

- در Windows X Lite و driverهای RTX 3060 کم‌ریسک و پایدار است.
- نسبت به DX12 هزینهٔ پیاده‌سازی و دیباگ بسیار کمتری دارد.
- برای deferred/forward rendering، shadow، material و post-process کافی است.
- ابزارهای NVIDIA و PIX/RenderDoc مسیر آزمایش مشخصی دارند.
- DX12 یا DXR بعداً می‌تواند backend جدا باشد و هستهٔ scene/runtime را تغییر ندهد.

## ترتیب بازسازی

### Phase 0 — PC foundation

- CMake و warning policy سازگار با MSVC
- backend selection و configuration مستقل از موبایل
- native window و input desktop
- D3D11 device/swapchain
- GPU/driver diagnostics
- loop با fixed update و variable render

### Phase 1 — Runtime واقعی

- جداکردن `GameWorld` از editor state
- entity/component storage پایدار
- transform hierarchy
- physics synchronization
- input action map مشترک برای keyboard/mouse/controller
- audio mixer و volume
- collision events قابل‌مصرف برای Logic

### Phase 2 — Render pipeline

- GPU resource cache
- texture upload و mipmap
- materialهای base-color/normal/roughness/metallic
- shadow و lighting چندمنبعی
- frustum culling و batching
- MSAA، HDR و post-processing اختیاری

### Phase 3 — Desktop editor

- native viewport و inspector
- scene hierarchy واقعی
- multi-select، duplicate و undo/redo
- asset browser و dependency graph
- WebWorkbench به‌عنوان remote API امن

### Phase 4 — Packaging و release

- project container شامل stage، blueprint و manifest
- bundle خودکفا شامل asset و runtime
- build profile مخصوص Windows
- crash log و diagnostics
- smoke test روی directory خالی

## تنظیمات پیشنهادی RTX 3060 / 16 GB

- رزولوشن پیش‌فرض editor: 1920x1080
- VSync: روشن به‌صورت پیش‌فرض، گزینهٔ off برای profiling
- MSAA: 4x
- shadow map: 2048 برای صحنهٔ اصلی، 1024 برای preview
- texture streaming budget: حدود 2 تا 3 GB، نه کل VRAM
- asset cache روی SSD
- fixed update: 60 Hz برای gameplay؛ physics داخلی می‌تواند 120 Hz بماند
- render thread و asset upload از game thread جدا شوند، اما قبل از profiling
  worker pool اضافه نشود
- software renderer فقط fallback و remote capture باشد، نه مسیر اصلی PC

## توصیه‌های محیط Windows

1. driver رسمی NVIDIA را نصب نگه دار؛ Windows X Lite ممکن است componentهای لازم
   برای Visual C++، WebView یا DirectX را حذف کرده باشد.
2. Visual Studio Build Tools شامل MSVC، Windows SDK و CMake را نصب کن.
3. برای اجرای WebWorkbench و صدا، مرورگر Chromium/Edge کامل لازم است.
4. قبل از هر تغییر در Windows X Lite یک restore point یا image از سیستم بگیر.
5. فایل‌های پروژه و asset cache روی NTFS/SSD باشند؛ مسیرهای شبکه برای build و
   shader cache استفاده نشوند.
6. برای profiling از Nsight Graphics، RenderDoc و PIX استفاده شود.
7. Windows Defender را کورکورانه خاموش نکن؛ فقط پوشهٔ build/cache را در صورت نیاز
   exclude کن.

## چیزهایی که عمداً فعلاً انجام نمی‌شوند

- مهاجرت مستقیم به DX12
- اضافه‌کردن Vulkan فقط برای «مدرن بودن»
- ray tracing اجباری
- حذف فوری WebWorkbench
- حذف بازی‌های فعلی قبل از داشتن تست معادل
- bundle کردن dependencyهای ناشناخته بدون manifest

## وضعیت پیاده‌سازی فعلی

این شاخه بخشی از Phase 0 را به‌صورت قابل‌اجرا پیاده کرده است:

- `kimia_runtime` با fixed-step، clamp، pause، reset و statistics از `WorldEditor`
  جدا شده و loop اصلی را برای gameهای بعدی آماده می‌کند.
- `EngineOptions` اکنون مسیر native window را از headless/remote جدا می‌کند.
  `kimia_world --desktop` پنجرهٔ SDL را درخواست می‌کند و در Windows، در صورت آماده
  بودن device، D3D11 را مسیر نمایش اصلی می‌کند؛ WebWorkbench همچنان capture جانبی
  دریافت می‌کند.
- D3D11 device، swap chain، depth buffer، mesh upload، shader، texture RGBA، UV و
  sampler پایه وجود دارد. این هنوز material system نهایی نیست و تغییر محتوای یک
  `Image` در cache باید در مرحلهٔ بعد با version/invalidation حل شود.
- `CMakePresets.json` presetهای `windows-pc-debug` و `windows-pc-release` را برای
  Visual Studio 17 2022 x64 فراهم می‌کند.

روی Windows بعد از نصب Visual Studio 2022، Windows SDK و CMake اجرا کن:

```text
cmake --preset windows-pc-debug
cmake --build --preset windows-pc-debug --parallel
cmake --preset windows-pc-release
cmake --build --preset windows-pc-release --parallel
```

یا به‌صورت مستقیم:

```text
cmake -S . -B build-pc -G "Visual Studio 17 2022" -A x64 ^
  -DKIMIA_BUILD_PC=ON -DKIMIA_RENDER_BACKEND=D3D11 -DKIMIA_BUILD_TESTS=OFF
cmake --build build-pc --config Release --parallel
```

smoke target را روی سیستم هدف اجرا و خروجی adapter، feature level، FPS و memory را
ثبت کن. نمونهٔ پیشنهادی:

```text
build-pc\\bin\\Release\\kimia_pc_d3d11_smoke.exe --frames 600 --no-vsync
build-pc\\bin\\Release\\kimia_pc_d3d11_smoke.exe --frames 120 --debug
```

سپس یک بار با Graphics Debugger و یک بار بدون آن اجرا شود. برای Workbench remote
اگر bind خارج از loopback لازم است، token را هم فعال کن؛ صفحهٔ مرورگر با
`http://HOST:PORT/?token=TOKEN` یک cookie امن برای درخواست‌های بعدی می‌گیرد:

```text
kimia_world.exe --desktop --bind 0.0.0.0 --auth CHANGE_ME
```

در حالت عادی `127.0.0.1` استفاده می‌شود و هیچ auth یا LAN exposure لازم نیست.
برای build فعلی Linux، تست aggregate همچنان فعال است و نبود EGL/OpenGL فقط باید
به‌عنوان skip گزارش شود؛ این skip اثبات موفقیت D3D11 نیست.

## موارد باز قبل از release PC

- build واقعی MSVC/Windows SDK و اجرای RTX 3060 هنوز باید انجام شود.
- native keyboard/mouse/gamepad action map و resize notification پایه متصل شده‌اند؛
  SDL2 برای پنجره و controller backend در preset PC لازم است. device-lost recovery،
  remapping UI و hot-plug policy نهایی هنوز باید اضافه شوند.
- texture/material اکنون با signature محتوای تصویر invalidation می‌شود، اما mip
  generation، چند texture و shader permutation هنوز باید اضافه شود.
- viewport native editor، scene hierarchy و undo/redo هنوز از WebWorkbench جدا
  نشده‌اند؛ مسیر فعلی hybrid foundation است، نه editor نهایی.
- packaging باید manifest، DLLهای لازم، shader cache و asset validation را قبل از
  release بررسی کند.

هر مرحله باید با build، تست و یک اجرای واقعی روی Windows تأیید شود. sandbox فعلی
Linux است و بنابراین compile/validation واقعی D3D11 باید روی سیستم Windows انجام
شود.
