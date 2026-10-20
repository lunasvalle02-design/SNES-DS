/* Drawing for the app's own screens (the game list, the menu): rounded
 * boxes with soft edges and shadows, gradients, pictures, text and a few
 * icons, all with SDL_Renderer. Everything is in design units: a screen is
 * 360 units high (and as wide as its aspect makes it), whatever its pixels,
 * so the same layout fits 1024x768 panels and a small PC window. */
#ifndef SNESDS_GFX_H
#define SNESDS_GFX_H

#include <SDL.h>
#include <stdbool.h>
#include <stdint.h>

#define GFX_DESIGN_H 360.0f

typedef struct {
	uint8_t r, g, b, a;
} Color;

static inline Color rgb(uint32_t hex) { return (Color) { hex >> 16 & 255, hex >> 8 & 255, hex & 255, 255 }; }
static inline Color with_alpha(Color c, float a) {
	c.a = (uint8_t)(c.a * (a < 0 ? 0 : a > 1 ? 1 : a));
	return c;
}
Color color_mix(Color a, Color b, float t);

enum { FONT_TEXT, FONT_BOLD, FONTS };
enum { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT };

typedef enum {
	ICON_PLAY, ICON_SAVE, ICON_LOAD, ICON_GEAR, ICON_HOME, ICON_POWER, ICON_BACK, ICON_LEFT, ICON_RIGHT,
	ICON_CHECK, ICON_PAUSE, ICON_GRID,
} Icon;

typedef struct {
	SDL_Texture* tex;
	int w, h;
} Image;

typedef struct Gfx Gfx;

/* fonts from assetsDir/fonts; false if they're missing */
Gfx* gfx_new(SDL_Renderer* ren, const char* assetsDir);
void gfx_free(Gfx* g);
SDL_Renderer* gfx_renderer(Gfx* g);

/* draw into this part of the window (pixels) from now on, and only there */
void gfx_area(Gfx* g, SDL_Rect area);
/* done drawing: the whole window again */
void gfx_done(Gfx* g);
float gfx_w(Gfx* g); /* the area's design size */
float gfx_h(Gfx* g);
float gfx_scale(Gfx* g); /* pixels per unit */
/* design units -> window pixels and back */
void gfx_to_px(Gfx* g, float x, float y, float* px, float* py);
void gfx_from_px(Gfx* g, float px, float py, float* x, float* y);
void gfx_clip(Gfx* g, float x, float y, float w, float h);
void gfx_unclip(Gfx* g);

void gfx_fill(Gfx* g, float x, float y, float w, float h, float r, Color c);
void gfx_fill_grad(Gfx* g, float x, float y, float w, float h, float r, Color top, Color bottom);
void gfx_stroke(Gfx* g, float x, float y, float w, float h, float r, float t, Color c);
/* a soft shadow around the box, fading out over blur */
void gfx_shadow(Gfx* g, float x, float y, float w, float h, float r, float blur, Color c);
/* the same, only outside the box (the box itself is left alone) */
void gfx_glow(Gfx* g, float x, float y, float w, float h, float r, float blur, Color c);
void gfx_circle(Gfx* g, float cx, float cy, float rad, Color c);
void gfx_line(Gfx* g, float x0, float y0, float x1, float y1, float t, Color c);
void gfx_tri(Gfx* g, float x0, float y0, float x1, float y1, float x2, float y2, Color c);
void gfx_icon(Gfx* g, Icon icon, float cx, float cy, float size, Color c);

/* text: y is the top of the line, which is `size` high */
float gfx_text(Gfx* g, int font, float size, float x, float y, Color c, int align, const char* s);
float gfx_text_w(Gfx* g, int font, float size, const char* s);
/* one line cut to maxW with an ellipsis */
void gfx_text_fit(Gfx* g, int font, float size, float x, float y, float maxW, Color c, int align, const char* s);
/* wrapped to maxW, at most maxLines (the last one ellipsised); the lines used */
int gfx_text_wrap(Gfx* g, int font, float size, float x, float y, float maxW, int maxLines, float lineH, Color c,
                  int align, const char* s);

/* pictures: a file (png/jpg), or 0x00RRGGBB pixels */
bool gfx_image_load(Gfx* g, const char* path, Image* out);
bool gfx_image_pixels(Gfx* g, const uint32_t* px, int w, int h, Image* io); /* creates or updates */
void gfx_image_free(Image* im);
/* in the box: cover = fill it (cropping), else fit inside it */
void gfx_image(Gfx* g, const Image* im, float x, float y, float w, float h, float r, bool cover, float alpha);

#endif
