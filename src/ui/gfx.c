#include "gfx.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stb_image.h"
#include "stb_truetype.h"

#define PI 3.14159265f

/* the characters the fonts are rasterised with: ASCII, Latin-1 (Spanish
 * and the rest of western Europe) and some punctuation */
static const struct {
	int first, n;
} ranges[] = { { 32, 95 }, { 160, 96 }, { 0x2013, 2 }, { 0x2018, 6 }, { 0x2022, 1 }, { 0x2026, 1 } };
#define RANGES (int)(sizeof(ranges) / sizeof(ranges[0]))
#define NCHARS (95 + 96 + 2 + 6 + 1 + 1)
#define FACES 24

typedef struct {
	int font, px;
	SDL_Texture* tex;
	int tw, th;
	float ascent, scale;
	stbtt_packedchar pc[NCHARS];
} Face;

struct Gfx {
	SDL_Renderer* ren;
	float s, ox, oy, w, h; /* the area: pixels per unit, origin, design size */
	SDL_Rect area;
	unsigned char* fontData[FONTS];
	stbtt_fontinfo font[FONTS];
	Face faces[FACES];
	int nfaces, nextFace;
};

Color color_mix(Color a, Color b, float t) {
	if (t < 0) t = 0;
	if (t > 1) t = 1;
	return (Color) { (uint8_t)(a.r + (b.r - a.r) * t), (uint8_t)(a.g + (b.g - a.g) * t),
		             (uint8_t)(a.b + (b.b - a.b) * t), (uint8_t)(a.a + (b.a - a.a) * t) };
}

static unsigned char* read_all(const char* path) {
	FILE* f = fopen(path, "rb");
	if (!f) return NULL;
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	fseek(f, 0, SEEK_SET);
	unsigned char* d = n > 0 ? malloc(n) : NULL;
	if (d && fread(d, 1, n, f) != (size_t)n) {
		free(d);
		d = NULL;
	}
	fclose(f);
	return d;
}

Gfx* gfx_new(SDL_Renderer* ren, const char* assetsDir) {
	static const char* const files[FONTS] = { "Nunito-600.ttf", "Nunito-800.ttf" };
	Gfx* g = calloc(1, sizeof(Gfx));
	g->ren = ren;
	for (int i = 0; i < FONTS; i++) {
		char p[1200];
		snprintf(p, sizeof(p), "%s/fonts/%s", assetsDir, files[i]);
		g->fontData[i] = read_all(p);
		if (!g->fontData[i] || !stbtt_InitFont(&g->font[i], g->fontData[i], 0)) {
			fprintf(stderr, "can't load the font %s\n", p);
			gfx_free(g);
			return NULL;
		}
	}
	SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
	gfx_area(g, (SDL_Rect) { 0, 0, 640, 360 });
	gfx_done(g);
	return g;
}

void gfx_free(Gfx* g) {
	if (!g) return;
	for (int i = 0; i < g->nfaces; i++) SDL_DestroyTexture(g->faces[i].tex);
	for (int i = 0; i < FONTS; i++) free(g->fontData[i]);
	free(g);
}

SDL_Renderer* gfx_renderer(Gfx* g) { return g->ren; }

void gfx_area(Gfx* g, SDL_Rect a) {
	g->area = a;
	SDL_RenderSetClipRect(g->ren, &a);
	g->s = a.h / GFX_DESIGN_H;
	g->ox = a.x;
	g->oy = a.y;
	g->w = a.w / g->s;
	g->h = GFX_DESIGN_H;
}

float gfx_w(Gfx* g) { return g->w; }
float gfx_h(Gfx* g) { return g->h; }
float gfx_scale(Gfx* g) { return g->s; }

void gfx_to_px(Gfx* g, float x, float y, float* px, float* py) {
	*px = g->ox + x * g->s;
	*py = g->oy + y * g->s;
}

