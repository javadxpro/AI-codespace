// Selection implementation — see Selection.h.
#include <kimia/Selection.h>

#include <algorithm>

namespace kimia::ui {

namespace {

bool rectsIntersect(const Rect& a, const Rect& b) {
  return a.x < b.x + b.w
      && a.x + a.w > b.x
      && a.y < b.y + b.h
      && a.y + a.h > b.y;
}

}  // namespace

std::vector<std::string> shiftTapSelect(
    const std::vector<std::string>& selection,
    const std::string& name) {
  std::vector<std::string> out = selection;
  const auto it = std::find(out.begin(), out.end(), name);
  if (it == out.end()) {
    out.push_back(name);
  } else {
    out.erase(it);
  }
  return out;
}

std::vector<std::string> boxSelect(
    const std::vector<std::string>& allNames,
    const std::vector<Rect>& allScreenRects,
    const Rect& box) {
  std::vector<std::string> out;
  const std::size_t n = std::min(allNames.size(), allScreenRects.size());
  for (std::size_t i = 0; i < n; ++i) {
    if (rectsIntersect(allScreenRects[i], box)) {
      out.push_back(allNames[i]);
    }
  }
  return out;
}

std::vector<std::string> singleSelect(
    const std::vector<std::string>& /*prev*/,
    const std::string& name) {
  return {name};
}

}  // namespace kimia::ui
