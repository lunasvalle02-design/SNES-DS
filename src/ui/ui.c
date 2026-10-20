#include "ui.h"

#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "core.h"
#include "i18n.h"
#include "pad.h"

/* --- the look ------------------------------------------------------------ */

#define C_BG0 rgb(0x221B16)
#define C_BG1 rgb(0x141010)
#define C_SURFACE rgb(0x2E2620)
#define C_SURFACE2 rgb(0x3B3129)
#define C_LINE rgb(0x4B3E34)
#define C_TEXT rgb(0xFFF3E8)
#define C_MUTED rgb(0xC7B1A0)
#define C_FAINT rgb(0x8C786A)
#define C_ORANGE rgb(0xFF8A1F)
#define C_ORANGE_LIGHT rgb(0xFFB45E)
#define C_ORANGE_DEEP rgb(0xDB5F0A)
#define C_ON_ORANGE rgb(0x2B1405)
/* the SNES' four face buttons */
#define C_BTN_A rgb(0xE23D3D)
#define C_BTN_B rgb(0xF2BE2C)
#define C_BTN_X rgb(0x3F7BDB)
#define C_BTN_Y rgb(0x3A9E5A)

#define HEADER_H 54.0f
#define FOOTER_H 30.0f
#define MARGIN 16.0f

typedef enum { PAGE_LIBRARY, PAGE_PAUSE, PAGE_SLOTS, PAGE_SETTINGS } Page;
typedef enum { DLG_QUIT, DLG_CLOSE_GAME, DLG_OVERWRITE } DialogKind;

/* the library's header buttons, as focus values */
#define FOCUS_SETTINGS (-1)
#define FOCUS_QUIT (-2)

enum { SET_ASPECT, SET_SWAP, SET_SPLIT, SET_MAP, SET_VOLUME, SET_FPS, SET_LANG, SETTINGS_N };

struct Ui {
	Gfx* g;
	Settings* set;
	Library* lib;
	Game* game;
	char romDir[1024];
	char version[32];
	Page page, settingsFrom;
	bool slotsSave;
	int focus[4];
	float scroll, scrollTo;

	/* touch, in the bottom screen's units */
	UiTouch t;
	float tx, ty, startX, startY, lastY;
	int pressId;
	bool dragging, modal;

	/* the focus highlight glides to where it's going */
	float hx, hy, hw, hh;
	bool hSet;

	struct {
		bool open;
		DialogKind kind;
		int arg, focus;
		char title[80], msg[200], yes[32];
	} dlg;

	char toast[128];
	uint32_t toastAt;

	Image* covers;
	bool* coverTried;
	int coverCount;
	Image frame; /* the paused game's picture */
	Image slotPic[STATE_SLOTS];
	time_t slotTime[STATE_SLOTS];
	float sw[SETTINGS_N]; /* the switches' knobs, gliding */

	int battery;
	uint32_t batteryAt;
	uint32_t frames;
};

/* --- small things -------------------------------------------------------- */

static float ease(float from, float to, float k) {
	float d = to - from;
	return fabsf(d) < 0.05f ? to : from + d * k;
}

static int read_battery(void) {
	DIR* d = opendir("/sys/class/power_supply");
	if (!d) return -1;
	int cap = -1;
	for (struct dirent* e; (e = readdir(d)) && cap < 0;) {
		if (e->d_name[0] == '.') continue;
		char p[512], type[32] = "";
		snprintf(p, sizeof(p), "/sys/class/power_supply/%s/type", e->d_name);
		FILE* f = fopen(p, "r");
		if (!f) continue;
		if (!fgets(type, sizeof(type), f)) type[0] = 0;
		fclose(f);
		if (strncmp(type, "Battery", 7)) continue;
		snprintf(p, sizeof(p), "/sys/class/power_supply/%s/capacity", e->d_name);
		f = fopen(p, "r");
		if (f) {
			if (fscanf(f, "%d", &cap) != 1) cap = -1;
			fclose(f);
		}
	}
	closedir(d);
	return cap;
}

static const char* game_short(const LibGame* lg) {
	return lg->profile->shortTitle ? lg->profile->shortTitle : lg->profile->title;
}

/* the SNES pad's face buttons as a diamond: the app's mark */
static void draw_mark(Gfx* g, float cx, float cy, float s) {
	float d = s * 0.3f, r = s * 0.17f;
	gfx_circle(g, cx, cy - d, r, C_BTN_X);
	gfx_circle(g, cx + d, cy, r, C_BTN_A);
	gfx_circle(g, cx, cy + d, r, C_BTN_B);
	gfx_circle(g, cx - d, cy, r, C_BTN_Y);
}

static void draw_background(Gfx* g) {
	float w = gfx_w(g), h = gfx_h(g);
	gfx_fill_grad(g, 0, 0, w, h, 0, C_BG0, C_BG1);
	/* a warm glow in a corner */
	gfx_shadow(g, w * 0.62f, -h * 0.45f, w * 0.7f, h * 0.7f, h * 0.35f, h * 0.4f, with_alpha(C_ORANGE, 0.10f));
}

/* a button hint: the SNES button's colour and letter, then what it does */
static float draw_hint(Gfx* g, float x, float cy, const char* b, Color bc, const char* label) {
	float r = 8.5f;
	gfx_circle(g, x + r, cy, r, bc);
	gfx_text(g, FONT_BOLD, 11, x + r, cy - 5.7f, rgb(0x1A1210), ALIGN_CENTER, b);
	float w = gfx_text(g, FONT_TEXT, 12, x + r * 2 + 6, cy - 6.5f, C_MUTED, ALIGN_LEFT, label);
	return r * 2 + 6 + w + 16;
}

static void draw_footer(Gfx* g, const char* const* btn, const Color* col, const char* const* label, int n) {
	float y = gfx_h(g) - FOOTER_H;
	gfx_fill(g, 0, y, gfx_w(g), 1, 0, with_alpha(C_LINE, 0.6f));
	float x = MARGIN;
	for (int i = 0; i < n; i++) x += draw_hint(g, x, y + FOOTER_H / 2, btn[i], col[i], label[i]);
}

/* the highlight around what has the focus */
static void draw_focus(Ui* ui, float x, float y, float w, float h, float r) {
	if (!ui->hSet) {
		ui->hx = x;
		ui->hy = y;
		ui->hw = w;
		ui->hh = h;
		ui->hSet = true;
	}
	ui->hx = ease(ui->hx, x, 0.35f);
	ui->hy = ease(ui->hy, y, 0.35f);
	ui->hw = ease(ui->hw, w, 0.35f);
	ui->hh = ease(ui->hh, h, 0.35f);
	float pulse = 0.75f + 0.25f * sinf(ui->frames * 0.08f);
	gfx_glow(ui->g, ui->hx - 3, ui->hy - 3, ui->hw + 6, ui->hh + 6, r + 3, 12, with_alpha(C_ORANGE, 0.35f * pulse));
	gfx_stroke(ui->g, ui->hx - 3, ui->hy - 3, ui->hw + 6, ui->hh + 6, r + 3, 2.5f, C_ORANGE);
}

/* touch on a box: 1 while pressed on it, 2 when let go on it (a tap) */
static int hit(Ui* ui, int id, float x, float y, float w, float h, bool inDialog) {
	if (ui->modal && !inDialog) return 0;
	bool in = ui->tx >= x && ui->tx < x + w && ui->ty >= y && ui->ty < y + h;
	if (ui->t.pressed && in && ui->pressId < 0) ui->pressId = id;
	if (ui->pressId != id || ui->dragging) return 0;
	if (ui->t.released) return in ? 2 : 0;
	return ui->t.down && in ? 1 : 0;
}

