# LakeSnes in snes-ds

Upstream: https://github.com/angelo-wf/LakeSnes (MIT, `LICENSE.txt`), the `snes/` directory, taken 2026-10-20.
Each change is marked `snes-ds` in the code.

None of the speed changes alter what the emulator does: `games/zelda_alttp/tests/core_check.txt` gives the same
picture, split-picture and WRAM hashes before and after them, with the render thread on and off, on x86-64 and
aarch64.

## Second screen

- `ppu.h`: `hideMask[240]` (layers left out of the picture, per line) and `splitMask[240]` (layers drawn into
  `splitBuffer` instead of the picture, per line). Neither is part of save states.
- `ppu.c`: on a line with a split mask, the split layers are drawn into `splitBuffer` (main screen, no colour math,
  brightness applied; alpha 0 on the backdrop), and the picture is drawn without them. A layer whose bit is set in
  `curHide` counts as disabled.
- `skipRender`: lines are not drawn, but sprites are still evaluated, because the game can read the time/range-over
  flags. Used by the hidden map core while the map is opening.

## Speed (about 3.7x on the emulation of A Link to the Past, with the render thread)

- **Lines, not pixels** (`ppu_prepareLine`, `ppu_composeLineFast`). Per line, before drawing:
  - each background layer is decoded once, tile by tile (one tilemap read and one row decode per 8 pixels; pixel by
    pixel only with mosaic or offset-per-tile), into `bgLine[layer][priority][x]`;
  - mode 7 into `m7LinePixel`, the windows into `winLine`, the brightness into `brightLine`, and which layer and
    priority pairs have any pixel at all into `lineHas`.

  Each screen (main, and sub only when colour math needs it) is then composed one layer at a time in priority order,
  with the same rule as `ppu_getPixel` (the first layer with a pixel wins), followed by the same colour math.
  Hi-res modes (5, 6) and forced blank keep the original per-pixel path.
- **The picture**: only the main screen's pixel is kept, straight into `outBuffer` (0x00RRGGBB, 256 x 239).
  `pixelBuffer` (512 x 480 x 2, ~1 MB) is gone; `ppu_putPixels` doubles `outBuffer` for anyone still calling it.
- **Timing** (`snes.c`): `snes_runCycles` covers the steps between events (H positions 0, 16, 512 and 1104, the
  H-IRQ position, the end of a line) in one go (`snes_quietRun`). After the IRQ setup changes (`irqDirty`: writes to
  $4200 and $4207-$420A, reset, state load), the next step runs in full. The APU catch-up counter still adds one step
  at a time, so its rounding is the same.
- **Render thread** (`ppu_setAsync`, `ppu_sync`):
  - `ppu_runLine` evaluates sprites (game-visible) and hands the line to a thread of its own, as a copy of everything
    before `snapEnd` in `Ppu` (the registers, the sprite line, the masks).
  - `vram`, `cgram`, `outBuffer` and `splitBuffer` are allocated and shared. Writes to VRAM ($2118/$2119) and CGRAM
    ($2122), reset and state handling first wait for every line handed over (`ppu_sync`), and so does whoever reads
    the picture (snes-ds's `core_run_frame`).
  - The thread spins briefly between lines and sleeps between frames.

## Super FX

LakeSnes has no enhancement chips. snes-ds adds the Super FX (GSU-1 and GSU-2: Star Fox, Yoshi's Island, Doom…),
written for snes-ds in `gsu.c` / `gsu.h`, modelled on how [ares](https://github.com/ares-emulator/ares) (ISC)
emulates it.
Only a cartridge of the new type 4 has one, so for every other game nothing runs differently: the core's hashes
(`games/zelda_alttp/tests/core_check.expected`) are unchanged.

- `snes_other.c`: a LoROM header with coprocessor 1 (the high nibble of `$FFD6`) and RAM is type 4, `SuperFX`. Its
  RAM is the size in the extended header (`$FFBD`, 32K on Yoshi's Island), else 64K.
- `cart.c`: type 4's map: the GSU's registers and cache at `00-3f,80-bf:3000-34ff`, ROM as LoROM at `8000-ffff` and
  linear in banks `40-5f`, RAM in `70-71` and its first 8K at `6000-7fff`. While the GSU has the ROM bus (`SFR.G` and
  `SCMR.RON`), the CPU reads the GSU's interrupt vectors (into WRAM at `$0100-$010C`); while it has the RAM bus, the
  CPU gets open bus.
- `snes.c`: `snes_runCycles` adds up the master cycles the CPU spends (`gsu_addCycles`), and the GSU catches up on
  them (`gsu_catchUp`), running its instructions for as long as they last (one master cycle per cached instruction at
  21 MHz, two at 10.7). It catches up before the CPU touches the cartridge (`cart.c`), every 256 cycles (so its IRQ
  comes at most that late) and at the end of each frame (`snes_runFrame`), so a state never owes it any. That's the
  same work as running it in step with the CPU, in about a quarter of the calls (2,800 per frame in Yoshi's Island's
  1-1 instead of 12,000): ~25% faster there on PC. Its IRQ (on `STOP`) shares
  the CPU's IRQ line with the H/V timer, so reading `$4211` or turning the timer off leaves the line as the cartridge
  holds it (`cart_irq`); reading `$3031` clears it.
- `gsu.c`: every instruction with its ALT1/ALT2/B prefixes, the one-byte pipeline (with the delay slot after jumps),
  the 512-byte instruction cache, the two-entry pixel cache of `PLOT` and `RPIX`, the four bit depths and screen
  heights of `SCMR` and `POR`. Simplified, as games don't need it: the ROM buffer and RAM writes take effect at once
  instead of some cycles later, the GSU doesn't stall when the CPU takes the ROM or RAM back while it runs, and
  `STOP` flushes the pixel cache.
- `src/core.c` (outside the core) reads the title of a type 4 cartridge from the LoROM header.

## Save states

LakeSnes' states start with an id, `stateVersion` (in `snes_other.c`), their length and the cartridge's type, and a
state that doesn't match is refused. The snes-ds patches don't change what goes into a state (the masks, the
caches and the render thread are left out of it), so states keep loading across them. **Any change to what a state
holds must raise `stateVersion`**, so that older states are refused instead of loaded wrongly.

The Super FX's registers, caches and timing are saved after the cartridge's RAM, only in a type 4 cartridge's states:
there were none of those before, and every other game's states hold the same as before, so `stateVersion` stays 2.
