/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * See dm_settings.h.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "dm_settings.h"
#include "gamma_tables.h"

const int dm_stage_level[DM_AREAS][DM_STAGES_PER_AREA] = {
	{  3,  4,  6 },     /* The Suburbs   */
	{  8,  9, 11 },     /* The Sewers    */
	{ 13, 14, 16 },     /* Scaffolding   */
	{ 18, 19, 21 },     /* The Factory   */
};

int dm_is_stage(int level)
{
	int a, s;
	for (a = 0; a < DM_AREAS; a++)
		for (s = 0; s < DM_STAGES_PER_AREA; s++)
			if (dm_stage_level[a][s] == level) return 1;
	return 0;
}

void dm_settings_default(struct dm_settings *s)
{
	s->mobster = 1;
	s->music = DM_VOLUME_MAX;
	s->sound = DM_VOLUME_MAX;
	s->brightness = 0;
	s->contrast = 0;
	s->gamma = DM_GAMMA_DEFAULT;
}

static int clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

void dm_settings_load(struct dm_settings *s)
{
	char line[80], key[32];
	int value;
	FILE *f;

	dm_settings_default(s);
	f = fopen(DM_SETTINGS_FILE, "r");
	if (!f) return;
	while (fgets(line, sizeof(line), f)) {
		if (sscanf(line, "%31[A-Z]=%d", key, &value) != 2) continue;
		if (!strcmp(key, "MOBSTER"))    s->mobster = clamp(value, 1, 2);
		if (!strcmp(key, "MUSIC"))      s->music = clamp(value, 0, DM_VOLUME_MAX);
		if (!strcmp(key, "SOUND"))      s->sound = clamp(value, 0, DM_VOLUME_MAX);
		if (!strcmp(key, "BRIGHTNESS")) s->brightness = clamp(value, -DM_PICTURE_RANGE, DM_PICTURE_RANGE);
		if (!strcmp(key, "CONTRAST"))   s->contrast = clamp(value, -DM_PICTURE_RANGE, DM_PICTURE_RANGE);
		if (!strcmp(key, "GAMMA"))      s->gamma = clamp(value, DM_GAMMA_MIN, DM_GAMMA_MAX);
	}
	fclose(f);
}

int dm_write_file(const char *name, const char *text)
{
	FILE *f = fopen(name, "w");
	int ok;
	if (!f) return -1;
	ok = fputs(text, f) >= 0;
	ok = (fflush(f) == 0) && ok;
	fsync(fileno(f));
	ok = (fclose(f) == 0) && ok;
	return ok ? 0 : -1;
}

int dm_settings_save(const struct dm_settings *s)
{
	char text[256];
	snprintf(text, sizeof(text),
	         "MOBSTER=%d\nMUSIC=%d\nSOUND=%d\nBRIGHTNESS=%d\nCONTRAST=%d\nGAMMA=%d\n",
	         s->mobster, s->music, s->sound, s->brightness, s->contrast, s->gamma);
	return dm_write_file(DM_SETTINGS_FILE, text);
}

void dm_colour_lut(const struct dm_settings *s, unsigned char lut[256])
{
	const unsigned char *gamma = dm_gamma_tables[clamp(s->gamma, DM_GAMMA_MIN, DM_GAMMA_MAX) - DM_GAMMA_MIN];
	int i;
	for (i = 0; i < 256; i++) {
		int v = gamma[i];
		v = 128 + (v - 128) * (100 + 6 * s->contrast) / 100;     /* 70% .. 130% */
		v += 8 * s->brightness;                                  /* -40 .. +40  */
		lut[i] = (unsigned char)clamp(v, 0, 255);
	}
}

int dm_is_music(const char *path)
{
	const char *name = strrchr(path, '/');
	name = name ? name + 1 : path;
	/* lowmenu, lowsong, lowboss ... and sewerlow */
	return strncasecmp(name, "low", 3) == 0 || strcasecmp(name, "sewerlow.wav") == 0;
}

int dm_mix_volume(int step)
{
	return clamp(step, 0, DM_VOLUME_MAX) * 128 / DM_VOLUME_MAX;
}