static void open_dialog(Ui* ui, DialogKind kind, int arg, const char* title, const char* msg, const char* yes) {
	ui->dlg.open = true;
	ui->dlg.kind = kind;
	ui->dlg.arg = arg;
	ui->dlg.focus = 0;
	snprintf(ui->dlg.title, sizeof(ui->dlg.title), "%s", title);
	snprintf(ui->dlg.msg, sizeof(ui->dlg.msg), "%s", msg);
	snprintf(ui->dlg.yes, sizeof(ui->dlg.yes), "%s", yes);
}

static UiAction act(UiActionKind k, int arg) { return (UiAction) { k, arg }; }

static void go(Ui* ui, Page p) {
	ui->page = p;
	ui->hSet = false;
	ui->pressId = -1;
}

/* --- life ---------------------------------------------------------------- */

Ui* ui_new(Gfx* g, Settings* set, Library* lib, const char* romDir, const char* version) {
	Ui* ui = calloc(1, sizeof(Ui));
	ui->g = g;
	ui->set = set;
	ui->lib = lib;
	snprintf(ui->romDir, sizeof(ui->romDir), "%s", romDir ? romDir : "");
	snprintf(ui->version, sizeof(ui->version), "%s", version);
	ui->pressId = -1;
	ui->battery = read_battery();
	ui->batteryAt = SDL_GetTicks();
	ui_library_changed(ui);
	return ui;
}

static void free_covers(Ui* ui) {
	for (int i = 0; i < ui->coverCount; i++) gfx_image_free(&ui->covers[i]);
	free(ui->covers);
	free(ui->coverTried);
	ui->covers = NULL;
	ui->coverTried = NULL;
	ui->coverCount = 0;
}

void ui_library_changed(Ui* ui) {
	free_covers(ui);
	ui->coverCount = ui->lib->count;
	ui->covers = calloc(ui->coverCount + 1, sizeof(Image));
	ui->coverTried = calloc(ui->coverCount + 1, sizeof(bool));
	if (ui->focus[PAGE_LIBRARY] >= ui->lib->count) ui->focus[PAGE_LIBRARY] = ui->lib->count - 1;
	if (ui->focus[PAGE_LIBRARY] < 0 && ui->lib->count) ui->focus[PAGE_LIBRARY] = 0;
	if (!ui->lib->count) ui->focus[PAGE_LIBRARY] = FOCUS_SETTINGS;
}

void ui_free(Ui* ui) {
	if (!ui) return;
	free_covers(ui);
	gfx_image_free(&ui->frame);
	for (int i = 0; i < STATE_SLOTS; i++) gfx_image_free(&ui->slotPic[i]);
	free(ui);
}

void ui_show_library(Ui* ui) {
	ui->game = NULL;
	ui->dlg.open = false;
	go(ui, PAGE_LIBRARY);
}

static void load_slots(Ui* ui) {
	for (int i = 0; i < STATE_SLOTS; i++) {
		gfx_image_free(&ui->slotPic[i]);
		ui->slotTime[i] = game_state_time(ui->game, i + 1);
		uint32_t* px = ui->slotTime[i] ? game_state_picture(ui->game, i + 1) : NULL;
		if (px) gfx_image_pixels(ui->g, px, CORE_W, CORE_H, &ui->slotPic[i]);
		free(px);
	}
}

void ui_show_pause(Ui* ui, Game* game) {
	ui->game = game;
	ui->dlg.open = false;
	gfx_image_pixels(ui->g, core_frame(game->core), CORE_W, CORE_H, &ui->frame);
	ui->focus[PAGE_PAUSE] = 0;
	go(ui, PAGE_PAUSE);
}

void ui_toast(Ui* ui, const char* msg) {
	snprintf(ui->toast, sizeof(ui->toast), "%s", msg);
	ui->toastAt = SDL_GetTicks();
}

/* --- the top screen's pieces --------------------------------------------- */

static void draw_status(Ui* ui) {
	Gfx* g = ui->g;
	float w = gfx_w(g);
	char clock[16];
	time_t now = time(NULL);
	struct tm* tm = localtime(&now);
	strftime(clock, sizeof(clock), "%H:%M", tm);
	gfx_text(g, FONT_BOLD, 13, w - MARGIN, 10, C_MUTED, ALIGN_RIGHT, clock);
	if (SDL_GetTicks() - ui->batteryAt > 30000) {
		ui->battery = read_battery();
		ui->batteryAt = SDL_GetTicks();
	}
	if (ui->battery >= 0) {
		float cw = gfx_text_w(g, FONT_BOLD, 13, clock);
		float bx = w - MARGIN - cw - 50, by = 12;
		char pct[8];
		snprintf(pct, sizeof(pct), "%d%%", ui->battery);
		gfx_stroke(g, bx, by, 20, 11, 3, 1.4f, C_MUTED);
		gfx_fill(g, bx + 20.5f, by + 3.5f, 2, 4, 1, C_MUTED);
		Color fill = ui->battery <= 15 ? C_BTN_A : C_MUTED;
		gfx_fill(g, bx + 2.5f, by + 2.5f, 15 * ui->battery / 100.0f, 6, 1.5f, fill);
		gfx_text(g, FONT_BOLD, 12, bx - 5, 10.5f, C_MUTED, ALIGN_RIGHT, pct);
	}
}

static void draw_wordmark(Gfx* g, float x, float y, float size) {
	draw_mark(g, x + size * 0.55f, y + size * 0.5f, size * 1.1f);
	float tx = x + size * 1.35f;
	float w = gfx_text(g, FONT_BOLD, size, tx, y, C_TEXT, ALIGN_LEFT, "SNES ");
	gfx_text(g, FONT_BOLD, size, tx + w, y, C_ORANGE, ALIGN_LEFT, "DS");
}

/* a game's cover, or one made up: orange, with its title */
static void draw_cover(Ui* ui, int i, float x, float y, float w, float h, float r, float titleSize) {
	Gfx* g = ui->g;
	LibGame* lg = &ui->lib->games[i];
	if (i < ui->coverCount && !ui->coverTried[i]) {
		ui->coverTried[i] = true;
		if (lg->cover && !gfx_image_load(g, lg->cover, &ui->covers[i])) fprintf(stderr, "can't load the cover %s\n", lg->cover);
	}
	if (i < ui->coverCount && ui->covers[i].tex) {
		gfx_fill(g, x, y, w, h, r, rgb(0x0E0B09));
		gfx_image(g, &ui->covers[i], x, y, w, h, r, false, 1);
		return;
	}
	gfx_fill_grad(g, x, y, w, h, r, C_ORANGE_LIGHT, C_ORANGE_DEEP);
	draw_mark(g, x + w - h * 0.16f, y + h - h * 0.16f, h * 0.2f);
	float pad = w * 0.09f;
	gfx_text_wrap(g, FONT_BOLD, titleSize, x + pad, y + pad, w - pad * 2, 3, titleSize * 1.12f, C_ON_ORANGE, ALIGN_LEFT,
	              game_short(lg));
}

/* a game picture (the paused one, a slot's), dimmed, at the aspect it's
 * played at */
static void draw_game_picture(Ui* ui, const Image* pic, float dim) {
	Gfx* g = ui->g;
	float w = gfx_w(g), h = gfx_h(g);
	gfx_fill(g, 0, 0, w, h, 0, rgb(0x000000));
	float aspect = ui->set->wide ? 4.0f / 3 : (float)CORE_W / CORE_H;
	float pw = h * aspect, ph = h;
	if (pw > w) {
		pw = w;
		ph = w / aspect;
	}
	if (pic->tex) {
		/* stretched to the box: gfx_image keeps the picture's own aspect */
		Image im = *pic;
		im.w = (int)(pw * 100);
		im.h = (int)(ph * 100);
		gfx_image(g, &im, (w - pw) / 2, (h - ph) / 2, pw, ph, 0, false, 1);
	}
	gfx_fill(g, 0, 0, w, h, 0, with_alpha(rgb(0x0C0806), dim));
}

