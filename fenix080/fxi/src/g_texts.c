/*
 *  Fenix - Videogame compiler/interpreter
 *  Current release       : FENIX - PROJECT 1.0 - R 0.76
 *  Last stable release   : 
 *  Project documentation : http://fenix.divsite.net
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software 
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 *  Copyright © 1999 José Luis Cebrián Pagüe
 *  Copyright © 2002 Fenix Team
 *
 */

/*
 * FILE        : g_texts.c
 * DESCRIPTION : Text manipulation routines
 *
 * HISTORY:	0.74 - gr_text_new_var modified to support data types
 *			0.74 - gr_text_put_all modified to suppor new data types 
 *			0.72 - Corrected gr_text_bitmap (WRITE_IN_MAP)
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include "fxi.h"

#define MAX_TEXTS 512

/* ---------------------------------------------------------------------- */
/* Fuentes                                                                */
/* ---------------------------------------------------------------------- */

FONT * fonts[256] = { 0 } ;
int    font_count = 1 ;		/* Fuente 0 reservada para sistema */

struct {
	int on ;		/* 1 - Texto ; 2 - VarSTR; 3 - VarINT; 4 - VarFLOAT; 5 - VarWORD; 6 - VarBYTE */
	int fontid ;
	int x ;
	int y ;
	int centered ;
	char * text ;           /* Memoria dinámica */
	const void * var  ;		/* CHANGED TO VOID to allow diff. data types */
} texts[MAX_TEXTS] ;

int text_nextid = 1 ;
int text_count  = 0 ;

#ifdef LINUX
char *strrev(char *str)
{
      char *p1, *p2;

      if (! str || ! *str)
            return str;
      for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2)
      {
            *p1 ^= *p2;
            *p2 ^= *p1;
            *p1 ^= *p2;
      }
      return str;
}

#endif


int gr_text_new (int fontid, int x, int y, int centered, const char * text)
{
	int textid = text_nextid ;

	if (text_nextid == MAX_TEXTS)
	{
		for (textid = 1 ; textid < MAX_TEXTS ; textid++)
			if (!texts[textid].on) break ;
		if (textid == MAX_TEXTS)
			gr_error ("Demasiados textos en pantalla") ;
	}
	else text_nextid++ ;
	text_count++ ;

	texts[textid].on = 1 ;
	texts[textid].fontid = fontid ;
	texts[textid].x = x ;
	texts[textid].y = y ;
	texts[textid].centered = centered ;
	texts[textid].text = text ? strdup(text) : 0 ;
	return textid ;
}

int gr_text_new_var (int fontid, int x, int y, int centered, const void * var, int type)
{
	int textid = gr_text_new (fontid, x, y, centered, 0) ;

	if (textid < 0) return -1 ;
	texts[textid].on = type ;
	if (type > 1)
		texts[textid].var = var ;
	
	return textid ;
}

void gr_text_move (int textid, int x, int y)
{
	if (textid > 0 && textid < text_nextid)
	{
		texts[textid].x = x ;
		texts[textid].y = y ;
	}
}

void gr_text_destroy (int textid)
{
	if (textid == 0)
	{
		for (textid = 1 ; textid < text_nextid ; textid++)
		{
			if (texts[textid].on)
				free (texts[textid].text) ;
			texts[textid].on = 0 ;
		}
		text_count = 0 ;
		text_nextid = 1 ;
		return ;
	}
	if (textid > 0 && textid < text_nextid)
	{
		if (!texts[textid].on) return ;

		free (texts[textid].text) ;
		texts[textid].on = 0 ;
		if (textid == text_nextid-1)
		{
			while (text_nextid > 1 && !texts[text_nextid-1].on)
				text_nextid-- ;
		}
		text_count-- ;
	}
}

