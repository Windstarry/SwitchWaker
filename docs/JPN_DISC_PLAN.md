# The Japanese disc (GZLJ01): plan

Status: **implemented and checked on the Linux build in the Docker container**, 2026-10-15. The Switch
NRO wiring is in place (`--jpn`, `switchwaker-jpn.nro`) but was not built or checked here (the
devkitPro image it needs is not available on this machine). The USA disc (GZLE01 revision 0) stays the
reference build: every change below keeps the USA build's output and checks unchanged.

## What is already there

- The decompilation covers the Japanese version: `VERSION_JPN` (1) and `VERSION_SELECT(...)` branches in
  `game/`, and the decomp's `config/GZLJ01` (`config.yml`, `symbols.txt`, `splits.txt`, the SHA-1 of
  main.dol and of each REL).
- `COS_VERSION` and `COS_GAME_ID` in `native/cmake/GameConfig.cmake` (`1` is Japan).
- Runtime assets (docs/RUNTIME_ASSETS.md): the binary holds no game data, the arrays come from the
  player's disc at start-up, so a Japanese binary can be built from stubs as the USA one is.

## Decisions

| Topic | Decision |
|---|---|
| Builds | Japan is a third build: `-DCOS_VERSION=1` (the `COS_GAME_ID` follows: `GZLJ01`). Linux: its own build directory (`build/native-linux-jpn`). Switch: `switchwaker-jpn.nro` (NACP name "SwitchWaker JPN"), installed as `sdmc:/switch/switchwaker/switchwaker.nro` like the USA and European builds (one region per SD card, so the HOME-menu forwarder starts any of them). |
| Disc file | `sdmc:/switch/switchwaker/GZLJ01.iso` for the Japanese NRO. Revision 0, uncompressed, as for USA. |
| Disc ID in code | The build defines `COS_GAME_ID_STR` ("GZLE01" / "GZLP01" / "GZLJ01") for the harness (`native/include/pc/pc_game_id.h` names the disc, its region and image); the disc checks, messages, the default path on the Switch and the smoke tests use it instead of the literal. |
| Language | Japan has a single language, as the USA disc: no `dComIfGs_getPalLanguage`, no `COS_GAME_LANGUAGE` (that is PAL only). |
| 50/60 Hz | Not applicable: the PAL 50 Hz mode and its prompt are European only. |
| Message archives | Japan's message system is structurally different: one archive (`bmgres.arc`) holds the group files (`zel_%02d.bmg`), and `fopMsgM_msgGet_c::getMesgHeader` routes by the number's group (`i_msgNo >> 16`). There is no `bmgresh.arc`, no second message archive and no `fopMsgM_hyrule_language_check` (f_op_msg_mng.cpp, its `VERSION <= VERSION_JPN` branch). |
| Arrays | 163, the same as the USA disc (the European build's extra four are its language message tables). |
| USA build | Unchanged: every new branch reads the disc-ID macro or is guarded by `#if VERSION > VERSION_JPN` / `== VERSION_JPN`; the USA build's checks (regress.sh) must still pass. |

## Steps

1. **This plan.**
2. **Build configuration.** `GameConfig.cmake`'s `COS_GAME_ID` list and `pc_check.cpp`'s static assert
   accept Japan; `gen_assets.sh --version GZLJ01`, `gen_asset_stubs.py --version`,
   `check_runtime_assets.py --version`.
3. **Disc check.** `pc_game_id.h` names the Japanese disc, region and image; `pc_disc.cpp` gained the
   third `PlayerBuild` and the three-region messages.
4. **Runtime assets for GZLJ01.** `disc_manifest.py`: the GZLJ01 revision 0 hashes (image and main.dol);
   `gen_assets.sh --version GZLJ01` splits the disc and checks main.dol through dtk.
5. **Regression tools.** `run.sh`, `regress.sh`, `disc_manifest.py` and the sweeps pick the disc ID
   from the build (`COS_BUILD_DIR`'s `COS_GAME_ID`), so Japan follows without a literal;
   `actor_sweep.py` maps `GZLJ01` to the `VERSION_JPN` branches of the profile list.
6. **Switch build.** `build_native.sh --jpn` (its own build directory and `switchwaker-jpn.nro`),
   `make_sd.sh`, `push.sh native-jpn`, `guard_nro.py --version GZLJ01`, `package_switch.sh --with-jpn`,
   `switchwaker_debug.py deploy --jpn`.
7. **CI.** `.github/workflows/ci.yml` builds the Japanese version's stub build like the USA and
   European ones.
8. **Docs.** This file; README.md, INSTALL.md, docs/SWITCH_BUILD.md, native/README.md.

## Implementation (2026-10-15)

| Part | As built |
|---|---|
| Build | `GameConfig.cmake` (COS_VERSION 1 = GZLJ01, comment), `pc_check.cpp` accepts `VERSION_JPN`, `gen_assets.sh` / `gen_asset_stubs.py` / `check_runtime_assets.py --version GZLJ01`, `scripts/docker/build.sh` (`GZLJ01` → `-DCOS_VERSION=1`, `build/native-linux-jpn`). |
| Disc | `native/include/pc/pc_game_id.h`: `PC_GAME_ID_JPN`, the region macros and `PC_GAME_ISO_NAME`; `native/src/pc/runtime/pc_disc.cpp`: the third `PlayerBuild` and the three-region messages. `disc_manifest.py`: the GZLJ01 image SHA-1 (`1c4b574d…`, Redump disc 4998) and main.dol SHA-1 (`995406d9…`, from `config/GZLJ01/build.sha1`). |
| Host fixes, Japan only | The port's harness `native/src/pc/harness/boot_tests/pc_msg.cpp` used USA/PAL-only APIs: `fopMsgM_hyrule_language_check` (declared `#if VERSION >= VERSION_USA`), `dComIfGp_setMsgDt2Archive` and `dComIfGs_getPalLanguage` (`#if VERSION > VERSION_JPN`). They are now guarded by the same conditions: for Japan the message sweep covers the single `bmgres.arc`/`zel_00.bmg` (its two-file/kingdom model is USA/PAL only) and the fresh-save check keeps only the clear count. `pc_language.cpp` was already `#if VERSION == VERSION_PAL` end to end, `pc_save.cpp` guards its `getPalLanguage` uses. `pc_save.cpp`'s save-folder region (its `01-….gci` path) mapped Japan to `USA`, while Aurora's `get_card_region` puts a `GZL` disc in `JAP`: it now follows the disc name (`E`→USA, `P`→EUR, `J`→JAP). |
| Tools | `actor_sweep.py`'s `build_version()` returns "JPN" for `COS_GAME_ID GZLJ01`; `run.sh`/`regress.sh` and the sweeps' `--disc` help name Japan. |
| Switch | `build_native.sh --jpn`, `make_sd.sh`, `push.sh native-jpn`, `guard_nro.py --version GZLJ01` (163 arrays, as USA), `package_switch.sh --with-jpn`, `switchwaker_debug.py deploy --jpn`, `switch/native/CMakeLists.txt` ("SwitchWaker JPN"), `scripts/docker/build.sh switch --disc …GZLJ01.iso`. `cos_switch.h`'s default disc path follows `PC_GAME_ISO_NAME`, so it is automatic. |
| Container network | Not a code change: on this machine Docker Desktop's resolver poisoned `github.com` (`127.0.0.1`) and its release-asset CDN reset transfers, so the container is given `--dns` (`COS_CONTAINER_DNS`) and the Dawn/nod packages and zstd are prefetched on the host (Aurora via `FETCHCONTENT_SOURCE_DIR_AURORA`, `AURORA_{DAWN,NOD}_PACKAGE_URL=file://…`, `FETCHCONTENT_SOURCE_DIR_ZSTD`). |

Checked in the container (Linux build, `-DCOS_VERSION=1`, the Redump-verified GZLJ01 image):

- `gen_assets.sh --version GZLJ01 --disc GZLJ01.iso`: 774 headers, dtk checks main.dol (exit 0).
- `scripts/docker/build.sh linux --disc GZLJ01.iso`: `build/native-linux-jpn/switchwaker` (ELF 64-bit
  x86-64, 140,945,072 bytes), `cos_sdk_smoke`, `cos_pc_tests` link.
- `scripts/docker/build.sh linux --test`: `cos_sdk_smoke`, `cos_pc_tests`, `run static-init`,
  `run disc-ls` and `run title` all pass (software Vulkan under Xvfb) with the Japanese disc.

## Verification (pending)

- Switch: `switchwaker-jpn.nro` builds (needs the devkitPro image); `guard_nro.py` finds none of the
  disc's arrays in it.
- Console tests of `switchwaker-jpn.nro`: boot with `GZLJ01.iso` on the card.
- Full regression (`regress.sh`) for the Japanese build.
- The message sweep's Japanese routing: `pc_msg.cpp` checks each file's own entries and skips the
  cross-file classification; routing by the number's group (`i_msgNo >> 16`) is left as a TODO where
  the Japanese archive layout is known.

## Risks

- Japan-only code paths the port never compiled may reference things the port replaced or stubbed (the
  message system's group routing, the single message archive).
- Data offsets: fixes keyed to GZLE01 addresses would silently miss on Japan; the PAL review (step 7 of
  PAL_DISC_PLAN.md) looks for them, and no Japan-specific offset work has been done yet.

## Review of the host and game code against Japan (2026-10-17)

The analogue of the PAL review (step 7 of PAL_DISC_PLAN.md). Two passes:

1. **Hardcoded GameCube addresses in the port layer.** Every `0x8…` literal in `native/` is either a
   hardware/algorithm constant (the DSP boot mails `0x8071FEED`/`0x80F3….`, the EXI channel bits, the
   J3D MEM1 convention `0x80000000 | (ptr & 0x3FFFFFFF)`, SHA-1's `0x8F1BBCDC`, the test pattern) or a
   comment naming where a value comes from (`pc_aspect.h`, `pc_meter_hooks.cpp`: the USA 16:9 Gecko
   code's `.sdata2` values). **No address is used as one**, so nothing in the port layer is keyed to a
   GZLE01 address. The port-layer version guards are right for Japan: `pc_language.cpp` is PAL-only
   (`#if VERSION == VERSION_PAL`), `pc_logo_hooks.cpp`'s `l_msgDtCommand2` is `#if VERSION > VERSION_JPN`,
   and the harness (`pc_msg.cpp`, `pc_save.cpp`) is done.

2. **Units that lost their Japan branches to the zeldaret convergence** (the "lost PAL code" of
   PAL_DISC_PLAN.md, for Japan). Counting `VERSION_JPN` per file in `game/` against the fork baseline
   (snrubrm/tww b09eebc, the `build/decomp-docker` checkout), over the 2026 files present in both (168
   carry Japan references), found four files with fewer Japan references in the port:

   | Unit | Lost | Genuine? |
   |---|---|---|
   | `d/d_menu_window.cpp` | `event_wait_frame` (its declaration and logic, `#if VERSION > VERSION_JPN`) | **Yes.** Japan has no event-wait before the item menu opens; the converged file compiles it unguarded, so a Japanese build waits like the USA one. |
   | `d/actor/d_a_bst.cpp` | the frame count branch (`#if VERSION <= VERSION_JPN \|\| VERSION == VERSION_PAL` → 410; the USA path uses 500) | **Yes.** The converged file has no version handling at all (`VERSION_SELECT`/`VERSION_JPN`: 0), so a Japanese build uses the USA timing for the boss's message. |
   | `d/actor/d_a_saku.cpp` | `#if VERSION > VERSION_JPN` around the hit loop | No: the two sides only differ by `dCcD_Cyl* const cyl = &mCyl[id][i]` against a direct member access — the same object (a matching detail). |
   | `JSystem/J3DGraphAnimator/J3DModel.cpp` | `#if VERSION == VERSION_JPN`'s `mpScaleFlagArr` check and the `VERSION <= VERSION_JPN` `#pragma optimization_level` blocks | Runtime: no (the OOM checks still exist, ordered differently); the pragmas only matter for matching the original. |

   The first two are the real gaps: the fix is the one PAL already uses for its converged units — `d_menu_window.cpp`
   and `d_a_bst.cpp` include snrubrm's unit from `game/src/pal/` under `#if VERSION == VERSION_PAL`, and that
   `.inc` already carries the Japan branches (`VERSION_JPN`: 3 and 1). Extending the guard to
   `VERSION == VERSION_PAL || VERSION == VERSION_JPN` restores them for Japan; no new `.inc` is needed, and
   the USA build is untouched (it keeps the `#else` side).

   Limits of the count: it only sees lost branches. Japan code re-expressed as `VERSION_SELECT(jpn, …)`
   without the `VERSION_JPN` token would not show (checked: none of the four had a `VERSION_SELECT`), and
   behaviour moved between files is out of its reach.

