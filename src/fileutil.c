#include "fileutil.h"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

static void tmp_path(char* out, size_t n, const char* path) { snprintf(out, n, "%s.tmp", path); }

FILE* atomic_open(const char* path) {
	char tmp[1200];
	tmp_path(tmp, sizeof(tmp), path);
	return fopen(tmp, "wb");
}

/* the folder's entry for the renamed file on the disk too (Linux) */
static void sync_dir(const char* path) {
#ifndef _WIN32
	char dir[1200];
	snprintf(dir, sizeof(dir), "%s", path);
	char* slash = strrchr(dir, '/');
	if (slash) *slash = 0;
	else snprintf(dir, sizeof(dir), ".");
	int fd = open(dir, O_RDONLY);
	if (fd >= 0) {
		fsync(fd);
		close(fd);
	}
#else
	(void)path;
#endif
}

bool atomic_close(FILE* f, const char* path, bool ok) {
	char tmp[1200];
	tmp_path(tmp, sizeof(tmp), path);
	if (!f) return false;
	ok = fflush(f) == 0 && ok;
#ifdef _WIN32
	ok = ok && _commit(_fileno(f)) == 0;
#else
	ok = ok && fsync(fileno(f)) == 0;
#endif
	ok = fclose(f) == 0 && ok;
	if (!ok) {
		remove(tmp);
		return false;
	}
#ifdef _WIN32
	ok = MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
	ok = rename(tmp, path) == 0;
#endif
	if (!ok) remove(tmp);
	else sync_dir(path);
	return ok;
}

bool file_write_atomic(const char* path, const void* data, size_t size) {
	FILE* f = atomic_open(path);
	if (!f) return false;
	return atomic_close(f, path, fwrite(data, 1, size, f) == size);
}
