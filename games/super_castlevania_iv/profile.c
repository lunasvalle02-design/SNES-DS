/* Super Castlevania IV. Everything snes-ds knows about this game is here and
 * in this directory (README.md: the memory map and what each value was
 * checked against; tests/: harness scripts). */
#include "profiles.h"

/* $7E0032 is the main mode: 02 the Konami logo, the title and the
 * prologue, 01 play select, 03 fading out of it, 04 name entry and the game
 * (the castle gate's scene included) */
#define IN_GAME PROBE(0x7E0032, 0x04)
/* $7E0070 is the state within a stage: 00 name entry and the gate's scene,
 * 01-03 loading (the screen off), 04 fading in, 05 play (pausing doesn't
 * change it), 06 dying, 0A hit (knocked back), 0C falling into a pit */
#define STAGE 0x7E0070

/* the HUD: BG3 lines 0-31 (score, the health bars, the box, block, hearts,
 * lives, time) and the sprites on them, the subweapon in the box. The
 * stage's layers start at line 32; its sprites stay below it in play (none
 * seen above), but one that went up there would go down with the HUD */
#define HUD .kind = SPLIT_HUD, .layers = LAYER_BG3 | LAYER_OBJ, .y0 = 0, .y1 = 32

const Profile game_super_castlevania_iv = {
	.id = "super_castlevania_iv",
	.title = "Super Castlevania IV",
	.about = {
		[LANG_EN] = "The HUD (score, health, hearts, time and the subweapon) on the bottom screen.",
		[LANG_ES] = "El HUD (puntos, vida, corazones, tiempo y el arma secundaria) en la pantalla de abajo.",
	},
	.crc32 = { 0xB64FFB12 }, /* USA, v1.0 */
	/* while the HUD is drawn; state 03 (and the first frames of 04) load
	 * with the screen off and have no rule, so the second screen keeps the
	 * last HUD instead of going blank */
	.split = {
		{ .when = { IN_GAME, PROBE_EITHER(STAGE, 0x05, 0x06) }, HUD },
		{ .when = { IN_GAME, PROBE_EITHER(STAGE, 0x04, 0x0A) }, HUD },
		{ .when = { IN_GAME, PROBE(STAGE, 0x0C) }, HUD },
	},
	.hudShowY0 = 0, .hudShowY1 = 32,
	/* the game has no map: .map stays off */
};
