#include <kimia_test.h>
#include <kimia/ShaderPanel.h>

KIMIA_TEST(Shader_DrawEmptyDoesNotCrash) {
  kimia::ui::drawShaderPanel({0, 0, 280, 200}, {}, 0);
}

KIMIA_TEST(Shader_DrawOneShader) {
  std::vector<kimia::ui::ShaderEntry> v(1);
  v[0].name = "rounded_rect";
  v[0].type = kimia::ui::ShaderType::Fragment;
  v[0].lineCount = 42;
  v[0].compiled = true;
  v[0].compileMs = 1.5f;
  kimia::ui::drawShaderPanel({0, 0, 280, 200}, v, 0);
}

KIMIA_TEST(Shader_DrawAllTypes) {
  std::vector<kimia::ui::ShaderEntry> v;
  v.push_back({"atlas_vs",   kimia::ui::ShaderType::Vertex,   60, true,  2.0f, ""});
  v.push_back({"atlas_fs",   kimia::ui::ShaderType::Fragment, 80, true,  3.5f, ""});
  v.push_back({"physics_cs", kimia::ui::ShaderType::Compute,  120, true, 5.0f, ""});
  v.push_back({"broken",     kimia::ui::ShaderType::Fragment, 50, false, 1.0f, "syntax error on line 12"});
  kimia::ui::drawShaderPanel({0, 0, 280, 200}, v, 0);
}

KIMIA_TEST(Shader_DrawAtScroll) {
  std::vector<kimia::ui::ShaderEntry> v;
  for (int i = 0; i < 30; ++i) {
    kimia::ui::ShaderEntry s;
    s.name = "shader_" + std::to_string(i);
    s.type = static_cast<kimia::ui::ShaderType>(i % 3);
    s.lineCount = i * 10;
    s.compiled = (i % 4 != 0);
    s.compileMs = static_cast<float>(i) * 0.5f;
    v.push_back(s);
  }
  kimia::ui::drawShaderPanel({0, 0, 280, 200}, v, -50);
  kimia::ui::drawShaderPanel({0, 0, 280, 200}, v, 100);
}

KIMIA_TEST(Shader_DrawAtPhonePortrait) {
  std::vector<kimia::ui::ShaderEntry> v;
  for (int i = 0; i < 5; ++i) {
    kimia::ui::ShaderEntry s;
    s.name = "s" + std::to_string(i);
    s.type = kimia::ui::ShaderType::Fragment;
    s.compiled = true;
    v.push_back(s);
  }
  kimia::ui::drawShaderPanel({0, 0, 240, 320}, v, 0);
}

KIMIA_TEST(Shader_DrawAtTabletLandscape) {
  std::vector<kimia::ui::ShaderEntry> v;
  for (int i = 0; i < 15; ++i) {
    kimia::ui::ShaderEntry s;
    s.name = "shader_" + std::to_string(i);
    s.type = static_cast<kimia::ui::ShaderType>(i % 3);
    s.lineCount = i * 20;
    s.compiled = (i % 5 != 0);
    s.compileMs = static_cast<float>(i);
    if (!s.compiled) s.error = "missing semicolon";
    v.push_back(s);
  }
  kimia::ui::drawShaderPanel({0, 0, 480, 320}, v, 0);
}
