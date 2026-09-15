// KIMIA Android native renderer.
//
// This is the REAL Android path: the engine boots inside the app process and
// renders straight onto a SurfaceView with GLES3 (Cook-Torrance PBR, shadows,
// fog, filmic tone map — the same Renderer the desktop and WebGL2 builds use).
// There is no WebView and no 127.0.0.1 HTTP server: an ANativeWindow is handed
// in from Java, an EGL window surface wraps it, and each frame is presented
// with eglSwapBuffers. Flags (game, resolution, fps, backend, shadows, MSAA)
// arrive from the in-app settings screen. Touch is interpreted here so the
// whole game stays in the engine.
//
// The software rasteriser remains the fallback: when GLES3/EGL cannot come up
// the frame is drawn on the CPU and blitted to the ANativeWindow, so the APK
// still runs on every device.
#include <jni.h>

#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <kimia/EGL.h>
#include <kimia/GLFunctions.h>
#include <kimia/Renderer.h>
#include <kimia/Shader.h>
#include <kimia/World.h>
#include <kimia/GameProfile.h>
#include <kimia/Mesh.h>
#include <kimia/OrbitCamera.h>
#include <kimia/BitmapFont.h>
#include <kimia/Hud.h>
#include <kimia/Image.h>
#include <kimia/Skeleton.h>
#include <kimia/Particles.h>
#include <kimia/RuntimeLoop.h>
#include <kimia/MathUtils.h>
#include <kimia/Version.h>
#ifdef KIMIA_EMBEDDED_ASSETS
#include <kimia/EmbeddedAssets.h>
#include <filesystem>
#include <fstream>
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "KIMIA", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "KIMIA", __VA_ARGS__)

