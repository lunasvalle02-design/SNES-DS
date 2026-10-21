/* Super Mario World 2: Yoshi's Island. Everything snes-ds knows about this
 * game is here and in this directory (README.md: the memory map and what
 * each value was checked against; tests/: harness scripts). It runs on the
 * Super FX 2, which the core emulates (third_party/lakesnes/gsu.c). */
#include "profiles.h"

/* $7E0118 is the game mode: 01-07 the opening, 08-0A the title and the
 * file menu, 37-39 the story, 0E a level's title card (until a button),
 * 0F a level (paused or not), 12 and 15 baby Mario taken */
#define MODE 0x7E0118
/* $7E0B10 is 01 from the moment START pauses a level until START closes
 * the pause screen again */
#define PAUSED 0x7E0B10
/* $7E0B0F is the pause screen coming in: 01 to 13 over some 35 frames
 * after START, back down to 00 when it goes. The game fades out and the
 * screen stays black (01-0A); the panel fades in through 0B-0D (frames
 * 21-29, every pixel of it changing each frame) and holds still from 0E
 * on, while only the PAUSE title drops in */
#define PAUSE_IN 0x7E0B0F
/* $7E0D0F is the message box of a hint block: 09, 0B, 0D as it opens and
 * turns its pages, 00 when there's none */
#define MESSAGE 0x7E0D0F

const Profile game_super_mario_world_2_yoshis_island = {
	.id = "super_mario_world_2_yoshis_island",
	.title = "Super Mario World 2: Yoshi's Island",
	.shortTitle = "Yoshi's Island",
	.about = {
		[LANG_EN] = "The pause screen, always up to date, on the bottom screen: the level's stars, red coins and flowers, the points, lives, coins and items, without pausing.",
		[LANG_ES] = "La pantalla de pausa, siempre al día, en la pantalla de abajo: las estrellas, monedas rojas y flores del nivel, los puntos, las vidas, monedas y objetos, sin pausar.",
	},
	.crc32 = { 0xCF98DDAA }, /* USA, Rev 1 */
	/* no split: a level has no HUD of its own (the star countdown comes and
	 * goes over the game), and the pause screen stays where it's used, on
	 * the game screen, its items picked there */
	.map = {
		.enabled = true,
		/* the "map" is the pause screen (PAUSE in sprites, the rest BG1, on
		 * black), opened on the hidden core: it's the only place the game
		 * shows the level's counts, lives and items */
		.open = { { PAD_START, 4 } },
		/* in a level, not paused, no message box up (START does nothing
		 * then) */
		.live = { PROBE(MODE, 0x0F), PROBE(PAUSED, 0x00), PROBE(MESSAGE, 0x00) },
		/* the panel all up: $0B0F 10-13 (mask FC), not waiting for 13, as
		 * the title is left out anyway. Earlier would draw the black frames
		 * on the hidden core, which costs more than it saves */
		.shown = { PROBE(PAUSED, 0x01), { PAUSE_IN, 0xFC, 0x10, false, 0 } },
		/* coins, red coins, flowers, lives: each change shows at once. The
		 * stars are $03B6 in tenths (6D with 10 stars, 19 with 2), which
		 * changes every few frames while the countdown runs: they go by
		 * `refresh`, every 5 seconds (the countdown is on the game screen) */
		.room = { 0x7E037B, 0x7E03B4, 0x7E03B8, 0x7E0379 },
		.openWithin = 60, .settleMax = 60, .stableFrames = 3, .maxDiff = 200,
		.minForeground = 2000, .bgColor = 0x000000,
		.animFrames = 1, .animStep = 4,
		.refresh = 300,
		/* the title, PAUSE, left out: on the second screen the game isn't
		 * paused. It's the only thing in sprites above the counts (lines
		 * 21-49, as split 16 and splitbox show); the counts are BG1 */
		.hideLayers = LAYER_OBJ,
		.hideY0 = 0, .hideY1 = 50,
	},
};
