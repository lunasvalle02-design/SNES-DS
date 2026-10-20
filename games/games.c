/* The games snes-ds has a profile for. Each one lives in its own directory
 * (games/<id>/profile.c) and is only reached through this list: adding a
 * game is a new directory and one line here, and touches no other game. */
#include "profiles.h"

extern const Profile game_zelda_alttp;

const Profile* const games[] = {
	&game_zelda_alttp,
	NULL,
};
