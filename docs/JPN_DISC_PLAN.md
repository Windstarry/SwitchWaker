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
  cross-file classification; routing by the number's group (`iMsgNo >> 16`) is left as a TODO where
  the Japanese archive layout is known.

## Behavioural check of the offset fixes (2026-10-17)

Run in the container with the Japanese disc and the `build/native-linux-jpn` build:

- The boss stage `M_DaiB:0:0` (Gohdan, `daBst_c`) boots and runs 900 frames uncapped with no fault:
  the restored `d_a_bst` (its 410-frame branch) is exercised and behaves.
- The item-menu target (`run.sh options-menu --stage sea:44:206 --input
  native/check/input/menu-travel-save.txt`) **fails (exit 13) on the Japanese build both with the
  restored `d_menu_window` and with it reverted to the PAL-only guard** (the `57a7fd9` version,
  rebuilt and rerun): the failure is not caused by the restore. The crash point moves between runs —
  `JUTResFont::isLeadByte` with a NULL `mIsLeadByte` (SIGSEGV at `switchwaker+0x16acc9`) in one, a
  SIGABRT in another — so the Japanese menu/text path is unstable on its own and is the next target
  (the fonts and their encoding; a `[aurora::card] Failed to open file` line appears in the same run,
  which may be the save card).

## The JUTCacheFont fixes and their verification (2026-10-17)

The exit-13 above is fully explained: the Japanese build is the only one that reaches `JUTCacheFont`
(`mDoExt_initFont0`, `VERSION <= VERSION_JPN`, the `rock_24_20_4i1.bfn` from `fontres.arc` with
`param_7=0xB4, param_8=0x800`), and three of its host assumptions break in sequence. All three are
GameCube 32-bit/4-byte assumptions the USA/PAL path never exercises:

1. **`setBlock` walked the font resource through a raw `int*`** (`switch (*pData)`, `pData[1]`): the
   block tags and sizes are big-endian disc data, so the little-endian host matched no tag and
   advanced by the byteswapped size (`0x20` read as `0x20000000`): the walk ran 512 MiB out of
   bounds and left `mIsLeadByte` unset (JUTResFont only sets it in its own `setBlock`), so the next
   text draw called `isLeadByte` through a null function pointer (the SIGSEGV at `+0x16acc9`; gdb:
   `setBlock <- internal_initiate <- JUTCacheFont ctor <- mDoExt_initFontCommon <- mDoExt_initFont0
   <- mDoExt_getMesgFont <- dScnLogo_Delete`, `r14 = data + 0x20000000`). Fixed by reading through
   `JUTDataBlockHeader` as `getMemorySize` and `JUTResFont::setBlock` already do.
2. **`allocArray` allocated `u32[]` for the block-pointer array**: the decomp's `u32` is the
   GameCube's 4-byte pointer; on the host the `setBlock` writes of `mpWidthBlocks`/`mpGlyphBlocks`/
   `mpMapBlocks` overflowed the array by half and corrupted the heap (the next crash moved into
   `JKRExpHeap::allocFromHead`, `+0x157a1a`, LOGO_SCENE). Fixed as `void*[]`, which
   `JUTResFont::protected_initiate` already uses for the same array.
