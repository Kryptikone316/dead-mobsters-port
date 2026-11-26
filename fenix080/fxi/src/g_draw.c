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

#include <string.h>
#include <assert.h>
#include <math.h>

#include <SDL.h>

#include "files.h"
#include "grlib.h"

#define ABS(x) ((x) < 0 ? -(x):(x))

static int color ;

#ifdef __GNUC__
#define _inline inline
#endif

_inline void _HLine8 (Uint8 * ptr, Uint32 length)
{
	memset (ptr, color, length) ;
}

_inline void _HLine16 (Uint16 * ptr, Uint32 length)
{
	register int n ;

	for (n = length ; n ; n--)
	{
		*ptr++ = color ;
	}
}

_inline void _Pixel8 (Uint8 * ptr)
{
	*ptr = color ;
}

_inline void _Pixel16 (Uint16 * ptr)
{
	*ptr = color ;
}

void gr_setcolor (int c)
{
	color = c ;
}

int gr_get_pixel (GRAPH * dest, int x, int y)
{
	if (x < 0 || y < 0 || x >= dest->width || y >= dest->height)
		return -1 ;

	if (dest->depth == 8)
		return ((Uint8 *)dest->data) [x + dest->width*y] ;
	else
		return ((Uint16 *)dest->data)[x + dest->width*y] ;
}

void gr_put_pixel (GRAPH * dest, int x, int y, int color)
{
	if (x < 0 || y < 0 || x >= dest->width || y >= dest->height)
		return ;

	dest->modified++ ;

	if (dest->depth == 8)
		((Uint8 *)dest->data)[x + dest->width*y] = color ;
	else
		((Uint16 *)dest->data)[x + dest->width*y] = color ;
}

void gr_clear (GRAPH * dest)
{
	dest->modified++ ;

	if (dest->depth == 8)
		memset (dest->data, 0, dest->pitch * dest->height) ;
	else
		memset (dest->data, 0, dest->pitch * dest->height * 2) ;
}

