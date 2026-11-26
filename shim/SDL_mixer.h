/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * The SDL_mixer 1.2 calls fxi's sound.c makes, declared without pulling in any SDL2 header.
 * They are the real SDL2_mixer functions (same names, same signatures), except the three that
 * the SF3500 build wraps (see shim/fxsdl_audio.c): opening the audio, closing it, and loading
 * a WAV from a file name.
 */
#ifndef FXSDL_MIXER_H
#define FXSDL_MIXER_H

#include "SDL_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MIX_DEFAULT_FREQUENCY 22050
#define MIX_DEFAULT_FORMAT    0x8010 /* AUDIO_S16LSB */
#define MIX_MAX_VOLUME        128

typedef struct Mix_Chunk Mix_Chunk;
typedef struct _Mix_Music Mix_Music;

#define Mix_OpenAudio  fxmix_OpenAudio
#define Mix_CloseAudio fxmix_CloseAudio
#define Mix_LoadWAV    fxmix_LoadWAV

int         Mix_OpenAudio(int frequency, Uint16 format, int channels, int chunksize);
void        Mix_CloseAudio(void);
Mix_Chunk * Mix_LoadWAV(const char *file);

int         Mix_QuerySpec(int *frequency, Uint16 *format, int *channels);
Mix_Music * Mix_LoadMUS(const char *file);
void        Mix_FreeMusic(Mix_Music *music);
void        Mix_FreeChunk(Mix_Chunk *chunk);
int         Mix_PlayMusic(Mix_Music *music, int loops);
int         Mix_PlayingMusic(void);
int         Mix_HaltMusic(void);
void        Mix_PauseMusic(void);
void        Mix_ResumeMusic(void);
int         Mix_VolumeMusic(int volume);
int         Mix_PlayChannelTimed(int channel, Mix_Chunk *chunk, int loops, int ticks);
#define     Mix_PlayChannel(channel, chunk, loops) Mix_PlayChannelTimed(channel, chunk, loops, -1)
int         Mix_Playing(int channel);
int         Mix_HaltChannel(int channel);
void        Mix_Pause(int channel);
void        Mix_Resume(int channel);
int         Mix_Volume(int channel, int volume);

#ifdef __cplusplus
}
#endif

#endif
