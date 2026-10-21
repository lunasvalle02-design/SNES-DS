/* A game being played: its core, its second screen, its battery save and
 * its save states. Opened from the game list and closed back to it. */
#ifndef SNESDS_GAME_H
#define SNESDS_GAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "core.h"
#include "profiles.h"
#include "screen2.h"

#define STATE_SLOTS 4

typedef struct {
	char romPath[1024];
	char base[1100];      /* the state files' path without their extension */
	char srmPath[1100];
	uint8_t* rom;
	size_t romSize;
	Core* core;
	const Profile* profile;
	Screen2* s2;
	uint8_t* sram;        /* as last written */
	uint8_t* sramSeen;    /* as at the last check */
	size_t sramSize;
	long sramWrittenAt;   /* the frame it was last handed to the writer */
	struct SramWriter* sramWriter; /* writes it on a thread of its own */
	bool mapOn, splitOn;
	bool stateLoaded;     /* since power-on */
	long frame;           /* frames run */
} Game;

/* saveDir / stateDir NULL: beside the ROM */
Game* game_open(const char* romPath, const char* saveDir, const char* stateDir, bool map, bool split, bool threaded);
/* writes the battery save if it changed */
void game_close(Game* g);

const char* game_title(Game* g);
/* one frame with these buttons; its audio into samples (stereo) */
void game_frame(Game* g, uint16_t buttons, int16_t* samples, int frames);
/* write the battery save now if it changed (game_frame writes it a few
 * seconds after it stops changing, or once a minute if it never does) */
void game_flush_sram(Game* g);

void game_set_split(Game* g, bool on);
/* the hidden map core on or off (the second screen starts over) */
void game_set_map(Game* g, bool on);

/* slots 1..STATE_SLOTS; saving also keeps the picture, for the menu */
bool game_save_state(Game* g, int slot);
bool game_load_state(Game* g, int slot);
bool game_load_state_file(Game* g, const char* path);
/* when the slot was saved (0: empty) */
time_t game_state_time(Game* g, int slot);
/* the slot's picture, 256x224 0x00RRGGBB (malloc'd), or NULL */
uint32_t* game_state_picture(Game* g, int slot);

#endif
