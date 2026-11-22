# Porting log: Command & Conquer (Vanilla Conquer) on Amiga, Warp

Plan and phases: `CLAUDE.md`. Test platform for now: WinUAE only
(`C:\amiga-dev\winuae\CommandConquer_A1200_OS32_net.uae`, volumes `CNC:` = build/amiga, `CNCDATA:` = data).

## State

| Step | State |
|---|---|
| F0: Docker toolchain (amixcom-build, bebbo gcc 6.5.0b, C++11) | done |
| F0: Vanilla Conquer cloned (`vanilla-conquer/`, git-ignored, 9103a8e) | done |
| F0: 68060 + FPU C++ smoke test (`tools/smoke`): 640x400x8 RTG screen in WinUAE | done |
| F0: game data (freeware TD/RA MIX files) in `data/` | missing |
| F0: Vanilla Conquer on PC | open |

## Build flags

`-mcpu=68060 -mhard-float -O2 -noixemul -fno-exceptions -fno-rtti` (smoke test). Check 64-bit MULS.L with `WarpBench/find64.sh`.

## Decisions

- **22.11.2026, backend:** own native AmigaOS backend (`video_amiga.cpp`, keyboard, AHI), no mini-SDL2. Reason: code speed is the priority and the Warp rules (Fast RAM frame buffer, MOVE16 to VRAM, hardware cursor, BltBitMap inside the screen, dirty rectangles) need full control of the frame path. Touch points: `common/video.h` (video_sdl2, video_sdl1, video_ddraw, video_null), `wwkeyboard_*`, `audio.h`/`soundio_openal.cpp`. Order: F2 on `video_null`, F3 `video_amiga.cpp`. Risk: game code using SDL directly (`options.cpp`, `startup.cpp`, ...), to be counted in F2.