static void draw_top_library(Ui* ui) {
	Gfx* g = ui->g;
	float w = gfx_w(g), h = gfx_h(g);
	draw_background(g);
	draw_wordmark(g, MARGIN, 9, 17);
	draw_status(ui);
	int f = ui->focus[PAGE_LIBRARY];
	if (ui->lib->count == 0) {
		draw_mark(g, w / 2, h * 0.42f, 90);
		gfx_text(g, FONT_BOLD, 26, w / 2, h * 0.62f, C_TEXT, ALIGN_CENTER, "SNES DS");
		gfx_text(g, FONT_TEXT, 13, w / 2, h * 0.62f + 34, C_FAINT, ALIGN_CENTER, ui->version);
		return;
	}
	if (f < 0) f = 0;
	LibGame* lg = &ui->lib->games[f];
	/* the cover on the left, the rest beside it */
	float cy = 50, ch = h - cy - 36, cw = ch * 0.78f;
	if (cw > w * 0.44f) {
		cw = w * 0.44f;
		ch = cw / 0.78f;
	}
	if (ui->covers[f].tex) {
		float a = (float)ui->covers[f].w / ui->covers[f].h;
		cw = ch * a;
		if (cw > w * 0.5f) {
			cw = w * 0.5f;
			ch = cw / a;
		}
	}
	float cx = MARGIN + 6;
	gfx_shadow(g, cx, cy + 6, cw, ch, 14, 22, with_alpha(rgb(0x000000), 0.55f));
	draw_cover(ui, f, cx, cy, cw, ch, 14, 24);
	float tx = cx + cw + 24, tw = w - tx - MARGIN;
	float y = cy + 4;
	/* "dual screen": what the profile brings */
	const char* dual = T("DUAL SCREEN", "DOBLE PANTALLA");
	float pill = gfx_text_w(g, FONT_BOLD, 11, dual) + 20;
	gfx_fill(g, tx, y, pill, 22, 11, with_alpha(C_ORANGE, 0.18f));
	gfx_text(g, FONT_BOLD, 11, tx + 10, y + 5, C_ORANGE_LIGHT, ALIGN_LEFT, dual);
	y += 34;
	int lines = gfx_text_wrap(g, FONT_BOLD, 22, tx, y, tw, 3, 26, C_TEXT, ALIGN_LEFT, lg->profile->title);
	y += lines * 26 + 10;
	/* at the foot: the file and how to start it */
	float by = h - 30, ny = by - 38;
	const char* about = lg->profile->about[i18n_lang] ? lg->profile->about[i18n_lang] : lg->profile->about[LANG_EN];
	if (about) {
		int room = (int)((ny - 6 - y) / 18);
		if (room > 5) room = 5;
		if (room > 0) gfx_text_wrap(g, FONT_TEXT, 13, tx, y, tw, room, 18, C_MUTED, ALIGN_LEFT, about);
	}
	gfx_text_fit(g, FONT_TEXT, 11, tx, ny, tw, C_FAINT, ALIGN_LEFT, lg->name);
	const char* play = T("Play", "Jugar");
	gfx_fill(g, tx, by - 16, gfx_text_w(g, FONT_BOLD, 14, play) + 50, 32, 16, C_SURFACE);
	gfx_circle(g, tx + 17, by, 10, C_BTN_A);
	gfx_text(g, FONT_BOLD, 12, tx + 17, by - 6.5f, rgb(0x1A1210), ALIGN_CENTER, "A");
	gfx_text(g, FONT_BOLD, 14, tx + 34, by - 8.5f, C_TEXT, ALIGN_LEFT, play);
	(void)lines;
}

static const char* const setting_names[SETTINGS_N][LANGS] = {
	{ "Aspect ratio", "Formato de imagen" },
	{ "Swap screens", "Intercambiar pantallas" },
	{ "HUD and menus below", "HUD y menús abajo" },
	{ "Map on the bottom screen", "Mapa en la pantalla de abajo" },
	{ "Volume", "Volumen" },
	{ "Show FPS", "Mostrar FPS" },
	{ "Language", "Idioma" },
};
static const char* const setting_help[SETTINGS_N][LANGS] = {
	{ "Original: the SNES picture as it is, with whole, sharp pixels. 4:3: stretched as on a TV, filling the "
	  "RG DS screens.",
	  "Original: la imagen de la SNES tal cual, con píxeles enteros y nítidos. 4:3: estirada como en un "
	  "televisor, ocupando las pantallas de la RG DS." },
	{ "The game on the bottom screen, and the second screen (map, HUD) on top.",
	  "El juego en la pantalla de abajo y la segunda pantalla (mapa, HUD) arriba." },
	{ "Moves the game's HUD and menus to the second screen, leaving the game screen clear.",
	  "Lleva el marcador y los menús del juego a la segunda pantalla, y deja la del juego despejada." },
	{ "A second core opens the game's map in the background, so it's always in view. Turn it off if you notice "
	  "stutter.",
	  "Un segundo núcleo abre el mapa del juego en segundo plano para tenerlo siempre a la vista. Desactívalo "
	  "si notas tirones." },
	{ "The volume of the game's sound.", "El volumen del sonido del juego." },
	{ "Frames per second, in a corner of the game screen.",
	  "Los fotogramas por segundo, en una esquina de la pantalla del juego." },
	{ "The language of SNES DS's screens and menus.", "El idioma de las pantallas y los menús de SNES DS." },
};

static void draw_top_panel(Gfx* g, const char* kicker, const char* title, const char* body) {
	float w = gfx_w(g), h = gfx_h(g);
	float pw = w - 2 * 40, ph = 150, px = 40, py = (h - ph) / 2;
	gfx_shadow(g, px, py + 8, pw, ph, 18, 26, with_alpha(rgb(0x000000), 0.5f));
	gfx_fill(g, px, py, pw, ph, 18, with_alpha(C_SURFACE, 0.96f));
	gfx_fill(g, px, py, 5, ph, 2.5f, C_ORANGE);
	gfx_text(g, FONT_BOLD, 11, px + 24, py + 20, C_ORANGE_LIGHT, ALIGN_LEFT, kicker);
	gfx_text_fit(g, FONT_BOLD, 21, px + 24, py + 38, pw - 48, C_TEXT, ALIGN_LEFT, title);
	gfx_text_wrap(g, FONT_TEXT, 13, px + 24, py + 72, pw - 48, 4, 18, C_MUTED, ALIGN_LEFT, body);
}

