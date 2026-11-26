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

SDL_Color  palette[256] ;
SDL_Color vpalette[256] ;

Uint16     colorequiv[256] ;	/* Equivalencia paleta -> pantalla   */
Uint16     colorghost[65536] ;	/* Deja un color a 50% de intensidad */

int palette_loaded = 0 ;
int palette_changed = 0 ;

int fade_on = 0 ;
int fade_step = 128 ;

static int fade_inc = 0 ;
static SDL_Color fade_color ;

/* Lista ordenada de colores */

int color_list[256] ;

/* Tabla de transparencias */

Uint8 trans_table[256][256] ;
int trans_table_updated = 0 ;

static void activate_vpalette()
{
	int n ;

	if (!screen) return ;

	for (n = 0 ; n < 256 ; n++)
	{
		vpalette[n].r  = palette[n].r * fade_step / 128 ;
		vpalette[n].g  = palette[n].g * fade_step / 128 ;
		vpalette[n].b  = palette[n].b * fade_step / 128 ;
		vpalette[n].r += fade_color.r * (128-fade_step) / 128 ;
		vpalette[n].g += fade_color.g * (128-fade_step) / 128 ;
		vpalette[n].b += fade_color.b * (128-fade_step) / 128 ;
	}

	if (enable_16bits)
	{
		for (n = 0 ; n < 256 ; n++)
			colorequiv[n] = SDL_MapRGB (screen->format,
				vpalette[n].r, vpalette[n].g, vpalette[n].b) ;
	}
	else SDL_SetColors (screen, vpalette, 0, 256) ;
}

void gr_roll_palette (int color0, int num, int inc)
{
	SDL_Color backup[256] ;

	if (!scr_initialized) gr_init (320, 200) ;

	if (color0 < 0 || color0 > 255) return ;
	if (num+color0 > 255) num = 256-color0 ;
	while (inc >= num) inc -= num ;
	while (inc <= -num) inc += num ;
	if (!inc) return ;

        memcpy  (&backup[color0],  &palette[color0],     sizeof(SDL_Color)*num) ;

	if (inc < 0) /* Derecha */
	{
		inc = -inc ;
		memmove (&palette[color0+inc], &palette[color0], sizeof(SDL_Color)*(num-inc)) ;
		memcpy  (&palette[color0], &backup[color0+num-inc], sizeof(SDL_Color)*inc) ;
	}
	else /* Izquierda */
	{
		memmove (&palette[color0], &palette[color0+inc], sizeof(SDL_Color)*(num-inc)) ;
		memcpy  (&palette[color0+num-inc], &backup[color0], sizeof(SDL_Color)*inc) ;
	}

	palette_changed = 1 ;
}

void gr_fade_init (int r, int g, int b, int speed, int dir)
{
	fade_inc = speed * dir ;

	if (0)
	{
		if (fade_inc > 0) fade_step = 0 ;
		else              fade_step = 128 ;
	}

	fade_on  = dir ;

	fade_color.r = r ;
	fade_color.g = g ;
	fade_color.b = b ;

        GLODWORD(FADING) = 1 ;
}

void gr_fade_step()
{
	if (fade_on != 0)
	{
		fade_step += fade_inc ;
		if (fade_step >= 128) fade_on = 0 ;
		if (fade_step <= 0) fade_on = 0 ;
		if (fade_step < 0) fade_step = 0 ;
		if (fade_step > 128) fade_step = 128 ;
		activate_vpalette() ;

                GLODWORD(FADING) = (fade_on != 0) ;
	}
}

int gr_read_pal (file * fp)
{
	unsigned char colors[256][3] ;
	int i ;

	if (!file_read (fp, colors, 3 * 256))
		return 0 ;

	/* Ignora definiciones de gama */
	file_seek (fp, 576, SEEK_CUR) ;

	for (i = 0 ; i < 256 ; i++)
	{
		palette[i].r = colors[i][0] << 2 ;
		palette[i].g = colors[i][1] << 2 ;
		palette[i].b = colors[i][2] << 2 ;
	}

	palette_loaded = 1 ;
	palette_changed = 1 ;

	return 1 ;
}

