# PS4 (لینوکس)

PS4 یک کامپیوتر x86-64 است و موتور C++17 قابل‌حمل است — به «پورت» نیاز ندارد، فقط به یک بیلد headless که به GPU و پنجره تکیه نکند.

---

## چرا بدون GPU

درایور شتاب‌سخت‌افزاری روی لینوکسِ PS4 (RadeonSI/AMDGPU) وصله‌ای، تجربی و قفل به کرنل خاص است و ممکن است ناپایدار باشد. مسیر تضمینی:

- **رسترایزر نرم‌افزاری** (`renderSoftware`) — بدون هیچ GPU.
- **WebViewer** — بازی/ویرایشگر روی یک پورت HTTP؛ از مرورگر گوشی/کامپیوتر روی همان شبکه وصل می‌شوی؛ خود PS4 بدون مانیتور هم اجرا می‌شود.

## شرط‌ها

- فریمور قابل اکسپلویت؛ **9.00** یکی از پایدارترین‌هاست. جیلبریک ماندگار نیست (بعد از هر ریبوت دوباره).
- CPU جگوار کند است: بیلد طول می‌کشد و رندر نرم‌افزاری در رزولوشن پایین است.

## اجرا

```bash
# ۱) لینوکس را بالا بیاور (payload Linux؛ توزیع مخصوص PS4 مثل psxitarch)
# ۲) کد را بگیر
git clone --branch arena/01a080a4-ai-codespace https://github.com/javadxpro/AI-codespace.git
cd AI-codespace
# ۳) بیلد
bash Tools/ps4_build.sh           # --run / --clean / --port=N
# ۴) اجرا و اتصال از گوشی
./build-ps4/bin/kimia_world --port 8080 --bind 0.0.0.0 --auth CHANGE_ME \
  --profiles build-ps4/bin/profiles
ip addr | grep 'inet '
# سپس: http://<IP-PS4>:8080/?token=CHANGE_ME   (بازی)
#       http://<IP-PS4>:8080/bench?token=CHANGE_ME   (ویرایشگر)
```

`--bind 0.0.0.0` یعنی کل LAN می‌تواند وصل شود؛ `--auth` با token جلوی دسترسی غریبه را می‌گیرد. پورت را NAT نکن.

## GPU (اختیاری)

برای درگیر کردن GPU خودِ PS4 (چیپ GCN «Liverpool») سه شرط هم‌زمان لازم‌اند — موتور خودش مسیر GL سخت‌افزاری را برمی‌دارد:

1. کرنل با درایور `amdgpu`/`radeon` که Liverpool را بشناسد.
2. Mesa وصله‌دار با RadeonSI.
3. `libGL.so.1` و `libEGL.so.1` روی دیسک (dlopen).

بررسی وضعیت:

```bash
bash Tools/ps4_gpu.sh            # گزارش سه لایه + فرمان نصب
bash Tools/ps4_gpu.sh --install  # نصب سمت کاربر (Mesa/libGL)
./build-ps4/bin/kimia_world --gpuinfo   # EGL context: yes / SOFTWARE
```

> حتی با GPU فعال، CPU کند است؛ و درایور تجربی است — مسیر نرم‌افزاری همیشه پشتیبان است.

## وضعیت

| مسیر | روی PS4 |
| --- | --- |
| رندر نرم‌افزاری + WebViewer | ✅ تضمینی |
| ریتریس آفلاین PNG | ✅ کند ولی کار می‌کند |
| OpenGL سخت‌افزاری | ⚠️ تجربی (درایور وصله‌ای) |
| D3D11 | ❌ فقط ویندوز |

---
