/* Fenix - Compilador/intérprete de videojuegos
 * Copyright (C) 1999 José Luis Cebrián Pagüe
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* HISTORY
 *
 *	REWRITEN ALL SOUND CODE USING SDL_MIXER
 *
 *
 *
 */


#pragma comment (lib, "SDL_mixer")


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fxi.h"
#include <SDL.h>


int sound_active=0;     //variable para comprobar si el sonido está activado.

static SDL_CD* cdrom ;


typedef struct _celda {
    int *sonido;
	int tipo;
} celda;

celda *mem_sonido=0;

int lista_canales[8]={-1,-1,-1,-1,-1,-1,-1,-1};

int sound_used=0;
int sound_free=512;



/*
 *  FUNCTION : sound_init
 *
 *  Set the SDL_Mixer library
 *
 *  PARAMS:
 *      no params
 *
 *  RETURN VALUE:
 *
 *		no return
 */


void sound_init () {

	int audio_rate;
	Uint16 audio_format;
	int audio_channels;
	int audio_buffers;
	int i;

	/* Initialize variables */
	audio_rate = MIX_DEFAULT_FREQUENCY;
	audio_format = MIX_DEFAULT_FORMAT;
	audio_channels = 2;
	audio_buffers = 4096;

	/* Open the audio device */
	if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) < 0) {
		gr_con_printf ("[SOUND] No se pudo inicializar el audio: %s\n",SDL_GetError()) ;
		sound_active=0;
		return;
	} else {
		Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);
		gr_con_printf ("Opened audio at %d Hz %d bit %s, %d bytes audio buffer\n", audio_rate,
			(audio_format&0xFF),
			(audio_channels > 1) ? "stereo" : "mono",
			audio_buffers );
		ini_musiccd();
		sound_active=1;

		mem_sonido=(celda *) malloc (512*sizeof(celda));

		for (i=0;i<512;i++) {
			mem_sonido[i].tipo=-1;
			mem_sonido[i].sonido=NULL;
		}
		sound_used=0;
		sound_free=512;

		return;
	}


}



/*
 *  FUNCTION : sound_close
 *
 *  Close all the audio set
 *
 *  PARAMS:
 *      no params
 *
 *  RETURN VALUE:
 *
 *		no return
 */


void sound_close() {

    if (sound_active==0) return;

	SDL_CDClose(cdrom);

	//falta por comprobar que todo esté descargado

	Mix_CloseAudio();

	free(mem_sonido);

	sound_active=0;
}


/* ------------ */
/* Sonido MOD   */
/* ------------ */



/*
 *  FUNCTION : load_mod
 *
 *  Load a MOD from a file
 *
 *  PARAMS:
 *      file name
 *
 *  RETURN VALUE:
 *
 *		mod id
 *
 */


int load_mod (const char * filename)
{
	int i;

	static Mix_Music *music = NULL;

	if (sound_active==0) return(-1);
	if (sound_free==0) return (-1);


	music = Mix_LoadMUS(filename);

	if ( music == NULL ) {
			gr_con_printf("Couldn't load %s: %s\n",filename, SDL_GetError());
			return(-1);
	}

	if (sound_used<512) {

		mem_sonido[sound_used].sonido=music;
		mem_sonido[sound_used].tipo=1;
		sound_used++;
		sound_free--;
		return (sound_used-1);
	} else {

		// se podría optimizar con una cola de posiciones libres

		i=0;
		while (mem_sonido[i].sonido!=NULL){
			i++;
		}
		mem_sonido[i].sonido=music;
		mem_sonido[i].tipo=1;
		sound_free--;
		return (i);

	}


}


/*
 *  FUNCTION : play_mod
 *
 *  Play a MOD
 *
 *  PARAMS:
 *      mod id
 *      number of loops (-1 infinite loops)
 *
 *  RETURN VALUE:
 *
 *	-1 if there is any error
 *  0 if all goes ok
 */



int play_mod (int id, int loops)
{

	if (sound_active==0) return (-1);

	if ((mem_sonido[id].sonido!=NULL) & (mem_sonido[id].tipo==1)){
		Mix_PlayMusic(mem_sonido[id].sonido,loops);
		return(0);
	} else {
		return(-1);
	}

}

/*
 *  FUNCTION : unload_mod
 *
 *  Play a MOD
 *
 *  PARAMS:
 *
 *  mod id
 *
 *  RETURN VALUE:
 *
 *	-1 if there is any error
 *  0 if all goes ok
 */


int unload_mod (int id)
{

	if (sound_active==0) return (-1);

	if ((mem_sonido[id].sonido!=NULL) & (mem_sonido[id].tipo==1)){
		Mix_FreeMusic(mem_sonido[id].sonido);
		sound_free++;
		mem_sonido[id].sonido=NULL;
		mem_sonido[id].tipo=-1;
		return (0);
	} else return (-1);


}



