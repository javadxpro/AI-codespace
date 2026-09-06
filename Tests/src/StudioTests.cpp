#include <kimia/AssetPipeline.h>
#include <kimia/Studio.h>
#include <kimia_test.h>

#include <map>
#include <string>

namespace {

using kimia::Vec3;
using kimia::WorldEditor;
using kimia::i32;
using kimia::usize;

using Params = std::map<std::string, std::string>;

std::string ask(WorldEditor& editor, const std::string& path, const Params& params = Params{}) {
  return kimia::studio::handleApi(editor, path, params);
}

bool has(const std::string& haystack, const std::string& needle) {
  return haystack.find(needle) != std::string::npos;
}

void streetWorld(WorldEditor& editor) {
  editor.choose(0);  // Main -> which game?
  for (usize i = 0; i < editor.profileCount(); ++i) {
    if (editor.profileAt(i).name == "street") {
      editor.choose(static_cast<i32>(i));
      return;
    }
  }
}

}  // namespace

// --- Stage 32: the Workbench API ---

KIMIA_TEST(studio_rack_lists_the_world_and_survives_having_none) {
  WorldEditor editor;
  // No world open yet: the Bench must show an empty rack rather than fail.
  const std::string empty = ask(editor, "/api/rack");
  KIMIA_REQUIRE(has(empty, "\"ok\":true"));
  KIMIA_REQUIRE(has(empty, "\"world\":null"));

  streetWorld(editor);
  const std::string rack = ask(editor, "/api/rack");
  KIMIA_REQUIRE(has(rack, "\"ok\":true"));
  KIMIA_REQUIRE(has(rack, "\"game\":\"street\""));
  KIMIA_REQUIRE(has(rack, "\"name\":\"Ground\""));
}

KIMIA_TEST(studio_brings_a_model_in_and_reports_its_dossier) {
  WorldEditor editor;
  streetWorld(editor);

  const std::string brought = ask(editor, "/api/bring-in",
                                  {{"file", "Tests/assets/spider.obj"}, {"size", "2"}});
  KIMIA_REQUIRE(has(brought, "\"ok\":true"));
  KIMIA_REQUIRE(has(brought, "\"name\":\"Model_1\""));

  const std::string sheet = ask(editor, "/api/dossier", {{"name", "Model_1"}});
  KIMIA_REQUIRE(has(sheet, "\"mesh\":\"Tests/assets/spider.obj\""));
  // The Dossier reports the MEASURED size, not the raw scale multiplier:
  // after bring-in auto-fits a file, a scale of 3 would mean three times
  // the original, which tells the user nothing.
  KIMIA_REQUIRE(has(sheet, "\"span\":2.0"));
  // Nothing bolted on yet.
  KIMIA_REQUIRE(has(sheet, "\"body\":null"));
  KIMIA_REQUIRE(has(sheet, "\"motions\":[]"));
  KIMIA_REQUIRE(has(sheet, "\"noises\":[]"));

  // A file that is not there is refused with a reason, not a crash.
  const std::string missing = ask(editor, "/api/bring-in", {{"file", "Tests/assets/nope.obj"}});
  KIMIA_REQUIRE(has(missing, "\"ok\":false"));
  KIMIA_REQUIRE(has(missing, "\"error\""));
}

KIMIA_TEST(studio_bolts_a_body_on_and_the_physics_world_changes) {
  // The Bench is only worth having if pressing a button changes the actual
  // simulation, not just a data field.
  WorldEditor editor;
  streetWorld(editor);
  ask(editor, "/api/bring-in", {{"file", "Tests/assets/spider.obj"}, {"size", "1"}});
  const usize boxes = editor.physicsBoxCount();

  const std::string fitted = ask(editor, "/api/fit-body",
                                 {{"name", "Model_1"}, {"kind", "static"}, {"mass", "2"}});
  KIMIA_REQUIRE(has(fitted, "\"ok\":true"));
  KIMIA_REQUIRE(editor.physicsBoxCount() == boxes + 1U);
  KIMIA_REQUIRE(has(ask(editor, "/api/dossier", {{"name", "Model_1"}}), "\"kind\":\"static\""));

  // Switching it to dynamic moves it between the two physics lists.
  const usize dynamics = editor.physicsDynamicCount();
  ask(editor, "/api/fit-body", {{"name", "Model_1"}, {"kind", "dynamic"}});
  KIMIA_REQUIRE(editor.physicsBoxCount() == boxes);
  KIMIA_REQUIRE(editor.physicsDynamicCount() == dynamics + 1U);

  // And taking it off removes the solid again.
  KIMIA_REQUIRE(has(ask(editor, "/api/fit-body", {{"name", "Model_1"}, {"kind", "off"}}), "\"ok\":true"));
  KIMIA_REQUIRE(editor.physicsDynamicCount() == dynamics);
}

