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

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <SDL.h>

#include "fxi.h"

extern SDL_Surface * screen ;

extern GRAPH * gr_read_png (const char * filename) ;
extern GRAPH * gr_read_pcx (const char * filename) ;

static int free_map_code = 1000 ;

/* ---------------------------------------------------------------------- */
/* Carga de gráficos y librerías de gráficos                              */
/* ---------------------------------------------------------------------- */

#define MAXLIBS 128
static GRLIB * libs[MAXLIBS] ;
GRLIB * syslib = 0 ;
static int lib_nextid = 0 ;
int lib_count = 0 ;

static GRLIB * grlib_create ()
{
	GRLIB * lib ;

	lib = (GRLIB *) malloc (sizeof(GRLIB)) ;
	if (!lib) return 0 ;
	lib->maps = (GRAPH **) malloc (16 * sizeof(GRAPH *)) ;
	if (!lib->maps) return 0 ;
	
	lib->map_count = 0 ;
	lib->map_reserved = 16 ;
	return lib ;
}

int grlib_new ()
{
	GRLIB * lib  = grlib_create();
	int i ;

	if (lib_nextid == MAXLIBS && lib_count == lib_nextid) return -1 ;

	lib_count++ ;

	if (lib_nextid == MAXLIBS)
	{
		for (i = 0 ; i < lib_nextid ; i++)
			if (!libs[i]) break ;
		libs[i] = lib ;
		return i ;
	}

	libs[lib_nextid] = lib ;
	return lib_nextid++ ;
}

GRAPH * bitmap_new (int code, int w, int h, int depth)
{
	GRAPH  * gr ;

	if (depth != 8 && depth != 16)
		gr_error ("Profundidad de color no soportada\n(new bitmap)") ;

	gr = (GRAPH *) malloc (sizeof(GRAPH)) ;
	if (!gr) gr_error ("bitmap_new: sin memoria") ; ;
	gr->width   = w ;
	gr->pitch   = w ;
	gr->height  = h ;
	gr->depth   = depth ;
	gr->name[0] = 0 ;
	gr->code    = code ;
	gr->data    = (char *) malloc (w * h * (depth == 16 ? 2:1)) ;
	if (!gr->data) gr_error ("bitmap_new: sin memoria") ; ;
	gr->offset = 0 ;
	gr->flags = 0 ;
	gr->modified = 0 ;
	gr->info_flags = 0 ;
	gr->cpoints = NULL ;
	gr->animation = NULL ;
        gr->blend_table = NULL ;
	return gr ;
}

GRAPH * bitmap_clone (GRAPH * this)
{
        GRAPH * gr ;

        gr = bitmap_new_syslib(this->pitch, this->height, this->depth) ;
        memcpy (gr->data, this->data, this->pitch * this->height *
                       (this->depth == 16 ? 2:1)) ;
        if (this->animation)
        {
                gr->animation = malloc(sizeof(ANIMATION)) ;
                memcpy (gr->animation, this->animation, sizeof(ANIMATION)) ;
                gr->animation->order = malloc(2 * (this->animation->length+1)) ;
                memcpy (gr->animation->order, this->animation->order,
                                2 * (this->animation->length)) ;
        }
        if (this->cpoints)
        {
                gr->cpoints = malloc(4 * (this->flags & F_NCPOINTS)) ;
                memcpy (gr->cpoints, this->cpoints, 4 * (this->flags & F_NCPOINTS)) ;
        }
        gr->width = this->width ;
        gr->flags = this->flags ;
        gr->offset = this->offset ;
        gr->info_flags = this->info_flags ;
        gr->modified = this->modified ;
        memcpy (gr->name, this->name, sizeof(this->name)) ;
        return gr ;
}


void bitmap_add_cpoint (GRAPH * map, int x, int y)
{
	int n = (map->flags & F_NCPOINTS) ;
	map->flags = ((map->flags & ~F_NCPOINTS) | (n+1)) ;
	map->cpoints = (CPOINT *) realloc (map->cpoints,
			(n+1) * sizeof(CPOINT)) ;
	map->cpoints[n].x = x ;
	map->cpoints[n].y = y ;
}

void bitmap_destroy (GRAPH * map)
{
	if (map->cpoints) free(map->cpoints) ;
        if (map->animation) {
                free (map->animation->order) ;
                free (map->animation) ;
        }
	free (map->data) ;
	free (map) ;
}

void grlib_destroy (int libid)
{
	int i ;
	GRLIB * lib = grlib_get (libid) ;
	assert (lib != 0);

	libs[libid] = 0 ;
	if (lib_nextid == libid+1) lib_nextid-- ;
	lib_count-- ;
	if (!lib_count) lib_nextid = 0 ;

	for (i = 0 ; i < lib->map_count ; i++)
		bitmap_destroy (lib->maps[i]) ;

	free (lib->maps) ;
	free (lib) ;
}

