/* What a game's second screen is made of, as data: one Profile per game,
 * each in its own directory (games/<id>/profile.c, listed in games/games.c),
 * so no game's settings or code mix with another's. Find a game's values
 * with tools/harness. */
#ifndef SNESDS_PROFILES_H
#define SNESDS_PROFILES_H

#include <stdbool.h>
#include <stdint.h>

#include "core.h"

#define PROBES 4

/* a byte in memory: (byte & mask) == val, or == alt when hasAlt.
 * addr 0 ends a list; lists match when every probe matches */
typedef struct {
	uint32_t addr; /* full address, $7E0010 */
	uint8_t mask, val;
	bool hasAlt;
	uint8_t alt;
} MemProbe;

/* a byte equal to v (or to v or w) */
#define PROBE(a, v) { (a), 0xFF, (v), false, 0 }
#define PROBE_EITHER(a, v, w) { (a), 0xFF, (v), true, (w) }

/* buttons held for some frames (buttons 0: just wait) */
typedef struct {
	uint16_t buttons;
	int frames;
} Step;

#define STEPS 6

/* layers the second screen takes from the game's picture */
typedef enum {
	SPLIT_HUD,  /* a strip, drawn over the second screen's top */
	SPLIT_FULL, /* a whole screen (a menu), shown instead of the map */
} SplitKind;

typedef struct {
	MemProbe when[PROBES];
	SplitKind kind;
	uint8_t layers; /* LAYER_* */
	int y0, y1;     /* picture lines */
} SplitRule;

#define SPLITS 4

typedef struct {
	bool enabled;
	Step open[STEPS];          /* opens the map from normal play */
	MemProbe open2If[PROBES];  /* then, when these match... */
	int open2Delay;            /* ...after this many frames... */
	Step open2[STEPS];         /* ...this (another zoom level) */
	MemProbe live[PROBES];     /* normal play: a capture can start */
	MemProbe shown[PROBES];    /* the map is on screen */
	uint32_t room[4];          /* 16-bit values whose change starts a capture */
	int openWithin;            /* frames for `shown` to match, else no map here */
	int settleMax;             /* frames for the picture to settle */
	int stableFrames;          /* frames in a row that differ by under maxDiff pixels */
	int maxDiff;
	int minForeground;         /* pixels not bgColor, or the capture is empty */
	uint32_t bgColor;
	int animFrames, animStep;  /* frames recorded, frames between them */
	int refresh;               /* frames between captures in normal play */
} MapSpec;

typedef struct {
	const char* id;    /* its directory under games/ */
	const char* title;
	const char* shortTitle; /* for the game list's cards; NULL: title */
	const char* about;      /* what its second screen shows, for the game list */
	uint32_t crc32[4]; /* of the ROM without a copier header */
	SplitRule split[SPLITS]; /* first match wins */
	/* the lines of a SPLIT_HUD worth showing (its strip on the second
	 * screen); 0, 0: the rule's lines */
	int hudShowY0, hudShowY1;
	/* after power-on, the sound stays off for this many frames: for a game
	 * that makes a noise while it starts up, before anything is shown
	 * (0: none) */
	int bootMuteFrames;
	MapSpec map;
} Profile;

/* the profile for a ROM, or NULL */
const Profile* profile_find(const uint8_t* rom, size_t size);
/* the same by the CRC32 of the ROM without a copier header */
const Profile* profile_find_crc(uint32_t crc);
uint32_t rom_crc32(const uint8_t* rom, size_t size);

bool probes_match(Core* c, const MemProbe* p);
/* the split rule that matches now, or NULL */
const SplitRule* profile_split(const Profile* p, Core* c);

#endif
