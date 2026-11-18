#!/bin/sh
# Write NoDisplay overrides for every app in hidden-apps.txt.
#   hide-apps.sh LIST [ROOT]   (ROOT defaults to /)
list="$1"; root="${2:-}"
dir="$root/usr/local/share/applications"
mkdir -p "$dir"
grep -vE '^\s*(#|$)' "$list" | while read -r app; do
  printf '[Desktop Entry]\nType=Application\nName=%s\nNoDisplay=true\n' "${app%.desktop}" > "$dir/$app"
done
