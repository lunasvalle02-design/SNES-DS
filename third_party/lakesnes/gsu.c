
// snes-ds: the Super FX (GSU-1/GSU-2). See gsu.h and PATCHES.md.
//
// It runs alongside the cpu: snes_runCycles hands it the master cycles the
// cpu just spent (gsu_run), and it runs instructions for as long as those
// last, one master cycle per cached instruction at 21 MHz (clsr 1), two at
// 10.7 MHz. Its rom buffer and ram writes take effect at once instead of
// some cycles later, and it doesn't wait when the cpu has the rom or ram
// back while it runs: games don't rely on either.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "gsu.h"
#include "snes.h"
#include "cpu.h"
#include "statehandler.h"

// sfr
#define SFR_Z 0x0002
#define SFR_CY 0x0004
#define SFR_S 0x0008
#define SFR_OV 0x0010
#define SFR_G 0x0020
#define SFR_R 0x0040
#define SFR_ALT1 0x0100
#define SFR_ALT2 0x0200
#define SFR_B 0x1000
#define SFR_IRQ 0x8000

// por (plot options, set by cmode)
#define POR_TRANSPARENT 0x01 // colour 0 is plotted too
#define POR_DITHER 0x02
#define POR_HIGHNIBBLE 0x04
#define POR_FREEZEHIGH 0x08
#define POR_OBJ 0x10

// scmr
#define SCMR_RAN 0x08
#define SCMR_RON 0x10

Gsu* gsu_init(Snes* snes, uint8_t* rom, uint32_t romSize, uint8_t* ram, uint32_t ramSize) {
  Gsu* gsu = malloc(sizeof(Gsu));
  memset(gsu, 0, sizeof(Gsu));
  gsu->snes = snes;
  gsu->rom = rom;
  gsu->romMask = romSize - 1;
  gsu->ram = ram;
  gsu->ramMask = ramSize - 1;
  gsu_reset(gsu);
  return gsu;
}

void gsu_free(Gsu* gsu) {
  free(gsu);
}

void gsu_reset(Gsu* gsu) {
  memset(gsu->r, 0, sizeof(gsu->r));
  gsu->sfr = 0;
  gsu->pbr = 0;
  gsu->rombr = 0;
  gsu->rambr = 0;
  gsu->cbr = 0;
  gsu->scbr = 0;
  gsu->scmr = 0;
  gsu->cfgr = 0;
  gsu->clsr = 0;
  gsu->bramr = 0;
  gsu->colr = 0;
  gsu->por = 0;
  gsu->sreg = 0;
  gsu->dreg = 0;
  gsu->pipeline = 0x01; // nop
  gsu->ramaddr = 0;
  gsu->romdr = 0;
  gsu->r14Set = false;
  gsu->r15Set = false;
  memset(gsu->cache, 0, sizeof(gsu->cache));
  memset(gsu->cacheValid, 0, sizeof(gsu->cacheValid));
  memset(gsu->pixelcache, 0, sizeof(gsu->pixelcache));
  gsu->budget = 0;
}

void gsu_handleState(Gsu* gsu, StateHandler* sh) {
  sh_handleWordArray(sh, gsu->r, 16);
  sh_handleWords(sh, &gsu->sfr, &gsu->cbr, &gsu->ramaddr, NULL);
  sh_handleBytes(sh,
    &gsu->pbr, &gsu->rombr, &gsu->rambr, &gsu->scbr, &gsu->scmr, &gsu->cfgr, &gsu->clsr, &gsu->bramr,
    &gsu->colr, &gsu->por, &gsu->sreg, &gsu->dreg, &gsu->pipeline, &gsu->romdr, NULL
  );
  sh_handleByteArray(sh, gsu->cache, 512);
  sh_handleByteArray(sh, (uint8_t*) gsu->cacheValid, sizeof(gsu->cacheValid));
  for(int i = 0; i < 2; i++) {
    sh_handleWords(sh, &gsu->pixelcache[i].offset, NULL);
    sh_handleBytes(sh, &gsu->pixelcache[i].bitpend, NULL);
    sh_handleByteArray(sh, gsu->pixelcache[i].data, 8);
  }
  sh_handleIntsS(sh, &gsu->budget, NULL);
}

bool gsu_ownsRom(Gsu* gsu) {
  return (gsu->sfr & SFR_G) && (gsu->scmr & SCMR_RON);
}

