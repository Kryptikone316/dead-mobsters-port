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

/*
 * HISTORY: changed gr->cpoint_count (non existent) by gr->cpoints check against 0 (NULL)
 */

#include <string.h>
#include <assert.h>
#include <math.h>

#include <SDL.h>

#include "files.h"
#include "grlib.h"

#ifndef M_PI
# define M_PI           3.14159265358979323846  /* pi */
#endif

/* ---------------------------------------------------------------------- */
/* Rutinas de bajo nivel                                                  */
/* ---------------------------------------------------------------------- */

extern SDL_Surface * screen ;

extern GRAPH * scrbitmap ;
extern Uint8 trans_table[256][256] ;

#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define ABS(a)     (((a) > 0  ) ? (a) :(-a))

/* Rutinas optimizadas en ensamblador, que no hace clipping */

void gr_blit (GRAPH * dest, REGION * clip, int x, int y, int flags, GRAPH * gr)
{
	register int cx ;
	int cy, maxcy, maxcx ;
	int hinc, vinc, offs ;
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

	/* Hack - En modo 16 bits, hay que refrecar la paleta para que
	 * las estructuras internas se actualicen */

	if (gr->cpoints > 0 && gr->cpoints[0].x >= 0)
	{
		x -= gr->cpoints[0].x ;
		y -= gr->cpoints[0].y ;
	}
	else
	{
		x -= gr->width / 2 ;
		y -= gr->height / 2 ;
	}
	
	/* Establece punteros de origen y trunca las coordenadas */

	if (x > clip->x2 || y > clip->y2)
		return ;

	maxcy = MIN (clip->y2, y + gr->height - 1) ;
	maxcx = MIN (clip->x2, x + gr->width  - 1) ;

	vinc = gr->width ;
	hinc = 1 ;

	offs = 0 ;

	if (flags & B_VMIRROR)
	{
		vinc = -vinc ;
		offs += gr->width * (gr->height-1) ;
	}
	if (flags & B_HMIRROR)
	{
		hinc = -1 ;
		offs += gr->width-1 ;
	}

	if (x < clip->x)
	{
		x -= clip->x ;
		offs += hinc > 0 ? -x:x ;
		x = clip->x ;
	}

	if (y < clip->y)
	{
		y -= clip->y ;
		offs += vinc * -y ;
		y = clip->y ;
	}

	if (maxcx < x || maxcy < y)
		return ;

	/* Copia de bits */

	maxcx -= x ;

	if (dest->depth == 8 && gr->depth == 8)
	{
		/* De 8 bits a 8 bits */
		register Uint8 * ptr ;
		register Uint8 * origptr ;
		Uint8 * line, * orig ;

		line = (Uint8*)dest->data + dest->width * y + x ;
		orig = (Uint8*)gr->data   + offs ;

		if (flags & B_TRANSLUCENT) for (cy = y ; cy <= maxcy ; cy++)
		{
			ptr = line, origptr = orig ;
			if (hinc > 0)
				for (cx = maxcx ; cx >= 0 ; cx--, ptr++, origptr++)
					*ptr = trans_table[*origptr][*ptr] ;
			else    for (cx = maxcx ; cx >= 0 ; cx--, ptr++, origptr--)
					*ptr = trans_table[*origptr][*ptr] ;
			line += dest->width ;
			orig += vinc ;
		}
		else for (cy = y ; cy <= maxcy ; cy++)
		{
			ptr = line, origptr = orig ;
			if (hinc > 0) for (cx = maxcx ; cx >= 0 ; cx--, ptr++, origptr++)
			{
				if (*origptr != 0) *ptr = *origptr ;
			}
			else    for (cx = maxcx ; cx >= 0 ; cx--, ptr++, origptr--)
			{
				if (*origptr != 0) *ptr = *origptr ;
			}
			line += dest->width ;
			orig += vinc ;
		}
	}
	else if (dest->depth == 16 && gr->depth == 8)
	{
		/* De 8 bits a 16 bits */
		register Uint16 * ptr16 ;
		register Uint8  * origptr ;
		Uint16 * line16 ;
		Uint8  * orig ;

		if (palette_changed) gr_refresh_palette() ;

		line16 = (Uint16*)dest->data + dest->width * y + x ;
		orig   = (Uint8*) gr->data   + offs ;

		if (flags & B_TRANSLUCENT) for (cy = y ; cy <= maxcy ; cy++)
		{
			ptr16 = line16, origptr = orig ;
			if (hinc > 0)
				for (cx = maxcx ; cx >= 0 ; cx--, ptr16++, origptr++)
				{
					if (*origptr != 0)
						*ptr16 = colorghost[colorequiv[*origptr]] 
						       + colorghost[*ptr16] ;
				}
			else    for (cx = maxcx ; cx >= 0 ; cx--, ptr16++, origptr--)
				{
					if (*origptr != 0)
						*ptr16 = colorghost[colorequiv[*origptr]] 
						       + colorghost[*ptr16] ;
				}
			line16 += dest->width ;
			orig   += vinc ;
		}
		else for (cy = y ; cy <= maxcy ; cy++)
		{
			ptr16 = line16, origptr = orig ;
			if (hinc > 0) for (cx = maxcx ; cx >= 0 ; cx--, ptr16++, origptr++)
			{
				if (*origptr != 0) 
					*ptr16 = colorequiv[*origptr] ;
			}
			else    for (cx = maxcx ; cx >= 0 ; cx--, ptr16++, origptr--)
			{
				if (*origptr != 0) 
					*ptr16 = colorequiv[*origptr] ;
			}
			line16 += dest->width ;
			orig += vinc ;
		}
	}
	else if (dest->depth == 16 && gr->depth == 16)
	{
		/* De 16 bits a 16 bits */
		register Uint16 * ptr16 ;
		register Uint16 * origptr16 ;
		Uint16 * line16, * orig16 ;

		line16 = (Uint16*)dest->data + dest->width * y + x ;
		orig16 = (Uint16*)gr->data   + offs ;

		if (flags & B_TRANSLUCENT) for (cy = y ; cy <= maxcy ; cy++)
		{
			ptr16 = line16, origptr16 = orig16 ;
			if (hinc > 0)
				for (cx = maxcx ; cx >= 0 ; cx--, ptr16++, origptr16++)
				{
					if (*origptr16 != 0)
						*ptr16 = colorghost[*origptr16] 
						       + colorghost[*ptr16] ;
				}
			else    for (cx = maxcx ; cx >= 0 ; cx--, ptr16++, origptr16--)
				{
					if (*origptr16 != 0)
						*ptr16 = colorghost[*origptr16] 
						       + colorghost[*ptr16] ;
				}
			line16 += dest->width ;
			orig16 += vinc ;
		}
		else for (cy = y ; cy <= maxcy ; cy++)
		{
			ptr16 = line16, origptr16 = orig16 ;
			if (hinc > 0) for (cx = maxcx ; cx >= 0 ; cx--, ptr16++, origptr16++)
			{
				if (*origptr16 != 0) 
					*ptr16 = *origptr16 ;
			}
			else    for (cx = maxcx ; cx >= 0 ; cx--, ptr16++, origptr16--)
			{
				if (*origptr16 != 0) 
					*ptr16 = *origptr16 ;
			}
			line16 += dest->width ;
			orig16 += vinc ;
		}
	}
	else	gr_error ("Profundidad de color no soportada\n(gr_blit)") ;
}


