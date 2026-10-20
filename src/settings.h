/* The app's settings, the same for every game, kept in a small text file
 * (key=value lines) so they survive between runs. */
#ifndef SNESDS_SETTINGS_H
#define SNESDS_SETTINGS_H

#include <stdbool.h>

typedef struct {
	bool wide;     /* the picture at 4:3 (else the SNES' own aspect) */
	bool swapped;  /* the game on the bottom screen */
	bool split;    /* the game's HUD and menus on the second screen */
	bool map;      /* the hidden core that captures the game's map */
	bool showFps;
	int volume;    /* 0-10 */
	int lang;      /* the interface's language, LANG_* (i18n.h) */
} Settings;

void settings_defaults(Settings* s);
/* missing file or keys: the defaults stay */
void settings_load(Settings* s, const char* path);
bool settings_save(const Settings* s, const char* path);

#endif
