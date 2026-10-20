/* snes-ds: the app. One window over both screens: on ROCKNIX (sway) it goes
 * fullscreen and is spanned over both panels. It starts on the game list
 * (src/ui), on the bottom (touch) screen; a game plays with its second
 * screen (screen2.c) below, and the menu button pauses it into the menu.
 *
 *   snes-ds [options]                 the game list of --rom-dir
 *   snes-ds [options] ROM             straight into that game (.sfc, .smc, .zip)
 *     --rom-dir=DIR              where the game list looks (and its
 *                                subfolders); default: the ROM's folder, or .
 *     --find-in=DIR              the same as --rom-dir
 *     --print-rom                the first game in --rom-dir with a profile:
 *                                print its path and quit
 *     --layout=auto|stack|side   (auto: side by side on a wide window, as
 *                                when spanning two panels; else stacked)
 *     --aspect=native|4:3        (also in the settings)
 *     --language=en|es           the interface's (also in the settings)
 *     --scale=N                  windowed: the picture's scale (default 2)
 *     --screen=WxH               windowed: each screen's size
 *     --fullscreen / --windowed  (default: fullscreen under sway or KMSDRM)
 *     --save-dir=DIR             battery saves (default: beside the ROM)
 *     --state-dir=DIR            save states (default: as --save-dir)
 *     --config=FILE              the settings (default: snes-ds.cfg beside
 *                                the program)
 *     --menu-button=NAME         the button that opens the menu: an SDL
 *                                controller button name (default guide) or
 *                                joy:N (a raw joystick button)
 *     --no-map                   no hidden map core, whatever the settings say
 *     --no-threads               draw the picture on the emulation's thread
 *     --no-audio
 *     --stats=N                  log speed every N seconds (default 10, 0 off)
 *     --frames=N                 quit after N frames (for testing)
 *     --shot=FILE.png            with --frames: the window before quitting
 *     --script=FILE              for testing: "FRAME command" lines, the
 *                                commands nav NAME (in the app's screens:
 *                                ok back up down left right x y l r start
 *                                select menu), key NAME (an SDL key name,
 *                                pressed as on the keyboard: "Escape"),
 *                                button NAME (an SDL controller button,
 *                                "guide", "leftstick", held 3 frames on a
 *                                virtual controller),
 *                                tap X Y (0-1 across the bottom screen),
 *                                shot FILE, shotbottom FILE (the bottom
 *                                screen alone), quit
 *     --load-state=FILE          with a ROM
 *
 * Keys: arrows, X = A, Z = B, S = X, A = Y, Q = L, W = R, Enter = Start,
 * Right Shift = Select; Esc the menu. F2 swap screens, F3 HUD split on/off,
 * F4 aspect, F5 save state, F7 load state, Tab fast forward, F11
 * fullscreen. The mouse taps the bottom screen. Controllers by button
 * position; shortcuts with Select: see pad.h.
 */
#include <SDL.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "core.h"
#include "game.h"
#include "gfx.h"
#include "i18n.h"
#include "library.h"
#include "pad.h"
#include "romfile.h"
#include "settings.h"
#include "ui.h"
#include "video.h"

#ifndef SNESDS_VERSION
#define SNESDS_VERSION "dev"
#endif

#define SAMPLE_RATE 48000
#define SAMPLES_PER_FRAME 800

static struct {
	const char* romPath;
	const char* romDir;
	const char* saveDir;
	const char* stateDir;
	const char* config;
	const char* menuButton;
	const char* loadState;
	const char* shot;
	const char* script;
	Layout layout;
	int aspect; /* -1: the settings', 0 native, 1 4:3 */
	int lang;   /* -1: the settings' */
	bool noMap, noAudio, printRom, noThreads;
	int fullscreen; /* -1: decide */
	int scale, screenW, screenH, stats;
	long frames;
} opt = { .layout = LAYOUT_AUTO, .aspect = -1, .lang = -1, .fullscreen = -1, .scale = 2, .stats = 10 };

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
		else if (arg(a, "--aspect", &v)) opt.aspect = !strcmp(v, "4:3");
		else if (arg(a, "--language", &v)) opt.lang = lang_from_code(v);
		else if (arg(a, "--scale", &v)) opt.scale = atoi(v);
		else if (arg(a, "--screen", &v)) sscanf(v, "%dx%d", &opt.screenW, &opt.screenH);
		else if (arg(a, "--save-dir", &v)) opt.saveDir = v;
		else if (arg(a, "--state-dir", &v)) opt.stateDir = v;
		else if (arg(a, "--rom-dir", &v) || arg(a, "--find-in", &v)) opt.romDir = v;
		else if (arg(a, "--config", &v)) opt.config = v;
		else if (arg(a, "--menu-button", &v)) opt.menuButton = v;
		else if (arg(a, "--stats", &v)) opt.stats = atoi(v);
		else if (arg(a, "--frames", &v)) opt.frames = atol(v);
		else if (arg(a, "--shot", &v)) opt.shot = v;
		else if (arg(a, "--script", &v)) opt.script = v;
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
		opt.screenW = CORE_W * opt.scale;
	}
	if (!opt.stateDir) opt.stateDir = opt.saveDir;
}

