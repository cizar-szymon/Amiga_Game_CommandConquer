# Porting log: Command & Conquer (Vanilla Conquer) on Amiga, Warp

Plan and phases: `CLAUDE.md`. Test platform for now: WinUAE only
(`C:\amiga-dev\winuae\CommandConquer_A1200_OS32_net.uae`, volumes `CNC:` = build/amiga, `CNCDATA:` = data).

## State

| Step | State |
|---|---|
| F0: Docker toolchain (amixcom-build, bebbo gcc 6.5.0b, C++11) | done |
| F0: Vanilla Conquer cloned (`vanilla-conquer/`, git-ignored, 9103a8e) | done |
| F0: 68060 + FPU C++ smoke test (`tools/smoke`): 640x400x8 RTG screen in WinUAE | done |
| F0: game data TD (freeware Gold ISO GDI95 + NOD95 from archive.org `gdi-95`) in `data/td`, `data/td-nod` | done |
| F0: game data RA | missing |
| F0: Vanilla Conquer on PC | skipped, going straight to the Amiga build (WinUAE) |

## Build flags

`-mcpu=68060 -mhard-float -O2 -noixemul -fno-exceptions -fno-rtti` (smoke test). Check 64-bit MULS.L with `WarpBench/find64.sh`.

## Decisions

- **22.11.2026, backend:** own native AmigaOS backend (`video_amiga.cpp`, keyboard, AHI), no mini-SDL2. Reason: code speed is the priority and the Warp rules (Fast RAM frame buffer, MOVE16 to VRAM, hardware cursor, BltBitMap inside the screen, dirty rectangles) need full control of the frame path. Touch points: `common/video.h` (video_sdl2, video_sdl1, video_ddraw, video_null), `wwkeyboard_*`, `audio.h`/`soundio_openal.cpp`. Order: F2 on `video_null`, F3 `video_amiga.cpp`. Risk: game code using SDL directly (`options.cpp`, `startup.cpp`, ...), to be counted in F2.

## Game data (not in git, `data/` and `downloads/` are ignored)

Layout expected by the engine (`Change_Local_Dir` in `tiberiandawn/conquer.cpp` looks for local folders `gdi`, `nod`, `covertops` under the data path):

- `data/td/`: files identical on both discs (AUD, CONQUER, DESERT, SCORES, SOUNDS, TEMPERAT, WINTER, CCLOCAL MIX, SIZZLE VQA/VQP).
- `data/td/gdi/`: `GENERAL.MIX`, `MOVIES.MIX` from the GDI disc.
- `data/td/nod/`: `GENERAL.MIX`, `MOVIES.MIX` from the NOD disc.
- Source: freeware Gold ISOs (archive.org `gdi-95`: GDI95.ISO, NOD95.ISO), extracted with Windows `tar.exe`; raw extraction in `downloads/iso-extracted/`.
- WinUAE: `CNCDATA:` = `data/`; `build/amiga/CONQUER.INI` (template `tools/winuae/CONQUER.INI`) sets `DataPath=CNCDATA:td`, `UserPath=RAM:cnc`.

## F2 log (22.11.2026)

Build: `docker run --rm -e CNC_BUILD=Debug -v cnc-build:/z -v <repo>:/w amixcom-build sh /w/tools/build_amiga.sh` (Release without `CNC_BUILD`). Toolchain file `tools/cmake/amiga-m68k.cmake` (`-mcpu=68060 -mhard-float -noixemul`). Vanilla Conquer sits in `vanilla-conquer/` (own git, branch `amiga`, ignored here); our changes are exported as `patches/*.patch`.

