#include <kimia/Theme.h>

namespace kimia::ui::theme {

namespace {
f32 gDpi = 2.0f;  // overwritten by resize(); default assumes a phone
}  // namespace

void setDpi(f32 dpi) { gDpi = dpi; }
f32 dpi() { return gDpi; }

f32 dp(i32 v) { return static_cast<f32>(v) * gDpi; }
f32 sp(i32 v) { return static_cast<f32>(v) * gDpi; }

}  // namespace kimia::ui::theme
