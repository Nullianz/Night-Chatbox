--  ____                  _                  ___  ____
-- |  _ \ ___  __ _ _ __ (_)_   _ _ __ ___  / _ \/ ___|
-- | |_) / _ \/ _` | '__|| | | | | '_ ` _ \| | | \___ \
-- |  _ <  __/ (_| | |   | | |_| | | | | | | |_| |___) |
-- |_| \_\___|\__, |_|   |_|\__,_|_| |_| |_|\___/|____/
--               |_|        Liquid Glass Desktop
--
-- Hyprland (0.57+) Lua config for ReqriumOS.
-- Option reference: https://wiki.hypr.land/configuring/

local terminal = "kitty"
local files    = "thunar"
local browser  = "firefox"
local launcher = "rofi -show drun -theme ~/.config/rofi/glass.rasi"
local scripts  = "~/.config/reqrium/scripts"
local mod      = "SUPER"

-- Machines without a real GPU (most VMs without 3D acceleration) render
-- with the CPU, so reqrium-session sets REQRIUM_LITE=1 there and we use a
-- lighter glass that stays smooth.
local lite = os.getenv("REQRIUM_LITE") == "1"

-- The live USB sets REQRIUM_LIVE=1. Its user has no password, so it must
-- never auto-lock (you could not unlock it again).
local live = os.getenv("REQRIUM_LIVE") == "1"

-- Blur variants, in Hyprland's order. "acrylic" gives refraction, a lens
-- bulge at the edges and a hint of chromatic aberration: liquid glass.
local BLUR = { kawase = 0, frost = 1, ripple = 2, drops = 3, water = 4,
               fluid_jar = 5, prism = 6, heat_shimmer = 7, acrylic = 8,
               aurora = 9, haze = 10 }

------------------
---- MONITORS ----
------------------
hl.monitor({ output = "", mode = "preferred", position = "auto", scale = "auto" })

-------------------
---- AUTOSTART ----
-------------------
hl.on("hyprland.start", function()
    hl.exec_cmd(scripts .. "/wallpaper.sh init")
    hl.exec_cmd("waybar")
    hl.exec_cmd("swaync")                      -- Control Center + notifications
    hl.exec_cmd("swayosd-server")              -- glass volume/brightness pop-ups
    hl.exec_cmd(scripts .. "/dock.sh")         -- glass dock
    if not live then hl.exec_cmd("hypridle") end   -- dim, lock and sleep when idle
    hl.exec_cmd("systemctl --user start hyprpolkitagent")
    hl.exec_cmd("nm-applet --indicator")
    hl.exec_cmd("blueman-applet")
    hl.exec_cmd("udiskie --tray")
    hl.exec_cmd("xdg-user-dirs-update")
    hl.exec_cmd("wl-paste --watch cliphist store")
end)

-----------------------
---- ENVIRONMENT ------
-----------------------
hl.env("XCURSOR_SIZE", "24")
hl.env("HYPRCURSOR_SIZE", "24")
hl.env("QT_QPA_PLATFORM", "wayland;xcb")
hl.env("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1")
hl.env("GDK_BACKEND", "wayland,x11")
hl.env("MOZ_ENABLE_WAYLAND", "1")
hl.env("XDG_CURRENT_DESKTOP", "Hyprland")
hl.env("XDG_SESSION_DESKTOP", "ReqriumOS")

----------------------
---- LIQUID GLASS ----
----------------------
hl.config({
    general = {
        gaps_in  = 6,
        gaps_out = 14,
        border_size = 2,
        col = {
            -- A light "glass edge" that slowly rotates around the active window
            active_border   = { colors = { "rgba(c4b5fdee)", "rgba(7dd3fcee)", "rgba(f0abfcee)" }, angle = 45 },
            inactive_border = "rgba(ffffff22)",
        },
        resize_on_border = true,
        layout = "dwindle",
    },

    decoration = {
        rounding       = 18,
        rounding_power = 3.0,        -- squircle-style corners

        active_opacity     = 0.90,
        inactive_opacity   = 0.80,
        fullscreen_opacity = 1.0,
        dim_inactive = true,
        dim_strength = 0.06,

        blur = {
            enabled = true,
            variant = lite and BLUR.kawase or BLUR.acrylic,
            size    = lite and 5 or 9,
            passes  = lite and 2 or 4,
            noise   = 0.018,         -- subtle grain so it doesn't look like plastic
            contrast   = 1.15,
            brightness = 1.05,
            vibrancy          = 0.42,  -- lets the colours behind the glass glow through
            vibrancy_darkness = 0.25,
            popups = true,
            popups_ignorealpha = 0.2,
            new_optimizations = true,
            xray = false,
            acrylic = {
                refraction = 18,     -- how far the glass bends what is behind it
                bulb       = 40,     -- width (px) of the curved, lens-like glass edge
                clarity    = 0.75,
                aberration = 0.06,   -- faint rainbow fringe at the edges
                tint       = "rgba(c4b5fd14)",
            },
        },

        shadow = {
            enabled = true,
            range = 32,
            render_power = 3,
            offset = { 0, 8 },
            color = "rgba(05031aaa)",
            color_inactive = "rgba(05031a55)",
        },

        -- Windows jiggle like jelly while you drag or resize them
        wobble = {
            enabled   = not lite,
            stiffness = 180,
            damping   = 14,
            mass      = 1.0,
            intensity = 0.5,
        },
        motion_blur = { enabled = not lite },
    },

    animations = { enabled = true },

    dwindle = { preserve_split = true },

    misc = {
        disable_hyprland_logo    = true,
        disable_splash_rendering = true,
        force_default_wallpaper  = 0,
        background_color = "rgb(0b0a1f)",
        animate_manual_resizes       = true,
        animate_mouse_windowdragging = true,
    },

    input = {
        kb_layout = "us",
        follow_mouse = 1,
        sensitivity = 0,
        touchpad = {
            natural_scroll = true,
            tap_to_click   = true,
        },
    },
})