| Step | State |
|---|---|
| TD builds and links for 68060+FPU with `video_null`, no SDL2/OpenAL, no networking | done (binary 1.8 MB Debug, 2.3 MB Release) |
| AmigaOS paths (`common/paths_amiga.cpp`: PROGDIR:, PROGDIR:data, PROGDIR:user) | done |
| Keyboard backend `wwkeyboard_amiga.cpp` | stub, F3 fills it |
| `ms_sleep`/`us_sleep` | `Delay()` placeholder, 20 ms granularity, F3 replaces with timer.device |
| Start in WinUAE: engine initialises, loads CCLOCAL.MIX, reaches the main loop | done (null video, no input) |
| Endianness audit | open (needs video output to compare) |

Findings:
- `Prog_End(msg, true)` is a deliberate crash in the original (`*((int*)0) = 0`), so a fatal error shows as SmartCrash trap #7; the reason is in `RAM:cnc-debug.txt` (Debug build only; log is appended and closed per line so it can be read while running).
- AmigaDOS: a trailing `/` means the parent directory, so engine code that appends `SEP` to a path must only concatenate strings (`Concatenate_Paths` does).
- `bitfields.h` uses `__attribute__((ms_struct))`, ignored by m68k gcc (warning). Bit-field layout on big endian is the main correctness risk and needs an explicit check.
- Linker prints `duplicate section .text._ZN4Pipe3EndEv has different size` (inline `Pipe::End` differs between translation units); to look at.
- Missing optional files (`rules.ini`, `*_slct.aud`) are logged as errors by `RawFileClass`; harmless.
- Debug build runs without crash to the main loop; Release build not yet run.

## F3 start (22.11.2026, WinUAE)

- `common/video_amiga.cpp`: P96 8-bit CLUT screen 640x400, surfaces in Fast RAM (16-byte aligned), palette via LoadRGB32, one `WritePixelArray(RECTFMT_LUT8)` per frame from `Video_Render_Frame()`, software cursor. F10 dumps the frame to `RAM:cnc-frame.bin` (`tools/frame2png.py` -> PNG; AmiAgent `shot` has wrong colours on 8-bit screens).
- `common/wwkeyboard_amiga.cpp`: IDCMP keys/mouse -> VK_* (Windows codes), Ctrl-C exits.
- `common/amiga_watchdog.cpp`: helper process, writes `RAM:cnc-hang.txt` (saved PC + stack; FPU registers sit in the context, map code words with `nm`) when the heartbeat stops for 4 s.
- Result: intro movie (VQA from MOVIES.MIX) plays with the right palette; mission SCG01EA loads and draws (`docs/mission1-winuae.png`). Endianness of MIX, VQA, palettes, shapes, templates is fine.

Bugs found and fixed:
- **amiga.lib shadows libnix:** linking `-lamiga` pulled in its `sprintf` (RawDoFmt, 16-bit `%d`/`%c`), so `sprintf("SC%c%02d...")` returned `SC`. Do not link amiga.lib; the `*Tags()` helpers are inline in the NDK headers.
- **Choose_Side without its assets** (`CHOOSE.WSA`, `SIDES.PAL`, `GDI_SLCT.AUD`, `NOD_SLCT.AUD`, `STRUGGLE.AUD` are in no MIX of the Gold ISOs): the original dereferences a null animation; on a 68k, address 0 is readable and `Copy_Delta_Buffer` wrote over memory, which froze Amiga 1 (22.11.2026, first real-Amiga test, no MOVIES.MIX there). Now the screen is skipped (player stays GDI).
- Keyboard `Clear()` and stack dumps: `Prog_End(..., true)` is a deliberate NULL write (SmartCrash trap).

Open: side-select assets, sidebar/interface not visible yet (probably missing assets or the `HidPage`/sidebar surface), music (SCORES), sound, real-Amiga run, performance.

## First run on a real Amiga (Amiga 1, A1200 + Warp 1260 Extreme, 22.11.2026)

