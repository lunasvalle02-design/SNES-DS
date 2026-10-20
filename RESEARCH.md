# SNES DS: can a "gba-ds" be made for the SNES?

**English** · [Español](INVESTIGACION_ES.md)

> [!NOTE]
> This is the feasibility study written before development started. What was decided afterwards (LakeSnes as the
> core, GPL-3.0-or-later as the license) and how it went is in the [roadmap](ROADMAP.md).

**Short answer: yes.** The idea behind [kyannon/gba-ds](https://github.com/kyannon/gba-ds) carries over to the SNES
almost as it is, and in some respects the SNES makes it easier. One important decision comes before any code: the
**license of the emulation core** (see [§4](#4-the-key-decision-which-core-and-its-license)).

## Contents

1. [How gba-ds works](#1-how-gba-ds-works-what-to-carry-over)
2. [The reference ROM](#2-the-reference-rom)
3. [What the second screen can show in A Link to the Past](#3-what-the-second-screen-can-show-in-a-link-to-the-past)
4. [The key decision: which core, and its license](#4-the-key-decision-which-core-and-its-license)
5. [An alternative for Zelda only: the zelda3 decompilation](#5-an-alternative-for-zelda-only-the-zelda3-decompilation)
6. [Next games worth doing](#6-next-games-worth-doing)
7. [Proposed plan](#7-proposed-plan)

## 1. How gba-ds works (what to carry over)

From its README, its `CLAUDE.md`, `src/profiles.c`/`profiles.h` and the wiki page *Adding second-screen
compatibility for a game*:

| Piece | What it does | SNES counterpart |
|---|---|---|
| **libmgba, linked statically** | GBA emulation | A SNES core used as a library (§4) |
| **One window across both screens** (sway "fullscreen global" on ROCKNIX, `dist/screens.sh`; KMSDRM with a window per panel on BananaOS) | Shows both screens | The same; the approach carries over unchanged |
| **Map capture on a hidden second core** (`mapcap.c`) | Every ~1.5 s, and after every room change, copies the game's state into an invisible instance, presses the map button, waits for it to settle and records a few frames | The same: `retro_serialize`/`unserialize`, or `S9xFreezeGame` in memory |
| **Checks by RAM and pixels** (`mem`, `live`, `menu`, `room`, `probes`) | Only shows the map when a RAM byte confirms it's open | The same, reading the 128 KB WRAM ($7E0000-$7FFFFF) |
| **Split HUD** (`emu_hud_split`) | A second renderer receives the same VRAM/OAM writes and draws only some BG layers and sprites | Feasible (§3.2) |
| **One `MapProfile` per game**, found by the header's game code | Everything specific to each game | Identify by CRC32/SHA-1: SNES headers carry no reliable unique code |
| **Headless harness** (`peek`, `dumpram`, `layershot`, `ramdiff.py`) | Finding RAM addresses | Essential from the start |

A useful detail: **gba-ds already supports A Link to the Past, in its GBA version**. Its profile uses the *same mode
byte* as the SNES original (the comment says "as on the SNES"). On the GBA that byte is at `0x03002BDC`; on the SNES
it is at `$7E0010`.

## 2. The reference ROM

```
File:     Legend of Zelda, The - A Link to the Past (USA).sfc   1,048,576 bytes, no copier header
Header:   "THE LEGEND OF ZELDA" @ $7FC0, map $20 = LoROM, ROM $0A = 1 MB, SRAM $03 = 8 KB, USA, v1.0
Checksum: $50F2 / complement $AF0D (valid)
SHA-1:    6D4F10A8B10E10DBE624CB23CF03B88BB8252973
MD5:      608C22B8FF930C62DC2DE54BCD6EBA72
```

This is the clean USA dump, the same one the zelda3 decompilation asks for, so every address below matches it. It
has no enhancement chip (no SA-1, no Super FX), which makes it a cheap game to emulate.

### Key RAM addresses (WRAM, bank $7E)

Checked against `src/variables.h` and `src/hud.c` in [snesrev/zelda3](https://github.com/snesrev/zelda3):

| Address | Meaning | Use on screen 2 |
|---|---|---|
| `$7E0010` | Main module: `$07` dungeon/indoors, `$09` overworld, `$0E` interface | `live` / `mem` |
| `$7E0011` | Submodule. With module `$0E`: `$01` item menu, `$03` dungeon map, `$07` world map (as in the GBA profile) | Checking the map capture |
| `$7E001B` | Indoors (1) or outdoors (0) | Choosing which map to show |
| `$7E0020` / `$7E0022` | Link's Y/X coordinates (16-bit) | A marker of our own over the map |
| `$7E008A`, `$7E040A` | Overworld area | Detecting room changes (`room`) |
| `$7E00A0` | Dungeon room (16-bit) | Room changes, the dungeon map |
| `$7E040C` | Current dungeon ×2 | Which dungeon map applies |
| `$7E0016` | "Update the HUD at the next NMI" flag | Forcing the HUD to redraw after an item change |
| `$7E0202` | Equipped Y item | Touch inventory (§3.3) |
| `$7E00EA` | Copy of BG3VOFS (slides the item menu) | Detecting the menu |
| `$7EC700` | The HUD's tilemap buffer (copied to VRAM at NMI) | A direct source for the HUD |
| `$7EF340…` | Saved inventory (`$7EF340` bow, `$7EF359` sword…) | Inventory on screen 2 |
| `$7EF360` / `$7EF362` | Rupees (target / shown) | A HUD of our own |
| `$7EF36C` / `$7EF36D` / `$7EF36E` | Max health / health / magic | A HUD of our own |

On the SNES the map button is **X** (on the GBA it was L). The SNES world map is drawn in **Mode 7**, and capturing
it works the same way as any other picture.

## 3. What the second screen can show in A Link to the Past

### 3.1 A live map (as in gba-ds)

The proven route: a hidden instance presses X, the capture is checked with `$10 == $0E && $11 ∈ {$03, $07}`, and the
cropped capture is shown. Captures run periodically and whenever `$A0`/`$8A` change.

**A cheaper variant gba-ds doesn't use:** capture the world map once, then draw a marker of our own over it in real
time from `$20/$22` and `$040A`. Outside dungeons the hidden core then needn't run every 1.5 s, and the overworld map
updates at 60 fps instead of ~0.7 fps.

### 3.2 The HUD on the second screen

This is **easier on the SNES than on the GBA**. Zelda's HUD is **BG3** alone (2bpp tiles in Mode 1): zelda3's code
builds it at `$7EC700` and slides the menu with BG3VOFS.

- **Drawing the HUD on screen 2** needs no second picture unit: reading BG3's tilemap and CHR from VRAM, plus CGRAM,
  and painting a 2bpp layer is about 150 lines of C, with no dependency on any core's internals.
- **Removing the HUD from the main screen** does need a core that can turn layers off. Snes9x can (`S9xToggleBG`, or
  the "Show layer N" core options), and so can bsnes. **A hypothesis to check with `layershot`:** in modules
  `$07`/`$09`, BG3 is used only for the HUD. Should rain or fog layers also land on BG3, the split would have to go
  by scanline, as gba-ds's `hud.bgPrioBelow` does.
- Text boxes probably live on BG3 too. Worth checking: it would allow a "dialogue below" mode.

> Development confirmed the hypothesis: BG3 carries the HUD, the dialogue text and the whole item menu, and the
> split is done per scanline inside the patched picture unit.

### 3.3 Touch inventory (as samyost1's Android port does)

Tap an item on the bottom screen, write it to `$7E0202`, and set `$7E0016` to 1 so the game redraws the icon. Items
the player doesn't have (`$7EF340…`) must not be selectable, and the write should only happen when
`$10 ∈ {$07, $09}` and `$11 == 0`. It's a memory write, like the camera `poke` gba-ds already uses for Super Mario
Advance 2.

## 4. The key decision: which core, and its license

gba-ds is **GPL-3**. Reusing its code (`screens.sh`, `mapcap.c`, `ui.c`, the harness…) means the new project must be
GPL-3 too, and so the core must be GPL-compatible.

| Core | License | Compatible with reusing gba-ds? | Speed on ARM (A55) | Notes |
|---|---|---|---|---|
| **Snes9x** | Its own, non-commercial only | ❌ Not GPL-compatible | Excellent: two instances with room to spare | Technically the most practical, but the host code would have to be written from scratch, without gba-ds |
| **bsnes / bsnes-jg** (fast PPU) | GPL-3 | ✅ | Tight with one instance; doubtful with two | Very accurate, heavier |
| **ares** (SNES core derived from bsnes) | ISC | ✅ | Heavy | C++, many dependencies |
| **Mesen2** | GPL-3 | ✅ | Heavy | Excellent debugging tools, useful while researching on PC |
| **LakeSnes** | MIT | ✅ | Unknown, probably middling | Plain C, easy to modify (zelda3 bundles it for verification). Needs measuring |

**Recommendation:**

1. Measure first: run bsnes-jg (fast PPU) and LakeSnes with this ROM on the device, unthrottled. If either clears
   ~120 fps comfortably on one CPU core, the hidden core fits and the project can stay GPL-3, reusing gba-ds.
2. If neither does, use Snes9x and write the host from scratch, taking only ideas from gba-ds, not code. Another way
   is to ask kyannon for a license exception.

On the hardware: gba-ds's `CLAUDE.md` says the RG DS Plus has two 1024×768 panels (the bottom one touch-enabled) and
the RG DS two at 640×480. The Plus's chip is said to be an RK3568 (4× Cortex-A55), though not officially confirmed.
A SNES picture (256×224) fits at a **whole-number 3×** in 1024×768 (768×672) and at **2×** in 640×480 (512×448).

## 5. An alternative for Zelda only: the zelda3 decompilation

Were the goal **only** A Link to the Past, dual-screen ports already exist based on
[snesrev/zelda3](https://github.com/snesrev/zelda3), the game rewritten in C, which uses this same ROM to extract its
assets:

- [samyost1/zelda3-android](https://github.com/samyost1/zelda3-android) (AYN Thor): a live world map, a dungeon map
  with the rooms visited, and a touch inventory.
- A 3DS port and a Switch port with a dual-screen mode.

Porting one of them to ROCKNIX on the RG DS would be the fastest way to get Zelda, but it would serve no other game.
**The emulator is the approach that scales**: one profile per game, as in gba-ds.

## 6. Next games worth doing

The same metroidvania-minded choice as gba-ds:

- **Super Metroid**: the pause map and the HUD's minimap. The top candidate.
- A Link to the Past (this one), Secret of Mana and Chrono Trigger (menus, the Mode 7 world map), Super Mario World
  (the world map), Castlevania: Dracula X and Super Castlevania IV (the HUD), and Final Fantasy VI (menus, the
  world map).
- Every game's HUD follows the same recipe: find its layer with `layershot` and render it apart.

## 7. Proposed plan

1. **A harness on PC first** (like `tools/harness.c`): the core plus `wait`, `hold`, `shot`, `peek`, `dumpram`,
   `layershot`, `save`/`load`. With this ROM, check the addresses of §2 and the BG3 hypothesis of §3.2 live.
2. **Measure the core** on the device (§4) and settle the license.
3. **Zelda profile v0**: map capture on a hidden instance (the X button, `mem`/`live`/`menu`/`room`).
4. **The HUD on BG3**, with the option of removing it from the main screen.
5. **Touch inventory** (`$0202` + `$0016`).
6. A ROCKNIX package, following the `screens.sh` / `launch.sh` approach.

## Sources

- [gba-ds](https://github.com/kyannon/gba-ds), its wiki and its `CLAUDE.md`
- [snesrev/zelda3](https://github.com/snesrev/zelda3)
- [zelda3-android with a second screen](https://retrohandhelds.gg/zelda-3-now-has-a-second-screen-with-this-port/)
- [The dual-screen 3DS port](https://www.timeextension.com/news/2026/07/zelda-link-to-the-past-gets-a-dual-screen-port-on-3ds-with-help-from-ai)
- [RG DS Plus specifications](https://retrohandhelds.gg/anbernic-rg-ds-plus-officially-revealed-equipped-with-larger-screens-and-built-in-stylus-for-under-100/)
- [Anbernic: RG DS Plus first look](https://anbernic.com/blogs/news/anbernic-rg-ds-plus-first-look)
