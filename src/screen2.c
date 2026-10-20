#include "screen2.h"

#include <stdlib.h>
#include <string.h>

#define BACKDROP 0x101418

struct Screen2 {
	const Profile* p;
	bool splitOn;
	MapWorker* worker;
	/* the map: the last good capture, dimmed when the game has none here */
	MapImage* map;
	bool haveMap, mapDim;
	/* what the game screen gave up this frame */
	const SplitRule* rule; /* in force for the frame that just ran */
	uint32_t hud[CORE_W * CORE_H];
	bool haveHud;
	int hudY0, hudY1;
	int cropHold; /* frames the game screen keeps gameShowY0/Y1 */
	uint32_t menu[CORE_W * CORE_H];
	bool menuUp;
	/* capture scheduling */
	uint32_t sinceCapture;
	uint16_t room[4];
	bool roomChanged;
	uint32_t ticks;
	uint32_t picture[CORE_W * CORE_H];
};

Screen2* screen2_new(const Profile* p, const uint8_t* rom, size_t romSize, bool mapCapture) {
	Screen2* s = calloc(1, sizeof(Screen2));
	if (!s) return NULL;
	s->p = p;
	s->splitOn = true;
	s->map = calloc(1, sizeof(MapImage));
	if (p && p->map.enabled && mapCapture) s->worker = mapworker_new(rom, romSize, &p->map);
	s->sinceCapture = 1000000;
	return s;
}

void screen2_free(Screen2* s) {
	if (!s) return;
	mapworker_free(s->worker);
	free(s->map);
	free(s);
}

void screen2_reset(Screen2* s) {
	s->haveHud = false;
	s->cropHold = 0;
	s->menuUp = false;
	s->roomChanged = true;
	s->sinceCapture = 1000000;
}

void screen2_set_split(Screen2* s, bool on) { s->splitOn = on; }

void screen2_before_frame(Screen2* s, Core* game) {
	s->rule = s->splitOn ? profile_split(s->p, game) : NULL;
	core_split_layers(game, 0, 0, 240);
	if (s->rule) core_split_layers(game, s->rule->layers, s->rule->y0, s->rule->y1);
}

static void schedule_capture(Screen2* s, Core* game) {
	const MapSpec* m = &s->p->map;
	for (int i = 0; i < 4 && m->room[i]; i++) {
		uint16_t v = core_peek(game, m->room[i]) | core_peek(game, m->room[i] + 1) << 8;
		if (v != s->room[i]) {
			s->room[i] = v;
			s->roomChanged = true;
		}
	}
	s->sinceCapture++;
	if (!probes_match(game, m->live)) return;
	if (!s->roomChanged && s->sinceCapture < (uint32_t)m->refresh) return;
	if (mapworker_request(s->worker, game)) {
		s->roomChanged = false;
		s->sinceCapture = 0;
	}
}

static void compose(Screen2* s) {
	uint32_t* out = s->picture;
	if (s->menuUp) {
		for (int i = 0; i < CORE_W * CORE_H; i++) out[i] = s->menu[i] >> 24 ? s->menu[i] & 0xFFFFFF : BACKDROP;
		return;
	}
	if (s->haveMap && s->map->frames) {
		int k = (s->ticks / s->map->step) % s->map->frames;
		const uint32_t* src = s->map->pixels[k];
		if (s->mapDim) {
			for (int i = 0; i < CORE_W * CORE_H; i++) out[i] = (src[i] >> 2) & 0x3F3F3F;
		} else {
			memcpy(out, src, sizeof(s->picture));
		}
	} else {
		for (int i = 0; i < CORE_W * CORE_H; i++) out[i] = BACKDROP;
	}
	if (s->haveHud) {
		for (int y = s->hudY0; y < s->hudY1; y++) {
			for (int x = 0; x < CORE_W; x++) {
				uint32_t c = s->hud[y * CORE_W + x];
				if (c >> 24) out[y * CORE_W + x] = c & 0xFFFFFF;
			}
		}
	}
}

void screen2_after_frame(Screen2* s, Core* game) {
	s->ticks++;
	const SplitRule* r = s->rule;
	s->menuUp = false;
	/* the crop stays through a few frames of no rule (a game's screen-off
	 * frames between two HUD screens), so the picture doesn't jump */
	if (r && r->kind == SPLIT_HUD) s->cropHold = 30;
	else if (r || !s->splitOn) s->cropHold = 0;
	else if (s->cropHold > 0) s->cropHold--;
	if (r) {
		const uint32_t* f = core_split_frame(game);
		if (r->kind == SPLIT_FULL) {
			memcpy(s->menu, f, sizeof(s->menu));
			s->menuUp = true;
		} else {
			memcpy(s->hud, f, sizeof(s->hud));
			s->haveHud = true;
			bool show = s->p->hudShowY1 > s->p->hudShowY0;
			s->hudY0 = show ? s->p->hudShowY0 : r->y0 < 0 ? 0 : r->y0;
			s->hudY1 = show ? s->p->hudShowY1 : r->y1 > CORE_H ? CORE_H : r->y1;
		}
	}
	if (s->worker) {
		schedule_capture(s, game);
		MapResult res;
		if (mapworker_poll(s->worker, &res, s->map)) {
			s->sinceCapture = 0; /* the next one counts from now */
			if (res == MAPCAP_OK) {
				s->haveMap = true;
				s->mapDim = false;
			} else if (res == MAPCAP_NO_MAP) {
				s->mapDim = true;
			}
		}
	}
	compose(s);
}

const uint32_t* screen2_picture(Screen2* s) { return s->picture; }

void screen2_game_lines(Screen2* s, int* y0, int* y1) {
	*y0 = 0;
	*y1 = CORE_H;
	const Profile* p = s->p;
	if (!p || s->cropHold <= 0 || p->gameShowY1 <= p->gameShowY0) return;
	*y0 = p->gameShowY0 < 0 ? 0 : p->gameShowY0;
	*y1 = p->gameShowY1 > CORE_H ? CORE_H : p->gameShowY1;
	if (*y1 <= *y0) {
		*y0 = 0;
		*y1 = CORE_H;
	}
}

bool screen2_map_stats(Screen2* s, int* count, double* avgMs, double* maxMs) {
	if (!s->worker) return false;
	mapworker_stats(s->worker, count, avgMs, maxMs);
	return true;
}