Debug build, mission SCG01EA, static scene, 640x400x8, whole frame every time: `PROF 50 frames: render 14.3 ms/frame, wall 3.3-3.5 s per 50 frames (14-15 fps)`. Render = `WritePixelArray` of 256 KB (~18 MB/s). The remaining ~54 ms per frame is game and engine work (map into HidPage, `Interpolate_2X_Scale`, blits); F4 profiles it. Data on the Amiga: only the small MIX files (no MOVIES/SCORES) in `Work:Games/CommandConquer-test/data/td`.
First attempt froze the machine (see above, Choose_Side null animation); after the fix it runs and stops on Ctrl-C (`stop vanillatd`).

## Interface and data set (22.11.2026)

- The ISOs `GDI95`/`NOD95` (archive.org `gdi-95`) are the German Windows 95 edition without the patch archives. The hi-res sidebar shapes (`HSIDE1.SHP`, `HSTRIP.SHP`, ... in `UPDATEC.MIX`), the side-select assets (`TRANSIT.MIX`), `12GREEN.FNT`/`SIDES.PAL` (`UPDATE.MIX`) and the English `CCLOCAL.MIX`/`LOCAL.MIX` come from the repack `command-aand-conquer-gold` (archive.org, Gold 1.04a; single files fetched from inside the zip). Dev set: `data/td2/` (flat; `Change_Local_Dir` now accepts the data dir itself as the `.` disc, AmigaDOS has no `.` entry). Not the officially supported source (README of Vanilla Conquer) - decide what players are told to use before a release.
- In 640x400 the sidebar is hidden until the **Sidebar** tab (top right) is clicked; `docs/mission1-sidebar.png` shows it working (tabs, radar logo, Repair/Sell/Map, two build columns, power bar).
- Test switches in the program drawer: `nomovies` (skips intro, side select and briefings; gameplay tests start in the mission), `autoinput.on` + `autoinput.txt` (scripts in `tools/autoinput/`: wait, move, click, rclick, drag, key, dump, quit).

## Performance table (Amiga 1, A1200 + Warp 1260 Extreme, 640x400x8, GDI mission 1, sidebar open, GameSpeed=0)

Measured with the in-program profiler (`RAM:cnc-log.txt`, lines `PROF` and `SLOTS`, ReadEClock, per frame; Debug build, `autoinput` script `tools/autoinput/perf1.txt`).

| Step | us/frame | fps | Note |
|---|---|---|---|
| Baseline | 79 000 | 12 | 45 ms of it asleep in Frame_Limiter (`std::chrono::steady_clock` too coarse -> sleep requests of 8.3, 16.6, 25 ... ms; first `Delay()` based sleep, then timer.device, same result) |
| E-Clock time source (`Amiga_Micros64`) in Frame_Limiter and `WinTimerClass::Now` | 33 000 | 28-30 | sleep 0 |

Breakdown after the clock fix (us/frame): Map.Render 15 000 (Draw_It 7 000-10 000, Blit_Display HidPage->SeenBuff 7 400), present (WritePixelArray 640x400 LUT8) 14 600, Logic.AI 2 400-5 000, input 300, sort 190, color cycle 220, LoadRGB32 20. Next: present via locked bitmap + MOVE16 (plan: ~60 MB/s = 4 ms), skip unchanged frames, remove the HidPage->SeenBuff copy, Draw_It inner loops.

| Present via `LockBitMapTags` + MOVE16 (after `CacheClearE`) instead of `WritePixelArray`; software cursor drawn with save-under instead of copying the frame | 25 000 | 39-41 | present 14 600 -> 5 300 |
| No HidPage->SeenBuff copy: `Blit_Display` hands HidPage (16-byte aligned buffer) to the video layer, SeenBuff is filled only when the engine touches it (`Flush_Hid`) | 16 000-18 000 | **56-63** | blitdisplay 7 400 -> 6; mainloop 16-18 ms: Draw_It 7 100-8 400, present 5 300, Logic.AI 2 400-4 600 |

Open for F4: Draw_It inner loops (tiles, shapes), present only when something changed (needs a dirty flag from the engine), Release build without profiler, heavy scenes (battles, many units), game speed settings. Numbers are one static scene on Amiga 1; WinUAE numbers mean nothing.

