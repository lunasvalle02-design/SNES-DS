#include "pad.h"

#include "core.h"
#include "i18n.h"

#define SELECT_HOLD 30 /* frames held alone before Select reaches the game */
#define SELECT_PULSE 6 /* frames of Select given to the game on a short press */
#define REPEAT_DELAY 18 /* the app's screens: frames before a held direction repeats */
#define REPEAT_EVERY 5

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t unmapped_buttons(SDL_GameController* gc);

/* --- bindings ---------------------------------------------------------------- */

/* the SNES buttons BIND_B... stand for */
static const uint16_t bind_bits[BIND_MENU] = { PAD_B, PAD_A, PAD_Y, PAD_X, PAD_L, PAD_R, PAD_START, PAD_SELECT };

/* by position, as SDL names them: its "a" is the bottom face button (the
 * SNES' B), "b" the right one (A), "x" the left one (Y), "y" the top one (X) */
static const char* const bind_defaults[BINDS] = {
	"button:a", "button:b", "button:x", "button:y", "button:leftshoulder", "button:rightshoulder",
	"button:start", "button:back", "none",
};

const char* pad_bind_default(int i) { return i >= 0 && i < BINDS ? bind_defaults[i] : "none"; }

static bool parse_bind(const char* s, Bind* b) {
	*b = (Bind) { BIND_NONE, 0, 0 };
	if (!strcmp(s, "none")) return true;
	if (!strncmp(s, "button:", 7)) {
		SDL_GameControllerButton c = SDL_GameControllerGetButtonFromString(s + 7);
		if (c == SDL_CONTROLLER_BUTTON_INVALID) return false;
		*b = (Bind) { BIND_BUTTON, c, 0 };
		return true;
	}
	if (!strncmp(s, "axis:", 5)) {
		char name[32];
		size_t n = strlen(s + 5);
		if (n < 2 || n >= sizeof(name)) return false;
		char sign = s[5 + n - 1];
		if (sign != '+' && sign != '-') return false;
		memcpy(name, s + 5, n - 1);
		name[n - 1] = 0;
		SDL_GameControllerAxis a = SDL_GameControllerGetAxisFromString(name);
		if (a == SDL_CONTROLLER_AXIS_INVALID) return false;
		*b = (Bind) { BIND_AXIS, a, sign == '+' ? 1 : -1 };
		return true;
	}
	if (!strncmp(s, "joy:", 4) && s[4] >= '0' && s[4] <= '9') {
		*b = (Bind) { BIND_JOY, atoi(s + 4), 0 };
		return b->code < 64;
	}
	if (!strncmp(s, "key:", 4) && s[4] >= '0' && s[4] <= '9') {
		*b = (Bind) { BIND_KEY, atoi(s + 4), 0 };
		return b->code > 0 && b->code < SDL_NUM_SCANCODES;
	}
	return false;
}

bool pad_set_bind(Pad* p, int i, const char* text) {
	if (i < 0 || i >= BINDS) return false;
	if (!text || !text[0]) text = bind_defaults[i];
	Bind b;
	if (!parse_bind(text, &b)) {
		parse_bind(bind_defaults[i], &p->bind[i]);
		return false;
	}
	p->bind[i] = b;
	return true;
}

static bool bind_held(const Bind* b, SDL_GameController* gc) {
	switch (b->kind) {
	case BIND_BUTTON: return gc && SDL_GameControllerGetButton(gc, (SDL_GameControllerButton)b->code);
	case BIND_AXIS: return gc && SDL_GameControllerGetAxis(gc, (SDL_GameControllerAxis)b->code) * b->dir > 16000;
	case BIND_JOY: {
		SDL_Joystick* j = gc ? SDL_GameControllerGetJoystick(gc) : NULL;
		return j && b->code < SDL_JoystickNumButtons(j) && SDL_JoystickGetButton(j, b->code);
	}
	case BIND_KEY: {
		int n;
		const Uint8* k = SDL_GetKeyboardState(&n);
		return b->code < n && k[b->code];
	}
	case BIND_NONE: break;
	}
	return false;
}

/* a controller button or raw button one of the SNES buttons has: it's not
 * a menu button then */
static bool bound(const Pad* p, BindKind kind, int code) {
	for (int i = 0; i < BIND_MENU; i++) {
		if (p->bind[i].kind == kind && p->bind[i].code == code) return true;
	}
	return false;
}

