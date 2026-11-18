#!/usr/bin/env bash
# Turn an existing Arch Linux install into ReqriumOS.
# Run as your normal user (not root); it asks for sudo when needed.
# To install onto an empty drive instead, use the ReqriumOS ISO.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
say() { printf '\033[1;38;5;117m◈ %s\033[0m\n' "$*"; }
yes_no() { local a; read -rp "  $1 [y/N] " a; [[ "$a" =~ ^[Yy] ]]; }

if [ "$(id -u)" -eq 0 ]; then
  echo "Run this as your normal user, not root." >&2; exit 1
fi
command -v pacman >/dev/null || { echo "This needs Arch Linux (pacman not found)." >&2; exit 1; }

say "ReqriumOS setup for $USER"
echo "  Optional extras:"
extras=()
yes_no "Gaming (Steam, GameMode, MangoHud)?"  && extras+=(gaming)
yes_no "Office (LibreOffice)?"                && extras+=(office)
yes_no "Printing (CUPS)?"                     && extras+=(printing)
yes_no "Developer tools (VS Code, Python, Node.js)?" && extras+=(dev)

EXTRAS="$(IFS=,; echo "${extras[*]}")"
sudo bash "$HERE/system/reqrium-configure" --src "$HERE" --user "$USER" --extras "$EXTRAS"

say "ReqriumOS is ready. Reboot, log in, and press SUPER+Space to open apps."