## Heavy scene: battle (Amiga 1, 22.11.2026)

Scenario `tools/autoinput/battle_SCG01EA.INI` (generated by `tools/make_battle_scenario.py`: 42 vehicles and 52 infantry of GDI and Nod on Hunt in the open ground, no win/lose triggers; copy to `RAM:cnc/SCG01EA.INI`), script `tools/autoinput/battle.txt`, `GameSpeed=0`, 640x400x8, Debug build. Probes read the Warp FPGA E-Clock (90 ns) when SysCfg id >= 0x0123456A, else ReadEClock; the game itself keeps ReadEClock (Amiga 1 has firmware with the FPGA counter, ReadEClock 530 ns).

fps follows the number of living units: **20 fps** at the start of the fight (about 95 units), 35 fps with about half left, up to 90 fps at the end.

Heaviest moment (us per frame, mainloop 45 000-49 000):

| Part | us |
|---|---|
| Layer ground (units, infantry, buildings: `Render`) | 13 600 |
| Terrain icons (`Redraw_Icons`) | 6 000-6 800 |
| Shroud (`Redraw_Shadow`) | 3 400-4 400 |
| Layer air / top | 500-900 |
| `UnitClass::AI` (42 units) | 8 400-9 800 |
| `InfantryClass::AI` (52 units) | 4 300-8 900 |
| other AI (anim, bullet, ...) | ~1 500 |
| present (MOVE16) | 5 300 |

Targets for F4 in this order: shape drawing in the ground layer (~140 us per object), unit/infantry AI (path finding and target search, ~200 us per unit), icon and shroud drawing.

## Shape blitters: what the 68060 on the Warp does (22.11.2026, Amiga 1; `tools/shapebench.c`, in-game `SHAPES` log line)

In the battle scene 11.9 ms of the 13.6 ms ground layer are three loops of `keybuff.cpp` (`BF_Ghost_Fading_Trans` style 7: ~100 calls, 140 kpixels, 8.5 ms; `BF_Ghost_Trans` style 3: 2.7 ms; `BF_Trans` style 1: 0.5 ms), 60-100 ns per pixel.

Machine facts measured with the benchmark (CPU 100 MHz, subq+bne loop = 10.2 ns, CACR 0x80001111: caches and copyback on, no write allocate; `CacheControl(CACRF_WriteAllocate)` is accepted but changes nothing):
- a **byte store costs ~55 ns, also into a hot cache line**; a long store 12-24 ns; `movem.l` of 4 registers 64 ns (4 ns per byte). The cost is per store instruction, so writing fewer, wider words is what pays off.
- libnix `memcpy` is a function call even for 4 bytes (it made a first "optimised" version slower). Copying a 24x24 terrain icon takes 4.3 us (7 us with an unaligned destination) with `memcpy`, six long moves or `movem.l` alike.
- compiler flags (`-O3`, `-funroll-loops`) made the loops slower.

Variants tried on a 36x36 sprite (ns per pixel, 25 / 60 / 95 % opaque): original 96 / 150 / 163; fade table folded into a 256 entry table 86 / 119 / 121; hand written 68k loop 90 / 115 / 112; four pixels per long read-modify-write in C 126 / 195 / 213 (the shifts cost more than the saved stores). **In the game none of them helped** (style 7: 8.5 ms original, 9.4 ms with the folded-table version): in the real frame the destination lines and the sprite data are cold and the loop is bound by memory, not by instructions. Reverted, nothing of it is in the port.

Conclusion: the shape drawing will only get faster by drawing fewer pixels or by wide stores (compose a row in registers, one long or `MOVE16` write), not by tighter byte loops. Next target: `UnitClass::AI` / `InfantryClass::AI` (14-17 ms in the battle).

## Battle scene: where the logic time goes (Amiga 1, 22.11.2026)

