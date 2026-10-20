#include "library.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "fileutil.h"
#include "romfile.h"

/* --- the cache: "size mtime crc path" lines, crc 0 for files that aren't
 * a ROM ------------------------------------------------------------------ */

typedef struct {
	long long size, mtime;
	uint32_t crc;
	char* path;
} CacheLine;

typedef struct {
	CacheLine* lines;
	int count, cap;
	bool changed;
} Cache;

static void cache_add(Cache* c, long long size, long long mtime, uint32_t crc, const char* path) {
	if (c->count == c->cap) {
		c->cap = c->cap ? c->cap * 2 : 64;
		c->lines = realloc(c->lines, c->cap * sizeof(CacheLine));
	}
	c->lines[c->count++] = (CacheLine) { size, mtime, crc, strdup(path) };
}

static void cache_load(Cache* c, const char* path) {
	FILE* f = path ? fopen(path, "r") : NULL;
	if (!f) return;
	char line[1200];
	while (fgets(line, sizeof(line), f)) {
		long long size, mtime;
		unsigned crc;
		int at = 0;
		if (sscanf(line, "%lld %lld %x %n", &size, &mtime, &crc, &at) < 3 || !at) continue;
		line[strcspn(line, "\r\n")] = 0;
		cache_add(c, size, mtime, crc, line + at);
	}
	fclose(f);
}

static CacheLine* cache_get(Cache* c, const char* path) {
	for (int i = 0; i < c->count; i++) {
		if (!strcmp(c->lines[i].path, path)) return &c->lines[i];
	}
	return NULL;
}

static void cache_save(Cache* c, const char* path) {
	FILE* f = path && c->changed ? atomic_open(path) : NULL;
	if (f) {
		bool ok = true;
		for (int i = 0; i < c->count; i++) {
			ok = fprintf(f, "%lld %lld %08x %s\n", c->lines[i].size, c->lines[i].mtime, (unsigned)c->lines[i].crc,
			             c->lines[i].path) > 0 && ok;
		}
		atomic_close(f, path, ok);
	}
	for (int i = 0; i < c->count; i++) free(c->lines[i].path);
	free(c->lines);
}

/* --- covers -------------------------------------------------------------- */

static bool exists(const char* p) {
	struct stat st;
	return stat(p, &st) == 0 && S_ISREG(st.st_mode);
}

/* the text of <tag> in [s, end), unescaped, into out */
static bool xml_tag(const char* s, const char* end, const char* tag, char* out, size_t n) {
	char open[32];
	snprintf(open, sizeof(open), "<%s>", tag);
	const char* a = strstr(s, open);
	if (!a || a >= end) return false;
	a += strlen(open);
	const char* b = strstr(a, "</");
	if (!b || b > end) return false;
	size_t k = 0;
	while (a < b && k + 1 < n) {
		static const struct { const char* ent; char c; } ents[] = {
			{ "&amp;", '&' }, { "&apos;", '\'' }, { "&quot;", '"' }, { "&lt;", '<' }, { "&gt;", '>' },
		};
		bool done = false;
		for (size_t i = 0; i < sizeof(ents) / sizeof(ents[0]) && !done; i++) {
			size_t len = strlen(ents[i].ent);
			if (!strncmp(a, ents[i].ent, len)) {
				out[k++] = ents[i].c;
				a += len;
				done = true;
			}
		}
		if (!done) out[k++] = *a++;
	}
	out[k] = 0;
	return k > 0;
}

/* EmulationStation's gamelist.xml in the ROM's folder: its <thumbnail> (box
 * art, as scrapers fill it) or else its <image> */
static char* gamelist_cover(const char* dir, const char* file) {
	char p[1100];
	snprintf(p, sizeof(p), "%s/gamelist.xml", dir);
	size_t n;
	char* xml = (char*)romfile_read(p, &n); /* not a .zip: read as it is */
	if (!xml) return NULL;
	xml = realloc(xml, n + 1);
	xml[n] = 0;
	char* found = NULL;
	for (char* g = strstr(xml, "<game"); g && !found; g = strstr(g + 5, "<game")) {
		char* end = strstr(g, "</game>");
		if (!end) break;
		char path[1024];
		if (!xml_tag(g, end, "path", path, sizeof(path))) continue;
		const char* base = strrchr(path, '/');
		base = base ? base + 1 : path;
		if (strcmp(base, file)) continue;
		static const char* const tags[] = { "thumbnail", "image" };
		for (int t = 0; t < 2 && !found; t++) {
			char img[1024];
			if (!xml_tag(g, end, tags[t], img, sizeof(img))) continue;
			char full[2200];
			if (img[0] == '/') snprintf(full, sizeof(full), "%s", img);
			else snprintf(full, sizeof(full), "%s/%s", dir, img[0] == '.' && img[1] == '/' ? img + 2 : img);
			if (exists(full)) found = strdup(full);
		}
	}
	free(xml);
	return found;
}

