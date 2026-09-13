# خط لولهٔ دارایی

بارگذاری و تبدیل مدل‌ها، تصاویر و صداها.

---

## فرمت‌های پشتیبانی‌شده

| نوع | فرمت | کتابخانه |
| --- | --- | --- |
| مش | OBJ (+MTL)، FBX | دست‌ساز / ufbx |
| تصویر | PNG، JPG | stb_image |
| صدا | WAV، MP3، OGG، FLAC | dr_libs + stb_vorbis |

- `.mtl` و `.blend` نوع مستقل نیستند؛ برای `.blend` مسیر File > Export > OBJ/FBX مستند شده (پارسر عمداً وجود ندارد).
- متریال (`MaterialData`): نام + رنگ پخش + مسیر تکسچر؛ «گذاشتن عکس روی جسم» = ساخت/به‌روزرسانی متریال.
- قرار دادن در صحنه: فایل‌های OBJ/FBX پوشهٔ `assets` با **فیت خودکار بزرگ‌ترین بُعد** وارد می‌شوند (مثل ایمپورت یونیتی).

## API

```cpp
kimia::assets::detectType(path)          // نوع با پسوند (بدون حساسیت به بزرگی)
kimia::assets::loadMesh / loadOBJAsset / loadFBXAsset
kimia::assets::loadImage / loadAudio     // std::optional + پیام خطا
kimia::assets::loadFBXSkinned(path, err) // مش پوست‌دار + اسکلت + کلیپ‌ها
```

- `MeshData`: `positions/normals/uvs` هم‌اندازه + `indices` مثلثی (مضرب ۳).
- `MeshAsset`: مش ترکیبی + `materials` + `subMeshes` (خالی = بدون متریال).
- `Image`: پیکسل‌های `u8` سطر-عمده (سطر ۰ = بالا).
- `AudioBuffer`: PCM میان‌گذاری‌شدهٔ `f32`؛ `encodeWAV()`؛ صداهای رویه‌ای `tone`/`thock`/`concat`.

## قراردادهای مش

- مثلث‌ها از بیرون **پادساعتگرد**؛ نرمال‌ها بیرون‌سو.
- OBJ: هر گوشهٔ face یک رأس؛ چهارضلعی → ۴ رأس/۶ ایندکس (مکعب استاندارد ۲۴v/۳۶i)؛ UV با قرارداد OBJ برگردانده می‌شود.
- FBX: محورها به راست‌دست Y-up، واحد به متر، نرمال‌های گم‌شده تولید و تاپل‌های یکسان ادغام می‌شوند.
- فرمت متنی مش KIMIA v1 (سرآیند `# KIMIA mesh v1`؛ بارگذاری مقاوم).

## ابزار خط فرمان

```bash
./build/bin/kimia_assets_cli model.fbx texture.png sound.mp3
```

گزارش متریال‌ها (نام/رنگ/تکسچر) + فایل‌های تبدیل‌شدهٔ کنار فایل اصلی (`.kimiamesh`، `.kimi.png/.kimi.jpg`، `.kimi.wav`).

## داده‌های تست

`Tests/assets/` — دارایی‌های کوچک با `kimia_asset_gen` تولید می‌شوند؛ فایل‌های مرجع (box.fbx، textured.fbx، spider.obj، sfx.ogg، tone.flac، …) از مخازن بالادستی رسمی کپی شده‌اند.

---
