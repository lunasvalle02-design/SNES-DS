# LakeSnes in snes-ds

Upstream: https://github.com/angelo-wf/LakeSnes (MIT, `LICENSE.txt`), the `snes/` directory, taken 2026-10-20.

Changes (each marked `snes-ds` in the code):

- `ppu.h`: `hideMask[240]` (layers left out of the picture, per line), `splitMask[240]` (layers drawn into
  `splitBuffer` instead of the picture, per line), `curHide`, `splitBuffer[256*240]`. None of them are part of
  save states.
- `ppu.c` `ppu_handlePixel`: when a line has a split mask, first renders that pixel with only the split layers
  (main screen, no colour math, brightness applied; alpha 0 on the backdrop) into `splitBuffer`, then the normal
  pixel without them.
- `ppu.c` `ppu_getPixel`: a layer whose bit is set in `curHide` counts as disabled.
