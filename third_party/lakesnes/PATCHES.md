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

## Save states

LakeSnes' states start with an id, `stateVersion` (in `snes_other.c`), their length and the cartridge's type, and a
state that doesn't match is refused. The snes-ds patches don't change what goes into a state (the masks, the
caches and the render thread are left out of it), so states keep loading across them. **Any change to what a state
holds must raise `stateVersion`**, so that older states are refused instead of loaded wrongly.
