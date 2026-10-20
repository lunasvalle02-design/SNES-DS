/* snes-ds: the app. One window over both screens: the game on one, the
 * second screen (screen2.c) on the other. On ROCKNIX (sway) the window goes
 * fullscreen and is spanned over both panels.
 *
 *   snes-ds [options] ROM             (.sfc, .smc, or a .zip with one)
 *   snes-ds [options] --find-in=DIR   the first game in DIR with a profile
 *     --print-rom                with --find-in: print the game's path, quit
 *     --layout=auto|stack|side   (auto: side by side on a wide window, as
 *                                when spanning two panels; else stacked)
 *     --aspect=native|4:3        the SNES' own 256x224 at whole-number
 *                                scales (default), or stretched to 4:3 like
 *                                on a TV, filling the RG DS Plus' panels
 *     --scale=N                  windowed: the picture's scale (default 2)
 *     --screen=WxH               windowed: each screen's size
 *     --fullscreen / --windowed  (default: fullscreen under sway or KMSDRM)
 *     --save-dir=DIR             battery saves (default: beside the ROM)
 *     --state-dir=DIR            save states (default: as --save-dir)
 *     --no-map                   no hidden map core
 *     --no-threads               draw the picture on the emulation's thread
 *                                (default: a thread of its own)
 *     --no-audio
 *     --stats=N                  log speed every N seconds (default 10, 0 off)
 *     --frames=N                 quit after N frames (for testing)
 *     --shot=FILE.png            with --frames: the window before quitting
 *     --load-state=FILE
 *
 * Keys: arrows, X = A, Z = B, S = X, A = Y, Q = L, W = R, Enter = Start,
 * Right Shift = Select. F2 swap screens, F3 HUD split on/off, F4 aspect,
 * F5 save state, F7 load state, Tab fast forward, F11 fullscreen, Esc quit.
 * Controllers by button position; shortcuts with Select: see pad.h.
 */
#include <SDL.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "core.h"
#include "pad.h"
#include "profiles.h"
#include "romfile.h"
#include "screen2.h"
#include "video.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define SAMPLE_RATE 48000
#define SAMPLES_PER_FRAME 800

static struct {
	const char* romPath;
	const char* findIn;
	const char* saveDir;
	const char* stateDir;
	const char* loadState;
	const char* shot;
	Layout layout;
	bool wide, noMap, noAudio, printRom, noThreads;
	int fullscreen; /* -1: decide */
	int scale, screenW, screenH, stats;
	long frames;
} opt = { .layout = LAYOUT_AUTO, .fullscreen = -1, .scale = 2, .stats = 10 };

static volatile sig_atomic_t stopRequested;
static void on_signal(int sig) {
	(void)sig;
	stopRequested = 1;
}

static bool arg(const char* a, const char* name, const char** val) {
	size_t n = strlen(name);
	if (strncmp(a, name, n) || a[n] != '=') return false;
	*val = a + n + 1;
	return true;
}

static void parse_args(int argc, char** argv) {
	for (int i = 1; i < argc; i++) {
		const char* a = argv[i];
		const char* v;
		if (arg(a, "--layout", &v)) opt.layout = !strcmp(v, "side") ? LAYOUT_SIDE : !strcmp(v, "stack") ? LAYOUT_STACK : LAYOUT_AUTO;
		else if (arg(a, "--aspect", &v)) opt.wide = !strcmp(v, "4:3");
		else if (arg(a, "--scale", &v)) opt.scale = atoi(v);
		else if (arg(a, "--screen", &v)) sscanf(v, "%dx%d", &opt.screenW, &opt.screenH);
		else if (arg(a, "--save-dir", &v)) opt.saveDir = v;
		else if (arg(a, "--state-dir", &v)) opt.stateDir = v;
		else if (arg(a, "--find-in", &v)) opt.findIn = v;
		else if (arg(a, "--stats", &v)) opt.stats = atoi(v);
		else if (arg(a, "--frames", &v)) opt.frames = atol(v);
		else if (arg(a, "--shot", &v)) opt.shot = v;
		else if (arg(a, "--load-state", &v)) opt.loadState = v;
		else if (!strcmp(a, "--fullscreen")) opt.fullscreen = 1;
		else if (!strcmp(a, "--windowed")) opt.fullscreen = 0;
		else if (!strcmp(a, "--no-map")) opt.noMap = true;
		else if (!strcmp(a, "--print-rom")) opt.printRom = true;
		else if (!strcmp(a, "--no-threads")) opt.noThreads = true;
		else if (!strcmp(a, "--no-audio")) opt.noAudio = true;
		else if (a[0] == '-' && a[1] == '-') fprintf(stderr, "unknown option %s\n", a);
		else opt.romPath = a;
	}
	if (opt.scale < 1) opt.scale = 1;
	if (opt.screenW <= 0 || opt.screenH <= 0) {
		opt.screenH = CORE_H * opt.scale;
		opt.screenW = opt.wide ? (opt.screenH * 4 + 2) / 3 : CORE_W * opt.scale;
	}
	if (!opt.stateDir) opt.stateDir = opt.saveDir;
}