Probes inside the AI (`PROF_SCOPE_N`, compiled only with `CNC_PROFILE=1`, `docker run -e CNC_PROFILE=1 ...`; slots in `SLOTS2`, counters in `COUNTS`). Heaviest moment, us per frame (inclusive, nested):

| Function | us |
|---|---|
| `UnitClass::AI` (42 units) | 9 500-12 000, of it `DriveClass::AI` 7 000-9 400 |
| `InfantryClass::AI` (52 units) | 5 000-7 700 |
| `DriveClass::While_Moving` | 2 200-5 700 |
| `DriveClass::Start_Of_Move` (calls `Basic_Path` -> `Find_Path`) | 2 000-5 800 |
| `FootClass::Find_Path` | 1 300-5 000 |
| `FootClass::Mark` (outermost, 450 calls/frame) | 10 500-12 000, of it `ObjectClass::Mark` 7 000-8 000, `CellClass::Redraw_Objects` (outermost, 1 100 calls/frame) 5 300-5 800 |
| `Greatest_Threat` (all classes), `Evaluate_Object` | < 400 |
| `TechnoClass::AI`, `MissionClass::AI` | ~900, ~500 |

Calls per frame: `Mark` 450 (foot) / 530 (object), `Occupy_Down/Up` 50, `Overlap_Down` 160, `Overlap_Up` 330, `Redraw_Objects` 1 100.

Experiments: (1) **forced full redraw when more than 300 `Redraw_Objects` calls per frame** (skip `Mark(MARK_CHANGE)` and `Redraw_Objects`, `Map.Flag_To_Redraw(true)` before `Render`): logic -4 ms, but Map.Render 25 -> 49 ms, net loss; the engine dirty-cell scheme is worth keeping. Reverted. (2) Release build instead of Debug: no difference (the `_DEBUG` checks are empty). (3) Probe overhead: heavy scene 48 ms without probes against 49-51 ms with them.

Clean build (no probes) in the battle: 48 ms per frame (21 fps) at the start of the fight with ~95 units, 120 fps at the end. That is enough for the original game speed (15 ticks/s = 66 ms) even in this scene, not yet for `GameSpeed=2` (33 ms).

Candidates left: path finding (re-planned too often by Hunt units; a result cache or a lower re-plan rate changes behaviour, so it needs a decision), the cost of `Mark` (virtual calls and cell array indexing, ~24 us per call), shape drawing by wide stores (~4 ms), `Redraw_Icons` (6 ms, 300 cells x 20 us).

## Terrain icons (Amiga 1, 22.11.2026)

`Redraw_Icons` 6 700-7 400 us per frame in the battle, of which `CellClass::Draw_It` 5 100-5 400, `GraphicViewPortClass::Draw_Stamp` 5 100, lock/unlock 300, `Buffer_Draw_Stamp_Clip` 4 400-4 700. Only **112-117 icons per frame** (not 300), all on the plain `memcpy` row path (transparent path 0, smudges and overlays < 100 us). That is **~38 us per 24x24 icon**, against 4.3 us for the same copy in `tools/shapebench.c`.

Explanation: the benchmark writes into a block that stays in the 96 KB L2; in the game the destination (HidPage, 256 KB) and the icon data are cold, every icon row lands in another 128-byte L2 line and every miss costs 0.6-0.8 us (DDR3). The same holds for the sprite blitters (about 85 us per call for 1.3 KB of source and 36 destination rows). The copy and loop code is not the limit, the memory latency is; micro-optimising loops cannot help.

Levers that remain: touch fewer lines (fewer or smaller redraws), keep the working set inside L2 (smaller or banded HidPage), overlap misses (none on the 060: no prefetch, blocking cache), or skip the HidPage altogether for static terrain. None is a local change.

## Sound (22.11.2026): `common/soundio_amiga.cpp`