bool gsu_ownsRam(Gsu* gsu) {
  return (gsu->sfr & SFR_G) && (gsu->scmr & SCMR_RAN);
}

// timing

static inline void gsu_step(Gsu* gsu, int cycles) {
  gsu->budget -= cycles;
}

static inline int gsu_memCycles(Gsu* gsu) {
  return gsu->clsr ? 5 : 6;
}

// the gsu's bus: rom in banks 00-5f, ram in 70-71

static uint8_t gsu_busRead(Gsu* gsu, uint32_t adr) {
  if((adr & 0xc00000) == 0x000000) {
    // 00-3f:8000-ffff (and its mirror at 0000-7fff), as lorom
    return gsu->rom[(((adr & 0x3f0000) >> 1) | (adr & 0x7fff)) & gsu->romMask];
  }
  if((adr & 0xe00000) == 0x400000) {
    // 40-5f:0000-ffff
    return gsu->rom[adr & gsu->romMask];
  }
  return gsu->ram[adr & gsu->ramMask]; // 60-7f
}

static uint8_t gsu_readRam(Gsu* gsu, uint16_t adr) {
  gsu_step(gsu, gsu_memCycles(gsu));
  return gsu->ram[((gsu->rambr << 16) + adr) & gsu->ramMask];
}

static void gsu_writeRam(Gsu* gsu, uint16_t adr, uint8_t val) {
  gsu_step(gsu, gsu_memCycles(gsu));
  gsu->ram[((gsu->rambr << 16) + adr) & gsu->ramMask] = val;
}

static void gsu_updateRomBuffer(Gsu* gsu) {
  gsu->romdr = gsu_busRead(gsu, (gsu->rombr << 16) + gsu->r[14]);
  gsu->sfr &= ~SFR_R;
}

static uint8_t gsu_readRomBuffer(Gsu* gsu) {
  gsu_step(gsu, gsu_memCycles(gsu));
  return gsu->romdr;
}

// the instruction cache: 512 bytes from cbr, in 32 lines of 16

static void gsu_flushCache(Gsu* gsu) {
  memset(gsu->cacheValid, 0, sizeof(gsu->cacheValid));
}

static uint8_t gsu_readOpcode(Gsu* gsu, uint16_t adr) {
  uint16_t offset = adr - gsu->cbr;
  if(offset < 512) {
    if(!gsu->cacheValid[offset >> 4]) {
      uint16_t dp = offset & 0xfff0;
      uint32_t sp = (gsu->pbr << 16) + ((gsu->cbr + dp) & 0xfff0);
      for(int n = 0; n < 16; n++) {
        gsu_step(gsu, gsu_memCycles(gsu));
        gsu->cache[dp++] = gsu_busRead(gsu, sp++);
      }
      gsu->cacheValid[offset >> 4] = true;
    } else {
      gsu_step(gsu, gsu->clsr ? 1 : 2);
    }
    return gsu->cache[offset];
  }
  gsu_step(gsu, gsu_memCycles(gsu));
  return gsu_busRead(gsu, (gsu->pbr << 16) | adr);
}

// the opcode being run is the one fetched before it (the pipeline): r15
// points past it, at the byte being fetched now

static inline uint8_t gsu_peekpipe(Gsu* gsu) {
  uint8_t result = gsu->pipeline;
  gsu->pipeline = gsu_readOpcode(gsu, gsu->r[15]);
  return result;
}

static inline uint8_t gsu_pipe(Gsu* gsu) {
  uint8_t result = gsu->pipeline;
  gsu->pipeline = gsu_readOpcode(gsu, ++gsu->r[15]);
  return result;
}

// plotting: pixels go through a two-entry cache of 8-pixel rows