int gr_load_pal (const char * filename)
{
	file * fp = file_open (filename, "rb") ;
	char header[8] ;
	int r = 0 ;

	if (!fp) return -1 ;
	file_read (fp, header, 8) ;
	if (strcmp (header, "map\x1A\x0D\x0A") == 0)
	{
		file_seek (fp, 48, SEEK_SET) ;
		r = gr_read_pal (fp) ;
	}
	else if (strcmp (header, "fpg\x1A\x0D\x0A") == 0 ||
	    strcmp (header, "fnt\x1A\x0D\x0A") == 0 ||
	    strcmp (header, "pal\x1A\x0D\x0A") == 0)
		r = gr_read_pal (fp) ;
	file_close(fp) ;
	return r;
}

int gr_rgb (int r, int g, int b)
{
	int color ;

	if (!scr_initialized) gr_init (320, 200) ;

	r >>= screen->format->Rloss ;
	g >>= screen->format->Gloss ;
	b >>= screen->format->Bloss ;

	color= (r << screen->format->Rshift) |
	       (g << screen->format->Gshift) |
	       (b << screen->format->Bshift) ;
	if (!color) return 1 ;
	return color ;
}

void gr_get_rgb (int color, int *r, int *g, int *b)
{
	if (!scr_initialized) gr_init (320, 200) ;

	if (!enable_16bits)
	{
		color &= 0xFF ;
		(*r) = palette[color].r ;
		(*g) = palette[color].g ;
		(*b) = palette[color].b ;
		return ;
	}

	(*r) = ((color & screen->format->Rmask) >> screen->format->Rshift) ;
	(*g) = ((color & screen->format->Gmask) >> screen->format->Gshift) ;
	(*b) = ((color & screen->format->Bmask) >> screen->format->Bshift) ;

        (*r) <<= screen->format->Rloss ;
        (*g) <<= screen->format->Gloss ;
        (*b) <<= screen->format->Bloss ;
}

/* Busca el color más cercano en la paleta a uno determinado */

/* Emplea una caché interna que acelera considerablemente el proceso,
 * a cambio de perder una parte de precisión */

static Uint8 nearest_table[64][64][64] ;

inline int find_nearest_color (const int first, const int last, SDL_Color key)
{
	int n, diff ;
	int best, bestdiff;
	SDL_Color * c ;

        n = nearest_table[key.r/4][key.g/4][key.b/4] ;
        if (!n && key.r/4 > 0) n = nearest_table[key.r/4-1][key.g/4][key.b/4] ;
        if (!n && key.g/4 > 0) n = nearest_table[key.b/4][key.g/4-1][key.b/4] ;
        if (!n && key.b/4 > 0) n = nearest_table[key.b/4][key.g/4][key.b/4-1] ;
        if (n) return n ;

        //printf ("*** %02d %02d %02d NOT FOUND ****\n", key.r/8, key.g/8, key.b/8) ;

	bestdiff = 999 ;
	
	for (n = first ; n <= last ; n++)
	{
		c = &palette[color_list[n]] ;
		diff =  (key.r > c->r ? key.r-c->r : c->r-key.r)  
		     +  (key.g > c->g ? key.g-c->g : c->g-key.g)  
		     +  (key.b > c->b ? key.b-c->b : c->b-key.b) ;
		if (diff < bestdiff) {
			bestdiff = diff ;
			best     = n ;
		}
	}

	return nearest_table[key.r/4][key.g/4][key.b/4] = color_list[best] ;
}

int gr_find_nearest_color (int r, int g, int b)
{
	SDL_Color c ;
	c.r = r ;
	c.g = g ;
	c.b = b ;

	return find_nearest_color(0, 255, c) ;
}

Uint16 gr_get_color (int r, int g, int b)
{
	return SDL_MapRGB (screen->format, (Uint8)r, (Uint8)g, (Uint8)b) ;
}

