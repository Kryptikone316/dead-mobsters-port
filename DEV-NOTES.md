# Dead Mobsters (SF3500) - dev notes

* Fenix 0.80 bytecode (DCB version 0). There is no game source, so the interpreter is Fenix 0.80 itself
  (`fenix080/`); the only change to its sources is `png_jmpbuf()` in the PNG loader.
* `game/` holds the author's files with lower-case names: the bytecode asks for `01.fpg`, `b1.fpg`, `font1.fnt`;
  the download has some in upper case. The Windows interpreter, DLLs and batch file are not included.
* `tools/dcbdis.py fenix080 game/mobsters.dcb` disassembles the game. What it shows:
  - Fire is `key(_alt)`; jump is `key(_space)` or `key(_control)`. The readme's "Ctrl shoots" is wrong.
    Confirmed in play (same start, one key held): Space jumps, Ctrl jumps and fires nothing, Alt fires.
  - Keys 1-4 set global 1644 (current weapon: pistol, machine gun, grenades, rockets). Initial value 1.
    The shim reads it to turn L1 / R1 / X into a number key.
  - Title menu: global 1604 = 1 START 1, 2 START 2, 3 EXIT. Esc is only tested after Enter there, so Esc alone
    does not leave from the title. Up / Down move once per frame while held.
  - In play: Esc returns to the title; Up is unused, so the shim also jumps with it. L = next level (cheat),
    K = ammo, M+O+B = infinite lives. Global 1684 = level number. Game Over offers Enter = continue.
  - `set_fps(22, 0)`: 22 fps is full speed.
  - Every sound is loaded at startup and none is unloaded.
* **Mixer channels must be 8 or fewer.** fxi's `sound.c` keeps `lista_canales[8]`, indexed by the channel
  SDL_mixer picks. The game can overlap at most 9 different sounds (1 music + 8 effects), because fxi will not
  start a sound that is already playing on a tracked channel.
* **Pause and intro skip** (`shim/mobsters.c` knows the game, `shim/fxsdl.c` does the rest). Where the game is
  = the main process's code position: 166 / 267 are the intro's two FRAME waits (16000, 84000), 320-425 the title
  menu, 5278 the level loop. The three waits are verified at run time; if they differ, both features are off.
  - Skip: set the main process's FRAME_PERCENT to 100 during an intro wait. The game's own code then removes the
    intro processes and opens the menu. Nothing else is written.
  - `intro_seen` (in the game folder) is created when the menu is first reached; the skip needs it.
  - Pause: Start's rising edge at 5278 (and level not finished, health >= 0) loops inside the input poll. The
    time spent is subtracted from `SDL_GetTicks`, so frame pacing and the game's timers see none of it.
  - The Start press used for either is hidden from the game until released, or it would act as Enter.
* Tests (qemu, `tools/qrun.sh`; `SF3500_KEYSCRIPT` letters are listed in `shim/fxsdl.c`): all levels, bosses and
  the ending reached with the W cheat; weapon cycle, fire, jump (B and D-pad up), Game Over, Esc to the title,
  the EXIT entry; intro skip (first run ignores Start, later runs skip from the logo and from mid-title); pause
  in a level and in a boss fight. Flat 22.2 fps.
* Device (SF3500): runs at 22.2 fps; intro skip and Select + Start while paused seen in the log. On one early
  run B did not jump; on the next, with the B code unchanged, it did (the pad word read bit 14 as expected).
  Cause not known.
* The build links libxmp-lite through SDL2_mixer although the game has only WAV sounds.
