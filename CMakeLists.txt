cmake_minimum_required(VERSION 3.16)
# Engine version = MAJOR.MINOR.PATCH, bumped by the size of the change (bug
# fix / small: +0.0.1, stage / big: +0.1.0), never by the date. The same
# literal lives in Engine/Core/include/kimia/Version.h; the configure step
# below and a unit test keep the two identical. Bump both + add the entry at
# the top of CHANGELOG.md per release.
project(KIMIA VERSION 0.21.0 LANGUAGES C CXX)
file(STRINGS Engine/Core/include/kimia/Version.h KIMIA_VERSION_LINE REGEX "#define KIMIA_ENGINE_VERSION \"")
string(REGEX REPLACE ".*\"([0-9.]+)\".*" "\\1" KIMIA_HEADER_VERSION "${KIMIA_VERSION_LINE}")
if(NOT KIMIA_HEADER_VERSION STREQUAL PROJECT_VERSION)
  message(FATAL_ERROR "Version.h says ${KIMIA_HEADER_VERSION} but CMakeLists.txt says ${PROJECT_VERSION}")
endif()
message(STATUS "KIMIA engine version ${PROJECT_VERSION}")

option(KIMIA_WERROR "Treat warnings as errors" ON)
option(KIMIA_BUILD_TOOLS "Build the kimia_assets CLI tool" ON)
option(KIMIA_ENABLE_SDL2 "Use SDL2 for the window backend when available" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)

# No build type given (the usual `cmake -B build` on a phone) -> Release: the
# software rasterizer draws every frame on the CPU, and an unoptimized build
# is several times slower there. Pass -DCMAKE_BUILD_TYPE=Debug to override.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (Release by default)" FORCE)
endif()
message(STATUS "KIMIA build type: ${CMAKE_BUILD_TYPE}, compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

# -Wsign-conversion is listed EXPLICITLY: GCC's -Wconversion does not imply
# it in C++ but Clang's does, so a signed/unsigned slip compiled clean on
# every GCC gate here and then failed on the phone's Clang. Naming it makes
# both compilers agree.
set(KIMIA_STRICT_WARNINGS -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
                          -Wdouble-promotion)
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

add_library(kimia_core
    Engine/Core/src/Log.cpp
    Engine/Core/src/Profiler.cpp
    Engine/Core/src/Time.cpp
)
target_include_directories(kimia_core PUBLIC Engine/Core/include)
target_compile_options(kimia_core PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_core PRIVATE -Werror)
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
  target_compile_options(kimia_scene PRIVATE -Werror)
endif()

add_library(kimia_physics STATIC
    Engine/Physics/src/Physics.cpp
)
target_include_directories(kimia_physics PUBLIC Engine/Physics/include)
target_link_libraries(kimia_physics PUBLIC kimia_core kimia_math)
target_compile_options(kimia_physics PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_physics PRIVATE -Werror)
endif()

# --- Game profiles (the data that turns the engine into one game) ---
add_library(kimia_profile STATIC
    Engine/Profile/src/GameProfile.cpp
)
target_include_directories(kimia_profile PUBLIC Engine/Profile/include)
target_link_libraries(kimia_profile PUBLIC kimia_core)
target_compile_options(kimia_profile PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_profile PRIVATE -Werror)
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
  target_compile_options(kimia_assets PRIVATE -Werror)
endif()

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
    Tests/src/ProfileTests.cpp
)
target_include_directories(kimia_tests PRIVATE Tests/include Engine/Core/include)
target_link_libraries(kimia_tests PRIVATE kimia_core kimia_math kimia_assets kimia_scene kimia_physics kimia_profile kimia_app kimia_golf kimia_world kimia_raytracer)
target_compile_options(kimia_tests PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_tests PRIVATE -Werror)
endif()
target_compile_definitions(kimia_tests PRIVATE
    KIMIA_CMAKE_VERSION="${PROJECT_VERSION}"
    KIMIA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    KIMIA_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Tests/assets"
    KIMIA_PROFILE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Profiles"
    KIMIA_TEST_TMP="${CMAKE_BINARY_DIR}/kimia_test_tmp"
)

# --- Raytracer (offline path tracer render preview) ---
add_library(kimia_raytracer STATIC
    Engine/Raytracer/src/Raytracer.cpp
)
target_include_directories(kimia_raytracer PUBLIC Engine/Raytracer/include)
target_link_libraries(kimia_raytracer PUBLIC kimia_world kimia_math)
target_compile_options(kimia_raytracer PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_raytracer PRIVATE -Werror)
endif()

# Tiny generator that writes the test assets (tone.wav, 2x3.png, 2x2.jpg,
# cube.obj, quad.obj) into Tests/assets. Runs on every build.
add_executable(kimia_asset_gen Tools/src/asset_gen.cpp)
target_link_libraries(kimia_asset_gen PRIVATE kimia_assets)
target_compile_options(kimia_asset_gen PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_asset_gen PRIVATE -Werror)
endif()
add_custom_target(kimia_generate_assets ALL
    COMMAND ${CMAKE_COMMAND} -E env
        KIMIA_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Tests/assets"
        $<TARGET_FILE:kimia_asset_gen>
    DEPENDS kimia_asset_gen
)
add_dependencies(kimia_tests kimia_generate_assets)