static bool write_file(const char* p, const void* d, size_t n) {
	FILE* f = fopen(p, "wb");
	if (!f) return false;
	bool ok = fwrite(d, 1, n, f) == n;
	return fclose(f) == 0 && ok;
}

/* DIR/name-of-rom.EXT, or beside the ROM */
static void save_path(char* out, size_t n, const char* dir, const char* ext) {
	const char* base = opt.romPath;
	const char* slash = strrchr(base, '/');
	const char* bslash = strrchr(base, '\\');
	if (bslash > slash) slash = bslash;
	const char* name = slash ? slash + 1 : base;
	const char* dot = strrchr(name, '.');
	int nameLen = dot ? (int)(dot - name) : (int)strlen(name);
	if (dir) snprintf(out, n, "%s/%.*s%s", dir, nameLen, name, ext);
	else snprintf(out, n, "%.*s%s", (int)(name - base) + nameLen, base, ext);
}

/* under sway, one fullscreen window spans every output only when asked */
static void sway_span(void) {
#ifndef _WIN32
	if (!getenv("SWAYSOCK")) return;
	char cmd[128];
	snprintf(cmd, sizeof(cmd), "swaymsg '[pid=%d] fullscreen enable global' >/dev/null 2>&1", (int)getpid());
	if (system(cmd) != 0) fprintf(stderr, "note: swaymsg fullscreen global failed\n");
#endif
}

static double ms_since(uint64_t t0) {
	return (double)(SDL_GetPerformanceCounter() - t0) * 1000.0 / SDL_GetPerformanceFrequency();
}

