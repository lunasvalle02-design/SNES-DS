#include "profiles.h"

#include <stddef.h>

#define M(a, v) { (a), 0xFF, (v), false, 0 }
#define M2(a, v, w) { (a), 0xFF, (v), true, (w) }

static const Profile profiles[] = {
	{
		.title = "The Legend of Zelda: A Link to the Past",
		.crc32 = { 0x777AAC2F }, /* USA */
		/* $7E0010 is the main module, $7E0011 the sub-module: 07 indoors,
		 * 09 outdoors, 0E the menus (01 items, 02 text, 03 dungeon map,
		 * 07 world map). BG3 holds the HUD, the text box and the item menu */
		.split = {
			/* the item menu: all of BG3, the world stays on the game screen */
			{ .when = { M(0x7E0010, 0x0E), M(0x7E0011, 0x01) }, .kind = SPLIT_FULL, .layers = LAYER_BG3, .y0 = 0, .y1 = 224 },
			/* in play (rooms, scrolling between them): the top of BG3, the HUD
			 * (lines 15-59) */
			{ .when = { M2(0x7E0010, 0x07, 0x09) }, .kind = SPLIT_HUD, .layers = LAYER_BG3, .y0 = 0, .y1 = 64 },
			/* text: the box starts at BG3 row 9 (line 72) when it's up top
			 * ($7E1CD2 = $6125), row 18 below ($6244), so it never reaches
			 * the HUD's lines */
			{ .when = { M(0x7E0010, 0x0E), M(0x7E0011, 0x02) }, .kind = SPLIT_HUD, .layers = LAYER_BG3, .y0 = 0, .y1 = 64 },
			/* going in or out of a door (the spotlight) */
			{ .when = { M2(0x7E0010, 0x0F, 0x10) }, .kind = SPLIT_HUD, .layers = LAYER_BG3, .y0 = 0, .y1 = 64 },
		},
		/* the HUD is lines 15-59; a little room around it */
		.hudShowY0 = 12, .hudShowY1 = 60,
		.map = {
			.enabled = true,
			.open = { { PAD_X, 4 } },
			/* on the world map, X again: the whole world, which doesn't scroll
			 * along with Link like the close-up does */
			.open2If = { M(0x7E0011, 0x07) }, .open2Delay = 20, .open2 = { { PAD_X, 4 } },
			.live = { M2(0x7E0010, 0x07, 0x09), M(0x7E0011, 0x00) },
			.shown = { M(0x7E0010, 0x0E), M2(0x7E0011, 0x07, 0x03) },
			/* overworld area, dungeon room, indoors */
			.room = { 0x7E008A, 0x7E00A0, 0x7E001B },
			.openWithin = 40, .settleMax = 200, .stableFrames = 4, .maxDiff = 400,
			.minForeground = 2000, .bgColor = 0x000000,
			.animFrames = 8, .animStep = 4,
			.refresh = 90,
		},
	},
};

uint32_t rom_crc32(const uint8_t* rom, size_t size) {
	/* a 512-byte copier header doesn't count */
	if (size % 0x8000 == 512) {
		rom += 512;
		size -= 512;
	}
	uint32_t crc = 0xFFFFFFFF;
	for (size_t i = 0; i < size; i++) {
		crc ^= rom[i];
		for (int k = 0; k < 8; k++) crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
	}
	return ~crc;
}

const Profile* profile_find(const uint8_t* rom, size_t size) {
	uint32_t crc = rom_crc32(rom, size);
	for (size_t i = 0; i < sizeof(profiles) / sizeof(profiles[0]); i++) {
		for (int j = 0; j < 4 && profiles[i].crc32[j]; j++) {
			if (profiles[i].crc32[j] == crc) return &profiles[i];
		}
	}
	return NULL;
}

bool probes_match(Core* c, const MemProbe* p) {
	for (int i = 0; i < PROBES && p[i].addr; i++) {
		uint8_t b = core_peek(c, p[i].addr) & p[i].mask;
		if (b != p[i].val && !(p[i].hasAlt && b == p[i].alt)) return false;
	}
	return true;
}

const SplitRule* profile_split(const Profile* p, Core* c) {
	if (!p) return NULL;
	for (int i = 0; i < SPLITS && p->split[i].layers; i++) {
		if (probes_match(c, p->split[i].when)) return &p->split[i];
	}
	return NULL;
}
