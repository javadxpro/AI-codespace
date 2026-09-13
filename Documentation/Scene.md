# صحنه و SceneIO

موجودیت‌ها، فرمت متنی v1 و قراردادهای ریاضی.

---

## مفاهیم

- **EntityHandle**: شناسهٔ `u32` **۱‌مبنا**؛ `0` (`kNullEntity`) یعنی «هیچ». هندل‌ها هرگز بازاستفاده نمی‌شوند.
- **EntityData**: نام، `Transform` (موقعیت/مقیاس/چرخش)، نوع مش (`cube/plane/sphere`)، رنگ، زبری، metalness، emissive، شفافیت، برچسب‌ها.
- **Scene**: `create / destroy / get / alive / count / forEach / clear` — تکرار به ترتیب هندل (قطعی، برای سریال‌سازی پایدار).

## فرمت متنی v1

```text
# KIMIA scene v1
e "Green" mesh plane pos 0 0 0 scale 1 1 1 color 0.22 0.45 0.24 rough 0.95
e "Ball" mesh sphere pos 0 0 0 scale 1 1 1 color 0.95 0.95 0.92 rough 0.3
# demo 0.000000 0.610000
```

قواعد:

- خطوط `#` کامنت‌اند، به‌جز `# demo <aim> <power>` (شات دموی نویسنده).
- `mesh` یکی از `cube / plane / sphere` است.
- چرخش اختیاری: `rot x y z w` فقط وقتی جسم واقعاً چرخیده است.
- نام داخل گیومهٔ دوتایی؛ escape های `\"` و `\\`.
- بارگذاری مقاوم: کلیدواژهٔ ناشناخته رد می‌شود، خط ناقص نادیده گرفته می‌شود.
- اعداد با `setprecision(9)` قطعی — `save → load → save` بایت‌به‌بایت یکسان است.

## قراردادهای ریاضی (`Engine/Math`)

- مختصات **راست‌دست**: `+X` راست، `+Y` بالا، `−Z` جلو.
- ماتریس‌ها **ستون‌عمده**؛ اندیس `at(column, row)`.
- زاویه‌ها **رادیان**؛ تبدیل در `MathUtils.h`.
- نرمال‌ماتریس: برای مقیاس غیریکنواخت از `inverseTranspose()` استفاده کن.
- `Mat4::perspective` عمق کلیپ `[−1, 1]` (سبک OpenGL) می‌دهد.

| فایل | محتوا |
| --- | --- |
| `Vec.h` | `Vec2/3/4` با عملگرها، طول، نرمال‌سازی، ضرب داخلی/خارجی |
| `Mat4.h` | ضرب، ترانهاده، دترمینان، معکوس، `perspective`، `lookAt`، `rotationX/Y/Z` |
| `Quat.h` | از محور-زاویه/ماتریس، ضرب، چرخش بردار، `toMat4`، رفت‌وبرگشت اویلر |
| `Camera.h` | دوربین پرسپکتیو و ماتریس‌های view/projection |
| `MathUtils.h` | `kPi`، `radians`، `degrees`، `clamp`، `lerp`، `approxEqual` |

## تست

`Tests/src/SceneTests.cpp` (هندل‌ها، round-trip بایت‌یکسان، بارگذاری مقاوم، نام‌های دارای فاصله) و `Tests/src/MathTests.cpp` (مقادیر دقیق projection/lookAt/چرخش/کواترنیون).

---