3. **The cache-page layout is host-different.** `TGlyphCacheInfo` holds `mPrev`/`mNext` pointers:
   0x20 bytes on the GameCube (where the header byte-for-byte overlays the 0x20-byte `ResFONT::GLY1`
   disc record, `mPrev`/`mNext` being the record's `mType`/`mSize` words), 0x28 on the host, so
   `sizeof(TCachePage)` is 0x48 against the decomp's hardcoded 0x40. Consequences, each hit in turn:
   - `getGlyphFromAram` memcpy'd the 0x20-byte GLY1 record with `sizeof(TGlyphCacheInfo)`: the copy
     overran the record and the `field_0x16`/`field_0x18` (numRows/numColumns) reads went past it,
     so the per-sheet divide `*param_2 / (numRows * numColumns)` raised SIGFPE at frame 677. Fixed
     by copying the shared fields explicitly (the GC's overlay is a big-endian accident, not a
     contract); the sheet's ARAM address is read back from `((ResFONT::GLY1*)param_0)->mType`, which
     `setBlock` stored there (on the GameCube that read went through the header's `mPrev` word).
   - The page stride (`field_0x94 = mMaxSheetSize + 0x40`) and the cache size
     (`calcCacheSize`, and the same inline computation in `mDoExt_initFontCommon`) sized the cache
     for 0x40-byte pages: pages, their texture areas and the total no longer lined up. New
     `JUTCacheFont::getTexAreaOffset()` = `ALIGN_NEXT(sizeof(TCachePage), 0x20)` — 0x40 on the
     GameCube (unchanged), 0x60 on the host — used by the stride, both cache-size computations and
     the texture area (the `pCachePage + 1` of the decomp).
   - That alignment is not cosmetic: `JKRAramPiece::orderAsync` asserts 32-byte alignment on both
     DMA ends, and the host's 0x48 header made the first glyph's texture transfer panic
     (`OSPanic`, JKRAramPiece.cpp:102) right when the first message was to draw (frame 685,
     `ARRIVAL_DRG2`'s message). The ARAM side is fine (the same addresses already passed the assert
     as DMA destinations in `setBlock`).
   - `invalidiateAllCache` threaded the free-page list through raw `int*` stores (4 bytes, and the
     fields at 0x0/0x4, not the host pointers' 0x0/0x8): rebuilt it with real pointers.

Verification, `options-menu --stage sea:44:206 --input menu-travel-save.txt` on the rebuilt
`build/native-linux-jpn` (runs/options-menu-20261017-153826): the crash and both panics are gone —
the run reaches the frame-3300 check point (129 s, through warps, events and menus), `#expect
alloc-failures 0` and `#expect opened 3` pass. Two checks still fail, and both trace to one new,
separate problem:

- `FAIL stage (not in PLAY) -1, expected sea 11` and `FAIL card-roundtrip` (no save file): after the
  first debug warp (sea 13, Dragon Roost, frame 470) the arrival demo `ARRIVAL_DRG2` (event 152)
  starts at ~563 and never ends — the second warp (sea 11) is requested but never executes, so the
  whole later flow (START, the save screens, the card write) never runs. The event sits in its
  MESSAGE staff's `PUSHBUTTON` cut from 600 frames in (event-watch reports every 300 frames through
  2700+). That cut waits for the message a prior `CREATE_MSG` cut created to reach
  `fopMsgStts_MSG_DISPLAYED_e` (d_event_data.cpp `ACT_PUSHBUTTON`); the cut did advance past
  `CREATE_MSG` (so `fopMsgM_messageSet` succeeded), but `dComIfGp_getMesgStatus()` reads 0 — no
  message is on screen: the Japanese message process is created and then dies or ends without ever
  displaying. The USA reference run of the same script has no such stall (B8, NATIVE_PORT_PLAN.md:
  "Viajar to Dragon Roost and Windfall" works), so this is a Japanese message-path defect, not disc
  flow — and it is only now reachable because the font fixes got the game this far. Next target:
  the message process's own path (`f_op_msg_mng.cpp`'s `VERSION <= VERSION_JPN` branches, `d_msg`)
  — why a created message ends without DISPLAYED; the `[aurora::card] Failed to open file: gczelda`
  line (the save card's file, JAP/Card A) is likely the same area's second head.

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


## The ARRIVAL_DRG2 hang: root cause and the JPN check input (2026-10-18)

The two failing checks of the previous section (`stage sea 11`, `card-roundtrip`) were traced with
source-level message traces (f_op_msg_mng, d_event_data, d_msg, f_pc_deletor) and a dump of
zel_00.bmg's INF1 entries from the disc. Findings:

1. **The BMG tag grammar is not JPN-specific.** The suspected truncated tag at message 0x5e5
   (`1a 08 ff`) was a dump artifact: a tag's payload can contain 0x00, and the dumper treated the
   first one as the string terminator. A constraint solve over all 30023 tags of the JPN
   zel_00.bmg confirms the USA layout (`1A size p1 p2 code args`, skip = size) byte for byte; the
   other disc with the same id ("塞尔达传说风之杖.gcm") differs only in text, not in grammar.

2. **The hang is a test-input problem, not a message-system bug.** The JPN ARRIVAL_DRG2 text is
   longer than the USA one and runs `0a 0a 0a` into the maxLines=3 page stop, so every message of
   the chain (0x5e5, 0x5e6, 0x5e7, 0x5e8, 0x5e9, the item get 0x62c) waits in the STOP state for
   A. The shared check input menu-travel-save.txt presses no A between the warp (frame 470) and
   the menu (700), so the menu's warp tears down the stopped message; the event manager then
   waits on the dead cut for good (PUSHBUTTON holds the deleted l_msg, FINISH never ends), the
   warp target never reaches PLAY and the save flow never happens. The USA text is short enough
   that its message chain is still mid-typing (not yet paged) at the same frames, which is why
   the shared input passes there.

3. **The Dragon Roost leg is not recoverable with input alone.** With A presses every 30 frames
   the whole message chain completes (all six messages DESTROYED, the event advances) - but the
   event chain then continues into the TACT_TEARCH0 wind-waker tutorial (message 0x637, then the
   song input), which the check input cannot realistically page through. The JPN check input
   therefore skips the Dragon Roost leg: native/check/input/menu-travel-save-jpn.txt travels to
   Windfall directly (Viajar list 2x DOWN); Windfall's arrival has no event, the save flow is
   reached at the USA timings and the run passes.

4. **regress.sh gained per-disc target lines.** A target line may end in `when GZLJ01` /
   `unless GZLJ01` (one game id); regress.sh's filter_targets keeps only the lines that match the
   build's COS_GAME_ID (regress_targets.txt lines 130-131). The USA/PAL builds keep
   menu-travel-save.txt and the JPN build runs menu-travel-save-jpn.txt.

Verification, `options-menu --stage sea:44:206 --input menu-travel-save-jpn.txt` on
build/native-linux-jpn (runs/msgverify-20261018-130111): **PASS at frame 3300, 0 errors** -
`opened 3`, `stage sea 11` (PLAY reached), `alloc-failures 0`, `card-roundtrip sea 44` all pass.
The full regression suite has not been rerun with the filter change (regress.sh's other targets
have no `when`/`unless` suffix and are unaffected); the four JPN smoke targets of step
"Verification (pending)" above still stand.

One host-level issue remains from the traces, deliberately not chased further here: the event
manager deadlocks when a scene change kills a message process that a PUSHBUTTON cut is waiting
on (the cut keeps polling the deleted message). Real players can open the host menu during any
dialogue, so USA/PAL have the same latent bug - it is just far less likely there. A fix would
make specialProcMessage's PUSHBUTTON (d_event_data.cpp) detect the deleted process (SearchByID
per frame) and end its cut; left to its own lane.

## The aarch64 handheld build: four runtime fixes (2026-10-21/22)

Context: the JPN build cross-compiled for aarch64 with g++-12 in the focal portmaster image
(Ubuntu's GCC adds `-D_FORTIFY_SOURCE=2` and `-fstack-protector-strong`, which the clang builds
do not), relinked with g++-14 for Aurora's newer-distro Dawn prebuilt plus five glibc 2.32/2.38
shims, and run on ROCKNIX 20261016 (glibc 2.40, GLIBCXX_3.4.33, Mali-G52, sway). Four crashes
in a row, each one only reachable after the previous was fixed:

1. **`char mArcName[3]` (`d_a_npc_zl1.h`).** `decideType` runs `strcpy(mArcName, "Zl2")`, 4 bytes
   with the NUL. `__memcpy_chk` aborted the process (`*** buffer overflow detected ***`,
   SIGABRT) creating the first room after the prologue (frame 8349). "Zl2" is the only actor
   name in the game that is 4 bytes (every other is 2 chars + NUL); the GameCube struct always
   had the byte at 0x6DF before `mShadowId` (0x6E0), so the array is [4] now.
2. **The JPN message heap was not doubled.** Decision H5 doubled the USA/PAL 0xa32d heap for the
   host's larger `dMesg_outFont_c`, but the `VERSION <= VERSION_JPN` branch kept the GameCube's
   0x9f2d. The host log showed it: `heap: cannot alloc 0x40 bytes ... EXPH heap ... size 0x9e20,
   free 0x28 ... frame 8721` - the 18th font's `JUTPalette` allocation returned NULL and
   `JUTTexture::attachPalette(NULL)` read `mPalette->mTlutName` (SIGSEGV addr=0x28). The JPN size
   is `0x9f2d * 2` on TARGET_PC now, like every other message heap.
3. **`ruby_character` returned a dead stack buffer.** The decomp returns `char buffer[3]` by
   address; GCC 12 at -O2 knows that address cannot be used and folds the return to NULL
   (`mov x0, #0`), so `do_systemTagCode` ran `strcat(field_0x97, NULL)` (SIGSEGV addr=0x0, the
   first message with a ruby tag: frames 9983 and 11019). The buffer is static now, which keeps
   the caller's immediate `strcat` working as on the GameCube.
4. **`cardicon.arc` can be unmounted under the save thread** (`mDoMemCdRWm_BuildHeader`).
   `JKRArchive::mount` hands out one object per entry number (`check_mount_already`,
   `mMountCount++`), so the file-select screen and the card thread can share the archive; a save
   in that window found it unmounted and `arc->getResource` asserted `isMounted()`
   (JKRArchivePub.cpp:337 in JPN) - `Halt`, frame 12482. The header now zeroes the icon area and
   skips the copy when the archive is missing or unmounted, with an OSReport that says which.

Reached on the device before each stop: logo-res 8.5 s, opening 9.2 s, title 18.5 s, file-select
51.5 s, new-game ~84 s, then sea room 44 (`outset-real`). State of the four: all four are verified
on the second test device, see "Checking the five open items" below.

## The save path: SaveSync's missing "busy" result (2026-10-22)

On the device the save completed (the card's `.gci` was written) but the save screen kept
answering "cannot save". The card layer was fine: the log had no CARD errors, and the file's
mtime moved with each attempt. The defect is in `mDoMemCd_Ctrl_c::SaveSync()`: its
`else if (field_0x1660 == 1) ret = 0` branch (0 = still busy) was wrapped in
`#if VERSION > VERSION_JPN`, so the Japan build answered 2 (failure) for every state other than
4 (done). `d_menu_save`'s JPN branch acts on `res != 0` and treats 2 as the failure message, so
the first poll of a save - taken before the card thread had finished - always produced "cannot
save", while the write itself went through. The branch is unconditional now.

Fixing SaveSync alone then left the screen at "saving" forever: the Japan build's save screen
polls `mDoMemCd_SaveSync()` every frame and had no latch, while the USA/PAL builds latch the
first result in `field_0x540` (`dataWrite` takes one poll right after `mDoMemCd_Save`,
`memCardDataSaveWait` only polls while it is 0). The USA flow is the one the handheld port is
verified with, so the Japan build now uses it too: `dataWrite` takes that first poll for every
version and `memCardDataSaveWait` is the USA body without the `VERSION <= VERSION_JPN`
variants. Verified on the device: the save completes and the screen returns to the game, and
after restarting the port the save loads with the same progress and items it was written with
(the round trip closes).

Diagnostics note: the game's `OSReport` is the weak `vprintf(stdout)` in
`game/src/dolphin/os/OSError.c`, and stdout is block-buffered when the launcher redirects it to
`log.txt`, so lines written just before a hang never reached the file. The field diagnostics used
to find the save defect (and the `setvbuf` that made them visible) have been removed again; the
only `OSReport`s left in the card code are the three real failure reports in
`mDoMemCdRWm_BuildHeader` (missing archive, unmounted archive, missing banner/icon).

## Checking the five open items (2026-10-22, second test device)

The checks below ran on a ROCKNIX RK3566 handheld (192.168.31.16, 4 GB, mainline Panfrost) with
the aarch64 JPN build of this source (docs/HANDHELD_BUILD.md), deployed as
`roms/ports/SwitchWaker/` + a launcher. Its GPU path is the null backend (no picture, the game's own
logic runs; see that document), which is enough for every check here - all of them are CPU-side.

1. **Fixes 3 (`ruby_character`) and 4 (`cardicon.arc`): verified in one run.**
   `COS_INPUT=new-game.txt`, empty card folder, `COS_FRAMES=14500`: the prologue ended at frame
   7993, the intro dialogue event ran 8034-10809 (the window in which `ruby_character` used to
   fault, frames 9983/11019 on the first device), the player was free at 10809, `outset-real`
   logged at frame 12420 and the run reached 14500 and exited 0 - no `SIGSEGV`, `Halt` or assert
   anywhere in the log. The same run created the save file at the name scene, so
   `mDoMemCdRWm_BuildHeader` built the card header (the `cardicon.arc` copy). On the device,
   `COS_SMOKE=save-sweep --stage sea:44:206` also saved through `d_menu_save`'s own steps: PASS,
   "saved file 1 on sea (return place sea room 44 point 128)" at frame 3182.
2. **Full regression on the JPN build, with the per-disc filter.** `native/tools/regress.sh -j 4`
   in the Linux build image, the Redump GZLJ01 image, `COS_BUILD_DIR=build/native-linux-jpn`. The
   `when`/`unless GZLJ01` lines work (the JPN `menu-travel-save-jpn.txt` target ran and passed,
   the USA line was skipped) and `options-menu` passes on this build. **93 checks, 17 failed** -
   the first full run of the suite on the Japanese build; the USA build passes it. The failures,
   by class:
   - **Two crashes in Aurora's FIFO accounting** (exit 13, 36 min into the run): `npc-variants`
     (frame 446) and `shore-foam` (frame 298) die in the Aurora FIFO thread with
     `[fatal] [aurora::gx::fifo] FIFO processing range [X, X+5) is outside buffered range [X, X)`,
     both early in `sea/Room0.arc`. Not JPN game code; the display-list FIFO's range bookkeeping -
     the two targets to bisect first.
   - **Seven timeouts under the software renderer** (exit 10, 720 s): `outset-control` x3,
     `outset-real` (QOL skip-intro), `telescope-demo` x2, `stage-hop`. The JPN paths reach fewer
     frames per second (and `outset-control`'s scripted hold moves the player 293.2 units, just
     under M13's 300 threshold: the JPN intro event ends at a different frame than the USA one,
     so the hold covers a slightly different path).
   - **JPN text/paging related check failures** (exit 1): `msg-sweep` (the Japanese routing TODO:
     4304 of 4316 indexes differ in the characters the JMessage processor read, and the font
     checks name the USA files), `save` (the card-header checks want the USA/PAL ASCII comment and
     " Save Data"; JPN writes the Japanese title and "<date>のセーブデータ", and the bytes
     round-trip correctly), `item-sweep` (the item-get event never ends), `mail-sort` (5 letters
     thrown of the 20 wanted), `picto-box` (the subject check saw nothing in the photo),
     `sun-glare --aspect 16:9` (the 4:3 line passes), `button-remap`.
   - `j3d-sweep` (exit 1): 50 manifest differences, all `/res/Stage/sea/LODALL.arc` LOD models the
     sweep never loads ("J3D2 file of the manifest not loaded").
   - `static-init`/`crash-test`/`panic-test`/`stall-test`/`timeout-test` exit non-zero by design
     (their lines in `regress_targets.txt` expect those codes).
3. **The event-manager `PUSHBUTTON` deadlock: fixed, end-to-end check still pending.**
   `specialProcMessage` now searches the message by ID every frame (`fopMsgM_SearchByID`), clears
   `l_msg` and ends the cut when the process is gone, and `FINISH`/`END` tolerate the cleared
   pointer (the old code asserted on it). It could not be proven end to end yet: the ARRIVAL_DRG2
   scenario of "The ARRIVAL_DRG2 hang" (the shared USA input, warp in the middle of the chain, JPN
   build, 2500 frames) still ends with event 38 "awake" running at frame 1806 and the warp target
   never reaches PLAY - and its log cannot tell a dead cut from a message that is merely paged and
   waiting for A. A dedicated check (A through the chain, or a test that deletes the message and
   observes the cut end) remains.
4. **The aarch64 recipe and the launcher: in the repository now.** The launcher is
   `native/tools/handheld/SwitchWaker.sh`, the five glibc shims the relink needs are
   `native/tools/handheld/dawn_compat.cpp`, and `docs/HANDHELD_BUILD.md` describes the image, the
   compiler/relink step, the runtime environment and the Panfrost limitation found on this board.
   The throwaway host scripts under `build/` stay out (they name the build machine's paths).
5. **The Chinese-modified image: checked against `docs/RUNTIME_ASSETS.md`'s revision-0 rule.**
   1,028,904,960 bytes; header GZLJ01, disc 0, revision 0, magic `C2339F3D`; FST offset 0x3B7500
   size 0xDA2C with 2337 entries and 2159 files, the last one ending at 1,028,903,882 - an exact
   trim (78 bytes of padding), every file inside the image. But the image SHA-1
   (`5f38e658…`) and the main.dol SHA-1 (`551aaad5…`, 3,770,400 bytes) differ from revision 0's
   (`1c4b574d…` / `995406d9…`), so the modification is in the game's own code as well as the text:
   `run.sh`/`disc_manifest.py --verify` rejects it (exit 14) and every host-side check must use the
   Redump image, while the runtime's disc check (ID, region, revision) accepts it. That is why the
   device boots it.

Still open, as a worklist: the two Aurora FIFO aborts (`npc-variants`, `shore-foam`), the seven
timeouts (the software renderer's frame budget and M13's 300-unit hold), the JPN expectations of
`msg-sweep`/`save`/`item-sweep`/`mail-sort`/`picto-box`/`sun-glare 16:9`/`button-remap` and the
`j3d-sweep` LOD entries, and the dedicated check for the PUSHBUTTON fix.

Build-side note: the repository stores Aurora patch `0022-pad-trigger-axis-buttons.patch` and
`switch/native/aurora/patches/0017-switch-deko3d-depth-copies.patch` with LF, but `core.autocrlf`
checked those two working-tree copies out with CRLF, and the container's `git apply` then refused
them (the configure died on 0022). Normalizing the two worktree files to LF fixes it; no
repository change is needed. The same applies to the check inputs: with
`native/check/regress_targets.txt` in CRLF every target that takes `--input` failed as a usage
error (`COS_INPUT …\r: cannot open`) because the trailing CR stays in the argument; normalizing it
to LF makes the suite run.
