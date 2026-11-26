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

## Launcher, title menu, settings, Stage Select

* Levels (global 1684): 1 opening card, 2 "The Suburbs" card, 3 4 stages, 5 "Oh No!", 6 boss; 7 card, 8 9, 10,
  11 boss; 12 card, 13 14, 15, 16 boss; 17 card, 18 19, 20, 21 boss; 22 ending card, 23 flying cutscene. Every
  level is built to start on its own (the game's continue starts any of them).
* Level complete = global 1844 = 1, however it is reached (goal: proc47; bosses: proc15; last boss: proc79).
  Stage mode sees it while the main process waits at 5278, before the loop moves on.
* "Continue" is globals 1656 = 1 and 1660 = level; 1656 = 1 skips the title menu. Stage mode sets both, and
  1604 (character), during the intro's first wait.
* Character: global 1608, set from the menu entry 1604; read only by the player's own processes, and it only
  picks sprite numbers. Both "starts" are one-player. The "1UP" tag marks 50 frames of spawn protection.
* Menu pictures: graphs 15 / 16 / 17 of deadmob.fpg (arrow on entry 1 / 2 / 3); text area x 133-186, rows
  158-195; white 1 px strokes 8 high with a dark outline.
* Sounds: names starting "low", and sewerlow.wav, are music (played with PLAY_WAV like any effect).
* Launcher: ignores the pad until everything has been released (Select + Start from the game is still down when
  it starts), and waits for release before it exits; the game likewise does not take a Start held at startup as
  a new press (it would skip the intro).
* Sprites 200-215 are 70-74 rows tall with the mobster in the top 37.
* Tested in qemu (2026-11-26): new menu, EXIT entry, character 2, picture settings; unlock at the ending; stage
  mode for a sewer stage and the last boss (cleared with the W cheat), Select, Game Over timeout and Continue;
  launcher locked / unlocked, settings saved, requests written; run.sh loop with stand-in programs.
* Device (SF3500, 2026-11-26): the launcher build tested by the port author and reported all working (the handover
  to the game and back, settings, Stage Select).

