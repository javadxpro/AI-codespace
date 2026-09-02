# معماری Kimiya World Editor

نسخهٔ اول یک ویرایشگر جهان سبک است: WebGL + Three.js، بدون وابستگی به GPU قوی، قابل اجرا در مرورگر Android و از طریق یک سرور محلی در Termux.

## لایه‌ها

```
UI (HUD / Touch)
    ↓ EventBus
Editor (observe / build / terrain / select + History)
    ↓
World model (Terrain, Entities, Sky, Player)
    ↓
Engine (Renderer, Camera, Input, Loop)
```

قانون طلایی: **مدل جهان منبع حقیقت است**. رندر فقط آن را نشان می‌دهد. ذخیره/بارگذاری همان مدل را به JSON می‌نویسد. قابلیت‌های آینده (هوش مصنوعی سازنده، آب‌وهوا، شهرسازی) باید World را تغییر دهند، نه GPU را مستقیم.

## ساختار فایل

| مسیر | نقش |
|---|---|
| `js/engine/` | حلقهٔ رندر کم‌مصرف، ورودی، دوربین اول/سوم شخص |
| `js/world/` | زمین ارتفاعی، موجودیت‌ها، متریال، آسمان |
| `js/editor/` | ابزار ساخت، قلم زمین، انتخاب/تبدیل، واگرد |
| `js/persist/` | localStorage + فایل `.kimiya.json` |
| `js/ui/` | HUD لمسی و جویستیک |
| `vendor/three.module.js` | Three.js r160، آفلاین |

## فرمت جهان

```json
{
  "format": "kimiya-world",
  "version": 1,
  "name": "جهان نو",
  "player": { "position": [0, 2, 16], "yaw": 3.14, "pitch": -0.12, "thirdPerson": false },
  "terrain": { "size": 64, "segments": 48, "heights": [], "colors": [] },
  "entities": [{ "id": "e_…", "type": "cube", "position": [], "rotation": [], "scale": [], "material": "wood", "color": "#c4a36a" }],
  "environment": { "sunElevation": 0.92, "sunAzimuth": 0.52 }
}
```

## نقاط توسعهٔ بعدی

- **هوش مصنوعی سازندهٔ جهان**: تابعی که `World` را از یک prompt پر کند (`world.createDefault` الگوی تولید رویه‌ای است).
- **آب‌وهوا**: `Sky.setSun` + مه و رنگ گنبد؛ سیستم ذره جداگانه اضافه شود.
- **نورپردازی پیشرفته**: فعلاً بدون سایه برای باتری؛ `DirectionalLight.castShadow` آمادهٔ فعال‌سازی است.
- **ساخت شهر**: ترکیب primitiveها + snap؛ بعداً prefab / نقشهٔ خیابان.
- **خروجی موتور بازی**: serializer می‌تواند glTF از entities بسازد؛ مدل از رندر جداست.
- **ساخت بازی کامل**: حالت Observe هستهٔ gameplay است؛ فیزیک سادهٔ بازیکن از قبل وجود دارد.

## عملکرد موبایل

- `powerPreference: 'low-power'`
- سقف pixel ratio بر اساس کیفیت
- بدون سایه، بدون post-processing
- هندسهٔ کم‌پلی، مواد Lambert/Phong
- توقف رندر وقتی تب مخفی است
- زمین ۴۸×۴۸ رأس (~2401) که روی دستگاه ضعیف هم نرم است