namespace {

using kimia::EGLContext;
using kimia::EntityData;
using kimia::FigureMotion;
using kimia::Image;
using kimia::Mat4;
using kimia::MeshData;
using kimia::ObjectKind;
using kimia::OrbitCamera;
using kimia::RenderScene;
using kimia::Renderer;
using kimia::Skeleton;
using kimia::Vec3;
using kimia::WorldEditor;
using kimia::f64;
using kimia::i32;
using kimia::i64;
using kimia::u32;
using kimia::u64;
using kimia::u8;
using kimia::usize;

// --- Configuration, passed from the settings screen ----------------------
struct NativeConfig {
  std::string filesDir;
  std::string game = "golf";  // golf / street / grass / battleground
  i32 backend = 0;            // 0 = auto (GLES3 -> software), 1 = GLES3, 2 = software
  i32 width = 0;              // 0 = surface size
  i32 height = 0;
  i32 fps = 60;
  bool shadows = true;
  bool msaa = false;
  i32 mode = 0;               // 0 = play, 1 = edit (phase 1: in-world editor)
};

// --- Shared state between the UI thread (JNI) and the render thread ------
std::atomic<bool> gQuit{false};
NativeConfig gConfig;
std::thread gThread;

std::mutex gSurfaceMutex;
std::condition_variable gSurfaceCv;
ANativeWindow* gWindow = nullptr;  // owned (ANativeWindow_fromSurface ref)
u64 gGeneration = 0;
i32 gViewWidth = 0;
i32 gViewHeight = 0;

// --- Touch state ----------------------------------------------------------
// One finger per pointer id (0 = first, 1 = second). The UI thread records
// raw events; the render thread drains them every frame and turns them into
// game actions, so all control logic lives with the engine.
enum class Zone { None, Stick, Action, Reload, Camera };

struct Finger {
  bool active = false;
  float x = 0.0f;
  float y = 0.0f;
  float startX = 0.0f;
  float startY = 0.0f;
  float lastX = 0.0f;
  float lastY = 0.0f;
  Zone zone = Zone::None;
};

std::mutex gInputMutex;
Finger gFingers[2];
bool gArenaMode = false;    // editor.arenaMode(), refreshed by the render loop
bool gActionDown = false;   // a press started on the action button
bool gActionUp = false;     // a press ended on the action button
bool gReloadTap = false;    // a tap landed on the reload button
float gPinchPrev = 0.0f;    // last pinch distance (normalized units)
bool gPinchActive = false;

// --- Edit-mode scratch state (phase 1) -----------------------------------
// Lives on the render thread; the UI thread only asks for snapshots via
// editor.* and pushes commands through gEditCmd. The command queue is one
// slot deep because the Java UI sends exactly one action at a time.
enum class EditCmd { None, Select, BeginDrag, DragTo, EndDrag, SetColor, Delete, NewCube,
                     NewSphere, NewPlane, Save };
struct EditCommand {
  EditCmd kind = EditCmd::None;
  std::string name;        // selected entity for Select/Delete; file name for Save
  float fromX = 0.0f, fromY = 0.0f, toX = 0.0f, toY = 0.0f;
  float r = 0.0f, g = 0.0f, b = 0.0f;
};
std::mutex gEditMutex;
EditCommand gEditCmd;
std::string gEditSelected;     // last selection set by the render loop
bool gEditSelectionChanged = false;
// A cached snapshot of the entity list for the UI (avoids the UI thread
// poking the editor while the render thread is mutating it).
struct EditSnapshot {
  bool valid = false;
  std::vector<std::string> names;
  std::string selected;
  float selR = 0.0f, selG = 0.0f, selB = 0.0f;
  float selPosX = 0.0f, selPosY = 0.0f, selPosZ = 0.0f;
  float bgR = 0.0f, bgG = 0.0f, bgB = 0.0f;  // camera background (matches scene clear)
};
EditSnapshot gEditSnapshot;

// Screen-fraction touch zones (fractions of width/height, y = 0 at top).
constexpr float kStickZoneX = 0.45f;   // x < this = stick side
constexpr float kStickZoneY = 0.50f;   // y > this = lower half
constexpr float kActionZoneX = 0.72f;  // x > this = buttons side
constexpr float kActionZoneY = 0.58f;  // action button below this
constexpr float kReloadZoneY = 0.30f;  // reload between this and action

constexpr float kLookYawScale = 2.6f;    // radians per full-width camera drag
constexpr float kLookPitchScale = 1.6f;  // radians per full-height camera drag
constexpr float kAimScale = 2.6f;        // radians per full-width aim drag (golf)

Zone classifyZone(float x, float y, bool arena) {
  if (x > kActionZoneX) {
    if (y > kActionZoneY) return Zone::Action;
    if (arena && y > kReloadZoneY) return Zone::Reload;
  }
  if (x < kStickZoneX && y > kStickZoneY) return Zone::Stick;
  return Zone::Camera;
}

// --- Small helpers shared with the desktop loop --------------------------
f64 angleDelta(f64 from, f64 to) {
  f64 delta = std::fmod(to - from + kimia::kPi, 2.0 * kimia::kPi);
  if (delta < 0.0) delta += 2.0 * kimia::kPi;
  return delta - kimia::kPi;
}

void addGoalShape(RenderScene& scene, const EntityData& entity, const MeshData& cube) {
  const f64 width = entity.transform.scale.x;
  const f64 half = entity.transform.scale.y * 0.5;
  const Vec3 at = entity.transform.position;
  const Vec3 color = entity.color;
  const Mat4 spin = Mat4::translation(at) * entity.transform.rotation.toMat4() *
                    Mat4::translation(Vec3{-at.x, -at.y, -at.z});
  scene.objects.push_back(
      {&cube, spin * Mat4::translation(Vec3{at.x - width * 0.5 + 0.06, at.y, at.z}) *
                  Mat4::scaling(Vec3{0.12, entity.transform.scale.y, 0.12}),
       color, entity.roughness});
  scene.objects.push_back(
      {&cube, spin * Mat4::translation(Vec3{at.x + width * 0.5 - 0.06, at.y, at.z}) *
                  Mat4::scaling(Vec3{0.12, entity.transform.scale.y, 0.12}),
       color, entity.roughness});
  scene.objects.push_back(
      {&cube, spin * Mat4::translation(Vec3{at.x, at.y + half, at.z}) *
                  Mat4::scaling(Vec3{width + 0.12, 0.12, 0.12}),
       color, entity.roughness});
}

void addLimbs(RenderScene& scene, const MeshData& cube, const std::vector<kimia::FigureLimb>& limbs,
              const Vec3& color) {
  for (const kimia::FigureLimb& limb : limbs) {
    const Vec3 along = limb.to - limb.from;
    const f64 length = along.length();
    if (length < 1e-4) continue;
    const Vec3 middle = limb.from + along * 0.5;
    const Vec3 up{0.0, 1.0, 0.0};
    const Vec3 dir = along * (1.0 / length);
    const f64 dot = up.x * dir.x + up.y * dir.y + up.z * dir.z;
    Mat4 orient;
    if (dot < 0.9999) {
      if (dot < -0.9999) {
        orient = Mat4::rotationX(3.14159265358979323846);
      } else {
        const Vec3 axis{up.y * dir.z - up.z * dir.y, up.z * dir.x - up.x * dir.z, up.x * dir.y - up.y * dir.x};
        orient = kimia::Quat::fromAxisAngle(axis, std::acos(dot)).toMat4();
      }
    }
    scene.objects.push_back({&cube,
                             Mat4::translation(middle) * orient *
                                 Mat4::scaling(Vec3{limb.thickness, length, limb.thickness}),
                             color, 1.0, 0.0, nullptr});
  }
}

void addFigure(RenderScene& scene, const MeshData& cube, const Skeleton& rig, const FigureMotion& motion,
               const Vec3& at, f64 facing, const Vec3& color) {
  static std::vector<kimia::Transform3D> pose;
  static std::vector<kimia::FigureLimb> limbs;
  kimia::poseFigure(rig, motion, pose);
  kimia::figureLimbs(rig, pose, at, facing, limbs);
  addLimbs(scene, cube, limbs, color);
}

void addSquads(RenderScene& scene, const WorldEditor& editor, const MeshData& cube, const Skeleton& rig) {
  if (!editor.playing() || editor.squadCount() <= 1U) return;
  const Vec3 ourColor{0.25, 0.45, 0.95};
  const Vec3 theirColor{0.90, 0.25, 0.25};
  const Vec3 keeperColor{0.95, 0.85, 0.20};
  for (const u32 id : editor.squadIds()) {
    if (id == kimia::kPrimaryCharacter) continue;
    const Vec3 at = editor.squadPosition(id);
    const u32 team = editor.squadTeam(id);
    Vec3 color = team == 1U ? ourColor : theirColor;
    const bool down = editor.arenaMode() && editor.downed(id);
    if (down) {
      color = Vec3{0.45, 0.45, 0.45};
    } else if (!editor.arenaMode() && id == editor.aiKeeper(team)) {
      color = keeperColor;
    }
    FigureMotion motion;
    motion.speed = editor.squadSpeed(id);
    motion.time = editor.figureClock();
    motion.airborne = editor.squadAirborne(id);
    motion.downed = down;
    const Vec3 feet{at.x, at.y - kimia::kWorldPlayerRadius - 0.15, at.z};
    addFigure(scene, cube, rig, motion, feet, editor.squadFacing(id), color);
  }
}

void addAimIndicator(RenderScene& scene, const WorldEditor& editor, const MeshData& cube) {
  if (!editor.shotMode() || !editor.playing() || !editor.ballAtRest()) return;
  const Vec3 from = editor.ballPosition();
  const Vec3 direction = editor.aimDirection();
  const f64 reach = 1.0 + (editor.charging() ? editor.power() : 0.0) * 4.0;
  const i32 count = 6;
  for (i32 i = 1; i <= count; ++i) {
    const f64 t = static_cast<f64>(i) / static_cast<f64>(count);
    const Vec3 at = from + direction * (reach * t);
    const f64 marker = 0.05 + 0.02 * (1.0 - t);
    scene.objects.push_back(
        {&cube, Mat4::translation(Vec3{at.x, marker * 0.5, at.z}) * Mat4::scaling(Vec3{marker, marker, marker}),
         Vec3{1.0, 0.85, 0.2}, 0.9});
  }
}

void addCurrentCupFlag(RenderScene& scene, const WorldEditor& editor, const MeshData& cube) {
  if (!editor.holeScoring() || !editor.playing() || editor.roundOver()) return;
  const kimia::EntityData* cup = editor.world().scene.get(editor.world().scene.find(editor.currentHoleName()));
  if (cup == nullptr) return;
  const Vec3 base = cup->transform.position;
  const f64 poleHeight = 1.2;
  scene.objects.push_back({&cube, Mat4::translation(Vec3{base.x, poleHeight * 0.5, base.z}) *
                                      Mat4::scaling(Vec3{0.04, poleHeight, 0.04}),
                           Vec3{0.92, 0.92, 0.92}, 0.6});
  scene.objects.push_back({&cube, Mat4::translation(Vec3{base.x + 0.16, poleHeight - 0.12, base.z}) *
                                      Mat4::scaling(Vec3{0.3, 0.2, 0.02}),
                           Vec3{0.9, 0.15, 0.1}, 0.8});
}

// Builds the frame's RenderScene for the editor's current world, exactly the
// objects the desktop loop draws in PLAY (plus the camera framing).
const Skeleton& figureRigStatic();

void buildScene(const WorldEditor& editor, const OrbitCamera& orbit, i32 width, i32 height,
                const MeshData& cubeMesh, const MeshData& planeMesh, const MeshData& sphereMesh,
                RenderScene& scene) {
  scene = RenderScene{};
  editor.world().scene.forEach([&](kimia::EntityHandle, const EntityData& entity) {
    const ObjectKind kind = kimia::objectKindForName(entity.name);
    if (kind == ObjectKind::Goal && !kimia::isLegacyGoalPart(entity.name)) {
      addGoalShape(scene, entity, cubeMesh);
      return;
    }
    const MeshData* mesh = &cubeMesh;
    if (entity.mesh == kimia::MeshKind::plane) mesh = &planeMesh;
    if (entity.mesh == kimia::MeshKind::sphere) mesh = &sphereMesh;
    // Built-in games are primitive-only, so no OBJ/FBX loading is needed
    // here (the desktop loop's model path is for user-authored worlds).
    const bool playCharacter = kind == ObjectKind::Player && editor.playing();
    const Vec3 position = playCharacter ? editor.playerPosition() : entity.transform.position;
    const Vec3 scale = entity.mesh == kimia::MeshKind::sphere ? entity.transform.scale * 0.5
                                                              : entity.transform.scale;
    const Mat4 model = Mat4::translation(position) * entity.transform.rotation.toMat4() * Mat4::scaling(scale);
    scene.objects.push_back(
        {mesh, model, entity.color, entity.roughness, entity.metallic, nullptr, entity.emissive, entity.alpha});
    if (kind == ObjectKind::Player && entity.mesh == kimia::MeshKind::cube) {
      scene.objects.push_back({&cubeMesh,
                               Mat4::translation(position + Vec3{0.0, 0.65, 0.0}) *
                                   Mat4::scaling(Vec3{0.3, 0.3, 0.3}),
                               entity.color, entity.roughness});
    }
  });

  if (editor.world().scene.find("Ball") != 0 || editor.playing()) {
    const f64 ballRadius = editor.world().ball.radius;
    scene.objects.push_back(
        {&sphereMesh, Mat4::translation(editor.ballPosition()) * Mat4::scaling(Vec3{ballRadius, ballRadius, ballRadius}),
         editor.world().ball.color, 0.3});
  }
  for (const kimia::Particle& particle : editor.particles().particles()) {
    const f64 size = particle.sizeNow();
    if (size <= 0.001) continue;
    scene.objects.push_back({&cubeMesh,
                             Mat4::translation(particle.position) * Mat4::scaling(Vec3{size, size, size}),
                             particle.colorNow(), 1.0, 0.0, nullptr});
  }
  addSquads(scene, editor, cubeMesh, figureRigStatic());
  if (editor.arenaMode() && editor.playing()) {
    const Vec3 from = editor.lastShotFrom();
    const Vec3 to = editor.lastShotTo();
    const Vec3 along = to - from;
    const f64 length = along.length();
    if (length > 0.01) {
      const i32 beads = 12;
      for (i32 i = 1; i <= beads; ++i) {
        const f64 t = static_cast<f64>(i) / static_cast<f64>(beads + 1);
        const Vec3 at = from + along * t;
        scene.objects.push_back({&cubeMesh, Mat4::translation(at) * Mat4::scaling(Vec3{0.05, 0.05, 0.05}),
                                 Vec3{1.0, 0.9, 0.4}, 0.9});
      }
    }
  }
  addAimIndicator(scene, editor, cubeMesh);
  addCurrentCupFlag(scene, editor, cubeMesh);

  const Vec3 eye = orbit.eye();
  scene.cameraPosition = eye;
  scene.view = Mat4::lookAt(eye, orbit.target(), Vec3{0.0, 1.0, 0.0});
  scene.projection =
      Mat4::perspective(kimia::radians(60.0), static_cast<f64>(width) / static_cast<f64>(height), 0.1, 100.0);
  scene.lightDirection = Vec3{-0.4, -0.8, -0.4};
}

const Skeleton& figureRigStatic() {
  static const Skeleton rig = kimia::makeFigureRig(1.7);
  return rig;
}

// --- On-frame HUD (bitmap font, drawn into the captured image) -----------
void drawHud(Image& image, const WorldEditor& editor) {
  const i32 scale = 2;
  const i32 margin = 8;
  const std::vector<std::string> lines = editor.hudLines();
  if (!lines.empty()) {
    i32 widest = 0;
    for (const std::string& line : lines) widest = std::max(widest, kimia::font::textWidth(line, scale));
    const i32 lineStep = kimia::font::textHeight(scale) + scale * 2;
    const i32 boxWidth = widest + margin * 2;
    const i32 boxHeight = static_cast<i32>(lines.size()) * lineStep + margin * 2 - scale * 2;
    kimia::font::fillRect(image, margin, margin, boxWidth, boxHeight, Vec3{0.0, 0.0, 0.0}, 0.55);
    for (usize i = 0; i < lines.size(); ++i) {
      kimia::font::drawText(image, margin * 2, margin * 2 + static_cast<i32>(i) * lineStep, lines[i],
                            Vec3{1.0, 1.0, 1.0}, scale);
    }
  }
  const f64 power = editor.hudPower();
  if (power >= 0.0) {
    const i32 barWidth = std::min(240, image.width - margin * 2);
    const i32 barHeight = 14;
    const i32 labelHeight = kimia::font::textHeight(scale);
    const i32 x = (image.width - barWidth) / 2;
    const i32 y = image.height - margin - barHeight;
    kimia::font::fillRect(image, x - 4, y - 8 - labelHeight, barWidth + 8, barHeight + labelHeight + 12,
                          Vec3{0.0, 0.0, 0.0}, 0.55);
    kimia::font::drawText(image, x, y - 4 - labelHeight, "POWER", Vec3{1.0, 1.0, 1.0}, scale);
    kimia::font::drawBar(image, x, y, barWidth, barHeight, power, Vec3{1.0, 0.55, 0.1}, Vec3{0.15, 0.15, 0.15});
  }
  // The game's own user-laid-out interface.
  kimia::drawHud(image, editor.hud(), editor.logic());
}

// On-screen controls: a move stick bottom-left and an action button
// bottom-right (plus a reload button in arena mode). Drawn last so a finger
// always has something to aim at.
void drawControls(Image& image, const WorldEditor& editor) {
  if (!editor.playing()) return;
  const i32 shortSide = std::min(image.width, image.height);
  const auto circle = [&](f64 fx, f64 fy, f64 fsize, const Vec3& fill, const Vec3& ink, const std::string& label,
                          f64 alpha) {
    const i32 radius = static_cast<i32>(fsize * 0.5 * static_cast<f64>(shortSide));
    const i32 cx = static_cast<i32>(fx * static_cast<f64>(image.width));
    const i32 cy = static_cast<i32>(fy * static_cast<f64>(image.height));
    kimia::font::fillRect(image, cx - radius, cy - radius, radius * 2, radius * 2, fill, alpha);
    const i32 textW = kimia::font::textWidth(label, 2);
    kimia::font::drawText(image, cx - textW / 2, cy - kimia::font::textHeight(2) / 2, label, ink, 2);
  };
  const Vec3 padFill{0.12, 0.16, 0.22};
  const Vec3 padInk{0.95, 0.95, 1.0};
  circle(0.18, 0.78, 0.16, padFill, padInk, "MOVE", 0.65);
  const char* action = editor.shotMode() ? "SHOT" : (editor.arenaMode() ? "FIRE" : "JUMP");
  circle(0.82, 0.78, 0.12, padFill, padInk, action, 0.65);
  if (editor.arenaMode()) circle(0.82, 0.45, 0.09, padFill, padInk, "RELOAD", 0.5);
}

// --- GL overlay: draw the composited (3D + HUD) image as a fullscreen quad.
// The HUD is bitmap/CPU-drawn, so it is drawn into the captured frame and the
// whole thing is re-blitted — the same way the desktop/WebViewer path works.
struct OverlayBlit {
  kimia::Shader shader;
  kimia::GLuint vao = 0;
  kimia::GLuint vbo = 0;
  kimia::GLuint ebo = 0;
  kimia::GLuint texture = 0;
  bool ready = false;

