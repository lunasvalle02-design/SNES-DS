# <Full title>

**English** · [Español](README_ES.md)

| | |
|---|---|
| Module | `<id>` ([profile.c](profile.c)) |
| ROM | <Region> v<x.y>, <LoROM/HiROM>, <size>, <SRAM size> |
| CRC32 | `<XXXXXXXX>` |
| SHA-1 | `<…>` |
| Address sources | <Disassemblies, wikis, data-crystal pages… or "found with the harness"> |

## Second screen

- **<HUD / menu / map>:** <which layer, which lines, when (the memory flags), what stays on the game screen>.

## Memory (WRAM, checked with the harness)

| Address | What it is | Values seen |
|---|---|---|
| `$7E….` | <Main mode, room, …> | <`00` title, `01` play…> |

<Which layer holds what, in each mode.>

## Findings

<Anything surprising: mid-frame register changes, layers shared between the HUD and the world, timing, sound.>

## Tests

Your own ROM is needed; saves are not, the scripts make them.

```bash
scripts/check.sh ROMDIR <id>
```

| Script | Checks |
|---|---|
| `<boot>.txt` | <From power-on to …> |

## To do

- <What's left, what needs the device to check.>