void gfx_from_px(Gfx* g, float px, float py, float* x, float* y) {
	*x = (px - g->ox) / g->s;
	*y = (py - g->oy) / g->s;
}

void gfx_clip(Gfx* g, float x, float y, float w, float h) {
	SDL_Rect r = { (int)floorf(g->ox + x * g->s), (int)floorf(g->oy + y * g->s), (int)ceilf(w * g->s),
		           (int)ceilf(h * g->s) };
	SDL_Rect in;
	if (!SDL_IntersectRect(&r, &g->area, &in)) in = (SDL_Rect) { g->area.x, g->area.y, 0, 0 };
	SDL_RenderSetClipRect(g->ren, &in);
}

void gfx_unclip(Gfx* g) { SDL_RenderSetClipRect(g->ren, &g->area); }

void gfx_done(Gfx* g) { SDL_RenderSetClipRect(g->ren, NULL); }

/* --- shapes: rounded boxes as triangle fans and strips, with a pixel of
 * fading edge so they look smooth ---------------------------------------- */

typedef struct {
	Color top, bot;
	float y0, y1;
	SDL_Texture* tex;
	float bx, by, bw, bh; /* the box the texture spans */
	float u0, v0, u1, v1;
} Paint;

static SDL_Vertex vtx(const Paint* p, float x, float y, float a) {
	float t = p->y1 > p->y0 ? (y - p->y0) / (p->y1 - p->y0) : 0;
	Color c = color_mix(p->top, p->bot, t);
	SDL_Vertex v = { { x, y }, { c.r, c.g, c.b, (uint8_t)(c.a * a) }, { 0, 0 } };
	if (p->tex) {
		float u = (x - p->bx) / p->bw, w = (y - p->by) / p->bh;
		u = u < 0 ? 0 : u > 1 ? 1 : u;
		w = w < 0 ? 0 : w > 1 ? 1 : w;
		v.tex_coord.x = p->u0 + u * (p->u1 - p->u0);
		v.tex_coord.y = p->v0 + w * (p->v1 - p->v0);
	}
	return v;
}

#define SEG_MAX 20
#define PATH_MAX_PTS (4 * (SEG_MAX + 1))

static int segs(float rad) {
	int n = (int)(rad / 3) + 3;
	return n > SEG_MAX ? SEG_MAX : n;
}

/* the outline of the box (pixels) moved out by d (in by -d), clockwise */
static int rr_path(float x, float y, float w, float h, float r, float d, int seg, SDL_FPoint* out) {
	float m = fminf(w, h) / 2;
	if (r > m) r = m;
	if (r < 0) r = 0;
	float rc = r;
	if (rc + d < 0) rc = -d;
	if (rc > m) rc = m;
	float rad = rc + d;
	if (rad < 0) rad = 0;
	float cx[4] = { x + w - rc, x + w - rc, x + rc, x + rc };
	float cy[4] = { y + rc, y + h - rc, y + h - rc, y + rc };
	int n = 0;
	for (int k = 0; k < 4; k++) {
		for (int i = 0; i <= seg; i++) {
			float a = (-90 + 90 * k + 90.0f * i / seg) * PI / 180;
			out[n++] = (SDL_FPoint) { cx[k] + cosf(a) * rad, cy[k] + sinf(a) * rad };
		}
	}
	return n;
}

static void rr_core(Gfx* g, float x, float y, float w, float h, float r, float d, int seg, const Paint* p) {
	SDL_FPoint pts[PATH_MAX_PTS];
	int n = rr_path(x, y, w, h, r, d, seg, pts);
	SDL_Vertex v[PATH_MAX_PTS + 1];
	int idx[PATH_MAX_PTS * 3];
	v[0] = vtx(p, x + w / 2, y + h / 2, 1);
	for (int i = 0; i < n; i++) v[i + 1] = vtx(p, pts[i].x, pts[i].y, 1);
	for (int i = 0; i < n; i++) {
		idx[i * 3] = 0;
		idx[i * 3 + 1] = i + 1;
		idx[i * 3 + 2] = (i + 1) % n + 1;
	}
	SDL_RenderGeometry(g->ren, p->tex, v, n + 1, idx, n * 3);
}