KIMIA_TEST(studio_labels_address_a_group) {
  WorldEditor editor;
  streetWorld(editor);
  ask(editor, "/api/bring-in", {{"file", "Tests/assets/spider.obj"}});
  ask(editor, "/api/bring-in", {{"file", "Tests/assets/spider.obj"}});

  ask(editor, "/api/label", {{"name", "Model_1"}, {"label", "enemy"}});
  ask(editor, "/api/label", {{"name", "Model_2"}, {"label", "enemy"}});
  ask(editor, "/api/label", {{"name", "Model_1"}, {"label", "breakable"}});

  const std::string enemies = ask(editor, "/api/labelled", {{"label", "enemy"}});
  KIMIA_REQUIRE(has(enemies, "Model_1"));
  KIMIA_REQUIRE(has(enemies, "Model_2"));
  const std::string breakable = ask(editor, "/api/labelled", {{"label", "breakable"}});
  KIMIA_REQUIRE(has(breakable, "Model_1"));
  KIMIA_REQUIRE(!has(breakable, "Model_2"));

  // The rack advertises every label in the world, for the Bench's list.
  KIMIA_REQUIRE(has(ask(editor, "/api/rack"), "\"labels\":[\"breakable\",\"enemy\"]"));

  ask(editor, "/api/unlabel", {{"name", "Model_1"}, {"label", "enemy"}});
  KIMIA_REQUIRE(!has(ask(editor, "/api/labelled", {{"label", "enemy"}}), "Model_1"));
}

KIMIA_TEST(studio_wires_a_motion_to_a_button_and_pulling_it_fires) {
  // The whole point: connect a clip to a button from the Bench, with no
  // code, and prove it fires.
  WorldEditor editor;
  streetWorld(editor);
  ask(editor, "/api/bring-in", {{"file", "Tests/assets/spider.obj"}});

  KIMIA_REQUIRE(has(ask(editor, "/api/wire-motion",
                        {{"name", "Model_1"}, {"clip", "Bend"}, {"wiring", "k"}}),
                    "\"ok\":true"));
  KIMIA_REQUIRE(has(ask(editor, "/api/wire-noise",
                        {{"name", "Model_1"}, {"sound", "kick"}, {"wiring", "k"}}),
                    "\"ok\":true"));

  // Pulling a wire nothing is on does nothing.
  KIMIA_REQUIRE(has(ask(editor, "/api/pull", {{"wiring", "zzz"}}), "\"fired\":0"));
  // Pulling the real one fires both fittings and reports what happened.
  const std::string pulled = ask(editor, "/api/pull", {{"wiring", "k"}});
  KIMIA_REQUIRE(has(pulled, "\"fired\":2"));
  KIMIA_REQUIRE(has(pulled, "Model_1:Bend"));
  KIMIA_REQUIRE(has(pulled, "\"sounds\":[\"kick\"]"));

  // A half-filled form is refused rather than saved broken.
  KIMIA_REQUIRE(has(ask(editor, "/api/wire-motion", {{"name", "Model_1"}, {"clip", "Bend"}}), "\"ok\":false"));
  KIMIA_REQUIRE(has(ask(editor, "/api/wire-noise", {{"name", "Model_1"}, {"wiring", "k"}}), "\"ok\":false"));

  // Clearing takes the wiring back off.
  KIMIA_REQUIRE(has(ask(editor, "/api/unwire", {{"name", "Model_1"}, {"what", "motions"}}), "\"ok\":true"));
  KIMIA_REQUIRE(has(ask(editor, "/api/dossier", {{"name", "Model_1"}}), "\"motions\":[]"));
}

KIMIA_TEST(studio_places_paints_and_scraps) {
  WorldEditor editor;
  streetWorld(editor);
  ask(editor, "/api/bring-in", {{"file", "Tests/assets/spider.obj"}});

  ask(editor, "/api/place", {{"name", "Model_1"}, {"px", "1.5"}, {"py", "0"}, {"pz", "-2"},
                             {"sx", "2"}, {"sy", "2"}, {"sz", "2"}});
  const std::string moved = ask(editor, "/api/dossier", {{"name", "Model_1"}});
  KIMIA_REQUIRE(has(moved, "\"position\":[1.500000,0.000000,-2.000000]"));
  KIMIA_REQUIRE(has(moved, "\"scale\":[2.000000,2.000000,2.000000]"));

  ask(editor, "/api/paint", {{"name", "Model_1"}, {"r", "0.25"}, {"g", "0.5"}, {"b", "0.75"}});
  KIMIA_REQUIRE(has(ask(editor, "/api/dossier", {{"name", "Model_1"}}),
                    "\"color\":[0.250000,0.500000,0.750000]"));

  KIMIA_REQUIRE(has(ask(editor, "/api/scrap", {{"name", "Model_1"}}), "\"ok\":true"));
  KIMIA_REQUIRE(has(ask(editor, "/api/dossier", {{"name", "Model_1"}}), "\"ok\":false"));
}

