# خط لولهٔ دارایی (Asset Pipeline)

ماژول `kimia_assets` بارگذاری، اعتبارسنجی و تبدیل دارایی‌های بازی را انجام می‌دهد.

## فرمت‌های پشتیبانی‌شده

| نوع | پسوندها | خواندن | نوشتن |
| --- | --- | --- | --- |
| مش (Mesh) | `.obj` | Wavefront OBJ (رأس/نرمال/UV، چندضلعی، ایندکس منفی) | فرمت متنی KIMIA v1 |
| مش (Mesh) | `.fbx` | FBX باینری و ASCII (via ufbx؛ مش، نرمال، UV، تبدیل سلسله‌مراتبی) | فرمت متنی KIMIA v1 |
| تصویر | `.png` | PNG (lossless) | PNG |
| تصویر | `.jpg` / `.jpeg` | JPEG (lossy) | JPEG |
| صدا | `.wav` | WAV PCM (هر تعداد کانال/نرخ نمونه) | WAV 16-bit PCM |
| صدا | `.mp3` | MP3 (decode به PCM) | WAV 16-bit PCM |

## API

- `kimia::assets::detectType(path)` — تشخیص نوع با پسوند (بدون حساسیت به بزرگی حروف).
- `kimia::assets::loadMesh / loadImage / loadAudio` — بارگذاری با `std::optional` + پیام خطا.
- `kimia::MeshData` — مش CPU: `positions/normals/uvs` هم‌اندازه، `indices` مثلثی (مضرب ۳).
- `kimia::Image` — پیکسل‌های `u8` سطر-عمده (سطر ۰ = بالا).
- `kimia::AudioBuffer` — PCM میان‌گذاری‌شدهٔ `f32` در بازهٔ [−1, 1].

## قراردادهای مش

- مثلث‌ها از بیرون **پادساعتگرد** (CCW) هستند؛ نرمال‌ها بیرون‌سو.
- OBJ: هر گوشهٔ face یک رأس می‌گیرد؛ چهارضلعی → ۴ رأس/۶ ایندکس (مکعب استاندارد
  → ۲۴v/۳۶i). UV با قرارداد OBJ (v=0 پایین) برگردانده می‌شود تا با ردیف بالای
  تصویر منطبق شود. `dedupe=true` رأس‌های هم‌تاپل را ادغام می‌کند.
- FBX: محورها به راست‌دست Y-up تبدیل می‌شوند، واحد به متر، نرمال‌های گم‌شده
  تولید می‌شوند و تاپل‌های یکسان ادغام می‌شوند.
- فرمت متنی مش KIMIA v1:
  ```
  # KIMIA mesh v1
  name Cube
  positions 24
  x y z
  ...
  normals 24
  ...
  uvs 24
  u v
  ...
  indices 36
  a b c
  ```
  بارگذاری مقاوم (tolerant): خطوط `#` کامنت هستند، کلیدواژهٔ ناشناخته نادیده
  گرفته می‌شود.

## ابزار خط فرمان

```bash
./build/kimia_assets_cli model.fbx texture.png sound.mp3
```

خروجی هر فایل: گزارش روی ترمینال + فایل‌های تبدیل‌شدهٔ کنار فایل اصلی
(`.kimiamesh`، `.kimi.png/.kimi.jpg`، `.kimi.wav`، `.kimiimage/.kimiaaudio`).

## کتابخانه‌های شخص ثالث (vendored، FOSS)

| کتابخانه | نسخه/کامیت | کاربرد | لایسنس |
| --- | --- | --- | --- |
| stb_image / stb_image_write | 2c980bb | PNG/JPG | MIT / Public Domain |
| dr_wav / dr_mp3 | dfe8377 | WAV/MP3 | Public Domain / MIT |
| ufbx | fcc5d6b | FBX | MIT |

## داده‌های تست

`Tests/assets/` — `kimia_asset_gen` بعد از هر بیلد دارایی‌های کوچک را تولید
می‌کند (`tone.wav`، `2x3.png`، `2x2.jpg`، `cube.obj`، `quad.obj`). فایل‌های
`box.fbx` (آزمون‌های assimp) و `blender_cube.fbx` (آزمون‌های ufbx) و
`440hz.mp3` (SoundManager2، BSD) از مخزن‌های بالادستی کپی شده‌اند.
