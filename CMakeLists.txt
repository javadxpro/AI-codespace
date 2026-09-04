cmake_minimum_required(VERSION 3.16)
project(KIMIA VERSION 0.1.0 LANGUAGES C CXX)

option(KIMIA_WERROR "Treat warnings as errors" ON)
option(KIMIA_BUILD_TOOLS "Build the kimia_assets CLI tool" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)

set(KIMIA_STRICT_WARNINGS -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wdouble-promotion)

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

# Vendored FBX loader (single-file library, upstream code).
add_library(kimia_ufbx STATIC ThirdParty/ufbx/ufbx.c)
target_include_directories(kimia_ufbx PUBLIC ThirdParty/ufbx)

add_library(kimia_assets STATIC
    Engine/Graphics/src/Image.cpp
    Engine/Graphics/src/Mesh.cpp
    Engine/Assets/src/AssetPipeline.cpp
    Engine/Assets/src/Audio.cpp
)
target_include_directories(kimia_assets PUBLIC
    Engine/Graphics/include
    Engine/Assets/include
    Engine/Core/include
    ThirdParty/stb
    ThirdParty/dr
)
target_link_libraries(kimia_assets PUBLIC kimia_core kimia_math kimia_ufbx)
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
)
target_include_directories(kimia_tests PRIVATE Tests/include Engine/Core/include)
target_link_libraries(kimia_tests PRIVATE kimia_core kimia_math kimia_assets kimia_scene)
target_compile_options(kimia_tests PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_tests PRIVATE -Werror)
endif()
target_compile_definitions(kimia_tests PRIVATE
    KIMIA_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Tests/assets"
    KIMIA_TEST_TMP="${CMAKE_BINARY_DIR}/kimia_test_tmp"
)

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
endif()

include(CTest)
add_test(NAME kimia_tests COMMAND kimia_tests)
