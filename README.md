# Command & Conquer: Tiberian Dawn for the Amiga (Warp cards)

A native AmigaOS 3.x port of [Vanilla Conquer](https://github.com/TheAssemblyArmada/Vanilla-Conquer),
the GPL engine of Tiberian Dawn.

**Hardware.** Made for Amiga 500 / 1200 / 3000 / 4000 with a **Warp** card (Warp 560, 1240, 1260,
1260 Extreme, 7060), ported and tested on such cards: it reaches its speed through the fast graphics
(RTG) of the Warp. Other Amigas with a 68040/68060 and Picasso96 RTG run it, but slower.
Tested on: A1200 + Warp 1260 Extreme (68060, 100 MHz), AmigaOS 3.2.2.1.

## What it has
- Tiberian Dawn (GDI and Nod campaigns), 640x400 with 256 colours, mouse control
- own AmigaOS back end (no SDL): Picasso96 screen, `ReadEClock` timing, AHI or Paula sound and music
- starts from the icon (Tool Types `SOUND=AHI|PAULA|OFF`) or from a Shell, Ctrl-C closes it
- not yet: game movies (skipped), multiplayer, Red Alert

## Play it
The ZIP is in `releases/amiga`:

    UnZip CommandConquer-0.1.zip -d Work:Games

The game data is **not included**: put the `*.MIX` files of Command & Conquer (Gold, the freeware
release by Electronic Arts) into `CommandConquer/data` (see `data/ReadMe.txt`).

## Requirements
AmigaOS 3.2, Picasso96, 68060.library (or 040), AHI (on a Warp card it plays through HDMI).

## Build
The engine is not copied here: this repository keeps the patches (`patches/`, made on a checkout of
Vanilla Conquer on the branch `amiga`) and the tools.

    git clone https://github.com/TheAssemblyArmada/Vanilla-Conquer vanilla-conquer
    cd vanilla-conquer && git checkout -b amiga && git am ../patches/*.patch
    # bebbo m68k-amigaos-gcc 6.5.0b in Docker (image amixcom-build): tools/build_amiga.sh

Release `-O2` is required: gcc 6.5 at `-O3` decodes some graphics wrongly (see `docs/PORTING.md`).
`tools/make_release.sh` builds the ZIP from `build/amiga/CommandConquer`.

## Licence
GNU GPL v3 (`docs/release/amiga/COPYING`). The game data belongs to Electronic Arts and is not included.
