/* snes-ds: the app. One window over both screens: the game on one, the
 * second screen (screen2.c) on the other.
 *
 *   snes-ds [options] ROM
 *     --layout=stack|side   screens one above the other (default) or side
 *                           by side (the RG DS layout under sway)
 *     --screen=WxH          size of each screen (default 256x224 times --scale)
 *     --scale=N             (default 2)
 *     --fullscreen
 *     --save-dir=DIR        where .srm and .stateN go (default: beside the ROM)
 *     --no-map              no hidden map core
 *     --no-audio
 *     --frames=N            quit after N frames (for testing)
 *     --shot=FILE.png       with --frames: save both screens before quitting
 *     --load-state=FILE
 *
 * Keys: arrows, X = A, Z = B, S = X, A = Y, Q = L, W = R, Enter = Start,
 * Right Shift = Select. F2 swap screens, F3 HUD split on/off, F5 save
 * state, F7 load state, Tab fast forward, F11 fullscreen, Esc quit.
 * Controllers by button position (bottom = B, right = A, top = X, left = Y).
 */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"
#include "profiles.h"
#include "screen2.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define SAMPLE_RATE 48000
#define SAMPLES_PER_FRAME 800

static struct {
	const char* romPath;
	const char* saveDir;
	const char* loadState;
	const char* shot;
	bool side, fullscreen, noMap, noAudio;
	int scale, screenW, screenH;
	long frames;
} opt = { .scale = 2 };

static bool arg(const char* a, const char* name, const char** val) {
	size_t n = strlen(name);
	if (strncmp(a, name, n)) return false;
	if (a[n] == '=') {
		*val = a + n + 1;
		return true;
	}
	return false;
}

static void parse_args(int argc, char** argv) {
	for (int i = 1; i < argc; i++) {
		const char* a = argv[i];
		const char* v;
		if (arg(a, "--layout", &v)) opt.side = !strcmp(v, "side");
		else if (arg(a, "--screen", &v)) sscanf(v, "%dx%d", &opt.screenW, &opt.screenH);
		else if (arg(a, "--scale", &v)) opt.scale = atoi(v);
		else if (arg(a, "--save-dir", &v)) opt.saveDir = v;
		else if (arg(a, "--frames", &v)) opt.frames = atol(v);
		else if (arg(a, "--shot", &v)) opt.shot = v;
		else if (arg(a, "--load-state", &v)) opt.loadState = v;
		else if (!strcmp(a, "--fullscreen")) opt.fullscreen = true;
		else if (!strcmp(a, "--no-map")) opt.noMap = true;
		else if (!strcmp(a, "--no-audio")) opt.noAudio = true;
		else if (a[0] == '-' && a[1] == '-') fprintf(stderr, "unknown option %s\n", a);
		else opt.romPath = a;
	}
	if (opt.scale < 1) opt.scale = 1;
	if (opt.screenW <= 0 || opt.screenH <= 0) {
		opt.screenW = CORE_W * opt.scale;
		opt.screenH = CORE_H * opt.scale;
	}
}

static uint8_t* read_file(const char* p, size_t* size) {
	FILE* f = fopen(p, "rb");
	if (!f) return NULL;
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	fseek(f, 0, SEEK_SET);
	uint8_t* d = malloc(n > 0 ? n : 1);
	if (d && fread(d, 1, n, f) != (size_t)n) {
		free(d);
		d = NULL;
	}
	fclose(f);
	*size = (size_t)n;
	return d;
}

static bool write_file(const char* p, const void* d, size_t n) {
	FILE* f = fopen(p, "wb");
	if (!f) return false;
	bool ok = fwrite(d, 1, n, f) == n;
	return fclose(f) == 0 && ok;
}

/* DIR/name-of-rom.EXT, or beside the ROM */
static void save_path(char* out, size_t n, const char* ext) {
	const char* base = opt.romPath;
	const char* slash = strrchr(base, '/');
	const char* bslash = strrchr(base, '\\');
	if (bslash > slash) slash = bslash;
	const char* name = slash ? slash + 1 : base;
	const char* dot = strrchr(name, '.');
	int nameLen = dot ? (int)(dot - name) : (int)strlen(name);
	if (opt.saveDir) snprintf(out, n, "%s/%.*s%s", opt.saveDir, nameLen, name, ext);
	else snprintf(out, n, "%.*s%s", (int)(name - base) + nameLen, base, ext);
}

/* SNES button for a key, or 0 */
static uint16_t key_button(SDL_Keycode k) {
	switch (k) {
	case SDLK_UP: return PAD_UP;
	case SDLK_DOWN: return PAD_DOWN;
	case SDLK_LEFT: return PAD_LEFT;
	case SDLK_RIGHT: return PAD_RIGHT;
	case SDLK_x: return PAD_A;
	case SDLK_z: return PAD_B;
	case SDLK_s: return PAD_X;
	case SDLK_a: return PAD_Y;
	case SDLK_q: return PAD_L;
	case SDLK_w: return PAD_R;
	case SDLK_RETURN: return PAD_START;
	case SDLK_RSHIFT: return PAD_SELECT;
	default: return 0;
	}
}