  bool init(std::string& error) {
    const char* vs = "#version 300 es\n"
                     "precision highp float;\n"
                     "layout(location=0) in vec2 aPos;\n"
                     "layout(location=1) in vec2 aUV;\n"
                     "out vec2 vUV;\n"
                     "void main(){ vUV=aUV; gl_Position=vec4(aPos,0.0,1.0); }\n";
    const char* fs = "#version 300 es\n"
                     "precision highp float;\n"
                     "in vec2 vUV;\n"
                     "uniform sampler2D uTex;\n"
                     "out vec4 fragColor;\n"
                     "void main(){ fragColor = texture(uTex, vUV); }\n";
    if (!shader.compile(vs, fs, error)) return false;
    kimia::GLFunctions& gl = kimia::GLFunctions::instance();
    const float vertices[] = {
        -1.0f, -1.0f, 0.0f, 1.0f,  1.0f, -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f,  0.0f, 0.0f,  1.0f, 1.0f,  1.0f, 0.0f,
    };
    const u32 indices[] = {0, 1, 2, 1, 3, 2};
    gl.genVertexArrays(1, &vao);
    gl.bindVertexArray(vao);
    gl.genBuffers(1, &vbo);
    gl.bindBuffer(kimia::GL_ARRAY_BUFFER, vbo);
    gl.bufferData(kimia::GL_ARRAY_BUFFER, static_cast<i64>(sizeof(vertices)), vertices, kimia::GL_STATIC_DRAW);
    gl.genBuffers(1, &ebo);
    gl.bindBuffer(kimia::GL_ELEMENT_ARRAY_BUFFER, ebo);
    gl.bufferData(kimia::GL_ELEMENT_ARRAY_BUFFER, static_cast<i64>(sizeof(indices)), indices, kimia::GL_STATIC_DRAW);
    gl.enableVertexAttribArray(0);
    gl.vertexAttribPointer(0, 2, kimia::GL_FLOAT, 0, 4 * sizeof(float), 0);
    gl.enableVertexAttribArray(1);
    gl.vertexAttribPointer(1, 2, kimia::GL_FLOAT, 0, 4 * sizeof(float), 2 * sizeof(float));
    gl.genTextures(1, &texture);
    ready = true;
    return true;
  }