static void rr_ring(Gfx* g, float x, float y, float w, float h, float r, float d0, float d1, float a0, float a1,
                    int seg, const Paint* p) {
	SDL_FPoint in[PATH_MAX_PTS], out[PATH_MAX_PTS];
	int n = rr_path(x, y, w, h, r, d0, seg, in);
	rr_path(x, y, w, h, r, d1, seg, out);
	SDL_Vertex v[PATH_MAX_PTS * 2];
	int idx[PATH_MAX_PTS * 6];
	for (int i = 0; i < n; i++) {
		v[i * 2] = vtx(p, in[i].x, in[i].y, a0);
		v[i * 2 + 1] = vtx(p, out[i].x, out[i].y, a1);
	}
	for (int i = 0; i < n; i++) {
		int j = (i + 1) % n;
		int* q = &idx[i * 6];
		q[0] = i * 2;
		q[1] = i * 2 + 1;
		q[2] = j * 2 + 1;
		q[3] = i * 2;
		q[4] = j * 2 + 1;
		q[5] = j * 2;
	}
	SDL_RenderGeometry(g->ren, p->tex, v, n * 2, idx, n * 6);
}

static void fill_px(Gfx* g, float x, float y, float w, float h, float r, const Paint* p) {
	if (w <= 0 || h <= 0) return;
	int seg = segs(r);
	rr_core(g, x, y, w, h, r, -0.5f, seg, p);
	rr_ring(g, x, y, w, h, r, -0.5f, 0.5f, 1, 0, seg, p);
}

static Paint solid(Color c) { return (Paint) { .top = c, .bot = c }; }

void gfx_fill(Gfx* g, float x, float y, float w, float h, float r, Color c) {
	Paint p = solid(c);
	fill_px(g, g->ox + x * g->s, g->oy + y * g->s, w * g->s, h * g->s, r * g->s, &p);
}

void gfx_fill_grad(Gfx* g, float x, float y, float w, float h, float r, Color top, Color bottom) {
	Paint p = { .top = top, .bot = bottom, .y0 = g->oy + y * g->s, .y1 = g->oy + (y + h) * g->s };
	fill_px(g, g->ox + x * g->s, p.y0, w * g->s, h * g->s, r * g->s, &p);
}

void gfx_stroke(Gfx* g, float x, float y, float w, float h, float r, float t, Color c) {
	Paint p = solid(c);
	float X = g->ox + x * g->s, Y = g->oy + y * g->s, W = w * g->s, H = h * g->s, R = r * g->s, T = t * g->s;
	if (T < 1) T = 1;
	int seg = segs(R);
	rr_ring(g, X, Y, W, H, R, -T - 0.5f, -T + 0.5f, 0, 1, seg, &p);
	if (T > 1) rr_ring(g, X, Y, W, H, R, -T + 0.5f, -0.5f, 1, 1, seg, &p);
	rr_ring(g, X, Y, W, H, R, -0.5f, 0.5f, 1, 0, seg, &p);
}

void gfx_shadow(Gfx* g, float x, float y, float w, float h, float r, float blur, Color c) {
	Paint p = solid(c);
	float X = g->ox + x * g->s, Y = g->oy + y * g->s, W = w * g->s, H = h * g->s, R = r * g->s, B = blur * g->s;
	int seg = segs(R + B);
	rr_core(g, X, Y, W, H, R, -B / 2, seg, &p);
	rr_ring(g, X, Y, W, H, R, -B / 2, B / 3, 1, 0.3f, seg, &p);
	rr_ring(g, X, Y, W, H, R, B / 3, B, 0.3f, 0, seg, &p);
}

