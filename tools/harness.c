/* Headless test harness: drives the core frame by frame from a script, to
 * find a game's memory flags and check second-screen ideas without the
 * device (like gba-ds' tools/harness.c).
 *
 *   harness ROM OUTDIR SCRIPT        (SCRIPT "-" reads stdin)
 *
 * Script commands, one per line (# starts a comment):
 *   wait N                     run N frames, no buttons
 *   hold BUTTONS N             run N frames holding BUTTONS (A+B+UP...; - none)
 *   tap BUTTONS                hold 4 frames, release 4
 *   until ADDR MASK VAL MAX [BUTTONS]
 *                              run until (byte & MASK) == VAL, at most MAX frames
 *   shot NAME                  OUTDIR/NAME.png
 *   layershot NAME             NAME_bg1..bg4, _obj, _nobg3 (next frame, each alone)
 *   hide MASK [Y0 Y1]          leave layers out (1 BG1, 2 BG2, 4 BG3, 8 BG4, 16 OBJ)
 *   split MASK [Y0 Y1]         draw those layers in the split picture instead
 *   splitbox                   where the split picture has pixels
 *   splitshot NAME             the split picture: NAME.png (transparent where
 *                              empty; on dark grey in the sheet)
 *   peek ADDR [LEN]            print bytes
 *   expect ADDR HEX...         fail (exit 1) unless the bytes from ADDR are these
 *   poke ADDR VAL              write a byte
 *   watch ADDR [LEN]           print those bytes whenever they change (unwatch: stop)
 *   dumpram NAME               NAME.wram / .vram / .cgram / .oam
 *   save NAME / load NAME      OUTDIR/NAME.state
 *   savesram NAME / loadsram NAME   OUTDIR/NAME.srm
 *   sheet NAME                 the shots since the last sheet, 4 per row, in NAME.png
 *   scale N                    shot size multiplier (default 1)
 *   bench N                    run N frames as fast as possible, print fps
 *   map NAME [RESULT]          run the profile's map capture on a hidden core:
 *                              NAME_0..N.png and the result; fail unless it's
 *                              RESULT (ok, not_live, no_map, unsettled, empty)
 *   echo TEXT / frame / quit
 * ADDR is hex; below 0x20000 it's an offset into WRAM ($7E0000), so 0x10
 * means $7E0010. Full addresses ($7EF36D, 0x00FFC0) work too.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "core.h"
#include "mapcap.h"
#include "profiles.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static Core* core;
static Core* hidden; /* for map, made on first use */
static const Profile* profile;
static uint8_t* romData;
static size_t romSize;
static const char* outdir;
static int scale = 1;
static int lineNo;

#define MAX_WATCH 16
static struct { uint32_t addr; int len; uint8_t last[16]; } watches[MAX_WATCH];
static int watchCount;

static void fail(const char* msg, const char* arg) {
	fprintf(stderr, "line %d: %s%s%s\n", lineNo, msg, arg ? ": " : "", arg ? arg : "");
	exit(1);
}

static uint32_t parse_addr(const char* s) {
	if (!s) fail("missing address", NULL);
	if (*s == '$') s++;
	uint32_t a = (uint32_t)strtoul(s, NULL, 16);
	return a < 0x20000 ? 0x7E0000 + a : a;
}

static long parse_num(const char* s) {
	if (!s) fail("missing number", NULL);
	return strtol(s, NULL, 0);
}

static uint16_t parse_buttons(const char* s) {
	static const struct { const char* name; uint16_t bit; } names[] = {
		{ "A", PAD_A }, { "B", PAD_B }, { "X", PAD_X }, { "Y", PAD_Y },
		{ "L", PAD_L }, { "R", PAD_R }, { "START", PAD_START }, { "SELECT", PAD_SELECT },
		{ "UP", PAD_UP }, { "DOWN", PAD_DOWN }, { "LEFT", PAD_LEFT }, { "RIGHT", PAD_RIGHT },
	};
	if (!s || !strcmp(s, "-")) return 0;
	uint16_t pad = 0;
	char buf[128];
	snprintf(buf, sizeof(buf), "%s", s);
	for (char* t = strtok(buf, "+"); t; t = strtok(NULL, "+")) {
		size_t i;
		for (i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
			if (!strcmp(t, names[i].name)) break;
		}
		if (i == sizeof(names) / sizeof(names[0])) fail("unknown button", t);
		pad |= names[i].bit;
	}
	return pad;
}