static void draw_top(Ui* ui) {
	Gfx* g = ui->g;
	float w = gfx_w(g);
	switch (ui->page) {
	case PAGE_LIBRARY: draw_top_library(ui); break;
	case PAGE_PAUSE: {
		draw_game_picture(ui, &ui->frame, 0.35f);
		const char* paused = T("PAUSED", "EN PAUSA");
		float pw = gfx_text_w(g, FONT_BOLD, 13, paused) + 46;
		gfx_fill(g, (w - pw) / 2, 14, pw, 30, 15, with_alpha(rgb(0x120D0A), 0.85f));
		gfx_icon(g, ICON_PAUSE, (w - pw) / 2 + 19, 29, 13, C_ORANGE);
		gfx_text(g, FONT_BOLD, 13, (w - pw) / 2 + 32, 21, C_TEXT, ALIGN_LEFT, paused);
		break;
	}
	case PAGE_SLOTS: {
		int s = ui->focus[PAGE_SLOTS];
		if (ui->slotPic[s].tex) draw_game_picture(ui, &ui->slotPic[s], 0.0f);
		else draw_game_picture(ui, &ui->frame, 0.6f);
		char label[64];
		if (ui->slotTime[s]) {
			char when[32];
			strftime(when, sizeof(when), T("%Y-%m-%d  %H:%M", "%d/%m/%Y  %H:%M"), localtime(&ui->slotTime[s]));
			snprintf(label, sizeof(label), T("Slot %d  ·  %s", "Ranura %d  ·  %s"), s + 1, when);
		} else {
			snprintf(label, sizeof(label), T("Slot %d  ·  empty", "Ranura %d  ·  vacía"), s + 1);
		}
		float pw = gfx_text_w(g, FONT_BOLD, 13, label) + 32;
		gfx_fill(g, (w - pw) / 2, gfx_h(g) - 46, pw, 30, 15, with_alpha(rgb(0x120D0A), 0.85f));
		gfx_text(g, FONT_BOLD, 13, w / 2, gfx_h(g) - 39, C_TEXT, ALIGN_CENTER, label);
		break;
	}
	case PAGE_SETTINGS: {
		if (ui->game) draw_game_picture(ui, &ui->frame, 0.6f);
		else draw_background(g);
		int f = ui->focus[PAGE_SETTINGS];
		draw_top_panel(g, T("SETTINGS", "AJUSTES"), setting_names[f][i18n_lang], setting_help[f][i18n_lang]);
		break;
	}
	}
}

/* --- the bottom screen: the game list ------------------------------------ */

typedef struct {
	int cols;
	float cw, ch, coverH, gap, viewY, viewH;
} Grid;

static Grid grid(Ui* ui) {
	Grid gr;
	float w = gfx_w(ui->g);
	gr.cols = w >= 440 ? 3 : 2;
	gr.gap = 14;
	gr.cw = (w - 2 * MARGIN - (gr.cols - 1) * gr.gap) / gr.cols;
	gr.coverH = gr.cw * 0.75f;
	gr.ch = gr.coverH + 34;
	gr.viewY = HEADER_H;
	gr.viewH = gfx_h(ui->g) - HEADER_H - FOOTER_H;
	return gr;
}

static void card_pos(Ui* ui, const Grid* gr, int i, float* x, float* y) {
	*x = MARGIN + (i % gr->cols) * (gr->cw + gr->gap);
	*y = gr->viewY + 10 + (i / gr->cols) * (gr->ch + gr->gap) - ui->scroll;
}

static void keep_visible(Ui* ui, const Grid* gr, int i) {
	int rows = (ui->lib->count + gr->cols - 1) / gr->cols;
	float top = 10 + (i / gr->cols) * (gr->ch + gr->gap) - 10, bottom = top + gr->ch + 20;
	if (top < ui->scrollTo) ui->scrollTo = top;
	if (bottom > ui->scrollTo + gr->viewH) ui->scrollTo = bottom - gr->viewH;
	float max = rows * (gr->ch + gr->gap) + 10 - gr->viewH;
	if (ui->scrollTo > max) ui->scrollTo = max;
	if (ui->scrollTo < 0) ui->scrollTo = 0;
}

static UiAction nav_library(Ui* ui, uint32_t nav) {
	Grid gr = grid(ui);
	int* f = &ui->focus[PAGE_LIBRARY];
	int n = ui->lib->count;
	if (*f < 0) {
		if (nav & (NAV_LEFT | NAV_RIGHT)) *f = *f == FOCUS_SETTINGS ? FOCUS_QUIT : FOCUS_SETTINGS;
		if ((nav & NAV_DOWN) && n) *f = 0;
		if (nav & NAV_OK) {
			if (*f == FOCUS_SETTINGS) {
				ui->settingsFrom = PAGE_LIBRARY;
				go(ui, PAGE_SETTINGS);
			} else {
				open_dialog(ui, DLG_QUIT, 0, T("Quit SNES DS?", "¿Salir de SNES DS?"),
		            T("The emulator will close.", "Se cerrará el emulador."), T("Quit", "Salir"));
			}
		}
	} else if (n) {
		if ((nav & NAV_LEFT) && *f > 0) (*f)--;
		if ((nav & NAV_RIGHT) && *f < n - 1) (*f)++;
		if (nav & NAV_UP) *f = *f >= gr.cols ? *f - gr.cols : FOCUS_SETTINGS;
		if ((nav & NAV_DOWN) && *f + gr.cols < n) *f += gr.cols;
		else if ((nav & NAV_DOWN) && *f / gr.cols < (n - 1) / gr.cols) *f = n - 1;
		if (*f >= 0) keep_visible(ui, &gr, *f);
		if (nav & NAV_OK) return act(UIA_PLAY, *f);
	}
	if (nav & NAV_X) {
		ui->settingsFrom = PAGE_LIBRARY;
		go(ui, PAGE_SETTINGS);
	}
	return act(UIA_NONE, 0);
}

static int icon_button(Ui* ui, int id, Icon icon, float x, float y, float d, bool focused) {
	Gfx* g = ui->g;
	int h = hit(ui, id, x, y, d, d, false);
	gfx_circle(g, x + d / 2, y + d / 2, d / 2, h == 1 ? C_SURFACE2 : C_SURFACE);
	gfx_icon(g, icon, x + d / 2, y + d / 2, d * 0.5f, focused ? C_ORANGE : C_MUTED);
	if (focused) draw_focus(ui, x, y, d, d, d / 2);
	return h;
}