bool pad_capture(SDL_GameController* gc, const SDL_Event* e, char* out, size_t n) {
	switch (e->type) {
	case SDL_CONTROLLERBUTTONDOWN: {
		const char* name = SDL_GameControllerGetStringForButton((SDL_GameControllerButton)e->cbutton.button);
		if (!name) return false;
		snprintf(out, n, "button:%s", name);
		return true;
	}
	case SDL_CONTROLLERAXISMOTION: {
		/* the left stick is the d-pad */
		if (e->caxis.axis == SDL_CONTROLLER_AXIS_LEFTX || e->caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) return false;
		if (abs(e->caxis.value) < 24000) return false;
		const char* name = SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)e->caxis.axis);
		if (!name) return false;
		snprintf(out, n, "axis:%s%c", name, e->caxis.value > 0 ? '+' : '-');
		return true;
	}
	case SDL_JOYBUTTONDOWN: {
		/* the mapped ones come as controller buttons too */
		if (!gc || e->jbutton.which != SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gc))) return false;
		if (e->jbutton.button >= 64 || !(unmapped_buttons(gc) >> e->jbutton.button & 1)) return false;
		snprintf(out, n, "joy:%d", e->jbutton.button);
		return true;
	}
	case SDL_KEYDOWN:
		if (e->key.repeat || e->key.keysym.scancode == SDL_SCANCODE_UNKNOWN) return false;
		snprintf(out, n, "key:%d", e->key.keysym.scancode);
		return true;
	default: return false;
	}
}

void pad_bind_label(const char* text, char* out, size_t n) {
	static const struct { const char* name; const char* en; const char* es; } names[] = {
		{ "a", "Bottom face button", "Botón de abajo" }, { "b", "Right face button", "Botón de la derecha" },
		{ "x", "Left face button", "Botón de la izquierda" }, { "y", "Top face button", "Botón de arriba" },
		{ "back", "Select", "Select" }, { "start", "Start", "Start" }, { "guide", "Home", "Inicio" },
		{ "leftshoulder", "L1", "L1" }, { "rightshoulder", "R1", "R1" }, { "leftstick", "L3", "L3" },
		{ "rightstick", "R3", "R3" }, { "dpup", "D-pad up", "Cruceta arriba" },
		{ "dpdown", "D-pad down", "Cruceta abajo" }, { "dpleft", "D-pad left", "Cruceta izquierda" },
		{ "dpright", "D-pad right", "Cruceta derecha" }, { "lefttrigger+", "L2", "L2" },
		{ "righttrigger+", "R2", "R2" }, { "rightx+", "Right stick right", "Stick derecho a la derecha" },
		{ "rightx-", "Right stick left", "Stick derecho a la izquierda" },
		{ "righty+", "Right stick down", "Stick derecho abajo" }, { "righty-", "Right stick up", "Stick derecho arriba" },
	};
	if (!text || !text[0]) text = "";
	Bind b;
	if (!text[0] || !parse_bind(text, &b)) {
		snprintf(out, n, "%s", T("Default", "Por defecto"));
		return;
	}
	const char* name = strchr(text, ':') ? strchr(text, ':') + 1 : text;
	switch (b.kind) {
	case BIND_NONE: snprintf(out, n, "%s", T("None", "Ninguno")); return;
	case BIND_BUTTON:
	case BIND_AXIS:
		for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
			if (!strcmp(name, names[i].name)) {
				snprintf(out, n, "%s", T(names[i].en, names[i].es));
				return;
			}
		}
		snprintf(out, n, "%s", name);
		return;
	case BIND_JOY: snprintf(out, n, T("Button %d", "Botón %d"), b.code); return;
	case BIND_KEY: {
		const char* k = SDL_GetScancodeName((SDL_Scancode)b.code);
		snprintf(out, n, T("Key %s", "Tecla %s"), k && k[0] ? k : "?");
		return;
	}
	}
}

void pad_init(Pad* p) {
	memset(p, 0, sizeof(*p));
	p->menuButton = SDL_CONTROLLER_BUTTON_INVALID;
	p->menuJoyButton = -1;
	for (int i = 0; i < BINDS; i++) pad_set_bind(p, i, NULL);
}