void gfx_glow(Gfx* g, float x, float y, float w, float h, float r, float blur, Color c) {
	Paint p = solid(c);
	float X = g->ox + x * g->s, Y = g->oy + y * g->s, W = w * g->s, H = h * g->s, R = r * g->s, B = blur * g->s;
	int seg = segs(R + B);
	rr_ring(g, X, Y, W, H, R, 0, B / 3, 1, 0.3f, seg, &p);
	rr_ring(g, X, Y, W, H, R, B / 3, B, 0.3f, 0, seg, &p);
}

void gfx_circle(Gfx* g, float cx, float cy, float rad, Color c) { gfx_fill(g, cx - rad, cy - rad, rad * 2, rad * 2, rad, c); }

void gfx_line(Gfx* g, float x0, float y0, float x1, float y1, float t, Color c) {
	float X0 = g->ox + x0 * g->s, Y0 = g->oy + y0 * g->s, X1 = g->ox + x1 * g->s, Y1 = g->oy + y1 * g->s;
	float dx = X1 - X0, dy = Y1 - Y0, len = sqrtf(dx * dx + dy * dy);
	if (len < 0.01f) return;
	float nx = -dy / len, ny = dx / len, h = t * g->s / 2;
	if (h < 0.5f) h = 0.5f;
	float o[4] = { -(h + 0.5f), -(h - 0.5f), h - 0.5f, h + 0.5f };
	float a[4] = { 0, 1, 1, 0 };
	SDL_Vertex v[8];
	for (int i = 0; i < 4; i++) {
		v[i] = (SDL_Vertex) { { X0 + nx * o[i], Y0 + ny * o[i] }, { c.r, c.g, c.b, (uint8_t)(c.a * a[i]) }, { 0, 0 } };
		v[i + 4] = (SDL_Vertex) { { X1 + nx * o[i], Y1 + ny * o[i] }, { c.r, c.g, c.b, (uint8_t)(c.a * a[i]) }, { 0, 0 } };
	}
	int idx[18];
	for (int i = 0; i < 3; i++) {
		int* q = &idx[i * 6];
		q[0] = i;
		q[1] = i + 1;
		q[2] = i + 5;
		q[3] = i;
		q[4] = i + 5;
		q[5] = i + 4;
	}
	SDL_RenderGeometry(g->ren, NULL, v, 8, idx, 18);
}

void gfx_tri(Gfx* g, float x0, float y0, float x1, float y1, float x2, float y2, Color c) {
	SDL_Color sc = { c.r, c.g, c.b, c.a };
	SDL_Vertex v[3] = {
		{ { g->ox + x0 * g->s, g->oy + y0 * g->s }, sc, { 0, 0 } },
		{ { g->ox + x1 * g->s, g->oy + y1 * g->s }, sc, { 0, 0 } },
		{ { g->ox + x2 * g->s, g->oy + y2 * g->s }, sc, { 0, 0 } },
	};
	SDL_RenderGeometry(g->ren, NULL, v, 3, NULL, 0);
}

/* a stroke through points, with round joins and ends */
static void polyline(Gfx* g, const float* p, int n, float t, Color c) {
	for (int i = 0; i + 1 < n; i++) gfx_line(g, p[i * 2], p[i * 2 + 1], p[i * 2 + 2], p[i * 2 + 3], t, c);
	for (int i = 0; i < n; i++) gfx_circle(g, p[i * 2], p[i * 2 + 1], t / 2, c);
}