/* ----------------------------------------------------------------------
 * Función que rellena la tabla de color más cercano (nearest_table)
 * ---------------------------------------------------------------------- */

#define DUP 1
#define DDOWN 2
#define DRIGHT 3
#define DLEFT 4
#define DIN 5
#define DOUT 6

typedef struct __fillpoint {
        Uint8 r, g, b, dir, color ;
        struct __fillpoint * next ;
        struct __fillpoint * prev ;
} fillpoint ;

static fillpoint * filling = 0 ;

inline fillpoint * add_filling(int r, int g, int b, int color, int dir) 
{ 
        fillpoint * n ;
        
        if (nearest_table[r*2][g*2][b*2] != 0)
                return 0 ;

        //printf ("%02d %02d %02d -> %d\n", r, g, b, color) ;

        nearest_table[r*2+0][g*2+0][b*2+0] = color ;
        nearest_table[r*2+0][g*2+0][b*2+1] = color ;
        nearest_table[r*2+0][g*2+1][b*2+0] = color ;
        nearest_table[r*2+0][g*2+1][b*2+1] = color ;
        nearest_table[r*2+1][g*2+0][b*2+0] = color ;
        nearest_table[r*2+1][g*2+0][b*2+1] = color ;
        nearest_table[r*2+1][g*2+1][b*2+0] = color ;
        nearest_table[r*2+1][g*2+1][b*2+1] = color ;

        n = malloc(sizeof(fillpoint)) ;
        n->r = r ;
        n->g = g ;
        n->b = b ;
        n->color = color ;
        n->dir = dir ;
        n->next = filling ; 
        if (filling)
                filling->prev = n ;
        n->prev = 0 ;
        return filling = n ; 
}

inline void del_filling(fillpoint * n)
{
        if (n->prev)
                n->prev->next = n->next ;
        else
                filling = n->next ;
        if (n->next)
                n->next->prev = n->prev ;
        free (n) ;
}

void gr_fill_nearest_table()
{
        int n ;
        SDL_Color c ;
        register fillpoint * ptr ;
        fillpoint * next ;

        for (n = 1 ; n < 256 ; n++)
        {
		c = palette[n] ;
                c.r /= 8 ;
                c.g /= 8 ;
                c.b /= 8 ;
                add_filling (c.r, c.g, c.b, n, DRIGHT) ;
                if (c.r < 31) add_filling (c.r+1, c.g,   c.b,   n, DRIGHT) ;
                if (c.r > 0)  add_filling (c.r-1, c.g,   c.b,   n, DLEFT) ;
                if (c.g < 31) add_filling (c.r,   c.g+1, c.b,   n, DDOWN) ;
                if (c.g > 0)  add_filling (c.r,   c.g-1, c.b,   n, DUP) ;
                if (c.b < 31) add_filling (c.r,   c.g,   c.b+1, n, DIN) ;
                if (c.b > 0)  add_filling (c.r,   c.g,   c.b-1, n, DOUT) ;
        }

        while (filling)
        {
                ptr = filling ;
                while (ptr)
                {
                        if (ptr->r < 31) add_filling(ptr->r+1, ptr->g,   ptr->b,   ptr->color, DRIGHT) ;
                        if (ptr->r > 0)  add_filling(ptr->r-1, ptr->g,   ptr->b,   ptr->color, DLEFT) ;
                        if (ptr->g < 31) add_filling(ptr->r,   ptr->g+1, ptr->b,   ptr->color, DUP) ;
                        if (ptr->g > 0)  add_filling(ptr->r,   ptr->g-1, ptr->b,   ptr->color, DDOWN) ;
                        if (ptr->b < 31) add_filling(ptr->r,   ptr->g,   ptr->b+1, ptr->color, DIN) ;
                        if (ptr->b > 0)  add_filling(ptr->r,   ptr->g,   ptr->b-1, ptr->color, DOUT) ;
                        next = ptr->next ;
                        del_filling (ptr) ;
                        ptr = next ;
                }
        }

}

