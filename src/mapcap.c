#include "mapcap.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef __linux__
#include <sys/resource.h>
#endif

static double now_ms(void) {
	struct timespec t;
	timespec_get(&t, TIME_UTC);
	return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

static void run_steps(Core* c, const Step* s) {
	for (int i = 0; i < STEPS && s[i].frames; i++) {
		core_set_pad(c, 0, s[i].buttons);
		for (int f = 0; f < s[i].frames; f++) core_run_frame(c);
	}
	core_set_pad(c, 0, 0);
}

/* run up to max frames until the probes match; frames it took, or -1 */
static int run_until(Core* c, const MemProbe* p, int max) {
	for (int i = 0; i <= max; i++) {
		if (probes_match(c, p)) return i;
		core_run_frame(c);
	}
	return -1;
}

static int frame_diff(const uint32_t* a, const uint32_t* b) {
	int n = 0;
	for (int i = 0; i < CORE_W * CORE_H; i++) n += a[i] != b[i];
	return n;
}

static int foreground(const MapSpec* m, const uint32_t* f) {
	int n = 0;
	for (int i = 0; i < CORE_W * CORE_H; i++) n += f[i] != m->bgColor;
	return n;
}

/* run until the picture holds still (blinking markers allowed). A (nearly)
 * empty picture doesn't count: a map can sit on a black screen for a while
 * before it's drawn */
static bool settle(Core* c, const MapSpec* m, uint32_t* prev) {
	int still = 0;
	memcpy(prev, core_frame(c), sizeof(uint32_t) * CORE_W * CORE_H);
	for (int i = 0; i < m->settleMax; i++) {
		core_run_frame(c);
		const uint32_t* now = core_frame(c);
		if (!probes_match(c, m->shown)) return false;
		bool same = frame_diff(prev, now) <= m->maxDiff && foreground(m, now) >= m->minForeground;
		still = same ? still + 1 : 0;
		memcpy(prev, now, sizeof(uint32_t) * CORE_W * CORE_H);
		if (still >= m->stableFrames) return true;
	}
	return false;
}

MapResult mapcap_capture(Core* hidden, const MapSpec* m, const uint8_t* state, size_t size, MapImage* out) {
	out->frames = 0;
	if (!core_load_state(hidden, state, size)) return MAPCAP_NOT_LIVE;
	if (!probes_match(hidden, m->live)) return MAPCAP_NOT_LIVE;
	/* the hidden core draws nothing but captures, so this can stay set */
	core_hide_layers(hidden, 0, 0, 240);
	if (m->hideLayers) core_hide_layers(hidden, m->hideLayers, m->hideY0, m->hideY1);
	/* nothing to look at until the map is up: those frames aren't drawn */
	core_set_render(hidden, false);
	run_steps(hidden, m->open);
	bool opened = run_until(hidden, m->shown, m->openWithin) >= 0;
	core_set_render(hidden, true);
	if (!opened) return MAPCAP_NO_MAP;
	uint32_t* prev = malloc(sizeof(uint32_t) * CORE_W * CORE_H);
	MapResult r = MAPCAP_OK;
	if (!settle(hidden, m, prev)) {
		r = MAPCAP_UNSETTLED;
		goto done;
	}
	if (m->open2[0].frames && probes_match(hidden, m->open2If)) {
		core_set_render(hidden, false);
		for (int i = 0; i < m->open2Delay; i++) core_run_frame(hidden);
		run_steps(hidden, m->open2);
		/* the zoom takes a while to start: don't take the old view as settled */
		for (int i = 0; i < 8; i++) core_run_frame(hidden);
		core_set_render(hidden, true);
		if (!probes_match(hidden, m->shown) || !settle(hidden, m, prev)) {
			r = MAPCAP_UNSETTLED;
			goto done;
		}
	}
	if (foreground(m, core_frame(hidden)) < m->minForeground) {
		r = MAPCAP_EMPTY;
		goto done;
	}
	int n = m->animFrames < 1 ? 1 : m->animFrames > MAP_MAX_FRAMES ? MAP_MAX_FRAMES : m->animFrames;
	for (int k = 0; k < n; k++) {
		if (k) {
			for (int i = 0; i < m->animStep; i++) core_run_frame(hidden);
		}
		memcpy(out->pixels[k], core_frame(hidden), sizeof(out->pixels[k]));
	}
	out->frames = n;
	out->step = m->animStep < 1 ? 1 : m->animStep;
done:
	free(prev);
	return r;
}

const char* mapcap_result_name(MapResult r) {
	switch (r) {
	case MAPCAP_OK: return "ok";
	case MAPCAP_NOT_LIVE: return "not in play";
	case MAPCAP_NO_MAP: return "no map here";
	case MAPCAP_UNSETTLED: return "never settled";
	case MAPCAP_EMPTY: return "empty";
	}
	return "?";
}

struct MapWorker {
	Core* hidden;
	const MapSpec* spec;
	pthread_t thread;
	pthread_mutex_t lock;
	pthread_cond_t wake;
	bool quit;
	/* request: the game's state, handed over by mapworker_request */
	uint8_t* state;
	size_t stateSize, stateCap;
	bool pending, busy;
	/* result */
	bool done;
	MapResult result;
	MapImage* img;  /* the worker's */
	MapImage* last; /* the last good one, handed out by poll */
	int count;
	double totalMs, maxMs;
};

static void* worker_main(void* arg) {
	MapWorker* w = arg;
#ifdef __linux__
	/* behind the game and its drawing for the CPU (on Linux this is the
	 * calling thread's niceness only) */
	setpriority(PRIO_PROCESS, 0, 10);
#endif
	pthread_mutex_lock(&w->lock);
	for (;;) {
		while (!w->pending && !w->quit) pthread_cond_wait(&w->wake, &w->lock);
		if (w->quit) break;
		w->pending = false;
		w->busy = true;
		pthread_mutex_unlock(&w->lock);
		double t0 = now_ms();
		MapResult r = mapcap_capture(w->hidden, w->spec, w->state, w->stateSize, w->img);
		double ms = now_ms() - t0;
		pthread_mutex_lock(&w->lock);
		w->count++;
		w->totalMs += ms;
		if (ms > w->maxMs) w->maxMs = ms;
		if (r == MAPCAP_OK) {
			MapImage* t = w->last;
			w->last = w->img;
			w->img = t;
		}
		w->result = r;
		w->done = true;
		w->busy = false;
	}
	pthread_mutex_unlock(&w->lock);
	return NULL;
}

MapWorker* mapworker_new(const uint8_t* rom, size_t romSize, const MapSpec* m) {
	MapWorker* w = calloc(1, sizeof(MapWorker));
	if (!w) return NULL;
	w->spec = m;
	w->hidden = core_new();
	w->img = calloc(1, sizeof(MapImage));
	w->last = calloc(1, sizeof(MapImage));
	if (!w->hidden || !w->img || !w->last || !core_load_rom(w->hidden, rom, romSize)) {
		core_free(w->hidden);
		free(w->img);
		free(w->last);
		free(w);
		return NULL;
	}
	pthread_mutex_init(&w->lock, NULL);
	pthread_cond_init(&w->wake, NULL);
	pthread_create(&w->thread, NULL, worker_main, w);
	return w;
}

void mapworker_free(MapWorker* w) {
	if (!w) return;
	pthread_mutex_lock(&w->lock);
	w->quit = true;
	pthread_cond_signal(&w->wake);
	pthread_mutex_unlock(&w->lock);
	pthread_join(w->thread, NULL);
	pthread_mutex_destroy(&w->lock);
	pthread_cond_destroy(&w->wake);
	core_free(w->hidden);
	free(w->state);
	free(w->img);
	free(w->last);
	free(w);
}

bool mapworker_request(MapWorker* w, Core* game) {
	pthread_mutex_lock(&w->lock);
	if (w->busy || w->pending) {
		pthread_mutex_unlock(&w->lock);
		return false;
	}
	size_t n = core_state_size(game);
	if (n > w->stateCap) {
		free(w->state);
		w->state = malloc(n);
		w->stateCap = w->state ? n : 0;
	}
	if (w->state) {
		core_save_state(game, w->state);
		w->stateSize = n;
		w->pending = true;
		pthread_cond_signal(&w->wake);
	}
	pthread_mutex_unlock(&w->lock);
	return w->state != NULL;
}

void mapworker_stats(MapWorker* w, int* count, double* avgMs, double* maxMs) {
	pthread_mutex_lock(&w->lock);
	*count = w->count;
	*avgMs = w->count ? w->totalMs / w->count : 0;
	*maxMs = w->maxMs;
	pthread_mutex_unlock(&w->lock);
}

bool mapworker_busy(MapWorker* w) {
	pthread_mutex_lock(&w->lock);
	bool b = w->busy || w->pending;
	pthread_mutex_unlock(&w->lock);
	return b;
}

bool mapworker_poll(MapWorker* w, MapResult* result, MapImage* img) {
	pthread_mutex_lock(&w->lock);
	bool done = w->done;
	if (done) {
		w->done = false;
		*result = w->result;
		if (w->result == MAPCAP_OK && img) memcpy(img, w->last, sizeof(MapImage));
	}
	pthread_mutex_unlock(&w->lock);
	return done;
}
