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

# Boot menu entries (UEFI systemd-boot, GRUB and BIOS syslinux)
find "$PROFILE/efiboot" "$PROFILE/grub" "$PROFILE/syslinux" -type f \( -name '*.conf' -o -name '*.cfg' \) \
  -exec sed -i -e 's/Arch Linux install medium/ReqriumOS Live/g' \
               -e 's/Arch Linux/ReqriumOS/g' {} +

# Make our scripts executable inside the image
sed -i '/^file_permissions=(/a\
  ["/usr/local/bin/reqrium-session"]="0:0:755"\
  ["/usr/local/bin/reqrium-installer"]="0:0:755"\
  ["/usr/share/reqriumos/system/reqrium-configure"]="0:0:755"\
  ["/usr/share/reqriumos/system/reqrium-session"]="0:0:755"\
  ["/etc/sudoers.d/10-reqrium-live"]="0:0:440"\
  ["/home/reqrium/"]="1000:1000:755"' \
  "$PROFILE/profiledef.sh"

# --- packages -----------------------------------------------------------------
grep -vE '^\s*(#|$)' "$ROOT/packages.txt" >> "$PROFILE/packages.x86_64"
# Live-only extras: installer UI and graphics drivers for any GPU
printf '%s\n' gum vulkan-radeon vulkan-intel intel-media-driver >> "$PROFILE/packages.x86_64"
sort -u -o "$PROFILE/packages.x86_64" "$PROFILE/packages.x86_64"

# --- files in the live system -------------------------------------------------
A="$PROFILE/airootfs"
cp -r "$ROOT/iso/airootfs/." "$A/"
install -Dm644 "$ROOT/system/os-release"      "$A/etc/os-release"
install -Dm644 "$ROOT/system/mimeapps.list"   "$A/etc/xdg/mimeapps.list"
install -Dm644 "$ROOT/iso/reqrium-installer.desktop" "$A/usr/share/applications/reqrium-installer.desktop"
install -Dm755 "$ROOT/system/reqrium-session" "$A/usr/local/bin/reqrium-session"
install -Dm644 "$ROOT/system/reqrium.desktop" "$A/usr/share/wayland-sessions/reqrium.desktop"
install -Dm755 "$ROOT/iso/reqrium-installer" "$A/usr/local/bin/reqrium-installer"

# Live user. Hyprland refuses to run as root, so the live session logs in
# as "reqrium" (no password, passwordless sudo) instead.
install -Dm644 /dev/stdin "$A/etc/sysusers.d/reqrium-live.conf" <<'SYSUSERS'
u reqrium 1000 "ReqriumOS Live" /home/reqrium /usr/bin/zsh
m reqrium wheel
SYSUSERS
install -Dm440 /dev/stdin "$A/etc/sudoers.d/10-reqrium-live" <<'SUDO'
reqrium ALL=(ALL:ALL) NOPASSWD: ALL
SUDO
install -Dm644 /dev/stdin "$A/etc/systemd/system/getty@tty1.service.d/autologin.conf" <<'GETTY'
[Service]
ExecStart=
ExecStart=-/usr/bin/agetty --noreset --noclear --autologin reqrium - ${TERM}
GETTY

LIVE_HOME="$A/home/reqrium"
mkdir -p "$LIVE_HOME/.config"
cp -r "$ROOT/dotfiles/." "$LIVE_HOME/.config/"
# The live desktop greets you with the installer (loaded by hyprland.lua)
cat > "$LIVE_HOME/.config/hypr/live.lua" <<'HYPR'
-- Live USB only: open the installer on start, SUPER+I reopens it
local installer = 'kitty --title "Install ReqriumOS" sudo reqrium-installer'
hl.on("hyprland.start", function() hl.exec_cmd("sleep 3 && " .. installer) end)
hl.bind("SUPER + I", hl.dsp.exec_cmd(installer))
HYPR

# Start the glass desktop on the first console (falls back to the shell)
cat > "$LIVE_HOME/.zlogin" <<'ZLOGIN'
if [ "$(tty)" = "/dev/tty1" ] && [ -z "${WAYLAND_DISPLAY:-}" ] && [ -e /dev/dri/card0 ]; then
  REQRIUM_LIVE=1 reqrium-session || echo "The desktop could not start. Run: sudo reqrium-installer"
fi
ZLOGIN
touch "$LIVE_HOME/.zshrc"

# The ReqriumOS apps (Settings works on the live USB too)
sh "$ROOT/system/install-apps.sh" "$ROOT" "$A"

# Keep the live app grid clean too
sh "$ROOT/system/hide-apps.sh" "$ROOT/system/hidden-apps.txt" "$A"

# Everything the disk installer copies onto the new system
mkdir -p "$A/usr/share/reqriumos"
cp -r "$ROOT/dotfiles" "$ROOT/system" "$ROOT/packages.txt" "$A/usr/share/reqriumos/"

# --- build ----------------------------------------------------------------------
echo "◈ Building ISO (this takes a while)"
mkdir -p "$OUT"
mkarchiso -v -w "$WORK/work" -o "$OUT" "$PROFILE"
echo "◈ Done: $(ls -1 "$OUT"/reqriumos-*.iso | tail -n1)"