/*
 *  FUNCTION : stop_mod
 *
 *  Stop the play of a mod
 *
 *  PARAMS:
 *
 *  no params
 *
 *  RETURN VALUE:
 *
 *	-1 if there is any error
 *  0 if all goes ok
 */

int stop_mod (void)
{
	    if (sound_active==0) return (-1);
		Mix_HaltMusic();
		return 0;
}



/*
 *  FUNCTION : pause_mod
 *
 *  Pause the mod in curse, you can resume it after
 *
 *  PARAMS:
 *
 *  no params
 *
 *  RETURN VALUE:
 *
 *	-1 if there is any error
 *  0 if all goes ok
 */


int pause_mod (void)
{
		if (sound_active==0) return (-1);
		Mix_PauseMusic();
		return 1;


}


/*
 *  FUNCTION : resume_mod
 *
 *  Resume the mod, paused before
 *
 *  PARAMS:
 *
 *  no params
 *
 *  RETURN VALUE:
 *
 *	-1 if there is any error
 *  0 if all goes ok
 */


int resume_mod (void)
{
	    if (sound_active==0) return (-1);
		Mix_ResumeMusic();
		return 1;

}


/*
 *  FUNCTION : is_playing_mod
 *
 *  Check if there is any mod playing
 *
 *  PARAMS:
 *
 *  no params
 *
 *  RETURN VALUE:
 *
 *	-1 if there is any error
 *  TRUE OR FALSE if there is no error
 */


int is_playing_mod(void) {
	if (sound_active==0) return (-1);
    return Mix_PlayingMusic();
}


/*
 *  FUNCTION : set_mod_volume
 *
 *  Set the volume for mod playing (0-128)
 *
 *  PARAMS:
 *
 *  int volume
 *
 *  RETURN VALUE:
 *
 *	-1 if there is any error
 *  0 if there is no error
 */


int set_mod_volume (int volume)
{
	if (sound_active==0) return (-1);

	if (volume<0) volume=0;
	if (volume>128) volume=128;

	Mix_VolumeMusic(volume);
	return 0;
}



/* ------------ */
/* Sonido WAV   */
/* ------------ */


/*
 *  FUNCTION : load_mod
 *
 *  Load a MOD from a file
 *
 *  PARAMS:
 *      file name
 *
 *  RETURN VALUE:
 *
 *		mod id
 *
 */


int load_wav (const char * filename)
{
	int i;

	static Mix_Chunk *music = NULL;

	if (sound_active==0) return(-1);
	if (sound_free==0) return (-1);

	music = Mix_LoadWAV(filename);

	if ( music == NULL ) {
			gr_con_printf("Couldn't load %s: %s\n",filename, SDL_GetError());
			return(-1);
	}

	if (sound_used<512) {

		mem_sonido[sound_used].sonido=music;
		mem_sonido[sound_used].tipo=2;
		sound_used++;
		sound_free--;
		return (sound_used-1);
	} else {

		// se podría optimizar con una cola de posiciones libres

		i=0;
		while (mem_sonido[i].sonido!=NULL){
			i++;
		}
		mem_sonido[i].sonido=music;
		mem_sonido[i].tipo=2;
		sound_free--;
		return (i);

	}


}



/*
 *  FUNCTION : play_wav
 *
 *  Play a WAV
 *
 *  PARAMS:
 *      wav id;
 *      number of loops (-1 infinite loops)
 *
 *  RETURN VALUE:
 *
 *	-1 if there is any error
 *  0 if all goes ok
 */


int play_wav (int id, int loops)
{

	int canal;
	int i;

	if (sound_active==0) return (-1);

	if ((mem_sonido[id].sonido!=NULL) & (mem_sonido[id].tipo==2)){

		for (i=0;i<8;i++) {
			if (lista_canales[i]==id) {
				if (Mix_Playing(i)) return(-1);
			}
		}

		canal=Mix_PlayChannel(-1,mem_sonido[id].sonido,loops);

		if (canal==-1) {return(-1);}
			lista_canales[canal]=id;

		return(0);
	}
	else {
		return (-1);
	}

}


/*
 *  FUNCTION : unload_wav
 *
 *  Frees the resources from a wav, unloading it
 *
 *  PARAMS:
 *
 *  mod id
 *
 *  RETURN VALUE:
 *
 *	-1 if there is any error
 *  0 if all goes ok
 */


