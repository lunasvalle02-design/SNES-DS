# The Legend of Zelda: A Link to the Past

**English** · [Español](README_ES.md)

| | |
|---|---|
| Module | `zelda_alttp` ([profile.c](profile.c)) |
| ROM | USA v1.0, LoROM, 1 MB, 8 KB SRAM |
| CRC32 | `777AAC2F` |
| SHA-1 | `6D4F10A8B10E10DBE624CB23CF03B88BB8252973` |
| Address source | [snesrev/zelda3](https://github.com/snesrev/zelda3) (`src/variables.h`, `hud.c`, `messaging.c`, `dungeon.c`) |

## Second screen

- **HUD:** BG3, lines 15-59. It stays on the game screen, in play, during text (`0E/02`) and through doors
  (`0F`/`10`). Until 0.5.0 it moved to the second screen in those modules and was drawn over the map, which looked
  wrong over the dungeon map; the profile has no `SPLIT_HUD` rule now.
- **Item menu** (`0E/01`): all of BG3 moves to the second screen, and the world stays visible on the game screen.
  The menu is a whole screen of its own that replaces the map while it's up (the game draws it in place of the HUD),
  not a strip over the map, so it stays.
- **Map:** the hidden core presses X.
  - Outdoors it presses X again for the whole world (`0E/07`).
  - In a dungeon the dungeon map comes up (`0E/03`). The map item isn't needed: it shows the rooms visited, and the
    item reveals the rest. About 30 black frames come first, after the fade.
  - In houses and caves X does nothing, and the last map is dimmed.

## Memory (WRAM, checked with the harness)

| Address | What it is | Values seen |
|---|---|---|
| `$7E0010` | Main module | `00` intro, `01` file select, `04` name entry, `05` loading, `07` indoors, `08`/`0F`/`10` transitions, `09` outdoors, `0E` menus, `17` save and quit, `1B` start menu |
| `$7E0011` | Submodule | With `0E`: `01` items, `02` text, `03` dungeon map, `07` world map. With `09`: `20-22` the map closing |
| `$7E001B` | Indoors | `01` in a house, `00` outside |
| `$7E0020` / `$7E0022` | Link's Y / X (16-bit) | |
| `$7E008A` | Overworld area | `2C` outside Link's house |
| `$7E00A0` | Dungeon room (16-bit) | `12` the Sanctuary |
| `$7E040C` | Dungeon ×2 | `FF` outside dungeons, `00` Hyrule Castle (the Sanctuary included) |
| `$7E1CD2` | The text box's corner (VRAM) | `$6125` at the top (BG3 row 9, line 72), `$6244` at the bottom (row 18) |
| `$7EF3C5` | Progress | `02` = Zelda rescued; lets a game start at the Sanctuary |
| `$7E0202` | Equipped Y item | Not used |
| `$7E0016` | Update the HUD at the next NMI | Not used yet |

In Mode 1, BG1 is the rain or the top layer, BG2 the terrain or the room, and BG3 the HUD, the text and the menu.

## Sound at power-on

At frame 82 after power-on, right after uploading its sound driver to ARAM, the game writes `$0A` to `$2143` (sound
effects): the driver plays sample 3 on voice 7 at a very high pitch (`$3D0A`), about 0.3 s that sound like a hiss
over the black screen. It isn't an emulation artefact: the sample is valid BRR, and the game's own code asks for it
(harness: `audio 200` prints the ports and the DSP). Nothing else plays before frame 110, so the profile mutes those
frames (`bootMuteFrames`).

## Tests

Your own ROM is needed; saves are not, the scripts make them.

```bash
scripts/check.sh ROMDIR zelda_alttp                              # all of them, in order (tests/order)
./build/harness ROM out games/zelda_alttp/tests/new_game.txt     # power-on to the map and the menu
./build/harness ROM out games/zelda_alttp/tests/sanctuary.txt    # a dungeon (uses the states the first leaves)
./build/harness ROM out games/zelda_alttp/tests/core_check.txt   # core hashes (reference: core_check.expected)
```

## To do

- Other regions (Europe, Japan): their CRC32s, and whether their addresses differ.
