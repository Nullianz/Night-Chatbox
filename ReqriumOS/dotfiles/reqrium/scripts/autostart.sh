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

# Dark glass for every app. GTK3 apps on Wayland (file dialogs, the network
# editor...) read these settings, not the gtk-3.0/settings.ini file.
if command -v gsettings >/dev/null; then
  gsettings set org.gnome.desktop.interface color-scheme 'prefer-dark'
  gsettings set org.gnome.desktop.interface gtk-theme 'adw-gtk3-dark'
  gsettings set org.gnome.desktop.interface icon-theme 'Papirus-Dark'
  gsettings set org.gnome.desktop.interface cursor-theme 'capitaine-cursors-light'
  gsettings set org.gnome.desktop.interface cursor-size 24
  gsettings set org.gnome.desktop.interface font-name 'Inter 11'
  gsettings set org.gnome.desktop.interface monospace-font-name 'JetBrainsMono Nerd Font 11'
fi 2>/dev/null

# Login chime (Settings → Sound can turn it off)
~/.config/reqrium/scripts/sound.sh login &

# Check for updates now, then every 30 minutes (shown on the panel)
~/.config/reqrium/scripts/updates.sh watch >"$LOGS/updates.log" 2>&1 &

# the top panel (kept alive if it dies right after starting)
~/.config/reqrium/scripts/panel.sh
