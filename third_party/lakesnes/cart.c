
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "cart.h"
#include "snes.h"
#include "statehandler.h"
#include "gsu.h"

static uint8_t cart_readLorom(Cart* cart, uint8_t bank, uint16_t adr);
static void cart_writeLorom(Cart* cart, uint8_t bank, uint16_t adr, uint8_t val);
static uint8_t cart_readHirom(Cart* cart, uint8_t bank, uint16_t adr);
static uint8_t cart_readExHirom(Cart* cart, uint8_t bank, uint16_t adr);
static void cart_writeHirom(Cart* cart, uint8_t bank, uint16_t adr, uint8_t val);
static uint8_t cart_readSuperFx(Cart* cart, uint8_t bank, uint16_t adr);
static void cart_writeSuperFx(Cart* cart, uint8_t bank, uint16_t adr, uint8_t val);

Cart* cart_init(Snes* snes) {
  Cart* cart = malloc(sizeof(Cart));
  cart->snes = snes;
  cart->type = 0;
  cart->rom = NULL;
  cart->romSize = 0;
  cart->ram = NULL;
  cart->ramSize = 0;
  cart->gsu = NULL;
  return cart;
}

void cart_free(Cart* cart) {
  if(cart->gsu != NULL) gsu_free(cart->gsu);
  if(cart->rom != NULL) free(cart->rom);
  if(cart->ram != NULL) free(cart->ram);
  free(cart);
}

void cart_reset(Cart* cart) {
  // do not reset ram, assumed to be battery backed
  if(cart->gsu != NULL) gsu_reset(cart->gsu);
}

bool cart_handleTypeState(Cart* cart, StateHandler* sh) {
  // when loading, return if values match
  if(sh->saving) {
    sh_handleBytes(sh, &cart->type, NULL);
    sh_handleInts(sh, &cart->romSize, &cart->ramSize, NULL);
    return true;
  } else {
    uint8_t type = 0;
    uint32_t romSize = 0;
    uint32_t ramSize = 0;
    sh_handleBytes(sh, &type, NULL);
    sh_handleInts(sh, &romSize, &ramSize, NULL);
    return !(type != cart->type || romSize != cart->romSize || ramSize != cart->ramSize);
  }
}

void cart_handleState(Cart* cart, StateHandler* sh) {
  if(cart->ram != NULL) sh_handleByteArray(sh, cart->ram, cart->ramSize);
  // snes-ds: only a Super FX cartridge's states hold this, so the others' don't change
  if(cart->gsu != NULL) gsu_handleState(cart->gsu, sh);
}

void cart_load(Cart* cart, int type, uint8_t* rom, int romSize, int ramSize) {
  cart->type = type;
  if(cart->gsu != NULL) gsu_free(cart->gsu);
  cart->gsu = NULL;
  if(cart->rom != NULL) free(cart->rom);
  if(cart->ram != NULL) free(cart->ram);
  cart->rom = malloc(romSize);
  cart->romSize = romSize;
  if(ramSize > 0) {
    cart->ram = malloc(ramSize);
    memset(cart->ram, 0, ramSize);
  } else {
    cart->ram = NULL;
  }
  cart->ramSize = ramSize;
  memcpy(cart->rom, rom, romSize);
  if(type == 4) cart->gsu = gsu_init(cart->snes, cart->rom, romSize, cart->ram, ramSize);
}

bool cart_handleBattery(Cart* cart, bool save, uint8_t* data, int* size) {
  if(save) {
    *size = cart->ramSize;
    if(data == NULL) return true;
    // assumes data is correct size
    if(cart->ram != NULL) memcpy(data, cart->ram, cart->ramSize);
    return true;
  } else {
    if(*size != cart->ramSize) return false;
    if(cart->ram != NULL) memcpy(cart->ram, data, cart->ramSize);
    return true;
  }
}

uint8_t cart_read(Cart* cart, uint8_t bank, uint16_t adr) {
  switch(cart->type) {
    case 0: return cart->snes->openBus;
    case 1: return cart_readLorom(cart, bank, adr);
    case 2: return cart_readHirom(cart, bank, adr);
    case 3: return cart_readExHirom(cart, bank, adr);
    case 4: return cart_readSuperFx(cart, bank, adr);
  }
  return cart->snes->openBus;
}

void cart_write(Cart* cart, uint8_t bank, uint16_t adr, uint8_t val) {
  switch(cart->type) {
    case 0: break;
    case 1: cart_writeLorom(cart, bank, adr, val); break;
    case 2: cart_writeHirom(cart, bank, adr, val); break;
    case 3: cart_writeHirom(cart, bank, adr, val); break;
    case 4: cart_writeSuperFx(cart, bank, adr, val); break;
  }
}

static uint8_t cart_readLorom(Cart* cart, uint8_t bank, uint16_t adr) {
  if(((bank >= 0x70 && bank < 0x7e) || bank >= 0xf0) && adr < 0x8000 && cart->ramSize > 0) {
    // banks 70-7e and f0-ff, adr 0000-7fff
    return cart->ram[(((bank & 0xf) << 15) | adr) & (cart->ramSize - 1)];
  }
  bank &= 0x7f;
  if(adr >= 0x8000 || bank >= 0x40) {
    // adr 8000-ffff in all banks or all addresses in banks 40-7f and c0-ff
    return cart->rom[((bank << 15) | (adr & 0x7fff)) & (cart->romSize - 1)];
  }
  return cart->snes->openBus;
}

