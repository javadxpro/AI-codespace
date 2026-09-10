#include <kimia/AssetPipeline.h>
#include <kimia/Hud.h>
#include <kimia/Input.h>
#include <kimia/Library.h>
#include <kimia/Studio.h>
#include <kimia/WorldIO.h>
#include <kimia_test.h>

#include <filesystem>
#include <fstream>
#include <cmath>
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

bool near(kimia::f64 a, kimia::f64 b, kimia::f64 eps = 1e-9) { return std::abs(a - b) <= eps; }

bool has(const std::string& haystack, const std::string& needle) {
  return haystack.find(needle) != std::string::npos;
}

void streetWorld(WorldEditor& editor) {
  // The reference games are hidden from the menu now, so reach «street»
  // directly instead of walking the «which game?» screen.
  for (usize i = 0; i < editor.profileCount(); ++i) {
    if (editor.profileAt(i).name == "street") {
      editor.createWorld(editor.profileAt(i));
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

// --- Stage 35: a character's own bones ---

KIMIA_TEST(studio_fits_a_default_frame_you_can_then_edit) {
  // The player asked to place the bones themselves. The default frame is a
  // STARTING POINT to drag, not something to accept: it comes back as real
  // editable coordinates rather than hiding inside the engine.
  WorldEditor editor;
  streetWorld(editor);
  ask(editor, "/api/bring-in", {{"file", "Tests/assets/crate.obj"}, {"size", "1"}});

  KIMIA_REQUIRE(has(ask(editor, "/api/dossier", {{"name", "Model_1"}}), "\"bones\":[]"));
  KIMIA_REQUIRE(has(ask(editor, "/api/default-rig", {{"name", "Model_1"}, {"height", "1.7"}}), "\"ok\":true"));

  const std::string sheet = ask(editor, "/api/dossier", {{"name", "Model_1"}});
  KIMIA_REQUIRE(has(sheet, "\"name\":\"LeftLeg\""));
  KIMIA_REQUIRE(has(sheet, "\"name\":\"Head\""));
  // Each bone says where it runs from and to, and how it swings.
  KIMIA_REQUIRE(has(sheet, "\"from\":"));
  KIMIA_REQUIRE(has(sheet, "\"swing\":"));
  // A foot hangs off a leg: the parent chain is real, not decoration.
  KIMIA_REQUIRE(has(sheet, "\"parent\":\"LeftLeg\""));

  const kimia::EntityData* model = editor.entity("Model_1");
  KIMIA_REQUIRE(model != nullptr);
  KIMIA_REQUIRE(model->rig.size() == 11U);
}

KIMIA_TEST(studio_setting_a_bone_moves_it_rather_than_duplicating_it) {
  WorldEditor editor;
  streetWorld(editor);
  ask(editor, "/api/bring-in", {{"file", "Tests/assets/crate.obj"}, {"size", "1"}});
  ask(editor, "/api/default-rig", {{"name", "Model_1"}, {"height", "1.7"}});
  const kimia::usize before = editor.entity("Model_1")->rig.size();

  // Dragging a bone in the Bench calls this repeatedly with one name.
  ask(editor, "/api/set-bone", {{"name", "Model_1"}, {"bone", "LeftLeg"}, {"parent", ""},
                                {"fx", "0"}, {"fy", "0.9"}, {"fz", "0"},
                                {"tx", "0.2"}, {"ty", "0.3"}, {"tz", "0"},
                                {"thickness", "0.1"}, {"swing", "1.4"}});
  KIMIA_REQUIRE(editor.entity("Model_1")->rig.size() == before);

  bool found = false;
  for (const kimia::RigBone& bone : editor.entity("Model_1")->rig) {
    if (bone.name != "LeftLeg") continue;
    found = true;
    KIMIA_REQUIRE(near(bone.to.x, 0.2));
    KIMIA_REQUIRE(near(bone.to.y, 0.3));
    KIMIA_REQUIRE(near(bone.swing, 1.4));
  }
  KIMIA_REQUIRE(found);

  // A brand new name really is a new bone: characters are not limited to
  // the default frame's parts.
  ask(editor, "/api/set-bone", {{"name", "Model_1"}, {"bone", "Tail"}, {"parent", "Torso"},
                                {"fx", "0"}, {"fy", "0.9"}, {"fz", "0"},
                                {"tx", "0"}, {"ty", "0.7"}, {"tz", "-0.5"},
                                {"thickness", "0.05"}, {"swing", "0.4"}});
  KIMIA_REQUIRE(editor.entity("Model_1")->rig.size() == before + 1U);

  // A bone with no name is refused rather than saved unusable.
  KIMIA_REQUIRE(has(ask(editor, "/api/set-bone", {{"name", "Model_1"}, {"bone", ""}}), "\"ok\":false"));
}

KIMIA_TEST(studio_dropping_a_bone_orphans_its_children_rather_than_losing_them) {
  WorldEditor editor;
  streetWorld(editor);
  ask(editor, "/api/bring-in", {{"file", "Tests/assets/crate.obj"}, {"size", "1"}});
  ask(editor, "/api/default-rig", {{"name", "Model_1"}, {"height", "1.7"}});

  KIMIA_REQUIRE(has(ask(editor, "/api/drop-bone", {{"name", "Model_1"}, {"bone", "LeftLeg"}}), "\"ok\":true"));
  // The foot that hung off it is still there, now standing on its own,
  // because silently deleting somebody's work would be worse.
  bool footSurvived = false;
  for (const kimia::RigBone& bone : editor.entity("Model_1")->rig) {
    if (bone.name != "LeftFoot") continue;
    footSurvived = true;
    KIMIA_REQUIRE(bone.parent.empty());
  }
  KIMIA_REQUIRE(footSurvived);

  KIMIA_REQUIRE(has(ask(editor, "/api/drop-bone", {{"name", "Model_1"}, {"bone", "Nope"}}), "\"ok\":false"));
  KIMIA_REQUIRE(has(ask(editor, "/api/clear-rig", {{"name", "Model_1"}}), "\"ok\":true"));
  KIMIA_REQUIRE(editor.entity("Model_1")->rig.empty());
}

KIMIA_TEST(studio_custom_bones_survive_a_save_and_load) {
  WorldEditor editor;
  streetWorld(editor);
  ask(editor, "/api/bring-in", {{"file", "Tests/assets/crate.obj"}, {"size", "1"}});
  ask(editor, "/api/default-rig", {{"name", "Model_1"}, {"height", "1.7"}});
  ask(editor, "/api/set-bone", {{"name", "Model_1"}, {"bone", "Tail"}, {"parent", "Torso"},
                                {"fx", "0.1"}, {"fy", "0.9"}, {"fz", "0.2"},
                                {"tx", "0.3"}, {"ty", "0.7"}, {"tz", "-0.5"},
                                {"thickness", "0.06"}, {"swing", "0.4"}});

  std::string text;
  KIMIA_REQUIRE(kimia::WorldIO::save(editor.world(), text));
  kimia::WorldData reloaded;
  std::string error;
  KIMIA_REQUIRE(kimia::WorldIO::load(text, reloaded, error));

  const kimia::EntityData* back = reloaded.scene.get(reloaded.scene.find("Model_1"));
  KIMIA_REQUIRE(back != nullptr);
  KIMIA_REQUIRE(back->rig.size() == 12U);
  bool tail = false;
  for (const kimia::RigBone& bone : back->rig) {
    if (bone.name != "Tail") continue;
    tail = true;
    KIMIA_REQUIRE(bone.parent == "Torso");
    KIMIA_REQUIRE(near(bone.from.x, 0.1));
    KIMIA_REQUIRE(near(bone.to.z, -0.5));
    KIMIA_REQUIRE(near(bone.thickness, 0.06));
    KIMIA_REQUIRE(near(bone.swing, 0.4));
  }
  KIMIA_REQUIRE(tail);

  // A world with no custom bones still saves exactly as before.
  WorldEditor plain;
  streetWorld(plain);
  std::string plainText;
  kimia::WorldIO::save(plain.world(), plainText);
  KIMIA_REQUIRE(plainText.find(" bone ") == std::string::npos);
}

// --- Visual logic through the Workbench: a game with no code ---

KIMIA_TEST(studio_builds_a_whole_game_out_of_rules) {
  // The point of the whole feature: a person makes a working game by
  // filling in forms, and never writes a line of C++.
  WorldEditor editor;
  streetWorld(editor);

  // WHEN start DO set score 0
  KIMIA_REQUIRE(has(ask(editor, "/api/add-rule", {{"rulename", "setup"}, {"trigger", "start"}}),
                    "\"index\":0"));
  ask(editor, "/api/add-action", {{"index", "0"}, {"act", "set"}, {"target", "score"}, {"number", "0"}});

  // WHEN key space DO add score 1
  ask(editor, "/api/add-rule", {{"rulename", "score"}, {"trigger", "key"}, {"subject", "space"}});
  ask(editor, "/api/add-action", {{"index", "1"}, {"act", "add"}, {"target", "score"}, {"number", "1"}});

  // WHEN every-frame IF score >= 3 DO message, end-game
  ask(editor, "/api/add-rule", {{"rulename", "win"}, {"trigger", "every-frame"}});
  ask(editor, "/api/add-condition", {{"index", "2"}, {"variable", "score"}, {"compare", ">="},
                                     {"number", "3"}});
  ask(editor, "/api/add-action", {{"index", "2"}, {"act", "message"}, {"text", "YOU WIN"}});
  ask(editor, "/api/add-action", {{"index", "2"}, {"act", "end-game"}, {"number", "1"}});

  // The rule list reads back as sentences, which is what the user sees.
  const std::string rules = ask(editor, "/api/rules");
  KIMIA_REQUIRE(has(rules, "WHEN start  DO set score 0"));
  KIMIA_REQUIRE(has(rules, "WHEN key space  DO add score 1"));
  KIMIA_REQUIRE(has(rules, "IF score >= 3"));

  // Now PLAY it. Nothing below touches the rules: it is the engine
  // running the game the user built.
  editor.choose(3);
  editor.update(1.0 / 60.0);
  KIMIA_REQUIRE(editor.logic().numberOf("score") == 0.0);  // the start rule ran

  for (i32 press = 0; press < 3; ++press) {
    editor.setLogicKeys({"space"}, {});
    editor.update(1.0 / 60.0);
  }
  KIMIA_REQUIRE(editor.logic().numberOf("score") == 3.0);
  KIMIA_REQUIRE(editor.logicFinished());
  KIMIA_REQUIRE(editor.logicWon());
  KIMIA_REQUIRE(editor.logicMessage() == "YOU WIN");

  // And the message reaches the HUD, or winning is invisible.
  bool onScreen = false;
  for (const std::string& line : editor.hudLines()) {
    if (line == "YOU WIN") onScreen = true;
  }
  KIMIA_REQUIRE(onScreen);
}

KIMIA_TEST(studio_a_pressed_key_lasts_one_frame_only) {
  // Held-down keys would otherwise count once per frame and a "press to
  // score" rule would rack up hundreds of points from one tap.
  WorldEditor editor;
  streetWorld(editor);
  ask(editor, "/api/add-rule", {{"rulename", "tap"}, {"trigger", "key"}, {"subject", "space"}});
  ask(editor, "/api/add-action", {{"index", "0"}, {"act", "add"}, {"target", "taps"}, {"number", "1"}});
  editor.choose(3);

  editor.setLogicKeys({"space"}, {});
  editor.update(1.0 / 60.0);
  KIMIA_REQUIRE(editor.logic().numberOf("taps") == 1.0);
  // Ten more frames with nobody telling it about a new press.
  for (i32 f = 0; f < 10; ++f) editor.update(1.0 / 60.0);
  KIMIA_REQUIRE(editor.logic().numberOf("taps") == 1.0);
}

KIMIA_TEST(studio_rules_can_be_reordered_disabled_and_dropped) {
  // Order decides which rule wins when two disagree, so the user has to
  // be able to control it.
  WorldEditor editor;
  streetWorld(editor);
  ask(editor, "/api/add-rule", {{"rulename", "first"}, {"trigger", "start"}});
  ask(editor, "/api/add-rule", {{"rulename", "second"}, {"trigger", "start"}});
  KIMIA_REQUIRE(editor.logic().rules[0].name == "first");

  KIMIA_REQUIRE(has(ask(editor, "/api/move-rule", {{"index", "1"}, {"dir", "up"}}), "\"ok\":true"));
  KIMIA_REQUIRE(editor.logic().rules[0].name == "second");
  // The top rule cannot move up, and the bottom one cannot move down.
  KIMIA_REQUIRE(has(ask(editor, "/api/move-rule", {{"index", "0"}, {"dir", "up"}}), "\"ok\":false"));
  KIMIA_REQUIRE(has(ask(editor, "/api/move-rule", {{"index", "1"}, {"dir", "down"}}), "\"ok\":false"));

  // A disabled rule stays in the list but does nothing.
  ask(editor, "/api/add-action", {{"index", "0"}, {"act", "add"}, {"target", "n"}, {"number", "1"}});
  ask(editor, "/api/toggle-rule", {{"index", "0"}, {"on", "0"}});
  KIMIA_REQUIRE(!editor.logic().rules[0].enabled);
  editor.choose(3);
  editor.update(1.0 / 60.0);
  KIMIA_REQUIRE(editor.logic().numberOf("n") == 0.0);

  KIMIA_REQUIRE(has(ask(editor, "/api/drop-rule", {{"index", "0"}}), "\"ok\":true"));
  KIMIA_REQUIRE(editor.logic().rules.size() == 1U);
  KIMIA_REQUIRE(has(ask(editor, "/api/drop-rule", {{"index", "9"}}), "\"ok\":false"));
}

KIMIA_TEST(studio_rules_survive_a_save_and_load) {
  // A game the user built has to still be there next time.
  WorldEditor editor;
  streetWorld(editor);
  ask(editor, "/api/add-rule", {{"rulename", "on goal"}, {"trigger", "event"}, {"subject", "goal"}});
  ask(editor, "/api/add-condition", {{"index", "0"}, {"variable", "lives"}, {"compare", ">"},
                                     {"number", "0"}});
  ask(editor, "/api/add-action", {{"index", "0"}, {"act", "add"}, {"target", "score"}, {"number", "10"}});
  ask(editor, "/api/set-var", {{"variable", "lives"}, {"number", "3"}});

  std::string text;
  KIMIA_REQUIRE(kimia::WorldIO::save(editor.world(), text));
  kimia::WorldData reloaded;
  std::string error;
  KIMIA_REQUIRE(kimia::WorldIO::load(text, reloaded, error));

  KIMIA_REQUIRE(reloaded.logic.rules.size() == 1U);
  const kimia::Rule& rule = reloaded.logic.rules[0];
  KIMIA_REQUIRE(rule.name == "on goal");
  KIMIA_REQUIRE(rule.trigger == kimia::Trigger::Event);
  KIMIA_REQUIRE(rule.subject == "goal");
  KIMIA_REQUIRE(rule.conditions.size() == 1U);
  KIMIA_REQUIRE(rule.conditions[0].variable == "lives");
  KIMIA_REQUIRE(rule.conditions[0].compare == kimia::Compare::Greater);
  KIMIA_REQUIRE(rule.actions.size() == 1U);
  KIMIA_REQUIRE(rule.actions[0].act == kimia::Act::AddVariable);
  KIMIA_REQUIRE(near(rule.actions[0].number, 10.0));
  KIMIA_REQUIRE(near(reloaded.logic.numberOf("lives"), 3.0));

  // A world with no rules still saves exactly as it always did.
  WorldEditor plain;
  streetWorld(plain);
  std::string plainText;
  kimia::WorldIO::save(plain.world(), plainText);
  KIMIA_REQUIRE(plainText.find("# rule ") == std::string::npos);
  KIMIA_REQUIRE(plainText.find("# var ") == std::string::npos);
}

KIMIA_TEST(studio_rule_forms_refuse_nonsense) {
  WorldEditor editor;
  streetWorld(editor);
  // Adding to a rule that does not exist.
  KIMIA_REQUIRE(has(ask(editor, "/api/add-action", {{"index", "5"}, {"act", "add"}}), "\"ok\":false"));
  KIMIA_REQUIRE(has(ask(editor, "/api/add-condition", {{"index", "5"}, {"variable", "x"}}), "\"ok\":false"));
  // A condition with no variable to test.
  ask(editor, "/api/add-rule", {{"rulename", "r"}, {"trigger", "start"}});
  KIMIA_REQUIRE(has(ask(editor, "/api/add-condition", {{"index", "0"}}), "\"ok\":false"));
  // A variable with no name.
  KIMIA_REQUIRE(has(ask(editor, "/api/set-var", {{"number", "1"}}), "\"ok\":false"));
  KIMIA_REQUIRE(has(ask(editor, "/api/drop-var", {{"variable", "ghost"}}), "\"ok\":false"));
  // An unknown trigger falls back rather than being refused, so a newer
  // save opened in an older build still loads.
  KIMIA_REQUIRE(has(ask(editor, "/api/add-rule", {{"rulename", "odd"}, {"trigger", "wat"}}), "\"ok\":true"));
}

// --- Blueprints and stages: the parts a real game is built from ---

KIMIA_TEST(studio_keeps_a_blueprint_with_everything_set_up) {
  // The point of a blueprint: set an object up ONCE, then stamp it twenty
  // times without twenty rounds of the same form-filling.
  WorldEditor editor;
  streetWorld(editor);
  ask(editor, "/api/bring-in", {{"file", "Tests/assets/crate.obj"}, {"size", "1"}});
  ask(editor, "/api/fit-body", {{"name", "Model_1"}, {"kind", "dynamic"}, {"mass", "3"}});
  ask(editor, "/api/label", {{"name", "Model_1"}, {"label", "enemy"}});
  ask(editor, "/api/wire-noise", {{"name", "Model_1"}, {"sound", "kick"}, {"wiring", "k"}});

  KIMIA_REQUIRE(has(ask(editor, "/api/keep", {{"name", "Model_1"}, {"as", "Barrel"}}), "\"ok\":true"));
  KIMIA_REQUIRE(has(ask(editor, "/api/library"), "\"blueprints\":[\"Barrel\"]"));

  // Stamping brings the WHOLE object, not just its shape.
  const std::string stamped = ask(editor, "/api/stamp", {{"blueprint", "Barrel"}, {"x", "4"}, {"z", "2"}});
  KIMIA_REQUIRE(has(stamped, "\"ok\":true"));
  const kimia::EntityData* copy = editor.entity("Barrel");
  KIMIA_REQUIRE(copy != nullptr);
  KIMIA_REQUIRE(copy->meshFile == "Tests/assets/crate.obj");
  KIMIA_REQUIRE(copy->body.has_value());
  KIMIA_REQUIRE(copy->body->kind == kimia::BodyKind::Dynamic);
  KIMIA_REQUIRE(near(copy->body->mass, 3.0));
  KIMIA_REQUIRE(copy->hasTag("enemy"));
  KIMIA_REQUIRE(copy->sounds.size() == 1U);
  KIMIA_REQUIRE(near(copy->transform.position.x, 4.0));

  // And it is SOLID immediately, not after a reload.
  const kimia::usize dynamics = editor.physicsDynamicCount();
  ask(editor, "/api/stamp", {{"blueprint", "Barrel"}, {"x", "-4"}});
  KIMIA_REQUIRE(editor.physicsDynamicCount() == dynamics + 1U);
}

KIMIA_TEST(studio_two_stamps_are_two_objects) {
  // Two copies must be two things the rules can tell apart, or a rule
  // saying "destroy Barrel" would be ambiguous.
  WorldEditor editor;
  streetWorld(editor);
  ask(editor, "/api/bring-in", {{"file", "Tests/assets/crate.obj"}, {"size", "1"}});
  ask(editor, "/api/keep", {{"name", "Model_1"}, {"as", "Barrel"}});

  const std::string first = ask(editor, "/api/stamp", {{"blueprint", "Barrel"}});
  const std::string second = ask(editor, "/api/stamp", {{"blueprint", "Barrel"}});
  KIMIA_REQUIRE(has(first, "\"name\":\"Barrel\""));
  KIMIA_REQUIRE(has(second, "\"name\":\"Barrel_2\""));
  KIMIA_REQUIRE(editor.entity("Barrel") != nullptr);
  KIMIA_REQUIRE(editor.entity("Barrel_2") != nullptr);

  // Keeping under an existing name EDITS that blueprint rather than
  // making a second one you cannot tell apart.
  ask(editor, "/api/keep", {{"name", "Barrel_2"}, {"as", "Barrel"}});
  KIMIA_REQUIRE(has(ask(editor, "/api/library"), "\"blueprints\":[\"Barrel\"]"));

  KIMIA_REQUIRE(has(ask(editor, "/api/forget", {{"blueprint", "Barrel"}}), "\"ok\":true"));
  KIMIA_REQUIRE(has(ask(editor, "/api/stamp", {{"blueprint", "Barrel"}}), "\"ok\":false"));
  KIMIA_REQUIRE(has(ask(editor, "/api/keep", {{"name", "ghost"}, {"as", "X"}}), "\"ok\":false"));
}

KIMIA_TEST(studio_stages_keep_their_own_scenes) {
  // A game is a menu, a level and a victory screen — not one endless
  // field. Switching away must not throw the work away.
  WorldEditor editor;
  streetWorld(editor);
  ask(editor, "/api/bring-in", {{"file", "Tests/assets/crate.obj"}, {"size", "1"}});
  KIMIA_REQUIRE(editor.entity("Model_1") != nullptr);
  KIMIA_REQUIRE(editor.currentStage() == "Main");

  KIMIA_REQUIRE(has(ask(editor, "/api/add-stage", {{"stage", "Level 2"}}), "\"ok\":true"));
  // A stage that already exists is refused rather than silently replacing.
  KIMIA_REQUIRE(has(ask(editor, "/api/add-stage", {{"stage", "Level 2"}}), "\"ok\":false"));
  KIMIA_REQUIRE(has(ask(editor, "/api/add-stage", {{"stage", "Main"}}), "\"ok\":false"));

  // The new stage is its own empty room.
  KIMIA_REQUIRE(has(ask(editor, "/api/go-stage", {{"stage", "Level 2"}}), "\"ok\":true"));
  KIMIA_REQUIRE(editor.currentStage() == "Level 2");
  KIMIA_REQUIRE(editor.entity("Model_1") == nullptr);
  KIMIA_REQUIRE(editor.entity("Ground") != nullptr);  // never opens on nothing

  // Going back brings the first stage's work back untouched.
  KIMIA_REQUIRE(has(ask(editor, "/api/go-stage", {{"stage", "Main"}}), "\"ok\":true"));
  KIMIA_REQUIRE(editor.entity("Model_1") != nullptr);

  // Work done AFTER a stage has been visited once must also survive.
  // Testing only the first switch missed this: the first time you leave a
  // stage it is filed away for the first time, and a later departure takes
  // a different path through the code. Editing on the second visit and
  // switching again is what actually exercises it.
  ask(editor, "/api/bring-in", {{"file", "Tests/assets/crate.obj"}, {"size", "1"}});
  KIMIA_REQUIRE(editor.entity("Model_2") != nullptr);
  ask(editor, "/api/go-stage", {{"stage", "Level 2"}});
  KIMIA_REQUIRE(editor.entity("Model_2") == nullptr);
  ask(editor, "/api/go-stage", {{"stage", "Main"}});
  KIMIA_REQUIRE(editor.entity("Model_1") != nullptr);
  KIMIA_REQUIRE(editor.entity("Model_2") != nullptr);

  // And the other stage keeps ITS own later work too.
  ask(editor, "/api/go-stage", {{"stage", "Level 2"}});
  ask(editor, "/api/bring-in", {{"file", "Tests/assets/crate.obj"}, {"size", "1"}});
  const std::string onLevelTwo = "Model_1";
  KIMIA_REQUIRE(editor.entity(onLevelTwo) != nullptr);
  ask(editor, "/api/go-stage", {{"stage", "Main"}});
  ask(editor, "/api/go-stage", {{"stage", "Level 2"}});
  KIMIA_REQUIRE(editor.entity(onLevelTwo) != nullptr);

  // Back to Main for the deletion checks below.
  ask(editor, "/api/go-stage", {{"stage", "Main"}});
  KIMIA_REQUIRE(editor.currentStage() == "Main");

  // The stage you are standing on cannot be deleted.
  KIMIA_REQUIRE(has(ask(editor, "/api/drop-stage", {{"stage", "Main"}}), "\"ok\":false"));
  KIMIA_REQUIRE(has(ask(editor, "/api/drop-stage", {{"stage", "Level 2"}}), "\"ok\":true"));
  KIMIA_REQUIRE(has(ask(editor, "/api/go-stage", {{"stage", "Level 2"}}), "\"ok\":false"));
}

KIMIA_TEST(studio_a_rule_can_send_the_player_to_another_stage) {
  // Several stages are only worth having if the game can move between
  // them, so the "scene" action has to really switch.
  WorldEditor editor;
  streetWorld(editor);
  ask(editor, "/api/add-stage", {{"stage", "Level 2"}});
  ask(editor, "/api/add-rule", {{"rulename", "next level"}, {"trigger", "key"}, {"subject", "n"}});
  ask(editor, "/api/add-action", {{"index", "0"}, {"act", "scene"}, {"text", "Level 2"}});

  editor.choose(3);  // PLAY
  KIMIA_REQUIRE(editor.currentStage() == "Main");
  editor.setLogicKeys({"n"}, {});
  editor.update(1.0 / 60.0);
  KIMIA_REQUIRE(editor.currentStage() == "Level 2");
}

// --- The game's own interface ---

KIMIA_TEST(studio_lays_out_a_hud_that_shows_the_game) {
  WorldEditor editor;
  streetWorld(editor);
  // A score label and a health bar, placed by fractions of the screen.
  KIMIA_REQUIRE(has(ask(editor, "/api/set-panel", {{"panel", "score"}, {"kind", "label"},
                                                   {"text", "Score: {score}"}, {"x", "0.02"},
                                                   {"y", "0.02"}}),
                    "\"ok\":true"));
  KIMIA_REQUIRE(has(ask(editor, "/api/set-panel", {{"panel", "health"}, {"kind", "bar"},
                                                   {"variable", "lives"}, {"maximum", "3"}}),
                    "\"ok\":true"));

  const std::string panels = ask(editor, "/api/panels");
  KIMIA_REQUIRE(has(panels, "\"name\":\"score\""));
  KIMIA_REQUIRE(has(panels, "Score: {score}"));
  KIMIA_REQUIRE(has(panels, "\"kind\":\"bar\""));
  KIMIA_REQUIRE(has(panels, "\"variable\":\"lives\""));

  // Moving a panel is a repeat call, not a second panel.
  ask(editor, "/api/set-panel", {{"panel", "score"}, {"kind", "label"}, {"x", "0.5"}});
  KIMIA_REQUIRE(editor.hud().panels.size() == 2U);
  KIMIA_REQUIRE(near(editor.hud().find("score")->x, 0.5));

  KIMIA_REQUIRE(has(ask(editor, "/api/set-panel", {{"kind", "label"}}), "\"ok\":false"));
  KIMIA_REQUIRE(has(ask(editor, "/api/drop-panel", {{"panel", "score"}}), "\"ok\":true"));
  KIMIA_REQUIRE(has(ask(editor, "/api/drop-panel", {{"panel", "score"}}), "\"ok\":false"));
}

KIMIA_TEST(studio_a_hud_button_drives_the_rules) {
  // The whole chain with no code: draw a button, wire a rule to its
  // event, press it, and watch the game change.
  WorldEditor editor;
  streetWorld(editor);
  ask(editor, "/api/set-panel", {{"panel", "give"}, {"kind", "button"}, {"text", "+10"},
                                 {"event", "bonus"}, {"x", "0.3"}, {"y", "0.4"},
                                 {"w", "0.4"}, {"h", "0.2"}});
  ask(editor, "/api/add-rule", {{"rulename", "bonus"}, {"trigger", "event"}, {"subject", "bonus"}});
  ask(editor, "/api/add-action", {{"index", "0"}, {"act", "add"}, {"target", "score"}, {"number", "10"}});

  editor.choose(3);  // PLAY
  editor.update(1.0 / 60.0);
  KIMIA_REQUIRE(editor.logic().numberOf("score") == 0.0);

  KIMIA_REQUIRE(has(ask(editor, "/api/press", {{"panel", "give"}}), "\"ok\":true"));
  editor.update(1.0 / 60.0);
  KIMIA_REQUIRE(editor.logic().numberOf("score") == 10.0);

  // Pressing again adds again — the event is not a one-off.
  ask(editor, "/api/press", {{"panel", "give"}});
  editor.update(1.0 / 60.0);
  KIMIA_REQUIRE(editor.logic().numberOf("score") == 20.0);

  // A label is not a button, however much it looks like one.
  ask(editor, "/api/set-panel", {{"panel", "title"}, {"kind", "label"}, {"text", "hi"}});
  KIMIA_REQUIRE(has(ask(editor, "/api/press", {{"panel", "title"}}), "\"ok\":false"));
  KIMIA_REQUIRE(has(ask(editor, "/api/press", {{"panel", "ghost"}}), "\"ok\":false"));
}

KIMIA_TEST(studio_the_hud_layout_survives_a_save_and_load) {
  WorldEditor editor;
  streetWorld(editor);
  ask(editor, "/api/set-panel", {{"panel", "score"}, {"kind", "label"}, {"text", "Score: {score}"},
                                 {"x", "0.1"}, {"y", "0.2"}, {"w", "0.4"}, {"h", "0.09"},
                                 {"r", "1"}, {"g", "0.5"}, {"b", "0"}, {"scale", "3"}});
  ask(editor, "/api/set-panel", {{"panel", "go"}, {"kind", "button"}, {"event", "start"},
                                 {"text", "PLAY"}});

  std::string text;
  KIMIA_REQUIRE(kimia::WorldIO::save(editor.world(), text));
  kimia::WorldData reloaded;
  std::string error;
  KIMIA_REQUIRE(kimia::WorldIO::load(text, reloaded, error));

  KIMIA_REQUIRE(reloaded.hud.panels.size() == 2U);
  const kimia::Panel* score = reloaded.hud.find("score");
  KIMIA_REQUIRE(score != nullptr);
  KIMIA_REQUIRE(score->kind == kimia::PanelKind::Label);
  // Text with a space AND braces has to survive intact.
  KIMIA_REQUIRE(score->text == "Score: {score}");
  KIMIA_REQUIRE(near(score->x, 0.1));
  KIMIA_REQUIRE(near(score->height, 0.09));
  KIMIA_REQUIRE(near(score->color.x, 1.0));
  KIMIA_REQUIRE(score->scale == 3);
  const kimia::Panel* go = reloaded.hud.find("go");
  KIMIA_REQUIRE(go != nullptr);
  KIMIA_REQUIRE(go->kind == kimia::PanelKind::Button);
  KIMIA_REQUIRE(go->event == "start");

  // A world with no panels still saves exactly as it always did.
  WorldEditor plain;
  streetWorld(plain);
  std::string plainText;
  kimia::WorldIO::save(plain.world(), plainText);
  KIMIA_REQUIRE(plainText.find("# panel ") == std::string::npos);
}

// --- Publishing: handing the game to somebody else ---

KIMIA_TEST(studio_publishes_a_folder_that_can_be_played) {
  // Everything a person needs to run the game, and nothing they need to
  // understand about the engine.
  WorldEditor editor;
  streetWorld(editor);
  ask(editor, "/api/add-rule", {{"rulename", "score"}, {"trigger", "key"}, {"subject", "space"}});
  ask(editor, "/api/add-action", {{"index", "0"}, {"act", "add"}, {"target", "score"}, {"number", "1"}});
  ask(editor, "/api/set-panel", {{"panel", "hud"}, {"kind", "label"}, {"text", "SCORE {score}"}});

  const std::string folder = "/tmp/kimia_publish_test";
  std::error_code ignored;
  std::filesystem::remove_all(folder, ignored);
  KIMIA_REQUIRE(has(ask(editor, "/api/publish", {{"folder", folder}}), "\"ok\":true"));

  // The game itself, plus a way to start it without knowing any options.
  KIMIA_REQUIRE(std::filesystem::exists(folder + "/game.kimia"));
  KIMIA_REQUIRE(std::filesystem::exists(folder + "/play.sh"));
  KIMIA_REQUIRE(std::filesystem::exists(folder + "/README.txt"));

  // The world file carries the WHOLE game — rules and screen included —
  // so a published game is one file plus a runner.
  std::ifstream saved(folder + "/game.kimia", std::ios::binary);
  const std::string body((std::istreambuf_iterator<char>(saved)), std::istreambuf_iterator<char>());
  KIMIA_REQUIRE(body.find("# rule ") != std::string::npos);
  KIMIA_REQUIRE(body.find("# panel ") != std::string::npos);

  // The runner starts it in play mode, not in the editor.
  std::ifstream script(folder + "/play.sh", std::ios::binary);
  const std::string runner((std::istreambuf_iterator<char>(script)), std::istreambuf_iterator<char>());
  KIMIA_REQUIRE(runner.find("--play game.kimia") != std::string::npos);

  std::filesystem::remove_all(folder, ignored);
}

KIMIA_TEST(studio_a_published_game_opens_in_play_with_no_editor) {
  // The person you gave the game to is a PLAYER. Every editor control on
  // screen is a way for them to break what you made.
  WorldEditor editor;
  streetWorld(editor);
  ask(editor, "/api/add-rule", {{"rulename", "score"}, {"trigger", "key"}, {"subject", "space"}});
  ask(editor, "/api/add-action", {{"index", "0"}, {"act", "add"}, {"target", "score"}, {"number", "1"}});

  const std::string folder = "/tmp/kimia_publish_play";
  std::error_code ignored;
  std::filesystem::remove_all(folder, ignored);
  ask(editor, "/api/publish", {{"folder", folder}});

  // Open it the way the runner does.
  WorldEditor game;
  std::string error;
  KIMIA_REQUIRE(game.startPublished(folder + "/game.kimia", error));
  KIMIA_REQUIRE(game.playOnly());
  // Straight into the game, not onto a menu.
  KIMIA_REQUIRE(game.playing());

  // No route back into the builder anywhere on the pads.
  for (const auto& pad : game.tapPad()) {
    KIMIA_REQUIRE(pad.second != "b");
  }
  // And the rules the author wrote really came along.
  game.setLogicKeys({"space"}, {});
  game.update(1.0 / 60.0);
  KIMIA_REQUIRE(game.logic().numberOf("score") == 1.0);

  // A missing file is refused with a reason rather than opening blank.
  WorldEditor missing;
  std::string why;
  KIMIA_REQUIRE(!missing.startPublished(folder + "/nope.kimia", why));
  KIMIA_REQUIRE(!why.empty());
  KIMIA_REQUIRE(!missing.playOnly());

  std::filesystem::remove_all(folder, ignored);
}

// --- Files, controls and textures, through the Workbench ---

KIMIA_TEST(studio_scans_the_asset_folder_and_finds_clips) {
  WorldEditor editor;
  streetWorld(editor);
  editor.setImportDirectory("Tests/assets");

  // A plain listing is the fast path and opens nothing.
  const std::string shallow = ask(editor, "/api/assets");
  KIMIA_REQUIRE(has(shallow, "\"deep\":false"));
  KIMIA_REQUIRE(has(shallow, "skinned_bar.fbx"));
  KIMIA_REQUIRE(has(shallow, "\"kind\":\"texture\""));

  // A deep scan looks inside, which is what lets a person PICK a clip
  // from a list instead of typing its name from memory.
  const std::string deep = ask(editor, "/api/assets", {{"deep", "1"}});
  KIMIA_REQUIRE(has(deep, "\"deep\":true"));
  KIMIA_REQUIRE(has(deep, "\"clips\":[\"Bend\"]"));
  KIMIA_REQUIRE(has(deep, "\"bones\":2"));
}

KIMIA_TEST(studio_paints_an_image_from_the_file_list_onto_an_object) {
  WorldEditor editor;
  streetWorld(editor);
  ask(editor, "/api/bring-in", {{"file", "Tests/assets/crate.obj"}, {"size", "1"}});
  KIMIA_REQUIRE(editor.entity("Model_1")->texture.empty());

  KIMIA_REQUIRE(has(ask(editor, "/api/skin", {{"name", "Model_1"},
                                              {"image", "Tests/assets/crate_skin.png"}}),
                    "\"ok\":true"));
  KIMIA_REQUIRE(editor.entity("Model_1")->texture == "Tests/assets/crate_skin.png");

  // A file that is not an image is refused, rather than leaving a blank
  // object and a person wondering why.
  KIMIA_REQUIRE(has(ask(editor, "/api/skin", {{"name", "Model_1"},
                                              {"image", "Tests/assets/crate.obj"}}),
                    "\"ok\":false"));
  // The good texture survived the failed attempt.
  KIMIA_REQUIRE(editor.entity("Model_1")->texture == "Tests/assets/crate_skin.png");

  // An empty image means "take it off".
  KIMIA_REQUIRE(has(ask(editor, "/api/skin", {{"name", "Model_1"}}), "\"ok\":true"));
  KIMIA_REQUIRE(editor.entity("Model_1")->texture.empty());
}

KIMIA_TEST(studio_one_control_serves_key_screen_and_gamepad) {
  // The player might tap glass, press a key, or push a pad button. The
  // game says "jump" once and the engine sorts out the hardware.
  WorldEditor editor;
  streetWorld(editor);
  KIMIA_REQUIRE(has(ask(editor, "/api/set-control",
                        {{"control", "jump"}, {"label", "JUMP"}, {"key", "space"},
                         {"pad", "a"}, {"touch", "1"},
                         {"clipfile", "Tests/assets/skinned_bar.fbx"}, {"clip", "Bend"}}),
                    "\"ok\":true"));

  KIMIA_REQUIRE(editor.actionFromControl(kimia::Source::Key, "space") == "jump");
  KIMIA_REQUIRE(editor.actionFromControl(kimia::Source::Pad, "a") == "jump");
  KIMIA_REQUIRE(editor.actionFromControl(kimia::Source::Touch, "jump") == "jump");
  KIMIA_REQUIRE(editor.actionFromControl(kimia::Source::Key, "z").empty());

  // Firing it plays the clip the user chose from the scanned FBX. A clip
  // picked this way has no animation component behind it, so it needs a
  // path of its own — without which choosing a clip for a button did
  // nothing at all.
  KIMIA_REQUIRE(editor.playingAnimations().empty());
  KIMIA_REQUIRE(has(ask(editor, "/api/do", {{"control", "jump"}}), "\"ok\":true"));
  const std::vector<std::string> playing = editor.playingAnimations();
  KIMIA_REQUIRE(playing.size() == 1U);
  KIMIA_REQUIRE(playing[0] == "Tests/assets/skinned_bar.fbx:Bend");

  // Pressing twice replays rather than stacking two copies.
  ask(editor, "/api/do", {{"control", "jump"}});
  KIMIA_REQUIRE(editor.playingAnimations().size() == 1U);

  KIMIA_REQUIRE(has(ask(editor, "/api/do", {{"control", "ghost"}}), "\"ok\":false"));
  KIMIA_REQUIRE(has(ask(editor, "/api/set-control", {{"key", "x"}}), "\"ok\":false"));
}

KIMIA_TEST(studio_a_control_is_an_event_the_rules_can_use) {
  // A button has to work before anyone attaches a clip to it, or the
  // input system would only be useful to people who have models.
  WorldEditor editor;
  streetWorld(editor);
  ask(editor, "/api/set-control", {{"control", "fire"}, {"key", "f"}, {"touch", "1"}});
  ask(editor, "/api/add-rule", {{"rulename", "shoot"}, {"trigger", "event"}, {"subject", "fire"}});
  ask(editor, "/api/add-action", {{"index", "0"}, {"act", "add"}, {"target", "shots"}, {"number", "1"}});

  editor.choose(3);  // PLAY
  editor.update(1.0 / 60.0);
  KIMIA_REQUIRE(editor.logic().numberOf("shots") == 0.0);

  ask(editor, "/api/do", {{"control", "fire"}});
  editor.update(1.0 / 60.0);
  KIMIA_REQUIRE(editor.logic().numberOf("shots") == 1.0);
}

KIMIA_TEST(studio_controls_survive_a_save_and_load) {
  WorldEditor editor;
  streetWorld(editor);
  ask(editor, "/api/set-control", {{"control", "jump"}, {"label", "JUMP UP"}, {"key", "space"},
                                   {"pad", "a"}, {"touch", "1"}, {"x", "0.7"}, {"y", "0.8"},
                                   {"clipfile", "assets/hero.fbx"}, {"clip", "Leap"},
                                   {"sound", "boing"}});
  ask(editor, "/api/stick", {{"on", "1"}});

  std::string text;
  KIMIA_REQUIRE(kimia::WorldIO::save(editor.world(), text));
  kimia::WorldData reloaded;
  std::string error;
  KIMIA_REQUIRE(kimia::WorldIO::load(text, reloaded, error));

  KIMIA_REQUIRE(reloaded.input.showStick);
  KIMIA_REQUIRE(reloaded.input.controls.size() == 1U);
  const kimia::Control& back = reloaded.input.controls[0];
  KIMIA_REQUIRE(back.name == "jump");
  // A label with a space in it has to come back whole.
  KIMIA_REQUIRE(back.spot.label == "JUMP UP");
  KIMIA_REQUIRE(near(back.spot.x, 0.7));
  KIMIA_REQUIRE(back.clipFile == "assets/hero.fbx");
  KIMIA_REQUIRE(back.clip == "Leap");
  KIMIA_REQUIRE(back.sound == "boing");
  KIMIA_REQUIRE(back.bindings.size() == 3U);
  KIMIA_REQUIRE(reloaded.input.actionFor(kimia::Source::Pad, "a") == "jump");

  // A world with no controls still saves exactly as it always did.
  WorldEditor plain;
  streetWorld(plain);
  std::string plainText;
  kimia::WorldIO::save(plain.world(), plainText);
  KIMIA_REQUIRE(plainText.find("# control ") == std::string::npos);
  KIMIA_REQUIRE(plainText.find("# stick ") == std::string::npos);
}

// --- Unity-style editing: Hierarchy verbs, Inspector rotation, transport ---

KIMIA_TEST(studio_object_create_duplicate_rename) {
  WorldEditor editor;
  streetWorld(editor);
  const std::string made = ask(editor, "/api/object/create", {{"kind", "cube"}});
  KIMIA_REQUIRE(has(made, "\"ok\":true"));
  KIMIA_REQUIRE(has(made, "\"name\":\"Cube\""));
  // Unknown kinds and missing worlds make nothing.
  KIMIA_REQUIRE(has(ask(editor, "/api/object/create", {{"kind", "dragon"}}), "\"ok\":false"));
  WorldEditor bare;
  KIMIA_REQUIRE(has(ask(bare, "/api/object/create", {{"kind", "cube"}}), "\"ok\":false"));

  const std::string copied = ask(editor, "/api/object/duplicate", {{"name", "Cube"}});
  KIMIA_REQUIRE(has(copied, "\"name\":\"Cube_2\""));
  KIMIA_REQUIRE(has(ask(editor, "/api/object/duplicate", {{"name", "ghost"}}), "\"ok\":false"));

  KIMIA_REQUIRE(has(ask(editor, "/api/object/rename", {{"name", "Cube_2"}, {"to", "Tower"}}),
                        "\"ok\":true"));
  KIMIA_REQUIRE(has(ask(editor, "/api/rack"), "\"name\":\"Tower\""));
  // Taken names and the structural three are refused, not forced.
  KIMIA_REQUIRE(has(ask(editor, "/api/object/rename", {{"name", "Tower"}, {"to", "Cube"}}),
                        "\"ok\":false"));
  KIMIA_REQUIRE(has(ask(editor, "/api/object/rename", {{"name", "Player"}, {"to", "Hero"}}),
                        "\"ok\":false"));
}

KIMIA_TEST(studio_object_rotate_euler_scale) {
  WorldEditor editor;
  streetWorld(editor);
  ask(editor, "/api/object/create", {{"kind", "block"}});
  // A quarter turn reads back 90 degrees, in route and dossier alike.
  const std::string turned = ask(editor, "/api/object/rotate", {{"name", "Block_1"}, {"dyaw", "90"}});
  KIMIA_REQUIRE(has(turned, "\"ok\":true"));
  KIMIA_REQUIRE(has(turned, "90.000000"));
  KIMIA_REQUIRE(has(ask(editor, "/api/dossier", {{"name", "Block_1"}}), "90.000000"));
  KIMIA_REQUIRE(has(ask(editor, "/api/object/rotate", {{"name", "ghost"}, {"dyaw", "90"}}),
                        "\"ok\":false"));
  // Writing degrees back turns the model the same way.
  KIMIA_REQUIRE(has(ask(editor, "/api/object/euler",
                            {{"name", "Block_1"}, {"x", "10"}, {"y", "20"}, {"z", "30"}}),
                        "\"ok\":true"));
  const std::string sheet = ask(editor, "/api/dossier", {{"name", "Block_1"}});
  KIMIA_REQUIRE(has(sheet, "10.000000"));
  KIMIA_REQUIRE(has(sheet, "20.000000"));
  KIMIA_REQUIRE(has(sheet, "30.000000"));
  // Scaling doubles and reports the new size; cups refuse, ghosts fail.
  const std::string grown = ask(editor, "/api/object/scale", {{"name", "Block_1"}, {"factor", "2"}});
  KIMIA_REQUIRE(has(grown, "2.000000"));
  ask(editor, "/api/object/create", {{"kind", "hole"}});
  KIMIA_REQUIRE(has(ask(editor, "/api/object/scale", {{"name", "Hole_1"}, {"factor", "2"}}),
                        "\"ok\":false"));
  KIMIA_REQUIRE(has(ask(editor, "/api/object/scale", {{"name", "ghost"}, {"factor", "2"}}),
                        "\"ok\":false"));
}

KIMIA_TEST(studio_object_clips_play_and_stop) {
  WorldEditor editor;
  streetWorld(editor);
  std::string error;
  const std::string name = editor.importModel("Tests/assets/skinned_bar.fbx", 1.0, error);
  KIMIA_REQUIRE(!name.empty());
  // The Inspector's Animation section: the file's own clips.
  const std::string clips = ask(editor, "/api/object/clips", {{"name", name}});
  KIMIA_REQUIRE(has(clips, "\"skeleton\":true"));
  KIMIA_REQUIRE(has(clips, "\"Bend\""));
  ask(editor, "/api/object/create", {{"kind", "cube"}});
  const std::string plain = ask(editor, "/api/object/clips", {{"name", "Cube"}});
  KIMIA_REQUIRE(has(plain, "\"skeleton\":false"));
  KIMIA_REQUIRE(has(plain, "\"clips\":[]"));
  // Playing shows up in the pulse; stopping clears it.
  KIMIA_REQUIRE(has(ask(editor, "/api/object/play-clip", {{"name", name}, {"clip", "Bend"}}),
                        "\"ok\":true"));
  KIMIA_REQUIRE(has(ask(editor, "/api/pulse"), name + ":Bend"));
  KIMIA_REQUIRE(has(ask(editor, "/api/object/stop-clips", {{"name", name}}), "\"stopped\":true"));
  KIMIA_REQUIRE(has(ask(editor, "/api/object/stop-clips", {{"name", name}}), "\"stopped\":false"));
  // A prop with no model file has nothing to play.
  KIMIA_REQUIRE(has(ask(editor, "/api/object/play-clip", {{"name", "Cube"}, {"clip", "Bend"}}),
                        "\"ok\":false"));
}

KIMIA_TEST(studio_transport_play_pause_step) {
  WorldEditor bare;
  KIMIA_REQUIRE(has(ask(bare, "/api/transport/play"), "\"ok\":false"));  // no world, no game
  WorldEditor editor;
  streetWorld(editor);
  KIMIA_REQUIRE(has(ask(editor, "/api/transport/play"), "\"playing\":true"));
  const std::string running = ask(editor, "/api/transport/state");
  KIMIA_REQUIRE(has(running, "\"playing\":true"));
  KIMIA_REQUIRE(has(running, "\"paused\":false"));
  KIMIA_REQUIRE(has(ask(editor, "/api/transport/pause", {{"paused", "1"}}), "\"paused\":true"));
  KIMIA_REQUIRE(has(ask(editor, "/api/pulse"), "\"paused\":true"));
  KIMIA_REQUIRE(has(ask(editor, "/api/transport/step"), "\"ok\":true"));
  KIMIA_REQUIRE(has(ask(editor, "/api/transport/pause", {{"paused", "0"}}), "\"paused\":false"));
}

KIMIA_TEST(studio_editor_page_uses_unity_layout) {
  // Hierarchy left, Inspector middle, Game right, transport on top,
  // Project and Console below — in Unity terms, all wired to /api/.
  const std::string page = kimia::studio::benchPage();
  KIMIA_REQUIRE(has(page, "Hierarchy"));
  KIMIA_REQUIRE(has(page, "Inspector"));
  KIMIA_REQUIRE(has(page, "Console"));
  KIMIA_REQUIRE(has(page, "Project"));
  KIMIA_REQUIRE(has(page, "object/create"));
  KIMIA_REQUIRE(has(page, "object/duplicate"));
  KIMIA_REQUIRE(has(page, "transport/play"));
  KIMIA_REQUIRE(has(page, "transport/pause"));
  KIMIA_REQUIRE(has(page, "transport/step"));
  KIMIA_REQUIRE(has(page, "setTool('rotate')"));
  KIMIA_REQUIRE(has(page, "setTool('scale')"));
}

KIMIA_TEST(studio_asset_rename_and_delete) {
  // The Project panel's file manager, fenced into a sandbox folder.
  namespace fs = std::filesystem;
  const fs::path sandbox = fs::temp_directory_path() / "kimia_asset_files";
  fs::remove_all(sandbox);
  fs::create_directories(sandbox);
  { std::ofstream out(sandbox / "a.obj"); out << "v 0 0 0\n"; }
  { std::ofstream out(sandbox / "c.obj"); out << "v 1 1 1\n"; }

  WorldEditor editor;
  streetWorld(editor);
  editor.setImportDirectory(sandbox.string());

  KIMIA_REQUIRE(has(ask(editor, "/api/asset/rename", {{"file", "a.obj"}, {"to", "b.obj"}}),
                        "\"ok\":true"));
  KIMIA_REQUIRE(!fs::exists(sandbox / "a.obj"));
  KIMIA_REQUIRE(fs::exists(sandbox / "b.obj"));
  KIMIA_REQUIRE(has(ask(editor, "/api/assets"), "b.obj"));
  // Refusals: taken names, missing files, and anything with a folder in it.
  KIMIA_REQUIRE(has(ask(editor, "/api/asset/rename", {{"file", "b.obj"}, {"to", "c.obj"}}),
                        "\"ok\":false"));
  KIMIA_REQUIRE(has(ask(editor, "/api/asset/rename", {{"file", "ghost.obj"}, {"to", "x.obj"}}),
                        "\"ok\":false"));
  KIMIA_REQUIRE(has(ask(editor, "/api/asset/rename", {{"file", "../a.obj"}, {"to", "x.obj"}}),
                        "\"ok\":false"));
  KIMIA_REQUIRE(has(ask(editor, "/api/asset/rename", {{"file", "b.obj"}, {"to", "sub/x.obj"}}),
                        "\"ok\":false"));
  KIMIA_REQUIRE(has(ask(editor, "/api/asset/rename", {{"file", "b.obj"}, {"to", ".."}}),
                        "\"ok\":false"));

  KIMIA_REQUIRE(has(ask(editor, "/api/asset/delete", {{"file", "b.obj"}}), "\"ok\":true"));
  KIMIA_REQUIRE(!fs::exists(sandbox / "b.obj"));
  KIMIA_REQUIRE(has(ask(editor, "/api/asset/delete", {{"file", "b.obj"}}), "\"ok\":false"));
  KIMIA_REQUIRE(has(ask(editor, "/api/asset/delete", {{"file", "../c.obj"}}), "\"ok\":false"));
  KIMIA_REQUIRE(fs::exists(sandbox / "c.obj"));  // the traversal touched nothing
  fs::remove_all(sandbox);
}

KIMIA_TEST(studio_asset_upload_saves_bytes) {
  // What the Upload button posts: raw bytes and a bare name.
  namespace fs = std::filesystem;
  const fs::path sandbox = fs::temp_directory_path() / "kimia_asset_upload";
  fs::remove_all(sandbox);
  fs::create_directories(sandbox);

  WorldEditor editor;
  streetWorld(editor);
  editor.setImportDirectory(sandbox.string());

  Params query;
  query["name"] = "up.obj";
  const std::string done = kimia::studio::saveAssetFile(editor, query, "v 1 2 3\n");
  KIMIA_REQUIRE(has(done, "\"file\":\"up.obj\""));
  std::string back;
  { std::ifstream in(sandbox / "up.obj"); std::getline(in, back); }
  KIMIA_REQUIRE(back == "v 1 2 3");
  // Never overwrite, never escape the folder.
  KIMIA_REQUIRE(has(kimia::studio::saveAssetFile(editor, query, "other"), "\"ok\":false"));
  query["name"] = "sub/evil.obj";
  KIMIA_REQUIRE(has(kimia::studio::saveAssetFile(editor, query, "x"), "\"ok\":false"));
  query["name"] = "..";
  KIMIA_REQUIRE(has(kimia::studio::saveAssetFile(editor, query, "x"), "\"ok\":false"));
  query["name"] = "";
  KIMIA_REQUIRE(has(kimia::studio::saveAssetFile(editor, query, "x"), "\"ok\":false"));
  KIMIA_REQUIRE(!fs::exists(sandbox / "sub"));
  fs::remove_all(sandbox);
}

KIMIA_TEST(studio_project_panel_manages_files) {
  // Every file row opens, renames and deletes; the column uploads.
  const std::string page = kimia::studio::benchPage();
  KIMIA_REQUIRE(has(page, "asset/rename"));
  KIMIA_REQUIRE(has(page, "asset/delete"));
  KIMIA_REQUIRE(has(page, "asset/upload"));
  KIMIA_REQUIRE(has(page, "uploadAsset()"));
  KIMIA_REQUIRE(has(page, "renameAsset("));
  KIMIA_REQUIRE(has(page, "deleteAsset("));
}

KIMIA_TEST(studio_asset_scan_sees_subfolders) {
  // An animation pack arrives as actions/, dances/, ... — the scan must
  // see all of it, with names relative to the scanned root.
  namespace fs = std::filesystem;
  const fs::path sandbox = fs::temp_directory_path() / "kimia_asset_tree";
  fs::remove_all(sandbox);
  fs::create_directories(sandbox / "actions");
  { std::ofstream out(sandbox / "actions" / "Kick.fbx"); out << "fbx-ish\n"; }
  { std::ofstream out(sandbox / "top.obj"); out << "v 0 0 0\n"; }

  WorldEditor editor;
  streetWorld(editor);
  editor.setImportDirectory(sandbox.string());
  const std::string listed = ask(editor, "/api/assets");
  KIMIA_REQUIRE(has(listed, "\"file\":\"actions/Kick.fbx\""));
  KIMIA_REQUIRE(has(listed, "\"file\":\"top.obj\""));

  // Rename inside the subfolder; `to` never moves between folders.
  KIMIA_REQUIRE(has(ask(editor, "/api/asset/rename", {{"file", "actions/Kick.fbx"}, {"to", "Pass.fbx"}}),
                        "\"ok\":true"));
  KIMIA_REQUIRE(fs::exists(sandbox / "actions" / "Pass.fbx"));
  KIMIA_REQUIRE(has(ask(editor, "/api/asset/rename",
                            {{"file", "actions/Pass.fbx"}, {"to", "../top.obj"}}),
                        "\"ok\":false"));
  // And no ".." segment escapes, however deep it hides.
  KIMIA_REQUIRE(has(ask(editor, "/api/asset/delete", {{"file", "actions/../../top.obj"}}),
                        "\"ok\":false"));
  KIMIA_REQUIRE(fs::exists(sandbox / "top.obj"));
  KIMIA_REQUIRE(has(ask(editor, "/api/asset/delete", {{"file", "actions/Pass.fbx"}}), "\"ok\":true"));
  KIMIA_REQUIRE(!fs::exists(sandbox / "actions" / "Pass.fbx"));
  fs::remove_all(sandbox);
}

KIMIA_TEST(studio_asset_scan_lists_the_animation_pack) {
  WorldEditor editor;
  streetWorld(editor);
  const std::string list =
      ask(editor, "/api/assets", {{"folder", "assets/animations/reactions"}, {"deep", "1"}});
  if (!has(list, "Victory.fbx")) {
    std::printf("SKIP: assets/animations not next to the test runner\n");
    return;
  }
  KIMIA_REQUIRE(has(list, "Defeated.fbx"));
  KIMIA_REQUIRE(has(list, "\"deep\":true"));
  KIMIA_REQUIRE(has(list, "\"clips\":[\"Victory\"]"));
  KIMIA_REQUIRE(has(list, "\"bones\":69"));
  KIMIA_REQUIRE(has(list, "\"skeleton\":true"));
}

KIMIA_TEST(studio_keeps_real_asset_paths_relative_and_plays_a_space_named_clip) {
  WorldEditor editor;
  streetWorld(editor);
  editor.setImportDirectory("assets");

  // This test exists to prove the space-named clip path end to end; without
  // the (removed) animation pack there is nothing to exercise, so it skips
  // like the other tracked-animation tests.
  std::string probe;
  if (!kimia::assets::loadFBXSkinned("assets/animations/pleyer move/walk.fbx", probe).has_value()) {
    std::printf("SKIP: assets/animations not next to the test runner\n");
    return;
  }

  const std::string listing = ask(editor, "/api/assets");
  KIMIA_REQUIRE(has(listing, "street/kids/kid_ali.obj"));
  KIMIA_REQUIRE(has(listing, "animations/pleyer move/walk.fbx"));
  KIMIA_REQUIRE(listing.find("\"path\":\"/") == std::string::npos);

  std::string error;
  const std::string prop = editor.importModel("street/kids/kid_ali.obj", 1.0, error);
  KIMIA_REQUIRE(!prop.empty());
  KIMIA_REQUIRE(error.empty());
  KIMIA_REQUIRE(editor.entity(prop)->meshFile == "street/kids/kid_ali.obj");
  KIMIA_REQUIRE(editor.assetPath(editor.entity(prop)->meshFile) == "assets/street/kids/kid_ali.obj");
  KIMIA_REQUIRE(editor.assetFor(editor.entity(prop)->meshFile) != nullptr);
  KIMIA_REQUIRE(has(ask(editor, "/api/dossier", {{"name", prop}}), "\"span\":"));

  const std::string animation = editor.importModel("animations/pleyer move/walk.fbx", 1.0, error);
  KIMIA_REQUIRE(!animation.empty());
  KIMIA_REQUIRE(error.empty());
  KIMIA_REQUIRE(editor.entity(animation)->meshFile == "animations/pleyer move/walk.fbx");
  KIMIA_REQUIRE(editor.hasSkeleton(animation));
  const std::vector<std::string> clips = editor.animationClips(animation);
  KIMIA_REQUIRE(clips.size() == 1U);
  KIMIA_REQUIRE(!clips[0].empty());
  editor.playClip(editor.entity(animation)->meshFile, clips[0]);
  KIMIA_REQUIRE(!editor.playingAnimations().empty());
  KIMIA_REQUIRE(editor.enterPlayMode());
  editor.update(1.0 / 60.0);
  kimia::MeshData stick;
  KIMIA_REQUIRE(editor.posedStickMesh(animation, stick));
  KIMIA_REQUIRE(stick.isValid());

  std::string saved;
  KIMIA_REQUIRE(kimia::WorldIO::save(editor.world(), saved));
  kimia::WorldData reloaded;
  KIMIA_REQUIRE(kimia::WorldIO::load(saved, reloaded, error));
  const kimia::EntityData* savedAnimation = reloaded.scene.get(reloaded.scene.find(animation));
  KIMIA_REQUIRE(savedAnimation != nullptr);
  KIMIA_REQUIRE(savedAnimation->meshFile == "animations/pleyer move/walk.fbx");
}

KIMIA_TEST(studio_control_targets_one_character_and_survives_serialization) {
  WorldEditor editor;
  streetWorld(editor);
  std::string error;
  const std::string first = editor.importModel("Tests/assets/skinned_bar.fbx", 1.0, error);
  const std::string second = editor.importModel("Tests/assets/skinned_bar.fbx", 1.0, error);
  KIMIA_REQUIRE(!first.empty() && !second.empty());

  const std::string saved = ask(editor, "/api/set-control",
                                {{"control", "kick"}, {"key", "k"},
                                 {"clipfile", "Tests/assets/skinned_bar.fbx"},
                                 {"clip", "Bend"}, {"target", second}});
  KIMIA_REQUIRE(has(saved, "\"ok\":true"));
  const std::string controls = ask(editor, "/api/controls");
  KIMIA_REQUIRE(has(controls, "\"target\":\"" + second + "\""));

  KIMIA_REQUIRE(has(ask(editor, "/api/do", {{"control", "kick"}}), "\"ok\":true"));
  KIMIA_REQUIRE(editor.playingAnimations().size() == 1U);
  kimia::MeshData posed;
  KIMIA_REQUIRE(editor.posedMesh(second, posed));
  KIMIA_REQUIRE(!editor.posedMesh(first, posed));

  std::string worldText;
  KIMIA_REQUIRE(kimia::WorldIO::save(editor.world(), worldText));
  kimia::WorldData reloaded;
  KIMIA_REQUIRE(kimia::WorldIO::load(worldText, reloaded, error));
  const kimia::Control* control = reloaded.input.find("kick");
  KIMIA_REQUIRE(control != nullptr);
  KIMIA_REQUIRE(control->target == second);
}

KIMIA_TEST(studio_bone_endpoint_returns_local_and_world_xz) {
  WorldEditor editor;
  streetWorld(editor);
  std::string error;
  const std::string name = editor.importModel("Tests/assets/spider.obj", 1.0, error);
  KIMIA_REQUIRE(!name.empty());
  kimia::RigBone bone;
  bone.name = "muzzle";
  bone.from = Vec3{0.0, 1.0, 0.0};
  bone.to = Vec3{0.0, 2.0, 0.0};
  KIMIA_REQUIRE(editor.setEntityBone(name, bone));
  KIMIA_REQUIRE(editor.setEntityTransform(name, Vec3{2.0, 0.0, 3.0}, Vec3{1.0, 1.0, 1.0}));
  const std::string response = ask(editor, "/api/bones", {{"name", name}, {"bone", "muzzle"}});
  KIMIA_REQUIRE(has(response, "\"local\":[0.000000,1.500000,0.000000]"));
  KIMIA_REQUIRE(has(response, "\"world\":[2.000000,1.500000,3.000000]"));
  KIMIA_REQUIRE(has(response, "\"x\":2.000000"));
  KIMIA_REQUIRE(has(response, "\"z\":3.000000"));
}
