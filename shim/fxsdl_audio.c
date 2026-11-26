/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Audio for fxi on the SF3500. Compiled against the real SDL2 / SDL2_mixer headers.
 *
 * SDL_mixer does the mixing (this game's sound effects and music are all WAV files). SDL's own audio
 * device is the "dummy" one, which only provides the real-time pacing; the already-mixed buffer is
 * tapped with Mix_SetPostMix() and written to the device's audio ring (hwaudio). The mixer runs at the
 * hardware's native 48000 Hz, so sounds are converted once at load time instead of continuously
 * while playing.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include "../sf3500/hwaudio.h"
#include "dm_settings.h"

/* ---- error text shared with the video side ------------------------------------------------- */

void fxsdl_set_error(const char *msg) { SDL_SetError("%s", msg); }
const char *fxsdl_GetError(void) { return SDL_GetError(); }

/* ---- mixer ------------------------------------------------------------------------------- */

static int audio_up = 0;
static int mixer_open = 0;

int fxsdl_audio_init(void)
{
	if (audio_up) return 0;
	/* Without this SDL looks for a real audio device and fails; the device is driven by hwaudio. */
	SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) return -1;
	audio_up = 1;
	return 0;
}

static volatile int muted = 0;      /* while Stage Select hurries past the intro */

void fxsdl_audio_mute(int on) { muted = on; }

static void postmix(void *udata, Uint8 *stream, int len)
{
	(void)udata;
	if (muted) memset(stream, 0, (size_t)len);
	/* Opened as 48000 Hz, signed 16 bit, stereo: 4 bytes per frame. */
	hwaudio_write((const short *)stream, len / 4);
}

int fxmix_OpenAudio(int frequency, Uint16 format, int channels, int chunksize)
{
	int rate = HWAUDIO_RATE, ch;
	Uint16 fmt;
	(void)frequency; (void)format; (void)channels; (void)chunksize;
	/* 2048 frames at 48 kHz is about 43 ms per mixer call, a size that has worked well on this device. */
	if (Mix_OpenAudio(rate, AUDIO_S16SYS, 2, 2048) < 0) return -1;
	hwaudio_init();
	Mix_SetPostMix(postmix, NULL);
	/* Not more than 8: fxi's sound.c records which sound is on which channel in lista_canales[8],
	 * indexed by whatever channel SDL_mixer picked. */
	Mix_AllocateChannels(8);
	Mix_QuerySpec(&rate, &fmt, &ch);
	mixer_open = 1;
	fprintf(stderr, "fxsdl: audio spec rate=%d channels=%d\n", rate, ch);
	return 0;
}

void fxmix_CloseAudio(void)
{
	if (!mixer_open) return;
	mixer_open = 0;
	Mix_CloseAudio();
}

Mix_Chunk *fxmix_LoadWAV(const char *file)
{
	static struct dm_settings settings;
	static int loaded = 0;
	Mix_Chunk *chunk = Mix_LoadWAV(file);

	if (!loaded) { dm_settings_load(&settings); loaded = 1; }
	/* Each sound gets the volume for its kind; the game itself sets none. */
	if (chunk)
		Mix_VolumeChunk(chunk, dm_mix_volume(dm_is_music(file) ? settings.music : settings.sound));
	return chunk;
}

/* Holds every sound and the music where they are; the audio output keeps running, silent. */
void fxsdl_audio_pause(int on)
{
	if (!mixer_open) return;
	if (on) { Mix_Pause(-1); Mix_PauseMusic(); }
	else    { Mix_Resume(-1); Mix_ResumeMusic(); }
}

void fxsdl_audio_quit(void)
{
	if (mixer_open) { mixer_open = 0; Mix_HaltMusic(); Mix_CloseAudio(); }
}
