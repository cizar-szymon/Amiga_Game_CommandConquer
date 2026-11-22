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

Breakdown after that (us/frame): Map.Render 15 000 (Draw_It 7 000-10 000, Blit_Display HidPage->SeenBuff 7 400), present (WritePixelArray 640x400 LUT8) 14 600, Logic.AI 2 400-5 000, input 300, sort 190, color cycle 220, LoadRGB32 20. Next: present via locked bitmap + MOVE16 (plan: ~60 MB/s = 4 ms), skip unchanged frames, remove the HidPage->SeenBuff copy, Draw_It inner loops.
