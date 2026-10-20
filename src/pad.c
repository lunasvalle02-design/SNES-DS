#include "pad.h"

#include "core.h"

#define SELECT_HOLD 30 /* frames held alone before Select reaches the game */
#define SELECT_PULSE 6 /* frames of Select given to the game on a short press */

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

void pad_key(Pad* p, const SDL_KeyboardEvent* e) {
	bool down = e->type == SDL_KEYDOWN;
	SDL_Keycode k = e->keysym.sym;
	uint16_t b = key_button(k);
	p->keys = down ? p->keys | b : p->keys & ~b;
	if (k == SDLK_TAB) p->fastKey = down;
	if (!down || e->repeat) return;
	switch (k) {
	case SDLK_ESCAPE: p->actions |= ACT_QUIT; break;
	case SDLK_F2: p->actions |= ACT_SWAP; break;
	case SDLK_F3: p->actions |= ACT_SPLIT; break;
	case SDLK_F4: p->actions |= ACT_ASPECT; break;
	case SDLK_F5: p->actions |= ACT_SAVE_STATE; break;
	case SDLK_F7: p->actions |= ACT_LOAD_STATE; break;
	case SDLK_F11: p->actions |= ACT_FULLSCREEN; break;
	default: break;
	}
}

/* the controller as SNES buttons, by position: bottom B, right A, top X,
 * left Y (SDL's A/B/X/Y are positions too) */
static uint16_t read_controller(SDL_GameController* gc) {
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

uint16_t pad_frame(Pad* p, SDL_GameController* gc) {
	static const struct { uint16_t bit; uint32_t act; } shortcuts[] = {
		{ PAD_START, ACT_QUIT }, { PAD_R, ACT_SAVE_STATE }, { PAD_L, ACT_LOAD_STATE }, { PAD_X, ACT_SWAP },
		{ PAD_Y, ACT_ASPECT }, { PAD_A, ACT_SPLIT }, { PAD_RIGHT, ACT_FAST },
	};
	uint16_t raw = gc ? read_controller(gc) : 0;
	bool guide = gc && SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_GUIDE);
	uint16_t pressed = raw & ~p->prev;
	bool select = raw & PAD_SELECT;

	/* L + Start + Select, in any order: quit (and no state saved or loaded
	 * on the way, though Select + L is part of it) */
	const uint16_t quit3 = PAD_L | PAD_START | PAD_SELECT;
	if ((raw & quit3) == quit3 && (pressed & quit3)) {
		p->actions |= ACT_QUIT;
		p->consumed |= quit3;
		p->pending = 0;
	}

	if (guide || select) {
		for (size_t i = 0; i < sizeof(shortcuts) / sizeof(shortcuts[0]); i++) {
			if (!(pressed & shortcuts[i].bit)) continue;
			/* states go when L or R is let go, so L + Start + Select can
			 * still turn out to be a quit */
			if (shortcuts[i].act & (ACT_SAVE_STATE | ACT_LOAD_STATE)) p->pending |= shortcuts[i].act;
			else p->actions |= shortcuts[i].act;
			p->consumed |= shortcuts[i].bit | (select ? PAD_SELECT : 0);
		}
	}
	if ((p->pending & ACT_LOAD_STATE) && !(raw & PAD_L)) {
		p->actions |= ACT_LOAD_STATE;
		p->pending &= ~ACT_LOAD_STATE;
	}
	if ((p->pending & ACT_SAVE_STATE) && !(raw & PAD_R)) {
		p->actions |= ACT_SAVE_STATE;
		p->pending &= ~ACT_SAVE_STATE;
	}

	/* Select: held back until it's clearly not a shortcut */
	if (select && !(p->consumed & PAD_SELECT)) {
		p->selectFrames++;
	} else if (!select && (p->prev & PAD_SELECT)) {
		if (!(p->consumed & PAD_SELECT) && p->selectFrames <= SELECT_HOLD) p->selectPulse = SELECT_PULSE;
		p->selectFrames = 0;
	}
	bool selectThrough = select && !(p->consumed & PAD_SELECT) && p->selectFrames > SELECT_HOLD;
	if (!select) p->selectFrames = 0;

	p->consumed &= raw; /* let go: no longer a shortcut */
	p->prev = raw;
	uint16_t out = raw & ~p->consumed & ~PAD_SELECT;
	if (selectThrough) out |= PAD_SELECT;
	if (p->selectPulse > 0) {
		out |= PAD_SELECT;
		p->selectPulse--;
	}
	return out | p->keys;
}
