#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "romfile.h"
#include "stb_image.h"
#include "stb_image_write.h"

static bool write_file(const char* p, const void* d, size_t n) {
	FILE* f = fopen(p, "wb");
	if (!f) return false;
	bool ok = fwrite(d, 1, n, f) == n;
	return fclose(f) == 0 && ok;
}

/* DIR/name-of-rom, or beside the ROM */
static void base_path(char* out, size_t n, const char* rom, const char* dir) {
	const char* slash = strrchr(rom, '/');
	const char* bslash = strrchr(rom, '\\');
	if (bslash > slash) slash = bslash;
	const char* name = slash ? slash + 1 : rom;
	const char* dot = strrchr(name, '.');
	int nameLen = dot ? (int)(dot - name) : (int)strlen(name);
	if (dir) snprintf(out, n, "%s/%.*s", dir, nameLen, name);
	else snprintf(out, n, "%.*s", (int)(name - rom) + nameLen, rom);
}

Game* game_open(const char* romPath, const char* saveDir, const char* stateDir, bool map, bool split, bool threaded) {
	Game* g = calloc(1, sizeof(Game));
	snprintf(g->romPath, sizeof(g->romPath), "%s", romPath);
	g->rom = romfile_read(romPath, &g->romSize);
	if (!g->rom) {
		fprintf(stderr, "can't read %s\n", romPath);
		free(g);
		return NULL;
	}
	g->core = core_new();
	if (!g->core || !core_load_rom(g->core, g->rom, g->romSize)) {
		fprintf(stderr, "not a SNES ROM: %s\n", romPath);
		if (g->core) core_free(g->core);
		free(g->rom);
		free(g);
		return NULL;
	}
	core_set_threaded(g->core, threaded);
	g->profile = profile_find(g->rom, g->romSize);
	printf("%s: %s (crc32 %08X): %s\n", romPath, core_title(g->core), rom_crc32(g->rom, g->romSize),
	       g->profile ? g->profile->title : "no second-screen profile");

	char b[1100];
	base_path(b, sizeof(b), romPath, saveDir);
	snprintf(g->srmPath, sizeof(g->srmPath), "%s.srm", b);
	base_path(g->base, sizeof(g->base), romPath, stateDir ? stateDir : saveDir);

	/* battery save: loaded, then written whenever it changes */
	g->sramSize = core_sram(g->core, NULL);
	g->sram = g->sramSize ? calloc(1, g->sramSize) : NULL;
	if (g->sram) {
		size_t n;
		uint8_t* d = romfile_read(g->srmPath, &n);
		if (d && core_load_sram(g->core, d, n)) printf("loaded %s\n", g->srmPath);
		free(d);
		core_sram(g->core, g->sram);
	}
	g->mapOn = map;
	g->splitOn = split;
	g->s2 = screen2_new(g->profile, g->rom, g->romSize, map);
	if (!split) screen2_set_split(g->s2, false);
	fflush(stdout);
	return g;
}

void game_flush_sram(Game* g) {
	if (!g->sram) return;
	uint8_t* now = malloc(g->sramSize);
	core_sram(g->core, now);
	if (memcmp(now, g->sram, g->sramSize)) {
		memcpy(g->sram, now, g->sramSize);
		if (!write_file(g->srmPath, g->sram, g->sramSize)) fprintf(stderr, "can't save %s\n", g->srmPath);
	}
	free(now);
}

void game_close(Game* g) {
	if (!g) return;
	game_flush_sram(g);
	screen2_free(g->s2);
	core_free(g->core);
	free(g->rom);
	free(g->sram);
	free(g);
}

const char* game_title(Game* g) { return g->profile ? g->profile->title : core_title(g->core); }

void game_frame(Game* g, uint16_t buttons, int16_t* samples, int frames) {
	core_set_pad(g->core, 0, buttons);
	screen2_before_frame(g->s2, g->core);
	core_run_frame(g->core);
	screen2_after_frame(g->s2, g->core);
	core_audio(g->core, samples, frames);
	/* a game's start-up noise, if its profile says it has one (not after a
	 * state is loaded: that isn't a start-up) */
	if (g->profile && !g->stateLoaded && g->frame < g->profile->bootMuteFrames)
		memset(samples, 0, sizeof(int16_t) * 2 * frames);
	/* battery saves: written a few seconds after they change */
	if (++g->frame % 300 == 0) game_flush_sram(g);
}

void game_set_split(Game* g, bool on) {
	g->splitOn = on;
	screen2_set_split(g->s2, on);
}

void game_set_map(Game* g, bool on) {
	if (on == g->mapOn) return;
	g->mapOn = on;
	screen2_free(g->s2);
	g->s2 = screen2_new(g->profile, g->rom, g->romSize, on);
	if (!g->splitOn) screen2_set_split(g->s2, false);
}

static void slot_path(Game* g, int slot, const char* ext, char* out, size_t n) {
	snprintf(out, n, "%s.state%d%s", g->base, slot, ext);
}

bool game_save_state(Game* g, int slot) {
	char p[1200];
	slot_path(g, slot, "", p, sizeof(p));
	size_t n = core_state_size(g->core);
	uint8_t* d = malloc(n);
	bool ok = core_save_state(g->core, d) && write_file(p, d, n);
	free(d);
	printf(ok ? "saved %s\n" : "can't save %s\n", p);
	if (ok) {
		/* its picture, for the slots in the menu */
		const uint32_t* px = core_frame(g->core);
		uint8_t* rgb = malloc(CORE_W * CORE_H * 3);
		for (int i = 0; i < CORE_W * CORE_H; i++) {
			rgb[i * 3] = px[i] >> 16;
			rgb[i * 3 + 1] = px[i] >> 8;
			rgb[i * 3 + 2] = px[i];
		}
		slot_path(g, slot, ".png", p, sizeof(p));
		stbi_write_png(p, CORE_W, CORE_H, 3, rgb, CORE_W * 3);
		free(rgb);
	}
	fflush(stdout);
	return ok;
}

bool game_load_state_file(Game* g, const char* path) {
	size_t n;
	uint8_t* d = romfile_read(path, &n);
	bool ok = d && core_load_state(g->core, d, n);
	free(d);
	if (ok) {
		screen2_reset(g->s2);
		g->stateLoaded = true;
	}
	printf(ok ? "loaded %s\n" : "can't load state %s\n", path);
	fflush(stdout);
	return ok;
}

bool game_load_state(Game* g, int slot) {
	char p[1200];
	slot_path(g, slot, "", p, sizeof(p));
	return game_load_state_file(g, p);
}

time_t game_state_time(Game* g, int slot) {
	char p[1200];
	slot_path(g, slot, "", p, sizeof(p));
	struct stat st;
	return stat(p, &st) == 0 ? st.st_mtime : 0;
}

uint32_t* game_state_picture(Game* g, int slot) {
	char p[1200];
	slot_path(g, slot, ".png", p, sizeof(p));
	int w, h, ch;
	uint8_t* rgb = stbi_load(p, &w, &h, &ch, 3);
	if (!rgb) return NULL;
	uint32_t* px = NULL;
	if (w == CORE_W && h == CORE_H) {
		px = malloc(CORE_W * CORE_H * 4);
		for (int i = 0; i < CORE_W * CORE_H; i++) px[i] = rgb[i * 3] << 16 | rgb[i * 3 + 1] << 8 | rgb[i * 3 + 2];
	}
	stbi_image_free(rgb);
	return px;
}
