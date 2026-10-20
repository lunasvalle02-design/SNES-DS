/* Input: the keyboard and a controller, to SNES buttons and the app's own
 * actions. On a handheld there's no keyboard: the actions are Select (or the
 * controller's guide/home button) held with another button:
 *
 *   Select + Start   quit            Select + X   swap the screens
 *   L + Start + Select  quit too     Select + Y   aspect: SNES / 4:3
 *   Select + R       save state      Select + A   HUD split on/off
 *   Select + L       load state      Select + Right  fast forward on/off
 *
 * Save and load happen when R or L is let go, so that L + Start + Select
 * never loads a state on its way to quitting.
 *
 * Select itself reaches the game when it's let go without any of those (or
 * after it's been held half a second), so a game never sees the Select of
 * a shortcut. */
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
};

typedef struct {
	uint16_t keys;   /* SNES buttons held on the keyboard */
	bool fastKey;    /* Tab held */
	uint32_t actions;
	/* the controller's hotkey state */
	uint16_t prev;      /* its buttons last frame, as read */
	uint16_t consumed;  /* held buttons that were part of a shortcut */
	int selectFrames;   /* frames Select has been held alone */
	int selectPulse;    /* frames left of a Select let through on release */
	uint32_t pending;   /* state actions waiting for L / R to be let go */
} Pad;

/* a keyboard event (keys, and the F-key actions) */
void pad_key(Pad* p, const SDL_KeyboardEvent* e);
/* once a frame: the buttons the game gets; p->actions holds what was asked
 * for since the last call (cleared by the caller) */
uint16_t pad_frame(Pad* p, SDL_GameController* gc);

#endif
