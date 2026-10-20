/* Putting the two pictures on the window: where each screen goes, and each
 * picture at its aspect (the SNES' own, or 4:3). */
#ifndef SNESDS_VIDEO_H
#define SNESDS_VIDEO_H

#include <SDL.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
	LAYOUT_AUTO,  /* side by side when the window is wide (sway spanning two panels), else stacked */
	LAYOUT_STACK, /* game above, second screen below */
	LAYOUT_SIDE,  /* game left, second screen right (the RG DS panels under sway) */
} Layout;

/* the two screens' places in a window w x h */
void video_layout(Layout l, int w, int h, SDL_Rect* game, SDL_Rect* second);

/* one screen's picture */
typedef struct {
	SDL_Texture* pic; /* 256x224, as the core draws it */
	SDL_Texture* up;  /* for 4:3: the picture at whole-number scales */
	int upW, upH;
} ScreenTex;

void screen_tex_init(SDL_Renderer* ren, ScreenTex* t);
void screen_tex_free(ScreenTex* t);
/* a 256x224 picture (0x00RRGGBB) inside r, at the SNES' aspect or 4:3 */
void video_present(SDL_Renderer* ren, ScreenTex* t, const uint32_t* pixels, SDL_Rect r, bool wide);

/* what's on the window into a PNG */
bool video_shot(SDL_Renderer* ren, const char* path);

#endif
