/* The app's settings, the same for every game, kept in a small text file
 * (key=value lines) so they survive between runs. */
#ifndef SNESDS_SETTINGS_H
#define SNESDS_SETTINGS_H

#include <stdbool.h>

/* the controls the player moved (pad.h's BIND_* order: B A Y X L R Start
 * Select, then the menu): pad_set_bind's text, "" for the default */
#define SETTINGS_BINDS 9
#define SETTINGS_BIND_LEN 32

typedef struct {
	bool wide;     /* the picture at 4:3 (else the SNES' own aspect) */
	bool swapped;  /* the game on the bottom screen */
	bool split;    /* the game's HUD and menus on the second screen */
	bool map;      /* the hidden core that captures the game's map */
	bool showFps;
	int volume;    /* 0-10 */
	int lang;      /* the interface's language, LANG_* (i18n.h) */
	char binds[SETTINGS_BINDS][SETTINGS_BIND_LEN];
} Settings;

void settings_defaults(Settings* s);
/* missing file or keys: the defaults stay */
void settings_load(Settings* s, const char* path);
bool settings_save(const Settings* s, const char* path);

#endif
