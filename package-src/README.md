# Dead Mobsters (SF3500 port)

*Dead Mobsters* is a freeware run-and-gun platformer by Outhouse Software, made with the Fenix game engine. Two
mobsters, back from the dead, clean up the streets: through the suburbs, the sewers, the scaffolding and the
factory, with a boss at the end of each.

The game files are the author's, with their contents unchanged. Only the Fenix interpreter that runs them is new:
the original Windows interpreter cannot run on this handheld, so this port builds the same Fenix 0.80 interpreter
for the SF3500's processor.

## Controls

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
| Select | Back to the title menu while playing |
| Select + Start | Leave the game at any time |

On the title menu, **START 1** and **START 2** pick which of the two mobsters you play, and **EXIT** leaves the game.
A button such as A, B or Start moves past a story card. Holding fire when the game ends counts as "continue".

The game's own cheat keys are not mapped.

## Added by this port

The original game has neither of these. Both are done by the port's own layer; the game's files are not changed.

* **Pause.** Press Start while playing a level. The game stops where it is, with sound held, until you press Start
  again. Select + Start still leaves the game.
* **Intro skip.** The first time, the logo and title sequence play through as the author made them. Once you reach
  the title menu, the port notes it in a small file, `intro_seen`. From then on **PRESS START TO SKIP** is shown
  during the intro. Delete `intro_seen` to make the intro unskippable again.

## Notes

* The original readme says Ctrl shoots. The game itself shoots with Alt and jumps with Space or Ctrl; the buttons
  above follow what the game does.
* The picture keeps the game's 4:3 shape with black bars at the sides. To fill the screen instead, delete the line
  `export SF3500_ASPECT=16:9` in `run.sh`.
* `log.txt` holds what the game printed on its last run. The message `File mobsters.prg not found` is normal: the
  compiled game mentions its source file, which the author did not publish.
* `readme.txt` is the author's own readme (Windows instructions; the controls differ here).
* The source, build instructions and licences: https://github.com/Kryptikone316/dead-mobsters-port
* A few file names were changed to lower case (for example `01.FPG` to `01.fpg`) to match what the game asks for.