static UiAction draw_library(Ui* ui) {
	Gfx* g = ui->g;
	float w = gfx_w(g), h = gfx_h(g);
	UiAction a = act(UIA_NONE, 0);
	draw_background(g);
	Grid gr = grid(ui);
	int* f = &ui->focus[PAGE_LIBRARY];

	/* the cards (under the header, clipped to the list) */
	ui->scroll = ease(ui->scroll, ui->scrollTo, 0.3f);
	if (ui->lib->count == 0) {
		gfx_icon(g, ICON_GRID, w / 2, gr.viewY + 52, 44, C_FAINT);
		gfx_text(g, FONT_BOLD, 17, w / 2, gr.viewY + 92, C_TEXT, ALIGN_CENTER, T("No supported games", "No hay juegos compatibles"));
		char msg[1400];
		snprintf(msg, sizeof(msg),
		         T("Copy your SNES ROMs (.sfc, .smc or .zip) to %s. Dual-screen support so far:",
		           "Copia tus ROMs de SNES (.sfc, .smc o .zip) en %s. Por ahora hay doble pantalla para:"),
		         ui->romDir[0] ? ui->romDir : T("the ROM folder", "la carpeta de ROMs"));
		int l = gfx_text_wrap(g, FONT_TEXT, 12, w / 2, gr.viewY + 120, w - 60, 3, 17, C_MUTED, ALIGN_CENTER, msg);
		extern const Profile* const games[];
		float y = gr.viewY + 126 + l * 17;
		for (int i = 0; games[i] && y < h - FOOTER_H - 20; i++, y += 18)
			gfx_text_fit(g, FONT_BOLD, 12, w / 2, y, w - 60, C_ORANGE_LIGHT, ALIGN_CENTER, games[i]->title);
	}
	gfx_clip(g, 0, gr.viewY, w, gr.viewH);
	float fx = 0, fy = 0;
	for (int i = 0; i < ui->lib->count; i++) {
		float x, y;
		card_pos(ui, &gr, i, &x, &y);
		if (i == *f) {
			fx = x;
			fy = y;
		}
		if (y > gr.viewY + gr.viewH || y + gr.ch < gr.viewY) continue;
		/* only the part of the card the list shows takes taps */
		float hy0 = fmaxf(y, gr.viewY), hy1 = fminf(y + gr.ch, gr.viewY + gr.viewH);
		int ht = hit(ui, 100 + i, x, hy0, gr.cw, hy1 - hy0, false);
		if (ht == 2) {
			*f = i;
			a = act(UIA_PLAY, i);
		}
		float lift = ht == 1 ? 1.5f : 0;
		gfx_shadow(g, x, y + 5, gr.cw, gr.ch, 12, 12, with_alpha(rgb(0x000000), 0.45f));
		gfx_fill(g, x, y + lift, gr.cw, gr.ch, 12, i == *f ? C_SURFACE2 : C_SURFACE);
		draw_cover(ui, i, x + 5, y + 5 + lift, gr.cw - 10, gr.coverH - 6, 8, 15);
		gfx_text_fit(g, FONT_BOLD, 13, x + 10, y + gr.coverH + 8 + lift, gr.cw - 20, i == *f ? C_TEXT : C_MUTED,
		             ALIGN_LEFT, game_short(&ui->lib->games[i]));
	}
	if (*f >= 0 && ui->lib->count) draw_focus(ui, fx, fy, gr.cw, gr.ch, 12);
	gfx_unclip(g);

	/* drag to scroll */
	if (ui->dragging && ui->t.down) {
		ui->scrollTo -= ui->ty - ui->lastY;
		int rows = (ui->lib->count + gr.cols - 1) / gr.cols;
		float max = rows * (gr.ch + gr.gap) + 10 - gr.viewH;
		if (ui->scrollTo > max) ui->scrollTo = max;
		if (ui->scrollTo < 0) ui->scrollTo = 0;
		ui->scroll = ui->scrollTo;
	}

	/* the header */
	gfx_fill_grad(g, 0, 0, w, HEADER_H, 0, with_alpha(C_BG0, 0.98f), with_alpha(C_BG0, 0.9f));
	gfx_fill(g, 0, HEADER_H - 1, w, 1, 0, with_alpha(C_LINE, 0.6f));
	gfx_text(g, FONT_BOLD, 19, MARGIN, 10, C_TEXT, ALIGN_LEFT, T("Games", "Juegos"));
	char count[48];
	snprintf(count, sizeof(count), ui->lib->count == 1 ? T("1 dual-screen game", "1 juego con doble pantalla")
	                                                   : T("%d dual-screen games", "%d juegos con doble pantalla"),
	         ui->lib->count);
	gfx_text(g, FONT_TEXT, 11, MARGIN, 33, C_FAINT, ALIGN_LEFT, count);
	float d = 36;
	if (icon_button(ui, 1, ICON_GEAR, w - MARGIN - d * 2 - 10, 9, d, *f == FOCUS_SETTINGS) == 2) {
		*f = FOCUS_SETTINGS;
		ui->settingsFrom = PAGE_LIBRARY;
		go(ui, PAGE_SETTINGS);
	}
	if (icon_button(ui, 2, ICON_POWER, w - MARGIN - d, 9, d, *f == FOCUS_QUIT) == 2) {
		*f = FOCUS_QUIT;
		open_dialog(ui, DLG_QUIT, 0, T("Quit SNES DS?", "¿Salir de SNES DS?"),
		            T("The emulator will close.", "Se cerrará el emulador."), T("Quit", "Salir"));
	}

	static const char* const b[] = { "A", "X" };
	const char* l[] = { T("Play", "Jugar"), T("Settings", "Ajustes") };
	Color c[] = { C_BTN_A, C_BTN_X };
	draw_footer(g, b, c, l, 2);
	return a;
}

/* --- the menu over a game ------------------------------------------------ */

enum { TILE_RESUME, TILE_SAVE, TILE_LOAD, TILE_SETTINGS, TILE_CLOSE, TILE_QUIT, TILES };

static UiAction tile_action(Ui* ui, int t) {
	switch (t) {
	case TILE_RESUME: return act(UIA_RESUME, 0);
	case TILE_SAVE:
	case TILE_LOAD:
		ui->slotsSave = t == TILE_SAVE;
		load_slots(ui);
		ui->focus[PAGE_SLOTS] = 0;
		go(ui, PAGE_SLOTS);
		break;
	case TILE_SETTINGS:
		ui->settingsFrom = PAGE_PAUSE;
		go(ui, PAGE_SETTINGS);
		break;
	case TILE_CLOSE:
		open_dialog(ui, DLG_CLOSE_GAME, 0, T("Back to the game list?", "¿Volver a la lista de juegos?"),
		            T("Anything not saved (in the game or in a state) will be lost.",
		              "Lo que no hayas guardado (en el juego o en un estado) se perderá."),
		            T("Leave the game", "Salir del juego"));
		break;
	case TILE_QUIT:
		open_dialog(ui, DLG_QUIT, 0, T("Quit SNES DS?", "¿Salir de SNES DS?"),
		            T("Anything not saved (in the game or in a state) will be lost.",
		              "Lo que no hayas guardado (en el juego o en un estado) se perderá."),
		            T("Quit", "Salir"));
		break;
	}
	return act(UIA_NONE, 0);
}

static void draw_page_header(Ui* ui, const char* kicker, const char* title, bool back) {
	Gfx* g = ui->g;
	float w = gfx_w(g);
	float x = MARGIN;
	if (back) {
		int h = hit(ui, 3, 0, 0, 64, HEADER_H, false);
		gfx_circle(g, MARGIN + 17, HEADER_H / 2, 17, h == 1 ? C_SURFACE2 : C_SURFACE);
		gfx_icon(g, ICON_BACK, MARGIN + 17, HEADER_H / 2, 18, C_TEXT);
		x += 46;
	}
	gfx_text(g, FONT_BOLD, 11, x, 11, C_ORANGE_LIGHT, ALIGN_LEFT, kicker);
	gfx_text_fit(g, FONT_BOLD, 18, x, 25, w - x - MARGIN, C_TEXT, ALIGN_LEFT, title);
	gfx_fill(g, 0, HEADER_H - 1, w, 1, 0, with_alpha(C_LINE, 0.6f));
}

/* the back button was tapped this frame (draw_page_header drew it) */
static bool back_tapped(Ui* ui) {
	return !ui->modal && ui->t.released && ui->pressId == 3 && !ui->dragging && ui->tx < 64 && ui->ty < HEADER_H &&
	       ui->ty >= 0;
}

static UiAction nav_pause(Ui* ui, uint32_t nav) {
	int* f = &ui->focus[PAGE_PAUSE];
	if (nav & (NAV_LEFT | NAV_RIGHT)) *f ^= 1;
	if ((nav & NAV_UP) && *f >= 2) *f -= 2;
	if ((nav & NAV_DOWN) && *f + 2 < TILES) *f += 2;
	if (nav & (NAV_BACK | NAV_MENU)) return act(UIA_RESUME, 0);
	if (nav & NAV_OK) return tile_action(ui, *f);
	return act(UIA_NONE, 0);
}