void gfx_icon(Gfx* g, Icon icon, float cx, float cy, float s, Color c) {
	float t = s * 0.11f;
#define P(...)                                                                                                       \
	do {                                                                                                         \
		const float pts[] = { __VA_ARGS__ };                                                                 \
		float q[sizeof(pts) / sizeof(pts[0])];                                                               \
		for (size_t i = 0; i < sizeof(pts) / sizeof(pts[0]); i += 2) {                                       \
			q[i] = cx + pts[i] * s;                                                                      \
			q[i + 1] = cy + pts[i + 1] * s;                                                              \
		}                                                                                                    \
		polyline(g, q, (int)(sizeof(q) / sizeof(q[0])) / 2, t, c);                                           \
	} while (0)
	switch (icon) {
	case ICON_PLAY:
		gfx_tri(g, cx - .24f * s, cy - .34f * s, cx - .24f * s, cy + .34f * s, cx + .38f * s, cy, c);
		break;
	case ICON_PAUSE:
		gfx_fill(g, cx - .28f * s, cy - .32f * s, .2f * s, .64f * s, .05f * s, c);
		gfx_fill(g, cx + .08f * s, cy - .32f * s, .2f * s, .64f * s, .05f * s, c);
		break;
	case ICON_SAVE:
		P(0, -.38f, 0, .1f);
		P(-.2f, -.1f, 0, .1f, .2f, -.1f);
		P(-.36f, .1f, -.36f, .36f, .36f, .36f, .36f, .1f);
		break;
	case ICON_LOAD:
		P(0, .12f, 0, -.38f);
		P(-.2f, -.18f, 0, -.38f, .2f, -.18f);
		P(-.36f, .1f, -.36f, .36f, .36f, .36f, .36f, .1f);
		break;
	case ICON_GEAR:
		for (int i = 0; i < 8; i++) {
			float a = i * PI / 4;
			gfx_line(g, cx + cosf(a) * .26f * s, cy + sinf(a) * .26f * s, cx + cosf(a) * .46f * s,
			         cy + sinf(a) * .46f * s, .17f * s, c);
		}
		gfx_stroke(g, cx - .34f * s, cy - .34f * s, .68f * s, .68f * s, .34f * s, .15f * s, c);
		break;
	case ICON_HOME:
		P(-.42f, -.02f, 0, -.38f, .42f, -.02f);
		P(-.27f, -.12f, -.27f, .36f, .27f, .36f, .27f, -.12f);
		break;
	case ICON_POWER: {
		float q[34];
		for (int i = 0; i <= 16; i++) {
			float a = (-60 - 300.0f * i / 16) * PI / 180;
			q[i * 2] = cx + cosf(a) * .34f * s;
			q[i * 2 + 1] = cy + sinf(a) * .34f * s + .04f * s;
		}
		polyline(g, q, 17, t, c);
		P(0, -.42f, 0, -.04f);
		break;
	}
	case ICON_BACK: P(.12f, -.3f, -.18f, 0, .12f, .3f); break;
	case ICON_LEFT: P(.1f, -.24f, -.14f, 0, .1f, .24f); break;
	case ICON_RIGHT: P(-.1f, -.24f, .14f, 0, -.1f, .24f); break;
	case ICON_CHECK: P(-.3f, 0, -.08f, .24f, .32f, -.24f); break;
	case ICON_GRID:
		for (int i = 0; i < 4; i++) {
			gfx_fill(g, cx + (i % 2 ? .05f : -.37f) * s, cy + (i / 2 ? .05f : -.37f) * s, .32f * s, .32f * s,
			         .07f * s, c);
		}
		break;
	}
#undef P
}

/* --- text ---------------------------------------------------------------- */

static int char_index(uint32_t cp) {
	int base = 0;
	for (int i = 0; i < RANGES; i++) {
		if (cp >= (uint32_t)ranges[i].first && cp < (uint32_t)(ranges[i].first + ranges[i].n))
			return base + (int)(cp - ranges[i].first);
		base += ranges[i].n;
	}
	return '?' - 32;
}

