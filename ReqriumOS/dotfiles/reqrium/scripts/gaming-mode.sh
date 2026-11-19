#!/usr/bin/env bash
# Gaming mode: switch off blur, shadows, animations and window effects, and
# use the performance power mode, for the most FPS.
#   gaming-mode.sh on | off | toggle | status   (status prints true/false)
set -u
LUA="${XDG_CONFIG_HOME:-$HOME/.config}/hypr/gaming.lua"
STATE="${XDG_STATE_HOME:-$HOME/.local/state}/reqrium/gaming"
mkdir -p "$(dirname "$STATE")"

is_on() { [ -f "$STATE" ]; }

turn_on() {
  cat >"$LUA" <<'LUA'
-- Gaming mode (written by gaming-mode.sh). Turn it off from the Control Center.
hl.config({
    decoration = {
        blur = { enabled = false },
        shadow = { enabled = false },
        wobble = { enabled = false },
        motion_blur = { enabled = false },
        active_opacity = 1.0,
        inactive_opacity = 1.0,
        dim_inactive = false,
    },
    animations = { enabled = false },
})
LUA
  # remember the power mode to restore it later
  powerprofilesctl get >"$STATE" 2>/dev/null || echo balanced >"$STATE"
  powerprofilesctl set performance 2>/dev/null
  hyprctl reload >/dev/null
  notify-send -a "ReqriumOS" -i input-gaming "Gaming mode on" \
    "Effects off and performance mode on. Turn it off from the Control Center."
}

turn_off() {
  previous="$(cat "$STATE" 2>/dev/null || echo balanced)"
  : >"$LUA"
  rm -f "$STATE"
  powerprofilesctl set "${previous:-balanced}" 2>/dev/null
  hyprctl reload >/dev/null
  notify-send -a "ReqriumOS" -i input-gaming "Gaming mode off" "Liquid glass is back."
}

case "${1:-toggle}" in
  on)     is_on || turn_on ;;
  off)    is_on && turn_off ;;
  toggle) if is_on; then turn_off; else turn_on; fi ;;
  status) is_on && echo true || echo false ;;
  *) echo "usage: $0 on|off|toggle|status" >&2; exit 2 ;;
esac
exit 0