/* ----------------------------------------------------------------------
 * Rellena el contenido de la tabla de transparencias
 * ---------------------------------------------------------------------- */

static int compare_colors (const void * p1, const void * p2)
{
	SDL_Color * c1 = &palette[*(int *)p1] ;
	SDL_Color * c2 = &palette[*(int *)p2] ;

	return (c1->r + c1->g + c1->b) - (c2->r + c2->g + c2->b) ;
}

void gr_make_trans_table()
{
	int ticks1, ticks2 ;
	int a, b, c1, c2, n  ;
	SDL_Color base, dest ;

	ticks1 = SDL_GetTicks() ;

	memset (nearest_table, 0, sizeof(nearest_table)) ;
	for (n = 1 ; n < 256 ; n++) 
	{
		SDL_Color *c = &palette[n] ;
		nearest_table[c->r/4][c->g/4][c->b/4] = n ;
	}

	for (n = 0 ; n < 256 ; n++) color_list[n] = n ;
	qsort (color_list, 256, sizeof(int), compare_colors) ;

	gr_fill_nearest_table() ;

	for (a = 0 ; a < 256 ; a++)
	{
		c1 = color_list[a] ;
		base.r = palette[c1].r/2 ;
		base.g = palette[c1].g/2 ;
		base.b = palette[c1].b/2 ;

		for (b = 0 ; b < a ; b++)
		{
			c2 = color_list[b] ;
			dest.r = base.r + palette[c2].r/2 ;
			dest.g = base.g + palette[c2].g/2 ;
			dest.b = base.b + palette[c2].b/2 ;

			trans_table[c1][c2] = 
			trans_table[c2][c1] = 
				find_nearest_color (b, a, dest) ;
		}

		trans_table[c1][c1] = c1 ;
		trans_table[0][c1]  = c1 ;
	}

        trans_table_updated = 1 ;

	ticks2 = SDL_GetTicks() ;
        if (report_graphics)
                gr_con_printf ("[GRAPH] Palette analysis done in %d ms\n", 
                                ticks2-ticks1) ;
}

#include "font.h"

void gr_refresh_palette()
{
        int n ;

	if (!scr_initialized) return ;

	memset (nearest_table, 0, sizeof(nearest_table)) ;

	for (n = 0 ; n < 256 ; n++) color_list[n] = n ;
	qsort (color_list, 256, sizeof(int), compare_colors) ;

	trans_table_updated = 0 ;

	/* Actualiza la paleta */

	palette_changed = 0 ;

	if (fade_step != 128) 
	{
		memcpy (vpalette, palette, sizeof(vpalette)) ;
		activate_vpalette() ;
	}
	else 
	{
		if (enable_16bits)
			for (n = 0 ; n < 256 ; n++)
				colorequiv[n] = SDL_MapRGB (screen->format,
					palette[n].r, palette[n].g, palette[n].b) ;
		else
			SDL_SetColors (screen, palette, 0, 256) ;
	}

	/* Actualiza la fuente del sistema */

	gr_font_systemfont (default_font) ;
}

void gr_set_rgb (int color, int r, int g, int b)
{
	if (color < 0 || color > 255) return ;
	palette[color].r = r << 2 ;
	palette[color].g = g << 2 ;
	palette[color].b = b << 2 ;
	palette_changed  = 1 ;
}

/* Rutinas para guardar y recibir trozos de paleta desde un buffer */

void gr_get_colors (int color, int num, Uint8 * pal)
{
	if (num < 1 || color < 0 || color > 255) return ;
	if (color+num >= 256) num = 256-color ;

	while (num--)
	{
		*pal++ = palette[color  ].r ;
		*pal++ = palette[color  ].g ;
		*pal++ = palette[color++].b ;
	}
	palette_changed = 1 ;
}

void gr_set_colors (int color, int num, Uint8 * pal)
{
	if (num < 1 || color < 0 || color > 255) return ;
	if (color+num >= 256) num = 256-color ;

	while (num--)
	{
		palette[color  ].r = *pal++ ;
		palette[color  ].g = *pal++ ;
		palette[color++].b = *pal++ ;
	}
	palette_changed = 1 ;
}