static void path(char* out, size_t n, const char* name, const char* ext) {
	snprintf(out, n, "%s/%s%s", outdir, name, ext);
}

static void check_watches(void) {
	for (int i = 0; i < watchCount; i++) {
		uint8_t now[16];
		for (int j = 0; j < watches[i].len; j++) now[j] = core_peek(core, watches[i].addr + j);
		if (memcmp(now, watches[i].last, watches[i].len)) {
			printf("frame %u: $%06X:", core_frame_count(core), watches[i].addr);
			for (int j = 0; j < watches[i].len; j++) printf(" %02X", now[j]);
			printf("\n");
			memcpy(watches[i].last, now, watches[i].len);
		}
	}
}

static void step(uint16_t pad) {
	core_set_pad(core, 0, pad);
	core_run_frame(core);
	check_watches();
}

static void run(uint16_t pad, long frames) {
	for (long i = 0; i < frames; i++) step(pad);
	core_set_pad(core, 0, 0);
}

#define MAX_SHEET 32
static uint32_t* sheetFrames[MAX_SHEET];
static int sheetCount;

static void write_png(const char* name) {
	if (sheetCount < MAX_SHEET) {
		sheetFrames[sheetCount] = malloc(sizeof(uint32_t) * CORE_W * CORE_H);
		memcpy(sheetFrames[sheetCount++], core_frame(core), sizeof(uint32_t) * CORE_W * CORE_H);
	}
	char p[1024];
	path(p, sizeof(p), name, ".png");
	int w = CORE_W * scale, h = CORE_H * scale;
	uint8_t* rgb = malloc((size_t)w * h * 3);
	const uint32_t* f = core_frame(core);
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			uint32_t c = f[(y / scale) * CORE_W + x / scale];
			uint8_t* d = rgb + ((size_t)y * w + x) * 3;
			d[0] = c >> 16;
			d[1] = c >> 8;
			d[2] = c;
		}
	}
	if (!stbi_write_png(p, w, h, 3, rgb, w * 3)) fail("can't write", p);
	free(rgb);
	printf("shot %s\n", p);
}

static void write_split(const char* name) {
	char p[1024];
	path(p, sizeof(p), name, ".png");
	const uint32_t* f = core_split_frame(core);
	uint8_t* rgba = malloc((size_t)CORE_W * CORE_H * 4);
	if (sheetCount < MAX_SHEET) sheetFrames[sheetCount] = malloc(sizeof(uint32_t) * CORE_W * CORE_H);
	for (int i = 0; i < CORE_W * CORE_H; i++) {
		uint32_t c = f[i];
		rgba[i * 4 + 0] = c >> 16;
		rgba[i * 4 + 1] = c >> 8;
		rgba[i * 4 + 2] = c;
		rgba[i * 4 + 3] = c >> 24;
		if (sheetCount < MAX_SHEET) sheetFrames[sheetCount][i] = c >> 24 ? c & 0xFFFFFF : 0x202020;
	}
	if (sheetCount < MAX_SHEET) sheetCount++;
	if (!stbi_write_png(p, CORE_W, CORE_H, 4, rgba, CORE_W * 4)) fail("can't write", p);
	free(rgba);
	printf("splitshot %s\n", p);
}

/* the shots since the last sheet in a grid, 4 to a row, 2 px apart */
static void write_sheet(const char* name) {
	if (!sheetCount) return;
	int cols = sheetCount < 4 ? sheetCount : 4, rows = (sheetCount + 3) / 4;
	int w = cols * (CORE_W + 2) - 2, h = rows * (CORE_H + 2) - 2;
	uint8_t* rgb = calloc((size_t)w * h, 3);
	for (int i = 0; i < sheetCount; i++) {
		int ox = (i % 4) * (CORE_W + 2), oy = (i / 4) * (CORE_H + 2);
		for (int y = 0; y < CORE_H; y++) {
			for (int x = 0; x < CORE_W; x++) {
				uint32_t c = sheetFrames[i][y * CORE_W + x];
				uint8_t* d = rgb + ((size_t)(oy + y) * w + ox + x) * 3;
				d[0] = c >> 16;
				d[1] = c >> 8;
				d[2] = c;
			}
		}
		free(sheetFrames[i]);
	}
	char p[1024];
	path(p, sizeof(p), name, ".png");
	if (!stbi_write_png(p, w, h, 3, rgb, w * 3)) fail("can't write", p);
	free(rgb);
	printf("sheet %s (%d shots)\n", p, sheetCount);
	sheetCount = 0;
}