int main(int argc, char** argv) {
	parse_args(argc, argv);
	char found[1024];
	if (!opt.romPath && opt.findIn) {
		if (!romfile_find(opt.findIn, found, sizeof(found))) {
			fprintf(stderr, "no game with a second-screen profile in %s\n", opt.findIn);
			return 1;
		}
		opt.romPath = found;
		if (opt.printRom) {
			puts(found);
			return 0;
		}
	}
	if (!opt.romPath) {
		fprintf(stderr, "usage: snes-ds [options] ROM (see src/main.c)\n");
		return 2;
	}
	size_t romSize;
	uint8_t* rom = romfile_read(opt.romPath, &romSize);
	if (!rom) {
		fprintf(stderr, "can't read %s\n", opt.romPath);
		return 1;
	}
	Core* game = core_new();
	if (!game || !core_load_rom(game, rom, romSize)) {
		fprintf(stderr, "not a SNES ROM: %s\n", opt.romPath);
		return 1;
	}
	core_set_threaded(game, !opt.noThreads);
	const Profile* profile = profile_find(rom, romSize);
	printf("%s: %s (crc32 %08X): %s\n", opt.romPath, core_title(game), rom_crc32(rom, romSize),
	       profile ? profile->title : "no second-screen profile");

	/* battery save: loaded, then written whenever it changes */
	char srmPath[1024];
	save_path(srmPath, sizeof(srmPath), opt.saveDir, ".srm");
	size_t sramSize = core_sram(game, NULL);
	uint8_t* sram = sramSize ? calloc(1, sramSize) : NULL;
	if (sram) {
		size_t n;
		uint8_t* d = romfile_read(srmPath, &n);
		if (d && core_load_sram(game, d, n)) printf("loaded %s\n", srmPath);
		free(d);
		core_sram(game, sram);
	}
	if (opt.loadState) {
		size_t n;
		uint8_t* d = romfile_read(opt.loadState, &n);
		if (!d || !core_load_state(game, d, n)) fprintf(stderr, "can't load state %s\n", opt.loadState);
		free(d);
	}
	char statePath[1024];
	save_path(statePath, sizeof(statePath), opt.stateDir, ".state1");

	Screen2* s2 = screen2_new(profile, rom, romSize, !opt.noMap);

#ifndef _WIN32
	setenv("SDL_VIDEO_WAYLAND_WMCLASS", "snes-ds", 0);
#endif
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
		fprintf(stderr, "SDL: %s\n", SDL_GetError());
		return 1;
	}
	signal(SIGINT, on_signal);
	signal(SIGTERM, on_signal);
	const char* driver = SDL_GetCurrentVideoDriver();
	if (opt.fullscreen < 0) opt.fullscreen = getenv("SWAYSOCK") || (driver && !strcmp(driver, "kmsdrm"));
	bool side = opt.layout == LAYOUT_SIDE;
	int winW = side ? opt.screenW * 2 : opt.screenW, winH = side ? opt.screenH : opt.screenH * 2;
	char title[128];
	snprintf(title, sizeof(title), "snes-ds - %s", profile ? profile->title : core_title(game));
	SDL_Window* win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, winW, winH,
	                                   SDL_WINDOW_RESIZABLE | (opt.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));
	SDL_Renderer* ren = win ? SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE) : NULL;
	if (!ren) ren = win ? SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE) : NULL;
	if (!ren) {
		fprintf(stderr, "SDL: %s\n", SDL_GetError());
		return 1;
	}
	SDL_RendererInfo ri;
	SDL_GetRendererInfo(ren, &ri);
	printf("video: %s, renderer %s, %s\n", driver, ri.name, opt.fullscreen ? "fullscreen" : "windowed");
	ScreenTex texGame, texSecond;
	screen_tex_init(ren, &texGame);
	screen_tex_init(ren, &texSecond);

	SDL_AudioDeviceID audio = 0;
	if (!opt.noAudio) {
		SDL_AudioSpec want = { .freq = SAMPLE_RATE, .format = AUDIO_S16SYS, .channels = 2, .samples = 512 }, have;
		audio = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
		if (audio) SDL_PauseAudioDevice(audio, 0);
		else fprintf(stderr, "no audio: %s\n", SDL_GetError());
	}
	SDL_GameController* gc = NULL;
	for (int i = 0; i < SDL_NumJoysticks() && !gc; i++) {
		if (SDL_IsGameController(i)) gc = SDL_GameControllerOpen(i);
	}
	if (gc) printf("controller: %s\n", SDL_GameControllerName(gc));
	fflush(stdout);

	Pad pad = { 0 };
	bool quit = false, swapped = false, fastToggle = false, splitOn = true;
	int16_t samples[SAMPLES_PER_FRAME * 2];
	uint64_t freq = SDL_GetPerformanceFrequency(), next = SDL_GetPerformanceCounter();
	const double frameTime = 1.0 / 60.0988;
	long frame = 0;
	/* speed, for the log: frames, emulation and drawing time, audio running dry */
	struct {
		uint64_t since;
		long frames, underruns;
		double emuMs, emuMax, drawMs;
	} st = { SDL_GetPerformanceCounter(), 0, 0, 0, 0, 0 };

	while (!quit) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) quit = true;
			else if (e.type == SDL_CONTROLLERDEVICEADDED && !gc) gc = SDL_GameControllerOpen(e.cdevice.which);
			else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) pad_key(&pad, &e.key);
		}
		if (stopRequested) quit = true;
		if (frame == 2 || frame == 30 || frame == 120) {
			if (opt.fullscreen) sway_span();
		}

		uint16_t buttons = pad_frame(&pad, gc);
		uint32_t act = pad.actions;
		pad.actions = 0;
		if (act & ACT_QUIT) quit = true;
		if (act & ACT_SWAP) swapped = !swapped;
		if (act & ACT_ASPECT) opt.wide = !opt.wide;
		if (act & ACT_FAST) fastToggle = !fastToggle;
		if (act & ACT_SPLIT) {
			splitOn = !splitOn;
			screen2_set_split(s2, splitOn);
		}
		if (act & ACT_FULLSCREEN) {
			opt.fullscreen = !opt.fullscreen;
			SDL_SetWindowFullscreen(win, opt.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
			if (opt.fullscreen) sway_span();
		}
		if (act & ACT_SAVE_STATE) {
			size_t n = core_state_size(game);
			uint8_t* d = malloc(n);
			core_save_state(game, d);
			printf(write_file(statePath, d, n) ? "saved %s\n" : "can't save %s\n", statePath);
			free(d);
		}
		if (act & ACT_LOAD_STATE) {
			size_t n;
			uint8_t* d = romfile_read(statePath, &n);
			if (d && core_load_state(game, d, n)) {
				screen2_reset(s2);
				printf("loaded %s\n", statePath);
			}
			free(d);
		}
		bool fast = fastToggle || pad.fastKey;

		uint64_t t0 = SDL_GetPerformanceCounter();
		core_set_pad(game, 0, buttons);
		screen2_before_frame(s2, game);
		core_run_frame(game);
		screen2_after_frame(s2, game);
		core_audio(game, samples, SAMPLES_PER_FRAME);
		double emu = ms_since(t0);
		st.emuMs += emu;
		if (emu > st.emuMax) st.emuMax = emu;
		st.frames++;
		frame++;

		/* battery saves: written a few seconds after they change */
		if (sram && frame % 300 == 0) {
			uint8_t* now = malloc(sramSize);
			core_sram(game, now);
			if (memcmp(now, sram, sramSize)) {
				memcpy(sram, now, sramSize);
				write_file(srmPath, sram, sramSize);
			}
			free(now);
		}

		if (opt.stats > 0 && ms_since(st.since) >= opt.stats * 1000.0) {
			double secs = ms_since(st.since) / 1000.0;
			int caps = 0;
			double capAvg = 0, capMax = 0;
			bool haveMap = screen2_map_stats(s2, &caps, &capAvg, &capMax);
			printf("stats: %.1f fps, emulation %.2f ms/frame (max %.2f), drawing %.2f ms, audio underruns %ld",
			       st.frames / secs, st.emuMs / st.frames, st.emuMax, st.drawMs / st.frames, st.underruns);
			if (haveMap) printf(", map captures %d (%.0f ms avg, %.0f max)", caps, capAvg, capMax);
			printf("\n");
			fflush(stdout);
			st.since = SDL_GetPerformanceCounter();
			st.frames = st.underruns = 0;
			st.emuMs = st.emuMax = st.drawMs = 0;
		}

		bool last = opt.frames && frame >= opt.frames;
		if (last) quit = true;

		/* fast forward: draw every 4th frame, no audio, no waiting */
		if (fast && frame % 4 && !last) continue;

		uint64_t t1 = SDL_GetPerformanceCounter();
		int ow, oh;
		SDL_GetRendererOutputSize(ren, &ow, &oh);
		SDL_Rect gameRect, secondRect;
		video_layout(opt.layout, ow, oh, &gameRect, &secondRect);
		SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
		SDL_RenderClear(ren);
		video_present(ren, &texGame, core_frame(game), swapped ? secondRect : gameRect, opt.wide);
		video_present(ren, &texSecond, screen2_picture(s2), swapped ? gameRect : secondRect, opt.wide);
		if (last && opt.shot) printf(video_shot(ren, opt.shot) ? "shot %s\n" : "can't save %s\n", opt.shot);
		SDL_RenderPresent(ren);
		st.drawMs += ms_since(t1);

		if (fast) {
			if (audio) SDL_ClearQueuedAudio(audio);
			next = SDL_GetPerformanceCounter();
			continue;
		}
		/* pace by the audio queue when there is one, else by the clock */
		if (audio) {
			if (SDL_GetQueuedAudioSize(audio) == 0 && frame > 60) st.underruns++;
			SDL_QueueAudio(audio, samples, sizeof(samples));
			while (SDL_GetQueuedAudioSize(audio) > sizeof(samples) * 3) SDL_Delay(1);
		} else if (!opt.frames) {
			next += (uint64_t)(frameTime * freq);
			uint64_t now = SDL_GetPerformanceCounter();
			if (next > now) SDL_Delay((uint32_t)((next - now) * 1000 / freq));
			else next = now;
		}
	}

	if (sram) {
		uint8_t* now = malloc(sramSize);
		core_sram(game, now);
		if (memcmp(now, sram, sramSize)) printf(write_file(srmPath, now, sramSize) ? "saved %s\n" : "can't save %s\n", srmPath);
		free(now);
	}
	screen2_free(s2);
	core_free(game);
	free(rom);
	free(sram);
	if (gc) SDL_GameControllerClose(gc);
	if (audio) SDL_CloseAudioDevice(audio);
	screen_tex_free(&texGame);
	screen_tex_free(&texSecond);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}
