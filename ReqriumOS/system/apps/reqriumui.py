"""Shared look and helpers for the ReqriumOS apps (Welcome, Settings)."""

import json
import os
import shutil
import subprocess

import gi

gi.require_version("Gtk", "4.0")
gi.require_version("Adw", "1")
from gi.repository import Adw, Gdk, Gtk  # noqa: E402

HOME = os.path.expanduser("~")
CONFIG_DIR = os.path.join(HOME, ".config", "reqrium")
SETTINGS_FILE = os.path.join(CONFIG_DIR, "settings.json")
HYPR_SETTINGS = os.path.join(HOME, ".config", "hypr", "reqrium-settings.lua")
WALLPAPERS = os.path.join(CONFIG_DIR, "wallpapers")
SCRIPTS = os.path.join(CONFIG_DIR, "scripts")
LOGO = "/usr/share/plymouth/themes/reqrium/logo.png"
LOGO_FALLBACK = "/usr/share/reqriumos/system/plymouth/reqrium/logo.png"

DEFAULTS = {
    "glass": "acrylic",
    "opacity": 0.90,
    "rounding": 18,
    "animations": True,
    "wobble": True,
    "motion_blur": True,
    "dock": True,
    "scale": "auto",
    "kb_layout": "",
    "show_welcome": True,
    "sounds": True,
}

# Hyprland's blur.variant numbers
GLASS_STYLES = [
    ("acrylic", "Liquid glass", 8),
    ("prism", "Prism", 6),
    ("frost", "Frosted", 1),
    ("water", "Water", 4),
    ("aurora", "Aurora", 9),
    ("haze", "Haze", 10),
    ("kawase", "Plain blur", 0),
]

CSS = """
/* ReqriumOS glass for GTK4 / libadwaita apps. Hyprland blurs the window,
   so translucent backgrounds turn into frosted glass. */
:root {
  --accent-bg-color: #8b7cf6;
  --accent-color: #c4b5fd;
  --accent-fg-color: #ffffff;
  --window-bg-color: rgba(18, 14, 44, 0.62);
  --view-bg-color: transparent;
  --headerbar-bg-color: transparent;
  --sidebar-bg-color: rgba(255, 255, 255, 0.04);
  --card-bg-color: rgba(255, 255, 255, 0.07);
  --popover-bg-color: rgba(30, 24, 70, 0.92);
  --dialog-bg-color: rgba(22, 18, 54, 0.9);
}
window.background, .background {
  background: linear-gradient(160deg, rgba(196, 181, 253, 0.12), rgba(16, 12, 40, 0.66) 50%,
                              rgba(125, 211, 252, 0.10));
}
headerbar { background: transparent; box-shadow: none; }
.navigation-sidebar { background: transparent; }
.navigation-sidebar row { border-radius: 14px; margin: 2px 6px; }
.navigation-sidebar row:selected {
  background: linear-gradient(90deg, rgba(196, 181, 253, 0.35), rgba(125, 211, 252, 0.25));
}
.boxed-list, list.boxed-list {
  background: rgba(255, 255, 255, 0.06);
  border: 1px solid rgba(255, 255, 255, 0.14);
  border-radius: 18px;
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.18);
}
.boxed-list > row { background: transparent; }
button.pill, button.suggested-action {
  border-radius: 999px;
}
button.suggested-action {
  background: linear-gradient(90deg, #c4b5fd, #7dd3fc);
  color: #0b0a1f;
  font-weight: 700;
}
button.glass {
  background: rgba(255, 255, 255, 0.08);
  border: 1px solid rgba(255, 255, 255, 0.16);
  border-radius: 18px;
}
button.glass:hover { background: rgba(255, 255, 255, 0.16); }
button.glass:checked {
  background: linear-gradient(135deg, rgba(196, 181, 253, 0.55), rgba(125, 211, 252, 0.45));
}
.hero-title { font-size: 34px; font-weight: 800; }
.hero-sub { font-size: 16px; opacity: 0.75; }
.keycap {
  background: rgba(255, 255, 255, 0.10);
  border: 1px solid rgba(255, 255, 255, 0.22);
  border-bottom-width: 3px;
  border-radius: 9px;
  padding: 3px 9px;
  font-family: "JetBrainsMono Nerd Font", monospace;
  font-weight: 700;
}
.wallpaper-tile { border-radius: 16px; padding: 4px; }
.wallpaper-tile picture { border-radius: 12px; }
.wallpaper-tile.current { background: linear-gradient(135deg, #c4b5fd, #7dd3fc); }
.app-tile { padding: 14px 8px; border-radius: 20px; }
.app-tile label.title { font-weight: 700; }
.dim { opacity: 0.65; }
/* dialogs (e.g. the Wi-Fi password) are more solid glass so text stays readable */
dialog floating-sheet, dialog floating-sheet > sheet {
  background-color: rgba(24, 20, 58, 0.96);
}
dialog.alert floating-sheet, dialog floating-sheet {
  border: 1px solid rgba(255, 255, 255, 0.18);
  border-radius: 24px;
  box-shadow: 0 18px 50px rgba(5, 3, 26, 0.6);
}
"""


