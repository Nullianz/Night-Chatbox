# ◈ ReqriumOS

An Arch Linux based OS with a **liquid glass** desktop: frosted translucent
windows, a floating glass panel, and soft, bouncy animations everywhere.

![wallpaper](dotfiles/reqrium/wallpapers/liquid-night.png)

## What makes it "liquid glass"

| Piece | What it does |
|---|---|
| **Hyprland** (compositor) | Native acrylic glass (refraction, curved lens edges, a hint of chromatic aberration) over a 4-pass blur with vibrancy and grain behind every translucent surface, rounded squircle corners, soft shadows, and a gradient border that slowly rotates around the focused window |
| **Animations** | Spring physics: windows pop in with a jelly overshoot, wobble while you drag them (with motion blur), and drip away on close. Workspaces glide vertically. Panels and menus float in. |
| **Waybar** | A floating glass panel made of glass "pills". The active workspace stretches like liquid. |
| **Rofi** | Frosted app launcher, clipboard picker and power menu |
| **Hyprlock** | Lock screen with a big clock and a glass password field over your blurred desktop |
| **Mako** | Glass notifications |
| **swww** | Wallpapers change with a wave ripple that starts at your cursor |
| **Firefox** | Installed by default as the web browser |
| **Kitty** | Translucent terminal with the Reqrium Night colour palette and a cursor trail |

## What's included

**Every install gets:** Firefox, Thunar file manager (with archive, USB-drive
and thumbnail support), mpv, Loupe image viewer, Text Editor, btop, fastfetch
(with a ReqriumOS logo), Bluetooth and Wi-Fi tray apps, laptop power modes in
the panel, NTFS/exFAT drive support, Flatpak with Flathub, the **paru** AUR
helper, a firewall (ufw, on), compressed RAM swap (zram) and **automatic
snapshots** (snapper, with snapshots before every update, bootable from GRUB).

**Picked automatically for your hardware:** NVIDIA, AMD or Intel graphics
drivers, CPU microcode, and VirtualBox/QEMU/VMware guest tools.

**Optional packs (you choose during install):**

| Pack | Contents |
|---|---|
| `gaming` | Steam, GameMode, MangoHud, 32-bit drivers |
| `office` | LibreOffice |
| `printing` | CUPS printer support |
| `dev` | VS Code, Python, Node.js |

## Install

### Option A: install onto a drive (the ReqriumOS ISO)
1. On an Arch machine, build the ISO:
   ```bash
   sudo pacman -S archiso
   sudo ./iso/build.sh          # writes out/reqriumos-*.iso
   ```
2. Write it to a USB stick (with balenaEtcher, Ventoy, or
   `sudo dd if=out/reqriumos-*.iso of=/dev/sdX bs=4M status=progress`).
3. Boot from the USB. The glass desktop starts with the **installer** already
   open (`SUPER + I` reopens it). Without 3D graphics you get a console
   instead; type `reqrium-installer` there.
4. The installer walks you through 6 steps: internet, drive, your account,
   region and keyboard, optional packs, then a final summary before it erases
   anything.

What the installer sets up:
- GPT partitions: a 1 GB EFI partition (or a BIOS boot partition on older PCs)
  and a btrfs partition using the rest of the drive
- btrfs subvolumes `@`, `@home`, `@snapshots`, `@log` and `@cache`, compressed with zstd
- GRUB bootloader (branded ReqriumOS) with a menu for booting old snapshots
- Your user account with sudo. The root login is locked.

> ⚠️ The installer erases the **whole** drive you pick. Dual-booting
> alongside Windows on the same drive is not supported yet.

### Option B: on an existing Arch install
```bash
git clone https://github.com/nullianz/night-chatbox
cd night-chatbox/ReqriumOS
./install.sh
```
It asks about the optional packs and backs up your old configs to
`~/.config-backup-reqrium-*`.

### How it fits together
`system/reqrium-configure` does the actual setup (packages, drivers,
services, snapshots, desktop config). Both installers call it, so they
always produce the same system.

## Shortcuts

| Keys | Action |
|---|---|
| `SUPER + Space` | App launcher |
| `SUPER + Return` | Terminal |
| `SUPER + E` | Files |
| `SUPER + I` | Open the installer (live USB only) |
| `SUPER + B` | Firefox |
| `SUPER + Q` | Close window |
| `SUPER + F` / `V` | Fullscreen / float |
| `SUPER + 1…9` | Switch workspace (`+ SHIFT` moves the window) |
| `SUPER + S` | Glass drawer (scratchpad) |
| `SUPER + W` | Next wallpaper (ripple transition) |
| `SUPER + C` | Clipboard history |
| `SUPER + L` | Lock |
| `SUPER + SHIFT + S` | Screenshot a region |
| `SUPER + SHIFT + E` | Power menu |

## Customising

- **Glass style:** in `dotfiles/hypr/hyprland.lua`, set `blur.variant` to another effect
  (`BLUR.acrylic` is the default; try `BLUR.prism`, `BLUR.frost`, `BLUR.water` or `BLUR.aurora`) and tune
  the `acrylic` block (refraction, bulb width, clarity, aberration, tint).
- **More or less see-through:** `decoration.active_opacity` / `inactive_opacity`.
- **Animation speed:** the `speed` of each `hl.animation` line (lower is faster).
- **Wallpapers:** put images in `~/.config/reqrium/wallpapers/`.

## Notes

- Blur needs a GPU with working 3D drivers. In VirtualBox or VMware, enable 3D acceleration. In QEMU, use `-device virtio-vga-gl -display gtk,gl=on`.
- The desktop config is written for **Hyprland 0.57+**, which uses a Lua config file
  (`hyprland.lua`). It was boot-tested with Hyprland 0.57.2 and loads with no config errors.
- In virtual machines without 3D acceleration, ReqriumOS switches to a lighter "lite" glass
  (no refraction, wobble or motion blur) so the desktop stays usable.
