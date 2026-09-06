#!/usr/bin/env bash
# KIMIA — build an offline release package (the "نسخهٔ آفلاین" of a game).
#
#   bash Tools/package_release.sh                 # package every game
#   bash Tools/package_release.sh --game golf     # just the golf release
#   bash Tools/package_release.sh --out /tmp/rel  # where to put the archive
#
# What it produces: a self-contained folder + .tar.gz that a player can copy
# to a phone or a PC, unpack, and run with ONE command — no toolchain, no
# network, no Git. It is "offline" in both senses: no download at play time
# and no online multiplayer.
#
# What it does, in order:
#   1. Configures and builds a Release with -DKIMIA_WERROR=ON (zero warnings).
#   2. Runs the whole test suite — a package is never cut from a red tree.
#   3. Stages the runtime: the kimia_world binary, the game profiles, an
#      empty worlds/ + assets/ folder, the licences, and a run script.
#   4. Writes VERSION/MANIFEST (engine version, git commit, sha256 of every
#      shipped file) so a package can always be traced back to a commit.
#   5. Smoke-tests the STAGED package: starts it on a spare port, checks the
#      menu really answers, and only then makes the archive.
#
# The engine ships no game content: a release contains the game *maker* and
# the profiles, and the player builds (or opens) their own course.
set -u

BOLD=$'\033[1m'; RED=$'\033[31m'; GREEN=$'\033[32m'; YELLOW=$'\033[33m'; RESET=$'\033[0m'
say()  { printf '%s==>%s %s\n' "$BOLD" "$RESET" "$*"; }
ok()   { printf '%s ok %s %s\n' "$GREEN" "$RESET" "$*"; }
warn() { printf '%s !! %s %s\n' "$YELLOW" "$RESET" "$*"; }
die()  { printf '%s XX %s %s\n' "$RED" "$RESET" "$*" >&2; exit 1; }

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT" || die "cannot cd to $ROOT"

GAME="all"
OUT_DIR="$ROOT/release"
BUILD_DIR="$ROOT/build-release"
SKIP_TESTS=0
for arg in "$@"; do
  case "$arg" in
    --game=*) GAME="${arg#--game=}" ;;
    --game)   die "use --game=NAME (e.g. --game=golf)" ;;
    --out=*)  OUT_DIR="${arg#--out=}" ;;
    --build=*) BUILD_DIR="${arg#--build=}" ;;
    --skip-tests) SKIP_TESTS=1 ;;   # for a re-package of an already tested tree
    -h|--help) sed -n '2,25p' "$0"; exit 0 ;;
    *) die "unknown option: $arg (try --help)" ;;
  esac
done

JOBS="$(nproc 2>/dev/null || echo 2)"

# --- 1. Build ---------------------------------------------------------------
GEN=()
command -v ninja >/dev/null 2>&1 && GEN=(-G Ninja)
say "configure + build (Release, -Werror)"
cmake -S "$ROOT" -B "$BUILD_DIR" "${GEN[@]}" -DKIMIA_WERROR=ON -DCMAKE_BUILD_TYPE=Release >/dev/null \
  || die "configure failed"
cmake --build "$BUILD_DIR" -j"$JOBS" 2>&1 | tee "$BUILD_DIR/build.log" | tail -1
grep -q 'error:' "$BUILD_DIR/build.log" && die "build failed"
WARNINGS="$(grep -c 'warning:' "$BUILD_DIR/build.log" || true)"
[ "$WARNINGS" = "0" ] || die "$WARNINGS warning(s): the engine promises zero, refusing to package"
ok "0 warnings"
BIN="$BUILD_DIR/bin/kimia_world"
[ -x "$BIN" ] || die "$BIN was not produced"

# --- 2. Tests ---------------------------------------------------------------
if [ "$SKIP_TESTS" = 1 ]; then
  warn "--skip-tests: packaging WITHOUT running the suite"
else
  say "running the test suite (a package is never cut from a red tree)"
  TEST_LINE="$("$BUILD_DIR/bin/kimia_tests" | tail -1)" || die "tests failed: $TEST_LINE"
  case "$TEST_LINE" in
    *" tests passed") ok "$TEST_LINE" ;;
    *) die "tests failed: $TEST_LINE" ;;
  esac
fi

