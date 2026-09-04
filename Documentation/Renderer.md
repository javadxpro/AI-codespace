# رندرینگ (Engine/Renderer)

دو مسیر رندر مستقل با یک قرارداد صحنهٔ مشترک: خط لولهٔ GL 3.3 برای ماشین‌های
دارای درایور، و رسترایزر نرم‌افزاری برای Termux/ماشین‌های بدون GL (تضمین‌شده).

## قرارداد صحنه

- `RenderScene`: `view`، `projection`، `cameraPosition`، `lightDirection`
  (جهت حرکت نور)، `ambient`، و `std::vector<RenderObject>`.
- `RenderObject`: اشاره‌گر `MeshData` + ماتریس مدل + رنگ + ضریب
  (ambient/diffuse/specular یکسان — صحنهٔ سادهٔ Phong).
- خروجی هر دو مسیر: `Image` با ۳ کانال، ۸ بیت، بالابه‌پایین.

## خط لولهٔ GL (اختیاری)

- `GLFunctions`: لودر `dlopen` (`libGL.so.1` سپس `libGL.so`) — بدون نیاز به
  هدر GL. اگر کتابخانه‌ای نباشد، همهٔ صداها **no-op امن** می‌شوند و
  `loaded() == false`. زیرمجموعهٔ GLSL 330 core لازم برای Phong + سایه.
- `EGLContext`: پیاده‌سازی `dlopen`-only اتصال EGL (بدون هدر) با سطح
  **pbuffer** — در ماشین‌های بدون نمایشگر هم رندر آف‌اسکرین ممکن است.
  روی Android تنظیم `LIBGL_ALWAYS_SOFTWARE=1` (در صورت عدم تنظیم) انجام
  می‌شود تا Mesa/llvmpipe همیشه جواب بدهد.
- `Shader`: کامپایل/لینک با گزارش کامل info log؛ setMat4/setVec3/setFloat/
  setInt؛ move-only.
- `GpuMesh`: بافر interleaved (pos3f+normal3f+uv2f) با VAO/VBO/EBO؛ move-only.
- `Renderer`: سایهٔ PCF با shadow map 1024² و depth FBO؛ گاما؛
  `capturePNG` از طریق readPixels + flip + `Image::encodePNG`؛ کش
  `meshFor` (یک GpuMesh به ازای هر MeshData). بعداً نقطهٔ تعویض به Vulkan
  است — رابط `Renderer` مستقل از GL می‌ماند.

## رسترایزر نرم‌افزاری (`renderSoftware`)

مسیر **تضمینی** که همیشه کار می‌کند (Termux، سرور، سندباکس بدون GL):

- z-buffer با عمق تصحیح‌شدهٔ پرسپکتیو (`1/w` میان‌یابی)؛
- حذف سطح پشتی با نرمال جهان (بدون نیاز به ترتیب مثلث)؛
- ردّ مثلث‌هایی که صفحهٔ near را قطع می‌کنند؛
- سایه‌زنی تخت Lambert + ambient، کدگذاری گاما **یک بار به ازای مثلث**؛
- سقف اندازهٔ تصویر 8192² (ردّ درخواست‌های ناممکن).

## تست‌ها

`Tests/src/RendererTests.cpp`:

- مکعب قرمز: پیکسل مرکزی وجه جلو `R∈(190,230)` بعد از گاما، رنگ پاک‌سازی
  دقیق، بیش از ۳۰۰۰ پیکسل قرمز؛
- صفحهٔ سبز زیر مکعب با پیکسل سبز-غالب دقیق؛
- حذف سطح پشتی (سمت دور هرگز پس‌زمینه را نمی‌پوشاند)؛
- ردّ اندازهٔ نامعتبر؛
- مسیر GL: در نبود EGL **تمیز skip** می‌شود؛ با درایور، رندر کامل +
  `capturePNG` + shutdown.
