#include "romfile.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include "profiles.h"
#include "zip.h"

static bool ends_with(const char* s, const char* ext) {
	size_t n = strlen(s), e = strlen(ext);
	return n >= e && !strcasecmp(s + n - e, ext);
}

static bool is_rom_name(const char* s) { return ends_with(s, ".sfc") || ends_with(s, ".smc"); }

static uint8_t* read_plain(const char* path, size_t* size) {
	FILE* f = fopen(path, "rb");
	if (!f) return NULL;
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	fseek(f, 0, SEEK_SET);
	uint8_t* d = n > 0 ? malloc(n) : NULL;
	if (d && fread(d, 1, n, f) != (size_t)n) {
		free(d);
		d = NULL;
	}
	fclose(f);
	*size = d ? (size_t)n : 0;
	return d;
}

/* open the first ROM inside a zip; the zip, or NULL */
static struct zip_t* open_zip_rom(const char* path) {
	struct zip_t* z = zip_open(path, 0, 'r');
	if (!z) return NULL;
	int n = zip_total_entries(z);
	for (int i = 0; i < n; i++) {
		if (zip_entry_openbyindex(z, i) != 0) continue;
		if (is_rom_name(zip_entry_name(z))) return z;
		zip_entry_close(z);
	}
	zip_close(z);
	return NULL;
}

uint8_t* romfile_read(const char* path, size_t* size) {
	if (!ends_with(path, ".zip")) return read_plain(path, size);
	struct zip_t* z = open_zip_rom(path);
	if (!z) return NULL;
	void* buf = NULL;
	size_t n = 0;
	if (zip_entry_read(z, &buf, &n) < 0) {
		free(buf);
		buf = NULL;
	}
	zip_entry_close(z);
	zip_close(z);
	*size = buf ? n : 0;
	return buf;
}

bool romfile_crc(const char* path, uint32_t* crc) {
	if (ends_with(path, ".zip")) {
		/* zips say their files' CRC32 without unpacking them (unless a
		 * copier header has to come off first) */
		struct zip_t* z = open_zip_rom(path);
		if (!z) return false;
		bool header = zip_entry_size(z) % 0x8000 == 512;
		*crc = zip_entry_crc32(z);
		zip_entry_close(z);
		zip_close(z);
		if (!header) return true;
	} else if (!is_rom_name(path)) {
		return false;
	}
	size_t n;
	uint8_t* d = romfile_read(path, &n);
	if (d) *crc = rom_crc32(d, n);
	free(d);
	return d != NULL;
}

bool romfile_is_rom(const char* path) { return is_rom_name(path) || ends_with(path, ".zip"); }

static bool has_profile(const char* path) {
	uint32_t crc;
	return romfile_crc(path, &crc) && profile_find_crc(crc);
}

static int cmp_names(const void* a, const void* b) { return strcmp(*(char* const*)a, *(char* const*)b); }

/* the entries of a folder, sorted */
int romfile_list_dir(const char* dir, char*** out) {
	DIR* d = opendir(dir);
	if (!d) return 0;
	int n = 0, cap = 0;
	char** names = NULL;
	for (struct dirent* e; (e = readdir(d));) {
		if (e->d_name[0] == '.') continue;
		if (n == cap) {
			cap = cap ? cap * 2 : 64;
			names = realloc(names, cap * sizeof(char*));
		}
		names[n++] = strdup(e->d_name);
	}
	closedir(d);
	qsort(names, n, sizeof(char*), cmp_names);
	*out = names;
	return n;
}

static bool find_in(const char* dir, char* out, size_t n, int depth) {
	char** names = NULL;
	int count = romfile_list_dir(dir, &names);
	bool found = false;
	for (int i = 0; i < count; i++) {
		char p[1024];
		snprintf(p, sizeof(p), "%s/%s", dir, names[i]);
		struct stat st;
		if (!found && stat(p, &st) == 0) {
			if (S_ISDIR(st.st_mode)) {
				if (depth > 0) found = find_in(p, out, n, depth - 1);
			} else if (romfile_is_rom(p) && has_profile(p)) {
				snprintf(out, n, "%s", p);
				found = true;
			}
		}
		free(names[i]);
	}
	free(names);
	return found;
}

bool romfile_find(const char* dir, char* out, size_t n) { return find_in(dir, out, n, 1); }
