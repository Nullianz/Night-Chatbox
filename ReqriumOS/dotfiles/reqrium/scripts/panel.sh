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

# if it exits within 20 seconds, try again (up to 5 times)
for attempt in 1 2 3 4 5; do
  start=$(date +%s)
  waybar >"$LOGS/waybar.log" 2>&1
  [ $(( $(date +%s) - start )) -ge 20 ] && break
  echo "panel exited early (attempt $attempt), restarting" >>"$LOGS/autostart.log"
  sleep 1
done