static UiAction draw_pause(Ui* ui) {
	Gfx* g = ui->g;
	float w = gfx_w(g);
	UiAction a = act(UIA_NONE, 0);
	draw_background(g);
	const char* title = ui->game->profile && ui->game->profile->shortTitle ? ui->game->profile->shortTitle : game_title(ui->game);
	draw_page_header(ui, T("PAUSED", "EN PAUSA"), title, false);
	const char* names[TILES] = { T("Resume", "Continuar"), T("Save state", "Guardar estado"),
		                         T("Load state", "Cargar estado"), T("Settings", "Ajustes"),
		                         T("Leave the game", "Salir del juego"), T("Quit SNES DS", "Salir de SNES DS") };
	static const Icon icons[TILES] = { ICON_PLAY, ICON_SAVE, ICON_LOAD, ICON_GEAR, ICON_HOME, ICON_POWER };
	float gap = 12, tw = (w - 2 * MARGIN - gap) / 2, top = HEADER_H + 14;
	float th = (gfx_h(g) - top - FOOTER_H - 14 - gap * 2) / 3;
	int* f = &ui->focus[PAGE_PAUSE];
	float fx = 0, fy = 0;
	for (int i = 0; i < TILES; i++) {
		float x = MARGIN + (i % 2) * (tw + gap), y = top + (i / 2) * (th + gap);
		int ht = hit(ui, 10 + i, x, y, tw, th, false);
		if (ht == 2) {
			*f = i;
			a = tile_action(ui, i);
		}
		bool primary = i == TILE_RESUME;
		Color bg = primary ? C_ORANGE : C_SURFACE;
		if (ht == 1) bg = primary ? C_ORANGE_DEEP : C_SURFACE2;
		gfx_shadow(g, x, y + 4, tw, th, 14, 10, with_alpha(rgb(0x000000), 0.4f));
		if (primary) gfx_fill_grad(g, x, y, tw, th, 14, ht == 1 ? C_ORANGE_DEEP : C_ORANGE_LIGHT, bg);
		else gfx_fill(g, x, y, tw, th, 14, bg);
		Color fg = primary ? C_ON_ORANGE : i == TILE_QUIT ? rgb(0xFF8F80) : C_TEXT;
		float ic = th * 0.36f;
		if (ic > 30) ic = 30;
		gfx_fill(g, x + 14, y + (th - ic * 1.5f) / 2, ic * 1.5f, ic * 1.5f, ic * 0.75f,
		         primary ? with_alpha(rgb(0xFFFFFF), 0.3f) : with_alpha(C_ORANGE, i == TILE_QUIT ? 0.0f : 0.14f));
		gfx_icon(g, icons[i], x + 14 + ic * 0.75f, y + th / 2, ic, primary ? C_ON_ORANGE : i == TILE_QUIT ? fg : C_ORANGE);
		gfx_text_fit(g, FONT_BOLD, 15, x + 24 + ic * 1.5f, y + th / 2 - 8.5f, tw - 34 - ic * 1.5f, fg, ALIGN_LEFT, names[i]);
		if (i == *f) {
			fx = x;
			fy = y;
		}
	}
	draw_focus(ui, fx, fy, tw, th, 14);
	static const char* const b[] = { "A", "B" };
	const char* l[] = { T("Choose", "Elegir"), T("Back to the game", "Volver al juego") };
	Color c[] = { C_BTN_A, C_BTN_B };
	draw_footer(g, b, c, l, 2);
	return a;
}

/* --- save / load slots --------------------------------------------------- */

static UiAction slot_action(Ui* ui, int s) {
	if (ui->slotsSave) {
		if (ui->slotTime[s]) {
			char t[64];
			snprintf(t, sizeof(t), T("Overwrite slot %d?", "¿Sobrescribir la ranura %d?"), s + 1);
			open_dialog(ui, DLG_OVERWRITE, s + 1, t,
			            T("The state saved in it will be replaced by the current one.",
			              "El estado guardado en ella se sustituirá por el actual."),
			            T("Overwrite", "Sobrescribir"));
			return act(UIA_NONE, 0);
		}
		return act(UIA_SAVE, s + 1);
	}
	if (!ui->slotTime[s]) {
		ui_toast(ui, T("That slot is empty", "Esa ranura está vacía"));
		return act(UIA_NONE, 0);
	}
	return act(UIA_LOAD, s + 1);
}

static UiAction nav_slots(Ui* ui, uint32_t nav) {
	int* f = &ui->focus[PAGE_SLOTS];
	if (nav & (NAV_LEFT | NAV_RIGHT)) *f ^= 1;
	if (nav & (NAV_UP | NAV_DOWN)) *f ^= 2;
	if (nav & NAV_BACK) go(ui, PAGE_PAUSE);
	if (nav & NAV_MENU) return act(UIA_RESUME, 0);
	if (nav & NAV_OK) return slot_action(ui, *f);
	return act(UIA_NONE, 0);
}

static UiAction draw_slots(Ui* ui) {
	Gfx* g = ui->g;
	float w = gfx_w(g);
	UiAction a = act(UIA_NONE, 0);
	draw_background(g);
	draw_page_header(ui, ui->slotsSave ? T("SAVE STATE", "GUARDAR ESTADO") : T("LOAD STATE", "CARGAR ESTADO"),
	                 T("Choose a slot", "Elige una ranura"), true);
	if (back_tapped(ui)) go(ui, PAGE_PAUSE);
	float gap = 12, tw = (w - 2 * MARGIN - gap) / 2, top = HEADER_H + 14;
	float th = (gfx_h(g) - top - FOOTER_H - 14 - gap) / 2;
	int* f = &ui->focus[PAGE_SLOTS];
	float fx = 0, fy = 0;
	for (int i = 0; i < STATE_SLOTS; i++) {
		float x = MARGIN + (i % 2) * (tw + gap), y = top + (i / 2) * (th + gap);
		int ht = hit(ui, 20 + i, x, y, tw, th, false);
		if (ht == 2) {
			*f = i;
			a = slot_action(ui, i);
		}
		gfx_shadow(g, x, y + 4, tw, th, 14, 10, with_alpha(rgb(0x000000), 0.4f));
		gfx_fill(g, x, y, tw, th, 14, ht == 1 ? C_SURFACE2 : C_SURFACE);
		float ph = th - 20, pw = ph * CORE_W / CORE_H;
		if (pw > tw * 0.55f) {
			pw = tw * 0.55f;
			ph = pw * CORE_H / CORE_W;
		}
		float px = x + 10, py = y + (th - ph) / 2;
		if (ui->slotPic[i].tex) {
			gfx_image(g, &ui->slotPic[i], px, py, pw, ph, 8, true, 1);
		} else {
			gfx_fill(g, px, py, pw, ph, 8, rgb(0x1C1714));
			gfx_icon(g, ui->slotsSave ? ICON_SAVE : ICON_LOAD, px + pw / 2, py + ph / 2, ph * 0.32f, C_LINE);
		}
		float tx = px + pw + 12, txw = x + tw - tx - 10;
		char name[32];
		snprintf(name, sizeof(name), T("Slot %d", "Ranura %d"), i + 1);
		gfx_text_fit(g, FONT_BOLD, 15, tx, y + th / 2 - 20, txw, C_TEXT, ALIGN_LEFT, name);
		char when[32];
		snprintf(when, sizeof(when), "%s", T("Empty", "Vacía"));
		if (ui->slotTime[i]) strftime(when, sizeof(when), T("%b %d  %H:%M", "%d/%m  %H:%M"), localtime(&ui->slotTime[i]));
		gfx_text_fit(g, FONT_TEXT, 12, tx, y + th / 2 + 2, txw, ui->slotTime[i] ? C_MUTED : C_FAINT, ALIGN_LEFT, when);
		if (i == *f) {
			fx = x;
			fy = y;
		}
	}
	draw_focus(ui, fx, fy, tw, th, 14);
	static const char* const b[] = { "A", "B" };
	const char* l[] = { ui->slotsSave ? T("Save", "Guardar") : T("Load", "Cargar"), T("Back", "Atrás") };
	Color c[] = { C_BTN_A, C_BTN_B };
	draw_footer(g, b, c, l, 2);
	return a;
}

/* --- settings ------------------------------------------------------------ */

static bool setting_bool(Ui* ui, int i, bool** out) {
	Settings* s = ui->set;
	bool* p = i == SET_ASPECT ? &s->wide : i == SET_SWAP ? &s->swapped : i == SET_SPLIT ? &s->split
	        : i == SET_MAP ? &s->map : i == SET_FPS ? &s->showFps : NULL;
	*out = p;
	return p != NULL;
}

