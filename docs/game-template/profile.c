/* <Full title>. Everything snes-ds knows about this game is here and in this
 * directory (README.md: the memory map and what each value was checked
 * against; tests/: harness scripts).
 *
 * Template: copy docs/game-template/ to games/<id>/, replace every <...>,
 * delete what the game doesn't use (a zero or missing field is "off"), and
 * say for every value how it was found. See AGENTS.md. */
#include "profiles.h"

const Profile game_<id> = {
	.id = "<id>",                 /* the directory's name */
	.title = "<Full title>",      /* as on the box, in English */
	.shortTitle = "<Short>",      /* for the game list's cards; leave out if the title is short */
	.about = {
		[LANG_EN] = "<What the second screen shows, one sentence.>",
		[LANG_ES] = "<Lo que muestra la segunda pantalla, una frase.>",
	},
	/* of the ROM without a copier header: the harness prints it at start */
	.crc32 = { 0x00000000 }, /* <region, version> */

	/* what goes to the second screen, by what memory says is on screen.
	 * The first rule that matches wins; up to SPLITS (4). Find the layers
	 * with `layershot`, the lines with `splitbox`, the flags with `watch` */
	.split = {
		/* <when, and why these layers and lines> */
		{ .when = { PROBE(0x7E0000, 0x00) }, .kind = SPLIT_HUD, .layers = LAYER_BG3, .y0 = 0, .y1 = 32 },
		/* a whole-screen menu: shown instead of the map */
		{ .when = { PROBE(0x7E0000, 0x00) }, .kind = SPLIT_FULL, .layers = LAYER_BG3, .y0 = 0, .y1 = 224 },
	},
	/* the lines of a SPLIT_HUD worth showing over the map (0, 0: the rule's) */
	.hudShowY0 = 0, .hudShowY1 = 0,

	/* power-on noise to mute, in frames (see games/zelda_alttp/README.md) */
	.bootMuteFrames = 0,

	/* the game's own map, opened on a hidden core; leave .enabled false if
	 * the game has none */
	.map = {
		.enabled = false,
		.open = { { PAD_START, 4 } },         /* presses that open it from play */
		.live = { PROBE(0x7E0000, 0x00) },   /* normal play: a capture may start */
		.shown = { PROBE(0x7E0000, 0x00) },  /* the map is on screen */
		.room = { 0x7E0000 },                 /* 16-bit values whose change starts a capture */
		.openWithin = 40, .settleMax = 200, .stableFrames = 4, .maxDiff = 400,
		.minForeground = 2000, .bgColor = 0x000000,
		.animFrames = 1, .animStep = 4,
		.refresh = 90,
		/* layers left out of the capture where the HUD strip goes over it,
		 * when the map has text of its own there (leave out if not) */
		.hideLayers = 0, .hideY0 = 0, .hideY1 = 0,
	},
};
