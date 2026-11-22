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
| F0: Vanilla Conquer on PC | open |

## Build flags

`-mcpu=68060 -mhard-float -O2 -noixemul -fno-exceptions -fno-rtti` (smoke test). Check 64-bit MULS.L with `WarpBench/find64.sh`.

## Decisions

- **22.11.2026, backend:** own native AmigaOS backend (`video_amiga.cpp`, keyboard, AHI), no mini-SDL2. Reason: code speed is the priority and the Warp rules (Fast RAM frame buffer, MOVE16 to VRAM, hardware cursor, BltBitMap inside the screen, dirty rectangles) need full control of the frame path. Touch points: `common/video.h` (video_sdl2, video_sdl1, video_ddraw, video_null), `wwkeyboard_*`, `audio.h`/`soundio_openal.cpp`. Order: F2 on `video_null`, F3 `video_amiga.cpp`. Risk: game code using SDL directly (`options.cpp`, `startup.cpp`, ...), to be counted in F2.

## Game data (not in git, `data/` and `downloads/` are ignored)

- `data/td/`: files from the GDI disc (AUD, CONQUER, DESERT, GENERAL, MOVIES, SCORES, SOUNDS, TEMPERAT, WINTER, CCLOCAL MIX, SIZZLE VQA/VQP). AUD, CONQUER, DESERT, SCORES, SOUNDS, TEMPERAT, WINTER are byte-identical on both discs.
- `data/td-nod/`: `GENERAL.MIX` and `MOVIES.MIX` from the NOD disc (they differ from GDI). How the engine picks the campaign files (one flat dir vs per-CD lookup in `cdfile.cpp`) is open, to be settled in F2.
- Raw extracted ISOs: `downloads/iso-extracted/`.
