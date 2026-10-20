/* The games snes-ds has a profile for. Each one lives in its own directory
 * (games/<id>/profile.c) and is only reached through this list: adding a
 * game is a new directory and one line here, and touches no other game. */
#include "profiles.h"

extern const Profile game_zelda_alttp;
extern const Profile game_super_metroid;
extern const Profile game_super_castlevania_iv;

const Profile* const games[] = {
	&game_zelda_alttp,
	&game_super_metroid,
	&game_super_castlevania_iv,
	NULL,
};
