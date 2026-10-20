/* Input: the keyboard and a controller, to SNES buttons and the app's own
 * actions. On a handheld there's no keyboard: the menu opens with any
 * button the SNES doesn't use (guide/home, misc, paddles, stick clicks,
 * the controller's raw buttons its mapping leaves out, other input
 * devices' buttons, Menu/Home keys; handhelds name theirs differently;
 * --menu-button adds one; on the RG DS Plus, L3 and the R (home) button),
 * or a tap on the bottom screen (main.c), and a few shortcuts are
 * Select held with another button:
 *
 *   L3 (or guide...)   the menu         Select + X   swap the screens
 *   Select + Start     the menu too     Select + Y   aspect: SNES / 4:3
 *   L + Start + Select quit             Select + A   HUD split on/off
 *   Select + R         save state       Select + Right  fast forward on/off
 *   Select + L         load state       (slot 1)
 *
 * Save and load happen when R or L is let go, so that L + Start + Select
 * never loads a state on its way to quitting.
 *
 * Select itself reaches the game when it's let go without any of those (or
 * after it's been held half a second), so a game never sees the Select of
 * a shortcut. In the app's own screens (pad_nav) the buttons move around
 * instead. */
#ifndef SNESDS_PAD_H
#define SNESDS_PAD_H

#include <SDL.h>
#include <stdbool.h>
#include <stdint.h>

enum {
	ACT_QUIT = 1 << 0,
	ACT_SAVE_STATE = 1 << 1,
	ACT_LOAD_STATE = 1 << 2,
	ACT_SWAP = 1 << 3,
	ACT_ASPECT = 1 << 4,
	ACT_SPLIT = 1 << 5,
	ACT_FAST = 1 << 6,       /* toggle */
	ACT_FULLSCREEN = 1 << 7,
	ACT_MENU = 1 << 8,
};

/* the app's own screens: presses (the directions repeat while held) */
enum {
	NAV_UP = 1 << 0, NAV_DOWN = 1 << 1, NAV_LEFT = 1 << 2, NAV_RIGHT = 1 << 3,
	NAV_OK = 1 << 4,   /* A (the right-hand button) */
	NAV_BACK = 1 << 5, /* B (the bottom one) */
	NAV_X = 1 << 6, NAV_Y = 1 << 7,
	NAV_L = 1 << 8, NAV_R = 1 << 9,
	NAV_START = 1 << 10, NAV_SELECT = 1 << 11,
	NAV_MENU = 1 << 12,
	NAV_QUIT = 1 << 13, /* L + Start + Select, or the window closed */
};

typedef struct {
	uint16_t keys;   /* SNES buttons held on the keyboard */
	uint16_t keysSwallowed; /* of those, the ones held since pad_swallow */
	bool fastKey;    /* Tab held */
	uint32_t actions;
	/* the controller's hotkey state */
	uint16_t prev;      /* its buttons last frame, as read */
	uint16_t consumed;  /* held buttons that were part of a shortcut */
	int selectFrames;   /* frames Select has been held alone */
	int selectPulse;    /* frames left of a Select let through on release */
	uint32_t pending;   /* state actions waiting for L / R to be let go */
	bool guidePrev, guideUsed; /* the menu button: a shortcut while held, else the menu */
	int menuButton;     /* another SDL_GameControllerButton that opens the menu */
	int menuJoyButton;  /* or a raw joystick button (-1: none) */
	int extraButtons;   /* buttons held on other input devices (pad_extra_button) */
	bool menuTapped;    /* a menu button went down since the last frame */
	/* the app's screens */
	uint32_t navKeys;   /* keyboard presses since the last pad_nav */
	uint16_t navPrev;
	int repeatFrames;
} Pad;

void pad_init(Pad* p);
/* "guide", "back", "misc1"... (SDL's names) or "joy:N": one more button
 * for the menu, on top of the ones it already takes */
bool pad_set_menu_button(Pad* p, const char* name);
/* a button of another input device (not the controller) went down / up:
 * it counts as the menu button (handhelds may put theirs on a device of
 * its own) */
void pad_extra_button(Pad* p, bool down);
/* the controller's button events: a menu button that goes down and up
 * between two frames still opens the menu */
void pad_button_event(Pad* p, SDL_GameController* gc, const SDL_Event* e);

/* a keyboard event (keys, and the F-key actions) */
void pad_key(Pad* p, const SDL_KeyboardEvent* e);
/* once a frame: the buttons the game gets; p->actions holds what was asked
 * for since the last call (cleared by the caller) */
uint16_t pad_frame(Pad* p, SDL_GameController* gc);
/* once a frame in the app's screens: NAV_* presses */
uint32_t pad_nav(Pad* p, SDL_GameController* gc);
/* what's held now stays away from the game until it's let go (after the
 * menu, so its A press doesn't reach the game) */
void pad_swallow(Pad* p, SDL_GameController* gc);

#endif
