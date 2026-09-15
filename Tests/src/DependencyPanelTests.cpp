#include <kimia_test.h>
#include <kimia/DependencyPanel.h>

KIMIA_TEST(Dependency_DrawEmptyDoesNotCrash) {
  kimia::ui::drawDependencyPanel({0, 0, 240, 200}, {}, 0);
}

KIMIA_TEST(Dependency_DrawOneDep) {
  std::vector<kimia::ui::Dependency> v(1);
  v[0].name = "default.mat";
  v[0].kind = "Material";
  v[0].direct = true;
  kimia::ui::drawDependencyPanel({0, 0, 240, 200}, v, 0);
}

KIMIA_TEST(Dependency_DrawManyDeps) {
  std::vector<kimia::ui::Dependency> v;
  const char* names[] = {"tree.obj", "tree.mat", "tree_tex.png",
                         "default.mat", "ground.png", "sky.png",
                         "fallback.mat"};
  const char* kinds[] = {"Model", "Material", "Texture",
                         "Material", "Texture", "Texture", "Material"};
  for (int i = 0; i < 7; ++i) {
    kimia::ui::Dependency d;
    d.name = names[i];
    d.kind = kinds[i];
    d.direct = (i < 4);
    v.push_back(d);
  }
  kimia::ui::drawDependencyPanel({0, 0, 280, 220}, v, 0);
}

KIMIA_TEST(Dependency_DrawAtScroll) {
  std::vector<kimia::ui::Dependency> v;
  for (int i = 0; i < 20; ++i) {
    kimia::ui::Dependency d;
    d.name = "d_" + std::to_string(i);
    d.kind = "Material";
    d.direct = (i % 2 == 0);
    v.push_back(d);
  }
  kimia::ui::drawDependencyPanel({0, 0, 240, 200}, v, -50);
  kimia::ui::drawDependencyPanel({0, 0, 240, 200}, v, 100);
}

KIMIA_TEST(Dependency_DrawAtPhonePortrait) {
  std::vector<kimia::ui::Dependency> v;
  for (int i = 0; i < 5; ++i) {
    kimia::ui::Dependency d;
    d.name = "p" + std::to_string(i);
    d.kind = "Texture";
    v.push_back(d);
  }
  kimia::ui::drawDependencyPanel({0, 0, 240, 320}, v, 0);
}

KIMIA_TEST(Dependency_DrawAtTabletLandscape) {
  std::vector<kimia::ui::Dependency> v;
  const char* kinds[] = {"Material", "Texture", "Model", "Script"};
  for (int i = 0; i < 12; ++i) {
    kimia::ui::Dependency d;
    d.name = "dep_" + std::to_string(i);
    d.kind = kinds[i % 4];
    d.direct = (i < 6);
    v.push_back(d);
  }
  kimia::ui::drawDependencyPanel({0, 0, 480, 280}, v, 0);
}
