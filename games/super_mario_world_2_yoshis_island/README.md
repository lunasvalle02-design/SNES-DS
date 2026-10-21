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
  (`$0B10` = `01`, `$0B0F` = `10`-`13`, ~36 frames). The `PAUSE` title is left out (sprites on lines 0-49): on the
  second screen the game isn't paused. It's taken again as soon as the coins, red coins, flowers or lives change, and
  every 5 seconds for the stars.
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
byte: `$7E03B6` holds them in tenths (`6D` = 109 with 10 stars, `19` = 25 with 2; the screen drops the tenths). It
changes every few frames while the countdown runs or climbs back, so as a trigger it would keep the hidden core
capturing without a break: the stars go by the periodic refresh, every 5 seconds (the countdown itself is on the game
screen).

## Findings

- In a level, BG1 and BG2 are the level, BG3 the message boxes (top of the screen) and part of the level (the plants
  at the bottom of 1-1), the sprites Yoshi, baby Mario, enemies and the star countdown.
- The pause screen is drawn on its own (BG1 and sprites over black, the game hidden). After START the game fades out
  and the screen stays black (`$0B0F` `01`-`0A`), the panel fades in (`0B`-`0D`, frames 21-29: every pixel of it
  changes each frame) and holds still from `0E` on, while only the title drops in. The capture waits for `10`-`13`
  and settles in ~36 frames: ~35 ms on a PC, ~300 ms on the console. Starting it earlier would mean drawing the
  black frames on the hidden core (it only draws once the pause screen is up), which costs more than it saves.
- Speed: on the console (1.1.1 with the first Super FX), 10.5-12 ms per frame on average, with peaks of 20-37 ms that
  showed as slowdowns in 1-1. On PC the cost per frame is steady (the dearest of 3,000 frames of play: 1.3x the
  average), so the console's peaks are other threads getting in the way (the render thread, the hidden core's
  captures, audio): what helps is a lower average and fewer captures. The Super FX now runs behind the CPU, in
  slices (see PATCHES.md): 1-1 went from ~595 to ~780 fps on PC with the render thread. And the captures went from
  one every 2 seconds plus each change to one every 5 seconds plus each change, 36 frames each instead of 39.
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

- **Speed on the console: some slowdowns left in 1-1, acceptable for now.** On the RG DS Plus (with the Super FX run
  in slices, the lighter captures and the battery save off the emulation thread) the log shows 10.3-11.5 ms of
  emulation per frame on average and peaks of 17-20 ms (they were 20-37 ms), no audio underruns. The render thread
  draws in ~6 ms and the emulation waits for it only 0.2-0.6 ms per frame: **the emulation thread itself (CPU,
  Super FX, sound) sets the pace**, and its peaks over 16.7 ms are the slowdowns left. Where to look next, in order:
  - **The CPU waiting for the Super FX.** While the GSU runs, the CPU reads `$3030` in a loop, ~1,400 times a frame
    in 1-1, each one emulated in full. Skipping ahead to when the GSU stops (or to the next event: NMI, IRQ, the next
    line the PPU needs) would save most of that work, but it has to keep interrupts on time: an `engine/` change, to
    be checked against the core's hashes and save states as #23 was.
  - **The Super FX interpreter** (~12% of the time on PC, ~25% of the emulation thread): its dispatch and flags, the
    pipeline fetch, `gsu_resetPrefix` on every instruction.
  - **The captures** of the pause screen, ~300 ms of the hidden core each, competing for the same 4 cores: about 4 per
    10 seconds while coins are being collected. Fewer (coins every few seconds instead of at once) or cheaper.
  - **More sound queued** (3 frames now, ~50 ms) would hide the peaks that are left, at the cost of latency, for every
    game: an app setting rather than a fix.
- The rest of the game, which the tests don't reach: the world map between levels (the second screen keeps the last
  capture there), bosses (Super FX scaling and rotation), the mini-games. If something draws wrong, it's the Super
  FX's emulation (an `engine/` matter) rather than the profile.
