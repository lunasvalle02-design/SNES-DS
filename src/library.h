/* The game list: the ROMs in a folder (and its subfolders) that have a
 * second-screen profile, each with its cover when one is around. Telling a
 * ROM's game means its CRC32, so what each file turned out to be is kept in
 * a cache file and only new or changed files are read. */
#ifndef SNESDS_LIBRARY_H
#define SNESDS_LIBRARY_H

#include "profiles.h"

typedef struct {
	char* path;
	char* name;     /* the file's name without its extension */
	char* cover;    /* an image of the game (box art, screenshot), or NULL */
	const Profile* profile;
} LibGame;

typedef struct {
	LibGame* games;
	int count;
	int scanned; /* ROM files looked at */
} Library;

/* cachePath may be NULL (no cache) */
void library_scan(Library* lib, const char* dir, const char* cachePath);
void library_free(Library* lib);

#endif