/* ---------------------------------------------------------------------- */

Sint16 * blend_create ()
{
        Sint16 * blend ;

        blend = malloc(65536*2*sizeof(Sint16)) ;
        if (!blend) gr_error ("blend_create: sin memoria") ;
        blend_init (blend) ;
        return blend ;
}

void blend_free (Sint16 * blend)
{
        free (blend) ;
}

void blend_init (Sint16 * blend)
{
        int i ;

        for (i = 0 ; i < 65536 ; i++)
                blend[i] = i ;

        blend += 65536 ;
        for (i = 0 ; i < 65536 ; i++)
                blend[i] = 0 ;
}

void blend_intensity (Sint16 * blend, double ammount)
{
        int i, r, g, b ;

        if (ammount < 0.0) ammount = 0.0 ;

        for (i = 0 ; i < 65536 ; i++)
        {
                gr_get_rgb (i, &r, &g, &b) ;
                r *= ammount ;
                g *= ammount ;
                b *= ammount ;
                if (r > 255) r = 255 ;
                if (g > 255) g = 255 ;
                if (b > 255) b = 255 ;
                blend[i] = gr_rgb (r, g, b) ;
        }

        blend += 65536 ;
        for (i = 0 ; i < 65536 ; i++)
                blend[i] = 0 ;
}

void blend_translucency (Sint16 * blend, double ammount)
{
        int i, r, g, b ;

        if (ammount > 1.0) ammount = 1.0 ;
        if (ammount < 0.0) ammount = 0.0 ;

        ammount = 1.0 - ammount ;
        for (i = 0 ; i < 65536 ; i++)
        {
                gr_get_rgb (blend[i], &r, &g, &b) ;
                r *= ammount ;
                g *= ammount ;
                b *= ammount ;
                blend[i] = gr_rgb (r, g, b) ;
        }

        blend += 65536 ;
        ammount = 1.0 - ammount ;
        for (i = 0 ; i < 65536 ; i++)
        {
                gr_get_rgb (i, &r, &g, &b) ;
                r *= ammount ;
                g *= ammount ;
                b *= ammount ;
                blend[i] = gr_rgb (r, g, b) ;
        }
}

void blend_tint (Sint16 * blend, double ammount, Uint8 cr, Uint8 cg, Uint8 cb)
{
        int i, r, g, b ;

        if (ammount > 1.0) ammount = 1.0 ;
        if (ammount < 0.0) ammount = 0.0 ;

        for (i = 0 ; i < 65536 ; i++)
        {
                gr_get_rgb (blend[i], &r, &g, &b) ;
                r = ammount * cr + (1.0 - ammount) * r ;
                g = ammount * cg + (1.0 - ammount) * g ;
                b = ammount * cb + (1.0 - ammount) * b ;
                if (r > 255) r = 255 ;
                if (g > 255) g = 255 ;
                if (b > 255) b = 255 ;
                blend[i] = gr_rgb (r, g, b) ;
        }

        blend += 65536 ;
        for (i = 0 ; i < 65536 ; i++)
                blend[i] = 0 ;
}

void blend_swap (Sint16 * blend)
{
        int i, j ;

        for (i = 0 ; i < 65536 ; i++)
        {
                j = blend[i] ;
                blend[i] = blend[65536+i] ;
                blend[65536+i] = j ;
        }
}

void blend_assign (GRAPH * map, Sint16 * blend)
{
        map->blend_table = blend ;
}

void blend_apply (GRAPH * map, Sint16 * blend)
{
	Uint16 * ptr ;
	int x, y ;

	if (map->depth != 16) return ;
	
	for (y = 0 ; y < map->height ; y++)
	{
		ptr = (Uint16 *)map->data + map->pitch * y ;
		for (x = 0 ; x < map->width ; x++, ptr++)
		{
			if (*ptr) 
				*ptr = blend[*ptr] + blend[65536+*ptr] ;
		}
	}
}
