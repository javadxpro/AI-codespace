cmake_minimum_required(VERSION 3.16)
# Engine version = MAJOR.MINOR.PATCH, bumped by the size of the change (bug
# fix / small: +0.0.1, stage / big: +0.1.0), never by the date. The same
# literal lives in Engine/Core/include/kimia/Version.h; the configure step
# below and a unit test keep the two identical. Bump both + add the entry at
# the top of CHANGELOG.md per release.
project(KIMIA VERSION 0.30.0 LANGUAGES C CXX)
file(STRINGS Engine/Core/include/kimia/Version.h KIMIA_VERSION_LINE REGEX "#define KIMIA_ENGINE_VERSION \"")
string(REGEX REPLACE ".*\"([0-9.]+)\".*" "\\1" KIMIA_HEADER_VERSION "${KIMIA_VERSION_LINE}")
if(NOT KIMIA_HEADER_VERSION STREQUAL PROJECT_VERSION)
  message(FATAL_ERROR "Version.h says ${KIMIA_HEADER_VERSION} but CMakeLists.txt says ${PROJECT_VERSION}")
endif()
message(STATUS "KIMIA engine version ${PROJECT_VERSION}")

option(KIMIA_WERROR "Treat warnings as errors" ON)
option(KIMIA_BUILD_TOOLS "Build the kimia_assets CLI tool" ON)
if(WIN32)
  # The current aggregate tests contain Unix-only test clients. The PC runtime
  # is buildable by default; the test port is a separate migration task and
  # can be requested explicitly with -DKIMIA_BUILD_TESTS=ON.
  option(KIMIA_BUILD_TESTS "Build the aggregate test executable" OFF)
else()
  option(KIMIA_BUILD_TESTS "Build the aggregate test executable" ON)
endif()
option(KIMIA_ENABLE_SDL2 "Use SDL2 for the window backend when available" ON)
option(KIMIA_BUILD_PC "Build the Windows desktop path when targeting Windows" ON)
option(KIMIA_ENABLE_D3D11 "Build the Direct3D 11 renderer on Windows" ON)
# Explicit opt-in for the Android APK build. Deliberately NOT gated on the
# CMake `ANDROID` variable: Termux also defines ANDROID but must keep building
# only the plain headless executables. Only Android/app/build.gradle turns
# this on.
option(KIMIA_BUILD_ANDROID_JNI "Build the JNI shared library for the Android APK" OFF)
# Self-contained single-file builds: embed Profiles/Worlds/Branding into the
# binary and unpack them at runtime, so a lone .exe/.apk runs with nothing
# next to it. Needs a Python interpreter at build time.
option(KIMIA_EMBED_ASSETS "Embed game assets into the binary (self-contained exe/apk)" OFF)
set(KIMIA_RENDER_BACKEND "AUTO" CACHE STRING "Primary renderer: AUTO, D3D11, OPENGL or SOFTWARE")
set_property(CACHE KIMIA_RENDER_BACKEND PROPERTY STRINGS AUTO D3D11 OPENGL SOFTWARE)
string(TOUPPER "${KIMIA_RENDER_BACKEND}" KIMIA_RENDER_BACKEND_NORMALIZED)
if(NOT KIMIA_RENDER_BACKEND_NORMALIZED MATCHES "^(AUTO|D3D11|OPENGL|SOFTWARE)$")
  message(FATAL_ERROR "KIMIA_RENDER_BACKEND must be AUTO, D3D11, OPENGL or SOFTWARE")
endif()
if(KIMIA_RENDER_BACKEND_NORMALIZED STREQUAL "D3D11" AND NOT WIN32)
  message(FATAL_ERROR "KIMIA_RENDER_BACKEND=D3D11 requires a Windows build")
endif()
if(KIMIA_RENDER_BACKEND_NORMALIZED STREQUAL "D3D11" AND NOT KIMIA_ENABLE_D3D11)
  message(FATAL_ERROR "KIMIA_RENDER_BACKEND=D3D11 requires KIMIA_ENABLE_D3D11=ON")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)