/* the program's folder (for the fonts, the settings, the cache). From
 * argv[0] first: on Windows its encoding is the one fopen takes */
static void program_dir(const char* argv0, char* out, size_t n) {
	const char* slash = strrchr(argv0, '/');
	const char* bslash = strrchr(argv0, '\\');
	if (bslash > slash) slash = bslash;
	if (slash) {
		snprintf(out, n, "%.*s", (int)(slash - argv0), argv0);
		return;
	}
#ifndef _WIN32
	char* base = SDL_GetBasePath();
	if (base) {
		snprintf(out, n, "%s", base);
		size_t len = strlen(out);
		if (len > 1 && out[len - 1] == '/') out[len - 1] = 0;
		SDL_free(base);
		return;
	}
#endif
	snprintf(out, n, ".");
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

/* --- the test script ------------------------------------------------------ */

typedef struct {
	long frame;
	char cmd[16];
	char arg[256];
	float x, y;
} ScriptLine;

static ScriptLine* script;
static int scriptLines;

static void script_load(const char* path) {
	FILE* f = fopen(path, "r");
	if (!f) {
		fprintf(stderr, "can't read %s\n", path);
		return;
	}
	char line[512];
	while (fgets(line, sizeof(line), f)) {
		ScriptLine l = { 0 };
		int at = 0;
		if (line[0] == '#' || sscanf(line, "%ld %15s %n", &l.frame, l.cmd, &at) < 2) continue;
		line[strcspn(line, "\r\n")] = 0;
		snprintf(l.arg, sizeof(l.arg), "%s", at ? line + at : "");
		sscanf(l.arg, "%f %f", &l.x, &l.y);
		script = realloc(script, (scriptLines + 1) * sizeof(ScriptLine));
		script[scriptLines++] = l;
	}
	fclose(f);
}

static uint32_t nav_by_name(const char* s) {
	static const struct { const char* n; uint32_t v; } names[] = {
		{ "ok", NAV_OK }, { "back", NAV_BACK }, { "up", NAV_UP }, { "down", NAV_DOWN }, { "left", NAV_LEFT },
		{ "right", NAV_RIGHT }, { "x", NAV_X }, { "y", NAV_Y }, { "l", NAV_L }, { "r", NAV_R },
		{ "start", NAV_START }, { "select", NAV_SELECT }, { "menu", NAV_MENU },
	};
	for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
		if (!strcmp(s, names[i].n)) return names[i].v;
	}
	return 0;
}

/* --- the app -------------------------------------------------------------- */

typedef enum { MODE_UI, MODE_GAME } Mode;

typedef struct {
	SDL_Window* win;
	SDL_Renderer* ren;
	SDL_AudioDeviceID audio;
	SDL_GameController* gc;
	Gfx* gfx;
	Ui* ui;
	Settings set;
	char configPath[1100];
	Library lib;
	Game* game;
	Mode mode;
	Pad pad;
	ScreenTex texGame, texSecond;
	char progDir[1024];
	Image background; /* the game's picture, behind a second screen with only the HUD */
} App;

static void save_settings(App* a) {
	if (!settings_save(&a->set, a->configPath)) fprintf(stderr, "can't save %s\n", a->configPath);
}

/* the controls from the settings into the pad (and the log, the ones that
 * aren't the default) */
static void apply_binds(App* a, bool log) {
	static const char* const names[BINDS] = { "b", "a", "y", "x", "l", "r", "start", "select", "menu" };
	for (int i = 0; i < BINDS; i++) {
		if (!pad_set_bind(&a->pad, i, a->set.binds[i])) fprintf(stderr, "controls: can't use %s for %s\n", a->set.binds[i], names[i]);
		if (log && a->set.binds[i][0]) printf("controls: %s = %s\n", names[i], a->set.binds[i]);
	}
	fflush(stdout);
}

/* the settings as they are now, onto the game */
static void apply_settings(App* a) {
	if (!a->game) return;
	game_set_split(a->game, a->set.split);
	game_set_map(a->game, a->set.map && !opt.noMap);
	screen2_set_see_through(a->game->s2, a->set.background && a->background.tex);
}

static bool load_first(App* a, const char* const* paths, int n) {
	for (int i = 0; i < n; i++) {
		struct stat st;
		if (paths[i][0] && stat(paths[i], &st) == 0 && gfx_image_load(a->gfx, paths[i], &a->background)) {
			printf("background: %s\n", paths[i]);
			return true;
		}
	}
	return false;
}

/* the picture behind the HUD: the player's own (backgrounds/<id> or
 * backgrounds/<the ROM's name>, .png or .jpg), else the game's cover (a
 * game with no profile too), else its icon */