static uint32_t gsu_charAddress(Gsu* gsu, uint8_t x, uint8_t y, int* bpp) {
  uint32_t cn = 0; // character number
  int ht = ((gsu->scmr >> 2) & 1) | ((gsu->scmr >> 4) & 2); // screen height
  switch((gsu->por & POR_OBJ) ? 3 : ht) {
    case 0: cn = ((x & 0xf8) << 1) + ((y & 0xf8) >> 3); break; // 128 lines
    case 1: cn = ((x & 0xf8) << 1) + ((x & 0xf8) >> 1) + ((y & 0xf8) >> 3); break; // 160
    case 2: cn = ((x & 0xf8) << 1) + ((x & 0xf8) << 0) + ((y & 0xf8) >> 3); break; // 192
    case 3: cn = ((y & 0x80) << 2) + ((x & 0x80) << 1) + ((y & 0x78) << 1) + ((x & 0x78) >> 3); break; // obj
  }
  int md = gsu->scmr & 3;
  *bpp = 2 << (md - (md >> 1)); // 2, 4, 4, 8
  return (cn * (*bpp << 3)) + (gsu->scbr << 10) + ((y & 7) * 2);
}

static void gsu_flushPixelCache(Gsu* gsu, GsuPixelCache* cache) {
  if(cache->bitpend == 0) return;
  uint8_t x = cache->offset << 3;
  uint8_t y = cache->offset >> 5;
  int bpp;
  uint32_t adr = gsu_charAddress(gsu, x, y, &bpp);
  for(int n = 0; n < bpp; n++) {
    uint32_t byte = ((n >> 1) << 4) + (n & 1); // 0, 1, 16, 17, 32, 33, 48, 49
    uint8_t data = 0;
    for(int i = 0; i < 8; i++) data |= ((cache->data[i] >> n) & 1) << i;
    if(cache->bitpend != 0xff) {
      gsu_step(gsu, gsu_memCycles(gsu));
      data &= cache->bitpend;
      data |= gsu->ram[(adr + byte) & gsu->ramMask] & ~cache->bitpend;
    }
    gsu_step(gsu, gsu_memCycles(gsu));
    gsu->ram[(adr + byte) & gsu->ramMask] = data;
  }
  cache->bitpend = 0;
}

static uint8_t gsu_color(Gsu* gsu, uint8_t source) {
  if(gsu->por & POR_HIGHNIBBLE) return (gsu->colr & 0xf0) | (source >> 4);
  if(gsu->por & POR_FREEZEHIGH) return (gsu->colr & 0xf0) | (source & 0x0f);
  return source;
}

static void gsu_plot(Gsu* gsu, uint8_t x, uint8_t y) {
  int md = gsu->scmr & 3;
  if(!(gsu->por & POR_TRANSPARENT)) {
    if(md == 3) {
      if(gsu->por & POR_FREEZEHIGH) {
        if((gsu->colr & 0x0f) == 0) return;
      } else {
        if(gsu->colr == 0) return;
      }
    } else {
      if((gsu->colr & 0x0f) == 0) return;
    }
  }
  uint8_t color = gsu->colr;
  if((gsu->por & POR_DITHER) && md != 3) {
    if((x ^ y) & 1) color >>= 4;
    color &= 0x0f;
  }
  uint16_t offset = (y << 5) + (x >> 3);
  if(gsu->pixelcache[0].offset != offset) {
    gsu_flushPixelCache(gsu, &gsu->pixelcache[1]);
    gsu->pixelcache[1] = gsu->pixelcache[0];
    gsu->pixelcache[0].bitpend = 0;
    gsu->pixelcache[0].offset = offset;
  }
  x = (x & 7) ^ 7;
  gsu->pixelcache[0].data[x] = color;
  gsu->pixelcache[0].bitpend |= 1 << x;
  if(gsu->pixelcache[0].bitpend == 0xff) {
    gsu_flushPixelCache(gsu, &gsu->pixelcache[1]);
    gsu->pixelcache[1] = gsu->pixelcache[0];
    gsu->pixelcache[0].bitpend = 0;
  }
}

static uint8_t gsu_rpix(Gsu* gsu, uint8_t x, uint8_t y) {
  gsu_flushPixelCache(gsu, &gsu->pixelcache[1]);
  gsu_flushPixelCache(gsu, &gsu->pixelcache[0]);
  int bpp;
  uint32_t adr = gsu_charAddress(gsu, x, y, &bpp);
  uint8_t data = 0;
  x = (x & 7) ^ 7;
  for(int n = 0; n < bpp; n++) {
    uint32_t byte = ((n >> 1) << 4) + (n & 1);
    gsu_step(gsu, gsu_memCycles(gsu));
    data |= ((gsu->ram[(adr + byte) & gsu->ramMask] >> x) & 1) << n;
  }
  return data;
}

// instructions