/* change setting i: dir -1 / +1 (left, right), 0 toggles */
static UiAction change_setting(Ui* ui, int i, int dir) {
	bool* b;
	if (i == SET_LANG) {
		/* left: English, right: Español */
		int v = dir < 0 ? LANG_EN : dir > 0 ? LANG_ES : (ui->set->lang + 1) % LANGS;
		if (v == ui->set->lang) return act(UIA_NONE, 0);
		ui->set->lang = v;
		i18n_lang = v; /* at once: the next frame is drawn in it */
	} else if (setting_bool(ui, i, &b)) {
		/* left: off (Original), right: on (4:3) */
		bool v = dir < 0 ? false : dir > 0 ? true : !*b;
		if (v == *b) return act(UIA_NONE, 0);
		*b = v;
	} else {
		int v = ui->set->volume + (dir ? dir : 1);
		if (!dir && v > 10) v = 0;
		if (v < 0) v = 0;
		if (v > 10) v = 10;
		if (v == ui->set->volume) return act(UIA_NONE, 0);
		ui->set->volume = v;
	}
	return act(UIA_SETTINGS, i);
}

static UiAction leave_settings(Ui* ui) {
	go(ui, ui->settingsFrom);
	return act(UIA_NONE, 0);
}

static UiAction nav_settings(Ui* ui, uint32_t nav) {
	int* f = &ui->focus[PAGE_SETTINGS];
	if ((nav & NAV_UP) && *f > 0) (*f)--;
	if ((nav & NAV_DOWN) && *f < SETTINGS_N - 1) (*f)++;
	if (nav & NAV_LEFT) return change_setting(ui, *f, -1);
	if (nav & NAV_RIGHT) return change_setting(ui, *f, 1);
	if (nav & NAV_OK) return change_setting(ui, *f, 0);
	if (nav & NAV_BACK) return leave_settings(ui);
	if (nav & NAV_MENU) return ui->game ? act(UIA_RESUME, 0) : leave_settings(ui);
	return act(UIA_NONE, 0);
}

static void draw_switch(Ui* ui, int i, float x, float y, bool on) {
	Gfx* g = ui->g;
	ui->sw[i] = ease(ui->sw[i], on ? 1 : 0, 0.3f);
	float k = ui->sw[i];
	gfx_fill(g, x, y, 44, 24, 12, color_mix(C_SURFACE2, C_ORANGE, k));
	gfx_circle(g, x + 12 + 20 * k, y + 12, 9, color_mix(C_MUTED, rgb(0xFFFFFF), k));
}

static UiAction draw_settings(Ui* ui) {
	Gfx* g = ui->g;
	float w = gfx_w(g);
	UiAction a = act(UIA_NONE, 0);
	draw_background(g);
	draw_page_header(ui, ui->game ? T("PAUSED", "EN PAUSA") : "SNES DS", T("Settings", "Ajustes"), true);
	if (back_tapped(ui)) a = leave_settings(ui);
	float top = HEADER_H + 8, rh = (gfx_h(g) - top - FOOTER_H - 8) / SETTINGS_N;
	if (rh > 44) rh = 44;
	int* f = &ui->focus[PAGE_SETTINGS];
	float rw = w - 2 * MARGIN;
	for (int i = 0; i < SETTINGS_N; i++) {
		float x = MARGIN, y = top + i * rh;
		float cx = x + rw - 12; /* the controls' right edge */
		int ht = hit(ui, 30 + i, x, y, rw - 150, rh - 4, false);
		if (i == *f) gfx_fill(g, x, y + 2, rw, rh - 4, 10, C_SURFACE);
		else if (ht == 1) gfx_fill(g, x, y + 2, rw, rh - 4, 10, with_alpha(C_SURFACE, 0.6f));
		gfx_text_fit(g, FONT_BOLD, 14, x + 12, y + rh / 2 - 8, rw - 190, i == *f ? C_TEXT : C_MUTED, ALIGN_LEFT,
		             setting_names[i][i18n_lang]);
		if (ht == 2) *f = i;
		float my = y + rh / 2;
		if (i == SET_ASPECT || i == SET_LANG) {
			/* two segments: Original | 4:3, English | Español (each language in its own words) */
			bool right = i == SET_ASPECT ? ui->set->wide : ui->set->lang == LANG_ES;
			const char* left = i == SET_ASPECT ? "Original" : "English";
			const char* rightName = i == SET_ASPECT ? "4:3" : "Español";
			float sw = i == SET_ASPECT ? 66 : 78, sx = cx - sw * 2;
			gfx_fill(g, sx, my - 13, sw * 2, 26, 13, C_SURFACE2);
			gfx_fill(g, sx + (right ? sw : 0) + 2, my - 11, sw - 4, 22, 11, C_ORANGE);
			gfx_text(g, FONT_BOLD, 12, sx + sw / 2, my - 7, right ? C_MUTED : C_ON_ORANGE, ALIGN_CENTER, left);
			gfx_text(g, FONT_BOLD, 12, sx + sw * 1.5f, my - 7, right ? C_ON_ORANGE : C_MUTED, ALIGN_CENTER, rightName);
			int id = i == SET_ASPECT ? 40 : 52;
			if (hit(ui, id, sx, y, sw, rh, false) == 2) {
				*f = i;
				a = change_setting(ui, i, -1);
			}
			if (hit(ui, id + 1, sx + sw, y, sw, rh, false) == 2) {
				*f = i;
				a = change_setting(ui, i, 1);
			}
		} else if (i == SET_VOLUME) {
			float bw = 100, bx = cx - 26 - bw - 8;
			if (hit(ui, 42, bx - 34, y, 34, rh, false) == 2) {
				*f = i;
				a = change_setting(ui, i, -1);
			}
			if (hit(ui, 43, cx - 30, y, 34, rh, false) == 2) {
				*f = i;
				a = change_setting(ui, i, 1);
			}
			gfx_circle(g, bx - 18, my, 13, C_SURFACE2);
			gfx_fill(g, bx - 24, my - 1.25f, 12, 2.5f, 1, C_TEXT);
			gfx_circle(g, cx - 13, my, 13, C_SURFACE2);
			gfx_fill(g, cx - 19, my - 1.25f, 12, 2.5f, 1, C_TEXT);
			gfx_fill(g, cx - 14.25f, my - 6, 2.5f, 12, 1, C_TEXT);
			for (int k = 0; k < 10; k++) {
				float sx = bx + k * (bw / 10);
				gfx_fill(g, sx + 1, my - 6, bw / 10 - 2, 12, 2, k < ui->set->volume ? C_ORANGE : C_SURFACE2);
			}
		} else {
			bool* b;
			setting_bool(ui, i, &b);
			draw_switch(ui, i, cx - 44, my - 12, *b);
			if (hit(ui, 44 + i, cx - 150, y, 150, rh, false) == 2) {
				*f = i;
				a = change_setting(ui, i, 0);
			}
		}
		if (ht == 2 && i != SET_VOLUME && i != SET_ASPECT && i != SET_LANG) a = change_setting(ui, i, 0);
	}
	float fy = top + *f * rh + 2;
	draw_focus(ui, MARGIN, fy, rw, rh - 4, 10);
	static const char* const b[] = { "A", "B" };
	const char* l[] = { T("Change", "Cambiar"), T("Back", "Atrás") };
	Color c[] = { C_BTN_A, C_BTN_B };
	draw_footer(g, b, c, l, 2);
	return a;
}

/* --- dialogs and toasts -------------------------------------------------- */