void gr_text_put_all ()
{
	int i, w, h, x, y ;
	const char * text ;
	char buffer[128] ;
	char * aux ;

	/* Un refresco de paleta en mitad de gr_text_put puede provocar efectos
	 * desagradables al modificar el tipo de letra del sistema */

	if (palette_changed) gr_refresh_palette() ;

	for (i = 1 ; i < text_nextid ; i++)
	{
		if (!texts[i].on) continue ;

		if (texts[i].on > 1 )
		{
			switch (texts[i].on) {
				case 2:
					text = string_get(*(int*)texts[i].var) ;
					break ;
				case 3:
					sprintf (buffer, "%d", *(int *)texts[i].var) ;
					text = buffer ;
					break ;
				case 4:
					sprintf (buffer, "%f", *(float *)texts[i].var) ;
					/* REMOVING TRAILING 0s */
					aux = strrev(buffer) ;
					w=0 ;
					while (*aux=='0') {
						w++ ;
						aux++;
					}					
					if (*aux=='.') aux-- ;
					strcpy(buffer,aux) ;
					strrev(buffer) ;
					text = buffer ;
					break ;
				case 5:
					sprintf (buffer, "%d", *(Uint8 *)texts[i].var) ;
					text = buffer ;
					break ;
				case 6:
					sprintf (buffer, "%d", *(Uint16 *)texts[i].var) ;
					text = buffer ;
					break ;

			}
			
		}
		else	text = texts[i].text ;

		w = gr_text_width  (texts[i].fontid, text) ;
		h = gr_text_height (texts[i].fontid, text) ;

		switch (texts[i].centered)
		{
			default:

			case 0:	/* Arriba izda */
				x = texts[i].x ;
				y = texts[i].y ;
				break ;
			case 1:	/* Arriba cent */
				x = texts[i].x - w/2 ;
				y = texts[i].y ;
				break ;
			case 2:	/* Arriba dcha */
				x = texts[i].x - w ;
				y = texts[i].y ;
				break ;
			case 3:	/* Centro izda */
				x = texts[i].x ;
				y = texts[i].y - h/2 ;
				break ;
			case 4:	/* Centro cent */
				x = texts[i].x - w/2 ;
				y = texts[i].y - h/2 ;
				break ;
			case 5:	/* Centro dcha */
				x = texts[i].x - w ;
				y = texts[i].y - h/2 ;
				break ;
			case 6:	/* Abajo  izda */
				x = texts[i].x ;
				y = texts[i].y - h+1 ;
				break ;
			case 7:	/* Abajo  cent */
				x = texts[i].x - w/2 ;
				y = texts[i].y - h+1 ;
				break ;
			case 8:	/* Abajo  dcha */
				x = texts[i].x - w ;
				y = texts[i].y - h+1 ;
				break ;
		}

		gr_text_put (0, texts[i].fontid, x, y, text) ;
	}
}

/* -------- */

static int
get_char_width (unsigned char *data, int height)
{
	int c, d, max = 0 ;

	while (height--)
	{ 
		c = *data++ ;
	        for (d = 8 ; d > 0 ; d--, c >>= 1)
		{
			if (c & 0x01) break ;
		}
      		if (d > max) max = d ;
    	}
  	return max+1 ;
}

static void
align_char_left (unsigned char *data, int height)
{
  	int        leftest, n ;
  	static int leftest_table[256] = { 0 } ;

  	/* Inicializa la tabla */
  	if (leftest_table[0] == 0)
    	{
		for (n = 0 ; n < 256 ; n++)
		{
			if (n & 0x80) leftest_table[n] = 0 ;
			else if (n & 0x40) leftest_table[n] = 1 ;
			else if (n & 0x20) leftest_table[n] = 2 ;
			else if (n & 0x10) leftest_table[n] = 3 ;
			else if (n & 0x08) leftest_table[n] = 4 ;
			else if (n & 0x04) leftest_table[n] = 5 ;
			else if (n & 0x02) leftest_table[n] = 6 ;
			else if (n & 0x01) leftest_table[n] = 7 ;
			else               leftest_table[n] = 8 ;
		}
    	}

  	leftest = 8 ;
  	for (n = 0 ; n < height ; n++)
	{
		if (leftest > leftest_table[data[n]])
			leftest = leftest_table[data[n]] ;
	}
	
  	for (n = 0 ; n < height ; n++)
		data[n] <<= leftest ;
}

