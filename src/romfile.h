/* ROM files: plain (.sfc/.smc) or the first ROM inside a .zip, and finding
 * the games with a profile in a folder (for a device without a game list). */
#ifndef SNESDS_ROMFILE_H
#define SNESDS_ROMFILE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* the ROM's bytes (malloc'd), or NULL */
uint8_t* romfile_read(const char* path, size_t* size);

/* a ROM's name: .sfc, .smc or .zip */
bool romfile_is_rom(const char* path);
/* the CRC32 of the ROM without a copier header (what profiles go by);
 * zips are read only as far as their directory when they can be */
bool romfile_crc(const char* path, uint32_t* crc);
/* a folder's entries (not . files), sorted, malloc'd (each and the list) */
int romfile_list_dir(const char* dir, char*** out);

/* the first ROM (by name) in dir or its subfolders that has a second-screen
 * profile, into out; false if none */
bool romfile_find(const char* dir, char* out, size_t n);

#endif