static inline void gsu_setReg(Gsu* gsu, int n, uint16_t val) {
  gsu->r[n] = val;
  if(n == 14) gsu->r14Set = true;
  if(n == 15) gsu->r15Set = true;
}

static inline uint16_t gsu_sr(Gsu* gsu) {
  return gsu->r[gsu->sreg];
}

static inline void gsu_setDr(Gsu* gsu, uint16_t val) {
  gsu_setReg(gsu, gsu->dreg, val);
}

static inline void gsu_setFlag(Gsu* gsu, uint16_t flag, bool set) {
  if(set) gsu->sfr |= flag;
  else gsu->sfr &= ~flag;
}

static inline void gsu_setSZ(Gsu* gsu, uint16_t val) {
  gsu_setFlag(gsu, SFR_S, val & 0x8000);
  gsu_setFlag(gsu, SFR_Z, val == 0);
}

// after an instruction (but not after a prefix or a branch)
static inline void gsu_resetPrefix(Gsu* gsu) {
  gsu->sfr &= ~(SFR_B | SFR_ALT1 | SFR_ALT2);
  gsu->sreg = 0;
  gsu->dreg = 0;
}

static void gsu_stop(Gsu* gsu) {
  if(!(gsu->cfgr & 0x80)) {
    // the irq isn't masked
    gsu->sfr |= SFR_IRQ;
    cpu_setIrq(gsu->snes->cpu, true);
  }
  // what's plotted is in ram for the cpu
  gsu_flushPixelCache(gsu, &gsu->pixelcache[1]);
  gsu_flushPixelCache(gsu, &gsu->pixelcache[0]);
  gsu->sfr &= ~SFR_G;
  gsu->pipeline = 0x01; // nop
  gsu_resetPrefix(gsu);
}

static void gsu_branch(Gsu* gsu, bool take) {
  int8_t displacement = (int8_t) gsu_pipe(gsu);
  if(take) gsu_setReg(gsu, 15, gsu->r[15] + displacement);
}