/* the controller's buttons the SNES has no use for: any of them opens the
 * menu (handhelds name their menu key differently: guide, misc1, a stick
 * click...) */
static const SDL_GameControllerButton spare[] = {
	SDL_CONTROLLER_BUTTON_GUIDE, SDL_CONTROLLER_BUTTON_MISC1, SDL_CONTROLLER_BUTTON_PADDLE1,
	SDL_CONTROLLER_BUTTON_PADDLE2, SDL_CONTROLLER_BUTTON_PADDLE3, SDL_CONTROLLER_BUTTON_PADDLE4,
	SDL_CONTROLLER_BUTTON_TOUCHPAD, SDL_CONTROLLER_BUTTON_LEFTSTICK, SDL_CONTROLLER_BUTTON_RIGHTSTICK,
};

/* the raw buttons the controller's mapping leaves out (bit i: button i) */
static uint64_t unmapped_buttons(SDL_GameController* gc) {
	static SDL_GameController* cached;
	static uint64_t mask;
	if (gc == cached) return mask;
	cached = gc;
	SDL_Joystick* j = SDL_GameControllerGetJoystick(gc);
	int n = j ? SDL_JoystickNumButtons(j) : 0;
	if (n > 64) n = 64;
	mask = n == 64 ? ~0ull : (1ull << n) - 1;
	for (int b = 0; b < SDL_CONTROLLER_BUTTON_MAX; b++) {
		SDL_GameControllerButtonBind bind = SDL_GameControllerGetBindForButton(gc, (SDL_GameControllerButton)b);
		if (bind.bindType == SDL_CONTROLLER_BINDTYPE_BUTTON && bind.value.button < 64) mask &= ~(1ull << bind.value.button);
	}
	for (int a = 0; a < SDL_CONTROLLER_AXIS_MAX; a++) {
		SDL_GameControllerButtonBind bind = SDL_GameControllerGetBindForAxis(gc, (SDL_GameControllerAxis)a);
		if (bind.bindType == SDL_CONTROLLER_BINDTYPE_BUTTON && bind.value.button < 64) mask &= ~(1ull << bind.value.button);
	}
	if (mask) printf("input: raw buttons outside the mapping (menu): %llx\n", (unsigned long long)mask);
	return mask;
}

bool pad_set_menu_button(Pad* p, const char* name) {
	if (!strncmp(name, "joy:", 4)) {
		p->menuJoyButton = atoi(name + 4);
		return true;
	}
	SDL_GameControllerButton b = SDL_GameControllerGetButtonFromString(name);
	if (b == SDL_CONTROLLER_BUTTON_INVALID) return false;
	p->menuButton = b;
	return true;
}

static bool menu_held(Pad* p, SDL_GameController* gc) {
	if (p->extraButtons) return true;
	if (!gc) return false;
	if (bind_held(&p->bind[BIND_MENU], gc)) return true;
	if (p->menuButton != SDL_CONTROLLER_BUTTON_INVALID && SDL_GameControllerGetButton(gc, p->menuButton)) return true;
	for (size_t i = 0; i < sizeof(spare) / sizeof(spare[0]); i++) {
		if (!bound(p, BIND_BUTTON, spare[i]) && SDL_GameControllerGetButton(gc, spare[i])) return true;
	}
	SDL_Joystick* j = SDL_GameControllerGetJoystick(gc);
	if (!j) return false;
	if (p->menuJoyButton >= 0 && p->menuJoyButton < SDL_JoystickNumButtons(j) && SDL_JoystickGetButton(j, p->menuJoyButton))
		return true;
	uint64_t raw = unmapped_buttons(gc);
	for (int b = 0; raw >> b; b++) {
		if ((raw >> b & 1) && !bound(p, BIND_JOY, b) && SDL_JoystickGetButton(j, b)) return true;
	}
	return false;
}

void pad_extra_button(Pad* p, bool down) {
	if (down) {
		p->extraButtons++;
		p->menuTapped = true;
	} else if (p->extraButtons > 0) {
		p->extraButtons--;
	}
}

