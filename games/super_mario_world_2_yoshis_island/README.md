# Super Mario World 2: Yoshi's Island

**English** · [Español](README_ES.md)

| | |
|---|---|
| Module | `super_mario_world_2_yoshis_island` ([profile.c](profile.c)) |
| ROM | USA Rev 1 (v1.1), Super FX 2 (GSU-2), 2 MB, 32 KB of GSU RAM (battery-backed) |
| CRC32 | `CF98DDAA` |
| SHA-1 | `34612A93741F156D6E497462AB7F253CB8A959A0` |
| Address sources | Found with the harness (`watch`, RAM dumps compared between states, values poked and read back on the pause screen) |

It runs on the Super FX 2, which LakeSnes doesn't have: the core's Super FX is in
[`third_party/lakesnes/gsu.c`](../../third_party/lakesnes/gsu.c) (see
[PATCHES.md](../../third_party/lakesnes/PATCHES.md)). Without it the game stays on a black screen.

## Second screen

- **The pause screen, always up to date** (the level's stars, red coins and flowers with their points, the total and
  the high score, and the bar with lives, coins, the star count and the items). A level has no HUD of its own: the
  game only shows these on the pause screen, which takes the whole picture (`PAUSE` in sprites, the rest BG1, on
  black). So the hidden core pauses the game for the second screen (the profile's "map"): from normal play
  (`$0118` = `0F`, not paused, no message box) it presses START and takes the picture once the pause screen is all up
  (`$0B10` = `01`, `$0B0F` = `13`, ~39 frames). The `PAUSE` title is left out (sprites on lines 0-49): on the second
  screen the game isn't paused. It's taken again as soon as the coins, red coins, flowers or lives change, and every
  2 seconds for the stars.
- **No split.** The star countdown (a sprite at the top when baby Mario is knocked off) comes and goes over the game;
  the message boxes of hint blocks are BG3, which also holds part of the level, and a window blacks out the other
  layers under them, so they'd leave a black hole on the game screen. The real pause screen stays on the game
  screen, where its items are picked.

## Memory (WRAM, checked with the harness)

| Address | What it is | Values seen |
|---|---|---|
| `$7E0118` | Game mode | `01`-`07` the opening, `08`-`0A` the title and the file menu, `37`/`38` leaving the menu, `39` the story, `0E` a level's title card (waits for a button), `0F` a level, paused too; `12`, `15` baby Mario taken |
| `$7E0B10` | Paused | `01` from START until START closes the pause screen, `00` otherwise |
| `$7E0B0F` | The pause screen coming in | `01` to `13` (all up, ~35 frames after START), back down to `00` as it closes |
| `$7E0D0F` | A hint block's message box | `09`, `0B`, `0D` while it opens and turns pages, `00` without one |
| `$7E0379` | Lives | `03` on a new file (poked `07`: the pause screen shows 7) |
| `$7E037B` | Coins | counts up as they're collected in 1-1 (`01`, `02`…) |
| `$7E03B4` | Red coins in the level | poked `03`: the pause screen shows 3/20 |
| `$7E03B8` | Flowers in the level | poked `02`: the pause screen shows 2/5 |

The byte after each count stays `00` (the second screen reads them as 16-bit values). The star count isn't a plain
byte: `$7E03B6` is `6D` with 10 stars, and poking it showed other values, so the stars go by the periodic refresh.

## Findings

- In a level, BG1 and BG2 are the level, BG3 the message boxes (top of the screen) and part of the level (the plants
  at the bottom of 1-1), the sprites Yoshi, baby Mario, enemies and the star countdown.
- The pause screen is drawn on its own (BG1 and sprites over black, the game hidden), from the hidden core it takes
  about 39 frames to come up and settle: ~40 ms per capture on a PC.
- Speed (PC, this machine): ~800 fps in the opening, ~480 fps in 1-1: the Super FX works hard in play, about half the
  speed of a game without one. To be measured on the console.
- Power-on: the first sound is the "Nintendo Presents" jingle (frame 81, the logo on screen): nothing to mute.
- Save states hold the Super FX too: a state saved in 1-1 and loaded again plays on to the same picture and RAM, with
  the render thread on and off.

## Tests

Your own ROM is needed; saves are not, the scripts make them.

```bash
scripts/check.sh ROMDIR super_mario_world_2_yoshis_island
```

| Script | Checks |
|---|---|
| `new_game.txt` | From power-on through the opening, the title, a new file and the story to 1-1: the game mode on the way, no capture while a message box is up, then the counts at the start and the capture of the pause screen |
| `play.txt` | Coins collected in 1-1 and the capture with them; red coins, flowers and lives (poked) and the capture; the real pause: no capture while paused, again after it |

## To do

- Speed on the console: the Super FX is the heaviest thing the core has run so far.
- The rest of the game, which the tests don't reach: the world map between levels (the second screen keeps the last
  capture there), bosses (Super FX scaling and rotation), the mini-games. If something draws wrong, it's the Super
  FX's emulation (an `engine/` matter) rather than the profile.
- The star count's address, to capture as soon as it changes instead of every 2 seconds.
