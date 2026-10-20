# Super Mario World

**English** · [Español](README_ES.md)

| | |
|---|---|
| Module | `super_mario_world` ([profile.c](profile.c)) |
| ROM | USA v1.0, LoROM, 512 KB, 2 KB SRAM |
| CRC32 | `B19ED489` |
| SHA-1 | `6B47BB75D16514B6A476AA0C73A683A2A4C18765` |
| Address sources | The community's RAM maps (game mode `$0100`, the world map's state `$13D9`, Mario's position on it `$1F17`/`$1F19`, submap `$1F11`), each checked with the harness |

## Second screen

- **Status bar** (lives, bonus stars, the item box, time, coins, score): BG3, drawn on lines 9-36 of 0-36. It leaves
  the game screen in a level (`$0100` = `14`), paused too. The level's BG1, BG2 and sprites are drawn behind it, so the
  game screen keeps all 224 lines; the level's own BG3 below line 37 (message boxes, tides) stays on the game screen.
  Between levels (fading, loading, the world map) there is no rule, and the second screen keeps the last status bar.
  **The item in the item box stays on the game screen:** it's a sprite (x 120-135, lines 15-30), so the box goes down
  empty and the item floats at the top of the level, without its frame.
- **Map:** the world map itself. The game has no map to open from a level, so the hidden core captures the world map
  while Mario stands on it (`$0100` = `0E`, `$13D9` = `03`): nothing to open, and the picture settles in about 5
  frames. It's captured again when Mario moves to another tile or submap, and every 2 seconds while he stands still
  (a cleared level opens new paths). In a level the last capture stays: the second screen shows where Mario went in
  from, with the status bar over it.

## Memory (WRAM, checked with the harness)

| Address | What it is | Values seen |
|---|---|---|
| `$7E0100` | Game mode | `01` Nintendo Presents, `02`-`06` the title coming up, `07` title, `08` file select, `0A` players, `0B` fading out to the world map, `0C`/`0D` loading and fading it in, `0E` world map, `0F`/`10` fading out to a level, `11`-`13` loading it and fading in, `14` level |
| `$7E13D9` | What the world map is doing | `0C` and `00` as it comes up, `03` Mario standing, waiting for a button, `04` walking a path, `05` just stopped on a tile (`03` the next frame) |
| `$7E1F17` / `$7E1F19` | Mario's X / Y on the world map (16-bit) | `$0068`, `$0078` at Yoshi's House; `$0038`, `$0088` on Yoshi's Island 1 |
| `$7E1F11` | Mario's submap | `01` Yoshi's Island |
| `$7E13BF` | The level, by its world-map number | `29` Yoshi's Island 1 |
| `$7E13D4` | Paused | `01` paused, `00` not |
| `$7E0DBE` | Lives | `04` at the start: the status bar shows one more (5) |
| `$7E0071` | Mario's animation state | `09` dying (poked: the level fades out to the world map, a life fewer) |

In a level BG1 and BG2 are the level, full height; BG3 is the status bar above line 37 and the level's own layer 3
under it (an IRQ changes its scroll there). On the world map BG1 and BG2 are the map, BG3 its frame and the line at
the top (Mario's lives and the tile's name), and Mario is a sprite.

## Findings

- The status bar's tiles don't reach line 37: `splitbox` on BG3 lines 0-36 gives lines 9-36 in every level tried
  (the intro, Yoshi's Island 1, paused), and the intro's message box (BG3, lines 40-110) stays on the game screen.
- The item in the item box is a sprite: with a mushroom poked in (`$0DC2` = `01`), `split 16 0 37` and `splitbox`
  give x 120-135, lines 15-30. It stays on the game screen: only BG3 goes down, because sprites over lines 0-36 are
  also Mario and the level's own when he jumps or flies, and a profile can only pick sprites by lines, not by place.
- The world map has its own top line (Mario's face, his lives and the tile's name, lines 17-37), which the status
  bar is drawn over on the second screen: the two overlap a little (the status bar's lower row and the item box over
  the tile's name). Both stay readable; a profile can't leave lines out of a capture yet.
- On the world map the second screen shows the same map as the game screen (a capture a moment old), with the last
  level's status bar over it.
- Power-on: the first sound is the Nintendo Presents chime (frame 88, with the logo on screen), nothing to mute.
- Pausing needs a moment before Start unpauses: the tests tap until `$13D4` changes.

## Tests

Your own ROM is needed; saves are not, the scripts make them.

```bash
scripts/check.sh ROMDIR super_mario_world
```

| Script | Checks |
|---|---|
| `new_game.txt` | From power-on through the title, a new file and the intro level (the status bar's lines, the message box left out of them, no capture in a level) to the world map at Yoshi's House: the mode, the map's state, the position, the capture |
| `yoshis_island_1.txt` | A new tile and its capture, into Yoshi's Island 1: the status bar in the level and paused, no capture there; Mario dies, back on the world map: a life fewer, the capture again |

## To do

- The Mode 7 boss rooms (Iggy, Morton, Bowser…), the castles' and ghost houses' intros, the bonus game, the
  switch palaces and the ending, which the tests don't reach yet: if one draws the status bar elsewhere, it shows.
- The item in the item box on the second screen, with its frame: it needs the engine to send sprites down by place
  (a rectangle, x 120-135 by lines 15-30), in an `engine/` branch first.
- On the RG DS Plus: how the status bar looks over the world map's top line, and the map's refresh.
- Two players: whether the capture follows Luigi on his turns (his position and submap aren't checked yet).
- Other regions (Europe, Japan's *Super Mario World: Super Mario Bros. 4*): their CRC32s and addresses.