static void gsu_execute(Gsu* gsu, uint8_t op) {
  int n = op & 0xf;
  bool alt1 = gsu->sfr & SFR_ALT1;
  bool alt2 = gsu->sfr & SFR_ALT2;
  uint16_t sr = gsu_sr(gsu);
  switch(op) {
    case 0x00: { // stop
      gsu_stop(gsu);
      break;
    }
    case 0x01: { // nop
      gsu_resetPrefix(gsu);
      break;
    }
    case 0x02: { // cache
      if(gsu->cbr != (gsu->r[15] & 0xfff0)) {
        gsu->cbr = gsu->r[15] & 0xfff0;
        gsu_flushCache(gsu);
      }
      gsu_resetPrefix(gsu);
      break;
    }
    case 0x03: { // lsr
      gsu_setFlag(gsu, SFR_CY, sr & 1);
      uint16_t result = sr >> 1;
      gsu_setDr(gsu, result);
      gsu_setSZ(gsu, result);
      gsu_resetPrefix(gsu);
      break;
    }
    case 0x04: { // rol
      bool carry = sr & 0x8000;
      uint16_t result = (sr << 1) | ((gsu->sfr & SFR_CY) ? 1 : 0);
      gsu_setDr(gsu, result);
      gsu_setSZ(gsu, result);
      gsu_setFlag(gsu, SFR_CY, carry);
      gsu_resetPrefix(gsu);
      break;
    }
    case 0x05: gsu_branch(gsu, true); break; // bra
    case 0x06: gsu_branch(gsu, !(gsu->sfr & SFR_S) == !(gsu->sfr & SFR_OV)); break; // bge
    case 0x07: gsu_branch(gsu, !(gsu->sfr & SFR_S) != !(gsu->sfr & SFR_OV)); break; // blt
    case 0x08: gsu_branch(gsu, !(gsu->sfr & SFR_Z)); break; // bne
    case 0x09: gsu_branch(gsu, gsu->sfr & SFR_Z); break; // beq
    case 0x0a: gsu_branch(gsu, !(gsu->sfr & SFR_S)); break; // bpl
    case 0x0b: gsu_branch(gsu, gsu->sfr & SFR_S); break; // bmi
    case 0x0c: gsu_branch(gsu, !(gsu->sfr & SFR_CY)); break; // bcc
    case 0x0d: gsu_branch(gsu, gsu->sfr & SFR_CY); break; // bcs
    case 0x0e: gsu_branch(gsu, !(gsu->sfr & SFR_OV)); break; // bvc
    case 0x0f: gsu_branch(gsu, gsu->sfr & SFR_OV); break; // bvs
    case 0x10 ... 0x1f: { // to rn / move (after with)
      if(!(gsu->sfr & SFR_B)) {
        gsu->dreg = n;
        break;
      }
      gsu_setReg(gsu, n, sr);
      gsu_resetPrefix(gsu);
      break;
    }
    case 0x20 ... 0x2f: { // with rn
      gsu->sreg = n;
      gsu->dreg = n;
      gsu->sfr |= SFR_B;
      break;
    }
    case 0x30 ... 0x3b: { // stw (rn) / stb (rn)
      gsu->ramaddr = gsu->r[n];
      gsu_writeRam(gsu, gsu->ramaddr, sr);
      if(!alt1) gsu_writeRam(gsu, gsu->ramaddr ^ 1, sr >> 8);
      gsu_resetPrefix(gsu);
      break;
    }
    case 0x3c: { // loop
      gsu->r[12]--;
      gsu_setSZ(gsu, gsu->r[12]);
      if(gsu->r[12] != 0) gsu_setReg(gsu, 15, gsu->r[13]);
      gsu_resetPrefix(gsu);
      break;
    }
    case 0x3d: { // alt1
      gsu->sfr &= ~SFR_B;
      gsu->sfr |= SFR_ALT1;
      break;
    }
    case 0x3e: { // alt2
      gsu->sfr &= ~SFR_B;
      gsu->sfr |= SFR_ALT2;
      break;
    }
    case 0x3f: { // alt3
      gsu->sfr &= ~SFR_B;
      gsu->sfr |= SFR_ALT1 | SFR_ALT2;
      break;
    }
    case 0x40 ... 0x4b: { // ldw (rn) / ldb (rn)
      gsu->ramaddr = gsu->r[n];
      uint16_t data = gsu_readRam(gsu, gsu->ramaddr);
      if(!alt1) data |= gsu_readRam(gsu, gsu->ramaddr ^ 1) << 8;
      gsu_setDr(gsu, data);
      gsu_resetPrefix(gsu);
      break;
    }
    case 0x4c: { // plot / rpix
      if(!alt1) {
        gsu_plot(gsu, gsu->r[1], gsu->r[2]);
        gsu->r[1]++;
      } else {
        uint16_t result = gsu_rpix(gsu, gsu->r[1], gsu->r[2]);
        gsu_setDr(gsu, result);
        gsu_setSZ(gsu, result);
      }
      gsu_resetPrefix(gsu);
      break;
    }
    case 0x4d: { // swap
      uint16_t result = (sr >> 8) | (sr << 8);
      gsu_setDr(gsu, result);
      gsu_setSZ(gsu, result);
      gsu_resetPrefix(gsu);
      break;
    }
    case 0x4e: { // color / cmode
      if(!alt1) gsu->colr = gsu_color(gsu, sr);
      else gsu->por = sr & 0x1f;
      gsu_resetPrefix(gsu);
      break;
    }
    case 0x4f: { // not
      uint16_t result = ~sr;
      gsu_setDr(gsu, result);
      gsu_setSZ(gsu, result);
      gsu_resetPrefix(gsu);
      break;
    }
    case 0x50 ... 0x5f: { // add rn / adc rn / add #n / adc #n
      uint16_t operand = alt2 ? n : gsu->r[n];
      int result = sr + operand + ((alt1 && (gsu->sfr & SFR_CY)) ? 1 : 0);
      gsu_setFlag(gsu, SFR_OV, ~(sr ^ operand) & (operand ^ result) & 0x8000);
      gsu_setFlag(gsu, SFR_S, result & 0x8000);
      gsu_setFlag(gsu, SFR_CY, result >= 0x10000);
      gsu_setFlag(gsu, SFR_Z, (uint16_t) result == 0);
      gsu_setDr(gsu, result);
      gsu_resetPrefix(gsu);
      break;
    }
    case 0x60 ... 0x6f: { // sub rn / sbc rn / sub #n / cmp rn
      uint16_t operand = (alt2 && !alt1) ? n : gsu->r[n];
      int result = sr - operand - ((alt1 && !alt2 && !(gsu->sfr & SFR_CY)) ? 1 : 0);
      gsu_setFlag(gsu, SFR_OV, (sr ^ operand) & (sr ^ result) & 0x8000);
      gsu_setFlag(gsu, SFR_S, result & 0x8000);
      gsu_setFlag(gsu, SFR_CY, result >= 0);
      gsu_setFlag(gsu, SFR_Z, (uint16_t) result == 0);
      if(!(alt1 && alt2)) gsu_setDr(gsu, result);
      gsu_resetPrefix(gsu);
      break;
    }
    case 0x70: { // merge
      uint16_t result = (gsu->r[7] & 0xff00) | (gsu->r[8] >> 8);
      gsu_setDr(gsu, result);
      gsu_setFlag(gsu, SFR_OV, result & 0xc0c0);
      gsu_setFlag(gsu, SFR_S, result & 0x8080);
      gsu_setFlag(gsu, SFR_CY, result & 0xe0e0);
      gsu_setFlag(gsu, SFR_Z, result & 0xf0f0);
      gsu_resetPrefix(gsu);
      break;
    }
    case 0x71 ... 0x7f: { // and rn / bic rn / and #n / bic #n
      uint16_t operand = alt2 ? n : gsu->r[n];
      if(alt1) operand = ~operand;
      uint16_t result = sr & operand;
      gsu_setDr(gsu, result);
      gsu_setSZ(gsu, result);
      gsu_resetPrefix(gsu);
      break;
    }
    case 0x80 ... 0x8f: { // mult rn / umult rn / mult #n / umult #n
      uint16_t operand = alt2 ? n : gsu->r[n];
      uint16_t result = alt1 ? (uint8_t) sr * (uint8_t) operand : (int8_t) sr * (int8_t) operand;
      gsu_setDr(gsu, result);
      gsu_setSZ(gsu, result);
      gsu_resetPrefix(gsu);
      if(!(gsu->cfgr & 0x20)) gsu_step(gsu, gsu->clsr ? 1 : 2); // not the fast multiplier
      break;
    }
    case 0x90: { // sbk
      gsu_writeRam(gsu, gsu->ramaddr, sr);
      gsu_writeRam(gsu, gsu->ramaddr ^ 1, sr >> 8);
      gsu_resetPrefix(gsu);
      break;
    }
    case 0x91 ... 0x94: { // link #n
      gsu_setReg(gsu, 11, gsu->r[15] + n);
      gsu_resetPrefix(gsu);
      break;
    }
    case 0x95: { // sex
      uint16_t result = (int8_t) sr;
      gsu_setDr(gsu, result);
      gsu_setSZ(gsu, result);
      gsu_resetPrefix(gsu);
      break;
    }
    case 0x96: { // asr / div2
      gsu_setFlag(gsu, SFR_CY, sr & 1);
      uint16_t result = ((int16_t) sr >> 1) + ((alt1 && sr == 0xffff) ? 1 : 0);
      gsu_setDr(gsu, result);
      gsu_setSZ(gsu, result);
      gsu_resetPrefix(gsu);
      break;
    }
    case 0x97: { // ror
      bool carry = sr & 1;
      uint16_t result = (((gsu->sfr & SFR_CY) ? 1 : 0) << 15) | (sr >> 1);
      gsu_setDr(gsu, result);
      gsu_setSZ(gsu, result);
      gsu_setFlag(gsu, SFR_CY, carry);
      gsu_resetPrefix(gsu);
      break;
    }
    case 0x98 ... 0x9d: { // jmp rn / ljmp rn
      if(!alt1) {
        gsu_setReg(gsu, 15, gsu->r[n]);
      } else {
        gsu->pbr = gsu->r[n] & 0x7f;
        gsu_setReg(gsu, 15, sr);
        gsu->cbr = gsu->r[15] & 0xfff0;
        gsu_flushCache(gsu);
      }
      gsu_resetPrefix(gsu);
      break;
    }
    case 0x9e: { // lob
      uint16_t result = sr & 0xff;
      gsu_setDr(gsu, result);
      gsu_setFlag(gsu, SFR_S, result & 0x80);
      gsu_setFlag(gsu, SFR_Z, result == 0);
      gsu_resetPrefix(gsu);
      break;
    }
    case 0x9f: { // fmult / lmult
      uint32_t result = (int16_t) sr * (int16_t) gsu->r[6];
      if(alt1) gsu_setReg(gsu, 4, result);
      gsu_setDr(gsu, result >> 16);
      gsu_setFlag(gsu, SFR_S, result & 0x80000000);
      gsu_setFlag(gsu, SFR_CY, result & 0x8000);
      gsu_setFlag(gsu, SFR_Z, (result >> 16) == 0);
      gsu_resetPrefix(gsu);
      gsu_step(gsu, ((gsu->cfgr & 0x20) ? 3 : 7) * (gsu->clsr ? 1 : 2));
      break;
    }
    case 0xa0 ... 0xaf: { // ibt rn, #pp / lms rn, (yy) / sms (yy), rn
      if(alt1) {
        gsu->ramaddr = gsu_pipe(gsu) << 1;
        uint16_t data = gsu_readRam(gsu, gsu->ramaddr);
        data |= gsu_readRam(gsu, gsu->ramaddr ^ 1) << 8;
        gsu_setReg(gsu, n, data);
      } else if(alt2) {
        gsu->ramaddr = gsu_pipe(gsu) << 1;
        gsu_writeRam(gsu, gsu->ramaddr, gsu->r[n]);
        gsu_writeRam(gsu, gsu->ramaddr ^ 1, gsu->r[n] >> 8);
      } else {
        gsu_setReg(gsu, n, (int8_t) gsu_pipe(gsu));
      }
      gsu_resetPrefix(gsu);
      break;
    }
    case 0xb0 ... 0xbf: { // from rn / moves (after with)
      if(!(gsu->sfr & SFR_B)) {
        gsu->sreg = n;
        break;
      }
      uint16_t result = gsu->r[n];
      gsu_setDr(gsu, result);
      gsu_setFlag(gsu, SFR_OV, result & 0x80);
      gsu_setSZ(gsu, result);
      gsu_resetPrefix(gsu);
      break;
    }
    case 0xc0: { // hib
      uint16_t result = sr >> 8;
      gsu_setDr(gsu, result);
      gsu_setFlag(gsu, SFR_S, result & 0x80);
      gsu_setFlag(gsu, SFR_Z, result == 0);
      gsu_resetPrefix(gsu);
      break;
    }
    case 0xc1 ... 0xcf: { // or rn / xor rn / or #n / xor #n
      uint16_t operand = alt2 ? n : gsu->r[n];
      uint16_t result = alt1 ? sr ^ operand : sr | operand;
      gsu_setDr(gsu, result);
      gsu_setSZ(gsu, result);
      gsu_resetPrefix(gsu);
      break;
    }
    case 0xd0 ... 0xde: { // inc rn
      gsu_setReg(gsu, n, gsu->r[n] + 1);
      gsu_setSZ(gsu, gsu->r[n]);
      gsu_resetPrefix(gsu);
      break;
    }
    case 0xdf: { // getc / ramb / romb
      if(!alt2) gsu->colr = gsu_color(gsu, gsu_readRomBuffer(gsu));
      else if(!alt1) gsu->rambr = sr & 0x01;
      else gsu->rombr = sr & 0x7f;
      gsu_resetPrefix(gsu);
      break;
    }
    case 0xe0 ... 0xee: { // dec rn
      gsu_setReg(gsu, n, gsu->r[n] - 1);
      gsu_setSZ(gsu, gsu->r[n]);
      gsu_resetPrefix(gsu);
      break;
    }
    case 0xef: { // getb / getbh / getbl / getbs
      uint8_t data = gsu_readRomBuffer(gsu);
      uint16_t result;
      if(alt1 && alt2) result = (int8_t) data;
      else if(alt1) result = (data << 8) | (sr & 0xff);
      else if(alt2) result = (sr & 0xff00) | data;
      else result = data;
      gsu_setDr(gsu, result);
      gsu_resetPrefix(gsu);
      break;
    }
    case 0xf0 ... 0xff: { // iwt rn, #xx / lm rn, (xx) / sm (xx), rn
      if(alt1) {
        gsu->ramaddr = gsu_pipe(gsu);
        gsu->ramaddr |= gsu_pipe(gsu) << 8;
        uint16_t data = gsu_readRam(gsu, gsu->ramaddr);
        data |= gsu_readRam(gsu, gsu->ramaddr ^ 1) << 8;
        gsu_setReg(gsu, n, data);
      } else if(alt2) {
        gsu->ramaddr = gsu_pipe(gsu);
        gsu->ramaddr |= gsu_pipe(gsu) << 8;
        gsu_writeRam(gsu, gsu->ramaddr, gsu->r[n]);
        gsu_writeRam(gsu, gsu->ramaddr ^ 1, gsu->r[n] >> 8);
      } else {
        uint16_t data = gsu_pipe(gsu);
        data |= gsu_pipe(gsu) << 8;
        gsu_setReg(gsu, n, data);
      }
      gsu_resetPrefix(gsu);
      break;
    }
  }
}