static void write_file(const char* name, const char* ext, const void* data, size_t n) {
	char p[1024];
	path(p, sizeof(p), name, ext);
	FILE* f = fopen(p, "wb");
	if (!f || fwrite(data, 1, n, f) != n) fail("can't write", p);
	fclose(f);
}

static uint8_t* read_file(const char* p, size_t* size) {
	FILE* f = fopen(p, "rb");
	if (!f) return NULL;
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	fseek(f, 0, SEEK_SET);
	uint8_t* data = malloc(n > 0 ? n : 1);
	if (fread(data, 1, n, f) != (size_t)n) {
		free(data);
		fclose(f);
		return NULL;
	}
	fclose(f);
	*size = (size_t)n;
	return data;
}

static void layershot(const char* name) {
	static const struct { const char* suffix; uint8_t hide; } views[] = {
		{ "_bg1", 0x1E }, { "_bg2", 0x1D }, { "_bg3", 0x1B }, { "_bg4", 0x17 },
		{ "_obj", 0x0F }, { "_nobg3", 0x04 }, { "_all", 0x00 },
	};
	size_t n = core_state_size(core);
	uint8_t* st = malloc(n);
	core_save_state(core, st);
	uint32_t frames = core_frame_count(core);
	for (size_t i = 0; i < sizeof(views) / sizeof(views[0]); i++) {
		core_load_state(core, st, n);
		core_hide_layers(core, views[i].hide, 0, 240);
		core_set_pad(core, 0, 0);
		core_run_frame(core);
		char full[256];
		snprintf(full, sizeof(full), "%s%s", name, views[i].suffix);
		write_png(full);
	}
	core_hide_layers(core, 0, 0, 240);
	core_load_state(core, st, n);
	(void)frames;
	free(st);
}