int grlib_unload_map (GRLIB * lib, int mapcode)
{
	int i ;
	assert (lib != 0) ;

	for (i = 0 ; i < lib->map_count ; i++)
	{
		if (lib->maps[i]->code == mapcode)
		{
			bitmap_destroy (lib->maps[i]) ;
			for (; i < lib->map_count-1 ; i++)
				lib->maps[i] = lib->maps[i+1] ;
                        lib->map_count-- ;
			return 1 ;
		}
	}
	return 0 ;
}

int grlib_add_map (GRLIB * lib, GRAPH * map)
{
	if (!lib) return -1 ;	

	if (lib->map_reserved == lib->map_count)
	{
		lib->map_reserved += 16 ;
		lib->maps = (GRAPH **) realloc (lib->maps, sizeof(GRAPH*) * lib->map_reserved) ;
		if (!lib->maps) gr_error ("grlib_add_map: sin memoria\n") ;
	}
	lib->maps[lib->map_count] = map ;
	return lib->map_count++ ;
}

GRLIB * grlib_get (int libid)
{
	if (libid < 0 || libid >= lib_nextid)
		return 0 ;
	return libs[libid] ;
}

GRAPH * bitmap_get (int libid, int mapcode)
{
	GRLIB * lib ;
	int i ;

	if (libid == 0 || mapcode > 999)
	{
		if (mapcode == 0)
		{
			background_is_black = 0 ;

			if (enable_16bits && background_8bits_used)
				return background_8bits ;
			return background ;
		}

		if (mapcode == -1)
		{
			if (!scr_initialized) gr_init(320,200) ;
			if (!scrbitmap) gr_lock_screen() ;
			return scrbitmap ;
		}

		if (!syslib) syslib = grlib_create() ;
		assert (syslib) ;
		for (i = 0 ; i < syslib->map_count ; i++)
			if (syslib->maps[i]->code == mapcode)
				return syslib->maps[i] ;
	}

        lib = grlib_get(libid) ;
        if (!lib) return 0 ;
        for (i = 0 ; i < lib->map_count ; i++)
                if (lib->maps[i]->code == mapcode)
                        return lib->maps[i] ;
	return 0 ;
}

/* Rutinillas de lectura desde ficheros, para uso interno */

static void gr_normalize16 (Uint16 * ptr, int len)
{
	Uint8 r, g, b ;
	int n ;

	static Uint16 normalized[65536] ;
	static int normalized_ok = 0 ;

	if (!normalized_ok)
	{
		if (!scr_initialized) gr_init(320,200) ;
		for (n = 0 ; n < 65536 ; n++)
		{
			r = ((n >> 8) & 0xF8) >> screen->format->Rloss ;
			g = ((n >> 3) & 0xFC) >> screen->format->Gloss ;
			b = ((n << 3) & 0xF8) >> screen->format->Bloss ;

			normalized[n] = 
			       (r << screen->format->Rshift) |
			       (g << screen->format->Gshift) |
			       (b << screen->format->Bshift) ;
		}
	}

	while (len--) *ptr++ = normalized[*ptr] ;
}

static int gr_read_lib (file * fp)
{
	char header[8] ;
	short int px, py ;
	int bpp, is_16bits, libid, c, len ;
	GRLIB * lib ;
	GRAPH * gr ;

	struct 
	{
		int	code ;
		int	regsize ;
		char	name[32] ;
		char	fpname[12] ;
		int	width ;
		int	height ;
		int	flags ;

	} chunk ;

	libid = grlib_new() ;
	if (libid < 0) return 0 ;
	lib = libs[libid] ;
	if (lib == 0) return 0;

	file_read (fp, header, 8) ;
	if (strcmp (header, "f16\x1A\x0D\x0A") == 0) 
		is_16bits = 1 ;
	else if (strcmp (header, "fpg\x1A\x0D\x0A") == 0) 
		is_16bits = 0 ;
	else
		return 0 ;

	bpp = is_16bits? 2:1 ;

	if (!is_16bits)
	{
		if (palette_loaded)
			file_seek (fp, 576 + 768, SEEK_CUR) ;
		else if (!gr_read_pal (fp)) 
			return 0 ;
	}

	while (!file_eof(fp))
	{
		if (!file_read (fp, &chunk, 64)) break ;

		/* Cabecera del gráfico */

		gr = bitmap_new (chunk.code, chunk.width, chunk.height,
				is_16bits ? 16:8) ;
		if (!gr) return 0 ;
		memcpy (gr->name, chunk.name, 32) ;
		gr->name[31] = 0 ;
		gr->flags = chunk.flags ;
		gr->modified = 1 ;

		/* Puntos de control */

		if (gr->flags & F_NCPOINTS)
		{
			gr->cpoints = (CPOINT *) malloc 
				((gr->flags & F_NCPOINTS) * sizeof(CPOINT)) ;
			if (!gr->cpoints) { free(gr) ; return 0 ; }
			for (c = 0 ; c < (gr->flags & F_NCPOINTS) ; c++)
			{
				file_read (fp, &px, 2) ;
				file_read (fp, &py, 2) ;
				gr->cpoints[c].x = px ;
				gr->cpoints[c].y = py ;
			}
		}
		else	gr->cpoints = 0 ;

		/* Animación */

		len = gr->pitch * gr->height * bpp ;

		if (gr->flags & F_ANIMATION)
		{
			if (!gr->animation) gr->animation = malloc(sizeof(ANIMATION)) ;
			file_read (fp, &gr->animation->frames, 2) ;
			file_read (fp, &gr->animation->length, 2) ;
			file_read (fp, &gr->animation->speed, 2) ;
			gr->animation->order = malloc(gr->animation->length * 2) ;
			file_read (fp, gr->animation->order, gr->animation->length * 2) ;
			gr->pitch *= gr->animation->frames ;
			len *= gr->animation->frames;
			gr->data = realloc(gr->data, len) ;
		}

		/* Datos del gráfico */

		if (!file_read (fp, gr->data, len))
		{
			free (gr) ;
			break ;
		}
		if (is_16bits) gr_normalize16 (gr->data, gr->pitch * gr->height);

		grlib_add_map (lib, gr) ;

	}
	return libid ;
}