void pad_button_event(Pad* p, SDL_GameController* gc, const SDL_Event* e) {
	if (!gc) return;
	bool menu = false;
	const Bind* m = &p->bind[BIND_MENU];
	if (e->type == SDL_CONTROLLERBUTTONDOWN) {
		int b = e->cbutton.button;
		for (size_t i = 0; i < sizeof(spare) / sizeof(spare[0]); i++) menu |= b == (int)spare[i] && !bound(p, BIND_BUTTON, b);
		menu |= p->menuButton != SDL_CONTROLLER_BUTTON_INVALID && b == p->menuButton;
		menu |= m->kind == BIND_BUTTON && m->code == b;
	} else if (e->type == SDL_JOYBUTTONDOWN && e->jbutton.button < 64) {
		int b = e->jbutton.button;
		menu = ((unmapped_buttons(gc) >> b & 1) && !bound(p, BIND_JOY, b)) || b == p->menuJoyButton;
		menu |= m->kind == BIND_JOY && m->code == b;
	}
	if (menu) p->menuTapped = true;
}

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
	if (!down) return;
	/* keys a handheld's menu or home button may come as */
	static const SDL_Keycode menuKeys[] = { SDLK_MENU, SDLK_APPLICATION, SDLK_AC_HOME, SDLK_HOME, SDLK_AC_BACK, SDLK_MODE };
	for (size_t i = 0; i < sizeof(menuKeys) / sizeof(menuKeys[0]); i++) {
		if (k == menuKeys[i] && !e->repeat) {
			p->actions |= ACT_MENU;
			p->navKeys |= NAV_MENU;
		}
	}
	/* the app's screens (directions repeat with the keyboard's own repeat) */
	switch (k) {
	case SDLK_UP: p->navKeys |= NAV_UP; break;
	case SDLK_DOWN: p->navKeys |= NAV_DOWN; break;
	case SDLK_LEFT: p->navKeys |= NAV_LEFT; break;
	case SDLK_RIGHT: p->navKeys |= NAV_RIGHT; break;
	default: break;
	}
	if (e->repeat) return;
	switch (k) {
	case SDLK_RETURN: case SDLK_x: case SDLK_SPACE: p->navKeys |= NAV_OK; break;
	case SDLK_z: case SDLK_BACKSPACE: p->navKeys |= NAV_BACK; break;
	case SDLK_s: p->navKeys |= NAV_X; break;
	case SDLK_a: p->navKeys |= NAV_Y; break;
	case SDLK_q: p->navKeys |= NAV_L; break;
	case SDLK_w: p->navKeys |= NAV_R; break;
	case SDLK_ESCAPE: p->navKeys |= NAV_MENU; break;
	default: break;
	}
	switch (k) {
	case SDLK_ESCAPE: p->actions |= ACT_MENU; break;
	case SDLK_F2: p->actions |= ACT_SWAP; break;
	case SDLK_F3: p->actions |= ACT_SPLIT; break;
	case SDLK_F4: p->actions |= ACT_ASPECT; break;
	case SDLK_F5: p->actions |= ACT_SAVE_STATE; break;
	case SDLK_F7: p->actions |= ACT_LOAD_STATE; break;
	case SDLK_F11: p->actions |= ACT_FULLSCREEN; break;
	default: break;
	}
}

/* the controller as SNES buttons: by the bindings (by default by position:
 * bottom B, right A, top X, left Y), the d-pad and the left stick the
 * directions */