static char* find_cover(const char* dir, const char* file, const char* name) {
	char* c = gamelist_cover(dir, file);
	if (c) return c;
	/* the usual places scrapers leave them */
	static const char* const pats[] = {
		"%s/images/%s-thumb.%s", "%s/images/%s-image.%s", "%s/images/%s.%s", "%s/media/box2dfront/%s.%s",
		"%s/media/images/%s.%s", "%s/boxart/%s.%s", "%s/covers/%s.%s", "%s/%s.%s",
	};
	static const char* const exts[] = { "png", "jpg" };
	for (size_t i = 0; i < sizeof(pats) / sizeof(pats[0]); i++) {
		for (int e = 0; e < 2; e++) {
			char p[1200];
			snprintf(p, sizeof(p), pats[i], dir, name, exts[e]);
			if (exists(p)) return strdup(p);
		}
	}
	return NULL;
}

char* library_cover(const char* romPath) {
	const char* file = strrchr(romPath, '/');
	const char* bs = strrchr(romPath, '\\');
	if (!file || (bs && bs > file)) file = bs;
	char dir[1024];
	if (file) {
		snprintf(dir, sizeof(dir), "%.*s", (int)(file - romPath), romPath);
		file++;
	} else {
		snprintf(dir, sizeof(dir), ".");
		file = romPath;
	}
	char name[512];
	snprintf(name, sizeof(name), "%s", file);
	char* dot = strrchr(name, '.');
	if (dot) *dot = 0;
	return find_cover(dir, file, name);
}

/* --- the scan------------------------------------------------------------ */

static void add_game(Library* lib, const char* dir, const char* file, const char* path, const Profile* p) {
	lib->games = realloc(lib->games, (lib->count + 1) * sizeof(LibGame));
	LibGame* g = &lib->games[lib->count++];
	g->path = strdup(path);
	const char* dot = strrchr(file, '.');
	size_t len = dot ? (size_t)(dot - file) : strlen(file);
	g->name = malloc(len + 1);
	memcpy(g->name, file, len);
	g->name[len] = 0;
	g->profile = p;
	g->cover = find_cover(dir, file, g->name);
}

static void scan(Library* lib, Cache* cache, const char* dir, int depth) {
	char** names = NULL;
	int count = romfile_list_dir(dir, &names);
	for (int i = 0; i < count; i++) {
		char p[1024];
		snprintf(p, sizeof(p), "%s/%s", dir, names[i]);
		struct stat st;
		if (stat(p, &st) != 0) {
			/* skip */
		} else if (S_ISDIR(st.st_mode)) {
			/* not the scrapers' image folders */
			if (depth > 0 && strcmp(names[i], "images") && strcmp(names[i], "media") && strcmp(names[i], "videos") &&
			    strcmp(names[i], "manuals"))
				scan(lib, cache, p, depth - 1);
		} else if (romfile_is_rom(p)) {
			lib->scanned++;
			CacheLine* l = cache_get(cache, p);
			uint32_t crc = 0;
			if (l && l->size == (long long)st.st_size && l->mtime == (long long)st.st_mtime) {
				crc = l->crc;
			} else {
				if (!romfile_crc(p, &crc)) crc = 0;
				if (l) {
					l->size = st.st_size;
					l->mtime = st.st_mtime;
					l->crc = crc;
				} else {
					cache_add(cache, st.st_size, st.st_mtime, crc, p);
				}
				cache->changed = true;
			}
			const Profile* prof = crc ? profile_find_crc(crc) : NULL;
			if (prof) add_game(lib, dir, names[i], p, prof);
		}
		free(names[i]);
	}
	free(names);
}

void library_scan(Library* lib, const char* dir, const char* cachePath) {
	memset(lib, 0, sizeof(*lib));
	Cache cache = { 0 };
	cache_load(&cache, cachePath);
	scan(lib, &cache, dir, 1);
	cache_save(&cache, cachePath);
}

void library_add_icons(Library* lib, const char* dir) {
	for (int i = 0; i < lib->count; i++) {
		LibGame* g = &lib->games[i];
		if (g->cover) continue;
		char p[1100];
		snprintf(p, sizeof(p), "%s/%s.png", dir, g->profile->id);
		if (exists(p)) g->cover = strdup(p);
	}
}

void library_free(Library* lib) {
	for (int i = 0; i < lib->count; i++) {
		free(lib->games[i].path);
		free(lib->games[i].name);
		free(lib->games[i].cover);
	}
	free(lib->games);
	memset(lib, 0, sizeof(*lib));
}
