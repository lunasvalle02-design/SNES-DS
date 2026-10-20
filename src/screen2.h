/* The second screen: what it shows for the game being played (its map, its
 * HUD, its menu), from the game's profile. Kept apart from SDL so the
 * harness can drive it too. */
#ifndef SNESDS_SCREEN2_H
#define SNESDS_SCREEN2_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core.h"
#include "mapcap.h"
#include "profiles.h"

typedef struct Screen2 Screen2;

/* rom is needed for the hidden map core; profile may be NULL */
Screen2* screen2_new(const Profile* p, const uint8_t* rom, size_t romSize, bool mapCapture);
void screen2_free(Screen2* s);

/* around each game frame: before sets the game's layer split for the frame
 * about to run, after collects it and updates the picture */
void screen2_before_frame(Screen2* s, Core* game);
void screen2_after_frame(Screen2* s, Core* game);

/* the picture, 256x224 0x00RRGGBB: the menu while it's up, else the map
 * with the HUD over it where the game has it */
const uint32_t* screen2_picture(Screen2* s);

/* the lines of the game's picture its screen shows now (the profile's
 * gameShowY0/Y1 while its HUD is down here, else all of them) */
void screen2_game_lines(Screen2* s, int* y0, int* y1);

/* map captures so far and their times (ms); false without a map core */
bool screen2_map_stats(Screen2* s, int* count, double* avgMs, double* maxMs);

/* the state was swapped under it (a save state was loaded) */
void screen2_reset(Screen2* s);

/* turn the layer split off (everything back on the game screen) */
void screen2_set_split(Screen2* s, bool on);

#endif