# The JNI target is a shared library that links the static kimia_* archives.
# Those must be position-independent to link into a .so on arm64; the
# executables don't care either way.
if(KIMIA_BUILD_ANDROID_JNI)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# No build type given (the usual `cmake -B build` on a phone) -> Release: the
# software rasterizer draws every frame on the CPU, and an unoptimized build
# is several times slower there. Pass -DCMAKE_BUILD_TYPE=Debug to override.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (Release by default)" FORCE)
endif()
message(STATUS "KIMIA build type: ${CMAKE_BUILD_TYPE}, compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

# Keep the strict gate on GCC/Clang, but make the same project consumable by
# the Windows/MSVC toolchain selected for the PC rebuild. MSVC does not accept
# GCC's -W* switches; /permissive- and /W4 are the closest useful baseline.
if(MSVC)
  set(KIMIA_STRICT_WARNINGS /W4 /permissive- /utf-8)
  set(KIMIA_WERROR_FLAGS /WX)
else()
  # -Wsign-conversion is listed explicitly: GCC's -Wconversion does not imply
  # it in C++ but Clang's does.
  set(KIMIA_STRICT_WARNINGS -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
                            -Wdouble-promotion)
  set(KIMIA_WERROR_FLAGS -Werror)
endif()
# Binaries live in <build>/bin (the paths the engine spec's gates use).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Threads REQUIRED)

# --- SDL2 (optional window backend) ---
set(KIMIA_HAS_SDL2 OFF)
set(KIMIA_SDL_TARGET "")
set(KIMIA_SDL_INCLUDE_DIRS "")
if(KIMIA_ENABLE_SDL2)
  find_package(SDL2 CONFIG QUIET)
  if(TARGET SDL2::SDL2-static)
    set(KIMIA_SDL_TARGET SDL2::SDL2-static)
    set(KIMIA_HAS_SDL2 ON)
  elseif(TARGET SDL2::SDL2)
    set(KIMIA_SDL_TARGET SDL2::SDL2)
    set(KIMIA_HAS_SDL2 ON)
  else()
    find_package(SDL2 MODULE QUIET)
    if(SDL2_FOUND)
      set(KIMIA_SDL_TARGET ${SDL2_LIBRARIES})
      set(KIMIA_SDL_INCLUDE_DIRS ${SDL2_INCLUDE_DIRS})
      set(KIMIA_HAS_SDL2 ON)
    endif()
  endif()
endif()
if(KIMIA_HAS_SDL2)
  message(STATUS "SDL2 found: window backend enabled")
elseif(KIMIA_ENABLE_SDL2)
  message(STATUS "SDL2 not found: headless-only (WebViewer display)")
else()
  message(STATUS "SDL2 disabled by KIMIA_ENABLE_SDL2=OFF: headless-only (WebViewer display)")
endif()
if(WIN32 AND KIMIA_BUILD_PC AND NOT KIMIA_HAS_SDL2)
  message(FATAL_ERROR
      "KIMIA_BUILD_PC requires SDL2. Install the SDL2 development package or configure with -DKIMIA_BUILD_PC=OFF.")
endif()

add_library(kimia_core
    Engine/Core/src/Log.cpp
    Engine/Core/src/Profiler.cpp
    Engine/Core/src/Time.cpp
)
target_include_directories(kimia_core PUBLIC Engine/Core/include)
target_compile_options(kimia_core PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_core PRIVATE ${KIMIA_WERROR_FLAGS})
endif()

# Portable runtime clock shared by the future PC game loop and the editor.
# Keeping it outside WorldEditor is the first step in separating engine state
# from the reference games.
add_library(kimia_runtime STATIC
    Engine/Runtime/src/RuntimeLoop.cpp
)
target_include_directories(kimia_runtime PUBLIC Engine/Runtime/include)
target_link_libraries(kimia_runtime PUBLIC kimia_core)
target_compile_options(kimia_runtime PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_runtime PRIVATE ${KIMIA_WERROR_FLAGS})
endif()

add_library(kimia_math INTERFACE)
target_include_directories(kimia_math INTERFACE Engine/Math/include Engine/Core/include)

add_library(kimia_scene STATIC
    Engine/Scene/src/Scene.cpp
    Engine/Scene/src/SceneIO.cpp
)
target_include_directories(kimia_scene PUBLIC Engine/Scene/include)
target_link_libraries(kimia_scene PUBLIC kimia_math)
target_compile_options(kimia_scene PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_scene PRIVATE ${KIMIA_WERROR_FLAGS})
endif()

add_library(kimia_physics STATIC
    Engine/Physics/src/Physics.cpp
)
target_include_directories(kimia_physics PUBLIC Engine/Physics/include)
target_link_libraries(kimia_physics PUBLIC kimia_core kimia_math)
target_compile_options(kimia_physics PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_physics PRIVATE ${KIMIA_WERROR_FLAGS})
endif()

# --- Game profiles (the data that turns the engine into one game) ---
add_library(kimia_profile STATIC
    Engine/Profile/src/GameProfile.cpp
)
target_include_directories(kimia_profile PUBLIC Engine/Profile/include)
target_link_libraries(kimia_profile PUBLIC kimia_core)
target_compile_options(kimia_profile PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_profile PRIVATE ${KIMIA_WERROR_FLAGS})
endif()

# Vendored FBX loader (single-file library, upstream code).
add_library(kimia_ufbx STATIC ThirdParty/ufbx/ufbx.c)
target_include_directories(kimia_ufbx SYSTEM PUBLIC ThirdParty/ufbx)

# Vendored OGG/Vorbis decoder (upstream C code; strict C++ warnings don't apply).
add_library(kimia_stb_vorbis STATIC ThirdParty/stb/stb_vorbis_impl.c)
target_include_directories(kimia_stb_vorbis SYSTEM PUBLIC ThirdParty/stb)
# Clang (Termux) flags an upstream pointer-overflow guard in stb_vorbis.c as
# tautological; that is vendored code, so keep our zero-warning gate honest
# without patching the library.
target_compile_options(kimia_stb_vorbis PRIVATE $<$<C_COMPILER_ID:Clang>:-Wno-tautological-compare>)

add_library(kimia_assets STATIC
    Engine/Graphics/src/Image.cpp
    Engine/Graphics/src/Mesh.cpp
    Engine/Graphics/src/BitmapFont.cpp
    Engine/Graphics/src/Skeleton.cpp
    Engine/Graphics/src/Animator.cpp
    Engine/Assets/src/AssetPipeline.cpp
    Engine/Assets/src/Audio.cpp
)
target_include_directories(kimia_assets PUBLIC
    Engine/Graphics/include
    Engine/Assets/include
    Engine/Core/include
)
# SYSTEM: third-party headers are not ours to keep warning-clean, and with
# -Wsign-conversion on they would drown our own diagnostics in stb noise.
target_include_directories(kimia_assets SYSTEM PUBLIC
    ThirdParty/stb
    ThirdParty/dr
)
target_link_libraries(kimia_assets PUBLIC kimia_core kimia_math kimia_ufbx kimia_stb_vorbis)
target_compile_options(kimia_assets PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_assets PRIVATE ${KIMIA_WERROR_FLAGS})
endif()

if(KIMIA_BUILD_TESTS)
add_executable(kimia_tests
    Tests/src/main.cpp
    Tests/src/CoreTests.cpp
    Tests/src/MathTests.cpp
    Tests/src/GraphicsTests.cpp
    Tests/src/AssetsTests.cpp
    Tests/src/SceneTests.cpp
    Tests/src/PhysicsTests.cpp
    Tests/src/WebTests.cpp
    Tests/src/RendererTests.cpp
    Tests/src/GolfTests.cpp
    Tests/src/WorldTests.cpp
    Tests/src/RaytracerTests.cpp
    Tests/src/SkeletonTests.cpp
    Tests/src/StudioTests.cpp
    Tests/src/LogicTests.cpp
    Tests/src/PickingTests.cpp
    Tests/src/HudTests.cpp
    Tests/src/ParticleTests.cpp
    Tests/src/InputTests.cpp
    Tests/src/ProfileTests.cpp
    Tests/src/RuntimeTests.cpp
    Tests/src/EditorUITests.cpp
    Tests/src/GizmoTests.cpp
    Tests/src/UndoStackTests.cpp
    Tests/src/AssetBrowserTests.cpp
    Tests/src/TimelineTests.cpp
    Tests/src/PhysicsPanelTests.cpp
    Tests/src/ParticlePanelTests.cpp
    Tests/src/SceneFileTests.cpp
    Tests/src/SelectionTests.cpp
    Tests/src/LogPanelTests.cpp
    Tests/src/HelpPanelTests.cpp
    Tests/src/ObjectTreeTests.cpp
    Tests/src/PropertySheetTests.cpp
    Tests/src/CameraPanelTests.cpp
    Tests/src/SceneBrowserTests.cpp
    Tests/src/StatsPanelTests.cpp
    Tests/src/DialogPanelTests.cpp
    Tests/src/TooltipPanelTests.cpp
    Tests/src/VersionPanelTests.cpp
    Tests/src/InspectorPanelTests.cpp
    Tests/src/StatusBarTests.cpp
    Tests/src/ContextMenuPanelTests.cpp
    Tests/src/ProfilerPanelTests.cpp
    Tests/src/SearchPanelTests.cpp
    Tests/src/HierarchyTreePanelTests.cpp
    Tests/src/AnimationPanelTests.cpp
    Tests/src/AudioPanelTests.cpp
    Tests/src/LightingPanelTests.cpp
    Tests/src/TerrainPanelTests.cpp
    Tests/src/NavMeshPanelTests.cpp
    Tests/src/MaterialPanelTests.cpp
    Tests/src/EditorUIIntegrationTests.cpp
    Tests/src/RasterBridgeTests.cpp
    Tests/src/NativePainterTests.cpp
    Tests/src/AtlasBuilderTests.cpp
    Tests/src/EditorGlTests.cpp
)
target_include_directories(kimia_tests PRIVATE Tests/include Engine/Core/include)
target_link_libraries(kimia_tests PRIVATE kimia_core kimia_math kimia_assets kimia_scene kimia_physics kimia_profile kimia_app kimia_golf kimia_world kimia_raytracer kimia_editor_ui)
target_compile_options(kimia_tests PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_tests PRIVATE ${KIMIA_WERROR_FLAGS})
endif()
target_compile_definitions(kimia_tests PRIVATE
    KIMIA_CMAKE_VERSION="${PROJECT_VERSION}"
    KIMIA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    KIMIA_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Tests/assets"
    KIMIA_PROFILE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Profiles"
    KIMIA_TEST_TMP="${CMAKE_BINARY_DIR}/kimia_test_tmp"
)

endif()

# --- Raytracer (offline path tracer render preview) ---
add_library(kimia_raytracer STATIC
    Engine/Raytracer/src/Raytracer.cpp
)
target_include_directories(kimia_raytracer PUBLIC Engine/Raytracer/include)
target_link_libraries(kimia_raytracer PUBLIC kimia_world kimia_math)
target_compile_options(kimia_raytracer PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_raytracer PRIVATE ${KIMIA_WERROR_FLAGS})
endif()

# Tiny generator that writes the test assets (tone.wav, 2x3.png, 2x2.jpg,
# cube.obj, quad.obj) into Tests/assets. A PC release does not build or run
# test-asset tooling unless the aggregate tests were explicitly enabled.
if(KIMIA_BUILD_TESTS)
  add_executable(kimia_asset_gen Tools/src/asset_gen.cpp)
  target_link_libraries(kimia_asset_gen PRIVATE kimia_assets)
  target_compile_options(kimia_asset_gen PRIVATE ${KIMIA_STRICT_WARNINGS})
  if(KIMIA_WERROR)
    target_compile_options(kimia_asset_gen PRIVATE ${KIMIA_WERROR_FLAGS})
  endif()
  add_custom_target(kimia_generate_assets ALL
      COMMAND ${CMAKE_COMMAND} -E env
          KIMIA_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Tests/assets"
          $<TARGET_FILE:kimia_asset_gen>
      DEPENDS kimia_asset_gen
  )
  add_dependencies(kimia_tests kimia_generate_assets)
endif()

if(KIMIA_BUILD_TOOLS)
  add_executable(kimia_assets_cli Tools/src/kimia_assets.cpp)
  target_link_libraries(kimia_assets_cli PRIVATE kimia_assets)
  target_compile_options(kimia_assets_cli PRIVATE ${KIMIA_STRICT_WARNINGS})
  if(KIMIA_WERROR)
    target_compile_options(kimia_assets_cli PRIVATE ${KIMIA_WERROR_FLAGS})
  endif()

  add_executable(kimia_raytrace_cli Tools/src/kimia_raytrace.cpp)
  set_target_properties(kimia_raytrace_cli PROPERTIES OUTPUT_NAME kimia_raytrace)
  target_link_libraries(kimia_raytrace_cli PRIVATE kimia_raytracer kimia_world)
  target_compile_options(kimia_raytrace_cli PRIVATE ${KIMIA_STRICT_WARNINGS})
  if(KIMIA_WERROR)
    target_compile_options(kimia_raytrace_cli PRIVATE ${KIMIA_WERROR_FLAGS})
  endif()
endif()

# --- Renderer (D3D11 PC backend + GL/EGL + software fallback) ---
add_library(kimia_renderer STATIC
    Engine/Renderer/src/GLFunctions.cpp
    Engine/Renderer/src/EGL.cpp
    Engine/Renderer/src/Shader.cpp
    Engine/Renderer/src/GpuMesh.cpp
    Engine/Renderer/src/Shaders.cpp
    Engine/Renderer/src/Renderer.cpp
    Engine/Renderer/src/SoftwareRenderer.cpp
    Engine/Renderer/src/D3D11Renderer.cpp
)
target_include_directories(kimia_renderer PUBLIC Engine/Renderer/include)
# dlopen/dlsym for the runtime GL loader (libdl on Android/bionic and old glibc).
target_link_libraries(kimia_renderer PUBLIC kimia_assets kimia_math ${CMAKE_DL_LIBS})
if(WIN32 AND KIMIA_ENABLE_D3D11)
  target_link_libraries(kimia_renderer PUBLIC d3d11 dxgi d3dcompiler)
  target_compile_definitions(kimia_renderer PUBLIC KIMIA_HAS_D3D11=1)
endif()
if(KIMIA_RENDER_BACKEND_NORMALIZED STREQUAL "D3D11")
  target_compile_definitions(kimia_renderer PUBLIC KIMIA_PRIMARY_RENDERER_D3D11=1)
elseif(KIMIA_RENDER_BACKEND_NORMALIZED STREQUAL "OPENGL")
  target_compile_definitions(kimia_renderer PUBLIC KIMIA_PRIMARY_RENDERER_OPENGL=1)
elseif(KIMIA_RENDER_BACKEND_NORMALIZED STREQUAL "SOFTWARE")
  target_compile_definitions(kimia_renderer PUBLIC KIMIA_PRIMARY_RENDERER_SOFTWARE=1)
else()
  target_compile_definitions(kimia_renderer PUBLIC KIMIA_PRIMARY_RENDERER_AUTO=1)
endif()
target_compile_options(kimia_renderer PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_renderer PRIVATE ${KIMIA_WERROR_FLAGS})
endif()

if(WIN32 AND KIMIA_BUILD_PC AND KIMIA_ENABLE_D3D11)
  add_executable(kimia_pc_d3d11_smoke Examples/PCD3D11Smoke.cpp)
  target_link_libraries(kimia_pc_d3d11_smoke PRIVATE kimia_renderer kimia_assets)
  target_compile_options(kimia_pc_d3d11_smoke PRIVATE ${KIMIA_STRICT_WARNINGS})
  if(KIMIA_WERROR)
    target_compile_options(kimia_pc_d3d11_smoke PRIVATE ${KIMIA_WERROR_FLAGS})
  endif()
endif()

# --- Platform (input + optional SDL2 window) ---
add_library(kimia_platform STATIC
    Engine/Platform/src/InputState.cpp
    Engine/Platform/src/Window.cpp
)
target_include_directories(kimia_platform PUBLIC Engine/Platform/include)
target_link_libraries(kimia_platform PUBLIC kimia_assets kimia_math)
if(KIMIA_HAS_SDL2)
  target_sources(kimia_platform PRIVATE Engine/Platform/src/SDLWindow.cpp)
  target_include_directories(kimia_platform PRIVATE ${KIMIA_SDL_INCLUDE_DIRS})
  target_link_libraries(kimia_platform PUBLIC ${KIMIA_SDL_TARGET})
  target_compile_definitions(kimia_platform PUBLIC KIMIA_HAS_SDL2=1)
endif()
target_compile_options(kimia_platform PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_platform PRIVATE ${KIMIA_WERROR_FLAGS})
endif()

# --- App (WebViewer + engine bootstrap) ---
add_library(kimia_app STATIC
    Engine/App/src/WebViewer.cpp
    Engine/App/src/Engine.cpp
)
target_include_directories(kimia_app PUBLIC Engine/App/include)
target_link_libraries(kimia_app PUBLIC kimia_platform kimia_renderer kimia_runtime Threads::Threads)
if(WIN32)
  target_link_libraries(kimia_app PUBLIC ws2_32)
endif()
target_compile_options(kimia_app PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_app PRIVATE ${KIMIA_WERROR_FLAGS})
endif()

# --- Examples ---
add_executable(kimia_hello Examples/HelloWindow.cpp)
target_link_libraries(kimia_hello PRIVATE kimia_app)
target_compile_options(kimia_hello PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_hello PRIVATE ${KIMIA_WERROR_FLAGS})
endif()

add_executable(kimia_first3d Examples/First3DScene.cpp)
target_link_libraries(kimia_first3d PRIVATE kimia_app)
target_compile_options(kimia_first3d PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_first3d PRIVATE ${KIMIA_WERROR_FLAGS})
endif()

add_executable(kimia_remote Examples/RemoteView.cpp)
target_link_libraries(kimia_remote PRIVATE kimia_app)
target_compile_options(kimia_remote PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_remote PRIVATE ${KIMIA_WERROR_FLAGS})
endif()

# --- World (option-driven editor + SceneIO-v1-compatible world files) ---
add_library(kimia_world STATIC
    Engine/World/src/World.cpp
    Engine/World/src/WorldBuilder.cpp
    Engine/World/src/Studio.cpp
    Engine/World/src/Logic.cpp
    Engine/World/src/Picking.cpp
    Engine/World/src/Library.cpp
    Engine/World/src/Hud.cpp
    Engine/World/src/Particles.cpp
    Engine/World/src/Assets.cpp
    Engine/World/src/Input.cpp
    Engine/World/src/WorldIO.cpp
)

# --- EditorUI (immediate-mode UI toolkit for the in-app editor) ---
# A native GLES-rendered UI that replaces the WebViewer Workbench. Phase 1
# keeps the renderer stubbed (CPU-side logic only); the Android JNI wires
# up the real GL path on top of the same code.
add_library(kimia_editor_ui STATIC
    Engine/EditorUI/src/EditorUI.cpp
    Engine/EditorUI/src/Widget.cpp
    Engine/EditorUI/src/Panel.cpp
    Engine/EditorUI/src/Theme.cpp
    Engine/EditorUI/src/Icons.cpp
    Engine/EditorUI/src/HostBridge.cpp
    Engine/EditorUI/src/RasterBridge.cpp
    Engine/EditorUI/src/NativePainter.cpp
    Engine/EditorUI/src/AtlasBuilder.cpp
    Engine/EditorUI/src/EditorGl.cpp
    Engine/EditorUI/src/UndoStack.cpp
    Engine/EditorUI/src/AssetBrowser.cpp
    Engine/EditorUI/src/Timeline.cpp
    Engine/EditorUI/src/PhysicsPanel.cpp
    Engine/EditorUI/src/ParticlePanel.cpp
    Engine/EditorUI/src/SceneFile.cpp
    Engine/EditorUI/src/Selection.cpp
    Engine/EditorUI/src/LogPanel.cpp
    Engine/EditorUI/src/HelpPanel.cpp
    Engine/EditorUI/src/ObjectTree.cpp
    Engine/EditorUI/src/PropertySheet.cpp
    Engine/EditorUI/src/CameraPanel.cpp
    Engine/EditorUI/src/SceneBrowser.cpp
    Engine/EditorUI/src/StatsPanel.cpp
    Engine/EditorUI/src/DialogPanel.cpp
    Engine/EditorUI/src/TooltipPanel.cpp
    Engine/EditorUI/src/VersionPanel.cpp
    Engine/EditorUI/src/InspectorPanel.cpp
    Engine/EditorUI/src/StatusBar.cpp
    Engine/EditorUI/src/ContextMenuPanel.cpp
    Engine/EditorUI/src/ProfilerPanel.cpp
    Engine/EditorUI/src/SearchPanel.cpp
    Engine/EditorUI/src/HierarchyTreePanel.cpp
    Engine/EditorUI/src/AnimationPanel.cpp
    Engine/EditorUI/src/AudioPanel.cpp
    Engine/EditorUI/src/LightingPanel.cpp
    Engine/EditorUI/src/TerrainPanel.cpp
    Engine/EditorUI/src/NavMeshPanel.cpp
    Engine/EditorUI/src/MaterialPanel.cpp
    Engine/EditorUI/src/Gizmo.cpp
)
target_include_directories(kimia_editor_ui PUBLIC Engine/EditorUI/include)
target_link_libraries(kimia_editor_ui PUBLIC kimia_math kimia_world kimia_assets)
target_include_directories(kimia_world PUBLIC Engine/World/include)
target_link_libraries(kimia_world PUBLIC kimia_profile kimia_scene kimia_physics kimia_assets)
target_compile_options(kimia_world PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_world PRIVATE ${KIMIA_WERROR_FLAGS})
endif()

add_executable(kimia_world_app Examples/WorldEditorApp.cpp)
set_target_properties(kimia_world_app PROPERTIES OUTPUT_NAME kimia_world)
target_link_libraries(kimia_world_app PRIVATE kimia_app kimia_world)
target_compile_options(kimia_world_app PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_world_app PRIVATE ${KIMIA_WERROR_FLAGS})
endif()

# --- Android (JNI shared library for the APK) --------------------------------
# The Gradle app in Android/ points its externalNativeBuild at this CMakeLists
# and builds only kimia_jni. The library boots the engine in-process and
# renders natively: an ANativeWindow from the Activity's SurfaceView is wrapped
# in an EGL window surface and drawn with GLES3 (Cook-Torrance PBR, shadows,
# fog, filmic tone map), with the software rasteriser as the on-device
# fallback. There is no WebView and no HTTP server. jni_glue.cpp is fully
# self-contained (game boot, touch, HUD, on-screen controls, settings flags),
# so the desktop WorldEditorApp server is NOT part of the APK. Built only when
# Gradle sets KIMIA_BUILD_ANDROID_JNI=ON.
if(KIMIA_BUILD_ANDROID_JNI)
  add_library(kimia_jni SHARED
      Android/app/src/main/cpp/jni_glue.cpp
  )
  # liblog (__android_log_print) and libandroid (ANativeWindow_*). Both are
  # system libraries on every Android device; EGL/GLES stay dlopen'd at
  # runtime so there is no build-time GPU dependency.
  target_link_libraries(kimia_jni PRIVATE kimia_app kimia_world kimia_editor_ui log android)
  target_compile_options(kimia_jni PRIVATE ${KIMIA_STRICT_WARNINGS})
endif()

# --- Embedded assets (self-contained single-file exe/apk) -------------------
# Off by default. When on, Tools/make_embedded_assets.py packs Profiles/,
# Worlds/ and Branding/ into a generated C++ source; the app unpacks them at
# runtime. Only the self-contained builds (Windows exe CI, Android APK) enable
# this, so the Termux/native headless builds stay exactly as before.
if(KIMIA_EMBED_ASSETS)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  set(KIMIA_EMBED_OUT ${CMAKE_BINARY_DIR}/generated/EmbeddedAssetsData.cpp)
  add_custom_command(
      OUTPUT ${KIMIA_EMBED_OUT}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/generated
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Tools/make_embedded_assets.py
              ${KIMIA_EMBED_OUT}
              ${CMAKE_CURRENT_SOURCE_DIR}/Profiles=profiles
              ${CMAKE_CURRENT_SOURCE_DIR}/Worlds=worlds
              ${CMAKE_CURRENT_SOURCE_DIR}/Branding=Branding
      DEPENDS Tools/make_embedded_assets.py
      VERBATIM)
  add_library(kimia_embedded STATIC
      Engine/App/src/EmbeddedAssets.cpp
      ${KIMIA_EMBED_OUT})
  target_include_directories(kimia_embedded PUBLIC Engine/App/include)
  target_compile_definitions(kimia_embedded PUBLIC KIMIA_EMBEDDED_ASSETS=1)
  target_compile_options(kimia_embedded PRIVATE ${KIMIA_STRICT_WARNINGS})
  if(KIMIA_WERROR)
    target_compile_options(kimia_embedded PRIVATE ${KIMIA_WERROR_FLAGS})
  endif()
  target_link_libraries(kimia_world_app PRIVATE kimia_embedded)
  if(KIMIA_BUILD_ANDROID_JNI)
    target_link_libraries(kimia_jni PRIVATE kimia_embedded)
  endif()
endif()

# The game profile files ship next to the binaries (kimia_world looks in
# ./profiles by default, --profiles DIR overrides).
add_custom_command(TARGET kimia_world_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Profiles $<TARGET_FILE_DIR:kimia_world_app>/profiles
)

# --- Golf (reference game logic + the kimia_golf example) ---
add_library(kimia_golf STATIC
    Engine/Golf/src/Golf.cpp
)
target_include_directories(kimia_golf PUBLIC Engine/Golf/include)
target_link_libraries(kimia_golf PUBLIC kimia_scene kimia_physics)
target_compile_options(kimia_golf PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_golf PRIVATE ${KIMIA_WERROR_FLAGS})
endif()

add_executable(kimia_golf_app Examples/GolfGame.cpp)
set_target_properties(kimia_golf_app PROPERTIES OUTPUT_NAME kimia_golf)
target_link_libraries(kimia_golf_app PRIVATE kimia_app kimia_golf)
target_compile_options(kimia_golf_app PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_golf_app PRIVATE ${KIMIA_WERROR_FLAGS})
endif()

# --- Web (WebGL2 + WebAssembly via Emscripten) ---------------------------
# Real client-side graphics/physics: the engine compiles to WASM and draws
# with the GL renderer onto a WebGL2 canvas. Built with
#   cmake --preset webgl-release && cmake --build --preset webgl-release
# (requires an activated Emscripten SDK; see Documentation/WEBGL.md).
if(EMSCRIPTEN)
  add_executable(kimia_webgl Examples/WebGLApp.cpp)
  set_target_properties(kimia_webgl PROPERTIES OUTPUT_NAME kimia_webgl SUFFIX ".html")
  target_link_libraries(kimia_webgl PRIVATE kimia_renderer kimia_math)
  target_compile_options(kimia_webgl PRIVATE ${KIMIA_STRICT_WARNINGS})
  if(KIMIA_WERROR)
    target_compile_options(kimia_webgl PRIVATE ${KIMIA_WERROR_FLAGS})
  endif()
  # WebGL2 + full GLES3 (the GL renderer's feature set), a persistent runtime
  # for the main loop, and a custom shell with the brand theme.
  target_link_options(kimia_webgl PRIVATE
    -sUSE_WEBGL2=1
    -sFULL_ES3=1
    -sMAX_WEBGL_VERSION=2
    -sALLOW_MEMORY_GROWTH=1
    -sEXIT_RUNTIME=0
    -sINITIAL_MEMORY=67108864
    --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/Web/webgl-shell.html)
endif()

include(CTest)
if(KIMIA_BUILD_TESTS)
  # Run from the source tree: several tests read real files out of
  # Tests/assets, and ctest would otherwise start them in the build folder
  # where those files do not exist.
  add_test(NAME kimia_tests COMMAND kimia_tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# Release staging. The PowerShell PC packager uses this install graph so the
# executable/profile layout is identical to the layout tested in development.
include(GNUInstallDirs)
install(TARGETS kimia_world_app RUNTIME DESTINATION .)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Profiles/ DESTINATION profiles)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Branding)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Branding/ DESTINATION Branding)
endif()
if(WIN32 AND KIMIA_BUILD_PC AND KIMIA_ENABLE_D3D11)
  install(TARGETS kimia_pc_d3d11_smoke RUNTIME DESTINATION .)
endif()
if(TARGET SDL2::SDL2)
  # Shared SDL2 builds need the runtime DLL beside kimia_world.exe.
  install(FILES $<TARGET_FILE:SDL2::SDL2> DESTINATION .)
endif()