/* Rutinas matemáticas de punto fijo, basadas en Allegro */

#include "fmath.h"

fixed cos_table[90001] ;
int cos_table_initialized = 0 ;

void init_cos_tables()
{
	int i ;

	for (i = 0 ; i <= 90000 ; i++)
	{
		cos_table[i] = ftofix(cos (i * M_PI / 180000.0)) ;
	}
	cos_table_initialized = 1 ;
}

#define ROTATEDX(x,y,sina,cosa) (fmul(x,cosa) - fmul(y,sina))
#define ROTATEDY(x,y,sina,cosa) (fmul(x,sina) + fmul(y,cosa))

/* Cálculo de bounding box */

void gr_get_bbox (REGION * dest, REGION * clip, int x, int y, int angle, int scale, GRAPH * gr)
{
	fixed minx, miny, maxx, maxy ;
	fixed grmidx, grmidy ;
	fixed centerx, centery;
	fixed scenterx, scentery; /* Distancia del centro a esquina inf.dcha */
	fixed cosa, sina ;
	float factor ;

	while (angle < 0) angle += 360000 ;
	while (angle > 360000) angle -= 360000 ;

	if (!cos_table_initialized) init_cos_tables() ;

	/* Centro de giro */

	grmidx = itofix(gr->width / 2) ;
	grmidy = itofix(gr->height / 2) ;

	/* Centro del gráfico (para su posición en pantalla) */

	if (gr->cpoints > 0 && gr->cpoints[0].x >= 0)
	{
		centerx = itofix(gr->cpoints[0].x) ;
		centery = itofix(gr->cpoints[0].y) ;
	}
	else
	{
		centerx = grmidx ;
		centery = grmidy ;
		scenterx = grmidx ;
	}
	scenterx = itofix(gr->width) - centerx ;
	scentery = itofix(gr->height) - centery ;

	/* Averigua la "bounding box" del sprite */

	cosa = fcos (-angle) ;
	sina = fsin (-angle) ;

	if (angle > 270000)
	{
		miny = ROTATEDY(-centerx,-centery,sina,cosa) ;
		minx = ROTATEDX(-centerx,scentery,sina,cosa) ;
		maxy = ROTATEDY(scenterx,scentery,sina,cosa) ;
		maxx = ROTATEDX(scenterx,-centery,sina,cosa) ;
	}
	else if (angle > 180000)
	{
		miny = ROTATEDY(-centerx,scentery,sina,cosa) ;
		minx = ROTATEDX(scenterx,scentery,sina,cosa) ;
		maxy = ROTATEDY(scenterx,-centery,sina,cosa) ;
		maxx = ROTATEDX(-centerx,-centery,sina,cosa) ;
	}
	else if (angle > 90000)
	{
		miny = ROTATEDY(scenterx,scentery,sina,cosa) ;
		minx = ROTATEDX(scenterx,-centery,sina,cosa) ;
		maxy = ROTATEDY(-centerx,-centery,sina,cosa) ;
		maxx = ROTATEDX(-centerx,scentery,sina,cosa) ;
	}
	else
	{
		miny = ROTATEDY(scenterx,-centery,sina,cosa) ;
		minx = ROTATEDX(-centerx,-centery,sina,cosa) ;
		maxy = ROTATEDY(-centerx,scentery,sina,cosa) ;
		maxx = ROTATEDX(scenterx,scentery,sina,cosa) ;
	}

	if (scale != 100.0F)
	{
		factor = scale / 100.0F ;
		minx  = (fixed)((float)minx*factor) ;
		maxx  = (fixed)((float)maxx*factor) ;
		miny  = (fixed)((float)miny*factor) ;
		maxy  = (fixed)((float)maxy*factor) ;
		factor = 1.0F / factor ;
		cosa   = ftofix(fixtof(cosa) * factor) ;
		sina   = ftofix(fixtof(sina) * factor) ;
	}

	maxx = fixtoi(maxx);
	maxy = fixtoi(maxy);
	minx = fixtoi(minx);
	miny = fixtoi(miny);

	if (clip)
	{
		if (x+minx <  clip->x)  minx = clip->x - x ;
		if (y+miny <  clip->y)  miny = clip->y - y ;
		if (y+maxy >= clip->y2) maxy = clip->y2-y-1 ;
		if (x+maxx >= clip->x2) maxx = clip->x2-x-1 ;
	}

	dest->x2 = x+maxx;
	dest->y2 = y+maxy;
	dest->x  = x+minx;
	dest->y  = y+miny;
}

