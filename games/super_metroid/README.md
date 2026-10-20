# Super Metroid

**English** · [Español](README_ES.md)

| | |
|---|---|
| Module | `super_metroid` ([profile.c](profile.c)) |
| ROM | Japan and USA (one ROM, English and Japanese text) v1.0, LoROM, 3 MB, 8 KB SRAM |
| CRC32 | `D63ED5F8` |
| SHA-1 | `DA957F0D63D14CB441D215462904C4FA8519C613` |
| Address sources | The community RAM maps (game state `$0998`, room `$079B`, area `$079F`), each checked with the harness |

## Second screen

- **HUD** (energy, missiles, super missiles, power bombs, grapple, x-ray, the minimap): BG3, lines 0-30. It leaves
  the game screen in play and through doors (`$0998` = `08`-`0B`) and on the pause screen (`0C`, `0F`, `10`, `12`),
  and is drawn over the map. The game screen's top 32 lines are left black.
- **Map:** the hidden core presses Start; the pause screen opens on the map of the area Samus is in, centred on her
  (`$0998` = `0F`, `$0727` = `00`), about 70 frames later after a fade. Captured again when the room or the area
  changes, and every 3 seconds (the map fills in as Samus explores a room).
  - Where Samus can't pause (Ceres Station, getting out of the ship, elevators, cutscenes) the last map is dimmed.

## Memory (WRAM, checked with the harness)

| Address | What it is | Values seen |
|---|---|---|
| `$7E0998` | Game state | `01` title, `02` options, `04` file select, `06`/`07` loading, `08` play, `09` touching a door, `0B` through it, `0C` darkening to pause, `0D`/`0E` loading the pause screen, `0F` pause screen, `10` darkening it, `11` loading the room, `12` brightening to play, `1E` intro, `1F` new game, `20`-`22` Ceres escaped → Zebes |
| `$7E0727` | Pause screen page | `00` map, `01` equipment, `02`-`04` between them |
| `$7E079B` | Room (16-bit, its header in bank `$8F`) | `$DF45` Ceres's first room, `$91F8` Crateria's landing site, `$92FD` the room left of it |
| `$7E079F` | Area | `00` Crateria, `06` Ceres Station |
| `$7E09C2` / `$7E09C4` | Energy / maximum (16-bit) | `99` / `99` at the start |

In play, Mode 1: BG1 is the room's foreground, BG2 its background, BG3 the HUD above line 31 and the room's fog,
water or lava below it (an IRQ at line 31 changes the setup). On the pause screen BG1 is the map's rooms, BG2 the grid
and the frame, BG3 the HUD again (lines 0-31), the sprites Samus's marker and the L/R buttons.

## Findings

- The intro waits on each page of text until a button is pressed (A works, Start doesn't); left alone it never
  reaches Ceres. The tests tap A through it.
- No pausing on Ceres Station: Start does nothing there. The tests skip Ceres's escape by setting the game state to
  `20` (Samus reached the elevator): the game runs the explosion and the flight, lands the ship on Zebes and goes on
  as it would after the escape (`$079F` = `00`, room `$91F8`).
- Samus can't pause for about 10 seconds after landing, while she gets out of the ship.
- The HUD isn't drawn on a few frames around the pause screen (`0D`/`0E`, `11` and the first frame of `12` load with
  the screen off); those states have no rule, so the second screen keeps the last HUD instead of blinking.
- In the pause screen, a 4-frame press of R or Start is sometimes missed; 12 frames always work.
- Power-on: nothing plays until the intro starts (frame 369), so nothing is muted.
- A capture takes about 170 frames of the hidden core: 70 to open, the fade in, 8 frames of the blinking marker
  8 frames apart.

## Tests

Your own ROM is needed; saves are not, the scripts make them.

```bash
scripts/check.sh ROMDIR super_metroid
```

| Script | Checks |
|---|---|
| `new_game.txt` | From power-on through a new file and the intro to Ceres Station: the game state, the area, energy, no map there, the HUD's lines |
| `zebes.txt` | Ceres skipped, the ship lands on Zebes: no map while it lands, then the pause map; the HUD through a door; the map in the next room; the pause screen's pages and back to play |

## To do

- On the RG DS Plus: whether a capture every 3 seconds keeps the game at full speed, and how the HUD over the top of
  the pause map looks (it covers the map's own copy of it).
- Other regions (Europe): its CRC32, and whether its addresses differ.
- Maybe the equipment page on the second screen while paused.