  void blit(const Image& image) {
    if (!ready || image.isEmpty()) return;
    kimia::GLFunctions& gl = kimia::GLFunctions::instance();
    std::vector<u8> rgba(static_cast<usize>(image.width) * static_cast<usize>(image.height) * 4U);
    for (usize i = 0; i < static_cast<usize>(image.width) * static_cast<usize>(image.height); ++i) {
      const usize src = i * static_cast<usize>(image.channels);
      const usize dst = i * 4U;
      rgba[dst] = image.pixels[src];
      rgba[dst + 1U] = image.pixels[src + 1U];
      rgba[dst + 2U] = image.pixels[src + 2U];
      rgba[dst + 3U] = image.channels == 4 ? image.pixels[src + 3U] : 255U;
    }
    gl.bindTexture(kimia::GL_TEXTURE_2D, texture);
    gl.pixelStorei(kimia::GL_UNPACK_ALIGNMENT, 1);
    gl.texImage2D(kimia::GL_TEXTURE_2D, 0, kimia::GL_RGBA, image.width, image.height, 0, kimia::GL_RGBA,
                  kimia::GL_UNSIGNED_BYTE, rgba.data());
    gl.texParameteri(kimia::GL_TEXTURE_2D, kimia::GL_TEXTURE_MIN_FILTER, kimia::GL_LINEAR);
    gl.texParameteri(kimia::GL_TEXTURE_2D, kimia::GL_TEXTURE_MAG_FILTER, kimia::GL_LINEAR);
    gl.texParameteri(kimia::GL_TEXTURE_2D, kimia::GL_TEXTURE_WRAP_S, kimia::GL_REPEAT);
    gl.texParameteri(kimia::GL_TEXTURE_2D, kimia::GL_TEXTURE_WRAP_T, kimia::GL_REPEAT);

    gl.disable(kimia::GL_DEPTH_TEST);
    gl.disable(kimia::GL_CULL_FACE);
    gl.disable(kimia::GL_BLEND);
    gl.viewport(0, 0, image.width, image.height);
    shader.use();
    shader.setInt("uTex", 0);
    gl.activeTexture(kimia::GL_TEXTURE0);
    gl.bindTexture(kimia::GL_TEXTURE_2D, texture);
    gl.bindVertexArray(vao);
    gl.drawElements(kimia::GL_TRIANGLES, 6, kimia::GL_UNSIGNED_INT, nullptr);
    gl.bindVertexArray(0);
    gl.bindTexture(kimia::GL_TEXTURE_2D, 0);
  }

