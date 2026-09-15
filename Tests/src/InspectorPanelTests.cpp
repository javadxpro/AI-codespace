#include <kimia_test.h>
#include <kimia/InspectorPanel.h>

KIMIA_TEST(Inspector_DrawEmptyDoesNotCrash) {
  kimia::ui::drawInspectorPanel({0, 0, 280, 200}, "Obj", "Empty", {}, 0);
}

KIMIA_TEST(Inspector_DrawOneFloat) {
  std::vector<kimia::ui::InspectorProp> v(1);
  v[0].name = "Speed";
  v[0].kind = kimia::ui::InspectorKind::Float;
  v[0].v0 = 1.5f;
  kimia::ui::drawInspectorPanel({0, 0, 280, 200}, "Player", "Cube", v, 0);
}

KIMIA_TEST(Inspector_DrawManyKinds) {
  std::vector<kimia::ui::InspectorProp> v;
  {
    kimia::ui::InspectorProp p; p.name = "Health";
    p.kind = kimia::ui::InspectorKind::Int; p.v0 = 100;
    v.push_back(p);
  }
  {
    kimia::ui::InspectorProp p; p.name = "Alive";
    p.kind = kimia::ui::InspectorKind::Bool; p.v0 = 1;
    v.push_back(p);
  }
  {
    kimia::ui::InspectorProp p; p.name = "Name";
    p.kind = kimia::ui::InspectorKind::String; p.s = "Hero";
    v.push_back(p);
  }
  {
    kimia::ui::InspectorProp p; p.name = "UV";
    p.kind = kimia::ui::InspectorKind::Vec2;
    p.v0 = 0.5f; p.v1 = 0.25f;
    v.push_back(p);
  }
  {
    kimia::ui::InspectorProp p; p.name = "Position";
    p.kind = kimia::ui::InspectorKind::Vec3;
    p.v0 = 1; p.v1 = 2; p.v2 = 3;
    v.push_back(p);
  }
  {
    kimia::ui::InspectorProp p; p.name = "Quat";
    p.kind = kimia::ui::InspectorKind::Vec4;
    p.v0 = 0; p.v1 = 0; p.v2 = 0; p.v3 = 1;
    v.push_back(p);
  }
  {
    kimia::ui::InspectorProp p; p.name = "Color";
    p.kind = kimia::ui::InspectorKind::Color;
    p.v0 = 1; p.v1 = 0.5f; p.v2 = 0.25f; p.v3 = 1;
    v.push_back(p);
  }
  kimia::ui::drawInspectorPanel({0, 0, 320, 320},
    "Hero", "Character", v, 0);
}

KIMIA_TEST(Inspector_DrawWithDisabledProps) {
  std::vector<kimia::ui::InspectorProp> v;
  kimia::ui::InspectorProp p; p.name = "A";
  p.kind = kimia::ui::InspectorKind::Float;
  p.v0 = 1.5f; p.enabled = false; v.push_back(p);
  kimia::ui::InspectorProp q; q.name = "B";
  q.kind = kimia::ui::InspectorKind::Float;
  q.v0 = 2.5f; q.enabled = true; v.push_back(q);
  kimia::ui::drawInspectorPanel({0, 0, 280, 200}, "Obj", "T", v, 0);
}

KIMIA_TEST(Inspector_DrawAtScroll) {
  std::vector<kimia::ui::InspectorProp> v;
  for (int i = 0; i < 30; ++i) {
    kimia::ui::InspectorProp p;
    p.name = "P" + std::to_string(i);
    p.kind = kimia::ui::InspectorKind::Float;
    p.v0 = static_cast<kimia::f32>(i);
    v.push_back(p);
  }
  kimia::ui::drawInspectorPanel({0, 0, 280, 200}, "Big", "Type", v, -50);
  kimia::ui::drawInspectorPanel({0, 0, 280, 200}, "Big", "Type", v, 100);
}

KIMIA_TEST(Inspector_DrawWithUnknownKind) {
  std::vector<kimia::ui::InspectorProp> v(1);
  v[0].name = "X";
  v[0].kind = static_cast<kimia::ui::InspectorKind>(99);
  kimia::ui::drawInspectorPanel({0, 0, 280, 200}, "Obj", "T", v, 0);
}

KIMIA_TEST(Inspector_DrawAtPhonePortrait) {
  std::vector<kimia::ui::InspectorProp> v;
  for (int i = 0; i < 10; ++i) {
    kimia::ui::InspectorProp p;
    p.name = "p" + std::to_string(i);
    p.kind = kimia::ui::InspectorKind::Float;
    p.v0 = static_cast<kimia::f32>(i) * 0.5f;
    v.push_back(p);
  }
  kimia::ui::drawInspectorPanel({0, 0, 240, 320}, "O", "T", v, 0);
}

KIMIA_TEST(Inspector_DrawAtTabletLandscape) {
  std::vector<kimia::ui::InspectorProp> v;
  for (int i = 0; i < 30; ++i) {
    kimia::ui::InspectorProp p;
    p.name = "field_" + std::to_string(i);
    p.kind = (i % 4 == 0) ? kimia::ui::InspectorKind::Int
            : (i % 4 == 1) ? kimia::ui::InspectorKind::Bool
            : (i % 4 == 2) ? kimia::ui::InspectorKind::String
            : kimia::ui::InspectorKind::Float;
    p.v0 = static_cast<kimia::f32>(i);
    p.s = "v" + std::to_string(i);
    v.push_back(p);
  }
  kimia::ui::drawInspectorPanel({0, 0, 480, 320}, "Hero", "Character", v, 0);
}
