#!/usr/bin/env bash
# ReqriumOS wallpaper switcher with animated "liquid" transitions.
#   wallpaper.sh init   start the daemon and show the current wallpaper
#   wallpaper.sh next   ripple to the next wallpaper in the folder
#   wallpaper.sh set F  ripple to the wallpaper file F (used by Settings)
set -euo pipefail

DIR="${REQRIUM_WALLPAPERS:-$HOME/.config/reqrium/wallpapers}"
STATE="${XDG_STATE_HOME:-$HOME/.local/state}/reqrium/wallpaper"
mkdir -p "$(dirname "$STATE")"

# swww was renamed to awww upstream; support whichever is installed.
if command -v awww >/dev/null; then W=awww; else W=swww; fi

mapfile -t walls < <(find "$DIR" -maxdepth 1 -type f \
    \( -iname '*.png' -o -iname '*.jpg' -o -iname '*.jpeg' -o -iname '*.webp' -o -iname '*.gif' \) | sort)
[ "${#walls[@]}" -gt 0 ] || { echo "No wallpapers in $DIR" >&2; exit 1; }

current="$(cat "$STATE" 2>/dev/null || echo "${walls[0]}")"

case "${1:-init}" in
  init)
    pgrep -x "${W}-daemon" >/dev/null || { "${W}-daemon" & }
    # wait (up to ~15s on slow machines) until the daemon answers
    for _ in $(seq 1 60); do "$W" query >/dev/null 2>&1 && break; sleep 0.25; done
    [ -f "$current" ] || current="${walls[0]}"
    "$W" img "$current" --transition-type grow --transition-pos center \
        --transition-duration 1.6 --transition-fps 60 --transition-bezier .22,1,.36,1
    ;;
  next)
    next="${walls[0]}"
    for i in "${!walls[@]}"; do
      if [ "${walls[$i]}" = "$current" ]; then
        next="${walls[$(( (i + 1) % ${#walls[@]} ))]}"
      fi
    done
    # Ripple out from the mouse cursor
    pos="$(hyprctl cursorpos 2>/dev/null | tr -d ' ' || echo center)"
    "$W" img "$next" --transition-type wave --transition-angle 30 \
        --transition-wave 40,20 --transition-pos "${pos:-center}" --invert-y \
        --transition-duration 1.4 --transition-fps 60 --transition-bezier .34,1.56,.64,1
    current="$next"
    ;;
  set)
    [ -f "${2:-}" ] || { echo "No such file: ${2:-}" >&2; exit 1; }
    pos="$(hyprctl cursorpos 2>/dev/null | tr -d ' ' || echo center)"
    "$W" img "$2" --transition-type wave --transition-angle 30 \
        --transition-wave 40,20 --transition-pos "${pos:-center}" --invert-y \
        --transition-duration 1.4 --transition-fps 60 --transition-bezier .34,1.56,.64,1
    current="$2"
    ;;
  *) echo "usage: $0 [init|next|set FILE]" >&2; exit 2 ;;
esac

echo "$current" > "$STATE"
