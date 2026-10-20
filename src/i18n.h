/* The interface's language. Each text is written where it's used, with its
 * translation beside it: T("Settings", "Ajustes"). English is the default. */
#ifndef SNESDS_I18N_H
#define SNESDS_I18N_H

typedef enum { LANG_EN, LANG_ES, LANGS } Lang;

extern int i18n_lang;

#define T(en, es) (i18n_lang == LANG_ES ? (es) : (en))

/* "en", "es" */
const char* lang_code(int lang);
/* LANG_EN when unknown */
int lang_from_code(const char* code);

#endif
