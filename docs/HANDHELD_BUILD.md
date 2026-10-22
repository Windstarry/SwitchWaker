# The aarch64 handheld build (PortMaster / ROCKNIX / EmuELEC)

An ARM64 Linux binary of the same native port, installed as a PortMaster port. It is built from the
same sources as the desktop and Switch builds and reads the player's own disc at start-up, like
every other build (docs/RUNTIME_ASSETS.md); nothing from the disc is in the binary.

Tested with the Japanese build (COS_VERSION=1, GZLJ01) on a ROCKNIX 20261016 RK3566 handheld
(glibc 2.40, GLIBCXX_3.4.33, sway frontend).

## Device layout

```
roms/ports/SwitchWaker.sh          the launcher (native/tools/handheld/SwitchWaker.sh)
roms/ports/SwitchWaker/
    switchwaker                    the aarch64 binary (chmod +x)
    switchwaker.gptk               all-blanked key map: gptokeyb must not inject keys
    GZLJ01.iso                     the player's disc (revision 0, uncompressed; .gcm also works)
    user/                          settings.ini, hd_textures/, JAP/Card A/ (the emulated memory card)
    log.txt, log.prev.txt          the launcher's log (the previous run is kept)
```

The launcher is the PortMaster-standard script (controlfolder, `get_controls`, `gptokeyb`,
`pm_finish`). Its environment: `SDL_VIDEODRIVER=wayland` on ROCKNIX (kmsdrm + the Mali blob
directory on LD_LIBRARY_PATH on KMS/DRM firmwares), `COS_DISC` naming the disc, `COS_BACKEND`
(`opengles` by default; Aurora falls back to Vulkan and then to its null backend), and
`COS_ASPECT=16:9` with `COS_FULLSCREEN=1` for the 1280x720 panel (see Display below).

Do **not** put a Debian `libstdc++.so.6` (a port's `libs.aarch64/`) on LD_LIBRARY_PATH: the binary
needs no library newer than the firmware, and a Debian `libstdc++` only shadows the firmware's own.

## Building it

The build image is the PortMaster aarch64 template (`ghcr.io/monkeyx-net/portmaster-build-templates/portmaster-builder:aarch64-latest`,
Ubuntu 20.04 "focal") plus GCC 12 from the ubuntu-toolchain-r PPA (focal's `libstdc++-10` cannot
build RecompCore: `std::bit_cast`, `std::atomic::wait`) and the development headers of
`scripts/docker/Containerfile.linux` minus the packages focal does not have
(`libpipewire-0.3-dev`, `libdecor-0-dev`; SDL drops those backends). Committed once as a local
image, every later build runs without apt:

```
docker run --name swbuild <image> bash /setup-portmaster.sh    # g++-12, headers; commit as an image
```

Aurora's FetchContent dependencies and the **aarch64** Dawn/nod prebuilt packages are prefetched on
the host under `build/pkgs/` and wired in with `FETCHCONTENT_SOURCE_DIR_*` and
`AURORA_{DAWN,NOD}_PACKAGE_URL=file://…`, so the container needs no network. Configure and compile
like `scripts/docker/build.sh linux` does, with the aarch64 package files and
`-DCOS_VERSION=1` for the Japanese build:

```
cmake -S native -B build/native-linux-jpn-arm64 -G Ninja \
    -DCMAKE_C_COMPILER=gcc-12 -DCMAKE_CXX_COMPILER=g++-12 \
    -DFETCHCONTENT_SOURCE_DIR_AURORA=/work/build/aurora-<pin> -DFETCHCONTENT_SOURCE_DIR_ABSEIL-CPP=… \
    … (zstd, SDL, freetype, imgui, sqlite3, RmlUi, tracy, png, zlib, xxhash, fmt) \
    -DAURORA_DAWN_PACKAGE_URL=file:///work/build/pkgs/dawn-<version>-linux-aarch64.tar.gz \
    -DAURORA_NOD_PACKAGE_URL=file:///work/build/pkgs/libnod-<version>-linux-aarch64.tar.gz \
    -DCOS_ASSETS_DIR=/work/build/assets/GZLJ01 -DCOS_VERSION=1 \
    -DCOS_RECOMPCORE_DIR=/work/ref/recompcore
ninja -C build/native-linux-jpn-arm64 switchwaker     # the final link fails: see below
```

**The final link is relinked with GCC 14.** The Dawn prebuilt objects were compiled on a newer
distro and reference `std::__cxx11::basic_string<…>::_M_replace_cold` and the C23
`__isoc23_strtol` family, which GCC 12's libstdc++/glibc 2.31 do not provide, so ninja's link with
`g++-12` fails with undefined references. Take the link command ninja printed,
`ninja -C <dir> -t commands switchwaker | tail -1`, replace `g++-12` with `g++-14`, add
`native/tools/handheld/dawn_compat.cpp` (compiled with `g++-12 -O2`) to the link, and run it. The
result is a binary whose `NEEDED` list is glibc ≤ 2.30 / GLIBCXX ≤ 3.4.32, so it runs on any
firmware at or above focal: `file` (ELF 64-bit ARM aarch64), `readelf -d` (NEEDED) and `ldd`
(everything resolved) are the three checks before copying it to the device.

## Graphics on the device

Aurora tries Vulkan, then OpenGL ES, then its null backend (game logic, no picture). On the
ROCKNIX RK3566 board used for verification neither GPU path is usable, and the launcher therefore
pins the Panfrost ICD:

- the Mali blob's Vulkan ICD (`/usr/share/vulkan/icd.d/mali.json`) is listed first by the loader
  and the blob has no `/dev/mali0` on a mainline kernel: `vkCreateInstance` walks a NULL and kills
  the process (`SIGSEGV`, `libMaliVulkan.so.1+0x3114c4`). `VK_ICD_FILENAMES=<panfrost_icd>.json`
  avoids that;
- Mesa's panvk enumerates on the G52 only with `PAN_I_WANT_A_BROKEN_VULKAN_DRIVER=1` and then
  answers "Vulkan fragmentStoresAndAtomics feature required" to Dawn;
- Mesa's Panfrost GL ES reports version 3.1, no storage buffers in the vertex stage
  (`maxStorageBuffersInVertexStage: 0`), so Dawn's GLES backend fails the device request too.

So the game runs on such a board with the null backend: no picture, but the game's own logic,
messages, memory card and saves work, which is enough to verify the port there. Boards whose
firmware provides the blob (kernel with `mali_kbase`, `/dev/mali0`) or a GL ES 3.2 / Vulkan 1.1
driver with the two features above render normally.

## Display: the 1280x720 panel

The verification board's panel is 1280x720 (DSI-1 reports 720x1280 with a 270 transform, so sway's
logical output is 1280x720 at scale 1). The port's default window is 960x720 - the 4:3 picture at 720
points - and the frontend presents a window fullscreen by stretching it over the whole panel, so the
first picture on the device was that 4:3 output enlarged and cropped on all four sides.

