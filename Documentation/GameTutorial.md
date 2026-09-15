# آموزش ساخت بازی با KIMIA

این راهنما فرض میکنه با مفاهیم بازیسازی آشنایی (از Unity/Godot یا جای دیگه) آشنا هستی. تمرکزش اینه که بفهمی KIMIA چطور فکر میکنه و چطور اولین بازی خودت رو بسازی و توی مرورگر منتشر کنی.

---

## ۱. فلسفهٔ KIMIA

KIMIA موتوری متمرکز بر **صحنه-مبنا** است، مثل Unity. اما چند تفاوت بنیادین دارد:

| مفهوم | Unity / Godot | KIMIA |
| --- | --- | --- |
| تعریف صحنه | ویرایشگر گرافیکی + اسکریپت | فایل متنی ساده (`*.kimia`) |
| منطق بازی | C#/GDScript/Visual Scripting | قواعد متنی (`WHEN`/`IF`/`DO`) |
| Build | یک پروژه با Scene/Asset | یک فایل `.html` تکی (برای وب) |
| رندر GL | OpenGL/D3D | GL/GLES3/WebGL2 + یک رندر نرمافزاری همیشگی |

ایده: هر بازی دو فایل دارد - **پروفایل** (قوانین بازی، مثلاً گلف یا فوتبال) و **صحنه** (مکان، اشیاء، بازیکن، توپ).

---

## ۲. ساختار یک پروژهٔ بازی

```
my-game/
├── game.kimiaprofile     ← قوانین بازی (امتیاز، سرعت، حالت)
├── world.kimia           ← صحنه (زمین، دیوار، توپ، بازیکن)
├── assets/               ← اختیاری: مدل/متن/صدای سفارشی
│   ├── props/
│   └── kids/
└── play.sh               ← اسکریپت اجرا (پس از publish تولید میشود)
```

دو فایل ضروری: پروفایل + صحنه. بقیه اختیاری.

---

## ۳. فایل پروفایل (`*.kimiaprofile`)

سینتکس خط-مبنا، بدون آکولاد. هر خط یک قانون یا پارامتر.

### ۳.۱ ساختار کلی

```ini
# KIMIA profile v1
# توضیح اختیاری - خطی که با # شروع شود نادیده گرفته میشود
name <نام-داخلی>
title <نام-فارسی-برای-منو>
field <طول-متر> <عرض-متر>
environment <sand|grass|asphalt>
player speed <m/s> jump <m/s²>
ball accurate <on|off> choice <on|off>
kick <پایه-قدرت> <ضریب-قدرت> <پایه-زاویه>
mode <shot|kick>
scoring <hole|gate>
team <1..4>
match <ثانیه>
ai <0..1>          # 0=بازیکن، 1=AI کامل
camera <chase|free|topdown>
# اختیاری
weather <باد-شتاب> <باد-جهت-رادیان>
time <ساعت-شبیهسازی>
tricks <on|off>
```

### ۳.۲ مثال واقعی: فوتبال یکبهیک خیابانی

```ini
# KIMIA profile v1
# street soccer: 1v1، زمین 16×5، دروازه‌های کوچک، 3 دقیقه
name street_soccer
title فوتبال خیابانی
field 16.000000 5.000000
environment asphalt
player speed 5.000000 jump 1.800000
ball accurate choice off
kick 3.000000 0.600000 2.000000    # شوت متوسط، پرتاب بادقت
mode kick                          # "shot" فقط برای گلف
scoring gate                       # امتیاز با گل زدن
team 1                             # 1 بازیکن
match 180.000000                   # 3 دقیقه
weather 0.000000 0.000000
time 9.000000
tricks off
ai 0.600000                        # بازیکن در 60% مواقع AI
camera chase
```

> **نکته**: تمام اعداد اعشاری هستند. حتی مقادیر ساده مثل `team 1` بهتر است به صورت `team 1.000000` نوشته شوند (طبق فایلهای نمونه در `Profiles/`).

---

## ۴. فایل صحنه (`*.kimia`)

صحنه با فعلهای ساده تعریف میشود. هر خط یک **entity** (شیء) میسازد.

### ۴.۱ شکل کلی

