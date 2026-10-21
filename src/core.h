/* The emulation core, behind a small interface so the rest of snes-ds never
 * touches LakeSnes directly (and the core can be swapped after the device
 * benchmark, see ROADMAP.md). Several cores can live side by side: the
 * visible game and the hidden one that opens the map. */
#ifndef SNESDS_CORE_H
#define SNESDS_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CORE_W 256
#define CORE_H 224

/* joypad bits, in the order the SNES shifts them out */
enum {
	PAD_B = 1 << 0, PAD_Y = 1 << 1, PAD_SELECT = 1 << 2, PAD_START = 1 << 3,
	PAD_UP = 1 << 4, PAD_DOWN = 1 << 5, PAD_LEFT = 1 << 6, PAD_RIGHT = 1 << 7,
	PAD_A = 1 << 8, PAD_X = 1 << 9, PAD_L = 1 << 10, PAD_R = 1 << 11,
};

/* layers, for core_hide_layers */
enum { LAYER_BG1 = 1, LAYER_BG2 = 2, LAYER_BG3 = 4, LAYER_BG4 = 8, LAYER_OBJ = 16 };

typedef struct Core Core;

Core* core_new(void);
void core_free(Core* c);
/* rom: the file as read (copier headers are handled by the core) */
bool core_load_rom(Core* c, const uint8_t* rom, size_t size);
/* the cartridge's title, from its header */
const char* core_title(Core* c);

/* draw the picture on a thread of its own while the emulation goes on
 * (the same pictures; another CPU core busy) */
void core_set_threaded(Core* c, bool on);
/* the render thread's time drawing, and the emulation's time waiting for
 * it, in ms since the last call (0 without a render thread) */
void core_render_stats(Core* c, double* drawMs, double* waitMs);

/* off: frames run without being drawn (core_frame keeps the last one) */
void core_set_render(Core* c, bool on);

void core_set_pad(Core* c, int player, uint16_t buttons);
void core_run_frame(Core* c);
uint32_t core_frame_count(Core* c);

/* the last frame, 256x224 0x00RRGGBB (overscan frames are cut to 224) */
const uint32_t* core_frame(Core* c);

/* audio of the last frame, interleaved stereo int16 at 48 kHz */
int core_audio(Core* c, int16_t* out, int frames);

/* memory: WRAM is $7E0000-$7FFFFF (128 KB) */
uint8_t* core_wram(Core* c);
const uint16_t* core_vram(Core* c);  /* 32K words */
const uint16_t* core_cgram(Core* c); /* 256 BGR555 colours */
const uint8_t* core_oam(Core* c);    /* 544 bytes: 512 low + 32 high */
const uint8_t* core_aram(Core* c);   /* the sound CPU's 64 KB */
const uint8_t* core_dsp(Core* c);    /* the sound DSP's 128 registers */
/* the four ports between the CPU and the sound CPU ($2140-$2143), as each
 * side last wrote them */
void core_apu_ports(Core* c, uint8_t toApu[4], uint8_t fromApu[4]);
/* any CPU address (bus read with no side effects is not guaranteed for I/O;
 * use it for WRAM, ROM and SRAM) */
uint8_t core_peek(Core* c, uint32_t addr);
void core_poke(Core* c, uint32_t addr, uint8_t val);

/* leave layers out of the picture on lines [y0, y1) (LAYER_* bits);
 * mask 0 shows everything again */
void core_hide_layers(Core* c, uint8_t mask, int y0, int y1);

/* draw layers into a picture of their own instead of the game's, on lines
 * [y0, y1): the second screen's HUD (LAYER_* bits; 0 stops). The split
 * picture is 256x224 0xAARRGGBB, alpha 0 where those layers are empty. */
void core_split_layers(Core* c, uint8_t mask, int y0, int y1);
const uint32_t* core_split_frame(Core* c);

/* save states in memory; size from core_state_size */
size_t core_state_size(Core* c);
bool core_save_state(Core* c, uint8_t* out);
bool core_load_state(Core* c, const uint8_t* data, size_t size);
/* copy one core's whole state into another core running the same game */
bool core_copy_state(Core* dst, Core* src);

/* battery save (SRAM) */
size_t core_sram(Core* c, uint8_t* out); /* out NULL: just the size */
bool core_load_sram(Core* c, const uint8_t* data, size_t size);

#endif
