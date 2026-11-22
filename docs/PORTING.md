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
