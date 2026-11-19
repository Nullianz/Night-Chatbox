#!/usr/bin/env bash
# Start the wallpaper and the top panel once Hyprland is ready.
# On a slow first login they could start before Hyprland answered and quit,
# leaving no panel. Wait for Hyprland, then keep the panel alive if it dies
# right after starting. Output goes to ~/.local/state/reqrium/ for debugging.
LOGS="${XDG_STATE_HOME:-$HOME/.local/state}/reqrium"
mkdir -p "$LOGS"

# wait (up to ~30s) until Hyprland answers
for _ in $(seq 1 150); do hyprctl monitors >/dev/null 2>&1 && break; sleep 0.2; done

~/.config/reqrium/scripts/wallpaper.sh init >"$LOGS/wallpaper.log" 2>&1 &

# the panel: if it exits within 20 seconds, try again (up to 5 times)
for attempt in 1 2 3 4 5; do
  start=$(date +%s)
  waybar >"$LOGS/waybar.log" 2>&1
  [ $(( $(date +%s) - start )) -ge 20 ] && break
  echo "panel exited early (attempt $attempt), restarting" >>"$LOGS/autostart.log"
  sleep 1
done