static UiAction dialog_yes(Ui* ui) {
	ui->dlg.open = false;
	switch (ui->dlg.kind) {
	case DLG_QUIT: return act(UIA_QUIT, 0);
	case DLG_CLOSE_GAME: return act(UIA_CLOSE_GAME, 0);
	case DLG_OVERWRITE: return act(UIA_SAVE, ui->dlg.arg);
	}
	return act(UIA_NONE, 0);
}

static UiAction nav_dialog(Ui* ui, uint32_t nav) {
	if (nav & (NAV_LEFT | NAV_RIGHT)) ui->dlg.focus ^= 1;
	if (nav & (NAV_BACK | NAV_MENU)) ui->dlg.open = false;
	if (nav & NAV_OK) {
		if (ui->dlg.focus) return dialog_yes(ui);
		ui->dlg.open = false;
	}
	return act(UIA_NONE, 0);
}

static UiAction draw_dialog(Ui* ui) {
	Gfx* g = ui->g;
	float w = gfx_w(g), h = gfx_h(g);
	UiAction a = act(UIA_NONE, 0);
	gfx_fill(g, 0, 0, w, h, 0, with_alpha(rgb(0x080504), 0.62f));
	float dw = w - 60, dh = 170, dx = 30, dy = (h - dh) / 2;
	if (dw > 400) {
		dw = 400;
		dx = (w - dw) / 2;
	}
	gfx_shadow(g, dx, dy + 8, dw, dh, 18, 24, with_alpha(rgb(0x000000), 0.6f));
	gfx_fill(g, dx, dy, dw, dh, 18, C_SURFACE);
	gfx_text_fit(g, FONT_BOLD, 17, dx + 22, dy + 20, dw - 44, C_TEXT, ALIGN_LEFT, ui->dlg.title);
	gfx_text_wrap(g, FONT_TEXT, 13, dx + 22, dy + 50, dw - 44, 3, 18, C_MUTED, ALIGN_LEFT, ui->dlg.msg);
	float bw = (dw - 44 - 12) / 2, by = dy + dh - 56;
	for (int i = 0; i < 2; i++) {
		float bx = dx + 22 + i * (bw + 12);
		int ht = hit(ui, 60 + i, bx, by, bw, 38, true);
		if (i == 1) gfx_fill(g, bx, by, bw, 38, 12, ht == 1 ? C_ORANGE_DEEP : C_ORANGE);
		else gfx_fill(g, bx, by, bw, 38, 12, ht == 1 ? C_LINE : C_SURFACE2);
		gfx_text_fit(g, FONT_BOLD, 14, bx + bw / 2, by + 11, bw - 16, i ? C_ON_ORANGE : C_TEXT, ALIGN_CENTER,
		             i ? ui->dlg.yes : T("Cancel", "Cancelar"));
		if (ht == 2) {
			if (i) a = dialog_yes(ui);
			else ui->dlg.open = false;
		}
	}
	float fx = dx + 22 + ui->dlg.focus * (bw + 12);
	gfx_stroke(g, fx - 3, by - 3, bw + 6, 44, 15, 2.5f, ui->dlg.focus ? C_ORANGE_LIGHT : C_ORANGE);
	return a;
}

static void draw_toast(Ui* ui, float y) {
	uint32_t age = SDL_GetTicks() - ui->toastAt;
	if (!ui->toast[0] || age > 2200) return;
	Gfx* g = ui->g;
	float alpha = age < 150 ? age / 150.0f : age > 1900 ? (2200 - age) / 300.0f : 1;
	float tw = gfx_text_w(g, FONT_BOLD, 13, ui->toast) + 52, w = gfx_w(g);
	float x = (w - tw) / 2;
	gfx_shadow(g, x, y + 4, tw, 34, 17, 12, with_alpha(rgb(0x000000), 0.5f * alpha));
	gfx_fill(g, x, y, tw, 34, 17, with_alpha(rgb(0x1E1814), 0.95f * alpha));
	gfx_circle(g, x + 20, y + 17, 9, with_alpha(C_ORANGE, alpha));
	gfx_icon(g, ICON_CHECK, x + 20, y + 17, 11, with_alpha(C_ON_ORANGE, alpha));
	gfx_text(g, FONT_BOLD, 13, x + 36, y + 9, with_alpha(C_TEXT, alpha), ALIGN_LEFT, ui->toast);
}

void ui_draw_overlay(Ui* ui, SDL_Rect r, double fps) {
	gfx_area(ui->g, r);
	if (ui->set->showFps && fps > 0) {
		char t[16];
		snprintf(t, sizeof(t), "%.0f FPS", fps);
		float tw = gfx_text_w(ui->g, FONT_BOLD, 12, t) + 16;
		gfx_fill(ui->g, 8, 8, tw, 22, 11, with_alpha(rgb(0x000000), 0.6f));
		gfx_text(ui->g, FONT_BOLD, 12, 16, 12.5f, fps >= 59 ? C_TEXT : C_ORANGE_LIGHT, ALIGN_LEFT, t);
	}
	draw_toast(ui, gfx_h(ui->g) - 50);
	gfx_done(ui->g);
}

/* --- a frame ------------------------------------------------------------- */

UiAction ui_frame(Ui* ui, uint32_t nav, const UiTouch* touch, SDL_Rect top, SDL_Rect bottom) {
	ui->frames++;
	UiAction a = act(UIA_NONE, 0);
	if (nav & NAV_QUIT) return act(UIA_QUIT, 0);

	/* the touch, in the bottom screen's units */
	gfx_area(ui->g, bottom);
	ui->t = *touch;
	gfx_from_px(ui->g, touch->x, touch->y, &ui->tx, &ui->ty);
	if (touch->pressed) {
		ui->startX = ui->tx;
		ui->startY = ui->ty;
		ui->lastY = ui->ty;
		ui->dragging = false;
		ui->pressId = -1;
		/* a touch outside the bottom screen isn't one */
		if (ui->ty < 0 || ui->ty > gfx_h(ui->g) || ui->tx < 0 || ui->tx > gfx_w(ui->g)) ui->t.pressed = false;
	}
	if (touch->down && ui->page == PAGE_LIBRARY && fabsf(ui->ty - ui->startY) > 10 && !ui->dlg.open) ui->dragging = true;

	/* buttons first */
	ui->modal = ui->dlg.open;
	if (nav) {
		if (ui->dlg.open) a = nav_dialog(ui, nav);
		else if (ui->page == PAGE_LIBRARY) a = nav_library(ui, nav);
		else if (ui->page == PAGE_PAUSE) a = nav_pause(ui, nav);
		else if (ui->page == PAGE_SLOTS) a = nav_slots(ui, nav);
		else a = nav_settings(ui, nav);
	}
	ui->modal = ui->dlg.open;

	/* then the screens (and the taps on them) */
	gfx_area(ui->g, top);
	draw_top(ui);
	gfx_area(ui->g, bottom);
	UiAction t = act(UIA_NONE, 0);
	Page before = ui->page;
	switch (ui->page) {
	case PAGE_LIBRARY: t = draw_library(ui); break;
	case PAGE_PAUSE: t = draw_pause(ui); break;
	case PAGE_SLOTS: t = draw_slots(ui); break;
	case PAGE_SETTINGS: t = draw_settings(ui); break;
	}
	if (ui->dlg.open) {
		UiAction d = draw_dialog(ui);
		if (d.kind != UIA_NONE) t = d;
	}
	(void)before;
	if (a.kind == UIA_NONE) a = t;
	draw_toast(ui, gfx_h(ui->g) - FOOTER_H - 46);
	gfx_done(ui->g);

	ui->lastY = ui->ty;
	if (touch->released) {
		ui->pressId = -1;
		ui->dragging = false;
	}
	return a;
}
