
#ifndef GSU_H
#define GSU_H

// snes-ds: the Super FX (GSU-1/GSU-2), the coprocessor of Star Fox, Yoshi's
// Island and a few others. Written for snes-ds; only a cartridge of type 4
// (cart.h) has one, so nothing changes for any other game.

#include <stdint.h>
#include <stdbool.h>

typedef struct Gsu Gsu;

#include "snes.h"
#include "statehandler.h"

typedef struct GsuPixelCache {
  uint16_t offset; // (y << 5) + (x >> 3)
  uint8_t bitpend; // pixels written, bit 7 the leftmost
  uint8_t data[8];
} GsuPixelCache;

struct Gsu {
  Snes* snes;
  uint8_t* rom;
  uint32_t romMask;
  uint8_t* ram;
  uint32_t ramMask;
  // registers
  uint16_t r[16];
  uint16_t sfr; // status flags ($3030)
  uint8_t pbr; // program bank ($3034)
  uint8_t rombr; // rom bank ($3036)
  uint8_t rambr; // ram bank ($303c)
  uint16_t cbr; // cache base ($303e)
  uint8_t scbr; // screen base ($3038)
  uint8_t scmr; // screen mode ($303a)
  uint8_t cfgr; // config ($3037)
  uint8_t clsr; // clock select ($3039)
  uint8_t bramr; // backup ram enable ($3033)
  uint8_t colr; // plot colour
  uint8_t por; // plot options
  // instruction state
  uint8_t sreg, dreg;
  uint8_t pipeline;
  uint16_t ramaddr; // last ram address, for sbk
  uint8_t romdr; // rom buffer
  bool r14Set, r15Set; // written by the current instruction
  uint8_t cache[512];
  bool cacheValid[32];
  GsuPixelCache pixelcache[2];
  int32_t budget; // master cycles it may still run
  int32_t pending; // master cycles the cpu spent that it hasn't run yet (0 between frames)
};

// it runs behind the cpu, in slices: when the cpu touches the cartridge (its
// registers, ram or rom), every GSU_SLICE master cycles (so its irq comes at
// most that late) and at the end of a frame
#define GSU_SLICE 256

Gsu* gsu_init(Snes* snes, uint8_t* rom, uint32_t romSize, uint8_t* ram, uint32_t ramSize);
void gsu_free(Gsu* gsu);
void gsu_reset(Gsu* gsu);
void gsu_handleState(Gsu* gsu, StateHandler* sh);
// runs for the master cycles the cpu spent so far
void gsu_catchUp(Gsu* gsu);
// the master cycles the cpu just spent
static inline void gsu_addCycles(Gsu* gsu, int cycles) {
  if(!(gsu->sfr & 0x0020)) return; // not running (sfr.g)
  gsu->pending += cycles;
  if(gsu->pending >= GSU_SLICE) gsu_catchUp(gsu);
}
// $3000-$34ff
uint8_t gsu_read(Gsu* gsu, uint16_t adr);
void gsu_write(Gsu* gsu, uint16_t adr, uint8_t val);
// the cpu's view of the rom / ram: while the gsu has them, it doesn't get them
bool gsu_ownsRom(Gsu* gsu);
bool gsu_ownsRam(Gsu* gsu);

#endif