static uint32_t utf8_next(const char** s) {
	const unsigned char* p = (const unsigned char*)*s;
	uint32_t c = *p++;
	if (c >= 0xF0 && p[0] && p[1] && p[2]) {
		c = (c & 7) << 18 | (p[0] & 63) << 12 | (p[1] & 63) << 6 | (p[2] & 63);
		p += 3;
	} else if (c >= 0xE0 && p[0] && p[1]) {
		c = (c & 15) << 12 | (p[0] & 63) << 6 | (p[1] & 63);
		p += 2;
	} else if (c >= 0xC0 && p[0]) {
		c = (c & 31) << 6 | (p[0] & 63);
		p += 1;
	}
	*s = (const char*)p;
	return c;
}

static Face* face(Gfx* g, int font, float size) {
	int px = (int)(size * g->s + 0.5f);
	if (px < 6) px = 6;
	for (int i = 0; i < g->nfaces; i++) {
		if (g->faces[i].font == font && g->faces[i].px == px) return &g->faces[i];
	}
	Face* f;
	if (g->nfaces < FACES) {
		f = &g->faces[g->nfaces++];
	} else {
		f = &g->faces[g->nextFace];
		g->nextFace = (g->nextFace + 1) % FACES;
		SDL_DestroyTexture(f->tex);
	}
	memset(f, 0, sizeof(*f));
	f->font = font;
	f->px = px;
	int asc, desc, gap;
	stbtt_GetFontVMetrics(&g->font[font], &asc, &desc, &gap);
	f->scale = stbtt_ScaleForPixelHeight(&g->font[font], (float)px);
	f->ascent = asc * f->scale;
	/* big enough for all the glyphs at this size */
	for (int tw = 256; tw <= 4096; tw *= 2) {
		int th = tw;
		unsigned char* bmp = calloc(tw, th);
		stbtt_pack_context pc;
		stbtt_PackBegin(&pc, bmp, tw, th, 0, 1, NULL);
		stbtt_PackSetOversampling(&pc, 1, 1);
		stbtt_pack_range rs[RANGES];
		int base = 0;
		for (int i = 0; i < RANGES; i++) {
			rs[i] = (stbtt_pack_range) { .font_size = (float)px, .first_unicode_codepoint_in_range = ranges[i].first,
				                         .num_chars = ranges[i].n, .chardata_for_range = &f->pc[base] };
			base += ranges[i].n;
		}
		int ok = stbtt_PackFontRanges(&pc, g->fontData[font], 0, rs, RANGES);
		stbtt_PackEnd(&pc);
		if (ok) {
			uint32_t* rgba = malloc((size_t)tw * th * 4);
			for (int i = 0; i < tw * th; i++) rgba[i] = 0x00FFFFFFu | (uint32_t)bmp[i] << 24;
			f->tex = SDL_CreateTexture(g->ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, tw, th);
			SDL_UpdateTexture(f->tex, NULL, rgba, tw * 4);
			SDL_SetTextureBlendMode(f->tex, SDL_BLENDMODE_BLEND);
			f->tw = tw;
			f->th = th;
			free(rgba);
			free(bmp);
			break;
		}
		free(bmp);
	}
	return f;
}

/* width in pixels */
static float run_w(Gfx* g, Face* f, const char* s) {
	float w = 0;
	uint32_t prev = 0;
	while (*s) {
		uint32_t c = utf8_next(&s);
		if (prev) w += stbtt_GetCodepointKernAdvance(&g->font[f->font], prev, c) * f->scale;
		w += f->pc[char_index(c)].xadvance;
		prev = c;
	}
	return w;
}

float gfx_text_w(Gfx* g, int font, float size, const char* s) { return run_w(g, face(g, font, size), s) / g->s; }

