#include "settings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void settings_defaults(Settings* s) {
	*s = (Settings) { .wide = false, .swapped = false, .split = true, .map = true, .showFps = false, .volume = 10 };
}

void settings_load(Settings* s, const char* path) {
	FILE* f = fopen(path, "r");
	if (!f) return;
	char line[256];
	while (fgets(line, sizeof(line), f)) {
		char* eq = strchr(line, '=');
		if (!eq || line[0] == '#') continue;
		*eq = 0;
		int v = atoi(eq + 1);
		if (!strcmp(line, "aspect")) s->wide = !strncmp(eq + 1, "4:3", 3);
		else if (!strcmp(line, "swapped")) s->swapped = v;
		else if (!strcmp(line, "split")) s->split = v;
		else if (!strcmp(line, "map")) s->map = v;
		else if (!strcmp(line, "show_fps")) s->showFps = v;
		else if (!strcmp(line, "volume")) s->volume = v < 0 ? 0 : v > 10 ? 10 : v;
	}
	fclose(f);
}

bool settings_save(const Settings* s, const char* path) {
	FILE* f = fopen(path, "w");
	if (!f) return false;
	fprintf(f, "# snes-ds settings (also changed from the menu)\n");
	fprintf(f, "aspect=%s\nswapped=%d\nsplit=%d\nmap=%d\nshow_fps=%d\nvolume=%d\n", s->wide ? "4:3" : "native",
	        s->swapped, s->split, s->map, s->showFps, s->volume);
	return fclose(f) == 0;
}