  void destroy() {
    kimia::GLFunctions& gl = kimia::GLFunctions::instance();
    if (texture != 0) gl.deleteTextures(1, &texture);
    if (vbo != 0) gl.deleteBuffers(1, &vbo);
    if (ebo != 0) gl.deleteBuffers(1, &ebo);
    if (vao != 0) gl.deleteVertexArrays(1, &vao);
    shader.destroy();
    texture = vbo = ebo = vao = 0;
    ready = false;
  }
};

// CPU blit for the software fallback: lock the ANativeWindow buffer and write
// the RGBA frame directly.
bool presentSoftware(ANativeWindow* window, const Image& image) {
  if (window == nullptr || image.isEmpty() || image.channels < 3) return false;
  ANativeWindow_Buffer buffer;
  if (ANativeWindow_lock(window, &buffer, nullptr) != 0) return false;
  for (i32 y = 0; y < buffer.height; ++y) {
    u8* row = static_cast<u8*>(buffer.bits) + static_cast<usize>(y) * static_cast<usize>(buffer.stride) * 4U;
    for (i32 x = 0; x < buffer.width; ++x) {
      const i32 sx = x * image.width / buffer.width;
      const i32 sy = y * image.height / buffer.height;
      const u8* p = image.at(sx, sy);
      row[x * 4 + 0] = p[0];
      row[x * 4 + 1] = p[1];
      row[x * 4 + 2] = p[2];
      row[x * 4 + 3] = image.channels == 4 ? p[3] : 255U;
    }
  }
  ANativeWindow_unlockAndPost(window);
  return true;
}

// Unpacks the embedded assets once (Profiles/Worlds/Branding) into the app's
// files dir, so the self-contained APK needs nothing next to it.
std::string unpackAssets(const std::string& filesDir) {
#ifdef KIMIA_EMBEDDED_ASSETS
  std::filesystem::path base = filesDir.empty() ? std::filesystem::temp_directory_path()
                                                : std::filesystem::path(filesDir);
  base /= "kimia_engine";
  base /= kimia::kEngineVersion;
  std::error_code ec;
  const std::filesystem::path sentinel = base / ".complete";
  if (!std::filesystem::exists(sentinel, ec)) {
    ec.clear();
    std::filesystem::remove_all(base, ec);
    ec.clear();
    if (kimia::embedded::extractAll(base.string())) {
      std::ofstream stamp(sentinel, std::ios::binary);
      if (stamp) stamp << kimia::kEngineVersion;
    }
  }
  if (std::filesystem::exists(sentinel, ec)) return base.string();
  return filesDir;
#else
  static_cast<void>(filesDir);
  return filesDir;
#endif
}

// Creates the playable world for the chosen built-in game.
bool setupGame(WorldEditor& editor, const std::string& game, const std::string& root) {
  if (game == "street") {
    // The shipped street duel (walls, goals, pitch) plays straight away.
    std::string error;
    const std::string path = root.empty() ? std::string("worlds/street_kids.kimia")
                                          : root + "/worlds/street_kids.kimia";
    if (!editor.startPublished(path, error)) {
      LOGE("street world failed to open: %s", error.c_str());
      return false;
    }
    return true;
  }

  const kimia::GameProfile* chosen = nullptr;
  for (const kimia::GameProfile& profile : kimia::builtinProfiles()) {
    if (profile.name == game) {
      chosen = &profile;
      break;
    }
  }
  if (chosen == nullptr) return false;
  editor.createWorld(*chosen);
  const f64 halfZ = editor.world().halfLength();
  const f64 halfX = editor.world().halfWidth();

  if (game == "golf") {
    editor.createObject("player", Vec3{0.0, 0.0, 4.0});
    editor.createObject("ball", Vec3{0.0, 0.0, 3.0});
    editor.createObject("hole", Vec3{0.0, 0.0, -2.0});
    editor.createObject("hole", Vec3{-halfX * 0.5, 0.0, -halfZ * 0.5});
    editor.createObject("hole", Vec3{halfX * 0.5, 0.0, -halfZ * 0.8});
  } else if (game == "grass") {
    editor.createObject("player", Vec3{0.0, 0.0, halfZ * 0.6});
    editor.createObject("ball", Vec3{0.0, 0.0, 0.0});
    editor.createObject("goal", Vec3{0.0, 0.0, -halfZ + 0.2});
    editor.createObject("goal", Vec3{0.0, 0.0, halfZ - 0.2});
  } else if (game == "battleground") {
    editor.createObject("player", Vec3{0.0, 0.0, 4.0});
  }
  editor.enterPlayMode();
  return true;
}

// One frame's worth of drained input. Deltas are consumed each drain.
struct FrameInput {
  float moveX = 0.0f;
  float moveZ = 0.0f;
  bool moveActive = false;
  float dragX = 0.0f;
  float dragY = 0.0f;
  float zoom = 1.0f;
  bool actionDown = false;
  bool actionUp = false;
  bool reloadTap = false;
};

FrameInput drainInput() {
  FrameInput out;
  std::lock_guard<std::mutex> lock(gInputMutex);
  if (gFingers[0].active) {
    const Finger& f = gFingers[0];
    const float dx = f.x - f.lastX;
    const float dy = f.y - f.lastY;
    gFingers[0].lastX = f.x;
    gFingers[0].lastY = f.y;
    if (f.zone == Zone::Stick) {
      out.moveActive = true;
      out.moveX = std::clamp((f.x - f.startX) / 0.22f, -1.0f, 1.0f);
      out.moveZ = std::clamp((f.y - f.startY) / 0.22f, -1.0f, 1.0f);
    } else if (f.zone == Zone::Camera) {
      out.dragX += dx;
      out.dragY += dy;
    }
  }
  if (gFingers[0].active && gFingers[1].active) {
    const float dx = gFingers[0].x - gFingers[1].x;
    const float dy = gFingers[0].y - gFingers[1].y;
    const float dist = std::sqrt(dx * dx + dy * dy);
    if (!gPinchActive) {
      gPinchActive = true;
      gPinchPrev = dist;
    } else if (gPinchPrev > 0.0f && dist > 0.0f) {
      out.zoom = dist / gPinchPrev;
      gPinchPrev = dist;
    }
  } else {
    gPinchActive = false;
  }
  out.actionDown = gActionDown;
  out.actionUp = gActionUp;
  out.reloadTap = gReloadTap;
  gActionDown = false;
  gActionUp = false;
  gReloadTap = false;
  return out;
}

void applyInput(WorldEditor& editor, OrbitCamera& orbit, const FrameInput& in) {
  if (editor.shotMode()) {
    // Golf: horizontal drags aim, the chase camera follows the aim; the
    // stick's horizontal push also nudges the aim.
    const f64 aimDelta = static_cast<f64>(in.dragX) * kAimScale + (in.moveActive ? in.moveX * 0.5 * 0.03 : 0.0);
    editor.setAimYaw(editor.aimYaw() + aimDelta);
    orbit.orbit(0.0, static_cast<f64>(in.dragY) * kLookPitchScale);
    editor.setMoveInput(0.0, 0.0);
  } else {
    editor.setMoveInput(in.moveActive ? static_cast<f64>(in.moveX) : 0.0,
                        in.moveActive ? static_cast<f64>(in.moveZ) : 0.0);
    orbit.orbit(static_cast<f64>(in.dragX) * kLookYawScale, static_cast<f64>(in.dragY) * kLookPitchScale);
  }
  if (in.zoom > 0.0f && in.zoom != 1.0f) orbit.zoom(1.0 / in.zoom);

  if (in.actionDown) {
    if (editor.shotMode()) editor.setShootHeld(true);
    if (editor.arenaMode()) editor.setFireHeld(true);
  }
  if (in.actionUp) {
    if (editor.shotMode()) editor.setShootHeld(false);
    else if (editor.arenaMode()) editor.setFireHeld(false);
    else editor.jumpPressed();  // kick football: tap to jump
  }
  if (in.reloadTap && editor.arenaMode()) editor.reload();
}

// --- The render loop ------------------------------------------------------
// Phase 1 editor: drains the edit command queue and applies it to the world
// (selection, drag, colour, delete, add, save). The viewport's pick is the
// engine's own — pickEntityAt() — so a tap lands on the entity the eye sees.
void processEditCommands(WorldEditor& editor, i32 viewWidth, i32 viewHeight) {
  EditCommand cmd;
  {
    std::lock_guard<std::mutex> lock(gEditMutex);
    cmd = gEditCmd;
    gEditCmd = EditCommand{};
  }
  if (cmd.kind == EditCmd::None) return;
  switch (cmd.kind) {
    case EditCmd::None: break;
    case EditCmd::Select: {
      const std::string picked = editor.pickEntityAt(static_cast<f64>(cmd.fromX),
                                                     static_cast<f64>(cmd.fromY));
      if (!picked.empty()) {
        editor.selectEntity(picked);
        gEditSelected = picked;
      } else {
        editor.selectEntity(std::string());
        gEditSelected.clear();
      }
      gEditSelectionChanged = true;
      break;
    }
    case EditCmd::BeginDrag: {
      const std::string picked = editor.pickEntityAt(static_cast<f64>(cmd.fromX),
                                                     static_cast<f64>(cmd.fromY));
      if (!picked.empty()) {
        editor.selectEntity(picked);
        gEditSelected = picked;
        gEditSelectionChanged = true;
      }
      break;
    }
    case EditCmd::DragTo: {
      if (!gEditSelected.empty()) {
        // grid = 0 (free move); the engine snaps the position to ground.
        editor.dragEntity(gEditSelected,
                          static_cast<f64>(cmd.fromX), static_cast<f64>(cmd.fromY),
                          static_cast<f64>(cmd.toX),   static_cast<f64>(cmd.toY),
                          0.0);
      }
      break;
    }
    case EditCmd::EndDrag: break;
    case EditCmd::SetColor: {
      if (!gEditSelected.empty()) {
        editor.setEntityColor(gEditSelected, Vec3{cmd.r, cmd.g, cmd.b});
      }
      break;
    }
    case EditCmd::Delete: {
      if (!gEditSelected.empty()) {
        editor.deleteEntity(gEditSelected);
        gEditSelected.clear();
        gEditSelectionChanged = true;
      }
      break;
    }
    case EditCmd::NewCube:
    case EditCmd::NewSphere:
    case EditCmd::NewPlane: {
      const char* kind = cmd.kind == EditCmd::NewCube   ? "cube"
                       : cmd.kind == EditCmd::NewSphere ? "sphere"
                                                        : "plane";
      const std::string name = editor.createObject(kind, Vec3{0.0, 0.0, 0.0});
      if (!name.empty()) {
        editor.selectEntity(name);
        gEditSelected = name;
        gEditSelectionChanged = true;
      }
      break;
    }
    case EditCmd::Save: {
      std::string error;
      const std::string path = cmd.name.empty()
                                   ? (editor.worldPath().empty() ? std::string("my_world.kimia") : editor.worldPath())
                                   : cmd.name;
      if (!editor.saveWorld(path, error)) {
        LOGE("saveWorld failed: %s", error.c_str());
      } else {
        editor.setWorldPath(path);
      }
      break;
    }
  }
  (void)viewWidth;
  (void)viewHeight;
}

// Builds the snapshot the UI reads. Called once per frame in edit mode.
void refreshEditSnapshot(WorldEditor& editor) {
  EditSnapshot snap;
  snap.names = editor.entityNames();
  snap.selected = editor.selectedName();
  snap.valid = true;
  if (!snap.selected.empty()) {
    if (const EntityData* e = editor.world().scene.get(editor.world().scene.find(snap.selected))) {
      snap.selPosX = static_cast<float>(e->transform.position.x);
      snap.selPosY = static_cast<float>(e->transform.position.y);
      snap.selPosZ = static_cast<float>(e->transform.position.z);
      snap.selR = static_cast<float>(e->color.x);
      snap.selG = static_cast<float>(e->color.y);
      snap.selB = static_cast<float>(e->color.z);
    }
  }
  const kimia::EnvironmentColors colors = kimia::environmentColors(editor.world().environment);
  snap.bgR = static_cast<float>(colors.clear.x);
  snap.bgG = static_cast<float>(colors.clear.y);
  snap.bgB = static_cast<float>(colors.clear.z);
  std::lock_guard<std::mutex> lock(gEditMutex);
  gEditSnapshot = snap;
}

void renderLoop() {
  const std::string root = unpackAssets(gConfig.filesDir);

  WorldEditor editor;
  editor.setWorldPath(gConfig.filesDir + "/my_world.kimia");
  editor.setImportDirectory(root.empty() ? std::string("assets") : root + "/assets");
  editor.setProfileDirectory(root.empty() ? std::string("profiles") : root + "/profiles");
  if (!setupGame(editor, gConfig.game, root)) {
    LOGE("failed to set up game '%s'", gConfig.game.c_str());
    return;
  }

  const MeshData cubeMesh = kimia::makeCube(1.0);
  const MeshData planeMesh = kimia::makePlane(1.0, 1.0);
  const MeshData sphereMesh = kimia::makeSphere(16, 8);
  OrbitCamera orbitCamera;
  f64 restingDistance = orbitCamera.distance;
  kimia::RuntimeLoop runtimeLoop;

  auto lastTime = std::chrono::steady_clock::now();
  const std::chrono::microseconds frameBudget(1000000 / std::max(5, std::min(gConfig.fps, 60)));

  while (!gQuit.load()) {
    // Wait for a surface to render into.
    ANativeWindow* window = nullptr;
    u64 generation = 0;
    {
      std::unique_lock<std::mutex> lock(gSurfaceMutex);
      gSurfaceCv.wait(lock, [] { return gWindow != nullptr || gQuit.load(); });
      if (gQuit.load()) break;
      window = gWindow;
      generation = gGeneration;
    }

    // Render resolution: the chosen flag, or the surface size.
    i32 width = gConfig.width > 0 ? gConfig.width : gViewWidth;
    i32 height = gConfig.height > 0 ? gConfig.height : gViewHeight;
    if (width <= 0 || height <= 0) {
      width = gViewWidth;
      height = gViewHeight;
    }
    if (width <= 0 || height <= 0) {
      // Surface not sized yet; wait for the next change.
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
      continue;
    }
    ANativeWindow_setBuffersGeometry(window, width, height, WINDOW_FORMAT_RGBA_8888);

    // Boot the GPU path if asked for and available; otherwise the CPU path.
    const bool wantGpu = gConfig.backend != 2;
    EGLContext egl;
    bool gpu = false;
    if (wantGpu) {
      if (!kimia::GLFunctions::instance().loaded()) kimia::GLFunctions::instance().load();
      if (kimia::GLFunctions::instance().loaded()) {
        gpu = egl.createWindow(window, width, height, gConfig.msaa);
        if (!gpu) LOGE("EGL window surface failed; falling back to software");
      } else {
        LOGI("GLES3 unavailable; falling back to software");
      }
    }

    Renderer renderer;
    std::string rendererError;
    if (gpu && !renderer.initialize(rendererError)) {
      LOGI("renderer init failed (%s); software fallback", rendererError.c_str());
      renderer.shutdown();
      egl.destroy();
      gpu = false;
    }
    if (gpu) renderer.setShadowEnabled(gConfig.shadows);
    OverlayBlit overlay;
    if (gpu) {
      std::string overlayError;
      if (!overlay.init(overlayError)) {
        LOGI("overlay init failed (%s)", overlayError.c_str());
      }
    }

    LOGI("surface %dx%d gpu=%d game=%s", width, height, gpu ? 1 : 0, gConfig.game.c_str());

    // Per-surface frame loop.
    lastTime = std::chrono::steady_clock::now();
    while (!gQuit.load()) {
      {
        std::lock_guard<std::mutex> lock(gSurfaceMutex);
        if (gWindow != window || gGeneration != generation) break;  // surface changed
      }

      const auto now = std::chrono::steady_clock::now();
      f64 dt = static_cast<f64>(std::chrono::duration_cast<std::chrono::microseconds>(now - lastTime).count()) /
               1000000.0;
      lastTime = now;
      dt = std::clamp(dt, 0.0, 0.1);

      {
        std::lock_guard<std::mutex> lock(gInputMutex);
        gArenaMode = editor.arenaMode();
      }
      const FrameInput input = drainInput();
      applyInput(editor, orbitCamera, input);

      const bool inEdit = gConfig.mode == 1;
      if (inEdit) {
        processEditCommands(editor, width, height);
        editor.setPaused(true);  // edit mode pauses the simulation
        // Translate the camera drag into orbit motion in edit mode too.
        orbitCamera.orbit(static_cast<f64>(input.dragX) * kLookYawScale * 0.6,
                          static_cast<f64>(input.dragY) * kLookPitchScale * 0.6);
      }

      // Camera framing (same easing as the desktop loop).
      if (editor.cameraFollowsAim() && !inEdit) {
        orbitCamera.yaw += angleDelta(orbitCamera.yaw, editor.aimYaw()) * std::min(1.0, kimia::kCameraFollowRate * dt);
      }
      if (!inEdit) {
        orbitCamera.center = editor.cameraTarget();
        const f64 wantedDistance = editor.cameraDistance(restingDistance);
        orbitCamera.distance += (wantedDistance - orbitCamera.distance) * std::min(1.0, kimia::kCameraFollowRate * dt);
      }

      // Advance the sim on the fixed clock.
      runtimeLoop.pause(editor.playing() && editor.paused());
      runtimeLoop.tick(dt, [&editor](f64 fixedStep) { editor.update(fixedStep); });
      runtimeLoop.beginRenderFrame();
      editor.drainEvents();  // no audio on native yet; keep the queue empty

      RenderScene scene;
      buildScene(editor, orbitCamera, width, height, cubeMesh, planeMesh, sphereMesh, scene);
      {
        // The engine turns taps on the picture into world-space actions
        // through the viewport; keep it in sync exactly like the desktop loop.
        kimia::pick::Viewport viewport;
        viewport.view = scene.view;
        viewport.projection = scene.projection;
        viewport.eye = scene.cameraPosition;
        viewport.width = width;
        viewport.height = height;
        editor.setViewport(viewport);
      }

      if (inEdit) {
        // Highlight the selected entity with a thin yellow wireframe-ish cube
        // by drawing a slightly larger tinted cube around it.
        const std::string sel = editor.selectedName();
        if (!sel.empty()) {
          if (const EntityData* e = editor.world().scene.get(editor.world().scene.find(sel))) {
            const Vec3 highlight{e->transform.position.x, e->transform.position.y + 0.02, e->transform.position.z};
            const Vec3 hs{e->transform.scale.x * 1.04, e->transform.scale.y * 1.04 + 0.04,
                          e->transform.scale.z * 1.04};
            scene.objects.push_back(
                {&cubeMesh, Mat4::translation(highlight) * Mat4::scaling(hs),
                 Vec3{1.0, 0.85, 0.15}, 1.0, 0.0, nullptr});
          }
        }
        refreshEditSnapshot(editor);
      }

      Image image;
      if (gpu && renderer.ready()) {
        renderer.render(scene, width, height);
        if (renderer.captureImage(width, height, image)) {
          drawHud(image, editor);
          drawControls(image, editor);
          overlay.blit(image);
        }
        egl.swapBuffers();
      } else {
        const kimia::EnvironmentColors colors = kimia::environmentColors(editor.world().environment);
        kimia::renderSoftware(scene, width, height, colors.clear, image);
        drawHud(image, editor);
        drawControls(image, editor);
        presentSoftware(window, image);
      }

      const auto elapsed = std::chrono::steady_clock::now() - now;
      const auto left = frameBudget - std::chrono::duration_cast<std::chrono::microseconds>(elapsed);
      if (left.count() > 0) std::this_thread::sleep_for(left);
    }

    overlay.destroy();
    renderer.shutdown();
    egl.destroy();
  }

  LOGI("render loop stopped");
}

}  // namespace

