#include <kimia_test.h>
#include <kimia/PluginPanel.h>

KIMIA_TEST(Plugin_DrawEmptyDoesNotCrash) {
  kimia::ui::drawPluginPanel({0, 0, 280, 200}, {}, 0);
}

KIMIA_TEST(Plugin_DrawOnePlugin) {
  std::vector<kimia::ui::PluginEntry> v(1);
  v[0].name = "Mesh Importer";
  v[0].version = "1.2.3";
  v[0].author = "Alice";
  v[0].enabled = true;
  v[0].hasUpdate = false;
  kimia::ui::drawPluginPanel({0, 0, 280, 200}, v, 0);
}

KIMIA_TEST(Plugin_DrawManyPlugins) {
  std::vector<kimia::ui::PluginEntry> v;
  v.push_back({"Mesh Importer",    "1.2.3", "Alice",  true,  false});
  v.push_back({"Physics Debugger", "0.9.1", "Bob",    true,  true});
  v.push_back({"Old Plugin",       "0.1.0", "Legacy", false, false});
  v.push_back({"Asset Browser",    "2.0.0", "Carol",  true,  false});
  v.push_back({"Profiler",         "1.5.2", "Dave",   true,  true});
  v.push_back({"Disabled One",     "0.5.0", "Eve",    false, false});
  kimia::ui::drawPluginPanel({0, 0, 320, 240}, v, 0);
}

KIMIA_TEST(Plugin_DrawAtScroll) {
  std::vector<kimia::ui::PluginEntry> v;
  for (int i = 0; i < 20; ++i) {
    kimia::ui::PluginEntry p;
    p.name = "p" + std::to_string(i);
    p.version = "v" + std::to_string(i);
    p.author = "a";
    p.enabled = (i % 3 != 0);
    p.hasUpdate = (i % 4 == 0);
    v.push_back(p);
  }
  kimia::ui::drawPluginPanel({0, 0, 280, 200}, v, -50);
  kimia::ui::drawPluginPanel({0, 0, 280, 200}, v, 100);
}

KIMIA_TEST(Plugin_DrawAtPhonePortrait) {
  std::vector<kimia::ui::PluginEntry> v;
  for (int i = 0; i < 4; ++i) {
    kimia::ui::PluginEntry p;
    p.name = "p" + std::to_string(i);
    p.version = "1.0";
    v.push_back(p);
  }
  kimia::ui::drawPluginPanel({0, 0, 240, 320}, v, 0);
}

KIMIA_TEST(Plugin_DrawAtTabletLandscape) {
  std::vector<kimia::ui::PluginEntry> v;
  for (int i = 0; i < 15; ++i) {
    kimia::ui::PluginEntry p;
    p.name = "Plugin_" + std::to_string(i);
    p.version = "1." + std::to_string(i);
    p.author = "Author_" + std::to_string(i);
    p.enabled = (i % 2 == 0);
    p.hasUpdate = (i % 5 == 0);
    v.push_back(p);
  }
  kimia::ui::drawPluginPanel({0, 0, 480, 320}, v, 0);
}