static void cart_writeLorom(Cart* cart, uint8_t bank, uint16_t adr, uint8_t val) {
  if(((bank >= 0x70 && bank < 0x7e) || bank > 0xf0) && adr < 0x8000 && cart->ramSize > 0) {
    // banks 70-7e and f0-ff, adr 0000-7fff
    cart->ram[(((bank & 0xf) << 15) | adr) & (cart->ramSize - 1)] = val;
  }
}

static uint8_t cart_readHirom(Cart* cart, uint8_t bank, uint16_t adr) {
  bank &= 0x7f;
  if(bank < 0x40 && adr >= 0x6000 && adr < 0x8000 && cart->ramSize > 0) {
    // banks 00-3f and 80-bf, adr 6000-7fff
    return cart->ram[(((bank & 0x3f) << 13) | (adr & 0x1fff)) & (cart->ramSize - 1)];
  }
  if(adr >= 0x8000 || bank >= 0x40) {
    // adr 8000-ffff in all banks or all addresses in banks 40-7f and c0-ff
    return cart->rom[(((bank & 0x3f) << 16) | adr) & (cart->romSize - 1)];
  }
  return cart->snes->openBus;
}

static uint8_t cart_readExHirom(Cart* cart, uint8_t bank, uint16_t adr) {
  if((bank & 0x7f) < 0x40 && adr >= 0x6000 && adr < 0x8000 && cart->ramSize > 0) {
    // banks 00-3f and 80-bf, adr 6000-7fff
    return cart->ram[(((bank & 0x3f) << 13) | (adr & 0x1fff)) & (cart->ramSize - 1)];
  }
  bool secondHalf = bank < 0x80;
  bank &= 0x7f;
  if(adr >= 0x8000 || bank >= 0x40) {
    // adr 8000-ffff in all banks or all addresses in banks 40-7f and c0-ff
    return cart->rom[(((bank & 0x3f) << 16) | (secondHalf ? 0x400000 : 0) | adr) & (cart->romSize - 1)];
  }
  return cart->snes->openBus;
}

static void cart_writeHirom(Cart* cart, uint8_t bank, uint16_t adr, uint8_t val) {
  bank &= 0x7f;
  if(bank < 0x40 && adr >= 0x6000 && adr < 0x8000 && cart->ramSize > 0) {
    // banks 00-3f and 80-bf, adr 6000-7fff
    cart->ram[(((bank & 0x3f) << 13) | (adr & 0x1fff)) & (cart->ramSize - 1)] = val;
  }
}

// snes-ds: a Super FX cartridge. Its registers at 3000-34ff; rom as lorom
// at 8000-ffff and linear in banks 40-5f; ram in banks 70-71 and its first
// 8K at 6000-7fff. While the Super FX has the rom, the cpu reads the
// interrupt vectors it gives instead (pointing into wram); while it has the
// ram, the cpu gets nothing from it.

bool cart_irq(Cart* cart) {
  return cart->gsu != NULL && (cart->gsu->sfr & 0x8000);
}

static uint8_t cart_readSuperFxRom(Cart* cart, uint32_t offset) {
  if(gsu_ownsRom(cart->gsu)) {
    static const uint8_t vector[16] = {
      0x00, 0x01, 0x00, 0x01, 0x04, 0x01, 0x00, 0x01,
      0x00, 0x01, 0x08, 0x01, 0x00, 0x01, 0x0c, 0x01,
    };
    return vector[offset & 15];
  }
  return cart->rom[offset & (cart->romSize - 1)];
}

static uint8_t cart_readSuperFxRam(Cart* cart, uint32_t offset) {
  if(gsu_ownsRam(cart->gsu)) return cart->snes->openBus;
  return cart->ram[offset & (cart->ramSize - 1)];
}

static void cart_writeSuperFxRam(Cart* cart, uint32_t offset, uint8_t val) {
  if(gsu_ownsRam(cart->gsu)) return;
  cart->ram[offset & (cart->ramSize - 1)] = val;
}

static uint8_t cart_readSuperFx(Cart* cart, uint8_t bank, uint16_t adr) {
  if(cart->gsu->pending) gsu_catchUp(cart->gsu); // up to now, before the cpu looks
  bank &= 0x7f;
  if(bank < 0x40) {
    if(adr >= 0x3000 && adr < 0x3500) return gsu_read(cart->gsu, adr);
    if(adr >= 0x6000 && adr < 0x8000) return cart_readSuperFxRam(cart, adr & 0x1fff);
    if(adr >= 0x8000) return cart_readSuperFxRom(cart, (bank << 15) | (adr & 0x7fff));
    return cart->snes->openBus;
  }
  if(bank < 0x60) return cart_readSuperFxRom(cart, ((bank & 0x1f) << 16) | adr);
  if(bank == 0x70 || bank == 0x71) return cart_readSuperFxRam(cart, ((bank & 1) << 16) | adr);
  return cart->snes->openBus;
}

static void cart_writeSuperFx(Cart* cart, uint8_t bank, uint16_t adr, uint8_t val) {
  if(cart->gsu->pending) gsu_catchUp(cart->gsu);
  bank &= 0x7f;
  if(bank < 0x40) {
    if(adr >= 0x3000 && adr < 0x3500) gsu_write(cart->gsu, adr, val);
    if(adr >= 0x6000 && adr < 0x8000) cart_writeSuperFxRam(cart, adr & 0x1fff, val);
    return;
  }
  if(bank == 0x70 || bank == 0x71) cart_writeSuperFxRam(cart, ((bank & 1) << 16) | adr, val);
}