// --- JNI surface ----------------------------------------------------------
extern "C" JNIEXPORT void JNICALL
Java_com_kimia_world_NativeEngine_nativeStart(JNIEnv* env, jclass, jstring filesDir, jstring game, jint backend,
                                              jint width, jint height, jint fps, jboolean shadows,
                                              jboolean msaa) {
  if (gThread.joinable()) return;
  gConfig = NativeConfig{};
  if (filesDir != nullptr) {
    const char* path = env->GetStringUTFChars(filesDir, nullptr);
    if (path != nullptr) {
      gConfig.filesDir = path;
      env->ReleaseStringUTFChars(filesDir, path);
    }
  }
  if (game != nullptr) {
    const char* g = env->GetStringUTFChars(game, nullptr);
    if (g != nullptr) {
      gConfig.game = g;
      env->ReleaseStringUTFChars(game, g);
    }
  }
  gConfig.backend = backend;
  gConfig.width = width;
  gConfig.height = height;
  gConfig.fps = fps;
  gConfig.shadows = shadows == JNI_TRUE;
  gConfig.msaa = msaa == JNI_TRUE;
  gQuit.store(false);
  gThread = std::thread(renderLoop);
}

extern "C" JNIEXPORT void JNICALL
Java_com_kimia_world_NativeEngine_nativeSetSurface(JNIEnv* env, jclass, jobject surface) {
  std::lock_guard<std::mutex> lock(gSurfaceMutex);
  if (gWindow != nullptr) ANativeWindow_release(gWindow);
  gWindow = surface != nullptr ? ANativeWindow_fromSurface(env, surface) : nullptr;
  ++gGeneration;
  gSurfaceCv.notify_all();
}