--------------------
---- ANIMATIONS ----
--------------------
-- Springs give real physics: things overshoot and settle like liquid.
hl.curve("jelly",  { type = "spring", mass = 1, stiffness = 220, damping = 16 })
hl.curve("settle", { type = "spring", mass = 1, stiffness = 170, damping = 22 })
hl.curve("liquid", { type = "bezier", points = { {0.22, 1.00}, {0.36, 1.00} } })
hl.curve("drip",   { type = "bezier", points = { {0.55, 0.00}, {0.10, 1.00} } })
hl.curve("smooth", { type = "bezier", points = { {0.25, 0.10}, {0.25, 1.00} } })
hl.curve("linear", { type = "bezier", points = { {0, 0}, {1, 1} } })

-- Windows pop in like a droplet and sink away on close
hl.animation({ leaf = "windows",       enabled = true, speed = 5,  spring = "settle" })
hl.animation({ leaf = "windowsIn",     enabled = true, speed = 5,  spring = "jelly",  style = "popin 70%" })
hl.animation({ leaf = "windowsOut",    enabled = true, speed = 4,  bezier = "drip",   style = "popin 85%" })
hl.animation({ leaf = "windowsMove",   enabled = true, speed = 5,  spring = "settle" })
hl.animation({ leaf = "fadeIn",        enabled = true, speed = 5,  bezier = "smooth" })
hl.animation({ leaf = "fadeOut",       enabled = true, speed = 4,  bezier = "smooth" })
hl.animation({ leaf = "fadeSwitch",    enabled = true, speed = 6,  bezier = "smooth" })
hl.animation({ leaf = "fadeDim",       enabled = true, speed = 6,  bezier = "smooth" })

-- Panel, launcher and notifications float in
hl.animation({ leaf = "layersIn",      enabled = true, speed = 4,  spring = "jelly",  style = "popin 90%" })
hl.animation({ leaf = "layersOut",     enabled = true, speed = 3,  bezier = "drip",   style = "fade" })
hl.animation({ leaf = "fadeLayersIn",  enabled = true, speed = 4,  bezier = "smooth" })
hl.animation({ leaf = "fadeLayersOut", enabled = true, speed = 3,  bezier = "smooth" })

-- The rotating gradient border ("light moving through glass")
hl.animation({ leaf = "border",        enabled = true, speed = 10, bezier = "smooth" })
hl.animation({ leaf = "borderangle",   enabled = true, speed = 90, bezier = "linear", style = "loop" })

-- Workspaces glide vertically with a fade
hl.animation({ leaf = "workspaces",       enabled = true, speed = 6, bezier = "liquid", style = "slidefadevert 25%" })
hl.animation({ leaf = "specialWorkspace", enabled = true, speed = 5, spring = "jelly",  style = "slidefadevert 40%" })

---------------------
---- GLASS LAYERS ---
---------------------
-- Panel, launcher, notifications and power menu become real glass
for _, ns in ipairs({ "waybar", "rofi", "swaync-control-center", "swaync-notification-window",
                      "nwg-dock", "swayosd" }) do
    hl.layer_rule({
        name  = "glass-" .. ns,
        match = { namespace = "^" .. ns .. "$" },
        blur  = true,
        ignore_alpha = true,
    })
end

-- Video players stay fully opaque
hl.window_rule({
    name  = "opaque-video",
    match = { class = "^(mpv)$" },
    opacity = "1.0 override 1.0 override",
})

hl.window_rule({
    name  = "suppress-maximize-events",
    match = { class = ".*" },
    suppress_event = "maximize",
})

