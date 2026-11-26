# Dead Mobsters for the Datafrog SF3500

*Dead Mobsters* is a freeware run-and-gun platformer by Outhouse Software, made with the Fenix game engine. Two
mobsters, back from the dead, clean up the streets: through the suburbs, the sewers, the scaffolding and the
factory, with a boss at the end of each. This repository ports it to the Datafrog SF3500 handheld (MIPS32, no
GPU, Linux, display driver and gamepad supplied by the TreeFrogUI front-end).

The contents of the game's files are the author's, unchanged (`game/`; some file names are lower-cased to match
what the game asks for). Two programs are built here. The game ships as compiled Fenix 0.80 bytecode with no
script source, so the **Fenix 0.80** interpreter (`fenix080/`, GPL) is built for the SF3500 (`fxi.mipsel`). With it
comes a small **launcher** written for this port (`launcher.mipsel`): Launch Game, Stage Select, Settings and
Exit Game.

## The launcher

Dead Mobsters opens in a small launcher made for this port:

* **Launch Game** plays the game as the author made it, from the intro and title menu on. The title menu reads
  **PLAY GAME** and **EXIT**; EXIT goes back to the launcher.
* **Stage Select** unlocks once you have finished the whole game. It plays any one of the twelve stages (two
  stages and a boss in each of the four areas). Clearing the stage brings you back to it in the launcher. After
  Game Over you can continue as usual; Select, or letting the countdown run out, also goes back.
* **Settings** chooses your mobster, sets the music and sound effect volumes, and adjusts brightness, contrast and
  gamma. A picture test shows the effect: with good settings, all eight grey steps can be told apart.
* **Exit Game** returns to the front-end.

The volumes only turn the game's sounds down from how the author set them; use the handheld's volume buttons for
overall loudness. Settings are saved when you leave the Settings screen.

## Controls

In the launcher: D-pad to move and change values, A (or Start) to choose, B to go back, Select + Start to leave.

In the game:

| Button | Action |
|---|---|
| D-pad left and right | Walk |
| B, or D-pad up | Jump |
| A or Y | Shoot |
| R1 or X | Next weapon |
| L1 | Previous weapon |
| D-pad up and down | Move through the title menu |
| A or Start | Choose in the title menu; continue after Game Over |
| Start, while playing | Pause and resume |
| Start, during the intro | Skip it (after you have watched it once) |
| Select | Back to the title menu while playing (in Stage Select: back to the launcher) |
| Select + Start | Leave the game and return to the launcher |

A button such as A, B or Start moves past a story card. Holding fire when the game ends counts as "continue".

The game's own cheat keys are not mapped.

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

The original game has none of these, and its files are not changed for them. `shim/mobsters.c` holds what the
port knows about the game's bytecode: where the main program is waiting says whether the intro, the title menu or
a level is on screen. Those positions are checked when the game runs; if they do not match, these features stay
off.

* **Launcher** (`launcher/launcher.c`): a separate program on the same display, sound and pad layer. `run.sh`
  runs it, then the game as it asked (`launch.txt`), then the launcher again. Settings are in `settings.cfg`
  (`shim/dm_settings.c`, shared by both).
* **Title menu:** the entries in the loaded menu pictures are redrawn as PLAY GAME and EXIT, and the pad moves a
  cursor that sets the game's menu variable. That variable is where the game takes the character from.
* **Settings in the game:** brightness, contrast and gamma are applied to the 8-bit palette, so they cost nothing
  per frame (the gamma curves are tables made beforehand; nothing calls `pow` on the device). Each sound gets the
  music or effects volume when it is loaded.
* **Stage Select:** `SF3500_STAGE=<level>` sets the variables the game's own "continue" uses during the intro,
  hurries through it on a black screen, and leaves when the level is cleared or the title menu would appear.
  Reaching the ending story card in a normal game writes `game_completed`.
* **Pause.** Start during a level. The shim waits inside the input poll, holds the sound, and hides the paused
  time from the game's clock.
* **Intro skip.** The first time, the logo and title sequence play through. Reaching the title menu creates a
  file, `intro_seen`; from then on **PRESS START TO SKIP** is shown. The skip ends the intro's two timed waits
  early, and the game's own code carries on to the menu.

## Build

You need the SF3500 MIPS toolchain, cmake, make, and three upstream tarballs in `deps/` (checksums are verified):

| File | From |
|---|---|
| `SDL2-2.0.14.tar.gz` | https://github.com/libsdl-org/SDL/releases/download/release-2.0.14/SDL2-2.0.14.tar.gz |
| `SDL2_mixer-2.6.3.tar.gz` | https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.6.3/SDL2_mixer-2.6.3.tar.gz |
| `libxmp-lite-4.6.0.tar.gz` | https://github.com/libxmp/libxmp/releases/download/libxmp-4.6.0/libxmp-lite-4.6.0.tar.gz |

```sh
SF3500_TC=/path/to/toolchain/host ./build_sf3500.sh   # build-sf3500/fxi.mipsel and launcher.mipsel
./package.sh                                          # package/ for the card's roms/ports
```

The generated font data and gamma tables are kept in the repository; `tools/make-fonts.py` (needs Pillow and the
DejaVu fonts) and `tools/make-gamma.py` remake them. On the device the game runs as `fxi.mipsel -b mobsters.dcb`. The executable's name must start with `fxi`.

## Testing without the device

`SF3500_TC=... tools/qrun.sh <folder> -b mobsters.dcb` runs it under qemu-user; the folder holds `fxi.mipsel`
and the files from `game/`. Hooks (no cost unless set): `SF3500_KEYSCRIPT="1100:E:5,1300:J:6"`
(frame:letter[:frames]; the letters are listed at the top of `shim/fxsdl.c`),
`SF3500_SHOT_PREFIX=p SF3500_SHOT_FRAMES=100,200` (writes `p_100.ppm`...), `SF3500_QUIT_FRAME=n`,
`SF3500_PAUSE_TEST_POLLS=n` (a pause ends by itself), `SF3500_STAGE=<level>`. Without `intro_seen` the title menu
appears after about 1000 frames. `QRUN_BIN=launcher.mipsel tools/qrun.sh <folder>` runs the launcher, with the
same hooks (its letters are listed at the top of `launcher/launcher.c`).

`tools/dcbdis.py fenix080 game/mobsters.dcb` disassembles the game's bytecode. Developer notes:
`DEV-NOTES.md`.

## Licences

The game is the author's freeware; its readme (`game/readme.txt`) asks for it to be distributed to as many people
as possible. The Fenix interpreter and the shim are GPL version 2 or later; the glue in `sf3500/` is derived from
TreeFrogUI's SF3000 code; scripts and packaging are under the Unlicense. See `licenses/CREDITS.txt`.