void gsu_run(Gsu* gsu, int cycles) {
  if(!(gsu->sfr & SFR_G)) return;
  gsu->budget += cycles;
  while(gsu->budget > 0) {
    gsu->r14Set = false;
    gsu->r15Set = false;
    uint8_t op = gsu_peekpipe(gsu);
    gsu_execute(gsu, op);
    if(gsu->r14Set) gsu_updateRomBuffer(gsu);
    if(!gsu->r15Set) gsu->r[15]++;
    if(!(gsu->sfr & SFR_G)) {
      gsu->budget = 0;
      break;
    }
  }
}

// the cpu's side: $3000-$34ff

static uint8_t gsu_readCache(Gsu* gsu, uint16_t adr) {
  return gsu->cache[(adr + gsu->cbr) & 511];
}

static void gsu_writeCache(Gsu* gsu, uint16_t adr, uint8_t val) {
  adr = (adr + gsu->cbr) & 511;
  gsu->cache[adr] = val;
  if((adr & 15) == 15) gsu->cacheValid[adr >> 4] = true;
}

uint8_t gsu_read(Gsu* gsu, uint16_t adr) {
  adr = 0x3000 | (adr & 0x3ff);
  if(adr >= 0x3100 && adr < 0x3300) return gsu_readCache(gsu, adr - 0x3100);
  if(adr < 0x3020) return gsu->r[(adr >> 1) & 15] >> ((adr & 1) << 3);
  switch(adr) {
    case 0x3030: return gsu->sfr;
    case 0x3031: {
      uint8_t val = gsu->sfr >> 8;
      gsu->sfr &= ~SFR_IRQ;
      cpu_setIrq(gsu->snes->cpu, gsu->snes->inIrq);
      return val;
    }
    case 0x3034: return gsu->pbr;
    case 0x3036: return gsu->rombr;
    case 0x303b: return 0x04; // version: gsu-2
    case 0x303c: return gsu->rambr;
    case 0x303e: return gsu->cbr;
    case 0x303f: return gsu->cbr >> 8;
  }
  return 0;
}

