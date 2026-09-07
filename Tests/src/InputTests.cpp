#include <kimia/Assets.h>
#include <kimia/Input.h>
#include <kimia_test.h>

#include <string>

namespace {

using kimia::Binding;
using kimia::Control;
using kimia::InputMap;
using kimia::Source;
using kimia::f64;
using kimia::usize;

Control makeControl(const char* name) {
  Control control;
  control.name = name;
  control.spot.label = name;
  return control;
}

void bind(Control& control, Source source, const char* code) {
  Binding binding;
  binding.source = source;
  binding.code = code;
  control.bindings.push_back(binding);
}

}  // namespace

// --- The input system: one action, many ways to do it ---

KIMIA_TEST(input_one_control_answers_to_key_touch_and_pad) {
  // The whole point: a game says "jump", and the player's hardware is
  // the engine's problem rather than the rule author's.
  InputMap map;
  Control jump = makeControl("jump");
  bind(jump, Source::Key, "space");
  bind(jump, Source::Pad, "a");
  bind(jump, Source::Touch, "jump");
  map.set(jump);

  KIMIA_REQUIRE(map.actionFor(Source::Key, "space") == "jump");
  KIMIA_REQUIRE(map.actionFor(Source::Pad, "a") == "jump");
  KIMIA_REQUIRE(map.actionFor(Source::Touch, "jump") == "jump");
  // The same code on a DIFFERENT device is not the same control: "a" on a
  // pad must not fire because someone typed the letter a.
  KIMIA_REQUIRE(map.actionFor(Source::Key, "a").empty());
  KIMIA_REQUIRE(map.actionFor(Source::Key, "nothing").empty());

  // A touch binding with no code defaults to the control's own name, so
  // adding an on-screen button needs no second piece of naming.
  Control kick = makeControl("kick");
  Binding screen;
  screen.source = Source::Touch;
  map.set(kick);
  Control* stored = map.find("kick");
  stored->bindings.push_back(screen);
  KIMIA_REQUIRE(map.actionFor(Source::Touch, "kick") == "kick");
}

KIMIA_TEST(input_controls_are_kept_by_name) {
  InputMap map;
  map.set(makeControl("jump"));
  KIMIA_REQUIRE(map.controls.size() == 1U);

  // Editing a control replaces it rather than making a second one that
  // answers to the same name.
  Control edited = makeControl("jump");
  edited.spot.x = 0.5;
  map.set(edited);
  KIMIA_REQUIRE(map.controls.size() == 1U);
  KIMIA_REQUIRE(map.find("jump")->spot.x == 0.5);

  map.set(makeControl(""));  // nameless: refused, not saved unreachable
  KIMIA_REQUIRE(map.controls.size() == 1U);

  KIMIA_REQUIRE(map.find("nope") == nullptr);
  KIMIA_REQUIRE(map.remove("jump"));
  KIMIA_REQUIRE(!map.remove("jump"));
}

KIMIA_TEST(input_only_touch_bound_controls_get_a_button) {
  // A key-only control must not put a button on the player's screen.
  InputMap map;
  Control keyOnly = makeControl("reload");
  bind(keyOnly, Source::Key, "r");
  map.set(keyOnly);
  Control onScreen = makeControl("jump");
  bind(onScreen, Source::Touch, "jump");
  bind(onScreen, Source::Key, "space");
  map.set(onScreen);

  const std::vector<const Control*> shown = map.touchControls();
  KIMIA_REQUIRE(shown.size() == 1U);
  KIMIA_REQUIRE(shown[0]->name == "jump");
}

KIMIA_TEST(input_a_touch_lands_on_the_button_it_looks_like) {
  InputMap map;
  Control jump = makeControl("jump");
  jump.spot.x = 0.8;
  jump.spot.y = 0.8;
  jump.spot.size = 0.2;
  bind(jump, Source::Touch, "jump");
  map.set(jump);

  // The middle of the button, on a 400x400 screen: centre (320, 320),
  // radius 40.
  KIMIA_REQUIRE(map.touchAt(320.0, 320.0, 400, 400) == "jump");
  KIMIA_REQUIRE(map.touchAt(330.0, 330.0, 400, 400) == "jump");
  // Well outside it, nothing.
  KIMIA_REQUIRE(map.touchAt(100.0, 100.0, 400, 400).empty());
  // A corner of the enclosing square is OUTSIDE the circle: buttons are
  // round because fingers are, and a square would steal nearby taps.
  KIMIA_REQUIRE(map.touchAt(320.0 + 38.0, 320.0 + 38.0, 400, 400).empty());

  // Positions are fractions, so the same layout works at any size.
  KIMIA_REQUIRE(map.touchAt(640.0, 640.0, 800, 800) == "jump");
  // A screen with no size cannot be touched, rather than dividing by zero.
  KIMIA_REQUIRE(map.touchAt(10.0, 10.0, 0, 0).empty());
}