```
e "<نام>" mesh <نوع> pos <x y z> scale <x y z> [rot <quat>] [color <r g b>] [rough <0..1>] [meshfile "<مسیر>"]
```

### ۴.۲ نوع mesh

| نوع | شکل |
| --- | --- |
| `cube` | جعبه (با scale کشیده میشود) |
| `plane` | صفحهٔ مسطح (برای زمین و دیوار) |
| `sphere` | کره (برای توپ) |

### ۴.۳ قواعد هندسی

- **محور +X راست، +Y بالا، -Z جلو** (همانند OpenGL راستگرد).
- **چرخش کواترنیون**: `<x y z w>` به ترتیب بردار و اسکالر.
- **مقیاس یک بردار سه بُردی** است.
- **رنگ در خطی sRGB** نوشته میشود (0..1)، ولی خود موتور به صورت خودکار تبدیل میکند.

### ۴.۴ مثال کامل: زمین فوتبال

```
# KIMIA scene v1
# ground + pitch + goals + ball + 2 players
e "Ground"    mesh plane  pos 0 0 0   scale 44 1 44   color 0.15 0.15 0.17  rough 0.95
e "Pitch"     mesh plane  pos 0 0.005 0 scale 5 1 16  color 0.30 0.28 0.26  rough 0.95
e "Wall_L"    mesh cube   pos -4 1.5 0 scale 0.5 3 17.5 color 0.55 0.27 0.18 rough 0.90
e "Wall_R"    mesh cube   pos  4 1.5 0 scale 0.5 3 17.5 color 0.55 0.27 0.18 rough 0.90
e "GoalA"     mesh cube   pos 0 0 -7  scale 1 1 1     color 0.92 0.92 0.90 rough 0.50 meshfile "assets/street/props/goal_small.obj"
e "GoalB"     mesh cube   pos 0 0  7  scale 1 1 1     color 0.88 0.86 0.82 rough 0.80 meshfile "assets/street/props/goal_small.obj"
e "Ball"      mesh sphere pos 0 0.15 0 scale 0.3 0.3 0.3 color 0.95 0.95 0.92 rough 0.30
e "Player"    mesh cube   pos 0 0 2.2 scale 1 1 1     color 0.25 0.45 0.95 rough 0.80 meshfile "assets/street/kids/kid_ali.obj"
e "Opponent"  mesh cube   pos 0 0 -2.2 scale 1 1 1    color 0.95 0.30 0.30 rough 0.80 meshfile "assets/street/kids/kid_reza.obj"
```

### ۴.۵ مدلهای خارجی (OBJ)

برای استفاده از مدل، از `meshfile "..."` استفاده کن. پوشهٔ `assets/street/props/` چند مدل آماده دارد:

- `goal_small.obj` - دروازهٔ کوچک
- `cone.obj` - مخروط
- `brick_stack.obj` - انبوه آجر
- `tire_stack.obj` - انباشتهٔ لاستیک
- `bench.obj` - نیمکت
- `kids/kid_ali.obj`, `kids/kid_reza.obj`, `kids/kid_hassan.obj` - شخصیتهای بچه

> **سفارشیسازی**: مدلهای خودت را در `assets/<your-game>/` بگذار و با `Tools/make_street_models.py` یا هر ابزار OBJ دیگری بساز. هیچ فرمت اختصاصی وجود ندارد - فقط OBJ با مختصات CCW و outward.

---

## ۵. گردش کار کامل: از صفر تا انتشار در وب

### مرحلهٔ ۱: ساخت پروژه

```bash
mkdir my-game && cd my-game
mkdir -p assets/props assets/kids
```

### مرحلهٔ ۲: نوشتن پروفایل

فایل `my-game.kimiaprofile` را با محتوای بخش ۳.۲ بساز.

### مرحلهٔ ۳: نوشتن صحنه

فایل `my-game.kimia` را با محتوای بخش ۴.۴ بساز (نام entityها را با نام دلخواه عوض کن).

### مرحلهٔ ۴: انتشار

```bash
kimia-publish \
  --profile my-game.kimiaprofile \
  --world   my-game.kimia \
  --target  web \
  --out     dist/
```

این دستور سه کار انجام میدهد:

