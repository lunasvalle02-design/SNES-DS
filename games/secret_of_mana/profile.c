/* Secret of Mana. Everything snes-ds knows about this game is here and in
 * this directory (README.md: the memory map and what each value was
 * checked against; tests/: harness scripts). */
#include "profiles.h"

/* $7E009B says what kind of screen runs: 00 the logo, the title, Game
 * Select and the name entry (and some 6 frames between two maps),
 * 01 the intro's captions, 02 the field (play, its text boxes, the ring
 * menu and the screens it opens) */
#define FIELD PROBE(0x7E009B, 0x02)
/* $7E007D is 01 while the ring menu is up (Y), and through the screens
 * it opens (Status, Action Grid, Controller Edit, Window Edit...) */
#define RING 0x7E007D
/* $7E00CC: 00 in play and in the ring itself, 08 in a screen the ring
 * opened (and in some text boxes, with $007D = 00), 04 for a frame as the
 * ring opens, 0C while a screen closes back to play */
#define MENU 0x7E00CC

/* the HUD: BG3 lines 199-216 (each character's HP and weapon gauge) and
 * the face beside it, a sprite (x 88-102, lines 199-214, the boy alone).
 * The field goes on under it, so the game screen keeps all its lines.
 * The boy's sprite stays above it (line 176 at the lowest, walked into
 * the bottom of three maps), so only the faces go down with the HUD; an
 * enemy that wandered that low would go too */
#define HUD .kind = SPLIT_HUD, .layers = LAYER_BG3 | LAYER_OBJ, .y0 = 199, .y1 = 217

const Profile game_secret_of_mana = {
	.id = "secret_of_mana",
	.title = "Secret of Mana",
	.about = {
		[LANG_EN] = "The HUD (each character's HP and weapon gauge) on the bottom screen, and the ring menu with the screens it opens (status, equipment, settings) while they're up.",
		[LANG_ES] = "El HUD (la vida y la carga del arma de cada personaje) en la pantalla de abajo, y el menú en anillo con las pantallas que abre (estado, equipo, ajustes) mientras están abiertos.",
	},
	.crc32 = { 0xD0176B24 }, /* USA */
	.split = {
		/* a screen the ring opened: it takes the whole picture (its
		 * windows on BG2, a pattern on BG1, the characters as sprites),
		 * so all of it goes down and the game screen is left dark */
		{ .when = { FIELD, PROBE(RING, 0x01), PROBE(MENU, 0x08) }, .kind = SPLIT_FULL,
		  .layers = LAYER_BG1 | LAYER_BG2 | LAYER_BG3 | LAYER_BG4 | LAYER_OBJ, .y0 = 0, .y1 = 224 },
		/* the ring: the item's name on BG3 (lines 9-36), the ring and the
		 * characters as sprites, the HUD; the dimmed field stays up top.
		 * $00CC = 0C (closing, the screen goes dark) has no rule */
		{ .when = { FIELD, PROBE(RING, 0x01), PROBE(MENU, 0x00) }, .kind = SPLIT_FULL,
		  .layers = LAYER_BG3 | LAYER_OBJ, .y0 = 0, .y1 = 224 },
		/* play, text boxes included: they're drawn at the top (lines
		 * 9-68), clear of the HUD. Before the sword the HUD's lines are
		 * empty, and the strip goes down empty */
		{ .when = { FIELD, PROBE(RING, 0x00) }, HUD },
	},
	.hudShowY0 = 199, .hudShowY1 = 217,
	/* the music starts with the Squaresoft logo (frame 122), with no noise
	 * before it */
	.bootMuteFrames = 0,
	/* the world map (Mode 7, on Flammie) comes late in the game: .map stays
	 * off for now (see README.md) */
};