VERSION="$("$BIN" --version | awk '{print $2}')"
[ -n "$VERSION" ] || die "could not read the engine version"
COMMIT="$(git -C "$ROOT" rev-parse --short HEAD 2>/dev/null || echo unknown)"
DIRTY=""
git -C "$ROOT" diff --quiet 2>/dev/null || DIRTY=" (uncommitted changes)"

# --- 3. Stage ---------------------------------------------------------------
package_one() {
  local game="$1"
  local name="kimia-${game}-${VERSION}"
  local stage="$OUT_DIR/$name"
  say "staging $name"
  rm -rf "$stage"
  mkdir -p "$stage/profiles" "$stage/worlds" "$stage/assets"

  cp "$BIN" "$stage/kimia_world"
  chmod +x "$stage/kimia_world"

  # Profiles: one game, or all of them.
  if [ "$game" = "all" ]; then
    cp "$ROOT"/Profiles/*.kimiaprofile "$stage/profiles/"
  else
    local src="$ROOT/Profiles/$game.kimiaprofile"
    [ -f "$src" ] || die "no such game profile: Profiles/$game.kimiaprofile"
    cp "$src" "$stage/profiles/"
  fi

  # Branding: the intro film and its poster. The app finds them by itself in
  # a Branding folder next to the binary, so no flag is needed in play.sh.
  if [ -f "$ROOT/Branding/kimia-intro.mp4" ]; then
    mkdir -p "$stage/Branding"
    cp "$ROOT/Branding/kimia-intro.mp4" "$stage/Branding/"
    [ -f "$ROOT/Branding/kimia-logo.png" ] && cp "$ROOT/Branding/kimia-logo.png" "$stage/Branding/"
  fi

  # Licences: ours plus every vendored library's. The vendored sources carry
  # their licence in the file header rather than a separate LICENSE file, so
  # we ship a summary that names each one and points at the header.
  mkdir -p "$stage/licenses"
  local lic
  for lic in "$ROOT"/ThirdParty/*/LICENSE*; do
    [ -e "$lic" ] || continue
    cp "$lic" "$stage/licenses/$(basename "$(dirname "$lic")")-$(basename "$lic")" 2>/dev/null || true
  done
  [ -f "$ROOT/LICENSE" ] && cp "$ROOT/LICENSE" "$stage/licenses/KIMIA-LICENSE"
  cat > "$stage/licenses/THIRD-PARTY.txt" <<'LICEOF'
KIMIA ships only free/open-source dependencies. Each vendored library keeps
its licence in the header of its own source file (ThirdParty/<lib>/ in the
repository); the licences are:

  stb_image, stb_image_write, stb_vorbis   Public Domain (Unlicense) or MIT,
                                           at your option — Sean Barrett
  dr_wav, dr_mp3, dr_flac                  Public Domain (Unlicense) or
                                           MIT-0, at your option — David Reid
  ufbx                                     MIT — ufbx authors
  SDL2 (optional, not linked in this
  package unless it was found at build)    zlib licence

The KIMIA engine and editor code itself is in the project repository.
No dependency requires network access, telemetry, or a licence key.
LICEOF

  # The one-command run script.
  cat > "$stage/play.sh" <<'RUNEOF'
#!/usr/bin/env sh
# KIMIA — start the game and print the address to open in a browser.
#   sh play.sh            # port 8080
#   sh play.sh 9000       # another port
set -u
DIR="$(cd "$(dirname "$0")" && pwd)"
PORT="${1:-8080}"
echo "KIMIA is starting on port $PORT ..."
echo "open this in your browser:  http://127.0.0.1:$PORT"
echo "(Ctrl+C stops the game)"
exec "$DIR/kimia_world" --port "$PORT" \
     --profiles "$DIR/profiles" \
     --assets "$DIR/assets" \
     --world "$DIR/worlds/my_world.kimia"
RUNEOF
  chmod +x "$stage/play.sh"

  # The player-facing readme (Persian: this is who the game is for).
  cat > "$stage/README.txt" <<EOF
KIMIA — نسخهٔ آفلاین
موتور: $VERSION   |   کامیت: $COMMIT

اجرا (یک دستور):

    sh play.sh

بعد در مرورگر باز کن:  http://127.0.0.1:8080
پورت دیگر:             sh play.sh 9000

همه‌چیز منو است — هیچ کدی نمی‌نویسی و هیچ کلیدی لازم نیست بدانی.
«دنیای جدید» → بازی را انتخاب کن → جسم اضافه کن → «بازی (PLAY)».

داخل این پوشه:
  kimia_world   خود بازی (بدون هیچ وابستگی؛ اینترنت لازم ندارد)
  play.sh       اجرای یک‌دستوره
  profiles/     فایل‌های متنی بازی‌ها — با ویرایش‌شان بازی عوض می‌شود
  worlds/       دنیاهایی که می‌سازی اینجا ذخیره می‌شوند
  assets/       فایل‌های OBJ/FBX خودت را اینجا بگذار تا در بازی بگذاری‌شان
  Branding/     فیلم معرفی و لوگو که موقع باز شدن بازی پخش می‌شود
  licenses/     لایسنس کتابخانه‌های آزاد استفاده‌شده
  MANIFEST.txt  فهرست فایل‌ها با sha256

موقع باز کردن صفحه، فیلم معرفی پخش می‌شود؛ با دکمهٔ «رد کردن / SKIP» می‌توانی
از آن بگذری و در همان تب دیگر تکرار نمی‌شود. برای خاموش کردن کامل آن،
پوشهٔ Branding را پاک کن یا بازی را با گزینهٔ --no-intro اجرا کن.

این نسخه آفلاین است: نه هنگام بازی به اینترنت نیاز دارد و نه بازی آنلاین
دارد. اگر GPU نباشد، خودش با رسترایزر نرم‌افزاری اجرا می‌شود.
EOF

  # --- 4. VERSION + MANIFEST (traceability) ---
  cat > "$stage/VERSION.txt" <<EOF
engine  $VERSION
game    $game
commit  $COMMIT$DIRTY
built   $(date -u '+%Y-%m-%dT%H:%M:%SZ')
EOF
  ( cd "$stage" && find . -type f ! -name MANIFEST.txt | sort | while read -r f; do
      printf '%s  %s\n' "$(sha256sum "$f" | awk '{print $1}')" "${f#./}"
    done ) > "$stage/MANIFEST.txt"

  # --- 5. Smoke-test the STAGED package, not the build tree ---
  say "smoke-testing the staged package"
  local sport=8791
  # Start it from inside the staged folder, exactly as a player would. Note
  # the `exec`: without it $! would be the subshell, not the game, and the
  # kill below would leave the real server running and holding the port.
  local pid
  ( cd "$stage" && exec ./kimia_world --port "$sport" --profiles ./profiles >/dev/null 2>&1 ) &
  pid=$!
  local menu="" tries=0
  while [ "$tries" -lt 40 ]; do
    menu="$(curl -s --max-time 1 "http://127.0.0.1:$sport/menu" 2>/dev/null || true)"
    [ -n "$menu" ] && break
    tries=$((tries + 1)); sleep 0.25
  done
  # Stop it for real: the game traps SIGINT/SIGTERM to shut down cleanly, so
  # give it a moment and then make sure it is gone (a leaked smoke-test
  # server would hold the port and confuse the next run).
  kill "$pid" 2>/dev/null
  local waited=0
  while kill -0 "$pid" 2>/dev/null && [ "$waited" -lt 20 ]; do sleep 0.25; waited=$((waited + 1)); done
  kill -9 "$pid" 2>/dev/null
  wait "$pid" 2>/dev/null
  kill -0 "$pid" 2>/dev/null && die "the smoke-test server on port $sport would not stop"
  case "$menu" in
    *'دنیای جدید'*) ok "the packaged game answers on /menu" ;;
    *) die "the staged package did not serve a menu (got: ${menu:-nothing})" ;;
  esac

  # --- Archive ---
  say "archiving"
  ( cd "$OUT_DIR" && tar czf "$name.tar.gz" "$name" ) || die "tar failed"
  local size; size="$(du -h "$OUT_DIR/$name.tar.gz" | awk '{print $1}')"
  ok "$OUT_DIR/$name.tar.gz ($size)"
}

mkdir -p "$OUT_DIR"
package_one "$GAME"

echo
echo "${BOLD}the offline release is ready:${RESET}"
echo "  $OUT_DIR"
echo
echo "to play it:"
echo "  tar xzf $OUT_DIR/kimia-$GAME-$VERSION.tar.gz"
echo "  cd kimia-$GAME-$VERSION && sh play.sh"
