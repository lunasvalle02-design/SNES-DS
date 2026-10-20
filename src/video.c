#include "video.h"

#include <stdio.h>
#include <stdlib.h>

#include "core.h"
#include "stb_image_write.h"

void video_layout(Layout l, int w, int h, SDL_Rect* game, SDL_Rect* second) {
	if (l == LAYOUT_AUTO) l = w * 10 >= h * 18 ? LAYOUT_SIDE : LAYOUT_STACK;
	if (l == LAYOUT_SIDE) {
		*game = (SDL_Rect) { 0, 0, w / 2, h };
		*second = (SDL_Rect) { w / 2, 0, w - w / 2, h };
	} else {
		*game = (SDL_Rect) { 0, 0, w, h / 2 };
		*second = (SDL_Rect) { 0, h / 2, w, h - h / 2 };
	}
}

/* the largest whole-number scale of the picture inside r, centred */
static SDL_Rect fit(SDL_Rect r) {
	int k = r.w / CORE_W;
	if (r.h / CORE_H < k) k = r.h / CORE_H;
	if (k < 1) k = 1;
	SDL_Rect o = { r.x + (r.w - CORE_W * k) / 2, r.y + (r.h - CORE_H * k) / 2, CORE_W * k, CORE_H * k };
	return o;
}

void screen_tex_init(SDL_Renderer* ren, ScreenTex* t) {
	t->pic = SDL_CreateTexture(ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, CORE_W, CORE_H);
	SDL_SetTextureScaleMode(t->pic, SDL_ScaleModeNearest);
	t->up = NULL;
	t->upW = t->upH = 0;
}

void screen_tex_free(ScreenTex* t) {
	SDL_DestroyTexture(t->pic);
	if (t->up) SDL_DestroyTexture(t->up);
}

/* 4:3 doesn't divide into whole pixels (1024x768 is 4x across but 3.43x
 * down), so the picture first goes up to whole-number scales at or above
 * that size, sharp, and only that is smoothed down to it: every pixel stays
 * the same size, with soft edges no wider than one line */
void video_present(SDL_Renderer* ren, ScreenTex* t, const uint32_t* pixels, SDL_Rect r, bool wide) {
	SDL_UpdateTexture(t->pic, NULL, pixels, CORE_W * 4);
	if (!wide) {
		SDL_Rect d = fit(r);
		SDL_RenderCopy(ren, t->pic, NULL, &d);
		return;
	}
	int w = r.w, h = w * 3 / 4;
	if (h > r.h) {
		h = r.h;
		w = h * 4 / 3;
	}
	SDL_Rect d = { r.x + (r.w - w) / 2, r.y + (r.h - h) / 2, w, h };
	int kx = (w + CORE_W - 1) / CORE_W, ky = (h + CORE_H - 1) / CORE_H;
	if (!t->up || t->upW != CORE_W * kx || t->upH != CORE_H * ky) {
		if (t->up) SDL_DestroyTexture(t->up);
		t->upW = CORE_W * kx;
		t->upH = CORE_H * ky;
		t->up = SDL_CreateTexture(ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_TARGET, t->upW, t->upH);
		if (t->up) SDL_SetTextureScaleMode(t->up, SDL_ScaleModeLinear);
	}
	if (!t->up) {
		SDL_RenderCopy(ren, t->pic, NULL, &d);
		return;
	}
	SDL_Texture* prev = SDL_GetRenderTarget(ren);
	SDL_SetRenderTarget(ren, t->up);
	SDL_RenderCopy(ren, t->pic, NULL, NULL);
	SDL_SetRenderTarget(ren, prev);
	SDL_RenderCopy(ren, t->up, NULL, &d);
}

bool video_shot(SDL_Renderer* ren, const char* path, const SDL_Rect* part) {
	int w, h;
	if (SDL_GetRendererOutputSize(ren, &w, &h) != 0) return false;
	if (part) {
		w = part->w;
		h = part->h;
	}
	uint32_t* px = malloc((size_t)w * h * 4);
	uint8_t* rgb = malloc((size_t)w * h * 3);
	bool ok = px && rgb && SDL_RenderReadPixels(ren, part, SDL_PIXELFORMAT_XRGB8888, px, w * 4) == 0;
	if (ok) {
		for (int i = 0; i < w * h; i++) {
			rgb[i * 3] = px[i] >> 16;
			rgb[i * 3 + 1] = px[i] >> 8;
			rgb[i * 3 + 2] = px[i];
		}
		ok = stbi_write_png(path, w, h, 3, rgb, w * 3) != 0;
	}
	free(px);
	free(rgb);
	return ok;
}
