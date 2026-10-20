/* Super Metroid. Everything snes-ds knows about this game is here and in this
 * directory (README.md: the memory map and what each value was checked
 * against; tests/: harness scripts). */
#include "profiles.h"

/* $7E0998 is the game state: 08 play, 09 touching a door, 0A/0B going
 * through it, 0C darkening to pause, 0D/0E loading the pause screen, 0F the
 * pause screen (map or equipment), 10 darkening it, 11 loading the room
 * again, 12 brightening back to play */
#define STATE 0x7E0998
/* 08-0B: play and doors */
#define IN_PLAY { STATE, 0xFC, 0x08, false, 0 }

/* the HUD: BG3, lines 0-30 in play (31 on the pause screen); an IRQ at line
 * 31 changes the layers, so BG3 below it is the room's fog, water or lava
 * and stays on the game screen */
#define HUD .kind = SPLIT_HUD, .layers = LAYER_BG3, .y0 = 0, .y1 = 32

const Profile game_super_metroid = {
	.id = "super_metroid",
	.title = "Super Metroid",
	.about = {
		[LANG_EN] = "The pause map of the area you're in, with the HUD (energy, ammo and minimap) over it, on the bottom screen.",
		[LANG_ES] = "El mapa de pausa de la zona en la que estás, con el HUD (energía, munición y minimapa) encima, en la pantalla de abajo.",
	},
	.crc32 = { 0xD63ED5F8 }, /* Japan and USA (the same ROM), v1.0 */
	/* the frames where the HUD isn't drawn (0D/0E and 11 load with the
	 * screen off; the first frame of 12 too) are left out, so the second
	 * screen keeps the last HUD instead of blinking; the HUD is on the pause
	 * screen too, so it stays down there while paused */
	.split = {
		{ .when = { IN_PLAY }, HUD },
		{ .when = { PROBE_EITHER(STATE, 0x0C, 0x0F) }, HUD },
		{ .when = { PROBE_EITHER(STATE, 0x10, 0x12) }, HUD },
	},
	/* the whole HUD: energy, missiles, super missiles, power bombs, grapple,
	 * x-ray and the minimap */
	.hudShowY0 = 0, .hudShowY1 = 32,
	.map = {
		.enabled = true,
		/* Start pauses: 0C 6 frames later, the map (0F) 60 frames after that */
		.open = { { PAD_START, 6 } },
		.live = { PROBE(STATE, 0x08) },
		/* $7E0727: the pause screen's page, 00 the map, 01 equipment (02-04
		 * between them); a pause always opens on the map */
		.shown = { PROBE(STATE, 0x0F), PROBE(0x7E0727, 0x00) },
		/* the room (its header's address in bank 8F) and the area; the map
		 * also fills in as Samus explores a room, which `refresh` catches */
		.room = { 0x7E079B, 0x7E079F },
		/* Samus can't pause while she's getting out of the ship, in an
		 * elevator or on Ceres Station: no map there (the last one dims) */
		.openWithin = 100, .settleMax = 200, .stableFrames = 4, .maxDiff = 400,
		.minForeground = 2000, .bgColor = 0x000000,
		/* Samus's marker blinks */
		.animFrames = 8, .animStep = 8,
		/* a capture runs ~110 frames on the hidden core */
		.refresh = 180,
	},
};