1. پروفایل + صحنه + مدلها را در یک فایل `game.kimia` ادغام میکند (embedded assets).
2. کلاینت WebGL (WASM) را از شاخهٔ `Web/` کپی میکند.
3. یک `play.sh` و `index.html` میسازد.

> **نکته**: اگر `kimia-publish` در PATH نیست، از خود ساخت استفاده کن:
> ```bash
> ./build-Release/install/bin/kimia-publish ...
> ```
> مسیر بیلد روی سیستم شما بسته به preset متفاوت است.

### مرحلهٔ ۵: اجرا در مرورگر

```bash
cd dist
./play.sh   # یک سرور پایتون روی 8080 بالا میآورد
```

مرورگر را باز کن: `http://localhost:8080`. بازی باید با کیبورد (WASD + Space) کار کند.

---

## ۶. گردش کار جایگزین: با ویرایشگر KIMIA

اگر ترجیح میدهی با UI کار کنی، ویرایشگر داخلی هست:

```bash
kimia-editor
```

در منوی **File → New** یک پروژه بساز. در پنل `/bench` میتوانی:

- Entityها را با لمس/کلیک انتخاب کنی (از لیست سمت چپ).
- Propertyها را در پنل سمت راست ویرایش کنی.
- پروفایل را از تب **Profile** بسازی.
- با دکمهٔ **Play** بلافاصله در پنجرهٔ سمت چپ تست کنی.

سپس از **File → Publish** خروجی بگیر (همان `kimia-publish`).

---

## ۷. اضافه کردن منطق (اختیاری)

برای بازیهای ساده نیازی به منطق نیست - قوانین پیشفرض در پروفایل هست. ولی اگر میخواهی رفتار سفارشی اضافه کنی، فایل `logic.kimia` بساز:

```
WHEN score_changes DO
  IF team_a > team_b DO
    SHOW "آبی جلوست!"
  END
END
```

سینتکس کامل در `Documentation/Logic.md`.

---

## ۸. تست قبل از انتشار

سه تست سریع:

```bash
# 1) پروفایل معتبر است؟
./build-Release/install/bin/kimia-cli --validate-profile my-game.kimiaprofile

# 2) صحنه بدون خطا لود میشود؟
./build-Release/install/bin/kimia-cli --validate-scene my-game.kimia

# 3) یک فریم رندر میشود؟
./build-Release/install/bin/kimia-cli --render-test my-game.kimia out.png
```

اگر هر سه سبز شد، آمادهٔ انتشار هستی.

---

## ۹. اشتباهات رایج

| مشکل | علت | راهحل |
| --- | --- | --- |
| توپ روی زمین فرو میرود | `pos y` کم است | `pos 0 0.15 0` یعنی شعاع توپ |
| دیوار نامرئی است | `color` همه صفر | حداقل یک کانال رنگ > 0.1 |
| دروازه گل نمیخورد | `scoring hole` در گلف | بازی فوتبال باید `scoring gate` باشد |
| بازی وب بالا نمیآید | CORS | حتماً از `play.sh` استفاده کن، نه `file://` |
| مدل OBJ نامرئی | Normals اشتباه | مدل را با CCW و outward بساز |

---

## ۱۰. گام بعدی

- **رنگ و نور**: `Documentation/Rendering.md` - متریال PBR، سایه، tone map.
- **فیزیک دقیق**: `Documentation/Physics.md` - ثابت پرش، اصطکاک، باد.
- **انیمیشن**: `Documentation/Animation.md` - اگر میخواهی شخصیت بدود.
- **HUD و منو**: `Documentation/Logic.md` - نمایش امتیاز، تایمر.
- **خروجی اندروید**: `Documentation/Android.md` - همین بازی در ۵ دقیقه APK میشود.
- **خروجی ویندوز**: `Documentation/Windows.md` - EXE تکفایلی.

---

## خلاصه

```
1. پروفایل بنویس (قوانین بازی)
2. صحنه بنویس (اشیاء و توپ)
3. اختیاری: logic.kimia (رفتار سفارشی)
4. اختیاری: assets/ (مدل سفارشی)
5. تست با CLI
6. Publish → خروجی وب/اندروید/ویندوز
```

کل چرخه برای یک بازی ساده کمتر از **۱۵ دقیقه** طول میکشد.
