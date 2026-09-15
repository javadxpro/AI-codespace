#!/usr/bin/env bash
# Standalone EditorUI build & syntax check.
#
# The Android APK builds through Gradle + NDK + cmake, but we want a
# fast feedback loop on the desktop too. This script:
#   1. Compiles each EditorUI .cpp to a .o with -fsyntax-only
#   2. Checks the includes resolve against the engine headers
#   3. Reports any error so CI can fail fast
#
# It is intentionally cheap: no link, no executable. The APK CI runs the
# real Gradle build.
set -euo pipefail

KIMIA="${KIMIA:-/home/user/KIMIA-ENGINE}"
INCLUDE="-I${KIMIA}/Engine/Core/include \
         -I${KIMIA}/Engine/Math/include \
         -I${KIMIA}/Engine/Graphics/include \
         -I${KIMIA}/Engine/Assets/include \
         -I${KIMIA}/Engine/Renderer/include \
         -I${KIMIA}/Engine/World/include \
         -I${KIMIA}/Engine/EditorUI/include"

CXXFLAGS="-std=c++17 -fsyntax-only -Wall -Wextra -Wno-unused-parameter \
          -DKIMIA_HAS_D3D11=0 -DKIMIA_PRIMARY_RENDERER_OPENGL=1"

cd "${KIMIA}/Engine/EditorUI/src"

echo "== Widget.cpp =="
g++ ${CXXFLAGS} ${INCLUDE} Widget.cpp
echo "== Theme.cpp =="
g++ ${CXXFLAGS} ${INCLUDE} Theme.cpp
echo "== Icons.cpp =="
g++ ${CXXFLAGS} ${INCLUDE} Icons.cpp
echo "== Panel.cpp =="
g++ ${CXXFLAGS} ${INCLUDE} Panel.cpp
echo "== EditorUI.cpp =="
g++ ${CXXFLAGS} ${INCLUDE} EditorUI.cpp

echo "All EditorUI files passed syntax check."