static uint16_t pad_buttons(SDL_GameController* gc) {
	if (!gc) return 0;
	static const struct { SDL_GameControllerButton b; uint16_t bit; } map[] = {
		{ SDL_CONTROLLER_BUTTON_A, PAD_B }, { SDL_CONTROLLER_BUTTON_B, PAD_A },
		{ SDL_CONTROLLER_BUTTON_X, PAD_Y }, { SDL_CONTROLLER_BUTTON_Y, PAD_X },
		{ SDL_CONTROLLER_BUTTON_LEFTSHOULDER, PAD_L }, { SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, PAD_R },
		{ SDL_CONTROLLER_BUTTON_START, PAD_START }, { SDL_CONTROLLER_BUTTON_BACK, PAD_SELECT },
		{ SDL_CONTROLLER_BUTTON_DPAD_UP, PAD_UP }, { SDL_CONTROLLER_BUTTON_DPAD_DOWN, PAD_DOWN },
		{ SDL_CONTROLLER_BUTTON_DPAD_LEFT, PAD_LEFT }, { SDL_CONTROLLER_BUTTON_DPAD_RIGHT, PAD_RIGHT },
	};
	uint16_t pad = 0;
	for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
		if (SDL_GameControllerGetButton(gc, map[i].b)) pad |= map[i].bit;
	}
	int lx = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTX);
	int ly = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTY);
	if (lx < -16000) pad |= PAD_LEFT;
	if (lx > 16000) pad |= PAD_RIGHT;
	if (ly < -16000) pad |= PAD_UP;
	if (ly > 16000) pad |= PAD_DOWN;
	return pad;
}

/* the largest whole-number scale of the picture inside a screen, centred */
static SDL_Rect fit(int sx, int sy) {
	int k = opt.screenW / CORE_W;
	if (opt.screenH / CORE_H < k) k = opt.screenH / CORE_H;
	if (k < 1) k = 1;
	SDL_Rect r = { sx + (opt.screenW - CORE_W * k) / 2, sy + (opt.screenH - CORE_H * k) / 2, CORE_W * k, CORE_H * k };
	return r;
}

/* both screens into a PNG (for --shot) */
static void save_shot(const char* p, const uint32_t* top, const uint32_t* bottom) {
	int w = opt.side ? CORE_W * 2 : CORE_W, h = opt.side ? CORE_H : CORE_H * 2;
	uint8_t* rgb = malloc((size_t)w * h * 3);
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			bool second = opt.side ? x >= CORE_W : y >= CORE_H;
			int sx = opt.side && second ? x - CORE_W : x, sy = !opt.side && second ? y - CORE_H : y;
			uint32_t c = (second ? bottom : top)[sy * CORE_W + sx];
			uint8_t* d = rgb + ((size_t)y * w + x) * 3;
			d[0] = c >> 16;
			d[1] = c >> 8;
			d[2] = c;
		}
	}
	stbi_write_png(p, w, h, 3, rgb, w * 3);
	free(rgb);
	printf("shot %s\n", p);
}