int gr_font_new8x8 (char * chardata, char color_on, char color_off)
{
	FONT * f ;
	char * ptr, c ;
	int i, x, y ;
	static int error = 0 ;

	if (error) return 0 ;

	f = (FONT *)malloc(sizeof(FONT)) ;
	if (!f) 
	{
		error = 1 ;
		gr_error ("gr_font_new8x8: sin memoria");
	}
	memset (f, 0, sizeof(FONT)) ;

	for (i = 0 ; i < 256 ; i++)
	{
		align_char_left (chardata + 8*i, 8) ;
		f->chars[i] = bitmap_new (i, 8, 8, 8) ;
		f->width[i] = get_char_width (chardata+8*i, 8) ;
		assert (f->chars[i]) ;
		bitmap_add_cpoint (f->chars[i], 0, 0) ;
		f->voffset[i] = 0 ;
		memset (f->chars[i]->data, color_off, 8*8) ;

		ptr = f->chars[i]->data ;
		for (y = 0 ; y < 8 ; y++)
		{
			c = chardata[8*i+y] ;
			for (x = 0x80 ; x >= 0x01 ; x >>= 1, ptr++)
				if (c & x) *ptr = color_on ;
		}

		f->chars[i]->modified = 0 ;
		f->chars[i]->info_flags = 0 ;
	}
	f->width[32] = 5 ;

	fonts[font_count] = f ;
	return font_count++ ;
}

static int gr_font_8x8_load (file * fp)
{
	char chardata[256*8] ;
	char line[256], *ptr ;
	int c, mask ;

	c = 0 ;
	for (c = 0 ; c < 256*8 ;)
	{
		if (file_eof(fp))
			break ;
		file_gets (fp, line, 256) ;
		if (!*line || *line == '#' || strcmp(line, "\n") == 0) 
			continue ;
		ptr = line ;
		mask = 0x80 ;
		while (*ptr) {
			if (*ptr == '0' || *ptr == '·')
			{
				chardata[c] &= ~mask ;
				mask >>= 1 ;
			}
			if (*ptr == '1' || *ptr == 'O')
			{
				chardata[c] |= mask ;
				mask >>= 1 ;
			}
			ptr++ ;
		}
		c++ ;
	}
	return gr_font_new8x8 (chardata, 
			(char)gr_find_nearest_color(255,255,255), 0) ;
}

int gr_font_systemfont (char * chardata)
{
	int last_count = font_count ;
	if (fonts[0]) gr_font_destroy(0) ;
	font_count = 0 ;
	gr_font_new8x8 (chardata, (char)gr_find_nearest_color(255,255,255), 0) ;
	if (last_count) font_count = last_count ;

	return 1;
}

void gr_font_destroy (int fontid)
{
	int n ;

	if (fontid >= 0 && fontid < font_count)
	{
		if (!fonts[fontid]) return ;
		for (n = 0 ; n < 256 ; n++)
		{
			if (fonts[fontid]->chars[n])
				bitmap_destroy (fonts[fontid]->chars[n]) ;
		}
		free (fonts[fontid]) ;
		fonts[fontid] = 0 ;
		while (font_count > 0 && fonts[font_count-1] == 0)
			font_count-- ;
	}
}

