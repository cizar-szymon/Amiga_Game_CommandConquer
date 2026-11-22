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