float gfx_text(Gfx* g, int font, float size, float x, float y, Color c, int align, const char* s) {
	Face* f = face(g, font, size);
	if (!f->tex) return 0;
	float w = run_w(g, f, s);
	float px = g->ox + x * g->s, py = g->oy + y * g->s;
	if (align == ALIGN_CENTER) px -= w / 2;
	else if (align == ALIGN_RIGHT) px -= w;
	px = floorf(px + 0.5f);
	float pen = 0, base = floorf(py + f->ascent + 0.5f);
	int n = (int)strlen(s);
	SDL_Vertex* v = malloc(sizeof(SDL_Vertex) * 4 * (n + 1));
	int* idx = malloc(sizeof(int) * 6 * (n + 1));
	int k = 0;
	uint32_t prev = 0;
	SDL_Color sc = { c.r, c.g, c.b, c.a };
	while (*s) {
		uint32_t ch = utf8_next(&s);
		if (prev) pen += stbtt_GetCodepointKernAdvance(&g->font[f->font], prev, ch) * f->scale;
		prev = ch;
		float xp = px + pen, yp = base;
		stbtt_aligned_quad q;
		stbtt_GetPackedQuad(f->pc, f->tw, f->th, char_index(ch), &xp, &yp, &q, 1);
		pen = xp - px;
		if (q.x1 <= q.x0) continue;
		SDL_Vertex* o = &v[k * 4];
		o[0] = (SDL_Vertex) { { q.x0, q.y0 }, sc, { q.s0, q.t0 } };
		o[1] = (SDL_Vertex) { { q.x1, q.y0 }, sc, { q.s1, q.t0 } };
		o[2] = (SDL_Vertex) { { q.x1, q.y1 }, sc, { q.s1, q.t1 } };
		o[3] = (SDL_Vertex) { { q.x0, q.y1 }, sc, { q.s0, q.t1 } };
		int* i6 = &idx[k * 6];
		i6[0] = k * 4;
		i6[1] = k * 4 + 1;
		i6[2] = k * 4 + 2;
		i6[3] = k * 4;
		i6[4] = k * 4 + 2;
		i6[5] = k * 4 + 3;
		k++;
	}
	if (k) SDL_RenderGeometry(g->ren, f->tex, v, k * 4, idx, k * 6);
	free(v);
	free(idx);
	return w / g->s;
}

void gfx_text_fit(Gfx* g, int font, float size, float x, float y, float maxW, Color c, int align, const char* s) {
	if (gfx_text_w(g, font, size, s) <= maxW) {
		gfx_text(g, font, size, x, y, c, align, s);
		return;
	}
	size_t n = strlen(s);
	char* b = malloc(n + 4);
	memcpy(b, s, n + 1);
	while (n > 0) {
		/* back to the previous character, and trailing spaces */
		do n--;
		while (n > 0 && ((unsigned char)b[n] & 0xC0) == 0x80);
		while (n > 0 && b[n - 1] == ' ') n--;
		memcpy(b + n, "\xE2\x80\xA6", 4);
		if (gfx_text_w(g, font, size, b) <= maxW) break;
	}
	gfx_text(g, font, size, x, y, c, align, b);
	free(b);
}

int gfx_text_wrap(Gfx* g, int font, float size, float x, float y, float maxW, int maxLines, float lineH, Color c,
                  int align, const char* s) {
	char line[512];
	int lines = 0;
	const char* p = s;
	while (*p && lines < maxLines) {
		while (*p == ' ') p++;
		/* as many words as fit */
		size_t len = 0, fit = 0;
		const char* end = p;
		for (;;) {
			const char* w = end;
			while (*w == ' ') w++;
			const char* we = w;
			while (*we && *we != ' ') we++;
			if (we == w) break;
			size_t nl = (size_t)(we - p);
			if (nl >= sizeof(line)) break;
			memcpy(line, p, nl);
			line[nl] = 0;
			if (fit && gfx_text_w(g, font, size, line) > maxW) break;
			fit = nl;
			end = we;
			len = nl;
			if (!*we) break;
		}
		if (!len) break;
		bool last = lines == maxLines - 1;
		if (last) snprintf(line, sizeof(line), "%s", p);
		else {
			memcpy(line, p, len);
			line[len] = 0;
		}
		gfx_text_fit(g, font, size, x, y + lines * lineH, maxW, c, align, line);
		lines++;
		p = last ? p + strlen(p) : end;
	}
	return lines;
}

