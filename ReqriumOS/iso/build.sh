#!/usr/bin/env bash
# Build the ReqriumOS live ISO. Must run as root on an Arch Linux machine.
#
#   sudo pacman -S archiso
#   sudo ./iso/build.sh            -> out/reqriumos-YYYY.MM.DD-x86_64.iso
#
# It starts from Arch's official "releng" profile (which provides the
# bootloaders and live-system plumbing) and layers ReqriumOS on top.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK="${WORK:-/tmp/reqriumos-build}"
OUT="${OUT:-$ROOT/out}"
PROFILE="$WORK/profile"
RELENG=/usr/share/archiso/configs/releng

[ "$(id -u)" -eq 0 ] || { echo "Run as root (sudo)." >&2; exit 1; }
[ -d "$RELENG" ] || { echo "archiso is not installed: sudo pacman -S archiso" >&2; exit 1; }

echo "◈ Preparing profile in $PROFILE"
rm -rf "$WORK"
mkdir -p "$WORK"
cp -r "$RELENG" "$PROFILE"

# --- branding -----------------------------------------------------------------
sed -i \
  -e 's/^iso_name=.*/iso_name="reqriumos"/' \
  -e 's/^iso_label=.*/iso_label="REQRIUM_$(date --date="@${SOURCE_DATE_EPOCH:-$(date +%s)}" +%Y%m)"/' \
  -e 's/^iso_publisher=.*/iso_publisher="ReqriumOS"/' \
  -e 's/^iso_application=.*/iso_application="ReqriumOS Live"/' \
  "$PROFILE/profiledef.sh"

# Make our scripts executable inside the image
sed -i '/^file_permissions=(/a\
  ["/usr/local/bin/reqrium-session"]="0:0:755"\
  ["/usr/local/bin/reqrium-postinstall"]="0:0:755"\
  ["/root/.config/reqrium/scripts/wallpaper.sh"]="0:0:755"\
  ["/root/.config/reqrium/scripts/powermenu.sh"]="0:0:755"' \
  "$PROFILE/profiledef.sh"

# --- packages -----------------------------------------------------------------
grep -vE '^\s*(#|$)' "$ROOT/packages.txt" >> "$PROFILE/packages.x86_64"
sort -u -o "$PROFILE/packages.x86_64" "$PROFILE/packages.x86_64"

# --- files in the live system -------------------------------------------------
A="$PROFILE/airootfs"
cp -r "$ROOT/iso/airootfs/." "$A/"
install -Dm644 "$ROOT/system/os-release"      "$A/etc/os-release"
install -Dm755 "$ROOT/system/reqrium-session" "$A/usr/local/bin/reqrium-session"
install -Dm644 "$ROOT/system/reqrium.desktop" "$A/usr/share/wayland-sessions/reqrium.desktop"
install -Dm755 "$ROOT/iso/reqrium-postinstall" "$A/usr/local/bin/reqrium-postinstall"

# Desktop config for the live root user
mkdir -p "$A/root/.config"
cp -r "$ROOT/dotfiles/." "$A/root/.config/"

# A copy of everything the post-install step needs
mkdir -p "$A/usr/share/reqriumos"
cp -r "$ROOT/dotfiles" "$ROOT/system" "$ROOT/packages.txt" "$A/usr/share/reqriumos/"

# Auto-start the glass desktop on tty1 (falls back to the shell if it exits)
cat >> "$A/root/.zlogin" <<'ZLOGIN'

# ReqriumOS: launch the desktop on the first console
if [ "$(tty)" = "/dev/tty1" ] && [ -z "${WAYLAND_DISPLAY:-}" ] && [ -e /dev/dri/card0 ] ; then
  reqrium-session || echo "Desktop exited. Type reqrium-session to try again."
fi
ZLOGIN

# --- build ----------------------------------------------------------------------
echo "◈ Building ISO (this takes a while)"
mkdir -p "$OUT"
mkarchiso -v -w "$WORK/work" -o "$OUT" "$PROFILE"
echo "◈ Done: $(ls -1 "$OUT"/reqriumos-*.iso | tail -n1)"
