/* Super Mario World. Everything snes-ds knows about this game is here and
 * in this directory (README.md: the memory map and what each value was
 * checked against; tests/: harness scripts). */
#include "profiles.h"

/* $7E0100 is the game mode: 00-06 the Nintendo logo and the title's
 * fade-in, 07 the title, 08 file select, 0A players, 0B fading out to the
 * world map, 0C/0D loading and fading it in, 0E the world map, 0F/10 fading
 * out to a level, 11-13 loading and fading it in, 14 a level */
#define MODE 0x7E0100
/* $7E13D9 is what the world map is doing: 0C and 00 as it comes up, 03
 * Mario stands still, waiting for a button, 04 walking a path, 05 just
 * stopped on a tile (03 the next frame) */
#define WALK 0x7E13D9

/* the status bar: BG3 lines 0-36 (lives, the bonus stars, the item box,
 * time, coins, score). An IRQ under it changes BG3's scroll, so the level's
 * own BG3 (message boxes, tides, backgrounds) stays on the game screen.
 * The level's BG1/BG2 and its sprites are drawn behind it, so the game
 * screen keeps all its lines. The item in the box is a sprite (x 120-135,
 * lines 15-30, seen with $0DC2 = 01): it stays on the game screen, and the
 * box goes down empty; sprites can't go down with it, or Mario would too
 * when he jumps that high */
#define HUD .kind = SPLIT_HUD, .layers = LAYER_BG3, .y0 = 0, .y1 = 37

const Profile game_super_mario_world = {
	.id = "super_mario_world",
	.title = "Super Mario World",
	.about = {
		[LANG_EN] = "The world map, where you last stood on it, with the status bar (lives, time, coins and score) over it, on the bottom screen. The item in reserve stays on the top screen.",
		[LANG_ES] = "El mapa del mundo, donde estuviste por última vez, con la barra de estado (vidas, tiempo, monedas y puntos) encima, en la pantalla de abajo. El objeto de reserva se queda en la de arriba.",
	},
	.crc32 = { 0xB19ED489 }, /* USA, v1.0 */
	/* in a level; the modes in between (fading, loading, the world map)
	 * have no rule, so the second screen keeps the last status bar */
	.split = {
		{ .when = { PROBE(MODE, 0x14) }, HUD },
	},
	.hudShowY0 = 0, .hudShowY1 = 37,
	.map = {
		.enabled = true,
		/* the map is the world map itself, nothing to open: taken while
		 * Mario stands on it, it stays on the second screen through the
		 * level he goes into (a capture settles in ~5 frames: only the
		 * water and Mario's sprite move) */
		.live = { PROBE(MODE, 0x0E), PROBE(WALK, 0x03) },
		.shown = { PROBE(MODE, 0x0E) },
		/* Mario's position on the world map (X, Y, 16-bit) and his submap
		 * (01 Yoshi's Island); `refresh` catches the paths and tiles a
		 * cleared level opens while he stands still */
		.room = { 0x7E1F17, 0x7E1F19, 0x7E1F11 },
		.openWithin = 1, .settleMax = 200, .stableFrames = 4, .maxDiff = 400,
		.minForeground = 2000, .bgColor = 0x000000,
		.animFrames = 1, .animStep = 4,
		.refresh = 120,
	},
};
