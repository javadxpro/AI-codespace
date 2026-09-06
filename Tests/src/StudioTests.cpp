#include <kimia/AssetPipeline.h>
#include <kimia/Hud.h>
#include <kimia/Library.h>
#include <kimia/Studio.h>
#include <kimia/WorldIO.h>
#include <kimia_test.h>

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
