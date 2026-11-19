#!/usr/bin/env bash
# Update checker for the panel.
#   updates.sh check    count updates now, update the panel, notify if new
#   updates.sh watch    check now, then every 30 minutes
#   updates.sh show     print the panel module's JSON (used by Waybar)
CACHE="${XDG_CACHE_HOME:-$HOME/.cache}/reqrium"
JSON="$CACHE/updates.json"
mkdir -p "$CACHE"

count_lines() { grep -c . || true; }

check() {
  # Arch packages (checkupdates syncs to a private copy, no root needed)
  sys=$(checkupdates 2>/dev/null | count_lines)
  aur=0; command -v paru >/dev/null && aur=$(paru -Qua 2>/dev/null | count_lines)
  flat=0; command -v flatpak >/dev/null && \
    flat=$(flatpak remote-ls --updates --columns=application 2>/dev/null | count_lines)
  os=0; [ "$(reqrium-update --check 2>/dev/null)" = available ] && os=1
  total=$(( sys + aur + flat + os ))

  if [ "$total" -gt 0 ]; then
    tip="Updates ready:\\n  System: $sys\\n  AUR: $aur\\n  Flatpak apps: $flat"
    [ "$os" = 1 ] && tip="$tip\\n  ReqriumOS: new version"
    printf '{"text":"󰚰 %s","tooltip":"%s\\n\\nClick to update","class":"pending"}\n' "$total" "$tip" >"$JSON"
  else
    printf '{"text":"","tooltip":"Up to date","class":"none"}\n' >"$JSON"
  fi
  pkill -RTMIN+8 waybar 2>/dev/null

  # notify only when the number goes up
  last=$(cat "$CACHE/updates.last" 2>/dev/null || echo 0)
  if [ "$total" -gt "$last" ]; then
    msg="$total updates are ready."
    [ "$os" = 1 ] && msg="$msg A new version of ReqriumOS is available."
    notify-send -a "ReqriumOS" -i system-software-update "Updates available" \
      "$msg Click the update icon on the panel to install them."
  fi
  echo "$total" >"$CACHE/updates.last"
}

case "${1:-check}" in
  check) check ;;
  watch) sleep 60; while true; do check; sleep 1800; done ;;
  show)  cat "$JSON" 2>/dev/null || echo '{"text":""}' ;;
esac
