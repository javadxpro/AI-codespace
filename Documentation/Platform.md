# پلتفرم (Engine/Platform)

ورودی یکپارچه و پنجرهٔ اختیاری — بدون وابستگی اجباری؛ SDL2 فقط در صورت
یافت‌شدن (`-DKIMIA_ENABLE_SDL2=ON|OFF`، پیش‌فرض ON) به بیلد اضافه می‌شود.

## ورودی (`Key`، `InputState`)

- `Key`: A–Z، Num0–9، جهت‌ها، Return/Space/Shift/Escape/Tab/Backspace.
  `keyFromName` نام‌های وب‌گونه (مثلاً `"KeyW"`, `"ArrowUp"`) را نگاشت می‌کند.
- `InputState`:
  - **LEVEL**ها: `down(key)` — وضعیت پایدار نگه‌داشتن (برای حرکت پیوسته)؛
  - **EDGE**ها: `pressed(key)` / `released(key)` — فقط در فریمی که رخ دادند؛
  - `tap(key)` و دلتاهای `lookX/lookY/zoom` — رویدادهای یک‌بارمصرف
    (`takeLook`/`takeZoom` آن‌ها را خالی می‌کند)؛
  - `endFrame()` فقط edgeها را پاک می‌کند و **بعد از** منطق بازی صدا زده
    می‌شود تا بازی پیش از پاک‌شدن آن‌ها را بخواند.

## پنجره (`Window`)

- `Window::create(title, w, h, hidden)` — کارخانه؛ اگر SDL2 نبود
  `nullptr` برمی‌گرداند (حالت headless).
- `SDLWindow` فقط با `KIMIA_HAS_SDL2` کامپایل می‌شود:
  - نگاشت scancode→Key، خروج با Escape؛
  - حالت **hidden**: پنجره بدون نیاز به نمایشگر (برای headless-with-window)؛
  - `present(Image)` — بلیت CPU با `SDL_BlitScaled` برای RGB24/RGBA32
    (مسیر نرم‌افزاری همیشه کار می‌کند).

## سیاست نمایش

- GL در دسترس → رندر GL؛ در غیر این صورت → `renderSoftware` و بلیت CPU.
- بدون SDL2 → headless + WebViewer: فریم‌ها در مرورگر دیده می‌شوند
  (مسیر Termux/Ubuntu که در بخش App مستند شده است).

## Termux (Android) — نکات بیلد

- کامپایلر Termux **Clang** است (۲۱ در حال حاضر)؛ موتور با Clang و GCC هر
  دو با `-Werror` و صفر اخطار ساخته می‌شود (Clang سخت‌گیرتر است: مثلاً
  `-Wsign-conversion` داخل `-Wconversion` روشن است — GCC آن را برای C++
  روشن نمی‌کند).
- `Tools/termux_build.sh` مسیر رسمی است: تشخیص شاخهٔ موتور (`main` فقط اسکلت
  است)، `pkg install clang cmake ninja`، تعمیر `cmake` خراب
  (`pkg uninstall -y cmake && pkg install -y cmake` وقتی خطای `CMAKE_ROOT`
  می‌آید)، configure با `-DKIMIA_WERROR=ON -DCMAKE_BUILD_TYPE=Release`،
  build، تست و چاپ دستور بعدی.
- بدون `CMAKE_BUILD_TYPE` بیلد **Release** است (رسترایزر نرم‌افزاری روی
  CPU؛ بیلد بدون بهینه‌سازی چند برابر کندتر است). برای دیباگ:
  `-DCMAKE_BUILD_TYPE=Debug`.
- `dlopen` برای لودر GL با `${CMAKE_DL_LIBS}` لینک می‌شود (روی bionic
  کتابخانهٔ جدا نیست؛ روی glibc قدیمی `-ldl` لازم است).
- کد فروشندهٔ `stb_vorbis.c` یک اخطار `-Wtautological-compare` در Clang دارد
  که فقط برای همان هدف خاموش شده است (کد upstream دست نمی‌خورد).