static void load_background(App* a, const char* romPath) {
	gfx_image_free(&a->background);
	const Profile* p = a->game->profile;
	const char* file = strrchr(romPath, '/');
	const char* bs = strrchr(romPath, '\\');
	if (!file || (bs && bs > file)) file = bs;
	file = file ? file + 1 : romPath;
	char base[512];
	snprintf(base, sizeof(base), "%s", file);
	char* dot = strrchr(base, '.');
	if (dot) *dot = 0;
	char own[4][1200];
	snprintf(own[0], sizeof(own[0]), "%s/backgrounds/%s.png", a->progDir, p ? p->id : "");
	snprintf(own[1], sizeof(own[1]), "%s/backgrounds/%s.jpg", a->progDir, p ? p->id : "");
	snprintf(own[2], sizeof(own[2]), "%s/backgrounds/%s.png", a->progDir, base);
	snprintf(own[3], sizeof(own[3]), "%s/backgrounds/%s.jpg", a->progDir, base);
	const char* const ownp[] = { p ? own[0] : "", p ? own[1] : "", own[2], own[3] };
	if (load_first(a, ownp, 4)) return;
	char* cover = library_cover(romPath);
	char icon[1200];
	snprintf(icon, sizeof(icon), "%s/assets/games/%s.png", a->progDir, p ? p->id : "");
	const char* const rest[] = { cover ? cover : "", p ? icon : "" };
	load_first(a, rest, 2);
	free(cover);
}

/* behind the second screen, when it has nothing of its own but the HUD:
 * the game's picture, filling it, darkened so the HUD reads */
static void draw_background(App* a, SDL_Rect r) {
	if (!a->set.background || !a->background.tex || !screen2_empty(a->game->s2)) return;
	gfx_area(a->gfx, r);
	float w = gfx_w(a->gfx), h = gfx_h(a->gfx);
	gfx_image(a->gfx, &a->background, 0, 0, w, h, 0, true, 1);
	gfx_fill(a->gfx, 0, 0, w, h, 0, with_alpha(rgb(0x0C0806), 0.55f));
	gfx_done(a->gfx);
}

static bool start_game(App* a, const char* path) {
	Game* g = game_open(path, opt.saveDir, opt.stateDir, a->set.map && !opt.noMap, a->set.split, !opt.noThreads);
	if (!g) return false;
	a->game = g;
	load_background(a, path);
	apply_settings(a);
	char title[300];
	snprintf(title, sizeof(title), "SNES DS - %s", game_title(g));
	SDL_SetWindowTitle(a->win, title);
	return true;
}

static void to_game(App* a) {
	printf("menu: back to the game\n");
	fflush(stdout);
	a->mode = MODE_GAME;
	pad_swallow(&a->pad, a->gc);
	if (a->audio) SDL_ClearQueuedAudio(a->audio);
}

static void to_menu(App* a) {
	printf("menu: open\n");
	fflush(stdout);
	a->mode = MODE_UI;
	ui_show_pause(a->ui, a->game);
	pad_swallow(&a->pad, a->gc);
	if (a->audio) SDL_ClearQueuedAudio(a->audio);
}

static void close_game(App* a) {
	game_close(a->game);
	a->game = NULL;
	gfx_image_free(&a->background);
	SDL_SetWindowTitle(a->win, "SNES DS");
	a->mode = MODE_UI;
	ui_show_library(a->ui);
	pad_swallow(&a->pad, a->gc);
}

/* the two screens' places: the game's and the second screen's (when the
 * game plays: swapped if asked); top is always the top panel */
static void screens(App* a, SDL_Rect* top, SDL_Rect* bottom) {
	int ow, oh;
	SDL_GetRendererOutputSize(a->ren, &ow, &oh);
	video_layout(opt.layout, ow, oh, top, bottom);
}

