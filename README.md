# Dead Mobsters for the Datafrog SF3500

*Dead Mobsters* is a freeware run-and-gun platformer by Outhouse Software, made with the Fenix game engine. Two
mobsters, back from the dead, clean up the streets: through the suburbs, the sewers, the scaffolding and the
factory, with a boss at the end of each. This repository ports it to the Datafrog SF3500 handheld (MIPS32, no
GPU, Linux, display driver and gamepad supplied by the TreeFrogUI front-end).

The contents of the game's files are the author's, unchanged (`game/`; some file names are lower-cased to match
what the game asks for). Only the interpreter is built here: the game ships as compiled Fenix 0.80 bytecode with
no script source, so the **Fenix 0.80** interpreter (`fenix080/`, GPL) is built for the SF3500.

## Controls

| Button | Action |
|---|---|
| D-pad left and right | Walk |
| B, or D-pad up | Jump |
| A or Y | Shoot |
| R1 or X / L1 | Next / previous weapon |
| D-pad up and down | Move through the title menu |
| A or Start | Choose in the title menu; continue after Game Over |
| Start, while playing | Pause and resume |
| Start, during the intro | Skip it (after you have watched it once) |
| Select | Back to the title menu while playing |
| Select + Start | Leave the game at any time |

The author's readme says Ctrl shoots. The game itself tests **Alt** for shooting, and jumps on Space or Ctrl, so
the buttons follow what the game does.

## How it works

Fenix 0.80 is written against SDL 1.2. The SF3500 has no SDL video or input device, so `shim/SDL.h` declares
the small part of the SDL 1.2 API that Fenix uses, with every function renamed (`SDL_Init` is `fxsdl_Init`), and
`shim/fxsdl.c` implements it directly:

* **video**: Fenix draws into an 8-bit surface owned by the shim; each flip converts it through the palette to
  RGB565 and hands it to the display driver (`sf3500/hwdisp.c`). `SF3500_ASPECT=16:9` pads the 4:3 picture.
* **input**: the pad's shared memory (`sf3500/joykey.c`) becomes SDL 1.2 key state and key events. The game
  only has number keys for its four weapons, so for L1 / R1 / X the shim reads which weapon is current and
  presses the key next to it.
* **timing**: `SDL_GetTicks` / `SDL_Delay` on the monotonic clock.
* **audio** (`shim/fxsdl_audio.c`): real SDL2 and SDL2_mixer with SDL's dummy audio device for pacing; the mixed
  output is tapped with `Mix_SetPostMix()` and written to the device's audio ring (`sf3500/hwaudio.c`) at its
  native 48000 Hz. Eight mixer channels are allocated, the most Fenix 0.80 can keep track of.

### Added by the port

The original game has neither of these, and its files are not changed for them. `shim/mobsters.c` holds what
the port knows about the game's bytecode: where the main program is waiting says whether the intro, the title
menu or a level is on screen. Those positions are checked when the game runs; if they do not match, both
features stay off.

* **Pause.** Start during a level. The shim waits inside the input poll, holds the sound, and hides the paused
  time from the game's clock.
* **Intro skip.** The first time, the logo and title sequence play through. Reaching the title menu creates a
  file, `intro_seen`; from then on **PRESS START TO SKIP** is shown. The skip ends the intro's two timed waits
  early, and the game's own code carries on to the menu. Delete `intro_seen` to make it unskippable again.

## Build

You need the SF3500 MIPS toolchain, cmake, make, and three upstream tarballs in `deps/` (checksums are verified):

| File | From |
|---|---|
| `SDL2-2.0.14.tar.gz` | https://github.com/libsdl-org/SDL/releases/download/release-2.0.14/SDL2-2.0.14.tar.gz |
| `SDL2_mixer-2.6.3.tar.gz` | https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.6.3/SDL2_mixer-2.6.3.tar.gz |
| `libxmp-lite-4.6.0.tar.gz` | https://github.com/libxmp/libxmp/releases/download/libxmp-4.6.0/libxmp-lite-4.6.0.tar.gz |

```sh
SF3500_TC=/path/to/toolchain/host ./build_sf3500.sh   # build-sf3500/fxi.mipsel
./package.sh                                          # package/ for the card's roms/ports
```

On the device the game runs as `fxi.mipsel -b mobsters.dcb`. The executable's name must start with `fxi`.

## Testing without the device

`SF3500_TC=... tools/qrun.sh <folder> -b mobsters.dcb` runs it under qemu-user; the folder holds `fxi.mipsel`
and the files from `game/`. Hooks (no cost unless set): `SF3500_KEYSCRIPT="1100:E:5,1300:J:6"`
(frame:letter[:frames]; the letters are listed at the top of `shim/fxsdl.c`),
`SF3500_SHOT_PREFIX=p SF3500_SHOT_FRAMES=100,200` (writes `p_100.ppm`...), `SF3500_QUIT_FRAME=n`,
`SF3500_PAUSE_TEST_POLLS=n` (a pause ends by itself). Without `intro_seen` the title menu appears after about
1000 frames.

`tools/dcbdis.py fenix080 game/mobsters.dcb` disassembles the game's bytecode. Developer notes:
`DEV-NOTES.md`.

## Licences

The game is the author's freeware; its readme (`game/readme.txt`) asks for it to be distributed to as many people
as possible. The Fenix interpreter and the shim are GPL version 2 or later; the glue in `sf3500/` is derived from
TreeFrogUI's SF3000 code; scripts and packaging are under the Unlicense. See `licenses/CREDITS.txt`.
