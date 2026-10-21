
#ifndef PPU_H
#define PPU_H

#include <stdint.h>
#include <stdbool.h>

typedef struct Ppu Ppu;

#include "snes.h"
#include "statehandler.h"

typedef struct BgLayer {
  uint16_t hScroll;
  uint16_t vScroll;
  bool tilemapWider;
  bool tilemapHigher;
  uint16_t tilemapAdr;
  uint16_t tileAdr;
  bool bigTiles;
  bool mosaicEnabled;
} BgLayer;

typedef struct Layer {
  bool mainScreenEnabled;
  bool subScreenEnabled;
  bool mainScreenWindowed;
  bool subScreenWindowed;
} Layer;

typedef struct WindowLayer {
  bool window1enabled;
  bool window2enabled;
  bool window1inversed;
  bool window2inversed;
  uint8_t maskLogic;
} WindowLayer;

struct Ppu {
  Snes* snes;
  // snes-ds: vram, cgram and the pictures are allocated, so a line's copy
  // of the registers (for the render thread, see ppu_setAsync) shares them
  uint16_t* vram; // 0x8000 words
  uint32_t* outBuffer; // snes-ds: the main screen's picture, 0x00RRGGBB, 256 x 239 (line 1 first)
  uint32_t* splitBuffer; // snes-ds: see splitMask
  // vram access
  uint16_t vramPointer;
  bool vramIncrementOnHigh;
  uint16_t vramIncrement;
  uint8_t vramRemapMode;
  uint16_t vramReadBuffer;
  // cgram access
  uint16_t* cgram; // 0x100 words
  uint8_t cgramPointer;
  bool cgramSecondWrite;
  uint8_t cgramBuffer;
  // oam access
  uint16_t oam[0x100];
  uint8_t highOam[0x20];
  uint8_t oamAdr;
  uint8_t oamAdrWritten;
  bool oamInHigh;
  bool oamInHighWritten;
  bool oamSecondWrite;
  uint8_t oamBuffer;
  // object/sprites
  bool objPriority;
  uint16_t objTileAdr1;
  uint16_t objTileAdr2;
  uint8_t objSize;
  uint8_t objPixelBuffer[256]; // line buffers
  uint8_t objPriorityBuffer[256];
  bool timeOver;
  bool rangeOver;
  bool objInterlace;
  // background layers
  BgLayer bgLayer[4];
  uint8_t scrollPrev;
  uint8_t scrollPrev2;
  uint8_t mosaicSize;
  uint8_t mosaicStartLine;
  // layers
  Layer layer[5];
  // mode 7
  int16_t m7matrix[8]; // a, b, c, d, x, y, h, v
  uint8_t m7prev;
  bool m7largeField;
  bool m7charFill;
  bool m7xFlip;
  bool m7yFlip;
  bool m7extBg;
  // mode 7 internal
  int32_t m7startX;
  int32_t m7startY;
  // windows
  WindowLayer windowLayer[6];
  uint8_t window1left;
  uint8_t window1right;
  uint8_t window2left;
  uint8_t window2right;
  // color math
  uint8_t clipMode;
  uint8_t preventMathMode;
  bool addSubscreen;
  bool subtractColor;
  bool halfColor;
  bool mathEnabled[6];
  uint8_t fixedColorR;
  uint8_t fixedColorG;
  uint8_t fixedColorB;
  // settings
  bool forcedBlank;
  uint8_t brightness;
  uint8_t mode;
  bool bg3priority;
  bool evenFrame;
  bool pseudoHires;
  bool overscan;
  bool frameOverscan; // if we are overscanning this frame (determined at 0,225)
  bool interlace;
  bool frameInterlace; // if we are interlacing this frame (determined at start vblank)
  bool directColor;
  // latching
  uint16_t hCount;
  uint16_t vCount;
  bool hCountSecond;
  bool vCountSecond;
  bool countersLatched;
  uint8_t ppu1openBus;
  uint8_t ppu2openBus;
  uint8_t pixelOutputFormat;
  bool skipRender; // snes-ds: lines aren't drawn (sprites are still evaluated: the game can see that)
  // snes-ds: per-line layers to leave out of the picture (bit 0-3 = BG1-4,
  // bit 4 = OBJ), main and sub screen; not part of save states
  uint8_t hideMask[240];
  // snes-ds: per-line layers drawn into splitBuffer (ARGB, alpha 0 where
  // none of them has a pixel) instead of the picture
  uint8_t splitMask[240];
  // snes-ds: everything above is what drawing a line needs (copied for the
  // render thread); everything below is that drawing's own work
  uint8_t snapEnd;
  uint8_t curHide; // layers getPixel leaves out for the pixel at hand
  // snes-ds: per-line work, filled by ppu_runLine before its pixels (the
  // registers can't change during it); not part of save states
  bool lineCached;                 // bgLine / m7Line hold this line's layers
  uint16_t bgLine[4][2][256];      // ppu_getPixelForBgLayer(x, priority) for this line
  uint8_t m7LinePixel[2][256];     // mode 7's raw pixel, per layer (mosaic differs)
  bool lineHas[5][4];              // layer (4: sprites) has a pixel at that priority on this line
  bool winAny[6];                  // the layer has a window at all
  bool winLine[6][256];            // ppu_getWindowState(layer, x)
  uint8_t brightLine[32];          // 5-bit colour -> 8-bit at this line's brightness
  struct PpuAsync* async;          // snes-ds: the render thread, when on
};

enum { ppu_pixelOutputFormatXBGR = 0, ppu_pixelOutputFormatBGRX = 1 };

Ppu* ppu_init(Snes* snes);
void ppu_free(Ppu* ppu);
void ppu_reset(Ppu* ppu);
void ppu_handleState(Ppu* ppu, StateHandler* sh);
bool ppu_checkOverscan(Ppu* ppu);
void ppu_handleVblank(Ppu* ppu);
void ppu_handleFrameStart(Ppu* ppu);
void ppu_runLine(Ppu* ppu, int line);
uint8_t ppu_read(Ppu* ppu, uint8_t adr);
void ppu_write(Ppu* ppu, uint8_t adr, uint8_t val);
void ppu_putPixels(Ppu* ppu, uint8_t* pixels);
void ppu_setPixelOutputFormat(Ppu* ppu, int pixelOutputFormat);
// snes-ds: draw the lines on a thread of their own (the emulation goes on
// meanwhile); ppu_sync waits until every line handed over is drawn (done
// before vram/cgram change, states, and by whoever reads outBuffer)
void ppu_setAsync(Ppu* ppu, bool on);
void ppu_sync(Ppu* ppu);
// snes-ds: the render thread's time drawing, and the time ppu_sync waited
// for it, in ms since the last call (0 without one)
void ppu_threadStats(Ppu* ppu, double* busyMs, double* waitMs);

#endif
