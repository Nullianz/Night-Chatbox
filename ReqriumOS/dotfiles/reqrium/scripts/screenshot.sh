#!/usr/bin/env bash
# Screenshots: "region" lets you drag a box, "screen" grabs everything.
# The shot opens in Satty so you can draw on it, then copy or save it.
set -euo pipefail
dir="$(xdg-user-dir PICTURES 2>/dev/null || echo "$HOME/Pictures")/Screenshots"
mkdir -p "$dir"
file="$dir/$(date +%Y-%m-%d_%H-%M-%S).png"

case "${1:-region}" in
  region) grim -g "$(slurp -b 0b0a1f88 -c 7dd3fcff -w 2)" - ;;
  screen) grim - ;;
esac | satty --filename - --output-filename "$file" --copy-command wl-copy \
             --early-exit --initial-tool brush