int main(int argc, char** argv) {
	parse_args(argc, argv);
	if (!opt.romPath) {
		fprintf(stderr, "usage: snes-ds [options] ROM (see src/main.c)\n");
		return 2;
	}
	size_t romSize;
	uint8_t* rom = read_file(opt.romPath, &romSize);
	if (!rom) {
		fprintf(stderr, "can't read %s\n", opt.romPath);
		return 1;
	}
	Core* game = core_new();
	if (!game || !core_load_rom(game, rom, romSize)) {
		fprintf(stderr, "not a SNES ROM: %s\n", opt.romPath);
		return 1;
	}
	const Profile* profile = profile_find(rom, romSize);
	printf("%s (crc32 %08X): %s\n", core_title(game), rom_crc32(rom, romSize),
	       profile ? profile->title : "no second-screen profile");

	char srmPath[1024];
	save_path(srmPath, sizeof(srmPath), ".srm");
	size_t sramSize = core_sram(game, NULL);
	uint8_t* sram = sramSize ? calloc(1, sramSize) : NULL;
	if (sram) {
		size_t n;
		uint8_t* d = read_file(srmPath, &n);
		if (d && core_load_sram(game, d, n)) printf("loaded %s\n", srmPath);
		free(d);
		core_sram(game, sram);
	}
	if (opt.loadState) {
		size_t n;
		uint8_t* d = read_file(opt.loadState, &n);
		if (!d || !core_load_state(game, d, n)) fprintf(stderr, "can't load state %s\n", opt.loadState);
		free(d);
	}

	Screen2* s2 = screen2_new(profile, rom, romSize, !opt.noMap);

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
		fprintf(stderr, "SDL: %s\n", SDL_GetError());
		return 1;
	}
	int winW = opt.side ? opt.screenW * 2 : opt.screenW;
	int winH = opt.side ? opt.screenH : opt.screenH * 2;
	char title[128];
	snprintf(title, sizeof(title), "snes-ds - %s", profile ? profile->title : core_title(game));
	SDL_Window* win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, winW, winH,
	                                   opt.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
	SDL_Renderer* ren = win ? SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED) : NULL;
	if (!ren) ren = win ? SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE) : NULL;
	if (!ren) {
		fprintf(stderr, "SDL: %s\n", SDL_GetError());
		return 1;
	}
	SDL_RenderSetLogicalSize(ren, winW, winH);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	SDL_Texture* texTop = SDL_CreateTexture(ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, CORE_W, CORE_H);
	SDL_Texture* texBottom = SDL_CreateTexture(ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, CORE_W, CORE_H);

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

	uint16_t keys = 0;
	bool quit = false, swapped = false, fast = false, splitOn = true;
	int16_t samples[SAMPLES_PER_FRAME * 2];
	uint64_t freq = SDL_GetPerformanceFrequency(), next = SDL_GetPerformanceCounter();
	const double frameTime = 1.0 / 60.0988;
	long frame = 0;
	uint32_t sramCheck = 0;
	char statePath[1024];
	save_path(statePath, sizeof(statePath), ".state1");

	while (!quit) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_QUIT: quit = true; break;
			case SDL_CONTROLLERDEVICEADDED:
				if (!gc) gc = SDL_GameControllerOpen(e.cdevice.which);
				break;
			case SDL_KEYDOWN:
			case SDL_KEYUP: {
				bool down = e.type == SDL_KEYDOWN;
				SDL_Keycode k = e.key.keysym.sym;
				keys = down ? keys | key_button(k) : keys & ~key_button(k);
				if (k == SDLK_TAB) fast = down;
				if (!down || e.key.repeat) break;
				if (k == SDLK_ESCAPE) quit = true;
				else if (k == SDLK_F2) swapped = !swapped;
				else if (k == SDLK_F3) {
					splitOn = !splitOn;
					screen2_set_split(s2, splitOn);
				} else if (k == SDLK_F5) {
					size_t n = core_state_size(game);
					uint8_t* d = malloc(n);
					core_save_state(game, d);
					printf(write_file(statePath, d, n) ? "saved %s\n" : "can't save %s\n", statePath);
					free(d);
				} else if (k == SDLK_F7) {
					size_t n;
					uint8_t* d = read_file(statePath, &n);
					if (d && core_load_state(game, d, n)) {
						screen2_reset(s2);
						printf("loaded %s\n", statePath);
					}
					free(d);
				} else if (k == SDLK_F11) {
					opt.fullscreen = !opt.fullscreen;
					SDL_SetWindowFullscreen(win, opt.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
				}
				break;
			}
			}
		}

		core_set_pad(game, 0, keys | pad_buttons(gc));
		screen2_before_frame(s2, game);
		core_run_frame(game);
		screen2_after_frame(s2, game);
		core_audio(game, samples, SAMPLES_PER_FRAME);
		frame++;

		/* battery saves: written a few seconds after they change */
		if (sram && ++sramCheck % 300 == 0) {
			uint8_t* now = malloc(sramSize);
			core_sram(game, now);
			if (memcmp(now, sram, sramSize)) {
				memcpy(sram, now, sramSize);
				write_file(srmPath, sram, sramSize);
			}
			free(now);
		}

		if (opt.frames && frame >= opt.frames) {
			if (opt.shot) save_shot(opt.shot, core_frame(game), screen2_picture(s2));
			quit = true;
		}

		/* fast forward: draw every 4th frame, no audio, no waiting */
		if (fast && frame % 4) continue;

		SDL_UpdateTexture(texTop, NULL, core_frame(game), CORE_W * 4);
		SDL_UpdateTexture(texBottom, NULL, screen2_picture(s2), CORE_W * 4);
		SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
		SDL_RenderClear(ren);
		SDL_Rect a = fit(0, 0);
		SDL_Rect b = opt.side ? fit(opt.screenW, 0) : fit(0, opt.screenH);
		SDL_RenderCopy(ren, swapped ? texBottom : texTop, NULL, &a);
		SDL_RenderCopy(ren, swapped ? texTop : texBottom, NULL, &b);
		SDL_RenderPresent(ren);

		if (fast) {
			if (audio) SDL_ClearQueuedAudio(audio);
			next = SDL_GetPerformanceCounter();
			continue;
		}
		/* pace by the audio queue when there is one, else by the clock */
		if (audio) {
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
		core_sram(game, sram);
		write_file(srmPath, sram, sramSize);
	}
	screen2_free(s2);
	core_free(game);
	free(rom);
	free(sram);
	if (gc) SDL_GameControllerClose(gc);
	if (audio) SDL_CloseAudioDevice(audio);
	SDL_DestroyTexture(texTop);
	SDL_DestroyTexture(texBottom);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}
