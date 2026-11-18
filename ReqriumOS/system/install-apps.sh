#!/bin/sh
# Install the ReqriumOS apps (Welcome, Settings).  install-apps.sh SRC [ROOT]
set -e
src="$1"; root="${2:-}"
install -Dm644 "$src/system/apps/reqriumui.py" "$root/usr/lib/reqriumos/reqriumui.py"
for app in reqrium-settings reqrium-welcome; do
  install -Dm755 "$src/system/apps/$app" "$root/usr/local/bin/$app"
done
for d in org.reqrium.Settings org.reqrium.Welcome; do
  install -Dm644 "$src/system/apps/$d.desktop" "$root/usr/share/applications/$d.desktop"
done
