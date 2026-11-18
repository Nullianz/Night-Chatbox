#!/usr/bin/env bash
# ReqriumOS power menu: full-screen glass tiles.
set -euo pipefail

choice="$(printf '%s\n' "󰌾  Lock" "󰍃  Log out" "󰤄  Sleep" "󰜉  Restart" "⏻  Shut down" |
    rofi -dmenu -i -theme ~/.config/rofi/powermenu.rasi \
         -mesg "See you soon, $(whoami)" -p "")" || exit 0

case "$choice" in
  *Lock)        hyprlock ;;
  *"Log out")   hyprctl dispatch 'hl.dsp.exit()' ;;
  *Sleep)       systemctl suspend ;;
  *Restart)     systemctl reboot ;;
  *"Shut down") systemctl poweroff ;;
esac