/* --- pictures ------------------------------------------------------------ */

bool gfx_image_load(Gfx* g, const char* path, Image* out) {
	int w, h, n;
	uint8_t* px = stbi_load(path, &w, &h, &n, 4);
	if (!px) return false;
	/* big scans are shrunk (averaging), so they don't shimmer when drawn small */
	int k = 1;
	while (w / k > 640 || h / k > 640) k++;
	int ow = w / k, oh = h / k;
	uint8_t* small = px;
	if (k > 1) {
		small = malloc((size_t)ow * oh * 4);
		for (int y = 0; y < oh; y++) {
			for (int x = 0; x < ow; x++) {
				unsigned sum[4] = { 0 };
				for (int j = 0; j < k; j++) {
					const uint8_t* row = px + ((size_t)(y * k + j) * w + x * k) * 4;
					for (int i = 0; i < k * 4; i++) sum[i & 3] += row[i];
				}
				for (int ch = 0; ch < 4; ch++) small[((size_t)y * ow + x) * 4 + ch] = (uint8_t)(sum[ch] / (k * k));
			}
		}
	}
	out->tex = SDL_CreateTexture(g->ren, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, ow, oh);
	if (out->tex) {
		SDL_UpdateTexture(out->tex, NULL, small, ow * 4);
		SDL_SetTextureBlendMode(out->tex, SDL_BLENDMODE_BLEND);
		SDL_SetTextureScaleMode(out->tex, SDL_ScaleModeLinear);
	}
	out->w = ow;
	out->h = oh;
	if (small != px) free(small);
	stbi_image_free(px);
	return out->tex != NULL;
}

bool gfx_image_pixels(Gfx* g, const uint32_t* px, int w, int h, Image* io) {
	if (!io->tex || io->w != w || io->h != h) {
		gfx_image_free(io);
		io->tex = SDL_CreateTexture(g->ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
		if (!io->tex) return false;
		SDL_SetTextureScaleMode(io->tex, SDL_ScaleModeLinear);
		SDL_SetTextureBlendMode(io->tex, SDL_BLENDMODE_BLEND);
		io->w = w;
		io->h = h;
	}
	SDL_UpdateTexture(io->tex, NULL, px, w * 4);
	return true;
}

void gfx_image_free(Image* im) {
	if (im->tex) SDL_DestroyTexture(im->tex);
	im->tex = NULL;
	im->w = im->h = 0;
}

void gfx_image(Gfx* g, const Image* im, float x, float y, float w, float h, float r, bool cover, float alpha) {
	if (!im->tex || im->w <= 0 || im->h <= 0) return;
	Paint p = solid(with_alpha(rgb(0xFFFFFF), alpha));
	p.tex = im->tex;
	p.u0 = p.v0 = 0;
	p.u1 = p.v1 = 1;
	float ia = (float)im->w / im->h, ba = w / h;
	if (cover) {
		if (ia > ba) {
			float f = ba / ia;
			p.u0 = (1 - f) / 2;
			p.u1 = 1 - p.u0;
		} else {
			float f = ia / ba;
			p.v0 = (1 - f) / 2;
			p.v1 = 1 - p.v0;
		}
	} else if (ia > ba) {
		float nh = w / ia;
		y += (h - nh) / 2;
		h = nh;
	} else {
		float nw = h * ia;
		x += (w - nw) / 2;
		w = nw;
	}
	float X = g->ox + x * g->s, Y = g->oy + y * g->s, W = w * g->s, H = h * g->s;
	p.bx = X;
	p.by = Y;
	p.bw = W;
	p.bh = H;
	fill_px(g, X, Y, W, H, r * g->s, &p);
}
