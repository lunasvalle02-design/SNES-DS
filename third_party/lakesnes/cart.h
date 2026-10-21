
#ifndef CART_H
#define CART_H

#include <stdint.h>
#include <stdbool.h>

typedef struct Cart Cart;
typedef struct Gsu Gsu;

#include "snes.h"
#include "statehandler.h"

struct Cart {
  Snes* snes;
  uint8_t type;

  uint8_t* rom;
  uint32_t romSize;
  uint8_t* ram;
  uint32_t ramSize;
  // snes-ds: the Super FX, on a cartridge of type 4 (else NULL)
  Gsu* gsu;
};

// TODO: how to handle reset & load?

Cart* cart_init(Snes* snes);
void cart_free(Cart* cart);
void cart_reset(Cart* cart); // will reset special chips etc, general reading is set up in load
bool cart_handleTypeState(Cart* cart, StateHandler* sh);
void cart_handleState(Cart* cart, StateHandler* sh);
void cart_load(Cart* cart, int type, uint8_t* rom, int romSize, int ramSize); // loads rom, sets up ram buffer
bool cart_handleBattery(Cart* cart, bool save, uint8_t* data, int* size); // saves/loads ram
uint8_t cart_read(Cart* cart, uint8_t bank, uint16_t adr);
void cart_write(Cart* cart, uint8_t bank, uint16_t adr, uint8_t val);
// snes-ds: the cartridge holds the cpu's irq line (the Super FX when it stops)
bool cart_irq(Cart* cart);

#endif
