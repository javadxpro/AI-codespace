#!/usr/bin/env bash
# KIMIA — one-command build for a PlayStation 4 running Linux.
#
#   bash Tools/ps4_build.sh            # toolchain check, configure, build, tests
#   bash Tools/ps4_build.sh --run      # ... then serve the editor on port 8080
#   bash Tools/ps4_build.sh --clean    # wipe build-ps4/ first
#   bash Tools/ps4_build.sh --port=9000
#
# The PS4 is an x86-64 PC; KIMIA is portable C++17. No port is required — this
# script builds the SAME engine the phone/desktop build, configured for the PS4's
# reality: no GPU driver to rely on, no window. So it uses the headless path
# (KIMIA_ENABLE_SDL2=OFF) — software rasterizer + the WebViewer HTTP server.
# You then point a phone/PC browser at the PS4's IP and use the editor there.
#
# Run this INSIDE Linux on the PS4 (psxitarch / any Arch- or Debian-based
# PS4 distro). The jailbreak must already be active and the system booted.
set -u

BOLD=$'\033[1m'; RED=$'\033[31m'; GREEN=$'\033[32m'; YELLOW=$'\033[33m'; RESET=$'\033[0m'
say()  { printf '%s==>%s %s\n' "$BOLD" "$RESET" "$*"; }
ok()   { printf '%s ok %s %s\n' "$GREEN" "$RESET" "$*"; }
warn() { printf '%s !! %s %s\n' "$YELLOW" "$RESET" "$*"; }
die()  { printf '%s XX %s %s\n' "$RED" "$RESET" "$*" >&2; exit 1; }

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT" || die "cannot cd to $ROOT"
BUILD_DIR="$ROOT/build-ps4"
RUN_AFTER=0
CLEAN=0
PORT=8080
BIND=0.0.0.0
for arg in "$@"; do
  case "$arg" in
    --run) RUN_AFTER=1 ;;
    --clean) CLEAN=1 ;;
    --port=*) PORT="${arg#--port=}" ;;
    --bind=*) BIND="${arg#--bind=}" ;;
    -h|--help) sed -n '2,19p' "$0"; exit 0 ;;
    *) die "unknown option: $arg (try --help)" ;;
  esac
done

JOBS="$(nproc 2>/dev/null || echo 2)"

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
ENGINE_VERSION="$(grep -o 'KIMIA_ENGINE_VERSION \"[0-9.]*\"' Engine/Core/include/kimia/Version.h | grep -o '[0-9.]*')"
ok "engine ${ENGINE_VERSION:-?} on branch $(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo '?')"

# --- 2. Toolchain (PS4 distros are Arch- or Debian-based) -------------------
need_pkgs=()
command -v cmake >/dev/null 2>&1 || need_pkgs+=(cmake)
command -v ninja >/dev/null 2>&1 || need_pkgs+=(ninja)
command -v c++   >/dev/null 2>&1 || command -v g++ >/dev/null 2>&1 || need_pkgs+=(gcc)
command -v git   >/dev/null 2>&1 || need_pkgs+=(git)

if [ "${#need_pkgs[@]}" -gt 0 ]; then
  if command -v pacman >/dev/null 2>&1; then
    say "sudo pacman -S --needed base-devel ${need_pkgs[*]}"
    sudo pacman -S --needed --noconfirm base-devel "${need_pkgs[@]}" \
      || die "pacman failed — check your network / keyring, then retry"
  elif command -v apt-get >/dev/null 2>&1; then
    say "sudo apt-get install -y build-essential ${need_pkgs[*]}"
    sudo apt-get update -y || true
    sudo apt-get install -y build-essential "${need_pkgs[@]}" \
      || die "apt failed — check your network, then retry"
  elif command -v dnf >/dev/null 2>&1; then
    say "sudo dnf install -y gcc-c++ cmake ninja-build git"
    sudo dnf install -y gcc-c++ cmake ninja-build git || die "dnf failed"
  else
    die "no package manager found (pacman/apt/dnf). Install cmake, ninja, gcc and git by hand."
  fi
fi
ok "cmake $(cmake --version | head -1 | awk '{print $3}'), $(c++ --version | head -1)"

# --- 3. Configure + build (headless: software rasterizer + WebViewer) -------
if [ "$CLEAN" = 1 ]; then say "removing $BUILD_DIR"; rm -rf "$BUILD_DIR"; fi
say "cmake -B build-ps4 -G Ninja -DKIMIA_WERROR=ON -DKIMIA_ENABLE_SDL2=OFF (Release)"
cmake -S "$ROOT" -B "$BUILD_DIR" -G Ninja -DKIMIA_WERROR=ON -DKIMIA_ENABLE_SDL2=OFF \
  -DCMAKE_BUILD_TYPE=Release || die "configure failed"
say "cmake --build build-ps4 -j$JOBS  (this is slow on the PS4's Jaguar cores — be patient)"
if ! cmake --build "$BUILD_DIR" -j"$JOBS" 2>&1 | tee "$BUILD_DIR/build.log"; then
  die "build failed — the log is in build-ps4/build.log; paste the first error line"
fi
if grep -q 'warning:' "$BUILD_DIR/build.log"; then
  warn "$(grep -c 'warning:' "$BUILD_DIR/build.log") warning(s) — the engine promises zero; paste them"
else
  ok "0 warnings"
fi
[ -x "$BUILD_DIR/bin/kimia_world" ] || die "build-ps4/bin/kimia_world was not produced"

# --- 4. Tests + next steps ---------------------------------------------------
say "running tests"
"$BUILD_DIR/bin/kimia_tests" | tail -2 || die "tests failed"
ok "$("$BUILD_DIR/bin/kimia_world" --version)"

# The PS4 has no screen of its own here: serve the editor to the whole LAN and
# drive it from a phone or PC browser. --auth keeps a stray LAN client out.
echo
echo "${BOLD}next:${RESET}"
echo "  ./build-ps4/bin/kimia_world --port $PORT --bind $BIND --auth CHANGE_ME --profiles build-ps4/bin/profiles"
echo "  then open  http://<PS4-IP>:$PORT/?token=CHANGE_ME  in a phone/PC browser on the same network"
echo "  (find the PS4's IP with: ip addr | grep 'inet ')"
if [ "$RUN_AFTER" = 1 ]; then
  say "starting kimia_world on $BIND:$PORT (Ctrl+C stops it)"
  exec "$BUILD_DIR/bin/kimia_world" --port "$PORT" --bind "$BIND" --auth CHANGE_ME \
    --profiles "$BUILD_DIR/bin/profiles"
fi
