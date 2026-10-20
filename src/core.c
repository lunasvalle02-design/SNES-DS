/* core.h on top of LakeSnes (third_party/lakesnes, MIT). */
#include "core.h"

#include <stdlib.h>
#include <string.h>

#include "snes.h"

#define SAMPLES_PER_FRAME 800 /* 48000 / 60 */

struct Core {
	Snes* snes;
	uint8_t* rom; /* LakeSnes keeps its own copy; ours is for the header */
	size_t romSize;
	char title[22];
	uint32_t frames;
	uint32_t frame[CORE_W * CORE_H];
	uint32_t split[CORE_W * CORE_H];
	uint8_t oam[544];
};

Core* core_new(void) {
	Core* c = calloc(1, sizeof(Core));
	if (!c) return NULL;
	c->snes = snes_init();
	if (!c->snes) {
		free(c);
		return NULL;
	}
	memset(c->snes->ppu->hideMask, 0, sizeof(c->snes->ppu->hideMask));
	memset(c->snes->ppu->splitMask, 0, sizeof(c->snes->ppu->splitMask));
	return c;
}

void core_free(Core* c) {
	if (!c) return;
	snes_free(c->snes);
	free(c->rom);
	free(c);
}

/* the header title of whichever mapping the core picked */
static void read_title(Core* c) {
	const Cart* cart = c->snes->cart;
	uint32_t at = cart->type == 1 ? 0x7FC0 : cart->type == 3 ? 0x40FFC0 : 0xFFC0;
	memset(c->title, 0, sizeof(c->title));
	if (!cart->rom || at + 21 > cart->romSize) return;
	for (int i = 0; i < 21; i++) {
		uint8_t ch = cart->rom[at + i];
		c->title[i] = ch >= 0x20 && ch < 0x7F ? (char)ch : '.';
	}
	for (int i = 20; i >= 0 && c->title[i] == ' '; i--) c->title[i] = 0;
}

bool core_load_rom(Core* c, const uint8_t* rom, size_t size) {
	if (!snes_loadRom(c->snes, rom, (int)size)) return false;
	free(c->rom);
	c->rom = malloc(size);
	if (c->rom) memcpy(c->rom, rom, size);
	c->romSize = size;
	c->frames = 0;
	read_title(c);
	return true;
}

const char* core_title(Core* c) { return c->title; }

void core_set_pad(Core* c, int player, uint16_t buttons) {
	Input* in = player == 1 ? c->snes->input2 : c->snes->input1;
	in->currentState = buttons;
}

void core_set_threaded(Core* c, bool on) { ppu_setAsync(c->snes->ppu, on); }

void core_set_render(Core* c, bool on) { c->snes->ppu->skipRender = !on; }

void core_run_frame(Core* c) {
	snes_runFrame(c->snes);
	ppu_sync(c->snes->ppu); /* every line drawn */
	c->frames++;
	/* the main screen's picture, as the PPU leaves it (snes-ds patch);
	 * overscan (239-line) frames lose 7 lines top and bottom */
	const Ppu* ppu = c->snes->ppu;
	int first = ppu->frameOverscan ? 7 : 0;
	memcpy(c->frame, ppu->outBuffer + first * 256, sizeof(c->frame));
	/* the split picture, cut the same way; lines not split are empty */
	for (int y = 0; y < CORE_H; y++) {
		uint32_t* dst = c->split + y * CORE_W;
		if (ppu->splitMask[first + y + 1]) memcpy(dst, ppu->splitBuffer + (first + y) * 256, CORE_W * 4);
		else memset(dst, 0, CORE_W * 4);
	}
}

uint32_t core_frame_count(Core* c) { return c->frames; }
const uint32_t* core_frame(Core* c) { return c->frame; }

int core_audio(Core* c, int16_t* out, int frames) {
	if (frames > SAMPLES_PER_FRAME) frames = SAMPLES_PER_FRAME;
	snes_setSamples(c->snes, out, frames);
	return frames;
}

uint8_t* core_wram(Core* c) { return c->snes->ram; }
const uint16_t* core_vram(Core* c) { return c->snes->ppu->vram; }
const uint16_t* core_cgram(Core* c) { return c->snes->ppu->cgram; }

const uint8_t* core_oam(Core* c) {
	/* LakeSnes keeps the low table as words and the high table apart */
	memcpy(c->oam, c->snes->ppu->oam, 512);
	memcpy(c->oam + 512, c->snes->ppu->highOam, 32);
	return c->oam;
}

/* WRAM behind a CPU address, or NULL */
static uint8_t* wram_at(Core* c, uint32_t addr) {
	uint8_t bank = addr >> 16;
	uint16_t a = addr & 0xFFFF;
	if (bank == 0x7E || bank == 0x7F) return &c->snes->ram[addr - 0x7E0000];
	if ((bank < 0x40 || (bank >= 0x80 && bank < 0xC0)) && a < 0x2000) return &c->snes->ram[a];
	return NULL;
}

uint8_t core_peek(Core* c, uint32_t addr) {
	uint8_t* w = wram_at(c, addr);
	if (w) return *w;
	uint8_t bank = addr >> 16;
	uint16_t a = addr & 0xFFFF;
	/* I/O isn't read: it would have side effects */
	if ((bank < 0x40 || (bank >= 0x80 && bank < 0xC0)) && a >= 0x2000 && a < 0x6000) return 0;
	return cart_read(c->snes->cart, bank, a);
}

void core_poke(Core* c, uint32_t addr, uint8_t val) {
	uint8_t* w = wram_at(c, addr);
	if (w) *w = val;
	else cart_write(c->snes->cart, addr >> 16, addr & 0xFFFF, val);
}

void core_hide_layers(Core* c, uint8_t mask, int y0, int y1) {
	uint8_t* m = c->snes->ppu->hideMask;
	if (y0 < 0) y0 = 0;
	/* the PPU counts visible lines from 1 */
	if (y1 > 239) y1 = 239;
	for (int y = y0; y < y1; y++) m[y + 1] = mask;
}

void core_split_layers(Core* c, uint8_t mask, int y0, int y1) {
	uint8_t* m = c->snes->ppu->splitMask;
	if (y0 < 0) y0 = 0;
	if (y1 > 239) y1 = 239;
	for (int y = y0; y < y1; y++) m[y + 1] = mask;
}

const uint32_t* core_split_frame(Core* c) { return c->split; }

size_t core_state_size(Core* c) { return (size_t)snes_saveState(c->snes, NULL); }

bool core_save_state(Core* c, uint8_t* out) {
	snes_saveState(c->snes, out);
	return true;
}

bool core_load_state(Core* c, const uint8_t* data, size_t size) {
	return snes_loadState(c->snes, (uint8_t*)data, (int)size);
}

bool core_copy_state(Core* dst, Core* src) {
	size_t n = core_state_size(src);
	uint8_t* buf = malloc(n);
	if (!buf) return false;
	core_save_state(src, buf);
	bool ok = core_load_state(dst, buf, n);
	free(buf);
	if (ok) dst->frames = src->frames;
	return ok;
}

size_t core_sram(Core* c, uint8_t* out) { return (size_t)snes_saveBattery(c->snes, out); }

bool core_load_sram(Core* c, const uint8_t* data, size_t size) {
	return snes_loadBattery(c->snes, (uint8_t*)data, (int)size);
}