if(KIMIA_BUILD_TOOLS)
  add_executable(kimia_assets_cli Tools/src/kimia_assets.cpp)
  target_link_libraries(kimia_assets_cli PRIVATE kimia_assets)
  target_compile_options(kimia_assets_cli PRIVATE ${KIMIA_STRICT_WARNINGS})
  if(KIMIA_WERROR)
    target_compile_options(kimia_assets_cli PRIVATE -Werror)
  endif()

  add_executable(kimia_raytrace_cli Tools/src/kimia_raytrace.cpp)
  set_target_properties(kimia_raytrace_cli PROPERTIES OUTPUT_NAME kimia_raytrace)
  target_link_libraries(kimia_raytrace_cli PRIVATE kimia_raytracer kimia_world)
  target_compile_options(kimia_raytrace_cli PRIVATE ${KIMIA_STRICT_WARNINGS})
  if(KIMIA_WERROR)
    target_compile_options(kimia_raytrace_cli PRIVATE -Werror)
  endif()
endif()

# --- Renderer (GL 3.3 loader + EGL + shaders + software rasterizer) ---
add_library(kimia_renderer STATIC
    Engine/Renderer/src/GLFunctions.cpp
    Engine/Renderer/src/EGL.cpp
    Engine/Renderer/src/Shader.cpp
    Engine/Renderer/src/GpuMesh.cpp
    Engine/Renderer/src/Shaders.cpp
    Engine/Renderer/src/Renderer.cpp
    Engine/Renderer/src/SoftwareRenderer.cpp
)
target_include_directories(kimia_renderer PUBLIC Engine/Renderer/include)
# dlopen/dlsym for the runtime GL loader (libdl on Android/bionic and old glibc).
target_link_libraries(kimia_renderer PUBLIC kimia_assets kimia_math ${CMAKE_DL_LIBS})
target_compile_options(kimia_renderer PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_renderer PRIVATE -Werror)
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
  target_compile_options(kimia_platform PRIVATE -Werror)
endif()

# --- App (WebViewer + engine bootstrap) ---
add_library(kimia_app STATIC
    Engine/App/src/WebViewer.cpp
    Engine/App/src/Engine.cpp
)
target_include_directories(kimia_app PUBLIC Engine/App/include)
target_link_libraries(kimia_app PUBLIC kimia_platform kimia_renderer Threads::Threads)
target_compile_options(kimia_app PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_app PRIVATE -Werror)
endif()

# --- Examples ---
add_executable(kimia_hello Examples/HelloWindow.cpp)
target_link_libraries(kimia_hello PRIVATE kimia_app)
target_compile_options(kimia_hello PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_hello PRIVATE -Werror)
endif()

add_executable(kimia_first3d Examples/First3DScene.cpp)
target_link_libraries(kimia_first3d PRIVATE kimia_app)
target_compile_options(kimia_first3d PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_first3d PRIVATE -Werror)
endif()

add_executable(kimia_remote Examples/RemoteView.cpp)
target_link_libraries(kimia_remote PRIVATE kimia_app)
target_compile_options(kimia_remote PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_remote PRIVATE -Werror)
endif()

# --- World (option-driven editor + SceneIO-v1-compatible world files) ---
add_library(kimia_world STATIC
    Engine/World/src/World.cpp
    Engine/World/src/WorldBuilder.cpp
    Engine/World/src/Studio.cpp
    Engine/World/src/Logic.cpp
    Engine/World/src/Picking.cpp
    Engine/World/src/WorldIO.cpp
)
target_include_directories(kimia_world PUBLIC Engine/World/include)
target_link_libraries(kimia_world PUBLIC kimia_profile kimia_scene kimia_physics kimia_assets)
target_compile_options(kimia_world PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_world PRIVATE -Werror)
endif()

add_executable(kimia_world_app Examples/WorldEditorApp.cpp)
set_target_properties(kimia_world_app PROPERTIES OUTPUT_NAME kimia_world)
target_link_libraries(kimia_world_app PRIVATE kimia_app kimia_world)
target_compile_options(kimia_world_app PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_world_app PRIVATE -Werror)
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
  target_compile_options(kimia_golf PRIVATE -Werror)
endif()

add_executable(kimia_golf_app Examples/GolfGame.cpp)
set_target_properties(kimia_golf_app PROPERTIES OUTPUT_NAME kimia_golf)
target_link_libraries(kimia_golf_app PRIVATE kimia_app kimia_golf)
target_compile_options(kimia_golf_app PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_golf_app PRIVATE -Werror)
endif()

include(CTest)
# Run from the source tree: several tests read real files out of
# Tests/assets, and ctest would otherwise start them in the build folder
# where those files do not exist.
add_test(NAME kimia_tests COMMAND kimia_tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