static GRAPH * gr_read_map (file * fp)
{
	char header[8] ;
	unsigned short int w, h, c ;
	int len ;
	int depth, code ;
	GRAPH * gr ;

	/* Carga los datos de cabecera */

	file_read (fp, header, 8) ;
	if (strcmp (header, "m16\x1A\x0D\x0A") == 0) 
		depth = 16 ;
	else if (strcmp (header, "map\x1A\x0D\x0A") == 0) 
		depth = 8 ;
	else
		return 0 ;

	file_read (fp, &w, 2) ;
	file_read (fp, &h, 2) ;
	file_read (fp, &code, 4) ;

	gr = bitmap_new (code, w, h, depth) ;
	if (!gr) return 0 ;
	file_read (fp, gr->name, 32) ;
	gr->name[31] = 0 ;

	/* Datos de paleta */

	if (gr->depth == 8)
	{
		if (palette_loaded)
			file_seek (fp, 576 + 768, SEEK_CUR) ;
		else
			if (!gr_read_pal (fp)) return 0 ;
	}

	/* Puntos de control */

	file_read (fp, &c, 2) ;
	gr->flags = c ;

	if (gr->flags & F_NCPOINTS)
	{
		gr->cpoints = (CPOINT *) malloc ((c & F_NCPOINTS) * sizeof(CPOINT)) ;
		if (!gr->cpoints) { free(gr) ; return 0 ; }
		for (c = 0 ; c < (gr->flags & F_NCPOINTS) ; c++)
		{
			file_read (fp, &w, 2) ;
			file_read (fp, &h, 2) ;
			gr->cpoints[c].x = w ;
			gr->cpoints[c].y = h ;
		}
	}
	else	gr->cpoints = 0 ;

	if (gr->flags & F_ANIMATION)
	{
                if (!gr->animation) gr->animation = malloc(sizeof(ANIMATION)) ;
		if (!gr->animation) { free(gr) ; return 0 ; }
		file_read (fp, &c, 2) ; gr->animation->frames = c ;
		file_read (fp, &c, 2) ; gr->animation->length = c ;
		file_read (fp, &c, 2) ; gr->animation->speed = c ;
		gr->animation->remaining = gr->animation->speed ;
		gr->animation->order = (Sint16 *) malloc(gr->animation->length * 2) ;
		if (!gr->animation->order) { free(gr) ; return 0 ; }
		file_read (fp, gr->animation->order, gr->animation->length * 2) ;
		gr->pitch *= gr->animation->frames ;
	}

	/* Datos del gráfico */

	len = gr->pitch * gr->height ;
	if (gr->depth == 16) len *= 2 ;
	gr->data = realloc(gr->data, len) ;
	if (!file_read (fp, gr->data, len))
	{
		free (gr->data) ;
		free (gr) ;
		return 0 ;
	}
	if (gr->depth == 16) gr_normalize16 (gr->data, len / 2);

	gr->modified = 1 ;
	return gr ;
}

/* Funciones de carga de nivel superior */

int gr_load_png (const char * mapname)
{
	GRAPH * gr ;

	gr = gr_read_png (mapname) ;
	if (!gr) return -1 ;
	gr->code = free_map_code++ ;
	if (!syslib) syslib = grlib_create() ;
	assert (syslib) ;
	grlib_add_map (syslib, gr) ;
	return gr->code ;
}

