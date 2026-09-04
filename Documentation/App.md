# برنامه (Engine/App)

بوت‌استرپ موتور و سرور وب نمایش فریم — همان مسیری که KIMIA World روی آن
ساخته می‌شود: بازی headless رندر می‌کند و مرورگر از `localhost:8080`
می‌بیند و کنترل می‌کند.

## WebViewer (`kimia::web::Server`)

سرور HTTP بدون وابستگی (سوکت POSIX + یک thread به ازای اتصال):

| مسیر | رفتار |
| --- | --- |
| `GET /` | 200 text/html — صفحهٔ کنترل (پد لمسی، دکمه‌ها، نمایش فریم) |
| `GET /frame.png` | 200 با بایت‌های دقیق آخرین فریم؛ **503** قبل از اولین `publishFrame` |
| `GET /stats` | 200 — آخرین خط آمار |
| `POST /input?key=K&down=0|1&tap=K&lookX=..&lookY=..&zoom=..` | 200 — تزریق ورودی |
| غیر از این‌ها | 404 |

- `publishFrame(pngBytes, statsLine)` — فریم بعدی از `Image::encodePNG`؛
- `drain()` — نقشهٔ کلیدهای پایدار (LEVEL) + tapها و دلتاهای look/zoom
  یک‌بارمصرف (EDGE) که با هر drain خالی می‌شوند؛
- پورت **موقت** در صورت ۰ (پورت واقعی با `port()` خوانده می‌شود)؛
  `stop()`/`start()` قابل تکرار (restart پشتیبانی‌شده).

صفحهٔ کنترل (`makePageHtml`): دکمه‌های hold-vs-tap، پد درگ برای چرخش
دوربین (pointer capture)، پرس‌وجوی فریم هر ۱۰۰ میلی‌ثانیه و آمار هر ۵۰۰
میلی‌ثانیه — بدون هیچ اسکریپت خارجی.

## Engine (بوت‌استرپ)

- `EngineOptions`: `headless`، عنوان/ابعاد پنجره، `webPort`، `enableWeb`.
- `initialize()`: اگر `XDG_RUNTIME_DIR` تنظیم نباشد → `/tmp/kimia-xdg` با
  مجوز 0700؛ روی Android اگر `LIBGL_ALWAYS_SOFTWARE` تنظیم نباشد → ۱؛
  ساخت پنجرهٔ پنهان (مگر headless)، بارگذاری `GLFunctions`، ساخت
  `EGLContext`، راه‌اندازی WebViewer.
- حلقهٔ بازی: یک `poll()` (پنجره + وب → `InputState`)، منطق بازی،
  `renderSoftware`/GL، `publishFrame`، و در پایان `endFrame()`.

## مثال‌ها (`Examples/`)

- `kimia_hello` — دود آزمایشی: یک فریم نرم‌افزاری + گزارش در دسترس بودن
  GL/پنجره.
- `kimia_first3d` — مکعب+صفحه+کره با دوربین مداری؛ پرچم‌های
  `--frames N`، `--capture file.png`، `--web`، `--port N`، `--window`.
- `kimia_remote` — headless + وب: کنترل با a/d/w/s، زوم z/x، بازنشانی r
  و چرخش دوربین با درگ؛ فریم‌ها روی پورت 8080.

## تست‌ها

`Tests/src/WebTests.cpp` (سوکت خام، بدون curl): ریشه 200 text/html،
frame 503 قبل از انتشار و 200 با بایت‌های دقیق بعد از آن، خط آمار، LEVEL
پایدار برای کلید نگه‌داشته‌شده، tap یک‌بارمصرف، دلتاهای look/zoom، 404،
محتوای صفحهٔ کنترل، و restart با پورت موقت.
