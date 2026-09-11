#!/usr/bin/env bash
# One-time Codespace setup: build KIMIA headless (software renderer +
# WebViewer) and run the aggregate test suite. Runs automatically via
# devcontainer.json `postCreateCommand`; safe to re-run manually too.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

# The Codespaces C++ image ships cmake/ninja on PATH; also support user-local
# installs (e.g. `pip install --user cmake ninja`) for other sandboxes.
export PATH="$HOME/.local/bin:$PATH"

echo "==> cmake configure (Release, -Werror, headless)"
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DKIMIA_WERROR=ON \
  -DKIMIA_ENABLE_SDL2=OFF

echo "==> build"
cmake --build build -j"$(nproc)"

echo "==> tests"
./build/bin/kimia_tests | tail -3

echo
echo "KIMIA is ready. Start the WebViewer with:"
echo "  ./build/bin/kimia_world --port 8080 --bind 0.0.0.0 --profiles build/bin/profiles"
echo "then open the forwarded port 8080 (PORTS panel, bottom of VS Code)."