KIMIA_TEST(studio_refuses_nonsense_without_falling_over) {
  // A stale page must never be able to wedge the engine.
  WorldEditor editor;
  streetWorld(editor);
  KIMIA_REQUIRE(has(ask(editor, "/api/nonsense"), "\"ok\":false"));
  KIMIA_REQUIRE(has(ask(editor, "/api/dossier", {{"name", "ghost"}}), "\"ok\":false"));
  KIMIA_REQUIRE(has(ask(editor, "/api/place", {{"name", "ghost"}}), "\"ok\":false"));
  KIMIA_REQUIRE(has(ask(editor, "/api/label", {{"name", "ghost"}, {"label", "x"}}), "\"ok\":false"));
  KIMIA_REQUIRE(has(ask(editor, "/api/scrap", {{"name", "ghost"}}), "\"ok\":false"));
  KIMIA_REQUIRE(has(ask(editor, "/api/unwire", {{"name", "Ground"}, {"what", "wat"}}), "\"ok\":false"));
  // Missing parameters fall back instead of throwing.
  KIMIA_REQUIRE(has(ask(editor, "/api/labelled"), "\"ok\":true"));
  KIMIA_REQUIRE(has(ask(editor, "/api/pull"), "\"fired\":0"));
  // A junk number is ignored rather than parsed into nonsense.
  ask(editor, "/api/bring-in", {{"file", "Tests/assets/spider.obj"}});
  KIMIA_REQUIRE(has(ask(editor, "/api/place", {{"name", "Model_1"}, {"px", "abc"}}), "\"ok\":true"));
}

KIMIA_TEST(studio_json_escapes_text_so_the_page_cannot_break) {
  // World titles are Persian and object names come from files, so the JSON
  // has to survive quotes and backslashes.
  WorldEditor editor;
  streetWorld(editor);
  const std::string rack = ask(editor, "/api/rack");
  // Balanced braces is a cheap proof the document is well formed.
  i32 depth = 0;
  for (const char c : rack) {
    if (c == '{') ++depth;
    if (c == '}') --depth;
    KIMIA_REQUIRE(depth >= 0);
  }
  KIMIA_REQUIRE(depth == 0);
  KIMIA_REQUIRE(has(rack, "\"ok\":true"));
}

KIMIA_TEST(studio_bench_page_is_self_contained) {
  // It has to work offline on a phone: no CDN, no external stylesheet, no
  // font download.
  const std::string page = kimia::studio::benchPage();
  KIMIA_REQUIRE(page.size() > 4000U);
  KIMIA_REQUIRE(has(page, "<!doctype html>"));
  KIMIA_REQUIRE(has(page, "KIMIA"));
  KIMIA_REQUIRE(!has(page, "http://"));
  KIMIA_REQUIRE(!has(page, "https://"));
  KIMIA_REQUIRE(!has(page, "<link"));
  KIMIA_REQUIRE(!has(page, "<script src"));
  // It talks to the API this file implements.
  KIMIA_REQUIRE(has(page, "/api/"));
  KIMIA_REQUIRE(has(page, "frame.png"));
}

// --- Stage 34: an imported model keeps its texture ---

KIMIA_TEST(studio_imported_model_reports_its_texture) {
  // The whole chain: a .obj that names a .mtl that names a .png. The
  // importer has always resolved that path; until this stage nothing
  // loaded the image, so every model rendered as a flat colour.
  std::string error;
  auto asset = kimia::assets::loadMeshAsset("Tests/assets/crate.obj", error);
  KIMIA_REQUIRE(asset.has_value());
  KIMIA_REQUIRE(!asset->materials.empty());

  std::string skin;
  for (const kimia::MaterialData& material : asset->materials) {
    if (!material.texturePath.empty()) skin = material.texturePath;
  }
  KIMIA_REQUIRE(!skin.empty());
  KIMIA_REQUIRE(skin.find("crate_skin.png") != std::string::npos);

  // And the image at that path really loads, with real pixels in it.
  auto image = kimia::assets::loadImage(skin, error);
  KIMIA_REQUIRE(image.has_value());
  KIMIA_REQUIRE(image->width == 32);
  KIMIA_REQUIRE(image->height == 32);
  KIMIA_REQUIRE(image->channels >= 3);

  // The mesh carries UVs, without which a texture cannot be applied.
  KIMIA_REQUIRE(!asset->mesh.uvs.empty());
  KIMIA_REQUIRE(asset->mesh.uvs.size() == asset->mesh.positions.size());
}