/* Rutina de rotado/escalado de sprites */

/* Datos "fixed" 16/16 */
typedef union
{
	Sint32 l ;
	Sint16 w[2] ;
}
composed ;

inline int c_div (int _a, int _b)
{
	if (!_b) return 0;
	return (65536 * (double)_a / (double)_b) ;
}


void gr_rotated_blit (GRAPH * dest, REGION * clip, int x, int y, int flags, int angle, int scale, GRAPH * gr)
{
	int cx, cy, dx, dy ;

	fixed centerx, centery ;
	fixed scenterx, scentery ;
	fixed grmidx, grmidy ;
	fixed bmpx, bmpy ;
	fixed cosa, sina ;
	int minx, miny, maxx, maxy ;
	int hinc, vinc ;
	Uint8 * basedata ;
	Uint8 * ptr, * orig, c ;
	Uint16 * base16, * ptr16, * orig16, c16 ;
	float factor ;

	composed n_rbmpx, n_rbmpy, n_bmpx, n_bmpy, n_cosa, n_sina ;
	composed _left, _right, _top, _bottom ;
	composed hinsteps, vinsteps, houtsteps, voutsteps ;
	composed hsteps_inc, vsteps_inc ;
	composed n_w, n_h ;

	while (angle < 0) angle += 360000 ;
	while (angle > 360000) angle -= 360000 ;

	if (scale < 1) return ;
	if (!cos_table_initialized) init_cos_tables() ;

	if (!dest) dest = scrbitmap ;
	if (!clip) clip = &regions[0] ;

	/* Centro de giro */

	grmidx = itofix(gr->width / 2) ;
	grmidy = itofix(gr->height / 2) ;

	/* Centro del gráfico (para su posición en pantalla) */

	if (gr->cpoints > 0 && gr->cpoints[0].x >= 0)
	{
		centerx = itofix(gr->cpoints[0].x) ;
		centery = itofix(gr->cpoints[0].y) ;
	}
	else
	{
		centerx = grmidx ;
		centery = grmidy ;
	}
	scenterx = itofix(gr->width) - centerx ;
	scentery = itofix(gr->height) - centery ;

	/* Averigua la "bounding box" del sprite */

	cosa = fcos (-angle) ;
	sina = fsin (-angle) ;

	if (angle > 270000)
	{
		miny = ROTATEDY(-centerx,-centery,sina,cosa) ;
		minx = ROTATEDX(-centerx,scentery,sina,cosa) ;
		maxy = ROTATEDY(scenterx,scentery,sina,cosa) ;
		maxx = ROTATEDX(scenterx,-centery,sina,cosa) ;
	}
	else if (angle > 180000)
	{
		miny = ROTATEDY(-centerx,scentery,sina,cosa) ;
		minx = ROTATEDX(scenterx,scentery,sina,cosa) ;
		maxy = ROTATEDY(scenterx,-centery,sina,cosa) ;
		maxx = ROTATEDX(-centerx,-centery,sina,cosa) ;
	}
	else if (angle > 90000)
	{
		miny = ROTATEDY(scenterx,scentery,sina,cosa) ;
		minx = ROTATEDX(scenterx,-centery,sina,cosa) ;
		maxy = ROTATEDY(-centerx,-centery,sina,cosa) ;
		maxx = ROTATEDX(-centerx,scentery,sina,cosa) ;
	}
	else
	{
		miny = ROTATEDY(scenterx,-centery,sina,cosa) ;
		minx = ROTATEDX(-centerx,-centery,sina,cosa) ;
		maxy = ROTATEDY(-centerx,scentery,sina,cosa) ;
		maxx = ROTATEDX(scenterx,scentery,sina,cosa) ;
	}

	/* Busca el punto ubicado en su esquina superior izda., en el GRAPH */

	bmpx = ROTATEDX(minx,miny,-sina,cosa) + centerx ;
	bmpy = ROTATEDY(minx,miny,-sina,cosa) + centery ;

	/* Escala las coordenadas si es preciso */

	if (scale != 100.0F)
	{
		factor = scale / 100.0F ;
		minx  = (fixed)((float)minx*factor) ;
		maxx  = (fixed)((float)maxx*factor) ;
		miny  = (fixed)((float)miny*factor) ;
		maxy  = (fixed)((float)maxy*factor) ;
		factor = 1.0F / factor ;
		cosa   = ftofix(fixtof(cosa) * factor) ;
		sina   = ftofix(fixtof(sina) * factor) ;
	}

	/* Ahora pasamos a coordenadas enteras */

	maxx = fixtoi(maxx);
	maxy = fixtoi(maxy);
	minx = fixtoi(minx);
	miny = fixtoi(miny);

	/* Clipping */

	if (x+minx < clip->x) 
	{
		bmpx += cosa * (clip->x-x-minx) ;
		bmpy -= sina * (clip->x-x-minx) ;
		minx  = clip->x-x ;
	}
	if (y+miny < clip->y) 
	{
		bmpx += sina * (clip->y-y-miny) ;
		bmpy += cosa * (clip->y-y-miny) ;
		miny  = clip->y-y ;
	}

	if (y+maxy >= clip->y2)
		maxy = clip->y2-y-1 ;
	if (x+maxx >= clip->x2)
		maxx = clip->x2-x-1 ;

	/* Prepara el bucle principal */

	dy = maxy-miny+1 ;
	dx = maxx-minx+1 ;

	hinc = 1 ;
	vinc = gr->width ;

	n_sina.l = (int)(fixtof(sina) * 65536.0F) ;
	n_cosa.l = (int)(fixtof(cosa) * 65536.0F) ;
	n_bmpx.l = (int)(fixtof(bmpx) * 65536.0F) ;
	n_bmpy.l = (int)(fixtof(bmpy) * 65536.0F) ;


	/* Cálculo de los pasos necesarios para que la línea
	 * entre y salga del bitmap */

	n_w.l = (gr->width  << 16) ;
	n_h.l = (gr->height << 16) ;

	_left.l   = c_div (-n_bmpx.l, n_cosa.l) ;
	_top.l    = c_div ( n_bmpy.l, n_sina.l) ;
	_right.l  = c_div (n_w.l-n_bmpx.l, n_cosa.l) ;
	_bottom.l = c_div (n_bmpy.l-n_h.l, n_sina.l) ;

	hinsteps.l  = n_cosa.l<0? _right.l  : _left.l ;
	vinsteps.l  = n_sina.l>0? _bottom.l : _top.l ;
	houtsteps.l = n_cosa.l<0? _left.l   : _right.l ;
	voutsteps.l = n_sina.l>0? _top.l    : _bottom.l ;

	hsteps_inc.l  = n_cosa.l ? c_div (-n_sina.l, n_cosa.l) : 0 ;
	vsteps_inc.l  = n_sina.l ? c_div ( n_cosa.l, n_sina.l) : 0 ;
			    
	/* Ajuste de errores */

	n_rbmpx.l = n_bmpx.l + hinsteps.w[1] * n_cosa.l ;
	if (n_rbmpx.l < 0 || n_rbmpx.w[1] >= gr->width)
		hinsteps.w[1]++ ;
	n_rbmpx.l = n_bmpx.l + houtsteps.w[1] * n_cosa.l ;
	if (n_rbmpx.l < 0 || n_rbmpx.w[1] >= gr->width)
		houtsteps.w[1]-- ;

	n_rbmpy.l = n_bmpy.l - vinsteps.w[1] * n_sina.l ;
	if (n_rbmpy.l < 0 || n_rbmpy.w[1] >= gr->height)
		vinsteps.w[1]++ ;
	n_rbmpy.l = n_bmpy.l - voutsteps.w[1] * n_sina.l ;
	if (n_rbmpy.l < 0 || n_rbmpy.w[1] >= gr->height)
		voutsteps.w[1]-- ;

	/* Casos especiales donde el ángulo es recto */

	if (n_sina.l == 0) 
	{
		vsteps_inc.l = hsteps_inc.l ;
		voutsteps.l  = houtsteps.l ;
		vinsteps.l   = hinsteps.l ;
		if (n_cosa.l > 0) dy-=2 ;
		else { 
			orig        += dest->width ;
			n_bmpx.l    += n_sina.l ;
			n_bmpy.l    += n_cosa.l ;
			hinsteps.l  += hsteps_inc.l ;
			houtsteps.l += hsteps_inc.l ;
			vinsteps.l  += vsteps_inc.l ;
			voutsteps.l += vsteps_inc.l ;
			dy-=2 ;
		}
	}
	if (n_cosa.l == 0) 
	{
		hsteps_inc.l = vsteps_inc.l ;
		houtsteps.l  = voutsteps.l ;
		hinsteps.l   = vinsteps.l ;
		if (n_sina.l > 0) dy-=2 ;
		else { 
			orig        += dest->width ;
			n_bmpx.l    += n_sina.l ;
			n_bmpy.l    += n_cosa.l ;
			hinsteps.l  += hsteps_inc.l ;
			houtsteps.l += hsteps_inc.l ;
			vinsteps.l  += vsteps_inc.l ;
			voutsteps.l += vsteps_inc.l ;
			dy-- ;
		}
	}

#define ROTATION_LOOP_START \
	for (cy = 0 ; cy <= dy ; cy++) \
	{ \
		insteps = MAX(0, MAX(vinsteps.w[1],  hinsteps.w[1])) ; \
		steps   = MIN(voutsteps.w[1], houtsteps.w[1]) - insteps ; \
		if (insteps+steps > dx) steps = dx-insteps ; \
 \
		n_rbmpx.l = n_bmpx.l + insteps * n_cosa.l ; \
		n_rbmpy.l = n_bmpy.l - insteps * n_sina.l ; \
		ptr = orig + insteps ; \
 \
		if (hinc > 0) \
		while (steps-- > 0) \
		{ \

#define ROTATION_LOOP_MIDDLE \
			n_rbmpx.l += n_cosa.l ; \
			n_rbmpy.l -= n_sina.l ; \
		} \
		else \
		while (steps-- > 0) \
		{ \

#define ROTATION_LOOP_END \
			n_rbmpx.l += n_cosa.l ; \
			n_rbmpy.l -= n_sina.l ; \
		} \
 \
		orig        += dest->width ; \
		n_bmpx.l    += n_sina.l ; \
		n_bmpy.l    += n_cosa.l ; \
		hinsteps.l  += hsteps_inc.l ; \
		houtsteps.l += hsteps_inc.l ; \
		vinsteps.l  += vsteps_inc.l ; \
		voutsteps.l += vsteps_inc.l ; \
	}
	if (dest->depth == 8 && gr->depth == 8)
	{
		/* De 8 bits a 8 bits */

		orig     = (Uint8 *)dest->data + (minx+x) + (miny+y)*dest->width ;
		basedata = (Uint8 *)gr->data ;
		if (flags & B_HMIRROR) { hinc = -1 ;    basedata += gr->width-1 ; }
		if (flags & B_VMIRROR) { vinc = -vinc ; basedata -= vinc*(gr->height-1) ; }

		if (flags & B_TRANSLUCENT) for (cy = 0 ; cy <= dy ; cy++)
		{
			n_rbmpx.l = n_bmpx.l ;
			n_rbmpy.l = n_bmpy.l ;
			ptr = orig ;

			for (cx = 0 ; cx <= dx ; cx++, ptr ++)
			{
				if (n_rbmpx.w[1] >= 0 && 
				    n_rbmpy.w[1] >= 0 && 
				    n_rbmpx.w[1] < gr->width && 
				    n_rbmpy.w[1] < gr->height)
					break ;
				n_rbmpx.l += n_cosa.l ;
				n_rbmpy.l -= n_sina.l ;
			}
			for (; cx <= dx ; cx++, ptr ++ )
			{
				if (n_rbmpx.w[1] < gr->width  && 
				    n_rbmpy.w[1] < gr->height && 
				    n_rbmpx.w[1] >= 0 && 
				    n_rbmpy.w[1] >= 0)
				{
					if (hinc > 0)
						c = basedata[vinc*n_rbmpy.w[1] + n_rbmpx.w[1]] ;
					else
						c = basedata[vinc*n_rbmpy.w[1] - n_rbmpx.w[1]] ;
					*ptr = trans_table[c][*ptr] ;
				}
				else	break ;

				n_rbmpx.l += n_cosa.l ;
				n_rbmpy.l -= n_sina.l ;
			}
			orig += dest->width ;
			n_bmpx.l += n_sina.l ;
			n_bmpy.l += n_cosa.l ;
		}
		else for (cy = 0 ; cy <= dy ; cy++)
		{
			int insteps = MAX(0, MAX(vinsteps.w[1],  hinsteps.w[1])) ;
			int steps   = MIN(voutsteps.w[1], houtsteps.w[1]) - insteps ;
			if (insteps+steps > dx) steps = dx-insteps ;

			n_rbmpx.l = n_bmpx.l + insteps * n_cosa.l ;
			n_rbmpy.l = n_bmpy.l - insteps * n_sina.l ;
			ptr = orig + insteps ;

			if (hinc > 0)
			while (steps-- > 0)
			{
				c = basedata[vinc*n_rbmpy.w[1] + n_rbmpx.w[1]] ;
				if (c) *ptr = c ; ptr++ ;
				n_rbmpx.l += n_cosa.l ;
				n_rbmpy.l -= n_sina.l ;
			}
			else
			while (steps-- > 0)
			{
				c = basedata[vinc*n_rbmpy.w[1] - n_rbmpx.w[1]] ;
				if (c) *ptr = c ; ptr++ ;
				n_rbmpx.l += n_cosa.l ;
				n_rbmpy.l -= n_sina.l ;
			}

			orig        += dest->width ;
			n_bmpx.l    += n_sina.l ;
			n_bmpy.l    += n_cosa.l ;
			hinsteps.l  += hsteps_inc.l ;
			houtsteps.l += hsteps_inc.l ;
			vinsteps.l  += vsteps_inc.l ;
			voutsteps.l += vsteps_inc.l ;
		}
	}
	else if (dest->depth == 16 && gr->depth == 8)
	{
		/* De 8 bits a 16 bits */

		orig16   = (Uint16 *)dest->data + (minx+x) + (miny+y)*dest->width ;
		basedata = (Uint8 *)gr->data ;
		if (flags & B_HMIRROR) { hinc = -1 ;    basedata += gr->width-1 ; }
		if (flags & B_VMIRROR) { vinc = -vinc ; basedata -= vinc*(gr->height-1) ; }

		if (flags & B_TRANSLUCENT) for (cy = 0 ; cy <= dy ; cy++)
		{
			n_rbmpx.l = n_bmpx.l ;
			n_rbmpy.l = n_bmpy.l ;
			ptr16 = orig16 ;

			for (cx = 0 ; cx <= dx ; cx++, ptr16 ++)
			{
				if (n_rbmpx.w[1] >= 0 && 
				    n_rbmpy.w[1] >= 0 && 
				    n_rbmpx.w[1] < gr->width && 
				    n_rbmpy.w[1] < gr->height)
					break ;
				n_rbmpx.l += n_cosa.l ;
				n_rbmpy.l -= n_sina.l ;
			}
			for (; cx <= dx ; cx++, ptr16 ++ )
			{
				if (n_rbmpx.w[1] < gr->width  && 
				    n_rbmpy.w[1] < gr->height && 
				    n_rbmpx.w[1] >= 0 && 
				    n_rbmpy.w[1] >= 0)
				{
					if (hinc > 0)
						c = basedata[vinc*n_rbmpy.w[1] + n_rbmpx.w[1]] ;
					else
						c = basedata[vinc*n_rbmpy.w[1] - n_rbmpx.w[1]] ;
					if (c != 0)
						*ptr16 = colorghost[colorequiv[c]] 
						       + colorghost[*ptr16] ;
				}
				else	break ;

				n_rbmpx.l += n_cosa.l ;
				n_rbmpy.l -= n_sina.l ;
			}
			orig16 += dest->width ;
			n_bmpx.l += n_sina.l ;
			n_bmpy.l += n_cosa.l ;
		}
		else for (cy = 0 ; cy <= dy ; cy++)
		{
			n_rbmpx.l = n_bmpx.l ;
			n_rbmpy.l = n_bmpy.l ;
			ptr16 = orig16 ;

			for (cx = 0 ; cx <= dx ; cx++, ptr16 ++)
			{
				if (n_rbmpx.w[1] >= 0 && 
				    n_rbmpy.w[1] >= 0 && 
				    n_rbmpx.w[1] < gr->width && 
				    n_rbmpy.w[1] < gr->height)
					break ;

				n_rbmpx.l += n_cosa.l ;
				n_rbmpy.l -= n_sina.l ;
			}
			for (; cx <= dx ; cx++, ptr16 ++ )
			{
				if (n_rbmpx.w[1] < gr->width  && 
				    n_rbmpy.w[1] < gr->height && 
				    n_rbmpx.w[1] >= 0 && 
				    n_rbmpy.w[1] >= 0)
				{
					if (hinc > 0)
						c = basedata[vinc*n_rbmpy.w[1] + n_rbmpx.w[1]] ;
					else
						c = basedata[vinc*n_rbmpy.w[1] - n_rbmpx.w[1]] ;
					if (c) *ptr16 = colorequiv[c] ;
				}
				else	break ;

				n_rbmpx.l += n_cosa.l ;
				n_rbmpy.l -= n_sina.l ;
			}
			orig16 += dest->width ;
			n_bmpx.l += n_sina.l ;
			n_bmpy.l += n_cosa.l ;
		}
	}
	else if (dest->depth == 16 && gr->depth == 16)
	{
		/* De 16 bits a 16 bits */

		orig16   = (Uint16 *)dest->data + (minx+x) + (miny+y)*dest->width ;
		base16   = (Uint16 *)gr->data ;
		if (flags & B_HMIRROR) { hinc = -1 ;    base16 += gr->width-1 ; }
		if (flags & B_VMIRROR) { vinc = -vinc ; base16 -= vinc*(gr->height-1) ; }

		if (flags & B_TRANSLUCENT) for (cy = 0 ; cy <= dy ; cy++)
		{
			n_rbmpx.l = n_bmpx.l ;
			n_rbmpy.l = n_bmpy.l ;
			ptr16 = orig16 ;

			for (cx = 0 ; cx <= dx ; cx++, ptr16 ++)
			{
				if (n_rbmpx.w[1] >= 0 && 
				    n_rbmpy.w[1] >= 0 && 
				    n_rbmpx.w[1] < gr->width && 
				    n_rbmpy.w[1] < gr->height)
					break ;
				n_rbmpx.l += n_cosa.l ;
				n_rbmpy.l -= n_sina.l ;
			}
			for (; cx <= dx ; cx++, ptr16 ++ )
			{
				if (n_rbmpx.w[1] < gr->width  && 
				    n_rbmpy.w[1] < gr->height && 
				    n_rbmpx.w[1] >= 0 && 
				    n_rbmpy.w[1] >= 0)
				{
					if (hinc > 0)
						c16 = base16[vinc*n_rbmpy.w[1] + n_rbmpx.w[1]] ;
					else
						c16 = base16[vinc*n_rbmpy.w[1] - n_rbmpx.w[1]] ;
					if (c16 != 0)
						*ptr16 = colorghost[c16] 
						       + colorghost[*ptr16] ;
				}
				else	break ;

				n_rbmpx.l += n_cosa.l ;
				n_rbmpy.l -= n_sina.l ;
			}
			orig16 += dest->width ;
			n_bmpx.l += n_sina.l ;
			n_bmpy.l += n_cosa.l ;
		}
		else for (cy = 0 ; cy <= dy ; cy++)
		{
			n_rbmpx.l = n_bmpx.l ;
			n_rbmpy.l = n_bmpy.l ;
			ptr16 = orig16 ;

			for (cx = 0 ; cx <= dx ; cx++, ptr16 ++)
			{
				if (n_rbmpx.w[1] >= 0 && 
				    n_rbmpy.w[1] >= 0 && 
				    n_rbmpx.w[1] < gr->width && 
				    n_rbmpy.w[1] < gr->height)
					break ;

				n_rbmpx.l += n_cosa.l ;
				n_rbmpy.l -= n_sina.l ;
			}
			for (; cx <= dx ; cx++, ptr16 ++ )
			{
				if (n_rbmpx.w[1] < gr->width  && 
				    n_rbmpy.w[1] < gr->height && 
				    n_rbmpx.w[1] >= 0 && 
				    n_rbmpy.w[1] >= 0)
				{
					if (hinc > 0)
						c16 = base16[vinc*n_rbmpy.w[1] + n_rbmpx.w[1]] ;
					else
						c16 = base16[vinc*n_rbmpy.w[1] - n_rbmpx.w[1]] ;
					if (c16) *ptr16 = c16 ;
				}
				else	break ;

				n_rbmpx.l += n_cosa.l ;
				n_rbmpy.l -= n_sina.l ;
			}
			orig16 += dest->width ;
			n_bmpx.l += n_sina.l ;
			n_bmpy.l += n_cosa.l ;
		}
	}
	else	gr_error ("Profundidad de color no soportada \n(gr_rotated_blit)") ;
}

