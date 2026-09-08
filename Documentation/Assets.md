# خط لولهٔ دارایی (Asset Pipeline)

ماژول `kimia_assets` بارگذاری، اعتبارسنجی و تبدیل دارایی‌های بازی را انجام می‌دهد.

## فرمت‌های پشتیبانی‌شده

| نوع | پسوندها | خواندن | نوشتن |
| --- | --- | --- | --- |
| مش (Mesh) | `.obj` (+`.mtl`) | Wavefront OBJ (رأس/نرمال/UV، چندضلعی، ایندکس منفی) + متریال MTL (رنگ `Kd`، تکسچر `map_Kd`) و زیر-مش به ازای هر `usemtl` | فرمت متنی KIMIA v1 |
| مش (Mesh) | `.fbx` | FBX باینری و ASCII (via ufbx؛ مش، نرمال، UV، تبدیل سلسله‌مراتبی) + متریال‌ها (رنگ و تکسچر؛ تکسچر جاسازی‌شده کنار فایل استخراج می‌شود) | فرمت متنی KIMIA v1 |
| تصویر | `.png` | PNG (lossless) | PNG |
| تصویر | `.jpg` / `.jpeg` | JPEG (lossy) | JPEG |
| صدا | `.wav` | WAV PCM (هر تعداد کانال/نرخ نمونه) | WAV 16-bit PCM |
| صدا | `.mp3` | MP3 (decode به PCM) | WAV 16-bit PCM |
| صدا | `.ogg` | OGG/Vorbis (decode به PCM، via stb_vorbis) | WAV 16-bit PCM |
| صدا | `.flac` | FLAC (decode به PCM، via dr_flac) | WAV 16-bit PCM |

### فایل‌های بلندر (`.blend`)

`.blend` قالب باینری خصوصی بلندر است و **بارگذاری مستقیم ندارد** — روش
استاندارد (مثل یونیتی) خروجی گرفتن است: در بلندر **File > Export > Wavefront
(.obj)** (با گزینهٔ Write Materials برای متریال/تکسچر) یا **FBX (.fbx)** را
انتخاب کنید و فایل خروجی را به کیمیا بدهید. اگر کسی `.blend` را مستقیم به
کیمیا بدهد، پیام خطا همین مسیر خروجی را راهنمایی می‌کند. (بلندر روی Termux
قابل نصب نیست؛ روی رایانه خروجی بگیرید و فایل را به گوشی منتقل کنید.)

## متریال‌ها

- `kimia::MaterialData` — نام، رنگ پخش (diffuse) و مسیر تکسچر. هر مش با نام
  متریالش به آن اشاره می‌کند (`MeshData::materialName`).
- `kimia::assets::MeshAsset` — مش ترکیبی + جدول متریال‌ها + زیر-مش به ازای هر
  متریال (مثل یونیتی: هر بخش رنگ/تکسچر خودش را دارد). «گذاشتن عکس روی جسم»
  یعنی ساخته شدن/به‌روزرسانی همین `MaterialData` (اتصال UI آن در مرحلهٔ رندر
  کامل می‌شود).
- OBJ بدون فایل MTL (گم/خراب) هم بار می‌شود: مش سالم می‌ماند و متریال‌ها
  سفید پیش‌فرض می‌شوند — مثل یونیتی که مدل بدون متریال را رد نمی‌کند.
