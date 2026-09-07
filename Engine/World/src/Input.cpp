#include <kimia/Input.h>

#include <cmath>

namespace kimia {

const char* sourceName(Source source) {
  switch (source) {
    case Source::Touch: return "touch";
    case Source::Pad: return "pad";
    case Source::Stick: return "stick";
    case Source::Key: break;
  }
  return "key";
}

bool sourceFromName(const std::string& name, Source& out) {
  if (name == "key") {
    out = Source::Key;
    return true;
  }
  if (name == "touch") {
    out = Source::Touch;
    return true;
  }
  if (name == "pad") {
    out = Source::Pad;
    return true;
  }
  if (name == "stick") {
    out = Source::Stick;
    return true;
  }
  return false;
}

const Control* InputMap::find(const std::string& name) const {
  for (const Control& control : controls) {
    if (control.name == name) return &control;
  }
  return nullptr;
}

Control* InputMap::find(const std::string& name) {
  for (Control& control : controls) {
    if (control.name == name) return &control;
  }
  return nullptr;
}

void InputMap::set(const Control& control) {
  if (control.name.empty()) return;
  for (Control& existing : controls) {
    if (existing.name != control.name) continue;
    existing = control;  // editing an action replaces it
    return;
  }
  controls.push_back(control);
}

bool InputMap::remove(const std::string& name) {
  for (usize i = 0; i < controls.size(); ++i) {
    if (controls[i].name != name) continue;
    controls.erase(controls.begin() + static_cast<std::ptrdiff_t>(i));
    return true;
  }
  return false;
}

std::string InputMap::actionFor(Source source, const std::string& code) const {
  for (const Control& control : controls) {
    for (const Binding& binding : control.bindings) {
      if (binding.source != source) continue;
      // A touch binding's code is the action's own name by default, so
      // adding an on-screen button needs no second piece of naming.
      const std::string& expect = binding.code.empty() ? control.name : binding.code;
      if (expect == code) return control.name;
    }
  }
  return std::string();
}

std::vector<const Control*> InputMap::touchControls() const {
  std::vector<const Control*> shown;
  for (const Control& control : controls) {
    for (const Binding& binding : control.bindings) {
      if (binding.source != Source::Touch) continue;
      shown.push_back(&control);
      break;  // one button per control, however many bindings it has
    }
  }
  return shown;
}

std::string InputMap::touchAt(f64 pixelX, f64 pixelY, i32 width, i32 height) const {
  if (width <= 0 || height <= 0) return std::string();
  // Later actions are drawn on top, so they answer first: a person means
  // the button they can see.
  const std::vector<const Control*> shown = touchControls();
  for (usize i = shown.size(); i > 0U; --i) {
    const Control& control = *shown[i - 1U];
    const f64 centreX = control.spot.x * static_cast<f64>(width);
    const f64 centreY = control.spot.y * static_cast<f64>(height);
    // Round buttons: a finger is round, and a circle is far more
    // forgiving at the corners than a square of the same size.
    const f64 radius = control.spot.size * 0.5 * static_cast<f64>(width < height ? width : height);
    const f64 dx = pixelX - centreX;
    const f64 dy = pixelY - centreY;
    if (std::sqrt(dx * dx + dy * dy) <= radius) return control.name;
  }
  return std::string();
}

}  // namespace kimia
