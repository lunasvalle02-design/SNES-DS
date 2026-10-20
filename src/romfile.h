/* ROM files: plain (.sfc/.smc) or the first ROM inside a .zip, and finding
 * the games with a profile in a folder (for a device without a game list). */
#ifndef SNESDS_ROMFILE_H
#define SNESDS_ROMFILE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* the ROM's bytes (malloc'd), or NULL */
uint8_t* romfile_read(const char* path, size_t* size);

/* the first ROM (by name) in dir or its subfolders that has a second-screen
 * profile, into out; false if none */
bool romfile_find(const char* dir, char* out, size_t n);

#endif