int main(int argc, char** argv) {
	parse_args(argc, argv);
	char dirBuf[1024];
	if (opt.printRom) {
		const char* dir = opt.romDir ? opt.romDir : ".";
		if (!romfile_find(dir, dirBuf, sizeof(dirBuf))) {
			fprintf(stderr, "no game with a second-screen profile in %s\n", dir);
			return 1;
		}
		puts(dirBuf);
		return 0;
	}
	if (!opt.romDir) {
		/* the game list looks beside the ROM given, or here */
		if (opt.romPath) {
			const char* slash = strrchr(opt.romPath, '/');
			const char* bslash = strrchr(opt.romPath, '\\');
			if (bslash > slash) slash = bslash;
			if (slash) snprintf(dirBuf, sizeof(dirBuf), "%.*s", (int)(slash - opt.romPath), opt.romPath);
			else snprintf(dirBuf, sizeof(dirBuf), ".");
		} else {
			snprintf(dirBuf, sizeof(dirBuf), ".");
		}
		opt.romDir = dirBuf;
	}

	static App app;
	App* a = &app;
	pad_init(&a->pad);
	if (opt.menuButton && !pad_set_menu_button(&a->pad, opt.menuButton))
		fprintf(stderr, "unknown --menu-button %s\n", opt.menuButton);
	char progDir[1024], assets[1100], cachePath[1100];
	program_dir(argv[0], progDir, sizeof(progDir));
	snprintf(a->progDir, sizeof(a->progDir), "%s", progDir);
	snprintf(assets, sizeof(assets), "%s/assets", progDir);
	snprintf(cachePath, sizeof(cachePath), "%s/library-cache.txt", progDir);
	if (opt.config) snprintf(a->configPath, sizeof(a->configPath), "%s", opt.config);
	else snprintf(a->configPath, sizeof(a->configPath), "%s/snes-ds.cfg", progDir);
	settings_defaults(&a->set);
	settings_load(&a->set, a->configPath);
	if (opt.aspect >= 0) a->set.wide = opt.aspect;
	if (opt.lang >= 0) a->set.lang = opt.lang;
	i18n_lang = a->set.lang;
	apply_binds(a, true);
	if (opt.script) script_load(opt.script);

#ifndef _WIN32
	setenv("SDL_VIDEO_WAYLAND_WMCLASS", "snes-ds", 0);
#endif
	/* taps come as touches; SDL's mouse made from them would be twice */
	SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
		fprintf(stderr, "SDL: %s\n", SDL_GetError());
		return 1;
	}
	/* no sound is no reason not to play */
	if (!opt.noAudio && SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
		fprintf(stderr, "no audio: %s\n", SDL_GetError());
		opt.noAudio = true;
	}
	signal(SIGINT, on_signal);
	signal(SIGTERM, on_signal);
	const char* driver = SDL_GetCurrentVideoDriver();
	if (opt.fullscreen < 0) opt.fullscreen = getenv("SWAYSOCK") || (driver && !strcmp(driver, "kmsdrm"));
	bool side = opt.layout == LAYOUT_SIDE;
	int winW = side ? opt.screenW * 2 : opt.screenW, winH = side ? opt.screenH : opt.screenH * 2;
	a->win = SDL_CreateWindow("SNES DS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, winW, winH,
	                          SDL_WINDOW_RESIZABLE | (opt.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));
	a->ren = a->win ? SDL_CreateRenderer(a->win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE) : NULL;
	if (!a->ren) a->ren = a->win ? SDL_CreateRenderer(a->win, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE) : NULL;
	if (!a->ren) {
		fprintf(stderr, "SDL: %s\n", SDL_GetError());
		return 1;
	}
	SDL_RendererInfo ri;
	SDL_GetRendererInfo(a->ren, &ri);
	printf("snes-ds %s; video: %s, renderer %s, %s\n", SNESDS_VERSION, driver, ri.name,
	       opt.fullscreen ? "fullscreen" : "windowed");
	screen_tex_init(a->ren, &a->texGame, false);
	screen_tex_init(a->ren, &a->texSecond, true);
	a->gfx = gfx_new(a->ren, assets);
	if (!a->gfx) {
		fprintf(stderr, "the app's fonts aren't in %s\n", assets);
		return 1;
	}

	if (!opt.noAudio) {
		SDL_AudioSpec want = { .freq = SAMPLE_RATE, .format = AUDIO_S16SYS, .channels = 2, .samples = 512 }, have;
		a->audio = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
		if (a->audio) SDL_PauseAudioDevice(a->audio, 0);
		else fprintf(stderr, "no audio: %s\n", SDL_GetError());
	}
	for (int i = 0; i < SDL_NumJoysticks() && !a->gc; i++) {
		if (SDL_IsGameController(i)) a->gc = SDL_GameControllerOpen(i);
	}
	if (a->gc) printf("controller: %s\n", SDL_GameControllerName(a->gc));
	/* every input device, for the log; the ones that aren't the controller
	 * stay open, so their buttons can open the menu */
	for (int i = 0; i < SDL_NumJoysticks(); i++) {
		SDL_Joystick* j = a->gc && SDL_JoystickGetDeviceInstanceID(i) == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(a->gc))
		                ? SDL_GameControllerGetJoystick(a->gc) : SDL_JoystickOpen(i);
		printf("input device %d: %s, %d buttons, %d axes, %d hats%s\n", i, SDL_JoystickNameForIndex(i),
		       j ? SDL_JoystickNumButtons(j) : 0, j ? SDL_JoystickNumAxes(j) : 0, j ? SDL_JoystickNumHats(j) : 0,
		       SDL_IsGameController(i) ? ", a controller" : "");
	}
	if (a->gc) {
		char* map = SDL_GameControllerMapping(a->gc);
		if (map) printf("controller mapping: %s\n", map);
		SDL_free(map);
	}

	/* the game list: a frame saying so first, the first scan can take a while */
	{
		SDL_Rect top, bottom;
		screens(a, &top, &bottom);
		SDL_SetRenderDrawColor(a->ren, 0, 0, 0, 255);
		SDL_RenderClear(a->ren);
		SDL_Rect both[2] = { top, bottom };
		for (int i = 0; i < 2; i++) {
			gfx_area(a->gfx, both[i]);
			gfx_fill_grad(a->gfx, 0, 0, gfx_w(a->gfx), gfx_h(a->gfx), 0, rgb(0x221B16), rgb(0x141010));
		}
		gfx_text(a->gfx, FONT_BOLD, 16, gfx_w(a->gfx) / 2, 170, rgb(0xC7B1A0), ALIGN_CENTER, T("Looking for games…", "Buscando juegos…"));
		gfx_done(a->gfx);
		SDL_RenderPresent(a->ren);
	}
	uint64_t scanT = SDL_GetPerformanceCounter();
	library_scan(&a->lib, opt.romDir, cachePath);
	char icons[1200];
	snprintf(icons, sizeof(icons), "%s/games", assets);
	library_add_icons(&a->lib, icons);
	printf("library: %s: %d games with a profile (of %d ROMs) in %.0f ms\n", opt.romDir, a->lib.count, a->lib.scanned,
	       ms_since(scanT));
	a->ui = ui_new(a->gfx, &a->set, &a->lib, opt.romDir, "v" SNESDS_VERSION);
	ui_show_library(a->ui);
	a->mode = MODE_UI;

	if (opt.romPath) {
		if (!start_game(a, opt.romPath)) return 1;
		if (opt.loadState) game_load_state_file(a->game, opt.loadState);
		to_game(a);
	}
	fflush(stdout);

	bool quit = false, fastToggle = false;
	int16_t samples[SAMPLES_PER_FRAME * 2] = { 0 };
	uint64_t freq = SDL_GetPerformanceFrequency(), next = SDL_GetPerformanceCounter();
	const double frameTime = 1.0 / 60.0988;
	long tick = 0, sinceResume = 0;
	int logged = 0;
	UiTouch touch = { 0 };
	bool tapRelease = false;
	/* speed, for the log and the FPS counter */
	struct {
		uint64_t since;
		long frames, underruns;
		double emuMs, emuMax, drawMs;
	} st = { SDL_GetPerformanceCounter(), 0, 0, 0, 0, 0 };
	uint64_t fpsSince = SDL_GetPerformanceCounter();
	long fpsFrames = 0;
	double fps = 0;

	SDL_Keycode keyUp = 0; /* the script's key, let go the frame after */
	/* the script's controller: a virtual one, made the controller on first use */
	SDL_Joystick* vjoy = NULL;
	int vbutton = -1;
	long vbuttonUp = 0;
	while (!quit) {
		uint64_t iterStart = SDL_GetPerformanceCounter();
		touch.pressed = touch.released = false;
		if (keyUp) {
			SDL_Event ev;
			SDL_zero(ev); /* all of it: an initialiser only clears a union's first member */
			ev.type = SDL_KEYUP;
			ev.key.keysym.sym = keyUp;
			SDL_PushEvent(&ev);
			keyUp = 0;
		}
		if (tapRelease) {
			touch.down = false;
			touch.released = true;
			tapRelease = false;
		}
		int ow, oh, ww, wh;
		SDL_GetRendererOutputSize(a->ren, &ow, &oh);
		SDL_GetWindowSize(a->win, &ww, &wh);
		SDL_Event e;
		bool captured = false; /* an input went to the controls page this frame */
		while (SDL_PollEvent(&e)) {
			if (a->mode == MODE_UI && ui_capturing(a->ui) >= 0) {
				char b[SETTINGS_BIND_LEN];
				if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
					ui_captured(a->ui, NULL);
					captured = true;
				} else if (pad_capture(a->gc, &e, b, sizeof(b))) {
					printf("controls: pressed %s\n", b);
					ui_captured(a->ui, b);
					captured = true;
				}
			}
			switch (e.type) {
			case SDL_QUIT: quit = true; break;
			case SDL_CONTROLLERDEVICEADDED:
				if (!a->gc) {
					a->gc = SDL_GameControllerOpen(e.cdevice.which);
					if (a->gc) printf("controller: %s\n", SDL_GameControllerName(a->gc));
				}
				break;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				if (e.type == SDL_KEYDOWN && !e.key.repeat && logged++ < 60)
					printf("input: key %s (scancode %d)\n", SDL_GetKeyName(e.key.keysym.sym), e.key.keysym.scancode);
				pad_key(&a->pad, &e.key);
				break;
			/* which button is which, on a device seen for the first time */
			case SDL_CONTROLLERBUTTONDOWN:
				if (logged++ < 60) printf("input: button %s\n", SDL_GameControllerGetStringForButton(e.cbutton.button));
				pad_button_event(&a->pad, a->gc, &e);
				break;
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP: {
				bool own = a->gc && e.jbutton.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(a->gc));
				if (e.type == SDL_JOYBUTTONDOWN && logged++ < 60)
					printf("input: %s button %d\n", own ? "controller's raw" : "other device's", e.jbutton.button);
				if (!own) pad_extra_button(&a->pad, e.type == SDL_JOYBUTTONDOWN);
				else pad_button_event(&a->pad, a->gc, &e);
				break;
			}
			case SDL_JOYDEVICEADDED:
				if (!SDL_IsGameController(e.jdevice.which)) {
					SDL_Joystick* j = SDL_JoystickOpen(e.jdevice.which);
					if (j) printf("input device: %s, %d buttons\n", SDL_JoystickName(j), SDL_JoystickNumButtons(j));
				}
				break;
			case SDL_FINGERDOWN:
			case SDL_FINGERUP:
			case SDL_FINGERMOTION:
				touch.x = e.tfinger.x * ow;
				touch.y = e.tfinger.y * oh;
				if (e.type == SDL_FINGERDOWN) {
					touch.down = touch.pressed = true;
					if (logged++ < 40) printf("input: touch at %.3f, %.3f (device %lld)\n", e.tfinger.x, e.tfinger.y, (long long)e.tfinger.touchId);
				} else if (e.type == SDL_FINGERUP) {
					touch.down = false;
					touch.released = true;
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEMOTION:
				if (e.type == SDL_MOUSEMOTION ? e.motion.which == SDL_TOUCH_MOUSEID : e.button.which == SDL_TOUCH_MOUSEID) break;
				if (e.type == SDL_MOUSEMOTION) {
					touch.x = e.motion.x * (float)ow / ww;
					touch.y = e.motion.y * (float)oh / wh;
				} else if (e.button.button == SDL_BUTTON_LEFT) {
					touch.x = e.button.x * (float)ow / ww;
					touch.y = e.button.y * (float)oh / wh;
					bool down = e.type == SDL_MOUSEBUTTONDOWN;
					touch.pressed |= down;
					touch.released |= !down;
					touch.down = down;
				}
				break;
			default: break;
			}
		}
		if (stopRequested) quit = true;
		if (tick == 2 || tick == 30 || tick == 120) {
			if (opt.fullscreen) sway_span();
		}

		/* the test script */
		uint32_t scriptNav = 0;
		if (vjoy && vbutton >= 0 && tick >= vbuttonUp) {
			SDL_JoystickSetVirtualButton(vjoy, vbutton, SDL_RELEASED);
			vbutton = -1;
		}
		const char* shotNow = NULL;
		bool shotBottom = false;
		for (int i = 0; i < scriptLines; i++) {
			ScriptLine* l = &script[i];
			if (l->frame != tick) continue;
			if (!strcmp(l->cmd, "nav")) scriptNav |= nav_by_name(l->arg);
			else if (!strcmp(l->cmd, "quit")) quit = true;
			else if (!strcmp(l->cmd, "shot")) shotNow = l->arg;
			else if (!strcmp(l->cmd, "shotbottom")) {
				shotNow = l->arg;
				shotBottom = true;
			}
			else if (!strcmp(l->cmd, "button")) {
				/* a controller button, held for 3 frames, on a virtual controller
				 * that takes the real one's place: the same path as a handheld's */
				if (!vjoy) {
					SDL_VirtualJoystickDesc d;
					SDL_zero(d);
					d.version = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
					d.type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
					d.naxes = SDL_CONTROLLER_AXIS_MAX;
					d.nbuttons = SDL_CONTROLLER_BUTTON_MAX;
					d.name = "script controller";
					int idx = SDL_JoystickAttachVirtualEx(&d);
					SDL_GameController* gc = idx >= 0 ? SDL_GameControllerOpen(idx) : NULL;
					if (gc) {
						a->gc = gc;
						vjoy = SDL_GameControllerGetJoystick(gc);
						pad_swallow(&a->pad, a->gc);
					} else {
						fprintf(stderr, "script: no virtual controller: %s\n", SDL_GetError());
					}
				}
				SDL_GameControllerButton b = SDL_GameControllerGetButtonFromString(l->arg);
				if (vjoy && b != SDL_CONTROLLER_BUTTON_INVALID) {
					SDL_JoystickSetVirtualButton(vjoy, b, SDL_PRESSED);
					vbutton = b;
					vbuttonUp = tick + 3;
				} else {
					fprintf(stderr, "script: button %s not pressed\n", l->arg);
				}
			} else if (!strcmp(l->cmd, "key")) {
				/* a key press as the keyboard makes it (handled next frame) */
				SDL_Event ev;
				SDL_zero(ev);
				ev.type = SDL_KEYDOWN;
				ev.key.state = SDL_PRESSED;
				ev.key.keysym.sym = SDL_GetKeyFromName(l->arg);
				if (SDL_PushEvent(&ev) < 1) fprintf(stderr, "script: key %s not sent: %s\n", l->arg, SDL_GetError());
				keyUp = ev.key.keysym.sym;
			} else if (!strcmp(l->cmd, "tap")) {
				SDL_Rect top, bottom;
				screens(a, &top, &bottom);
				touch.x = bottom.x + l->x * bottom.w;
				touch.y = bottom.y + l->y * bottom.h;
				touch.down = touch.pressed = true;
				tapRelease = true;
			}
		}

		SDL_Rect top, bottom;
		screens(a, &top, &bottom);
		SDL_SetRenderDrawColor(a->ren, 0, 0, 0, 255);
		SDL_RenderClear(a->ren);
		bool fast = false, drew = true;
		bool ran = false; /* a game frame ran (its sound is in samples) */

		/* the menu: its button (or Esc, Select + Start), or a tap on the
		 * bottom screen (games don't use touch). The tap that opens it
		 * doesn't count in the menu */
		if (a->mode == MODE_GAME) {
			SDL_Point p = { (int)touch.x, (int)touch.y };
			bool tap = touch.pressed && SDL_PointInRect(&p, &bottom);
			if ((a->pad.actions & ACT_MENU) || tap) {
				a->pad.actions &= ~ACT_MENU;
				to_menu(a);
				touch.pressed = false;
			}
		}
		bool uiIter = a->mode == MODE_UI;

		if (a->mode == MODE_UI) {
			uint32_t nav = pad_nav(&a->pad, a->gc) | scriptNav;
			/* what's pressed for a binding moves nothing */
			if (captured || ui_capturing(a->ui) >= 0) nav = 0;
			if (quit) nav |= NAV_QUIT;
			UiAction act = ui_frame(a->ui, nav, &touch, top, bottom);
			switch (act.kind) {
			case UIA_PLAY:
				if (start_game(a, a->lib.games[act.arg].path)) {
					to_game(a);
					sinceResume = 0;
				} else {
					ui_toast(a->ui, T("Couldn't open the game", "No se ha podido abrir el juego"));
				}
				break;
			case UIA_RESUME: to_game(a); sinceResume = 0; break;
			case UIA_SAVE: {
				char m[64];
				snprintf(m, sizeof(m), game_save_state(a->game, act.arg) ? T("Saved to slot %d", "Guardado en la ranura %d")
				                                                       : T("Couldn't save to slot %d", "No se ha podido guardar en la ranura %d"),
				         act.arg);
				ui_toast(a->ui, m);
				to_game(a);
				sinceResume = 0;
				break;
			}
			case UIA_LOAD: {
				char m[64];
				snprintf(m, sizeof(m), game_load_state(a->game, act.arg) ? T("Loaded slot %d", "Cargada la ranura %d")
				                                                       : T("Couldn't load slot %d", "No se ha podido cargar la ranura %d"),
				         act.arg);
				ui_toast(a->ui, m);
				to_game(a);
				sinceResume = 0;
				break;
			}
			case UIA_CLOSE_GAME: close_game(a); break;
			case UIA_QUIT: quit = true; break;
			case UIA_SETTINGS:
				save_settings(a);
				apply_settings(a);
				break;
			case UIA_CONTROLS:
				save_settings(a);
				apply_binds(a, true);
				pad_swallow(&a->pad, a->gc);
				break;
			case UIA_NONE: break;
			}
		} else {
			uint16_t buttons = pad_frame(&a->pad, a->gc);
			uint32_t act = a->pad.actions;
			/* the menu is opened at the top of the next frame, with the taps
			 * and Esc: kept, not dropped with the rest */
			a->pad.actions = act & ACT_MENU;
			if (act & ACT_QUIT) quit = true;
			if (act & (ACT_SWAP | ACT_ASPECT | ACT_SPLIT)) {
				if (act & ACT_SWAP) {
					a->set.swapped = !a->set.swapped;
					ui_toast(a->ui, a->set.swapped ? T("The game on the bottom screen", "El juego, en la pantalla de abajo")
				                                       : T("The game on the top screen", "El juego, en la pantalla de arriba"));
				}
				if (act & ACT_ASPECT) {
					a->set.wide = !a->set.wide;
					ui_toast(a->ui, a->set.wide ? T("4:3 picture", "Formato 4:3") : T("Original picture", "Formato original"));
				}
				if (act & ACT_SPLIT) {
					a->set.split = !a->set.split;
					ui_toast(a->ui, a->set.split ? T("HUD and menus below", "HUD y menús abajo") : T("HUD and menus in the game", "HUD y menús en el juego"));
				}
				save_settings(a);
				apply_settings(a);
			}
			if (act & ACT_FAST) fastToggle = !fastToggle;
			if (act & ACT_FULLSCREEN) {
				opt.fullscreen = !opt.fullscreen;
				SDL_SetWindowFullscreen(a->win, opt.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
				if (opt.fullscreen) sway_span();
			}
			if (act & ACT_SAVE_STATE) ui_toast(a->ui, game_save_state(a->game, 1) ? T("Saved to slot 1", "Guardado en la ranura 1")
			                                                              : T("Couldn't save", "No se ha podido guardar"));
			if (act & ACT_LOAD_STATE) ui_toast(a->ui, game_load_state(a->game, 1) ? T("Loaded slot 1", "Cargada la ranura 1")
			                                                              : T("Slot 1 is empty", "La ranura 1 está vacía"));
			fast = fastToggle || a->pad.fastKey;

			uint64_t t0 = SDL_GetPerformanceCounter();
			game_frame(a->game, buttons, samples, SAMPLES_PER_FRAME);
			ran = true;
			double emu = ms_since(t0);
			st.emuMs += emu;
			if (emu > st.emuMax) st.emuMax = emu;
			st.frames++;
			fpsFrames++;
			sinceResume++;
			if (a->set.volume < 10) {
				for (int i = 0; i < SAMPLES_PER_FRAME * 2; i++) samples[i] = (int16_t)(samples[i] * a->set.volume / 10);
			}

			if (opt.stats > 0 && ms_since(st.since) >= opt.stats * 1000.0) {
				double secs = ms_since(st.since) / 1000.0;
				int caps = 0;
				double capAvg = 0, capMax = 0;
				bool haveMap = screen2_map_stats(a->game->s2, &caps, &capAvg, &capMax);
				printf("stats: %.1f fps, emulation %.2f ms/frame (max %.2f), drawing %.2f ms, audio underruns %ld",
				       st.frames / secs, st.emuMs / st.frames, st.emuMax, st.drawMs / st.frames, st.underruns);
				if (haveMap) printf(", map captures %d (%.0f ms avg, %.0f max)", caps, capAvg, capMax);
				printf("\n");
				fflush(stdout);
				st.since = SDL_GetPerformanceCounter();
				st.frames = st.underruns = 0;
				st.emuMs = st.emuMax = st.drawMs = 0;
			}
			if (ms_since(fpsSince) >= 500) {
				fps = fpsFrames * 1000.0 / ms_since(fpsSince);
				fpsSince = SDL_GetPerformanceCounter();
				fpsFrames = 0;
			}

			/* fast forward: draw every 4th frame, no audio, no waiting */
			drew = !(fast && a->game->frame % 4);
			if (drew) {
				uint64_t t1 = SDL_GetPerformanceCounter();
				SDL_Rect gameRect = a->set.swapped ? bottom : top, secondRect = a->set.swapped ? top : bottom;
				int y0, y1;
				screen2_game_lines(a->game->s2, &y0, &y1);
				video_present(a->ren, &a->texGame, core_frame(a->game->core), y0, y1, gameRect, a->set.wide);
				draw_background(a, secondRect);
				video_present(a->ren, &a->texSecond, screen2_picture(a->game->s2), 0, CORE_H, secondRect, a->set.wide);
				ui_draw_overlay(a->ui, gameRect, fps);
				st.drawMs += ms_since(t1);
			}
		}

		tick++;
		bool last = opt.frames && tick >= opt.frames;
		if (last) quit = true;
		const char* shot = shotNow ? shotNow : last ? opt.shot : NULL;
		if (shot && drew) printf(video_shot(a->ren, shot, shotBottom ? &bottom : NULL) ? "shot %s\n" : "can't save %s\n", shot);
		if (!drew && !last) continue;
		SDL_RenderPresent(a->ren);

		if (a->mode == MODE_GAME && fast) {
			if (a->audio) SDL_ClearQueuedAudio(a->audio);
			next = SDL_GetPerformanceCounter();
			continue;
		}
		/* pace by the audio queue when the game plays, else by the clock.
		 * Only a frame that ran has sound to queue (on the frame a game
		 * starts or comes back from the menu, none has yet) */
		if (ran && a->audio) {
			if (SDL_GetQueuedAudioSize(a->audio) == 0 && sinceResume > 60) st.underruns++;
			SDL_QueueAudio(a->audio, samples, sizeof(samples));
			while (SDL_GetQueuedAudioSize(a->audio) > sizeof(samples) * 3) SDL_Delay(1);
			next = SDL_GetPerformanceCounter();
		} else if (!opt.frames && !scriptLines) {
			next += (uint64_t)(frameTime * freq);
			uint64_t now = SDL_GetPerformanceCounter();
			if (next > now) SDL_Delay((uint32_t)((next - now) * 1000 / freq));
			else next = now;
		}
		/* time in the app's screens (the game list, the menu) isn't game time:
		 * the speed counters leave it out, or a pause reads as a slowdown */
		if (uiIter) {
			uint64_t d = SDL_GetPerformanceCounter() - iterStart;
			st.since += d;
			fpsSince += d;
		}
	}

	if (a->game) game_close(a->game);
	ui_free(a->ui);
	library_free(&a->lib);
	gfx_free(a->gfx);
	if (a->gc) SDL_GameControllerClose(a->gc);
	if (a->audio) SDL_CloseAudioDevice(a->audio);
	screen_tex_free(&a->texGame);
	screen_tex_free(&a->texSecond);
	SDL_DestroyRenderer(a->ren);
	SDL_DestroyWindow(a->win);
	SDL_Quit();
	free(script);
	return 0;
}