Backend for the `SoundImp_*` interface of `soundio_common.cpp` (5 trackers, 22050 Hz mono 16 bit): software mixer in its own task (priority 12), output 22050 Hz stereo 16 bit through **Paula** (`audio.device`, left channel 0/3, right 1/2, three 8 bit buffers of 768 frames, period rounded up: PAL 161 = 22 030 Hz), **AHI** (two dynamic sounds in turn, `AHIA_SoundFunc` hook with an asm trampoline because amiga.lib must not be linked, block 1024 frames), or a **dump** to `RAM:cnc-sound.raw` (16 bit big endian stereo; `tools/analyze_sound.py` gives level per second and a WAV).
Choice by the file `PROGDIR:sound`: `paula` (default; falls back to AHI when the channels are busy), `ahi`, `ahi N` (block size 256-1536), `dump`, `off`. Log: `RAM:cnc-sound-log.txt` (backend, blocks, underruns). `exit()` after Ctrl-C does not call `Sound_End`, so shutdown is also registered with `atexit` (otherwise the mixer task outlives the program and the machine hangs).
Results on Amiga 1: Paula 768 frames 0 underruns in 87 s; AHI 768 frames 87 misses (two sound switches before the task ran, every ~0.7 s, independent of the task priority), 1024 / 1280 / 1536 frames 0 misses. In WinUAE (dump) the music is decoded correctly (level ~8000 rms, 800-1100 zero crossings per second, no byte swapped noise). `Sample_Read` in `soundio_common.cpp` now converts the little endian size of the AUD header.

**Not done / open:** nobody has listened to Paula or AHI output yet (ask the user, the game runs with `Echo paula >Work:Games/CommandConquer-test/sound`); fps cost of the mixer (interrupted measurement: battle scene, GameSpeed 0, `sound` = off / paula / ahi); sound of the VQA movies (`vqaaudio_null.cpp`, movies are switched off); EVA speech (`SPEECH.MIX`) and music themes not checked in play; sound options in a launcher or ini instead of the `sound` file.

## HANDOFF (22.11.2026): where to continue

State: Tiberian Dawn runs natively on Amiga 1 (A1200 + Warp 1260X), WinUAE for correctness only, speed numbers only from Amiga 1. Build: `docker run --rm -e CNC_BUILD=Release [-e CNC_PROFILE=1] -v cnc-build:/z -v "C:/Workprojects/AmigaGAMES/CommandConquer:/w" amixcom-build sh /w/tools/build_amiga.sh` -> `build/amiga/vanillatd` (the image must exist; start Docker Desktop first). Test game on Amiga 1: `Work:Games/CommandConquer-test` (data in `data/td2`, `nomovies`, `sound`, `autoinput.on` + `autoinput.txt`, `CONQUER.INI`; scenario/settings in `RAM:cnc`, lost on reboot). Scripts: `tools/autoinput/*.txt` (battle.txt, perf1.txt), battle scenario `tools/autoinput/battle_SCG01EA.INI` to `RAM:cnc/SCG01EA.INI`. Profiler output: `RAM:cnc-log.txt` (PROF, SLOTS, SLOTS2, SHAPES, COUNTS). Engine changes live in `vanilla-conquer/` (own git, branch `amiga`, also exported to `patches/`); data is not in git.

Open work, in order: (1) listen test of Paula and AHI, fps cost of sound; (2) sound for movies and EVA speech; (3) Red Alert data and build; (4) renderer speed (see the memory-latency notes: banded or smaller HidPage, wide stores) and path finding cost; (5) release (ZIP, WARP HUB layout, README for Warp cards, which data source players use - the freeware Gold ISOs lack the patch archives, see the data notes); (6) options (launcher or ini) instead of the test switch files.

Pitfalls: never link amiga.lib (its sprintf breaks `%c`/`%d`); do not trust WinUAE for speed; check the Amiga with `titles` and `Status` before starting a game; a stray game on Amiga 1 can freeze the machine (reset needed, the user does it); python in Git Bash: use `C:/...` paths and avoid `
` inside heredocs (use the Write/Edit tools for C++ with escapes).
