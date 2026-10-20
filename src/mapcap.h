/* Map capture: a hidden second core gets a copy of the game, opens the
 * game's own map there and records it (like gba-ds' mapcap.c). The player's
 * game never sees any of it. */
#ifndef SNESDS_MAPCAP_H
#define SNESDS_MAPCAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core.h"
#include "profiles.h"

#define MAP_MAX_FRAMES 8

typedef struct {
	int frames; /* 0: no map here */
	int step;   /* game frames per animation frame */
	uint32_t pixels[MAP_MAX_FRAMES][CORE_W * CORE_H];
} MapImage;

typedef enum {
	MAPCAP_OK,
	MAPCAP_NOT_LIVE,   /* the state wasn't in normal play */
	MAPCAP_NO_MAP,     /* the map didn't open (none here) */
	MAPCAP_UNSETTLED,  /* it opened but never held still */
	MAPCAP_EMPTY,      /* it held still but showed (nearly) nothing */
} MapResult;

/* run a capture on `hidden` (same game loaded) from a save state */
MapResult mapcap_capture(Core* hidden, const MapSpec* m, const uint8_t* state, size_t size, MapImage* out);
const char* mapcap_result_name(MapResult r);

/* the same in a background thread */
typedef struct MapWorker MapWorker;
MapWorker* mapworker_new(const uint8_t* rom, size_t romSize, const MapSpec* m);
void mapworker_free(MapWorker* w);
/* hand it the game's state; false while it's busy with the last one */
bool mapworker_request(MapWorker* w, Core* game);
bool mapworker_busy(MapWorker* w);
/* a capture finished since the last call: its result (and *img when OK) */
bool mapworker_poll(MapWorker* w, MapResult* result, MapImage* img);

#endif
