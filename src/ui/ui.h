/* The app's own screens: the game list it starts on, and the menu over a
 * paused game (states, settings, leaving). The bottom screen is the touch
 * one: the list and the menu are there, and the top screen goes along
 * (the chosen game, the paused picture, what a setting does). Everything
 * also works with the buttons. */
#ifndef SNESDS_UI_H
#define SNESDS_UI_H

#include <SDL.h>
#include <stdbool.h>

#include "game.h"
#include "gfx.h"
#include "library.h"
#include "settings.h"

/* a finger (or the mouse) this frame, in window pixels */
typedef struct {
	bool down;     /* touching now */
	bool pressed;  /* went down this frame */
	bool released; /* went up this frame */
	float x, y;
} UiTouch;

typedef enum {
	UIA_NONE,
	UIA_PLAY,       /* arg: the game's index in the library */
	UIA_RESUME,
	UIA_SAVE,       /* arg: slot */
	UIA_LOAD,       /* arg: slot */
	UIA_CLOSE_GAME, /* back to the game list */
	UIA_QUIT,
	UIA_SETTINGS,   /* the settings changed (already in the Settings) */
} UiActionKind;

typedef struct {
	UiActionKind kind;
	int arg;
} UiAction;

typedef struct Ui Ui;

Ui* ui_new(Gfx* g, Settings* set, Library* lib, const char* romDir, const char* version);
void ui_free(Ui* ui);
/* the library was scanned again */
void ui_library_changed(Ui* ui);

void ui_show_library(Ui* ui);
/* the menu over this game (its last picture is shown paused) */
void ui_show_pause(Ui* ui, Game* game);

/* one frame of the app's screens: input, then both screens drawn */
UiAction ui_frame(Ui* ui, uint32_t nav, const UiTouch* touch, SDL_Rect top, SDL_Rect bottom);

/* a short message, shown for a couple of seconds */
void ui_toast(Ui* ui, const char* msg);
/* over the game while it runs: the toast, and the speed when asked for */
void ui_draw_overlay(Ui* ui, SDL_Rect gameRect, double fps);

#endif
