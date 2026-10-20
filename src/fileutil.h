/* Writing files so that a crash or a power cut (a handheld's battery running
 * out) leaves either the old file or the new one, never half of one: the
 * data goes to PATH.tmp, is flushed to the disk, and only then renamed over
 * PATH. For battery saves, save states, the settings. */
#ifndef SNESDS_FILEUTIL_H
#define SNESDS_FILEUTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

bool file_write_atomic(const char* path, const void* data, size_t size);

/* the same, for a file written in pieces: write to what atomic_open gives,
 * then atomic_close (ok false: something went wrong, keep the old file) */
FILE* atomic_open(const char* path);
bool atomic_close(FILE* f, const char* path, bool ok);

#endif