static void command(char* line) {
	char* hash = strchr(line, '#');
	if (hash) *hash = 0;
	char* argv[8] = { 0 };
	int argc = 0;
	for (char* t = strtok(line, " \t\r\n"); t && argc < 8; t = strtok(NULL, " \t\r\n")) argv[argc++] = t;
	if (!argc) return;
	const char* cmd = argv[0];

	if (!strcmp(cmd, "wait")) {
		run(0, parse_num(argv[1]));
	} else if (!strcmp(cmd, "hold")) {
		run(parse_buttons(argv[1]), parse_num(argv[2]));
	} else if (!strcmp(cmd, "tap")) {
		run(parse_buttons(argv[1]), 4);
		run(0, 4);
	} else if (!strcmp(cmd, "until")) {
		uint32_t a = parse_addr(argv[1]);
		uint8_t mask = (uint8_t)parse_num(argv[2]), val = (uint8_t)parse_num(argv[3]);
		long max = parse_num(argv[4]);
		uint16_t pad = parse_buttons(argv[5]);
		long i;
		for (i = 0; i < max && (core_peek(core, a) & mask) != val; i++) step(pad);
		core_set_pad(core, 0, 0);
		if ((core_peek(core, a) & mask) != val) {
			printf("until $%06X: timed out after %ld frames (%02X)\n", a, max, core_peek(core, a));
		} else {
			printf("until $%06X: %ld frames\n", a, i);
		}
	} else if (!strcmp(cmd, "shot")) {
		write_png(argv[1] ? argv[1] : "shot");
	} else if (!strcmp(cmd, "layershot")) {
		layershot(argv[1] ? argv[1] : "layers");
	} else if (!strcmp(cmd, "hide")) {
		int y0 = argv[2] ? (int)parse_num(argv[2]) : 0, y1 = argv[3] ? (int)parse_num(argv[3]) : 240;
		core_hide_layers(core, 0, 0, 240);
		core_hide_layers(core, (uint8_t)parse_num(argv[1]), y0, y1);
	} else if (!strcmp(cmd, "split")) {
		int y0 = argv[2] ? (int)parse_num(argv[2]) : 0, y1 = argv[3] ? (int)parse_num(argv[3]) : 240;
		core_split_layers(core, 0, 0, 240);
		core_split_layers(core, (uint8_t)parse_num(argv[1]), y0, y1);
	} else if (!strcmp(cmd, "splitbox")) {
		/* where the split picture has pixels: per band of 8 lines, and overall */
		const uint32_t* f = core_split_frame(core);
		int x0 = CORE_W, y0 = CORE_H, x1 = -1, y1 = -1;
		for (int y = 0; y < CORE_H; y++) {
			int n = 0;
			for (int x = 0; x < CORE_W; x++) {
				if (!(f[y * CORE_W + x] >> 24)) continue;
				n++;
				if (x < x0) x0 = x;
				if (x > x1) x1 = x;
				if (y < y0) y0 = y;
				if (y > y1) y1 = y;
			}
			if (n) printf("  line %3d: %d px\n", y, n);
		}
		printf("splitbox: x %d-%d, y %d-%d\n", x0, x1, y0, y1);
	} else if (!strcmp(cmd, "splitshot")) {
		write_split(argv[1] ? argv[1] : "split");
	} else if (!strcmp(cmd, "peek")) {
		uint32_t a = parse_addr(argv[1]);
		long len = argv[2] ? parse_num(argv[2]) : 1;
		for (long i = 0; i < len; i++) {
			if (i % 16 == 0) printf("%s$%06X:", i ? "\n" : "", a + (uint32_t)i);
			printf(" %02X", core_peek(core, a + (uint32_t)i));
		}
		printf("\n");
	} else if (!strcmp(cmd, "expect")) {
		/* expect ADDR VAL...: consecutive bytes, or the script fails */
		uint32_t a = parse_addr(argv[1]);
		for (int i = 2; i < argc; i++) {
			uint8_t want = (uint8_t)strtoul(argv[i], NULL, 16), got = core_peek(core, a + i - 2);
			if (got != want) {
				char msg[64];
				snprintf(msg, sizeof(msg), "$%06X is %02X, expected %02X", a + i - 2, got, want);
				fail("expect failed", msg);
			}
		}
		printf("expect $%06X ok\n", a);
	} else if (!strcmp(cmd, "poke")) {
		core_poke(core, parse_addr(argv[1]), (uint8_t)parse_num(argv[2]));
	} else if (!strcmp(cmd, "watch")) {
		if (watchCount == MAX_WATCH) fail("too many watches", NULL);
		watches[watchCount].addr = parse_addr(argv[1]);
		watches[watchCount].len = argv[2] ? (int)parse_num(argv[2]) : 1;
		if (watches[watchCount].len > 16) watches[watchCount].len = 16;
		for (int j = 0; j < watches[watchCount].len; j++)
			watches[watchCount].last[j] = core_peek(core, watches[watchCount].addr + j);
		watchCount++;
	} else if (!strcmp(cmd, "unwatch")) {
		watchCount = 0;
	} else if (!strcmp(cmd, "dumpram")) {
		const char* n = argv[1] ? argv[1] : "ram";
		write_file(n, ".wram", core_wram(core), 0x20000);
		write_file(n, ".vram", core_vram(core), 0x10000);
		write_file(n, ".cgram", core_cgram(core), 0x200);
		write_file(n, ".oam", core_oam(core), 544);
		printf("dumpram %s\n", n);
	} else if (!strcmp(cmd, "save") || !strcmp(cmd, "load")) {
		char p[1024];
		path(p, sizeof(p), argv[1] ? argv[1] : "state", ".state");
		if (cmd[0] == 's') {
			size_t n = core_state_size(core);
			uint8_t* st = malloc(n);
			core_save_state(core, st);
			write_file(argv[1] ? argv[1] : "state", ".state", st, n);
			free(st);
		} else {
			size_t n;
			uint8_t* st = read_file(p, &n);
			if (!st || !core_load_state(core, st, n)) fail("can't load state", p);
			free(st);
		}
	} else if (!strcmp(cmd, "savesram") || !strcmp(cmd, "loadsram")) {
		char p[1024];
		const char* n = argv[1] ? argv[1] : "game";
		path(p, sizeof(p), n, ".srm");
		if (cmd[0] == 's') {
			size_t sz = core_sram(core, NULL);
			uint8_t* d = malloc(sz ? sz : 1);
			core_sram(core, d);
			write_file(n, ".srm", d, sz);
			free(d);
		} else {
			size_t sz;
			uint8_t* d = read_file(p, &sz);
			if (!d || !core_load_sram(core, d, sz)) fail("can't load sram", p);
			free(d);
		}
	} else if (!strcmp(cmd, "sheet")) {
		write_sheet(argv[1] ? argv[1] : "sheet");
	} else if (!strcmp(cmd, "scale")) {
		scale = (int)parse_num(argv[1]);
		if (scale < 1) scale = 1;
	} else if (!strcmp(cmd, "bench")) {
		long n = parse_num(argv[1]);
		clock_t t0 = clock();
		run(0, n);
		double s = (double)(clock() - t0) / CLOCKS_PER_SEC;
		printf("bench: %ld frames in %.2f s = %.1f fps (%.0f%% of real time)\n", n, s, n / s, n / s / 60.0988 * 100);
	} else if (!strcmp(cmd, "map")) {
		const char* n = argv[1] ? argv[1] : "map";
		if (!profile || !profile->map.enabled) fail("no map for this game", NULL);
		if (!hidden) {
			hidden = core_new();
			core_load_rom(hidden, romData, romSize);
		}
		size_t sz = core_state_size(core);
		uint8_t* st = malloc(sz);
		core_save_state(core, st);
		static MapImage img;
		uint32_t f0 = core_frame_count(hidden);
		clock_t t0 = clock();
		MapResult r = mapcap_capture(hidden, &profile->map, st, sz, &img);
		double ms = (double)(clock() - t0) * 1000 / CLOCKS_PER_SEC;
		free(st);
		printf("map: %s, %d frames, %u game frames, %.0f ms\n", mapcap_result_name(r), img.frames,
		       core_frame_count(hidden) - f0, ms);
		static const char* codes[] = { "ok", "not_live", "no_map", "unsettled", "empty" };
		if (argv[2] && strcmp(argv[2], codes[r])) fail("map: expected", argv[2]);
		for (int k = 0; k < img.frames; k++) {
			char full[256], p[1024];
			snprintf(full, sizeof(full), "%s_%d", n, k);
			path(p, sizeof(p), full, ".png");
			uint8_t* rgb = malloc(CORE_W * CORE_H * 3);
			for (int i = 0; i < CORE_W * CORE_H; i++) {
				rgb[i * 3] = img.pixels[k][i] >> 16;
				rgb[i * 3 + 1] = img.pixels[k][i] >> 8;
				rgb[i * 3 + 2] = img.pixels[k][i];
			}
			stbi_write_png(p, CORE_W, CORE_H, 3, rgb, CORE_W * 3);
			free(rgb);
			if (sheetCount < MAX_SHEET) {
				sheetFrames[sheetCount] = malloc(sizeof(img.pixels[k]));
				memcpy(sheetFrames[sheetCount++], img.pixels[k], sizeof(img.pixels[k]));
			}
		}
	} else if (!strcmp(cmd, "echo")) {
		for (int i = 1; i < argc; i++) printf("%s%s", argv[i], i + 1 < argc ? " " : "\n");
	} else if (!strcmp(cmd, "frame")) {
		printf("frame %u\n", core_frame_count(core));
	} else if (!strcmp(cmd, "quit")) {
		exit(0);
	} else {
		fail("unknown command", cmd);
	}
	fflush(stdout);
}

int main(int argc, char** argv) {
	if (argc < 4) {
		fprintf(stderr, "usage: harness ROM OUTDIR SCRIPT\n");
		return 2;
	}
	outdir = argv[2];
	size_t size;
	uint8_t* rom = read_file(argv[1], &size);
	if (!rom) {
		fprintf(stderr, "can't read %s\n", argv[1]);
		return 1;
	}
	core = core_new();
	if (!core || !core_load_rom(core, rom, size)) {
		fprintf(stderr, "can't load %s\n", argv[1]);
		return 1;
	}
	romData = rom;
	romSize = size;
	profile = profile_find(rom, size);
	printf("rom: \"%s\", crc32 %08X, profile: %s\n", core_title(core), rom_crc32(rom, size),
	       profile ? profile->title : "none");
	FILE* script = strcmp(argv[3], "-") ? fopen(argv[3], "r") : stdin;
	if (!script) {
		fprintf(stderr, "can't read %s\n", argv[3]);
		return 1;
	}
	char line[512];
	while (fgets(line, sizeof(line), script)) {
		lineNo++;
		command(line);
	}
	core_free(core);
	return 0;
}