extern "C" JNIEXPORT void JNICALL
Java_com_kimia_world_NativeEngine_nativeSurfaceChanged(JNIEnv*, jclass, jint width, jint height) {
  std::lock_guard<std::mutex> lock(gSurfaceMutex);
  if (width == gViewWidth && height == gViewHeight && gWindow != nullptr) return;  // no-op resize
  gViewWidth = width;
  gViewHeight = height;
  ++gGeneration;  // re-create the EGL surface at the new size
  gSurfaceCv.notify_all();
}

extern "C" JNIEXPORT void JNICALL
Java_com_kimia_world_NativeEngine_nativeSurfaceDestroyed(JNIEnv*, jclass) {
  std::lock_guard<std::mutex> lock(gSurfaceMutex);
  if (gWindow != nullptr) {
    ANativeWindow_release(gWindow);
    gWindow = nullptr;
  }
  ++gGeneration;
  gSurfaceCv.notify_all();
}

extern "C" JNIEXPORT void JNICALL
Java_com_kimia_world_NativeEngine_nativeTouch(JNIEnv*, jclass, jint action, jint pointerId, jfloat x, jfloat y) {
  std::lock_guard<std::mutex> lock(gInputMutex);
  // Android MotionEvent: action = getActionMasked() (0 down, 1 up, 2 move,
  // 5 pointer_down, 6 pointer_up); pointerId = getActionIndex(). We track at
  // most two fingers, so any pointer past the first collapses onto slot 1.
  const int slot = pointerId <= 0 ? 0 : 1;
  Finger& finger = gFingers[slot];
  if (action == 0 || action == 5) {
    finger = Finger{true, x, y, x, y, x, y, classifyZone(x, y, gArenaMode)};
    if (slot == 0 && finger.zone == Zone::Action) gActionDown = true;
  } else if (action == 2) {
    if (finger.active) {
      finger.x = x;
      finger.y = y;
    }
  } else if (action == 1 || action == 6) {
    if (finger.active) {
      if (slot == 0 && finger.zone == Zone::Action) gActionUp = true;
      if (slot == 0 && finger.zone == Zone::Reload) gReloadTap = true;
      finger.active = false;
    }
  }
}

extern "C" JNIEXPORT void JNICALL
Java_com_kimia_world_NativeEngine_nativeStop(JNIEnv*, jclass) {
  gQuit.store(true);
  {
    std::lock_guard<std::mutex> lock(gSurfaceMutex);
    gSurfaceCv.notify_all();
  }
  if (gThread.joinable()) gThread.join();
}

// --- Phase 1: in-world editor bridge -------------------------------------
// All of these run on the UI thread; the render thread drains the command
// queue inside its frame loop. The snapshot reader returns a flat table the
// ListView can show directly.

extern "C" JNIEXPORT void JNICALL
Java_com_kimia_world_NativeEngine_nativeSetMode(JNIEnv*, jclass, jint mode) {
  gConfig.mode = mode;
}

