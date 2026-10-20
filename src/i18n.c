#include "i18n.h"

#include <string.h>

int i18n_lang = LANG_EN;

static const char* const codes[LANGS] = { "en", "es" };

const char* lang_code(int lang) { return lang >= 0 && lang < LANGS ? codes[lang] : codes[LANG_EN]; }

int lang_from_code(const char* code) {
	for (int i = 0; i < LANGS; i++) {
		if (!strncmp(code, codes[i], 2)) return i;
	}
	return LANG_EN;
}