KIMIA_TEST(input_source_names_survive_a_round_trip) {
  for (const char* name : {"key", "touch", "pad", "stick"}) {
    Source source = Source::Key;
    KIMIA_REQUIRE(kimia::sourceFromName(name, source));
    KIMIA_REQUIRE(std::string(kimia::sourceName(source)) == name);
  }
  Source source = Source::Pad;
  KIMIA_REQUIRE(!kimia::sourceFromName("nonsense", source));
  KIMIA_REQUIRE(source == Source::Pad);  // left alone
}

// --- Scanning the folder the user dropped files into ---

KIMIA_TEST(assets_a_scan_sorts_and_recognises_what_it_finds) {
  const std::vector<kimia::assetscan::ScannedAsset> found =
      kimia::assetscan::scan("Tests/assets", false);
  KIMIA_REQUIRE(found.size() > 5U);

  // Sorted, so picking the third item gets the same file next time.
  for (usize i = 1; i < found.size(); ++i) {
    KIMIA_REQUIRE(found[i - 1U].file <= found[i].file);
  }

  bool sawModel = false;
  bool sawTexture = false;
  bool sawSound = false;
  for (const kimia::assetscan::ScannedAsset& asset : found) {
    // Anything the engine cannot use is left out, or the useful entries
    // would be lost among .txt and .zip files.
    KIMIA_REQUIRE(asset.kind != kimia::assetscan::AssetKind::Unknown);
    KIMIA_REQUIRE(asset.bytes > 0U);
    KIMIA_REQUIRE(asset.path.find(asset.file) != std::string::npos);
    if (asset.kind == kimia::assetscan::AssetKind::Model) sawModel = true;
    if (asset.kind == kimia::assetscan::AssetKind::Texture) sawTexture = true;
    if (asset.kind == kimia::assetscan::AssetKind::Sound) sawSound = true;
  }
  KIMIA_REQUIRE(sawModel);
  KIMIA_REQUIRE(sawTexture);
  KIMIA_REQUIRE(sawSound);

  // A folder that is not there is empty, not a crash.
  KIMIA_REQUIRE(kimia::assetscan::scan("/tmp/kimia-no-such-folder", true).empty());
}

KIMIA_TEST(assets_a_deep_scan_looks_inside_a_model) {
  // This is what lets a person PICK a clip from a list instead of
  // typing its name from memory.
  const std::vector<kimia::assetscan::ScannedAsset> found =
      kimia::assetscan::scan("Tests/assets", true);

  bool checked = false;
  for (const kimia::assetscan::ScannedAsset& asset : found) {
    if (asset.file != "skinned_bar.fbx") continue;
    checked = true;
    KIMIA_REQUIRE(asset.hasSkeleton);
    KIMIA_REQUIRE(asset.boneCount == 2U);
    KIMIA_REQUIRE(asset.clips.size() == 1U);
    KIMIA_REQUIRE(asset.clips[0] == "Bend");
    KIMIA_REQUIRE(asset.note.empty());
  }
  KIMIA_REQUIRE(checked);

  // A plain prop has no skeleton, and that is normal rather than a
  // failure worth reporting.
  for (const kimia::assetscan::ScannedAsset& asset : found) {
    if (asset.file != "crate.obj") continue;
    KIMIA_REQUIRE(!asset.hasSkeleton);
    KIMIA_REQUIRE(asset.clips.empty());
    KIMIA_REQUIRE(asset.note.empty());
  }

  // A shallow scan does NOT open anything: it is the fast path.
  for (const kimia::assetscan::ScannedAsset& asset : kimia::assetscan::scan("Tests/assets", false)) {
    KIMIA_REQUIRE(!asset.hasSkeleton);
    KIMIA_REQUIRE(asset.clips.empty());
  }
}