int unload_wav (int id)
{
	int i;
	if (sound_active==0) return (-1);

	if ((mem_sonido[id].sonido!=NULL) & (mem_sonido[id].tipo==2)){
		Mix_FreeChunk(mem_sonido[id].sonido);
		sound_free++;
		mem_sonido[id].sonido=NULL;
		mem_sonido[id].tipo=-1;
		for (i=0;i<8;i++) {
			if (lista_canales[i]==id) {
				lista_canales[i]=-1;
			}
		}
		return (0);
	} else return (-1);


}


static int fxi_unload_wav (INSTANCE * my, int * params)
{

	static Mix_Chunk *music = NULL;
	music =params[0];

	return 1;

}


/*
 *  FUNCTION : stop_wav
 *
 *  Stop a wav playing
 *
 *  PARAMS:
 *
 *  wav id
 *
 *  RETURN VALUE:
 *
 *	-1 if there is any error
 *  0 if all goes ok
 */

int stop_wav (int id)
{
		int i;

	    if (sound_active==0) return (-1);

		for (i=0;i<8;i++) {
			if (lista_canales[i]==id) {
				if (Mix_Playing(i)) {
					Mix_HaltChannel(i);
					lista_canales[i]=-1;
					return (0);
				} else {
					lista_canales[i]=-1;
				}
			}
		}

		return (-1);
}



/*
 *  FUNCTION : pause_wav
 *
 *  Pause a wav playing, you can resume it after
 *
 *  PARAMS:
 *
 *  wav id
 *
 *  RETURN VALUE:
 *
 *	-1 if there is any error
 *  0 if all goes ok
 */


int pause_wav (int id)
{
		int i;

	    if (sound_active==0) return (-1);

		for (i=0;i<8;i++) {
			if (lista_canales[i]==id) {
				if (Mix_Playing(i)) {
					Mix_Pause(i);
					return (0);
				} else {
					lista_canales[i]=-1;
				}
			}
		}

		return (-1);
}




/*
 *  FUNCTION : resume_wav
 *
 *  Resume a wav playing, paused before
 *
 *  PARAMS:
 *
 *  wav id
 *
 *  RETURN VALUE:
 *
 *	-1 if there is any error
 *  0 if all goes ok
 */


int resume_wav (int id)
{
		int i;

	    if (sound_active==0) return (-1);

		for (i=0;i<8;i++) {
			if (lista_canales[i]==id) {
				if (Mix_Playing(i)) {
					Mix_Resume(i);
					return (0);
				} else {
					lista_canales[i]=-1;
				}
			}
		}

		return (-1);
}


/*
 *  FUNCTION : is_playing_wav
 *
 *  Check a wav playing
 *
 *  PARAMS:
 *
 *  wav id
 *
 *  RETURN VALUE:
 *
 *	-1 if there is any error
 *  TRUE OR FALSE if there is no error
 */


int is_playing_wav(int id) {
		int i;

	    if (sound_active==0) return (-1);

		for (i=0;i<8;i++) {
			if (lista_canales[i]==id) {
				if (Mix_Playing(i)) {
					return (1);
				} else {
					lista_canales[i]=-1;
				}
			}
		}

		return (0);
}



/*
 *  FUNCTION : set_wav_volume
 *
 *  Set the volume for wav playing (0-128)
 *
 *  PARAMS:
 *
 *  wav id
 *  int volume
 *
 *  RETURN VALUE:
 *
 *	-1 if there is any error
 *  0 if there is no error
 */


int	 set_wav_volume	(int id,int volume) {

	int i;

	if (sound_active==0) return (-1);
	if (volume<0) volume=0;
	if (volume>128) volume=128;

	    if (sound_active==0) return (-1);

		for (i=0;i<8;i++) {
			if (lista_canales[i]==id) {
				if (Mix_Playing(i)) {
					Mix_Volume(i,volume);
					return(0);
				} else {
					lista_canales[i]=-1;
				}
			}
		}
		return (-1);
}




/* ------------ */
/* Sonido de CD */
/* ------------ */


void ini_musiccd()
{
	if (sound_active==0) return;

		if (cd_playing()) cd_stop();
		cdrom = SDL_CDOpen (0) ;
		if (!cdrom) {
			gr_con_printf ("[SOUND] No se pudo inicializar el CDROM: %s\n",SDL_GetError()) ;
		}
}


void cd_play (int track, int continuous)
{

	if (cdrom && CD_INDRIVE(SDL_CDStatus(cdrom)))
		SDL_CDPlayTracks (cdrom, track, 0, continuous ? 0:1, 0) ;
}

void cd_stop ()
{


	if (cdrom) SDL_CDStop (cdrom) ;
}

int cd_playing ()
{


	if (!cdrom) return 0 ;
	switch (SDL_CDStatus (cdrom))
	{
		case CD_PLAYING:
		case CD_PAUSED:
			return 1;
		default:
			return 0;
	}
}

