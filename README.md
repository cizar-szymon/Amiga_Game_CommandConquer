# Command & Conquer: Tiberian Dawn and Red Alert for the Amiga (Warp cards)

A native AmigaOS 3.x port of [Vanilla Conquer](https://github.com/TheAssemblyArmada/Vanilla-Conquer),
the GPL engine of Tiberian Dawn and Red Alert.

**Hardware.** Made for Amiga 500 / 1200 / 3000 / 4000 with a **Warp** card (Warp 560, 1240, 1260,
1260 Extreme, 7060), ported and tested on such cards: it reaches its speed through the fast graphics
(RTG) of the Warp. Other Amigas with a 68040/68060 and Picasso96 RTG run it, but slower.
Tested on: A1200 + Warp 1260 Extreme and A4000 + Warp 7060 (68060), AmigaOS 3.2.

## What it has
- **Tiberian Dawn** (GDI and Nod campaigns) and **Red Alert** (Allied and Soviet campaigns), 640x400 with
  256 colours, mouse control
- the game movies, with sound
- own AmigaOS back end (no SDL): Picasso96 screen, `ReadEClock` timing, sound and music through the
  Amiga's own audio chip (Paula, default) or AHI
- starts from the icon (Tool Types `SOUND=PAULA|AHI|OFF`, `MOVIES=OFF`) or from a Shell, Ctrl-C closes it
- bug reports with the F10 key: a window in the style of the game's menus sends the report (e-mail, words,
  state of the game, picture of the screen) to the game server (needs a TCP/IP stack)
- not yet: multiplayer

## Play it
Each game is its own package: the ZIP of Tiberian Dawn is in `releases/amiga`, the ZIP of Red Alert is in
`releases/amiga` of the branch `release-ra`:

    UnZip TiberianDawn-0.2.zip -d Work:Games
    UnZip RedAlert-0.2.zip -d Work:Games

The game data is **not included** (the freeware releases by Electronic Arts): see `data/ReadMe.txt` of
each package. Tiberian Dawn: the `*.MIX` files of Command & Conquer (Gold). Red Alert: `MAIN.MIX` and
`REDALERT.MIX` of the Allied or Soviet disc.

## Requirements
AmigaOS 3.2, Picasso96, 68060.library (or 040), the game data. AHI only for `SOUND=AHI`.
A TCP/IP stack (Roadshow, Miami, Genesis) only for sending bug reports.

## Build
The engine is not copied here: this repository keeps the patches (`patches/`, made on a checkout of
Vanilla Conquer on the branch `amiga`) and the tools.

    git clone https://github.com/TheAssemblyArmada/Vanilla-Conquer vanilla-conquer
    cd vanilla-conquer && git checkout -b amiga && git am ../patches/*.patch
    # bebbo m68k-amigaos-gcc 6.5.0b in Docker (image amixcom-build): tools/build_all.sh

Release `-O2` is required: gcc 6.5 at `-O3` decodes some graphics wrongly (see `docs/PORTING.md`).
`tools/make_release.sh td|ra` builds the ZIP from `build/amiga/vanillatd` or `vanillara`;
`tools/push_release_branch.sh` puts the Red Alert release on its branch.

## Branches

- `main`: the sources (patches, tools, notes) and the Tiberian Dawn release.
- `release-ra`: only the Red Alert release (`releases/amiga`), for WARP HUB, which takes one game from one place of a repository. Its history is separate from `main`; it is never merged.

## Licence
GNU GPL v3 (`docs/release/amiga/COPYING`). The game data belongs to Electronic Arts and is not included.
