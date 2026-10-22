#!/bin/bash
# PortMaster launcher of the SwitchWaker aarch64 build (docs/HANDHELD_BUILD.md). Installed on the
# device as roms/ports/SwitchWaker.sh, next to roms/ports/SwitchWaker/ (switchwaker, the user's
# disc image, user/). The binary links SDL3 statically and needs no library newer than the
# firmware (glibc 2.40 / GLIBCXX_3.4.33 on ROCKNIX 20261016): libs.aarch64 (a Debian libstdc++)
# must NOT be put on LD_LIBRARY_PATH, it would only shadow the firmware's own.

XDG_DATA_HOME=${XDG_DATA_HOME:-$HOME/.local/share}

if [ -d "/opt/system/Tools/PortMaster/" ]; then
  controlfolder="/opt/system/Tools/PortMaster"
elif [ -d "/opt/tools/PortMaster/" ]; then
  controlfolder="/opt/tools/PortMaster"
elif [ -d "$XDG_DATA_HOME/PortMaster/" ]; then
  controlfolder="$XDG_DATA_HOME/PortMaster"
else
  controlfolder="/roms/ports/PortMaster"
fi

source "$controlfolder/control.txt"
get_controls
[ -f "${controlfolder}/mod_${CFW_NAME}.txt" ] && source "${controlfolder}/mod_${CFW_NAME}.txt"

GAMEDIR="/$directory/ports/SwitchWaker"

# logging (the game's stdout is block-buffered, so keep the previous run's log for a crash report)
cd "$GAMEDIR" || exit 1
[ -f log.txt ] && mv -f log.txt log.prev.txt
> "$GAMEDIR/log.txt" && exec > >(tee "$GAMEDIR/log.txt") 2>&1

# ROCKNIX runs the frontend on sway: fall back to the firmware's wayland values when launched over
# SSH (from EmulationStation they are already in the environment).
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/var/run/0-runtime-dir}"
export WAYLAND_DISPLAY="${WAYLAND_DISPLAY:-wayland-1}"
export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"

# SDL driver selection. This build links SDL3 statically (no SDL3SHIM_*), so pick the video driver
# per firmware:
#   ROCKNIX (sway frontend) -> wayland
#   KMS/DRM firmware         -> kmsdrm, with the Mali blob on LD_LIBRARY_PATH for EGL (libglvnd
#                               ships no EGL vendor there, so SDL's dlopen("libEGL.so.1") must
#                               reach the blob)
if [ "$CFW_NAME" = "ROCKNIX" ]; then
  export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-wayland}"
elif [ -d /usr/lib/mali ]; then
  export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/lib/mali"
  export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-kmsdrm}"
else
  export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-wayland}"
fi

# Mainline-Panfrost boards (RK3566 and friends) have no /dev/mali0: the Vulkan loader's first ICD
# (mali.json) is the blob's, whose vkCreateInstance walks a NULL and kills the process before the
# fallback. Pin Mesa's panfrost ICD so the loader enumerates instead. Aurora then tries Vulkan,
# OpenGL ES and finally its null backend; panvk on the G52 lacks fragmentStoresAndAtomics, so
# those boards currently run the game with the null backend (no picture) until Dawn/pvk support it.
if [ ! -e /dev/mali0 ]; then
  for icd in /usr/share/vulkan/icd.d/panfrost_icd*.json; do
    [ -f "$icd" ] || continue
    export VK_ICD_FILENAMES="$icd"
    export PAN_I_WANT_A_BROKEN_VULKAN_DRIVER=1
    break
  done
fi

# The disc this build was compiled for (COS_VERSION in native/cmake/GameConfig.cmake: GZLJ01 for
# the Japanese build, GZLE01 for USA, GZLP01 for Europe). Revision 0, uncompressed .iso/.gcm.
export COS_DISC="${COS_DISC:-$GAMEDIR/GZLJ01.iso}"
export COS_BACKEND="${COS_BACKEND:-opengles}"

# The window follows the device, nothing here fixes a resolution: COS_FULLSCREEN=1 asks SDL for a
# fullscreen window, so its size is the panel's own mode (1280x720 on this board, 1920x1080 on a
# 1080p one) instead of the desktop window's 960x720, and the compositor never scales it.
export COS_FULLSCREEN="${COS_FULLSCREEN:-1}"

# The picture's shape must be known before the window exists (the game reads COS_ASPECT in static
# initialisers), so take it from the panel's mode: a 16:9 panel gets the widescreen option (content
# framebuffer = the panel's own resolution), a 4:3 one the original GameCube picture, which Aurora
# pillarboxes in the fullscreen window.
if [ -z "$COS_ASPECT" ]; then
  for f in /sys/class/drm/card*-DSI-*/modes /sys/class/drm/card*-eDP-*/modes \
           /sys/class/drm/card*-LVDS-*/modes /sys/class/drm/card*-*/modes; do
    [ -s "$f" ] || continue
    mode=$(head -n 1 "$f")
    w=${mode%x*}
    h=${mode#*x}
    [ "$w" != "$mode" ] || continue
    case "$w" in '' | *[!0-9]*) continue ;; esac
    case "$h" in '' | *[!0-9]*) continue ;; esac
    [ "$h" -gt 0 ] || continue
    # A panel's mode can arrive rotated (this DSI one is 720x1280), so compare its long side.
    if [ "$w" -lt "$h" ]; then
      t=$w
      w=$h
      h=$t
    fi
    if [ $((w * 100)) -ge $((h * 170)) ]; then
      COS_ASPECT=16:9
    elif [ $((w * 100)) -ge $((h * 152)) ]; then
      COS_ASPECT=16:10
    else
      COS_ASPECT=4:3
    fi
    break
  done
  export COS_ASPECT="${COS_ASPECT:-16:9}"
fi

# zip extraction loses the exec bit on the game binary.
chmod +x "$GAMEDIR/switchwaker" 2>/dev/null

# Positional arg = the process name gptokeyb killall's on Select+Start; -c loads the all-blanked
# .gptk so no synthetic keys fight the game's native controller reading.
$GPTOKEYB "switchwaker" -c "$GAMEDIR/switchwaker.gptk" &

./switchwaker "$COS_DISC"

pm_finish