int gr_font_load (char * filename)
{
	file * fp ;
	char header[8] ;
	int types, i ;
	FONT * f ;

	struct
	{
		int	width ;
		int	height ;
		int	voffset ;
		int	fileoffset ;
	}
	chardata[256] ;
		
	if (font_count == 256) return -1 ;

	fp = file_open (filename, "rb") ;
	if (!fp) return -1 ;
	if (!file_read(fp, header,8))        { file_close(fp); return -1 ; }
	if (strncmp(header,"FONTFILE",8) == 0) return gr_font_8x8_load(fp) ;
	if (palette_loaded)
		file_seek (fp, 576 + 768, SEEK_CUR) ;
	else if (!gr_read_pal (fp))          { file_close(fp); return -1 ; }
	if (!file_read(fp, &types,4))        { file_close(fp); return -1 ; }
	if (!file_read(fp, chardata,16*256)) { file_close(fp); return -1 ; }

	f = (FONT *)malloc(sizeof(FONT)) ;
	assert (f != 0) ;
	memset (f, 0, sizeof(FONT)) ;

	for (i = 0 ; i < 256 ; i++)
	{
		if (chardata[i].width >= 0)
			f->width[i] = chardata[i].width ;

		if (chardata[i].fileoffset == 0 ||
		    chardata[i].width      == 0 ||
		    chardata[i].height     == 0) continue ;

		file_seek (fp, chardata[i].fileoffset, SEEK_SET) ;
		f->chars[i] = bitmap_new (i, chardata[i].width, chardata[i].height, 8) ;
		assert (f->chars[i]) ;
		bitmap_add_cpoint (f->chars[i], 0, 0) ;

		if (!file_read (fp, f->chars[i]->data, chardata[i].width * chardata[i].height))
			continue ;

		f->voffset[i] = chardata[i].voffset ;
	}
	if (f->width[32] == 0) f->width[32] = 4 ;

	file_close (fp) ;

	fonts[font_count] = f ;
	return font_count++ ;
}

int gr_text_width (int fontid, const unsigned char * text)
{
	int l = 0 ;
	FONT * f ;

        if (fontid < 0 || fontid > 255 || !fonts[fontid])
                gr_error ("Tipo de letra incorrecto (%d)", fontid) ;

        f = fonts[fontid] ;
	
	while (*text) 
	{
		l += f->width[*text++] ;
	}
	return l ;
}

int gr_text_height (int fontid, const unsigned char * text)
{
	int l = 0 ;
	FONT * f ;

        if (fontid < 0 || fontid > 255 || !fonts[fontid])
                gr_error ("Tipo de letra incorrecto (%d)", fontid) ;

        f = fonts[fontid] ;

	while (*text) 
	{
		if (f->chars[*text])
			if (f->chars[*text]->height > l)
				l = f->chars[*text]->height ;
		text++ ;
	}
	return l ;
}

void gr_text_put (GRAPH * dest, int fontid, int x, int y, const unsigned char * text)
{
	GRAPH * ch ;
	FONT   * f ;
	int flags ;

	if (!dest) dest = scrbitmap ;

        if (fontid < 0 || fontid > 255 || !fonts[fontid])
                gr_error ("Tipo de letra incorrecto (%d)", fontid) ;

        f = fonts[fontid] ;

	/* Se permite imprimir texto antes de inicializar los globales */

	flags = globaldata ? GLODWORD(TEXT_FLAGS) : 0 ;
	
	while (*text)
	{
		ch = f->chars[*text] ;
		if (ch) gr_blit (dest, 0, x, y + f->voffset[*text], flags, ch) ;
		x += f->width[*text] ;
		text++ ;
	}
}

GRAPH * gr_text_bitmap (int fontid, const char * text, int centered)
{
	GRAPH * gr ;
	int x, y ;
	FONT   * f ;
        
        if (fontid < 0 || fontid > 255 || !fonts[fontid])
                gr_error ("Tipo de letra incorrecto (%d)", fontid) ;

        f = fonts[fontid] ;
	
	/* Un refresco de paleta en mitad de gr_text_put puede provocar efectos
	 * desagradables al modificar el tipo de letra del sistema */

	if (palette_changed) gr_refresh_palette() ;

	gr = bitmap_new_syslib (gr_text_width (fontid, text),
		gr_text_height (fontid, text), enable_16bits ? 16:8) ;
	assert (gr) ;
	gr_clear (gr) ;
	gr_text_put (gr, fontid, 0, -f->voffset['A'], text) ;

	switch (centered)
	{
		case 0:
		case 1:
		case 2:
			y = 0 ;
			break ;
		case 3:
		case 4:
		case 5:
			y = gr->height/2 ;
			break ;
		default:
			y = gr->height-1 ;
			break ;

	}

	switch (centered)
	{
		case 0:
		case 3:
		case 6:
			x = 0 ;
			break ;
		case 1:
		case 4:
		case 7:
			x = gr->width/2 ;
			break ;
		default:
			x = gr->width-1 ;
			break ;

	}

	bitmap_add_cpoint (gr, x, y) ;
	return gr ;
}