int gr_load_pcx (const char * mapname)
{
	GRAPH * gr ;

	gr = gr_read_pcx (mapname) ;
	if (!gr) return -1 ;
	gr->code = free_map_code++ ;
	if (!syslib) syslib = grlib_create() ;
	assert (syslib) ;
	grlib_add_map (syslib, gr) ;
	return gr->code ;
}

int gr_load_map (const char * mapname)
{
	GRAPH * gr ;
	file * fp = file_open (mapname, "rb") ;

	if (!fp) 
	{
		gr_error ("Mapa %s no encontrado\n", mapname) ;
		return -1 ;
	}
	gr = gr_read_map (fp) ;
	file_close (fp) ;
	if (!gr) return -1 ;
	gr->code = free_map_code++ ;
	if (!syslib) syslib = grlib_create() ;
	assert (syslib) ;
	grlib_add_map (syslib, gr) ;
	return gr->code ;
}

int gr_load_fpg (const char * libname)
{
	int libid ;
	file * fp = file_open (libname, "rb") ;

	if (!fp) 
	{
		gr_error ("Libreria %s no encontrada\n", libname) ;
		return -1 ;
	}
	libid = gr_read_lib (fp) ;
	file_close (fp) ;
	return libid ;
}

GRAPH * bitmap_new_syslib (int w, int h, int depth)
{
	GRAPH * gr ;
	
	gr = bitmap_new (0, w, h, depth) ;
	gr->code = free_map_code++ ;
	if (!syslib) syslib = grlib_create() ;
	assert (syslib) ;
	grlib_add_map (syslib, gr) ;
	return gr ;
}

/* Análisis */

void bitmap_analize (GRAPH * bitmap)
{
	Uint32 pixels = bitmap->pitch * bitmap->height ;

	bitmap->modified = 0 ;
	bitmap->info_flags = 0 ;

	if (bitmap->depth == 8)
	{
		Uint8 * ptr = memchr (bitmap->data, 0, pixels) ;
		if (!ptr) bitmap->info_flags |= GI_NOCOLORKEY ;
	}
	else
	{
		Uint16 * ptr = (Uint16 *)bitmap->data ;
		while (pixels--) if (!*ptr++) break ;
		if (!pixels) bitmap->info_flags |= GI_NOCOLORKEY ;
	}
}

/* Animación */

void bitmap_animate_to (GRAPH * bitmap, int pos, int speed)
{
	if (pos > bitmap->animation->length) 
		pos = bitmap->animation->length - 1 ;
	if (pos < 0)
		pos = 0 ;

	if (bitmap->depth == 16)
	{
		bitmap->data = (Uint16 *)bitmap->data - bitmap->offset ;
		bitmap->offset = bitmap->width * (bitmap->animation->order[pos]-1) ;
		bitmap->data = (Uint16 *)bitmap->data + bitmap->offset ;
	}
	else
	{
		bitmap->data = (Uint8 *)bitmap->data - bitmap->offset ;
		bitmap->offset = bitmap->width * (bitmap->animation->order[pos]-1) ;
		bitmap->data = (Uint8 *)bitmap->data + bitmap->offset ;
	}

	bitmap->animation->pos = pos ;
	bitmap->animation->speed = speed ;
	bitmap->animation->remaining = speed ;
}

extern int frame_count ;

void bitmap_animate (GRAPH * bitmap)
{
	if (!(bitmap->flags & F_ANIMATION))
		return ;
        if (!bitmap->animation)
                return ;
	if (bitmap->animation->last_frame == frame_count)
		return ;
	if (!bitmap->animation->speed)
		return ;

	bitmap->animation->last_frame = frame_count ;
	bitmap->animation->remaining -= last_frame_ms ;

	while (bitmap->animation->remaining < 0)
	{
		if (bitmap->animation->speed < 1) 
			bitmap->animation->speed = last_frame_ms ;
		if (bitmap->animation->speed < 10) 
			bitmap->animation->speed = 10 ;

		bitmap->animation->remaining += bitmap->animation->speed ;

		if (bitmap->depth == 16)
		{
			bitmap->data = (Uint16 *)bitmap->data - bitmap->offset ;
			bitmap->offset = bitmap->width * 
                                (bitmap->animation->order[bitmap->animation->pos]-1) ;
			bitmap->data = (Uint16 *)bitmap->data + bitmap->offset ;
		}
		else
		{
			bitmap->data = (Uint8 *)bitmap->data - bitmap->offset ;
			bitmap->offset = bitmap->width * 
                                (bitmap->animation->order[bitmap->animation->pos]-1) ;
			bitmap->data = (Uint8 *)bitmap->data + bitmap->offset ;
		}

		bitmap->animation->pos++ ;
		if (bitmap->animation->pos == bitmap->animation->length)
			bitmap->animation->pos = 0 ;
	}
}
