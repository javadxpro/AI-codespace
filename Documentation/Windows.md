# Windows (PC)

بیلد اختصاصی PC — رندر Direct3D 11 روی کارت گرافیک، با رسترایزر نرم‌افزاری به‌عنوان fallback.

---

## هدف سخت‌افزاری

- GPU اصلی: NVIDIA RTX 3060، حافظهٔ ۱۶ GB، Windows X Lite (به شرط driver رسمی NVIDIA و runtime های DirectX 11).
- toolchain: MSVC + Visual Studio 2026 + CMake؛ رندر اصلی D3D11.

## بیلد

Presetها: `windows-pc-release` / `windows-pc-debug` (generator `Visual Studio 18 2026`).

```powershell
cmake --preset windows-pc-release
cmake --build --preset windows-pc-release
```

یا دستی با vcpkg (SDL2 استاتیک) و CRT استاتیک:

```powershell
vcpkg install sdl2:x64-windows-static
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 `
  -DKIMIA_EMBED_ASSETS=ON -DKIMIA_ENABLE_SDL2=ON -DKIMIA_BUILD_PC=ON `
  -DKIMIA_ENABLE_D3D11=ON -DKIMIA_RENDER_BACKEND=D3D11 `
  -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static `
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded
cmake --build build --config Release
```

خروجی: `build/bin/Release/kimia_world.exe`. بسته‌بندی: `Tools/package_pc_release.ps1`.

> توجه: VS 2026 Community با workload «Desktop development with C++» لازم است؛ بدون آن `cl` شناخته نمی‌شود (runtime `VC_redist.x64.exe` کافی نیست).

## چرا D3D11

- روی Windows X Lite و درایورهای RTX 3060 کم‌ریسک و پایدار است؛ هزینهٔ پیاده‌سازی/دیباگ کمتر از DX12.
- برای forward/deferred، سایه، متریال و post-process کافی است.
- DXR/DX12 بعداً می‌توانند backend جدا باشند بدون تغییر هستهٔ scene/runtime.

## تنظیمات پیشنهادی (RTX 3060 / 16 GB)

- رزولوشن ویرایشگر ۱۹۲۰×۱۰۸۰، VSync روشن، MSAA 4x.
- shadow map 2048 (صحنهٔ اصلی) / 1024 (پیش‌نمایش).
- بودجهٔ texture streaming ~۲–۳ GB؛ کش asset روی SSD؛ fixed update 60 Hz (فیزیک 120 Hz می‌ماند).

## مسیرها

| مسیر | وضعیت |
| --- | --- |
| D3D11 | اصلی PC (در حال هم‌سازی کامل PBR با مسیر GL) |
| نرم‌افزاری | fallback و remote capture |
| WebWorkbench | ویرایشگر سبک/remote از مرورگر |
| ray tracing سخت‌افزاری (DXR) | آتی، اختیاری |

---

## وضعیت CI

Workflow `Build Windows EXE` یک `kimia_world.exe` خودکفا (CRT/SDL2 استاتیک + دارایی‌های جاسازی‌شده) می‌سازد، `--version` را دودآزمایی می‌کند و آرتیفکت `kimia-world-windows-x64` را بالا می‌برد.

---