void gr_clear_as (GRAPH * dest, int color)
{
	dest->modified++ ;

	if (dest->depth == 8)
		memset (dest->data, color, dest->pitch * dest->height) ;
	else
	{
		Uint16 * ptr = dest->data ;
		int n, m = dest->pitch * dest->height ;
		for (n = 0 ; n < m ; n++) *ptr++ = color ;
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - Línea vertical */

void gr_vline (GRAPH * dest, REGION * clip, int x, int y, int h)
{
	REGION base_clip ;

	if (!dest) dest = scrbitmap ;
	if (!clip)
	{
		clip = &base_clip ;
		clip->x = 0 ;
		clip->y = 0 ;
		clip->x2 = dest->width-1 ;
		clip->y2 = dest->height-1 ;
	}

	dest->modified++ ;

	if (h < 0) h = -h, y -= h ;
	if (x < clip->x || x > clip->x2) return ;

	if (y < clip->y) h += y-clip->y, y = clip->y ;
	if (y+h > clip->y2) h = clip->y2+1-y ;

	if (h < 1) return ;

	if (dest->depth == 8)
	{
		Uint8 * ptr = dest->data ;
		ptr += dest->pitch * y + x ;
		while (h--)
		{
			_Pixel8 (ptr) ;
			ptr += dest->width ;
		}
	}
	else if (dest->depth == 16)
	{
		Uint16 * ptr = dest->data ;
		ptr += dest->pitch * y + x ;
		while (h--)
		{
			_Pixel16 (ptr) ;
			ptr += dest->width ;
		}
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - Línea horizontal */

void gr_hline (GRAPH * dest, REGION * clip, int x, int y, int w)
{
	REGION base_clip ;

	if (!dest) dest = scrbitmap ;
	if (!clip)
	{
		clip = &base_clip ;
		clip->x = 0 ;
		clip->y = 0 ;
		clip->x2 = dest->width-1 ;
		clip->y2 = dest->height-1 ;
	}

	dest->modified++ ;

	if (w < 0) w = -w, x -= w ;
	if (y < clip->y || y > clip->y2) return ;

	if (x < clip->x) w += x-clip->x, x = clip->x ;
	if (x+w > clip->x2) w = clip->x2+1-x ;

	if (w < 1) return ;

	if (dest->depth == 8)
	{
		Uint8 * ptr = dest->data ;
		ptr += dest->pitch * y + x ;
		_HLine8 (ptr, w) ;
	}
	else if (dest->depth == 16)
	{
		Uint16 * ptr = dest->data ;
		ptr += dest->pitch * y + x ;
		_HLine16 (ptr, w) ;
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - Caja rellena */

void gr_box (GRAPH * dest, REGION * clip, int x, int y, int w, int h)
{
	REGION base_clip ;

	if (!dest) dest = scrbitmap ;
	if (!clip)
	{
		clip = &base_clip ;
		clip->x = 0 ;
		clip->y = 0 ;
		clip->x2 = dest->width-1 ;
		clip->y2 = dest->height-1 ;
	}

	dest->modified++ ;

	if (w < 0) w = -w, x -= w ;
	if (h < 0) h = -h, y -= h ;

	if (x < clip->x) w += x-clip->x, x = clip->x ;
	if (y < clip->y) h += y-clip->y, y = clip->y ;
	if (x+w > clip->x2) w = clip->x2+1-x ;
	if (y+h > clip->y2) h = clip->y2+1-y ;

	if (w < 1 || h < 1) return ;

	if (dest->depth == 8)
	{
		Uint8 * ptr = dest->data ;
		ptr += dest->pitch * y + x ;
		while (h--)
		{
			_HLine8 (ptr, w) ;
			ptr += dest->width ;
		}
	}
	else if (dest->depth == 16)
	{
		Uint16 * ptr = dest->data ;
		ptr += dest->pitch * y + x ;
		while (h--)
		{
			_HLine16 (ptr, w) ;
			ptr += dest->width ;
		}
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - Rectángulo */

void gr_rectangle (GRAPH * dest, REGION * clip, int x, int y, int w, int h)
{
	if (w < 0) w = -w, x -= w ;
	if (h < 0) h = -h, y -= h ;

	gr_hline (dest, clip, x, y, w) ;
	gr_vline (dest, clip, x, y, h) ;
	gr_hline (dest, clip, x, y+h-1, w) ;
	gr_vline (dest, clip, x+w-1, y, h) ;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - Círculo */

void gr_circle (GRAPH * dest, REGION * clip, int x, int y, int r)
{
	int cx = 0, cy = r ;
	int df = 1-r, de = 3, dse = -2*r + 5 ;

	do 
	{
		gr_put_pixel (dest, x+cx, y+cy, color) ;
		gr_put_pixel (dest, x+cx, y-cy, color) ;
		gr_put_pixel (dest, x-cx, y+cy, color) ;
		gr_put_pixel (dest, x-cx, y-cy, color) ;

		gr_put_pixel (dest, x+cy, y+cx, color) ;
		gr_put_pixel (dest, x+cy, y-cx, color) ;
		gr_put_pixel (dest, x-cy, y+cx, color) ;
		gr_put_pixel (dest, x-cy, y-cx, color) ;


		cx++ ;
		if (df < 0) df += de,  de += 2, dse += 2 ;
		else	    df += dse, de += 2, dse += 4, cy-- ;
	}
	while (cx <= cy) ;
}

void gr_fcircle (GRAPH * dest, REGION * clip, int x, int y, int r)
{
	int cx = 0, cy = r ;
	int df = 1-r, de = 3, dse = -2*r + 5 ;

	do 
	{
		gr_hline (dest, clip, x-cx, y-cy, 2*cx) ;
		gr_hline (dest, clip, x-cx, y+cy, 2*cx) ;
		gr_hline (dest, clip, x-cy, y-cx, 2*cy) ;
		gr_hline (dest, clip, x-cy, y+cx, 2*cy) ;

		cx++ ;
		if (df < 0) df += de,  de += 2, dse += 2 ;
		else	    df += dse, de += 2, dse += 4, cy-- ;
	}
	while (cx <= cy) ;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - Línea */

void gr_line (GRAPH * dest, REGION * clip, int x, int y, int w, int h)
{
	int n, m, hinc, vinc ;
	int i1, i2, dd ;
	REGION base_clip ;

	if (!w) { 
		gr_vline (dest, clip, x, y, h) ; 
		return ;
		}
	if (!h) { 
		gr_hline (dest, clip, x, y, w) ; 
		return ;
		}

	if (!dest) dest = scrbitmap ;
	if (!clip)
	{
		clip = &base_clip ;
		clip->x = 0 ;
		clip->y = 0 ;
		clip->x2 = dest->width-1 ;
		clip->y2 = dest->height-1 ;
	}

	dest->modified++ ;

	/* Clipping de la línea - INCORRECTO pero funcional */

	if (!w || !h) return ;

	if (x < clip->x) /* izquierda */
	{
		n = clip->x - x ; m = n*h/ABS(w) ;
		x += n, w -= n, y -= m, h -= m ;
	}
	if (x+w < clip->x) /* w < 0 */
	{
		n = clip->x - (x+w) ; m = n*h/ABS(w) ;
		w += n, h -= m ;
	}
	if (y < clip->y) /* arriba */
	{
		m = clip->y - y ; n = m*w/ABS(h) ;
		x += n, w -= n, y += m, h -= m ;
	}
	if (y+h < clip->y) /* h < 0 */
	{
		m = clip->y - (y+h) ; n = m*w/ABS(h) ;
		w -= n, h += m ;
	}
	if (x > clip->x2) /* derecha */
	{
		n = x - clip->x2 ; m = n*h/ABS(w) ;
		x -= n, w -= n, y -= m, h -= m ;
	}
	if (x+w > clip->x2) /* w > 0 */
	{
		n = (x+w) - clip->x2 ; m = n*h/ABS(w) ;
		w -= n, h -= m ;
	}
	if (y > clip->y2) /* abajo */
	{
		m = y - clip->y2 ; n = m*w/ABS(h) ;
		x -= n, w -= n, y -= m, h -= m ;
	}
	if (y+h > clip->y2) /* h > 0 */
	{
		m = (y+h) - clip->y2 ; n = m*w/ABS(h) ;
		w -= n, h -= m ;
	}

	if (w == 0 || h == 0) return ;

	hinc = (w > 0) ? 1 : -1 ;
	vinc = (h > 0) ? dest->width : -dest->width ;
	
	/* Aquí va una implementación deprisa y corriendo de Bresenham */

	w = ABS(w) ;
	h = ABS(h) ;

	if (w > h)
	{
		i1 = 2 * h ;
		dd = i1 - w ;
		i2 = dd - w ;
	}
	else
	{
		i1 = 2 * w ;
		dd = i1 - h ;
		i2 = dd - h ;
	}

	if (dest->depth == 8)
	{
		Uint8 * ptr = (Uint8 *)dest->data + dest->width*y + x;

		if (w > h) while (w--)
		{
			_Pixel8(ptr) ;
			if (dd >= 0) ptr += hinc+vinc, dd += i2 ;
			else         ptr += hinc,      dd += i1 ;
		}
		else while (h--)
		{
			_Pixel8(ptr) ;
			if (dd >= 0) ptr += vinc+hinc, dd += i2 ;
			else         ptr += vinc,      dd += i1 ;
		}
	}
	else if (dest->depth == 16)
	{
		Uint16 * ptr = (Uint16 *)dest->data + dest->width*y + x;

		if (w > h) while (w--)
		{
			_Pixel16(ptr) ;
			if (dd >= 0) ptr += hinc+vinc, dd += i2 ;
			else         ptr += hinc,      dd += i1 ;
		}
		else while (h--)
		{
			_Pixel16(ptr) ;
			if (dd >= 0) ptr += vinc+hinc, dd += i2 ;
			else         ptr += vinc,      dd += i1 ;
		}
	}
}
