#!/usr/bin/env bash
# KIMIA — GPU enablement/diagnostic for a PlayStation 4 running Linux.
#
#   bash Tools/ps4_gpu.sh            # report the driver stack + run the GPU probe
#   bash Tools/ps4_gpu.sh --install  # also try to install the needed Mesa bits
#
# What "make the GPU work" actually means on a PS4:
#   1. a kernel whose amdgpu (or radeon) driver knows the PS4's "Liverpool"
#      GCN chip (patched kernels: 5.15 for Belize, 6.6 by crashniels, ...);
#   2. a Mesa build whose RadeonSI Gallium3D driver also knows Liverpool;
#   3. libGL + libEGL on disk so the engine's dlopen path finds them.
#
# The engine itself needs nothing: when those three are true, `kimia_world`
# picks the hardware GL path on its own. This script just proves it and, with
# --install, installs the user-space side (Mesa/libGL) when a package manager
# is available. The kernel side must come from your PS4 distro's kernel.
set -u

BOLD=$'\033[1m'; RED=$'\033[31m'; GREEN=$'\033[32m'; YELLOW=$'\033[33m'; RESET=$'\033[0m'
say()  { printf '%s==>%s %s\n' "$BOLD" "$RESET" "$*"; }
ok()   { printf '%s ok %s %s\n' "$GREEN" "$RESET" "$*"; }
warn() { printf '%s !! %s %s\n' "$YELLOW" "$RESET" "$*"; }
die()  { printf '%s XX %s %s\n' "$RED" "$RESET" "$*" >&2; exit 1; }

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT" || die "cannot cd to $ROOT"
DO_INSTALL=0
for arg in "$@"; do
  case "$arg" in
    --install) DO_INSTALL=1 ;;
    -h|--help) sed -n '2,24p' "$0"; exit 0 ;;
    *) die "unknown option: $arg (try --help)" ;;
  esac
done

WORLD="$ROOT/build-ps4/bin/kimia_world"
if [ ! -x "$WORLD" ]; then
  say "kimia_world not built yet — building the headless path first"
  bash "$ROOT/Tools/ps4_build.sh" || die "build failed; fix that before chasing the GPU"
fi

echo
say "1. Kernel driver (the GPU chip must be claimed by amdgpu/radeon)"
if [ -d /sys/class/drm ]; then
  for card in /sys/class/drm/card*; do
    [ -e "$card" ] || continue
    printf '   %s\n' "$(basename "$card")"
  done
else
  warn "no /sys/class/drm — the kernel has no DRM/GPU driver loaded at all"
fi
if command -v lsmod >/dev/null 2>&1; then
  lsmod 2>/dev/null | grep -E '^(amdgpu|radeon)\b' || warn "amdgpu/radeon module not loaded (see the kernel note below)"
else
  warn "lsmod missing — cannot list modules on this distro"
fi
if command -v lspci >/dev/null 2>&1; then
  lspci -nn 2>/dev/null | grep -iE 'vga|3d|display' || warn "lspci found no display device"
fi

echo
say "2. User-space GL (Mesa's libGL + libEGL that the engine dlopens)"
HAVE_GL=0; HAVE_EGL=0
if command -v ldconfig >/dev/null 2>&1; then
  ldconfig -p 2>/dev/null | grep -q 'libGL.so.1'   && HAVE_GL=1
  ldconfig -p 2>/dev/null | grep -q 'libEGL.so.1'  && HAVE_EGL=1
else
  ls /usr/lib*/libGL.so.1 /usr/lib*/libEGL.so.1 >/dev/null 2>&1 && { HAVE_GL=1; HAVE_EGL=1; }
fi
[ "$HAVE_GL"  = 1 ] && ok "libGL.so.1 found"    || warn "libGL.so.1 NOT found — Mesa is not installed"
[ "$HAVE_EGL" = 1 ] && ok "libEGL.so.1 found"   || warn "libEGL.so.1 NOT found — Mesa EGL is not installed"

echo
say "3. The engine's own probe (this is the verdict that matters)"
set +e
"$WORLD" --gpuinfo
PROBE=$?
set -e

echo
if [ "$PROBE" -eq 0 ]; then
  ok "GPU is LIVE — kimia_world will render with hardware OpenGL."
  exit 0
fi

warn "the GPU is not reachable yet. The fix is two-sided:"
echo "   (a) KERNEL — use a PS4 kernel whose amdgpu/radeon knows Liverpool:"
echo "       Belize boards: kernel 5.15 (codedwrench) or 6.6 (crashniels);"
echo "       Aeolia/Baikal boards have their own builds. Your PS4 distro ships one."
echo "   (b) USERSPACE — install Mesa (RadeonSI knows Liverpool via the ps4-"
echo "       patched builds; the psxita repo ships mesa-git for arch-based PS4):"
echo
if command -v pacman >/dev/null 2>&1; then
  echo "       # arch-based (psxitarch):"
  echo "       sudo pacman -S --needed mesa libgl libegl"
  echo "       # and, for the Liverpool-patched RadeonSI, the psxita repo (see"
  echo "       # https://github.com/Ps3itaTeam/ps4linux-video-drivers):"
  echo "       #   [ps4] / SigLevel = Never / Server = https://psxita.it/repo-testing"
  echo "       #   sudo pacman -Syu && sudo pacman -S mesa-git libdrm-git xf86-video-amdgpu-git"
elif command -v apt-get >/dev/null 2>&1; then
  echo "       # debian/ubuntu-based:"
  echo "       sudo apt-get update && sudo apt-get install -y mesa-utils libgl1 libegl1"
elif command -v dnf >/dev/null 2>&1; then
  echo "       # fedora-based:"
  echo "       sudo dnf install -y mesa-libGL mesa-libEGL mesa-dri-drivers"
else
  echo "       (no package manager detected — install Mesa/libGL/libEGL by hand)"
fi
echo
if [ "$DO_INSTALL" = 1 ]; then
  say "attempting the user-space install (kernel side is yours to choose)"
  if command -v pacman >/dev/null 2>&1; then
    sudo pacman -S --needed --noconfirm mesa libgl libegl || warn "pacman install failed"
  elif command -v apt-get >/dev/null 2>&1; then
    sudo apt-get update -y || true
    sudo apt-get install -y mesa-utils libgl1 libegl1 || warn "apt install failed"
  elif command -v dnf >/dev/null 2>&1; then
    sudo dnf install -y mesa-libGL mesa-libEGL mesa-dri-drivers || warn "dnf install failed"
  else
    warn "no package manager to install with"
  fi
  echo
  say "re-running the probe after install"
  "$WORLD" --gpuinfo || warn "still no GPU — the kernel driver is the remaining piece"
fi

echo
say "Note: even with a live GPU, the PS4's Jaguar CPU stays slow; the GPU"
say "      accelerates rendering, not the build. See Documentation/PS4.md."
exit "$PROBE"