The launcher sets both options for these boards:

```
export COS_ASPECT="${COS_ASPECT:-16:9}"       # 1280x720 window, the picture drawn at that shape (docs/WIDESCREEN.md)
export COS_FULLSCREEN="${COS_FULLSCREEN:-1}"  # borderless fullscreen; the compositor does not scale it again
```

Verified on the second test board once its kernel ran the Mali blob (Vulkan rendering instead of the
null backend): `log.txt` reads `[aurora] Using framebuffer size 1280x720 scale 1` and
`[cos] aurora: backend=vulkan window=1280x720 framebuffer=1280x720`, and a `grim -o DSI-1` capture
of the title screen shows the whole picture - the logo with its margins, the sea to both sides and
the complete "请按下START键" line. `COS_ASPECT=4:3` with `COS_FULLSCREEN=0` gives the original
GameCube picture, pillarboxed in a 960x720 window.

## Verified on the device

The JPN build and its launcher pass, on the RK3566 board above: the boot milestones, the real
new-game flow from a clean card (prologue, intro event, `outset-real` at frame 12420 in a 14500
frame run), a save through `d_menu_save`'s own steps (`COS_SMOKE=save-sweep`: PASS at frame 3182)
and the save/reload round trip; the itemised results are in docs/JPN_DISC_PLAN.md, "Checking the
five open items". Both the first board (2 GB, blob driver, Vulkan) and this one run the same
binary.

The harness runs on the device as on a desktop: `COS_INPUT` (a frame-based pad script),
`COS_FRAMES`, `COS_MILESTONE`, `COS_SMOKE` and `COS_RUN_DIR` are read from the environment. When
the binary is started over SSH instead of from the frontend, give it the launcher's environment
(`XDG_RUNTIME_DIR=/var/run/0-runtime-dir`, `WAYLAND_DISPLAY=wayland-1`, `SDL_VIDEODRIVER=wayland`,
the Panfrost pin above); started from EmulationStation, the launcher does that.

The game's `OSReport` goes to stdout, which the log redirect block-buffers: lines written just
before a crash can miss `log.txt`, which is why the launcher keeps the previous run as
`log.prev.txt`.
