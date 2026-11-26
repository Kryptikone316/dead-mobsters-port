/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * The settings the launcher writes and the game reads, and the small files the two use to talk to
 * each other. All of them live in the port's folder. Plain C, no SDL.
 */
#ifndef DM_SETTINGS_H
#define DM_SETTINGS_H

#define DM_SETTINGS_FILE   "settings.cfg"
#define DM_COMPLETED_FILE  "game_completed"   /* made when a normal game reaches the ending; unlocks Stage Select */
#define DM_REQUEST_FILE    "launch.txt"       /* launcher -> run.sh: "play", "stage <level>" or "exit" */

#define DM_VOLUME_MAX      10
#define DM_PICTURE_RANGE    5                 /* brightness and contrast: -5 .. +5 */
#define DM_GAMMA_MIN        6                 /* tenths: 0.6 .. 1.6 */
#define DM_GAMMA_MAX       16
#define DM_GAMMA_DEFAULT   10

struct dm_settings {
	int mobster;            /* 1 or 2: the game's two characters (its old START 1 and START 2) */
	int music, sound;       /* 0 .. 10; 10 is the game as the author made it */
	int brightness, contrast;
	int gamma;              /* tenths */
};

/* The playable stages, by the game's own level number. Levels not listed are story cards. */
#define DM_AREAS            4
#define DM_STAGES_PER_AREA  3                 /* two stages and the boss */
extern const int dm_stage_level[DM_AREAS][DM_STAGES_PER_AREA];
int  dm_is_stage(int level);
#define DM_ENDING_LEVEL    22                 /* the story card shown after the last boss */

void dm_settings_default(struct dm_settings *s);
void dm_settings_load(struct dm_settings *s);           /* missing file or lines: defaults */
int  dm_settings_save(const struct dm_settings *s);     /* 0 on success */

/* 8-bit colour value -> the value to show, for the picture settings. Identity at the defaults. */
void dm_colour_lut(const struct dm_settings *s, unsigned char lut[256]);

/* The game plays its music as WAV files too; this tells them from the effects by name. */
int  dm_is_music(const char *path);
int  dm_mix_volume(int step);                           /* 0 .. 10 -> SDL_mixer's 0 .. 128 */

/* Writes a small file and makes sure it reaches the card (the handheld is often just switched off). */
int  dm_write_file(const char *name, const char *text);

#endif
