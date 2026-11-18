#!/usr/bin/env bash
# Start the ReqriumOS glass dock, pinning the default apps on first run.
pinned="${XDG_CACHE_HOME:-$HOME/.cache}/nwg-dock-pinned"
if [ ! -s "$pinned" ]; then
  mkdir -p "$(dirname "$pinned")"
  printf '%s\n' firefox kitty thunar org.gnome.Loupe mpv io.missioncenter.MissionCenter \
    org.gnome.TextEditor > "$pinned"
fi
exec nwg-dock-hyprland -r -x -p bottom -i 44 -mb 12 -hd 0 -lp start \
  -ico view-app-grid-symbolic -c "rofi -show drun -theme ~/.config/rofi/grid.rasi"