------------------
---- GESTURES ----
------------------
hl.gesture({ fingers = 3, direction = "vertical", action = "workspace" })

------------------
---- KEYBINDS ----
------------------
local function key(combo) return mod .. " + " .. combo end

hl.bind(key("Return"), hl.dsp.exec_cmd(terminal))
hl.bind(key("Space"),  hl.dsp.exec_cmd(launcher))
hl.bind(key("E"),      hl.dsp.exec_cmd(files))
hl.bind(key("B"),      hl.dsp.exec_cmd(browser))
hl.bind(key("D"),      hl.dsp.window.close())   -- close the focused window
hl.bind(key("Q"),      hl.dsp.window.close())
hl.bind(key("F"),      hl.dsp.window.fullscreen())
hl.bind(key("V"),      hl.dsp.window.float({ action = "toggle" }))
hl.bind(key("P"),      hl.dsp.window.pseudo())
hl.bind(key("J"),      hl.dsp.layout("togglesplit"))
hl.bind(key("L"),      hl.dsp.exec_cmd("hyprlock"))
hl.bind(key("W"),      hl.dsp.exec_cmd(scripts .. "/wallpaper.sh next"))
hl.bind(key("C"),      hl.dsp.exec_cmd("cliphist list | rofi -dmenu -theme ~/.config/rofi/glass.rasi | cliphist decode | wl-copy"))
hl.bind(key("SHIFT + E"), hl.dsp.exec_cmd(scripts .. "/powermenu.sh"))
hl.bind(key("A"),         hl.dsp.exec_cmd("rofi -show drun -theme ~/.config/rofi/grid.rasi"))
hl.bind(key("N"),         hl.dsp.exec_cmd("swaync-client -t -sw"))
hl.bind(key("SHIFT + C"), hl.dsp.exec_cmd("hyprpicker -a"))
hl.bind(key("SHIFT + S"), hl.dsp.exec_cmd(scripts .. "/screenshot.sh region"))
hl.bind("Print",          hl.dsp.exec_cmd(scripts .. "/screenshot.sh screen"))

-- Focus
hl.bind(key("left"),  hl.dsp.focus({ direction = "left" }))
hl.bind(key("right"), hl.dsp.focus({ direction = "right" }))
hl.bind(key("up"),    hl.dsp.focus({ direction = "up" }))
hl.bind(key("down"),  hl.dsp.focus({ direction = "down" }))

-- Workspaces 1-9
for i = 1, 9 do
    hl.bind(key(i),             hl.dsp.focus({ workspace = i }))
    hl.bind(key("SHIFT + " .. i), hl.dsp.window.move({ workspace = i }))
end
hl.bind(key("mouse_down"), hl.dsp.focus({ workspace = "e+1" }))
hl.bind(key("mouse_up"),   hl.dsp.focus({ workspace = "e-1" }))

-- The "glass drawer" (scratchpad)
hl.bind(key("S"),       hl.dsp.workspace.toggle_special("drawer"))
hl.bind(key("ALT + S"), hl.dsp.window.move({ workspace = "special:drawer" }))

-- Drag / resize with the mouse
hl.bind(key("mouse:272"), hl.dsp.window.drag(),   { mouse = true })
hl.bind(key("mouse:273"), hl.dsp.window.resize(), { mouse = true })

-- Media and hardware keys
local held = { locked = true, repeating = true }
-- (swayosd shows a glass pop-up for each change)
local osd = "swayosd-client "
hl.bind("XF86AudioRaiseVolume",  hl.dsp.exec_cmd(osd .. "--output-volume raise --max-volume 100"), held)
hl.bind("XF86AudioLowerVolume",  hl.dsp.exec_cmd(osd .. "--output-volume lower"), held)
hl.bind("XF86AudioMute",         hl.dsp.exec_cmd(osd .. "--output-volume mute-toggle"), held)
hl.bind("XF86AudioMicMute",      hl.dsp.exec_cmd(osd .. "--input-volume mute-toggle"), held)
hl.bind("XF86MonBrightnessUp",   hl.dsp.exec_cmd(osd .. "--brightness raise"), held)
hl.bind("XF86MonBrightnessDown", hl.dsp.exec_cmd(osd .. "--brightness lower"), held)
hl.bind("XF86AudioPlay", hl.dsp.exec_cmd(osd .. "--playerctl play-pause"), { locked = true })
hl.bind("XF86AudioNext", hl.dsp.exec_cmd(osd .. "--playerctl next"),       { locked = true })
hl.bind("XF86AudioPrev", hl.dsp.exec_cmd(osd .. "--playerctl prev"),       { locked = true })

-- Extra settings for the live USB (opens the installer); absent elsewhere
pcall(require, "live")
