# Super Castlevania IV

**English** · [Español](README_ES.md)

| | |
|---|---|
| Module | `super_castlevania_iv` ([profile.c](profile.c)) |
| ROM | USA v1.0, LoROM, 1 MB, no SRAM (passwords) |
| CRC32 | `B64FFB12` |
| SHA-1 | `684C1DFAFF8E5999422C24D48054D96BB12DA2F4` |
| Address sources | Found with the harness (RAM dumps compared between screens, pokes watched on the HUD) |

## Second screen

- **HUD** (score, Simon's and the enemy's health, the subweapon box, block, hearts, lives, time): BG3 lines 0-31 and
  the sprites on them (the subweapon's icon in the box is a sprite). It leaves the game screen in the game
  (`$0032` = `04`) while the stage state `$0070` is `04` fading in, `05` play (paused too), `06` dying, `0A` hit or
  `0C` falling into a pit; the game screen's top 32 lines are left with the backdrop colour.
- **Map:** the game has none; the map capture is off.

## Memory (WRAM, checked with the harness)

| Address | What it is | Values seen |
|---|---|---|
| `$7E0032` | Main mode | `02` Konami logo, title and prologue, `01` play select, `03` fading out of it, `04` name entry and the game |
| `$7E0070` | Stage state | `00` name entry and the castle gate's scene, `01`-`03` loading (screen off), `04` fading in, `05` play, `06` dying, `0A` hit (knocked back, Simon blinking), `0C` falling into a pit |
| `$7E0066` | Paused | `01` paused, `00` not |
| `$7E007C` | Lives | `05` at the start: the HUD shows one fewer (`P=04`) |
| `$7E008E` | Subweapon | `00` none, `01` dagger, `02` axe, `03` holy water, `04` cross, `05` stopwatch (poked, seen in the box) |
| `$7E13F0` | Time (16-bit, BCD) | `$0449` at the start of 1-1, one less a second |
| `$7E13F2` | Hearts | `05` at the start |
| `$7E13F4` / `$7E13F6` | Simon's / the enemy's health | `10` (16, the whole bar) |

In play, BG1 is the stage's front and BG2 its back from line 32 down; BG3 holds only the HUD, above line 32.

## Findings

- The HUD is drawn in every stage state but `03` (and the first frame or two of `04`): those load with the screen
  off and have no rule, so the second screen keeps the last HUD instead of going blank.
- Pausing doesn't change `$0070`: the HUD stays on the second screen while paused.
- The stage's sprites can be drawn over the HUD's lines (a poked position shows it), but none went there in play; one
  that did would show on the second screen with the HUD.
- Start pressed during the Konami logo goes straight to play select; one more Start on name entry goes back to the
  title; name entry left alone goes back to the title too. The tests wait for the title and press Start once per
  screen.
- With no health, Simon dies at the next hit, not at once.
- Power-on: the first sound is the Konami logo's (frame 534), nothing to mute.

## Tests

Your own ROM is needed; saves are not, the scripts make them.

```bash
scripts/check.sh ROMDIR super_castlevania_iv
```

| Script | Checks |
|---|---|
| `new_game.txt` | From power-on through play select, name entry and the gate's scene to Simon in block 1-1: the modes, hearts, health, lives, no subweapon, the HUD's lines |
| `hud.txt` | The HUD split in play, with a subweapon (its sprite in the box), paused, dying and reloading the block, falling into a pit |

There is no `map` check: the harness's `map` command refuses a game without a map.

## To do

- The states between blocks and stages (a block's exit, a boss, stage clear, the map of the route between stages,
  game over), which the tests don't reach yet: if one keeps the HUD drawn under a value not listed, the HUD stays on
  the game screen there.
- On the RG DS Plus: how the HUD strip looks on the second screen, and the empty strip at the top of the game screen.
- Other regions (Europe's *Super Castlevania IV*, Japan's *Akumajō Dracula*): their CRC32s and addresses.
