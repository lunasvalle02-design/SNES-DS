/* The profile engine: finding a game's profile and evaluating it. No game's
 * data lives here; profiles are in games/<id>/profile.c. */
#include "profiles.h"

#include <stddef.h>

/* games/games.c */
extern const Profile* const games[];

uint32_t rom_crc32(const uint8_t* rom, size_t size) {
	/* a 512-byte copier header doesn't count */
	if (size % 0x8000 == 512) {
		rom += 512;
		size -= 512;
	}
	uint32_t crc = 0xFFFFFFFF;
	for (size_t i = 0; i < size; i++) {
		crc ^= rom[i];
		for (int k = 0; k < 8; k++) crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
	}
	return ~crc;
}

const Profile* profile_find(const uint8_t* rom, size_t size) {
	uint32_t crc = rom_crc32(rom, size);
	for (int i = 0; games[i]; i++) {
		for (int j = 0; j < 4 && games[i]->crc32[j]; j++) {
			if (games[i]->crc32[j] == crc) return games[i];
		}
	}
	return NULL;
}

bool probes_match(Core* c, const MemProbe* p) {
	for (int i = 0; i < PROBES && p[i].addr; i++) {
		uint8_t b = core_peek(c, p[i].addr) & p[i].mask;
		if (b != p[i].val && !(p[i].hasAlt && b == p[i].alt)) return false;
	}
	return true;
}

const SplitRule* profile_split(const Profile* p, Core* c) {
	if (!p) return NULL;
	for (int i = 0; i < SPLITS && p->split[i].layers; i++) {
		if (probes_match(c, p->split[i].when)) return &p->split[i];
	}
	return NULL;
}