static uint16_t read_controller(Pad* p, SDL_GameController* gc) {
	static const struct { SDL_GameControllerButton b; uint16_t bit; } dirs[] = {
		{ SDL_CONTROLLER_BUTTON_DPAD_UP, PAD_UP }, { SDL_CONTROLLER_BUTTON_DPAD_DOWN, PAD_DOWN },
		{ SDL_CONTROLLER_BUTTON_DPAD_LEFT, PAD_LEFT }, { SDL_CONTROLLER_BUTTON_DPAD_RIGHT, PAD_RIGHT },
	};
	uint16_t pad = 0;
	for (int i = 0; i < BIND_MENU; i++) {
		if (bind_held(&p->bind[i], gc)) pad |= bind_bits[i];
	}
	if (!gc) return pad;
	for (size_t i = 0; i < sizeof(dirs) / sizeof(dirs[0]); i++) {
		if (SDL_GameControllerGetButton(gc, dirs[i].b)) pad |= dirs[i].bit;
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
		{ PAD_START, ACT_MENU }, { PAD_R, ACT_SAVE_STATE }, { PAD_L, ACT_LOAD_STATE }, { PAD_X, ACT_SWAP },
		{ PAD_Y, ACT_ASPECT }, { PAD_A, ACT_SPLIT }, { PAD_RIGHT, ACT_FAST },
	};
	uint16_t raw = read_controller(p, gc);
	bool guide = menu_held(p, gc);
	/* the menu button: let go without being part of a shortcut, the menu */
	if (guide && !p->guidePrev) p->guideUsed = false;
	if (!guide && p->guidePrev && !p->guideUsed) p->actions |= ACT_MENU;
	/* pressed and let go between two frames: never seen held */
	if (!guide && !p->guidePrev && p->menuTapped) p->actions |= ACT_MENU;
	p->menuTapped = false;
	p->guidePrev = guide;
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
			if (guide) p->guideUsed = true;
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
		if (!(p->consumed & PAD_SELECT) && p->selectFrames <= SELECT_HOLD) {
			p->selectPulse = SELECT_PULSE;
			printf("input: Select to the game (%d frames)\n", SELECT_PULSE);
		} else {
			printf("input: Select kept from the game (%s)\n", p->consumed & PAD_SELECT ? "a shortcut" : "held through");
		}
		p->selectFrames = 0;
	}
	bool selectThrough = select && !(p->consumed & PAD_SELECT) && p->selectFrames > SELECT_HOLD;
	if (selectThrough && p->selectFrames == SELECT_HOLD + 1) printf("input: Select held: to the game\n");
	if (!select) p->selectFrames = 0;

	p->consumed &= raw; /* let go: no longer a shortcut */
	p->prev = raw;
	uint16_t out = raw & ~p->consumed & ~PAD_SELECT;
	if (selectThrough) out |= PAD_SELECT;
	if (p->selectPulse > 0) {
		out |= PAD_SELECT;
		p->selectPulse--;
	}
	p->keysSwallowed &= p->keys;
	return out | (p->keys & ~p->keysSwallowed);
}

uint32_t pad_nav(Pad* p, SDL_GameController* gc) {
	static const struct { uint16_t bit; uint32_t nav; } map[] = {
		{ PAD_UP, NAV_UP }, { PAD_DOWN, NAV_DOWN }, { PAD_LEFT, NAV_LEFT }, { PAD_RIGHT, NAV_RIGHT },
		{ PAD_A, NAV_OK }, { PAD_B, NAV_BACK }, { PAD_X, NAV_X }, { PAD_Y, NAV_Y }, { PAD_L, NAV_L },
		{ PAD_R, NAV_R }, { PAD_START, NAV_START }, { PAD_SELECT, NAV_SELECT },
	};
	const uint16_t dirs = PAD_UP | PAD_DOWN | PAD_LEFT | PAD_RIGHT;
	uint16_t raw = read_controller(p, gc);
	bool guide = menu_held(p, gc);
	uint16_t pressed = raw & ~p->navPrev;
	/* held directions repeat */
	if (!(raw & dirs)) p->repeatFrames = 0;
	else if (pressed & dirs) p->repeatFrames = 1;
	else if (++p->repeatFrames > REPEAT_DELAY && (p->repeatFrames - REPEAT_DELAY) % REPEAT_EVERY == 0)
		pressed |= raw & dirs;
	uint32_t out = p->navKeys;
	p->navKeys = 0;
	for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
		if (pressed & map[i].bit) out |= map[i].nav;
	}
	if ((guide && !p->guidePrev) || (!guide && !p->guidePrev && p->menuTapped)) out |= NAV_MENU;
	p->menuTapped = false;
	const uint16_t quit3 = PAD_L | PAD_START | PAD_SELECT;
	if ((raw & quit3) == quit3 && (pressed & quit3)) out |= NAV_QUIT;
	p->guidePrev = guide;
	p->navPrev = raw;
	return out;
}

void pad_swallow(Pad* p, SDL_GameController* gc) {
	uint16_t raw = read_controller(p, gc);
	p->consumed = raw;
	p->keysSwallowed = p->keys;
	p->prev = raw;
	p->navPrev = raw;
	p->guidePrev = menu_held(p, gc);
	p->guideUsed = true; /* its release is no menu */
	p->menuTapped = false;
	p->pending = 0;
	p->actions = 0;
	p->navKeys = 0;
	p->selectFrames = 0;
	p->selectPulse = 0;
	p->repeatFrames = 0;
}
