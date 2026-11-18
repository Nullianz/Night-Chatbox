#!/usr/bin/env bash
# Turn an existing Arch Linux install into ReqriumOS.
# Run as your normal user (not root). It uses sudo for the system parts.
#
#   ./install.sh            install packages + desktop config
#   ./install.sh --no-pkgs  only copy the desktop config
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG="${XDG_CONFIG_HOME:-$HOME/.config}"
BACKUP="$HOME/.config-backup-reqrium-$(date +%Y%m%d-%H%M%S)"

say() { printf '\033[1;38;5;117m◈ %s\033[0m\n' "$*"; }

if [ "$(id -u)" -eq 0 ]; then
  echo "Run this as your normal user, not root." >&2
  exit 1
fi
if ! command -v pacman >/dev/null; then
  echo "This installer needs Arch Linux (pacman not found)." >&2
  exit 1
fi

if [ "${1:-}" != "--no-pkgs" ]; then
  say "Installing packages"
  mapfile -t pkgs < <(grep -vE '^\s*(#|$)' "$HERE/packages.txt")
  sudo pacman -Syu --needed --noconfirm "${pkgs[@]}"

  say "Enabling services"
  sudo systemctl enable NetworkManager.service bluetooth.service
  sudo install -Dm644 "$HERE/system/greetd.toml" /etc/greetd/config.toml
  # Replace any existing login manager (SDDM, GDM, LightDM...) with greetd
  sudo systemctl disable display-manager.service 2>/dev/null || true
  sudo systemctl enable greetd.service
  sudo install -Dm644 "$HERE/system/os-release" /etc/os-release
  sudo install -Dm755 "$HERE/system/reqrium-session" /usr/local/bin/reqrium-session
  sudo install -Dm644 "$HERE/system/reqrium.desktop" /usr/share/wayland-sessions/reqrium.desktop
fi

say "Installing desktop config (old files backed up to $BACKUP)"
for dir in hypr waybar rofi kitty mako reqrium; do
  if [ -e "$CONFIG/$dir" ]; then
    mkdir -p "$BACKUP"
    mv "$CONFIG/$dir" "$BACKUP/"
  fi
  cp -r "$HERE/dotfiles/$dir" "$CONFIG/$dir"
done
chmod +x "$CONFIG/reqrium/scripts/"*.sh

say "Applying GTK dark theme"
gsettings set org.gnome.desktop.interface gtk-theme 'adw-gtk3-dark' 2>/dev/null || true
gsettings set org.gnome.desktop.interface color-scheme 'prefer-dark' 2>/dev/null || true
gsettings set org.gnome.desktop.interface icon-theme 'Papirus-Dark' 2>/dev/null || true
gsettings set org.gnome.desktop.interface font-name 'Inter 11' 2>/dev/null || true

say "ReqriumOS is ready. Reboot, log in, and press SUPER+Space to open apps."