- MTL مقاوم است: نام متریالِ ناشناخته با رنگ سفید پیش‌فرض ثبت می‌شود،
  مسیرهای تکسچر نسبت به پوشهٔ MTL حل و نرمال (`\` → `/`، حذف `./`) می‌شوند،
  و خطوط ناشناخته نادیده گرفته می‌شوند.

## در ادیتور (KIMIA World)

فایل‌های OBJ/FBX پوشهٔ `assets` (با همهٔ زیرپوشه‌ها) در کاتالوگ با «مدل از فایل»
فهرست و در صحنه جای‌گذاری می‌شوند (اندازهٔ انتخابی + فیت خودکار بزرگ‌ترین بُعد).
نام‌های دارای فاصله و مسیرهایی مثل `animations/pleyer move/walk.fbx` معتبرند.
Workbench نام نسبی را در صحنه ذخیره می‌کند؛ زمان اجرا آن را نسبت به ریشهٔ assets
حل می‌کند، بنابراین world بعد از جابه‌جایی پوشه یا publish هم همان فایل را پیدا
می‌کند. native editor نیز اسکن بازگشتی انجام می‌دهد.

OBJ/MTL با چند material slot در software، OpenGL و D3D11 به‌صورت sub-meshهای
جدا با رنگ `Kd` رندر می‌شوند. FBX دارای مشِ skin شده با اسکلت و clip در زمان
اجرا sample و skin می‌شود؛ clipها loop/trigger می‌شوند و هنگام عوض‌شدن clip روی
همان rig با cross-fade کوتاه blend می‌شوند. FBX animation-only (مثل pack فعلی)
به‌عنوان rig زندهٔ stick-preview وارد می‌شود و برای نمایش شخصیت واقعی به FBX
skin-compatible نیاز دارد؛ فایل‌های OBJ کودکان street عمداً static هستند و
اسکلت ندارند.

### اعتبارسنجی pack واقعی KIMIA

تست `shipped_asset_pack_all_models_load_recursively` خود پوشهٔ `assets/` را
recursive پیمایش می‌کند و تمام فایل‌های موجود را باز می‌کند: در pack فعلی
۳۹ FBX (اسکلت و clip) و ۸ OBJ (هندسه، نرمال، UV، MTL و sub-mesh) با ۸ فایل MTL
تأیید می‌شوند. این تست جایگزین اجرای native D3D11 روی RTX 3060 نیست؛ آن مرحله
با `kimia_pc_d3d11_smoke.exe` روی Windows انجام می‌شود.


## Animator، retarget و نقاط بدن

`kimia::Animator` در `Engine/Graphics` مستقل از `WorldEditor` است: هر action به
یک `AnimatorClip` (اسکلت، clip و source asset) bind می‌شود و `playAction`، زمان،
سرعت، loop/one-shot، stop و cross-fade را مدیریت می‌کند. clip می‌تواند از FBX
جدا از مدل شخصیت بیاید؛ نام استخوان‌ها با حذف prefixهای رایج مثل
`mixamorig:`، `Armature|` و `skeleton|` retarget می‌شوند. اگر هیچ استخوان مشترکی
وجود نداشته باشد bind رد می‌شود و runtime فقط diagnostic state نگه می‌دارد؛
pose ساختگی یا خراب تولید نمی‌شود.

هر `WorldEditor` assetهای FBX را cache می‌کند تا pointerهای Animator تا پایان
world معتبر بمانند. `Control` علاوه بر `clipFile` و `clip`، فیلد اختیاری
`target` دارد؛ بنابراین یک دکمه می‌تواند `animations/pleyer move/walk.fbx` یا
هر فایلی با فاصله در نام را روی یک character مشخص اجرا کند. نبودن target همان
رفتار سازگار قبلی را دارد و اولین character با skeleton سازگار انتخاب می‌شود.
برای مدل بدون skeleton، rig ویرایش‌شدهٔ `EntityData::rig` fallback است؛ اسکلت
واقعی FBX، در صورت وجود، همیشه اولویت دارد.

`WorldEditor::characterBoneMarkers` و `characterBoneCenter` مرکز، start/end و
طول استخوان را از pose فعلی برمی‌گردانند. APIهای `/api/bones` و
`/api/object/bones` مختصات local و world را همراه `x/z` می‌دهند؛ transform خود
entity نیز در world اعمال می‌شود. برای leaf bone، joint به‌عنوان نقطهٔ پایدار
برگردانده می‌شود و برای boneهای دارای child، میانگین jointهای فرزند endpoint
است.

تأیید فعلی با assetهای tracked: aggregate suite با کامپایل مستقیم GCC در محیط
Linux (این sandbox ابزار CMake نداشت) نتیجهٔ `467/467 tests passed` داد؛ تست
EGL/OpenGL به‌دلیل نبود driver skip شد. این نتیجه build یا smoke واقعی
MSVC/D3D11 روی Windows را ادعا نمی‌کند؛ آن باید روی RTX 3060/Windows X Lite با
presetهای PC اجرا شود.

## API

- `kimia::assets::detectType(path)` — تشخیص نوع با پسوند (بدون حساسیت به بزرگی حروف؛
  `.mtl` و `.blend` عمداً نوع مستقل نیستند).
- `kimia::assets::loadMesh / loadOBJAsset / loadFBXAsset / loadImage / loadAudio`
  — بارگذاری با `std::optional` + پیام خطا.
- `kimia::MeshData` — مش CPU: `positions/normals/uvs` هم‌اندازه، `indices` مثلثی
  (مضرب ۳) + `materialName`.
- `kimia::MaterialData` — متریال: نام، `color`، `texturePath`.
- `kimia::assets::MeshAsset` — مش ترکیبی، `materials`، `subMeshes` (خالی = بدون متریال).
- `kimia::Image` — پیکسل‌های `u8` سطر-عمده (سطر ۰ = بالا).
- `kimia::AudioBuffer` — PCM میان‌گذاری‌شدهٔ `f32` در بازهٔ [−1, 1].
  - `encodeWAV()` همان WAV شانزده‌بیتی `writeWAV` را به‌صورت بایت برمی‌گرداند
    (سرآیند ۴۴ بایتی RIFF + داده؛ کوانتیزاسیون عین dr_wav، پس بایت‌به‌بایت
    برابر فایل) — چیزی که WebViewer از `/sfx/<name>` می‌فرستد.
  - صداهای **رویه‌ای** (بدون فایل، قطعی و روی هر دستگاه یکسان):
    `AudioBuffer::tone(hz, seconds, amplitude=0.6, endHz=0, rate=22050)` —
    سینوسی تک‌کانال با پوش نمایی (`exp(-5·t/T)`) و سُرش اختیاری زیر‌وبمی؛
    `AudioBuffer::thock(seconds, cutoffHz=900, amplitude=0.7, rate=22050)` —
    نویز سفید (LCG با بذر ثابت) از فیلتر پایین‌گذر یک‌قطبی با پوش
    `exp(-9·t/T)` = صدای ضربه به توپ؛ `AudioBuffer::concat(a, b)` — پشت‌سرهم
    (فرمت ناهمسان → همان `a`). آرگومان ناممکن (فرکانس/مدت ≤ ۰) بافر خالی.

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
./build/bin/kimia_assets_cli model.fbx model.obj sound.ogg tone.flac
```

گزارش هر مش حالا متریال‌ها را هم فهرست می‌کند (نام، رنگ، تکسچر) و برای FBX
تکسچر جاسازی‌شده را با نام `<فایل>_<متریال>_<شماره>.png` کنار فایل می‌نویسد.

خروجی هر فایل: گزارش روی ترمینال + فایل‌های تبدیل‌شدهٔ کنار فایل اصلی
(`.kimiamesh`، `.kimi.png/.kimi.jpg`، `.kimi.wav`، `.kimiimage/.kimiaaudio`).

## کتابخانه‌های شخص ثالث (vendored، FOSS)

| کتابخانه | نسخه/کامیت | کاربرد | لایسنس |
| --- | --- | --- | --- |
| stb_image / stb_image_write / stb_vorbis | 2c980bb | PNG/JPG / OGG-Vorbis | MIT / Public Domain |
| dr_wav / dr_mp3 / dr_flac | dfe8377 | WAV/MP3/FLAC | Public Domain / MIT |
| ufbx | fcc5d6b | FBX | MIT |

## داده‌های تست

`Tests/assets/` — `kimia_asset_gen` بعد از هر بیلد دارایی‌های کوچک را تولید
می‌کند (`tone.wav`، `2x3.png`، `2x2.jpg`، `cube.obj`، `quad.obj`). از مخزن‌های
بالادستی کپی شده‌اند: `box.fbx` (آزمون‌های assimp)، `blender_cube.fbx` و
`textured.fbx` و `material_mapping.fbx` (داده‌های آزمون ufbx)، `440hz.mp3`
(SoundManager2، BSD)، `cube_usemtl.obj/.mtl` و `spider.obj/.mtl` (آزمون‌های
assimp)، `sfx.ogg` (chromium media/test)، `tone.flac` (مجموعهٔ IETF
flac-test-files).
