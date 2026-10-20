/* The Legend of Zelda: A Link to the Past. Everything snes-ds knows about
 * this game is here and in this directory (README.md: the memory map and
 * what each value was checked against; tests/: harness scripts). */
#include "profiles.h"

const Profile game_zelda_alttp = {
	.id = "zelda_alttp",
	.title = "The Legend of Zelda: A Link to the Past",
	.shortTitle = "A Link to the Past",
	.about = "Mapa del mundo y de cada mazmorra, el HUD y el menú de objetos en la pantalla de abajo.",
	.crc32 = { 0x777AAC2F }, /* USA */
	/* $7E0010 is the main module, $7E0011 the sub-module: 07 indoors,
	 * 09 outdoors, 0E the menus (01 items, 02 text, 03 dungeon map,
	 * 07 world map). BG3 holds the HUD, the text box and the item menu */
	.split = {
		/* the item menu: all of BG3, the world stays on the game screen */
		{ .when = { PROBE(0x7E0010, 0x0E), PROBE(0x7E0011, 0x01) }, .kind = SPLIT_FULL, .layers = LAYER_BG3, .y0 = 0, .y1 = 224 },
		/* in play (rooms, scrolling between them): the top of BG3, the HUD
		 * (lines 15-59) */
		{ .when = { PROBE_EITHER(0x7E0010, 0x07, 0x09) }, .kind = SPLIT_HUD, .layers = LAYER_BG3, .y0 = 0, .y1 = 64 },
		/* text: the box starts at BG3 row 9 (line 72) when it's up top
		 * ($7E1CD2 = $6125), row 18 below ($6244), so it never reaches
		 * the HUD's lines */
		{ .when = { PROBE(0x7E0010, 0x0E), PROBE(0x7E0011, 0x02) }, .kind = SPLIT_HUD, .layers = LAYER_BG3, .y0 = 0, .y1 = 64 },
		/* going in or out of a door (the spotlight) */
		{ .when = { PROBE_EITHER(0x7E0010, 0x0F, 0x10) }, .kind = SPLIT_HUD, .layers = LAYER_BG3, .y0 = 0, .y1 = 64 },
	},
	/* the HUD is lines 15-59; a little room around it */
	.hudShowY0 = 12, .hudShowY1 = 60,
	/* at power-on, right after the sound driver is uploaded (frame 82),
	 * the game asks for sound effect $0A on $2143: a very high-pitched
	 * sample on voice 7, 0.3 s of hiss over the black screen. Nothing else
	 * plays before frame 110 (the intro starts later); see README.md */
	.bootMuteFrames = 110,
	.map = {
		.enabled = true,
		.open = { { PAD_X, 4 } },
		/* on the world map, X again: the whole world, which doesn't scroll
		 * along with Link like the close-up does */
		.open2If = { PROBE(0x7E0011, 0x07) }, .open2Delay = 20, .open2 = { { PAD_X, 4 } },
		.live = { PROBE_EITHER(0x7E0010, 0x07, 0x09), PROBE(0x7E0011, 0x00) },
		/* world map or dungeon map; the dungeon map comes up on a black
		 * screen after the fade, which the capture waits out */
		.shown = { PROBE(0x7E0010, 0x0E), PROBE_EITHER(0x7E0011, 0x07, 0x03) },
		/* overworld area, dungeon room, indoors */
		.room = { 0x7E008A, 0x7E00A0, 0x7E001B },
		.openWithin = 40, .settleMax = 200, .stableFrames = 4, .maxDiff = 400,
		.minForeground = 2000, .bgColor = 0x000000,
		.animFrames = 8, .animStep = 4,
		.refresh = 90,
	},
};