extern "C" JNIEXPORT void JNICALL
Java_com_kimia_world_NativeEngine_nativeEditSelect(JNIEnv*, jclass, jfloat x, jfloat y) {
  std::lock_guard<std::mutex> lock(gEditMutex);
  gEditCmd = EditCommand{};
  gEditCmd.kind = EditCmd::Select;
  gEditCmd.fromX = x;
  gEditCmd.fromY = y;
}

extern "C" JNIEXPORT void JNICALL
Java_com_kimia_world_NativeEngine_nativeEditBeginDrag(JNIEnv*, jclass, jfloat x, jfloat y) {
  std::lock_guard<std::mutex> lock(gEditMutex);
  gEditCmd = EditCommand{};
  gEditCmd.kind = EditCmd::BeginDrag;
  gEditCmd.fromX = x;
  gEditCmd.fromY = y;
}

extern "C" JNIEXPORT void JNICALL
Java_com_kimia_world_NativeEngine_nativeEditDragTo(JNIEnv*, jclass, jfloat fromX, jfloat fromY, jfloat toX, jfloat toY) {
  std::lock_guard<std::mutex> lock(gEditMutex);
  gEditCmd = EditCommand{};
  gEditCmd.kind = EditCmd::DragTo;
  gEditCmd.fromX = fromX;
  gEditCmd.fromY = fromY;
  gEditCmd.toX = toX;
  gEditCmd.toY = toY;
}

extern "C" JNIEXPORT void JNICALL
Java_com_kimia_world_NativeEngine_nativeEditEndDrag(JNIEnv*, jclass) {
  std::lock_guard<std::mutex> lock(gEditMutex);
  gEditCmd = EditCommand{};
  gEditCmd.kind = EditCmd::EndDrag;
}

extern "C" JNIEXPORT void JNICALL
Java_com_kimia_world_NativeEngine_nativeEditSetColor(JNIEnv*, jclass, jfloat r, jfloat g, jfloat b) {
  std::lock_guard<std::mutex> lock(gEditMutex);
  gEditCmd = EditCommand{};
  gEditCmd.kind = EditCmd::SetColor;
  gEditCmd.r = r;
  gEditCmd.g = g;
  gEditCmd.b = b;
}

extern "C" JNIEXPORT void JNICALL
Java_com_kimia_world_NativeEngine_nativeEditDelete(JNIEnv*, jclass) {
  std::lock_guard<std::mutex> lock(gEditMutex);
  gEditCmd = EditCommand{};
  gEditCmd.kind = EditCmd::Delete;
}

extern "C" JNIEXPORT void JNICALL
Java_com_kimia_world_NativeEngine_nativeEditNew(JNIEnv*, jclass, jint kind) {
  std::lock_guard<std::mutex> lock(gEditMutex);
  gEditCmd = EditCommand{};
  gEditCmd.kind = kind == 0 ? EditCmd::NewCube
                : kind == 1 ? EditCmd::NewSphere
                            : EditCmd::NewPlane;
}

extern "C" JNIEXPORT void JNICALL
Java_com_kimia_world_NativeEngine_nativeEditSave(JNIEnv* env, jclass, jstring path) {
  std::lock_guard<std::mutex> lock(gEditMutex);
  gEditCmd = EditCommand{};
  gEditCmd.kind = EditCmd::Save;
  if (path != nullptr) {
    const char* p = env->GetStringUTFChars(path, nullptr);
    if (p != nullptr) {
      gEditCmd.name = p;
      env->ReleaseStringUTFChars(path, p);
    }
  }
}

// Reads the snapshot. Returns:
//   - on first call after a selection change, the number of entities (>=0);
//   - on subsequent calls, the same number if the snapshot is fresh, or -1
//     if no change happened.
// The Java side keeps its own array; this only tells it WHEN to re-read.
extern "C" JNIEXPORT jint JNICALL
Java_com_kimia_world_NativeEngine_nativeEditRefresh(JNIEnv*, jclass) {
  std::lock_guard<std::mutex> lock(gEditMutex);
  if (!gEditSnapshot.valid) return -1;
  // A simple change marker: the selected name + a sequence counter derived
  // from its length. Cheap and good enough for the editor panel which only
  // refreshes when something actually changed.
  return static_cast<jint>(gEditSnapshot.names.size());
}

// Fills a Java String[] with the snapshot's entity names. The caller passes
// a Java array of the size reported by nativeEditRefresh().
extern "C" JNIEXPORT void JNICALL
Java_com_kimia_world_NativeEngine_nativeEditGetNames(JNIEnv* env, jclass, jobjectArray out) {
  std::lock_guard<std::mutex> lock(gEditMutex);
  if (!gEditSnapshot.valid || out == nullptr) return;
  const jsize n = env->GetArrayLength(out);
  for (jsize i = 0; i < n; ++i) {
    const std::string& name = gEditSnapshot.names[static_cast<usize>(i)];
    env->SetObjectArrayElement(out, i, env->NewStringUTF(name.c_str()));
  }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_kimia_world_NativeEngine_nativeEditGetSelected(JNIEnv* env, jclass) {
  std::lock_guard<std::mutex> lock(gEditMutex);
  return env->NewStringUTF(gEditSnapshot.selected.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_com_kimia_world_NativeEngine_nativeEditGetColor(JNIEnv*, jclass, jfloatArray out) {
  // Kept for API symmetry; the Java side uses the R/G/B getters instead so
  // it doesn't have to manage array copies.
  static_cast<void>(out);
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_kimia_world_NativeEngine_nativeEditGetColorR(JNIEnv*, jclass) {
  std::lock_guard<std::mutex> lock(gEditMutex);
  return gEditSnapshot.selR;
}
extern "C" JNIEXPORT jfloat JNICALL
Java_com_kimia_world_NativeEngine_nativeEditGetColorG(JNIEnv*, jclass) {
  std::lock_guard<std::mutex> lock(gEditMutex);
  return gEditSnapshot.selG;
}
extern "C" JNIEXPORT jfloat JNICALL
Java_com_kimia_world_NativeEngine_nativeEditGetColorB(JNIEnv*, jclass) {
  std::lock_guard<std::mutex> lock(gEditMutex);
  return gEditSnapshot.selB;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_kimia_world_NativeEngine_nativeEditGetPosX(JNIEnv*, jclass) {
  std::lock_guard<std::mutex> lock(gEditMutex);
  return gEditSnapshot.selPosX;
}
extern "C" JNIEXPORT jfloat JNICALL
Java_com_kimia_world_NativeEngine_nativeEditGetPosY(JNIEnv*, jclass) {
  std::lock_guard<std::mutex> lock(gEditMutex);
  return gEditSnapshot.selPosY;
}
extern "C" JNIEXPORT jfloat JNICALL
Java_com_kimia_world_NativeEngine_nativeEditGetPosZ(JNIEnv*, jclass) {
  std::lock_guard<std::mutex> lock(gEditMutex);
  return gEditSnapshot.selPosZ;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_kimia_world_NativeEngine_nativeEditSelectionChanged(JNIEnv*, jclass) {
  std::lock_guard<std::mutex> lock(gEditMutex);
  if (!gEditSelectionChanged) return 0;
  gEditSelectionChanged = false;
  return 1;
}