void gsu_write(Gsu* gsu, uint16_t adr, uint8_t val) {
  adr = 0x3000 | (adr & 0x3ff);
  if(adr >= 0x3100 && adr < 0x3300) {
    gsu_writeCache(gsu, adr - 0x3100, val);
    return;
  }
  if(adr < 0x3020) {
    int n = (adr >> 1) & 15;
    if(adr & 1) gsu->r[n] = (val << 8) | (gsu->r[n] & 0xff);
    else gsu->r[n] = (gsu->r[n] & 0xff00) | val;
    if(n == 14) gsu_updateRomBuffer(gsu);
    if(adr == 0x301f) gsu->sfr |= SFR_G; // writing r15's high byte starts it
    return;
  }
  switch(adr) {
    case 0x3030: {
      bool wasRunning = gsu->sfr & SFR_G;
      gsu->sfr = (gsu->sfr & 0xff00) | val;
      if(wasRunning && !(gsu->sfr & SFR_G)) {
        gsu->cbr = 0;
        gsu_flushCache(gsu);
      }
      break;
    }
    case 0x3031: gsu->sfr = (val << 8) | (gsu->sfr & 0x00ff); break;
    case 0x3033: gsu->bramr = val & 0x01; break;
    case 0x3034: gsu->pbr = val & 0x7f; gsu_flushCache(gsu); break;
    case 0x3037: gsu->cfgr = val; break;
    case 0x3038: gsu->scbr = val; break;
    case 0x3039: gsu->clsr = val & 0x01; break;
    case 0x303a: gsu->scmr = val; break;
  }
}
