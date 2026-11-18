#!/usr/bin/env bash
# ReqriumOS power menu, shown in the glass launcher.
set -euo pipefail

choice="$(printf '%s\n' \
    "  Lock" "󰍃  Log out" "󰤄  Sleep" "  Restart" "⏻  Shut down" |
    rofi -dmenu -i -p "Power" -theme ~/.config/rofi/glass.rasi \
         -theme-str 'window { width: 320px; } listview { lines: 5; }')" || exit 0

case "$choice" in
  *Lock)      hyprlock ;;
  *"Log out") hyprctl dispatch exit ;;
  *Sleep)     systemctl suspend ;;
  *Restart)   systemctl reboot ;;
  *"Shut down") systemctl poweroff ;;
esac
