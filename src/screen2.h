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

/* everything in one 256x224 0x00RRGGBB picture, the HUD over the map */
const uint32_t* screen2_picture(Screen2* s);

/* the parts, for a layout with room to show them apart (all 256x224):
 * the menu (0xAARRGGBB) while it's up, else NULL; the map's frame to show
 * (0x00RRGGBB; *dim when the game has none where the player is) or NULL;
 * the HUD (0xAARRGGBB, lines [*y0, *y1) worth showing) or NULL */
const uint32_t* screen2_menu(Screen2* s);
const uint32_t* screen2_map(Screen2* s, bool* dim);
const uint32_t* screen2_hud(Screen2* s, int* y0, int* y1);
uint32_t screen2_backdrop(void);

/* the state was swapped under it (a save state was loaded) */
void screen2_reset(Screen2* s);

/* turn the layer split off (everything back on the game screen) */
void screen2_set_split(Screen2* s, bool on);

#endif
