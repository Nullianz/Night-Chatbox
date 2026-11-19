#!/usr/bin/env bash
# Start (or restart) the top panel, and keep it alive if it dies right after
# starting. Used at login, after updates, and by Settings → Restart the panel.
#   panel.sh            start the panel
#   panel.sh restart    stop the running panel first, then start it
LOGS="${XDG_STATE_HOME:-$HOME/.local/state}/reqrium"
mkdir -p "$LOGS"
CONF="$HOME/.config/waybar/config.jsonc"

if [ "${1:-}" = restart ]; then
  # stop only the top panel (the dock is a separate Waybar), and wait for it
  pkill -f "^waybar$" ; pkill -f "^waybar -c $CONF"
  for _ in $(seq 1 50); do pgrep -f "^waybar$" >/dev/null || break; sleep 0.1; done
fi

# Is the panel actually on screen? (Waybar's layer is called "waybar"; the
# dock's is "dock".) On a slow start Waybar can time out waiting for
# Hyprland and keep running without ever appearing, so check, not just wait.
on_screen() { hyprctl layers 2>/dev/null | grep -q "namespace: waybar,"; }

for attempt in 1 2 3 4 5; do
  waybar >"$LOGS/waybar.log" 2>&1 &
  pid=$!
  for _ in $(seq 1 40); do
    on_screen && break
    kill -0 "$pid" 2>/dev/null || break
    sleep 0.5
  done
  if on_screen; then
    wait "$pid"          # panel is up: stay with it until it exits
    exit 0
  fi
  echo "panel not on screen (attempt $attempt), restarting" >>"$LOGS/autostart.log"
  kill "$pid" 2>/dev/null
  wait "$pid" 2>/dev/null
  sleep 1
done
