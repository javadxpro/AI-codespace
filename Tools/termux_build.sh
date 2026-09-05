#!/usr/bin/env bash
# KIMIA — one-command build for Termux (Android) and Debian/Ubuntu.
#
#   bash Tools/termux_build.sh            # toolchain check, configure, build, tests
#   bash Tools/termux_build.sh --run      # ... then start kimia_world on port 8080
#   bash Tools/termux_build.sh --clean    # wipe build/ first
#
# What it does, in order:
#   1. Checks that this checkout really contains the engine (main on GitHub
#      may hold only the skeleton; the engine lives on the arena/* branch).
#   2. Termux: pkg-installs clang, cmake, ninja (repairs a broken cmake).
#      Debian/Ubuntu: prints the apt line if something is missing.
#   3. cmake -B build -DKIMIA_WERROR=ON (Release by default) + build.
#   4. Runs build/bin/kimia_tests and prints the next commands.
set -u

BOLD=$'\033[1m'; RED=$'\033[31m'; GREEN=$'\033[32m'; YELLOW=$'\033[33m'; RESET=$'\033[0m'
say()  { printf '%s==>%s %s\n' "$BOLD" "$RESET" "$*"; }
ok()   { printf '%s ok %s %s\n' "$GREEN" "$RESET" "$*"; }
warn() { printf '%s !! %s %s\n' "$YELLOW" "$RESET" "$*"; }
die()  { printf '%s XX %s %s\n' "$RED" "$RESET" "$*" >&2; exit 1; }

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT" || die "cannot cd to $ROOT"
BUILD_DIR="$ROOT/build"
RUN_AFTER=0
CLEAN=0
PORT=8080
for arg in "$@"; do
  case "$arg" in
    --run) RUN_AFTER=1 ;;
    --clean) CLEAN=1 ;;
    --port=*) PORT="${arg#--port=}" ;;
    -h|--help) sed -n '2,15p' "$0"; exit 0 ;;
    *) die "unknown option: $arg (try --help)" ;;
  esac
done

JOBS="$(nproc 2>/dev/null || echo 2)"
IS_TERMUX=0
if [ -n "${TERMUX_VERSION:-}" ] || [ -d /data/data/com.termux/files/usr ]; then IS_TERMUX=1; fi

# --- 1. Is the engine actually here? ---------------------------------------
say "checkout: $ROOT"
if [ ! -f Engine/World/src/World.cpp ] || [ ! -f Examples/WorldEditorApp.cpp ]; then
  BRANCH="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo '?')"
  warn "this checkout has no KIMIA World (Engine/World missing) — branch: $BRANCH"
  warn "GitHub's 'main' holds only the Core skeleton; the engine is on the arena/* branch."
  FULL="$(git branch -r 2>/dev/null | grep -o 'origin/arena/[0-9a-f]*-ai-codespace' | tail -1)"
  if [ -n "$FULL" ]; then
    warn "switching to ${FULL#origin/} ..."
    git fetch -q origin "${FULL#origin/}" || die "git fetch failed"
    git checkout -q "${FULL#origin/}" 2>/dev/null || git checkout -q -b "${FULL#origin/}" "$FULL" || die "git checkout failed"
    git pull -q --ff-only origin "${FULL#origin/}" || true
    ok "now on $(git rev-parse --abbrev-ref HEAD) ($(git rev-parse --short HEAD))"
  else
    die "no arena/* branch found; run: git fetch origin && git branch -r"
  fi
fi
[ -f Engine/World/src/World.cpp ] || die "still no Engine/World — stop here"
ENGINE_VERSION="$(grep -o 'KIMIA_ENGINE_VERSION "[0-9.]*"' Engine/Core/include/kimia/Version.h | grep -o '[0-9.]*')"
ok "engine ${ENGINE_VERSION:-?} on branch $(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo '?')"

# --- 2. Toolchain ------------------------------------------------------------
need_pkgs=()
command -v cmake >/dev/null 2>&1 || need_pkgs+=(cmake)
command -v ninja >/dev/null 2>&1 || need_pkgs+=(ninja)
if [ "$IS_TERMUX" = 1 ]; then
  command -v clang++ >/dev/null 2>&1 || need_pkgs+=(clang)
  command -v git >/dev/null 2>&1 || need_pkgs+=(git)
  if [ "${#need_pkgs[@]}" -gt 0 ]; then
    say "pkg install ${need_pkgs[*]}"
    pkg install -y "${need_pkgs[@]}" || die "pkg install failed (run: pkg update && pkg upgrade, then retry)"
  fi
  # A half-upgraded cmake prints "CMAKE_ROOT" errors; reinstalling fixes it.
  if ! cmake --version >/dev/null 2>&1 || cmake --version 2>&1 | grep -q CMAKE_ROOT; then
    warn "cmake is broken — reinstalling"
    pkg uninstall -y cmake && pkg install -y cmake || die "cmake reinstall failed"
  fi
  export CC=clang CXX=clang++
else
  command -v c++ >/dev/null 2>&1 || need_pkgs+=(g++)
  if [ "${#need_pkgs[@]}" -gt 0 ]; then
    die "missing: ${need_pkgs[*]} — install with: sudo apt-get install -y build-essential cmake ninja-build git"
  fi
fi
ok "cmake $(cmake --version | head -1 | awk '{print $3}'), $("${CXX:-c++}" --version | head -1)"

# --- 3. Configure + build ----------------------------------------------------
if [ "$CLEAN" = 1 ]; then say "removing $BUILD_DIR"; rm -rf "$BUILD_DIR"; fi
GEN=()
command -v ninja >/dev/null 2>&1 && GEN=(-G Ninja)
say "cmake -B build -DKIMIA_WERROR=ON (Release)"
cmake -S "$ROOT" -B "$BUILD_DIR" "${GEN[@]}" -DKIMIA_WERROR=ON -DCMAKE_BUILD_TYPE=Release || die "configure failed"
say "cmake --build build -j$JOBS"
if ! cmake --build "$BUILD_DIR" -j"$JOBS" 2>&1 | tee "$BUILD_DIR/build.log"; then
  die "build failed — the log is in build/build.log; paste the first error line"
fi
if grep -q 'warning:' "$BUILD_DIR/build.log"; then
  warn "$(grep -c 'warning:' "$BUILD_DIR/build.log") warning(s) — the engine promises zero; paste them"
else
  ok "0 warnings"
fi
[ -x "$BUILD_DIR/bin/kimia_world" ] || die "build/bin/kimia_world was not produced"

# --- 4. Tests + next steps ---------------------------------------------------
say "running tests"
"$BUILD_DIR/bin/kimia_tests" | tail -2 || die "tests failed"
ok "$("$BUILD_DIR/bin/kimia_world" --version)"
echo
echo "${BOLD}next:${RESET}"
echo "  ./build/bin/kimia_world --port $PORT --profiles build/bin/profiles"
echo "  then open  http://127.0.0.1:$PORT  in the phone's browser"
if [ "$RUN_AFTER" = 1 ]; then
  say "starting kimia_world on port $PORT (Ctrl+C stops it)"
  exec "$BUILD_DIR/bin/kimia_world" --port "$PORT" --profiles "$BUILD_DIR/bin/profiles"
fi