def setup_style():
    """Dark theme plus the glass stylesheet."""
    Adw.StyleManager.get_default().set_color_scheme(Adw.ColorScheme.FORCE_DARK)
    provider = Gtk.CssProvider()
    provider.load_from_string(CSS)
    Gtk.StyleContext.add_provider_for_display(
        Gdk.Display.get_default(), provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION + 1
    )


def logo_widget(size=128):
    path = LOGO if os.path.exists(LOGO) else LOGO_FALLBACK
    img = Gtk.Image.new_from_file(path)
    img.set_pixel_size(size)
    img.set_halign(Gtk.Align.CENTER)
    return img


def load_settings():
    data = dict(DEFAULTS)
    try:
        with open(SETTINGS_FILE) as f:
            data.update(json.load(f))
    except (OSError, ValueError):
        pass
    return data


def save_settings(data):
    os.makedirs(CONFIG_DIR, exist_ok=True)
    with open(SETTINGS_FILE, "w") as f:
        json.dump(data, f, indent=2)


def lua_bool(v):
    return "true" if v else "false"


def write_hypr_settings(s):
    """Turn settings.json into a Lua file that hyprland.lua loads last."""
    variant = dict((k, n) for k, _, n in GLASS_STYLES).get(s["glass"], 8)
    opacity = max(0.5, min(1.0, float(s["opacity"])))
    lines = [
        "-- Written by ReqriumOS Settings. Change these in the Settings app.",
        "reqrium = reqrium or {}",
        f"reqrium.dock = {lua_bool(s['dock'])}",
        "hl.config({",
        "    decoration = {",
        f"        rounding = {int(s['rounding'])},",
        f"        active_opacity = {opacity:.2f},",
        f"        inactive_opacity = {max(0.4, opacity - 0.10):.2f},",
        f"        blur = {{ variant = {variant} }},",
        f"        wobble = {{ enabled = {lua_bool(s['wobble'])} }},",
        f"        motion_blur = {{ enabled = {lua_bool(s['motion_blur'])} }},",
        "    },",
        f"    animations = {{ enabled = {lua_bool(s['animations'])} }},",
        "})",
    ]
    if s.get("scale", "auto") != "auto":
        lines.append(
            'hl.monitor({ output = "", mode = "preferred", position = "auto", '
            f'scale = {float(s["scale"])} }})'
        )
    if s.get("kb_layout"):
        layout = "".join(c for c in s["kb_layout"] if c.isalnum() or c in ",_-")
        lines.append(f'hl.config({{ input = {{ kb_layout = "{layout}" }} }})')
    os.makedirs(os.path.dirname(HYPR_SETTINGS), exist_ok=True)
    with open(HYPR_SETTINGS, "w") as f:
        f.write("\n".join(lines) + "\n")


def apply_settings(s):
    save_settings(s)
    write_hypr_settings(s)
    run(["hyprctl", "reload"])


def run(cmd, wait=False):
    """Run a command without blocking the UI (unless wait=True)."""
    try:
        if wait:
            return subprocess.run(cmd, capture_output=True, text=True, timeout=10).stdout
        subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                         start_new_session=True)
    except (OSError, subprocess.SubprocessError):
        return ""
    return ""


def have(cmd):
    return shutil.which(cmd) is not None


def in_terminal(title, script):
    """Run a shell script in a glass kitty window and keep it open at the end."""
    full = f'{script}\necho; read -rp "Done. Press Enter to close this window. " _'
    run(["kitty", "--title", title, "bash", "-c", full])
