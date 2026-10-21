# Secret of Mana

**English** · [Español](README_ES.md)

| | |
|---|---|
| Module | `secret_of_mana` ([profile.c](profile.c)) |
| ROM | USA, HiROM, 2 MB, 8 KB battery save |
| CRC32 | `D0176B24` |
| SHA-1 | `8133041A363E3CC68CEDEF40B49B6D20D03C505D` |
| Address sources | Found with the harness (RAM dumps compared between play, the ring menu, its screens, text boxes, the title and the intro; each byte then watched through the changes). Data Crystal's RAM map was down while this was written |

## Second screen

- **HUD** (each character's HP and weapon gauge): BG3 lines 199-216 and the face beside it, a sprite (the boy's at
  x 88-102, lines 199-214). It goes down in the field (`$009B` = `02`) while the ring menu is closed (`$007D` = `00`),
  text boxes included: they are drawn at the top (lines 9-68). The field goes on under the HUD, so the game screen
  keeps all its lines. Before the boy has the sword the game draws no HUD, and the strip goes down empty.
- **Ring menu** (Y): while it's up (`$007D` = `01`, `$00CC` = `00`), BG3 (the item's name at the top, lines 9-36, and
  the HUD) and all the sprites (the ring, the characters) go to the second screen; the dimmed field stays on the game
  screen.
- **The screens the ring opens** (Status, Action Grid, Controller Edit, Window Edit...; `$007D` = `01`, `$00CC` =
  `08`): they take the whole picture (a pattern on BG1, the windows on BG2, the characters as sprites), so all of it
  goes down and the game screen is left dark until the menu closes.
- **Map:** none yet. The game's only map is the Mode 7 world map seen flying on Flammie, late in the game, which the
  tests can't reach without a saved game: `.map` stays off.

## Memory (WRAM, checked with the harness)

| Address | What it is | Values seen |
|---|---|---|
| `$7E009B` | Kind of screen | `00` the logo, the title, Game Select, the name entry (and some 6 frames between two maps), `01` the intro's captions, `02` the field: play, its text boxes, the ring menu and its screens |
| `$7E007D` | Ring menu up | `01` from the frame the ring is drawn until it's closed, through the screens it opens; `00` otherwise |
| `$7E00CC` | Menu state | `00` play and the ring, `04` the frame the ring opens, `08` a screen the ring opened (also some text boxes, with `$007D` = `00`), `0C` while that screen closes (dark), then `00` a frame after `$007D` |
| `$7E005A` | Ring menu (another sign) | `01` play, `11` with the ring up; not used |

In play, BG1 and BG2 are the field (ground and water, trees), BG3 holds the HUD at the bottom and the text boxes at the
top, and the sprites are the characters, the enemies and the HUD's faces. The intro's captions are BG3 too.

## Findings

- **Text drawn in hi-res is hard to read**, on the game screen and on the second: the name entry, Game Select and the
  screens the ring opens draw their letters in the SNES's 512-pixel mode, and the core keeps only every other column
  of it (see `third_party/lakesnes/ppu.c`, "only the main screen's pixel"). That is an engine matter, the same for any
  game that uses that mode; the dialogue boxes and the ring's names use normal letters and read well.
- The boy never walked below line 176 (into the bottom of three maps): sprites stay clear of the HUD's lines, so only
  its faces go down with it. An enemy that wandered that low would go too.
- The buttons: B is attack and "confirm", Y the ring menu, Up and Down change rings in it; X showed nothing in the
  first maps.
- Power-on: the first sound is the intro's music with the Squaresoft logo (frame 122), nothing to mute.
- The ring closes from one of its screens straight to play with Y: `$00CC` goes `0C` while the screen fades, `$007D`
  goes `00`, and `$00CC` follows a frame later.

## Tests

Your own ROM is needed; saves are not, the scripts make them.

```bash
scripts/check.sh ROMDIR secret_of_mana
```

| Script | Checks |
|---|---|
| `new_game.txt` | From power-on through the title, a new game, the name, the intro and the fall into the river to the sword in the stone (walked with fixed presses): `$009B` on the way, the ring closed, the HUD's lines and its face |
| `menus.txt` | The ring menu (`$007D`, `$00CC`, its layers), the Status screen it opens (`$00CC` = `08`, its windows on BG2), closing it all, and a text box at the top, clear of the HUD |

There is no `map` check: the harness's `map` command refuses a game without a map.

## To do

- The world map (Mode 7, flying on Flammie), with a test that pokes its way there, and the menus seen only later:
  shops, the magic rings, the screens for the girl and the sprite.
- The HUD with three characters (wider across the same lines, as far as one character shows) and the HUD's colour
  changes (poison, low HP).
- The hi-res text, as an engine change (blending each pair of columns instead of keeping one).
- On the RG DS Plus: how the HUD strip and the ring look on the second screen.
- Other regions (Europe, Japan's *Seiken Densetsu 2*): their CRC32s and addresses.
