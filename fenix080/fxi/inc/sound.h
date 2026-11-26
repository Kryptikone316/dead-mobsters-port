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

#ifndef __SOUND_H
#define __SOUND_H

#include "SDL_mixer.h"

void            sound_init      ();
void            sound_close     ();

extern int      load_mod        (const char * filename);
extern int      play_mod        (int id , int loops);
extern int      unload_mod      (int id);
extern int      stop_mod        ();
extern int      pause_mod       ();
extern int      resume_mod      ();
extern int      is_playing_mod  ();
extern int      set_mod_volume  (int volume);

extern int      load_wav        (const char * filename);
extern int      play_wav        (int id , int loops);
extern int      unload_wav      (int id);
extern int      stop_wav        (int id);
extern int      pause_wav       (int id);
extern int      resume_wav      (int id);
extern int      is_playing_wav  (int id);
extern int      set_wav_volume  (int id,int volume);

/* ------------ */
/* CD-ROM Audio */
/* ------------ */
extern void     ini_musiccd     ();
extern void     cd_play         (int track, int continuous);
extern void     cd_stop         ();
extern int      cd_playing      ();

#endif
