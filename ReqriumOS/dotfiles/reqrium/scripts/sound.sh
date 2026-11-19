#!/usr/bin/env bash
# Play a ReqriumOS sound:  sound.sh login | notify
# Respects "sounds": false in ~/.config/reqrium/settings.json.
SOUNDS=/usr/share/sounds/reqriumos
settings="$HOME/.config/reqrium/settings.json"
if [ -f "$settings" ] && grep -q '"sounds": *false' "$settings"; then exit 0; fi
file="$SOUNDS/${1:-notify}.wav"
[ -f "$file" ] || exit 0
# never fail: a failing sound must not cause another notification
if command -v pw-play >/dev/null; then pw-play --volume 0.6 "$file" 2>/dev/null
elif command -v paplay >/dev/null; then paplay "$file" 2>/dev/null
fi
exit 0
