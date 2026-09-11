# اجرای موتور در GitHub Codespaces

در Codespaces نیازی به هیچ نصب‌کردنی نیست؛ فقط یک کانتینر لینوکس در ابر باز
می‌شود، موتور به‌صورت headless (رندر نرم‌افزاری + WebViewer) بیلد می‌شود و
بازی را در مرورگر سرو می‌کند — دقیقاً همان مسیری که برای PS4 هم استفاده
می‌شود.

## گام‌ها

1. وارد `github.com/javadxpro/AI-codespace` شو.
2. شاخه‌ای که موتور کامل را دارد انتخاب کن: از منوی شاخه‌ها
   `arena/01a080a4-ai-codespace`.
3. دکمهٔ سبز **Code** → تب **Codespaces** → **Create codespace on
   arena/01a080a4-ai-codespace**.
4. صبر کن تا کانتینر ساخته شود. فایل `.devcontainer/devcontainer.json`
   خودکار موارد زیر را انجام می‌دهد:
   - تصویر `devcontainers/cpp` (شامل g++/clang، cmake، ninja، gdb)
   - بیلد headless با `-DKIMIA_WERROR=ON -DKIMIA_ENABLE_SDL2=OFF`
   - اجرای سوئیت تست (475 تست)
   - فوروارد پورت **8080**
5. وقتی بیلد تمام شد، در ترمینال Codespaces:

   ```sh
   ./build/bin/kimia_world --port 8080 --bind 0.0.0.0 --auth CHANGE_ME --profiles build/bin/profiles
   ```

6. در پنل **PORTS** (پایین VS Code) روی لینک پورت 8080 کلیک کن و در آدرس
   `?token=CHANGE_ME` را اضافه کن (چون bind غیر-loopback بدون توکن عمداً
   رد می‌شود). بازی/ادیتور در مرورگر باز می‌شود.

اگر می‌خواهی فقط در خود کانتینر و بدون توکن کار کنی، از
`--bind 127.0.0.1` استفاده کن و با `Forward Port` پیش‌نمایش را باز کن.

## چه چیزی کار می‌کند و چه چیزی نه

| قابلیت | وضعیت در Codespaces |
| --- | --- |
| موتور کامل + فیزیک + منطق + ادیتور (WebViewer) | ✅ |
| رندر نرم‌افزاری و استریم فریم به مرورگر | ✅ |
| پنجرهٔ بومی SDL / D3D11 | ❌ (لینوکس headless؛ GPU دسکتاپ نیست) |
| WebGL2/WASM (`kimia_webgl`) | ⚠️ نیاز به emsdk؛ در Codespaces هم دانلود SDK لازم دارد |

## نکته دربارهٔ حجم دانلود

دانلودهای قبلی بزرگ بودند چون `git clone` کامل، همهٔ شاخه‌ها را می‌گیرد —
به‌خصوص شاخهٔ `master` که فقط حاوی `assets/` (حدود ۹۳۰ مگابایت: فایل‌های FBX
کاراکترها تا ۸۳ مگ، افکت‌های صوتی WAV تا ۷۴ مگ، مپ‌ها و فونت‌ها) است. برای
دانلود کم‌حجم، همیشه فقط شاخهٔ کار را بگیر:

```sh
git clone --depth 1 -b arena/01a080a4-ai-codespace \
  https://github.com/javadxpro/AI-codespace.git
```

(خود شاخهٔ کار فقط ~۱۲ مگابایت سورس است.)
