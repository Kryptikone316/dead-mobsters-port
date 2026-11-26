/*
 *  Fenix - Videogame compiler/interpreter
 *  Current release       : FENIX - PROJECT 1.0 - R 0.76
 *  Last stable release   :
 *  Project documentation : http://fenix.sourceforge.net
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
 * FILE        : i_func.h
 * DESCRIPTION : Implements FENIX language function handlers
 *
 * HISTORY:  0.80 - div_ prefix substituted with fxi_
 *           0.80 - Added DLL support
 *			 0.80 - Added flag param in map_block_copy
 *			 0.80 - Removed region 0 clipper from map_put/map_xput
 *           0.76 - Patched SAVE to real BYTE size on saving
 *			 0.76 - Corrected a clipper bug in MAP_BLOCK_COPY
 *			 0.76 - DRAW_AT patched to avoid XGRAPH rotation
 *			 0.75 - SET_ICON second WM fenix function to set window/taskbar icon
 *			 0.74 - SET_TITLE first WM fenix function to set window title
 *			 0.74 - WRITE_VAR, WRITE_INT, WRITE_FLOAT, WRITE_STRING
 *			 0.74 - Collision now accepts TYPE or PROCESS ID
 *			 0.73 - Corrected advance & xadvance to init cos_tables
 *			 0.73 - Added some Win32 patches due to non-portable
 *					strftime formats
 *			 0.73 - Corrected fxi_set_center
 *			 0.73 - Added fxi_memory_free & fxi_memory_total
 *			 0.72 - Corrected fxi_get_real_point
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>

#include "fxi.h"
#include "fmath.h"

/* WIN32 INCLUDES */
#ifndef LINUX
#include <windows.h>
#include <winbase.h>
#endif
/* LINUX INCLUDES */
#ifdef LINUX
#include <dlfcn.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#define KERNELC_V_1 2
#define KERNELC_V_2 3
#define KERNELC_V_3 16
#endif

#include <fnx_loadlib.h>
#include <fnc_exports.h>

#include "dcb.h"

extern int cos_table_initialized ;

/* ---------------------------------------------------------------------- */
/* Funciones del sistema                                                  */
/* ---------------------------------------------------------------------- */

/* Funciones incompatibles con DIV */

static int fxi_say (INSTANCE * my, int * params)
{
	printf ("%s\n", string_get(params[0])) ;
	gr_con_printf("%s\n", string_get(params[0])) ;
	string_discard(params[0]) ;
	return 1 ;
}

static int fxi_print (INSTANCE * my, int * params)
{
	return printf ("%d ", params[0]) ;

}

static int fxi_printf (INSTANCE * my, int * params)
{
	return printf ("%f ", *(float *)&params[0]) ;
}

/* Funciones matemáticas */

static int fxi_rand (INSTANCE * my, int * params)
{
	int num1 = params[0] ;
	int num2 = params[1] ;

	return num1 + (int)(((double)(num2-num1+1) * rand()) / (RAND_MAX+1.0)) ;
}

static int fxi_rand_seed (INSTANCE * my, int * params)
{
	srand (params[0]) ;
	return 1 ;
}

static int fxi_abs (INSTANCE * my, int * params)
{
	float num = *(float *)&params[0] ;
	float res = num < 0 ? -num:num ;
	return *(int *)&res ;
}

static int fxi_pow (INSTANCE * my, int * params)
{
	float res = (float)pow (*(float *)&params[0], *(float *)&params[1]) ;
	return *(int *)&res ;
}

static int fxi_fget_angle (INSTANCE * my, int * params)
{
	double dx = params[2] - params[0] ;
	double dy = params[3] - params[1] ;
	int angle ;

	if (dx == 0) return dy > 0 ? 270000 : 90000 ;

	angle = (int) (atan(dy / dx) * 180000.0 / M_PI) ;

	return dx > 0 ? -angle:-angle+180000 ;
}

static int fxi_fget_dist (INSTANCE * my, int * params)
{
	double dx = params[2] - params[0] ;
	double dy = params[3] - params[1] ;

	return (int)sqrt (dx*dx + dy*dy) ;
}

static int fxi_near_angle (INSTANCE * my, int * params)
{
	int angle = params[0] ;
	int dest  = params[1] ;
	int incr  = params[2] ;

	if (angle < dest && dest-angle > 180000)
		angle += 360000 ;

	if (angle > dest && angle-dest > 180000)
		angle -= 360000 ;

	if (angle < dest)
	{
		angle += incr ;
		if (angle > dest) angle = dest ;
	}
	else
	{
		angle -= incr ;
		if (angle < dest) angle = dest ;
	}

	if (angle < 0) return angle + 360000 ;
	if (angle >= 360000) return angle - 360000 ;
	return angle ;
}

static int fxi_advance (INSTANCE * my, int * params)
{
	int angle ;

	angle = LOCDWORD(my,ANGLE) ;
	if (!cos_table_initialized) init_cos_tables() ;
	LOCDWORD(my,COORDX) += fixtoi(fmul(fcos(angle), itofix(params[0]))) ;
	LOCDWORD(my,COORDY) -= fixtoi(fmul(fsin(angle), itofix(params[0]))) ;
	return 1 ;
}

static int fxi_xadvance (INSTANCE * my, int * params)
{
	int angle ;

	angle = params[0] ;
	if (!cos_table_initialized) init_cos_tables() ;
	LOCDWORD(my,COORDX) += fixtoi(fmul(fcos(angle), itofix(params[1]))) ;
	LOCDWORD(my,COORDY) -= fixtoi(fmul(fsin(angle), itofix(params[1]))) ;
	return 1 ;
}

static int fxi_sqrt (INSTANCE * my, int * params)
{
	float res = (float)sqrt(*(float *)&params[0]) ;
	return *(int *)&res ;
}

/* Funciones matemáticas con floats */

static int fxi_cos (INSTANCE * my, int * params)
{
	float param = *(float *)&params[0] ;
	float result = (float)cos((double)(param*M_PI/180000.0)) ;
	return *(int *)&result ;
}
static int fxi_sin (INSTANCE * my, int * params)
{
	float param = *(float *)&params[0] ;
	float result = (float)sin((double)(param*M_PI/180000.0)) ;
	return *(int *)&result ;
}
static int fxi_tan (INSTANCE * my, int * params)
{
	float param = *(float *)&params[0] ;
	float result = (float)tan((double)(param*M_PI/180000.0)) ;
	return *(int *)&result ;
}
static int fxi_acos (INSTANCE * my, int * params)
{
	float param = *(float *)&params[0] ;
	float result = (float)(acos((double)param)*180000.0/M_PI) ;
	return *(int *)&result ;
}
static int fxi_asin (INSTANCE * my, int * params)
{
	float param = *(float *)&params[0] ;
	float result = (float)(asin((double)param)*180000.0/M_PI) ;
	return *(int *)&result ;
}
static int fxi_atan (INSTANCE * my, int * params)
{
	float param = *(float *)&params[0] ;
	float result = (float)(atan((double)param)*180000.0/M_PI) ;
	return *(int *)&result ;
}

/* Interacción entre procesos */

static int fxi_exit (INSTANCE * my, int * params)
{
	INSTANCE * i = first_instance ;

	while (i)
	{
		LOCDWORD(i,STATUS) = STATUS_KILLED ;
		i = i->next ;
	}
	must_exit = 1 ;
	return must_exit ;
}

static int fxi_running (INSTANCE * my, int * params)
{
	INSTANCE * i = instance_get (params[0]) ;
	return i ? 1 : 0 ;
}

static int fxi_signal (INSTANCE * my, int * params)
{
	INSTANCE * i = instance_get (params[0]) ;
	int status ;
	int subcall[2] ;

	if (!params[0]) return 0 ;

	if (!i && params[0] < FIRST_INSTANCE_ID)
	{
		i = first_instance ;
		subcall[1] = params[1] ;
		while (i)
		{
			if (LOCDWORD(i,PROCESS_TYPE) == params[0])
			{
				subcall[0] = LOCDWORD(i, PROCESS_ID) ;
				fxi_signal (my, subcall) ;
			}
			i = i->next ;
		}
		return 0 ;
	}

	if (i)
	{
		switch (params[1] >= 100 ? params[1]-100:params[1])
		{
			case 0:		/* S_KILL */
				status = STATUS_KILLED ;
				LOCDWORD(i,STATUS) = status ;
				break ;

			case 1:		/* S_WAKEUP */
				status = STATUS_RUNNING ;
				LOCDWORD(i,STATUS) = status ;
				break ;

			case 2: 	/* S_SLEEP */
				status = STATUS_SLEEPING ;
				LOCDWORD(i,STATUS) = status ;
				break ;

			case 3:		/* S_FREEZE */
				status = STATUS_FROZEN ;
				LOCDWORD(i,STATUS) = status ;
				break ;

			default:
				gr_error ("Tipo de señal desconocida") ;
		}

		if (params[1] >= 100)
		{
			i = instance_getson(i) ;
			while (i)
			{
				subcall[0] = LOCDWORD(i,PROCESS_ID) ;
				subcall[1] = params[1] ;
				fxi_signal (my, subcall) ;
				i = instance_getbigbro(i)  ;
			}
		}
	}
	return 1 ;
}

static int fxi_let_me_alone (INSTANCE * my, int * params)
{
	INSTANCE * i = first_instance ;

	while (i)
	{
		if (i != my) LOCDWORD(i, STATUS) = STATUS_KILLED ;
		i = i->next ;
	}
	return 1 ;
}

static int fxi_get_angle (INSTANCE * my, int * params)
{
	INSTANCE * b = instance_get (params[0]) ;
	int subcall[4] ;

	if (my && b)
	{
		subcall[0] = LOCDWORD(my, COORDX) ;
		subcall[1] = LOCDWORD(my, COORDY) ;
		subcall[2] = LOCDWORD(b,  COORDX) ;
		subcall[3] = LOCDWORD(b,  COORDY) ;
		return fxi_fget_angle (my, subcall) ;
	}
	return -1 ;
}

static int fxi_get_distx (INSTANCE * my, int * params)
{
	double angle = params[0] * M_PI / 180000.0 ;
	return (int)(params[1] * cos(angle)) ;
}

static int fxi_get_disty (INSTANCE * my, int * params)
{
	double angle = params[0] * M_PI / 180000.0 ;
	return (int)(params[1] * -sin(angle)) ;
}

static int fxi_get_dist (INSTANCE * a, int * params)
{
	INSTANCE * b = instance_get (params[0]) ;
	int subcall[4] ;

	if (a && b)
	{
		subcall[0] = LOCDWORD(a, COORDX) ;
		subcall[1] = LOCDWORD(a, COORDY) ;
		if (LOCDWORD(a, RESOLUTION))
		{
			subcall[0] /= LOCDWORD(a,RESOLUTION) ;
			subcall[1] /= LOCDWORD(a,RESOLUTION) ;
		}

		subcall[2] = LOCDWORD(b, COORDX) ;
		subcall[3] = LOCDWORD(b, COORDY) ;
		if (LOCDWORD(b, RESOLUTION))
		{
			subcall[2] /= LOCDWORD(b,RESOLUTION) ;
			subcall[3] /= LOCDWORD(b,RESOLUTION) ;
		}

		if (LOCDWORD(a, RESOLUTION))
			return fxi_fget_dist (a, subcall) * LOCDWORD(a, RESOLUTION) ;

		return fxi_fget_dist (a, subcall) ;
	}
	return -1 ;
}

static int fxi_get_id (INSTANCE * my, int * params)
{
	INSTANCE * ptr = first_instance ;

	if (!params[0])
	{
		if (LOCDWORD(my, ID_SCAN))
		{
			ptr = instance_get (LOCDWORD(my,ID_SCAN)) ;
			if (ptr) ptr = ptr->next ;
		}
		while (ptr)
		{
			if (LOCDWORD(ptr,STATUS) >= STATUS_RUNNING)
			{
				LOCDWORD(my,ID_SCAN) = LOCDWORD(ptr, PROCESS_ID) ;
				return LOCDWORD(ptr, PROCESS_ID) ;
			}
			ptr = ptr->next ;
		}
		LOCDWORD(my,ID_SCAN) = 0 ;
		return 0 ;
	}

	if (LOCDWORD(my,TYPE_SCAN))
	{
		ptr = instance_get (LOCDWORD(my,TYPE_SCAN)) ;
		if (LOCDWORD(ptr,PROCESS_TYPE) != params[0])
			ptr = first_instance ;
		else if (ptr)
			ptr = ptr->next ;
	}
	while (ptr)
	{
		if (LOCDWORD(ptr,PROCESS_TYPE) == params[0])
		{
			if (LOCDWORD(ptr,STATUS) >= STATUS_RUNNING)
			{
				LOCDWORD(my,TYPE_SCAN) = LOCDWORD(ptr,PROCESS_ID);
				return LOCDWORD(ptr,PROCESS_ID) ;
			}
		}
		ptr = ptr->next ;
	}
	LOCDWORD(my,TYPE_SCAN) = 0 ;
	return 0 ;
}

static int fxi_set_point (INSTANCE * my, int * params)
{
	GRAPH * bmp = bitmap_get (params[0], params[1]) ;
	if (!bmp || params[2] < 0 || params[2] > 999) return -1 ;
	while (params[2] >= (bmp->flags & F_NCPOINTS))
		bitmap_add_cpoint (bmp, -1, -1) ;
	bmp->cpoints[params[2]].x = params[3] ;
	bmp->cpoints[params[2]].y = params[4] ;
	return 1 ;
}

static int fxi_set_center (INSTANCE * my, int * params)
{
	GRAPH * bmp = bitmap_get (params[0], params[1]) ;
	if (!bmp) return -1 ;
	if ((bmp->flags & F_NCPOINTS) == 0)
		bitmap_add_cpoint (bmp, params[2], params[3]) ;
	else
	{
		/* CORRECTED: NOT PARAMS[2] AS CPOINT INDEX BUT 0 */
		bmp->cpoints[0].x = params[2] ;
		bmp->cpoints[0].y = params[3] ;
	}
	return 1 ;
}

static int fxi_get_point (INSTANCE * my, int * params)
{
	GRAPH * bmp ;

	bmp = bitmap_get (params[0], params[1]) ;
	if (!bmp) return -1 ;
	if (params[2] >= (bmp->flags & F_NCPOINTS) || params[2] < 0) return -1 ;

	*(int *)params[3] = bmp->cpoints[params[2]].x ;
	*(int *)params[4] = bmp->cpoints[params[2]].y ;
	return 0 ;
}

static int fxi_get_real_point (INSTANCE * my, int * params)
{
	GRAPH * b ;
	int x, y, r, centerx, centery, px, py, rx, ry ;

	double angle ;

	if (!my) return -1 ;
	b = get_graph (my) ;
	if (!b)  return -1 ;
	if (params[0] >= (b->flags & F_NCPOINTS) || params[0] < 0) return -1 ;

	r = LOCDWORD(my,REGIONID) ;
	if (r < 0 || r > 31) r = 0 ;

	if (b->cpoints[0].x >= 0)
	{
		centerx = b->cpoints[0].x ;
		centery = b->cpoints[0].y ;
	}
	else
	{
		centerx = b->width/2 ;
		centery = b->height/2 ;
	}


	if (b->cpoints[params[0]].x >= 0) {
		if (LOCDWORD(my,FLAGS) & B_HMIRROR)
			px = centerx - b->cpoints[params[0]].x - 1 ;
		else
			px = b->cpoints[params[0]].x - centerx ;

		if (LOCDWORD(my,FLAGS) & B_VMIRROR)
			py = centery - b->cpoints[params[0]].y - 1 ;
		else
			py = b->cpoints[params[0]].y - centery ;
	}
	else	px = py = 0 ;

	if (LOCDWORD(my,GRAPHSIZE) > 0)
	{
		px *= (int)(LOCDWORD(my,GRAPHSIZE) / 100.0F) ;
		py *= (int)(LOCDWORD(my,GRAPHSIZE) / 100.0F) ;
	}

	if (LOCDWORD(my,ANGLE) != 0)
	{
		angle = LOCDWORD(my,ANGLE) * M_PI / 180000.0 ;
		rx = (int)((double)px * cos(-angle) - (double)py * sin(-angle)) ;
		ry = (int)((double)px * sin(-angle) + (double)py * cos(-angle)) ;
		px = rx ;
		py = ry ;
	}

	x = LOCDWORD(my,COORDX) ;
	y = LOCDWORD(my,COORDY) ;
	if (LOCDWORD(my,RESOLUTION))
	{
		x /= LOCDWORD(my,RESOLUTION) ;
		y /= LOCDWORD(my,RESOLUTION) ;
	}

	rx = x+px ;
	ry = y+py ;

	if (LOCDWORD(my,RESOLUTION) > 0)
	{
		rx *= LOCDWORD(my,RESOLUTION) ;
		ry *= LOCDWORD(my,RESOLUTION) ;
	}

	*(int *)params[1] = rx ;
	*(int *)params[2] = ry ;
	return 1 ;
}

/* Rutinas de utilidad local */

static void draw_at (GRAPH * dest, int x, int y, REGION * r, INSTANCE * i)
{
	GRAPH * map ;
	map = get_graph (i) ;
	if (!map) return ;
	// PATCH - XGRAPH DOES NOT ROTATE DESTINATION GRAPHIC
	if (LOCDWORD(i,ANGLE) || LOCDWORD(i,GRAPHSIZE) != 100) {
		if (LOCDWORD(i,XGRAPH) && LOCDWORD(i,GRAPHSIZE) == 100) {
			gr_blit (dest, r, x, y, LOCDWORD(i,FLAGS), map) ;
		} else {
			if (LOCDWORD(i,XGRAPH)) {
				gr_rotated_blit (dest, r, x, y, LOCDWORD(i,FLAGS), 0, LOCDWORD(i,GRAPHSIZE), map) ;
			} else {
				gr_rotated_blit (dest, r, x, y, LOCDWORD(i,FLAGS), LOCDWORD(i,ANGLE), LOCDWORD(i,GRAPHSIZE), map) ;
			}
		}
	} else {
		gr_blit (dest, r, x, y, LOCDWORD(i,FLAGS), map) ;
	}
}

static void get_pos (INSTANCE * proc, int *x, int *y)
{
	*x = LOCDWORD(proc,COORDX) ;
	*y = LOCDWORD(proc,COORDY) ;
	if (LOCDWORD(proc,RESOLUTION))
	{
		*x /= LOCDWORD(proc,RESOLUTION) ;
		*y /= LOCDWORD(proc,RESOLUTION) ;
	}

}

static int get_bbox (REGION * bbox, INSTANCE * proc)
{
	GRAPH * b ;
	int     x, y ;

	b = get_graph (proc) ;
	if (!b) return 0 ;

	get_pos (proc, &x, &y) ;

	gr_get_bbox (bbox, 0, x, y, LOCDWORD(proc,ANGLE), LOCDWORD(proc,GRAPHSIZE), b) ;
	return 1 ;
}

/* Colisiones */

static int check_collision_with_mouse (INSTANCE * proc1)
{
	REGION bbox1 ;
	int x, y, mx, my ;
	static GRAPH * bmp ;

	if (!get_bbox (&bbox1, proc1)) return 0 ;

	mx = GLODWORD(MOUSEX) ;
	my = GLODWORD(MOUSEY) ;

	if (bbox1.x > mx || bbox1.x2 < mx ||
	    bbox1.y > my || bbox1.y2 < my)
		return 0 ;

	if (!bmp) bmp = bitmap_new (0, 2, 2, enable_16bits ? 16:8) ;
	if (!bmp) return 0 ;
	memset (bmp->data, 0, 2) ;

	bbox1.x = 0 ; bbox1.x2 = 3 ;
	bbox1.y = 0 ; bbox1.y2 = 3 ;
	get_pos (proc1, &x, &y) ;
	draw_at (bmp, x-mx, y-my, &bbox1, proc1) ;
	return enable_16bits ? *(Uint16 *)bmp->data : *(Uint8 *)bmp->data ;
}

static int check_collision (INSTANCE * proc1, INSTANCE * proc2)
{
	REGION bbox1, bbox2 ;
	int x, y, w, h ;
	GRAPH * bmp, * bmp2 ;
	int depth ;

	if (!get_bbox (&bbox1, proc1)) return 0 ;
	if (!get_bbox (&bbox2, proc2)) return 0 ;
	region_union (&bbox1, &bbox2) ;
	if (region_is_empty (&bbox1)) return 0 ;

	w = bbox1.x2 - bbox1.x + 1 ;
	h = bbox1.y2 - bbox1.y + 1 ;
	bbox2.x = bbox2.y = 0 ;
	bbox2.x2 = w-1 ;
	bbox2.y2 = h-1 ;

	depth = (enable_16bits ? 16:8) ;
	bmp = bitmap_new (0, w, h, depth) ;
	bmp2 = bitmap_new (0, w, h, depth) ;
	if (!bmp || !bmp2) return 0 ;
	memset (bmp->data, 0, w*h*(depth == 16?2:1)) ;
	memset (bmp2->data, 0, w*h*(depth == 16?2:1)) ;
	get_pos (proc1, &x, &y) ;
	x -= bbox1.x ;
	y -= bbox1.y ;
	draw_at (bmp,  x, y, &bbox2, proc1) ;
	get_pos (proc2, &x, &y) ;
	x -= bbox1.x ;
	y -= bbox1.y ;
	draw_at (bmp2, x, y, &bbox2, proc2) ;

	if (depth == 16)
	{
		Uint16 * ptr, * ptr2 ;

		for (y = 0 ; y < h ; y++)
		{
			ptr  = (Uint16 *)bmp->data  + w*y ;
			ptr2 = (Uint16 *)bmp2->data + w*y ;

			for (x = 0 ; x < w ; x++, ptr++, ptr2++)
			{
				if (*ptr && *ptr2)
				{
					bitmap_destroy (bmp) ;
					bitmap_destroy (bmp2) ;
					return 1;
				}
			}
		}
	}
	else
	{
		Uint8 * ptr, * ptr2 ;

		for (y = 0 ; y < h ; y++)
		{
			ptr  = (Uint8 *)bmp->data  + w*y ;
			ptr2 = (Uint8 *)bmp2->data + w*y ;

			for (x = 0 ; x < w ; x++, ptr++, ptr2++)
			{
				if (*ptr && *ptr2)
				{
					bitmap_destroy (bmp) ;
					bitmap_destroy (bmp2) ;
					return 1;
				}
			}
		}
	}

	bitmap_destroy (bmp) ;
	bitmap_destroy (bmp2) ;
	return 0;
}

static int fxi_collision (INSTANCE * my, int * params)
{
	INSTANCE * ptr = instance_get(params[0]) ;

	if (params[0] == -1)
		return check_collision_with_mouse(my) ? 1:0 ;

	/* ADDED IN 0.74 - Checks only for a single instance */
	/*DEBUG*/

	if (params[0] >= FIRST_INSTANCE_ID && ptr)
		return check_collision(my,ptr) ;

	/* we must use full list of instances or get types from it */
	ptr = first_instance ;

	if (!params[0])
	{
		if (LOCDWORD(my, ID_SCAN))
		{
			ptr = instance_get (LOCDWORD(my, ID_SCAN)) ;
			if (ptr) ptr = ptr->next ;
		}
		while (ptr)
		{
			if (ptr == my)
			{
				ptr = ptr->next ;
				continue ;
			}
			if ((LOCDWORD(ptr,STATUS) == STATUS_RUNNING ||
			     LOCDWORD(ptr,STATUS) == STATUS_FROZEN)
				&& check_collision (my, ptr))
			{
				LOCDWORD(my, ID_SCAN) = LOCDWORD(ptr, PROCESS_ID) ;
				return LOCDWORD(ptr, PROCESS_ID) ;
			}
			ptr = ptr->next ;
		}
		LOCDWORD (my, ID_SCAN) = 0 ;
		return 0 ;
	}

	if (LOCDWORD(my,TYPE_SCAN))
	{
		ptr = instance_get (LOCDWORD(my,TYPE_SCAN)) ;
		if (ptr && LOCDWORD(ptr,PROCESS_TYPE) != params[0])
			ptr = first_instance ;
		else if (ptr) ptr = ptr->next ;
	}
	while (ptr)
	{
		if (LOCDWORD(ptr,PROCESS_TYPE) == params[0] && ptr != my)
		{
			if ((LOCDWORD(ptr,STATUS) == STATUS_RUNNING ||
			     LOCDWORD(ptr,STATUS) == STATUS_FROZEN)
				&& check_collision (my, ptr))
			{
				LOCDWORD(my, TYPE_SCAN) = LOCDWORD(ptr, PROCESS_ID) ;
				return LOCDWORD(ptr, PROCESS_ID) ;
			}
		}
		ptr = ptr->next ;
	}
	LOCDWORD(my,TYPE_SCAN) = 0 ;
	return 0 ;
}

/* Entrada/salida */

static int selected_joystick = 0 ;

static int fxi_key (INSTANCE * my, int * params)
{
	key_equiv * curr ;
	int found = 0 ;
	curr = &key_table[params[0]] ;
	if (keytab_initialized==0) keytab_init() ;
	while (curr!=NULL && found==0) {
		found = keystate[curr->sdlk_equiv] ;
		curr = curr->next ;
		}
	return found ;
}

static int fxi_get_joy_button (INSTANCE * my, int * params)
{
	return (joy_b[selected_joystick] & (1 << params[0])) ? 1 : 0 ;
}

static int fxi_get_joy_position (INSTANCE * my, int * params)
{
	return (params[0] & 1) ? joy_y[selected_joystick] : joy_x[selected_joystick] ;
}

static int fxi_select_joy (INSTANCE * my, int * params)
{
	if (params[0] >= 0 && params[0] <= 7)
		selected_joystick = params[0] ;
	return SDL_NumJoysticks() ;
}

/* Funciones de inicialización y carga */

static int
fxi_set_mode (INSTANCE * my, int * params)
{
	if (params[0] < 3200200 || params[0] > 16001400)
		gr_error ("Modo gráfico no soportado") ;

	gr_init(params[0]/10000, params[0]%10000) ;
	return 1 ;
}

static int
fxi_set_fps (INSTANCE * my, int * params)
{
	gr_set_fps (params[0], params[1]) ;
	return params[0];
}

static int
fxi_load_png (INSTANCE * my, int * params)
{
	int r = gr_load_png (string_get(params[0])) ;
	string_discard(params[0]) ;
	return r ;
}

static int
fxi_load_pcx (INSTANCE * my, int * params)
{
	int r = gr_load_pcx (string_get(params[0])) ;
	string_discard(params[0]) ;
	return r ;
}

static int
fxi_load_map (INSTANCE * my, int * params)
{
	int r = gr_load_map (string_get(params[0])) ;
	string_discard(params[0]) ;
	return r ;
}

static int fxi_load_fpg (INSTANCE * my, int * params)
{
	int r = gr_load_fpg (string_get(params[0])) ;
	string_discard(params[0]) ;
	return r ;
}

static int fxi_unload_map (INSTANCE * my, int * params)
{
	GRLIB * lib ;
	GRAPH * map ;

	if (params[0] == 0 && params[1] >= 1000)
	{
		map = bitmap_get (0, params[1]) ;
		if (map)
		{
			grlib_unload_map (syslib, params[1]) ;
			return 1 ;
		}
	}

	map = bitmap_get (params[0], params[1]) ;
	lib = grlib_get(params[0]) ;
	if (!map) return 0 ;
	grlib_unload_map (lib, params[1]) ;
	return 1 ;
}

static int fxi_unload_fpg (INSTANCE * my, int * params)
{
	grlib_destroy (params[0]) ;
	if (my && LOCDWORD(my,FILEID))
		LOCDWORD(my,FILEID) = 0 ;
	return 1 ;
}

/* Regiones */

static int fxi_define_region (INSTANCE * my, int * params)
{
	region_define (params[0], params[1], params[2], params[3], params[4]) ;
	return params[0] ;
}

static int fxi_out_region (INSTANCE * my, int * params)
{
	INSTANCE * proc = instance_get (params[0]) ;
	int region = params[1], nscroll ;
	REGION bbox ;

	if (region < 0 || region > 31 || proc == 0)
		return 0 ;

	if (!get_bbox (&bbox, proc))
		return 0 ;

	if (LOCDWORD(proc,CTYPE) == 1)
	{
		nscroll = 0 ;
		if (LOCDWORD(proc,CFLAGS))
		{
			nscroll = 1 ;
			while (!(LOCDWORD(proc,CFLAGS) & (1 << nscroll)))
				nscroll++ ;
			nscroll-- ;
		}

		scroll_region (nscroll, &bbox) ;
	}

	return region_is_out (&regions[region], &bbox) ;
}

/* Paleta de colores */

static int fxi_load_pal (INSTANCE * my, int * params)
{
	const char * palname = string_get (params[0]) ;
	int r = palname ? gr_load_pal(palname) : 0 ;
	string_discard (params[0]) ;
	return r ;
}

static int fxi_convert_palette (INSTANCE * my, int * params)
{
	GRAPH * map = bitmap_get (params[0], params[1]) ;
	int * newpal = (int *) params[2], x, y ;
	Uint8 * orig, * ptr ;
        int total_width ;

	if (!map) return -1 ;
        if (map->depth != 8)
                gr_error ("Intento de usar convert_palette con un gráfico de 16 bits") ;

        total_width = map->width ;
        orig = (Uint8 *)map->data ;
        if ((map->flags & F_ANIMATION) && map->animation)
        {
                orig = (Uint8 *)map->data - map->offset ;
                total_width = map->width * map->animation->frames ;
        }
	for (y = 0 ; y < map->height ; y++)
        {
                ptr = orig + map->pitch * y ;
		for (x = 0 ; x < total_width ; x++, ptr++)
			*ptr = newpal[*ptr] ;
        }
	return 1 ;
}

static int fxi_set_colors (INSTANCE * my, int * params)
{
	gr_set_colors (params[0], params[1], (Uint8 *)params[2]) ;
	return 1 ;
}

static int fxi_get_colors (INSTANCE * my, int * params)
{
	gr_get_colors (params[0], params[1], (Uint8 *)params[2]) ;
	return 1 ;
}

static int fxi_roll_palette (INSTANCE * my, int * params)
{
	gr_roll_palette (params[0], params[1], params[2]) ;
	return 1 ;
}

static int last_fade_dir = 1 ;

static int fxi_fade (INSTANCE * my, int * params)
{
	if (!scr_initialized) gr_init (320, 200) ;
	gr_fade_init (params[0], params[1], params[2], params[3],
                        last_fade_dir = (last_fade_dir > 0 ? -1:1)) ;
	return 1 ;
}

static int fxi_fade_on (INSTANCE * my, int * params)
{
	if (!scr_initialized) gr_init (320, 200) ;
	gr_fade_init (0, 0, 0, 16, 1) ;
        last_fade_dir = 1 ;
/*
	while (fade_on != 0)
	{
		gr_wait_frame() ;
		gr_draw_frame() ;
	}
*/
	return 1 ;
}

static int fxi_find_color (INSTANCE * my, int * params)
{
	return gr_find_nearest_color (params[0], params[1], params[2]) ;
}

static int fxi_get_rgb (INSTANCE * my, int * params)
{
        gr_get_rgb (params[0], (int *)params[1], (int *)params[2], (int *)params[3]) ;
	return 1 ;
}

static int fxi_rgb (INSTANCE * my, int * params)
{
	return enable_16bits ? gr_rgb (params[0], params[1], params[2]) :
		gr_find_nearest_color (params[0], params[1], params[2]) ;
}

extern int fade_step ;

static int fxi_fade_off (INSTANCE * my, int * params)
{
	if (!scr_initialized) gr_init (320, 200) ;
	gr_fade_init (0, 0, 0, 16, -1) ;
        last_fade_dir = -1 ;

	while (fade_on)
	{
		gr_wait_frame() ;
		gr_draw_frame() ;
	}
    return 1;
}

/* Blendops */

static int fxi_create_blendop (INSTANCE * my, int * params)
{
        return (int) blend_create() ;
}

static int fxi_blendop_apply (INSTANCE * my, int * params)
{
    GRAPH * graph = bitmap_get (params[0], params[1]) ;
    if (graph) blend_apply(graph, (Sint16 *)params[2]) ;
	return 1 ;
}

static int fxi_blendop_assign (INSTANCE * my, int * params)
{
    GRAPH * graph = bitmap_get (params[0], params[1]) ;
    if (graph) blend_assign(graph, (Sint16 *)params[2]) ;
	return 1 ;
}

static int fxi_destroy_blendop (INSTANCE * my, int * params)
{
    blend_free((Sint16 *)params[0]) ;
	return 1 ;
}

static int fxi_blendop_identity (INSTANCE * my, int * params)
{
    blend_init ((Sint16 *)params[0]) ;
	return 1 ;
}

static int fxi_blendop_translucency (INSTANCE * my, int * params)
{
    blend_translucency ((Sint16 *)params[0], *(float *)(&params[1])) ;
	return 1 ;
}

static int fxi_blendop_intensity (INSTANCE * my, int * params)
{
    blend_intensity ((Sint16 *)params[0], *(float *)(&params[1])) ;
	return 1 ;
}

static int fxi_blendop_swap (INSTANCE * my, int * params)
{
    blend_swap ((Sint16 *)params[0]) ;
	return 1 ;
}

static int fxi_blendop_tint (INSTANCE * my, int * params)
{
    blend_tint ((Sint16 *)params[0], *(float *)(&params[1]), params[2], params[3], params[4]) ;
	return 1 ;
}

/* Primitivas */

static GRAPH * drawing_graph ;
extern int background_is_black ;

static int fxi_drawing_map (INSTANCE * my, int * params)
{
	if (!scr_initialized) gr_init (320, 200) ;
	if (params[0] == 0 && params[1] == 0)
	{
		drawing_graph = background_8bits_used ? background_8bits : background ;
		background_is_black = 0 ;
	}
	else
		drawing_graph = bitmap_get (params[0], params[1]) ;
	return 1 ;
}

static int fxi_drawing_color (INSTANCE * my, int * params)
{
	gr_setcolor (params[0]) ;
	return 1 ;
}

static int fxi_box (INSTANCE * my, int * params)
{
	if (!drawing_graph) gr_error ("BOX llamada antes que DRAWING_MAP") ;
	gr_box (drawing_graph, 0, params[0], params[1], params[2]-params[0], params[3]-params[1]) ;
	return 1 ;
}

static int fxi_rect (INSTANCE * my, int * params)
{
	if (!drawing_graph) gr_error ("RECT llamada antes que DRAWING_MAP") ;
	gr_rectangle (drawing_graph, 0, params[0], params[1], params[2]-params[0], params[3]-params[1]) ;
	return 1 ;
}

static int fxi_line (INSTANCE * my, int * params)
{
	if (!drawing_graph) gr_error ("LINE llamada antes que DRAWING_MAP") ;
	gr_line (drawing_graph, 0, params[0], params[1], params[2]-params[0], params[3]-params[1]) ;
	return 1 ;
}

static int fxi_circle (INSTANCE * my, int * params)
{
	if (!drawing_graph) gr_error ("CIRCLE llamada antes que DRAWING_MAP") ;
	gr_circle (drawing_graph, 0, params[0], params[1], params[2]) ;
	return 1 ;
}

static int fxi_fcircle (INSTANCE * my, int * params)
{
	if (!drawing_graph) gr_error ("FCIRCLE llamada antes que DRAWING_MAP") ;
	gr_fcircle (drawing_graph, 0, params[0], params[1], params[2]) ;
	return 1 ;
}

/* Funciones gráficas */

static int fxi_get_pixel (INSTANCE * my, int * params)
{
	if (!scr_initialized) gr_init (320, 200) ;

	if (enable_16bits && background_8bits_used)
		return gr_get_pixel (background_8bits, params[0], params[1]) ;
	else
		return gr_get_pixel (background, params[0], params[1]) ;
}

static int fxi_graphic_set (INSTANCE * my, int * params)
{
	GRAPH * map ;

	map = bitmap_get (params[0], params[1]) ;
	if (!map) return 0 ;

	switch (params[2])
	{
		case 2:		/* g_center_x */
			if ((map->flags & F_NCPOINTS) > 0)
				map->cpoints[0].x = params[3] ;
			else
				bitmap_add_cpoint
					(map, params[3], map->height/2) ;
			return 1 ;

		case 3:		/* g_center_y */
			if ((map->flags & F_NCPOINTS) > 0)
				map->cpoints[0].y = params[3] ;
			else
				bitmap_add_cpoint
					(map, map->width/2, params[3]) ;
			return 1 ;

		case 8:		/* animation step */
                        if (map->animation)
                                bitmap_animate_to (map, params[3], map->animation->speed) ;
			return 1 ;

		case 9:		/* animation speed */
                        if (map->animation)
                                bitmap_animate_to (map, map->animation->pos, params[3]) ;
			return 1 ;
	}
	return 1 ;
}

static int fxi_graphic_info (INSTANCE * my, int * params)
{
	GRAPH * map ;

	if (params[0] || params[1])
		map = bitmap_get (params[0], params[1]) ;
	else if (enable_16bits && background_8bits_used)
		map = background_8bits ;
	else
		map = background ;

	if (!map) return 0 ;

	switch (params[2])
	{
		case 0:		/* g_wide */
			return map->width ;

		case 1:		/* g_height */
			return map->height ;

		case 2:		/* g_center_x */
			if ((map->flags & F_NCPOINTS) > 0)
				if (map->cpoints[0].x >= 0)
					return map->cpoints[0].x ;
			return map->width/2 ;

		case 3:		/* g_center_y */
			if ((map->flags & F_NCPOINTS) > 0)
				if (map->cpoints[0].y >= 0)
					return map->cpoints[0].y ;
			return map->height/2 ;

		case 4:		/* g_pitch */
			return map->pitch ;

		case 5:		/* g_depth */
			return map->depth ;

		case 6:		/* frames */
			return map->animation ? map->animation->frames : 1 ;

		case 7:		/* animation steps */
			return map->animation ? map->animation->length : 0 ;

		case 8:		/* animation step */
			return map->animation ? map->animation->pos : 0 ;

		case 9:		/* animation speed */
			return map->animation ? map->animation->speed : 0 ;
	}
	return 1 ;
}

static int fxi_put (INSTANCE * my, int * params)
{
	GRAPH * map = bitmap_get (params[0], params[1]) ;
	if (!map) return 0 ;
	if (!scr_initialized) gr_init (320, 200) ;

	if (enable_16bits && (background_is_black || background_8bits_used)
			&& map->depth == 8)
	{
		background_8bits_used = 1 ;
		gr_blit (background_8bits, 0, params[2], params[3], 0, map) ;
	}
	else
		gr_blit (background, 0, params[2], params[3], 0, map) ;

	background_is_black = 0 ;
	return 1 ;
}

static int fxi_xput (INSTANCE * my, int * params)
{
	GRAPH * map = bitmap_get (params[0], params[1]) ;
	int r ;

	if (!map) return 0 ;
	if (!scr_initialized) gr_init (320, 200) ;
	r = params[7] ;
	if (r < 0 || r > 31) r = 0 ;

	background_is_black = 0 ;

	if (enable_16bits && background_8bits_used)
	{
		gr_blit (background, 0, 0, 0, 0, background_8bits);
		background_8bits_used = 0;
	}

	if (params[4] == 0 && params[5] == 100)
	{
		gr_blit (background, &regions[r],
				params[2], params[3], params[6], map) ;
		return 0 ;
	}

	gr_rotated_blit (background, &regions[r], params[2], params[3],
			 params[6], params[4], params[5], map) ;
	return 0 ;
}

static int fxi_put_screen (INSTANCE * my, int * params)
{
	GRAPH * map = bitmap_get (params[0], params[1]) ;
	if (!map) gr_error ("Mapa %d no disponible en el fichero %d", params[1], params[0]) ;
	if (!scr_initialized) gr_init (320, 200) ;

	background_is_black = 0 ;
	if (enable_16bits && map->depth == 8)
	{
		background_8bits_used = 1 ;
		gr_clear (background_8bits) ;
		gr_blit (background_8bits, 0, background->width/2, background->height/2, 0, map) ;
	}
	else
	{
		gr_clear (background) ;
		gr_blit (background, 0, background->width/2, background->height/2, 0, map) ;
	}
	return 1 ;
}

static int fxi_put_pixel (INSTANCE * my, int * params)
{
	if (!scr_initialized) gr_init (320, 200) ;

	background_is_black = 0 ;
	if (enable_16bits && background_8bits_used)
		gr_put_pixel (background_8bits, params[0], params[1], params[2]) ;
	else
		gr_put_pixel (background, params[0], params[1], params[2]) ;
	return 1 ;
}

static int fxi_clear_screen (INSTANCE * my, int * params)
{
	if (!scr_initialized) gr_init (320, 200) ;
	gr_clear (background) ;

	if (enable_16bits && background_8bits_used)
	{
		gr_clear (background_8bits) ;
		background_8bits_used = 0 ;
	}
	background_is_black = 1 ;
	return 1 ;
}

static int fxi_map_buffer (INSTANCE * my, int * params)
{
	GRAPH * map ;

	if (params[0] || params[1])
		map = bitmap_get (params[0], params[1]) ;
	else
	{
		background_is_black = 0 ;
		if (enable_16bits && background_8bits_used)
			map = background_8bits ;
		else
			map = background ;
	}

	return map ? (int)map->data : 0 ;
}

static int fxi_map_clear (INSTANCE * my, int * params)
{
	GRAPH *map = bitmap_get (params[0], params[1]) ;
	if (map) gr_clear_as (map, params[2]) ;
	return 1 ;
}

static int fxi_new_map (INSTANCE * my, int * params)
{
	GRAPH * map ;
	if (!scr_initialized) gr_init (320, 200) ;
	map = bitmap_new_syslib (params[0], params[1], params[2]) ;
	return map ? map->code : 0 ;
}

static int fxi_map_clone (INSTANCE * my, int * params)
{
	GRAPH * origin, * map = NULL ;
	if (!scr_initialized) gr_init (320, 200) ;
        origin = bitmap_get (params[0], params[1]) ;
	if (origin) map = bitmap_clone (origin) ;
	return map ? map->code : 0 ;
}

static int fxi_map_get_pixel (INSTANCE * my, int * params)
{
	GRAPH * map = bitmap_get (params[0], params[1]) ;
	if (map) return gr_get_pixel (map, params[2], params[3]) ;
	return 1 ;
}

static int fxi_map_put (INSTANCE * my, int * params)
{
	GRAPH * dest = bitmap_get (params[0], params[1]) ;
	GRAPH * orig = bitmap_get (params[0], params[2]) ;
	
	REGION clip ;
	int centerx, centery ;
	int x, y, w, h, dx, dy ;

	if (!dest) gr_error ("map_put: mapa de destino no válido") ;
	if (!orig) gr_error ("map_put: mapa de origen no válido") ;

	//Patched a new clipper to avoid 0 region clipping
	
	centery = orig->height/2 ;
	centerx = orig->width/2 ;
	if ((orig->flags & F_NCPOINTS) > 0)
	{
		if (orig->cpoints[0].x >= 0)
			centerx = orig->cpoints[0].x ;
		if (orig->cpoints[0].y >= 0)
			centery = orig->cpoints[0].y ;
	}

	x  = 0 ;
	y  = 0 ;
	w  = dest->width ;
	h  = dest->height ;
	dx = params[3] - centerx ;
	dy = params[4] - centery ;

	if (dx < 0) {
		x += dx ;
		w += dx ;
		dx = 0 ;
	}
	if (dy < 0) {
		y += dy ;
		h += dy ;
		dy = 0 ;
	}

	if (x + w > orig->width)
		w = orig->width - x ;

	if (y + h > orig->height)
		h = orig->height - y ;

	if (dx + w > dest->width)
		w = dest->width - dx ;

	if (dy + h > dest->height)
		h = dest->height - dy ;

	if (x  >= orig->width ||  y >= orig->height ||
	    dx >= dest->width || dy >= dest->height || w <= 0 || h <= 0)
		return 0 ;

	clip.x = dx ;
	clip.y = dy ;
	clip.x2 = dx + w - 1 ;
	clip.y2 = dy + h - 1 ;

	gr_blit (dest, &clip, params[3], params[4], 0, orig) ;
	return 1 ;
}

static int fxi_map_xput (INSTANCE * my, int * params)
{
	GRAPH * dest = bitmap_get (params[0], params[1]) ;
	GRAPH * orig = bitmap_get (params[0], params[2]) ;
	
	REGION clip ;
	int centerx, centery ;
	int x, y, w, h, dx, dy ;

	if (!dest || !orig) return 0 ;
	
	//Patched a new clipper to avoid 0 region clipping
	
	centery = orig->height/2 ;
	centerx = orig->width/2 ;
	if ((orig->flags & F_NCPOINTS) > 0)
	{
		if (orig->cpoints[0].x >= 0)
			centerx = orig->cpoints[0].x ;
		if (orig->cpoints[0].y >= 0)
			centery = orig->cpoints[0].y ;
	}

	x  = 0 ;
	y  = 0 ;
	w  = dest->width ;
	h  = dest->height ;
	dx = params[3] - centerx ;
	dy = params[4] - centery ;

	if (dx < 0) {
		x += dx ;
		w += dx ;
		dx = 0 ;
	}
	if (dy < 0) {
		y += dy ;
		h += dy ;
		dy = 0 ;
	}

	if (x + w > orig->width)
		w = orig->width - x ;

	if (y + h > orig->height)
		h = orig->height - y ;

	if (dx + w > dest->width)
		w = dest->width - dx ;

	if (dy + h > dest->height)
		h = dest->height - dy ;

	if (x  >= orig->width ||  y >= orig->height ||
	    dx >= dest->width || dy >= dest->height || w <= 0 || h <= 0)
		return 0 ;

	clip.x = dx ;
	clip.y = dy ;
	clip.x2 = dx + w - 1 ;
	clip.y2 = dy + h - 1 ;

	gr_rotated_blit (dest, &clip, params[3], params[4], params[7],
			 params[5], params[6], orig) ;
	return 1 ;
}

static int fxi_map_put_pixel (INSTANCE * my, int * params)
{
	GRAPH * map = bitmap_get (params[0], params[1]) ;
	if (map) gr_put_pixel (map, params[2], params[3], params[4]) ;
	else     gr_error ("Mapa %d no disponible en el fichero %d", params[1], params[0]) ;
	return 1 ;
}

static int fxi_map_block_copy (INSTANCE * my, int * params)
{
	GRAPH * dest = bitmap_get (params[0], params[1]) ;
	GRAPH * orig = bitmap_get (params[0], params[4]) ;
	REGION clip ;
	int centerx, centery, flag ;
	int x, y, w, h, dx, dy ;

	if (!dest)
		gr_error ("Mapa %d no disponible en el fichero %d",
				params[1], params[0]) ;

	if (!orig)
        {
                if (!params[4])
                {
                        orig = background ;
                        if (enable_16bits && background_8bits_used)
                                orig = background_8bits ;
                }
                else
                        gr_error ("Mapa %d no disponible en el fichero %d",
                                        params[4], params[0]) ;
        }

	centery = orig->height/2 ;
	centerx = orig->width/2 ;
	if ((orig->flags & F_NCPOINTS) > 0)
	{
		if (orig->cpoints[0].x >= 0)
			centerx = orig->cpoints[0].x ;
		if (orig->cpoints[0].y >= 0)
			centery = orig->cpoints[0].y ;
	}

	x  = params[5] ;
	y  = params[6] ;
	w  = params[7] ;
	h  = params[8] ;
	dx = params[2] ;
	dy = params[3] ;
    flag = params[9] ;
	// Added flag to blitting
	if (flag!=0) 
		flag = B_NOCOLORKEY ;

	if (x < 0)
	{
		dx += x ;
		w  += x ;
		x   = 0 ;
	}
	if (y < 0)
	{
		dy += y ;
		h  += y ;
		y   = 0 ;
	}
	if (dx < 0)
	{
		x += dx ;
		w += dx ;
		dx = 0 ;
	}
	if (dy < 0)
	{
		y += dy ;
		h += dy ;
		dy = 0 ;
	}

	if (x + w > orig->width)
		w = orig->width - x ;

	if (y + h > orig->height)
		h = orig->height - y ;

	if (dx + w > dest->width)
		w = dest->width - dx ;

	if (dy + h > dest->height)
		h = dest->height - dy ;

	if (x  >= orig->width ||  y >= orig->height ||
	    dx >= dest->width || dy >= dest->height || w <= 0 || h <= 0)
		return 0 ;

	clip.x = dx ;
	clip.y = dy ;
	clip.x2 = dx + w - 1 ;
	clip.y2 = dy + h - 1 ;
	gr_blit (dest, &clip, dx-x+centerx, dy-y+centery, flag, orig) ;
	return 1 ;
}

/* Funciones de búsqueda de caminos */

static int fxi_path_find (INSTANCE * my, int * params)
{
	GRAPH * dest = bitmap_get (params[0], params[1]) ;
	if (!dest) gr_error ("Mapa %d-%d no existe%d", params[0], params[1]) ;
	return path_find (dest, params[2], params[3], params[4], params[5], params[6]) ;
}

static int fxi_path_getxy (INSTANCE * my, int * params)
{
	return path_get ((int *)params[0], (int *)params[1]) ;
}

static int fxi_path_wall (INSTANCE * my, int * params)
{
	return path_set_wall (params[0]) ;
}

/* Funciones de visualziación de textos */

static int fxi_load_fnt (INSTANCE * my, int * params)
{
	char * text ;
	int r ;

	if (!scr_initialized) gr_init (320, 200) ;

	text = (char *)string_get (params[0]) ;
	r = text ? gr_font_load (text) : 0 ;
	string_discard (params[0]) ;
	return r ;
}

static int fxi_unload_fnt (INSTANCE * my, int * params)
{
	gr_font_destroy (params[0]) ;
	return 1 ;
}

static int fxi_write (INSTANCE * my, int * params)
{
	const char * text = string_get (params[4]) ;
	int r = text ? gr_text_new (params[0], params[1], params[2], params[3], text) : 0 ;
	string_discard (params[4]) ;
	return r ;
}

static int fxi_write_in_map (INSTANCE * my, int * params)
{
	const char * text = string_get (params[1]) ;
	GRAPH * gr ;

	if (!text) return 0 ;
	gr = gr_text_bitmap (params[0], text, params[2]) ;
	string_discard (params[1]) ;
	if (!gr) return 0 ;
	return gr->code ;
}

/*
 *  FUNCTION : fxi_write_var
 *
 *  Creates a new text associated with a variable
 *
 *  FENIX LANG PARAMS:
 *      params[0] : fnt to use
 *      params[1] : X
 *      params[2] : Y
 *      params[3] : align
 *      params[4] : pointer
 *      params[5] : type
 *
 *  FENIX RETURN VALUE:
 *     Text ID for the newly created text
 *
 */

static int fxi_write_var (INSTANCE * my, int * params)
{
	gr_text_new_var (params[0], params[1], params[2], params[3], (void *)params[4], params[5]) ;
	return 1 ;
}

/*
 *  Same as fxi_write_var, but param[5] not given and always set to VAR_STRING
 */

static int fxi_write_string (INSTANCE * my, int * params)
{
	gr_text_new_var (params[0], params[1], params[2], params[3], (void *)params[4], 2) ;
	return 1 ;
}

/*
 *  Same as fxi_write_var, but param[5] not given and always set to VAR_INT
 */

static int fxi_write_int (INSTANCE * my, int * params)
{
	gr_text_new_var (params[0], params[1], params[2], params[3], (void *)params[4], 3) ;
	return 1 ;
}

/*
 *  Same as fxi_write_var, but param[5] not given and always set to VAR_FLOAT
 */

static int fxi_write_float (INSTANCE * my, int * params)
{
	gr_text_new_var (params[0], params[1], params[2], params[3], (void *)params[4], 4) ;
	return 1 ;
}

static int fxi_move_text (INSTANCE * my, int * params)
{
	gr_text_move (params[0], params[1], params[2]) ;
	return 1 ;
}

static int fxi_delete_text (INSTANCE * my, int * params)
{
	gr_text_destroy (params[0]) ;
	return 1 ;
}

static int fxi_text_height (INSTANCE * my, int * params)
{
	const char * str = string_get (params[1]) ;
	int result = gr_text_height (params[0], str) ;
	string_discard (params[1]) ;
	return result ;
}

static int fxi_text_width (INSTANCE * my, int * params)
{
	const char * str = string_get (params[1]) ;
	int result = gr_text_width (params[0], str) ;
	string_discard (params[1]) ;
	return result ;
}

/* Funciones de acceso a ficheros */

static int fxi_save (INSTANCE * my, int * params)
{
	file * fp ;
	const char * filename ;
	int result ;

	filename = string_get (params[0]) ;
	if (!filename) return 0 ;

	fp = file_open (filename, "wb0") ;
	if (fp)
	{
		result = file_write (fp, (void *)params[1], params[2]) ;
		file_close (fp) ;
	}
	string_discard (params[0]) ;
	return result ;
}

static int fxi_load (INSTANCE * my, int * params)
{
	file * fp ;
	const char * filename ;
	int result ;

	filename = string_get (params[0]) ;
	if (!filename) return 0 ;

	fp = file_open (filename, "rb0") ;
	if (!fp) return 0 ;

	result = file_read (fp, (void *)params[1], file_size(fp)) ;
	file_close (fp) ;
	string_discard(params[0]) ;

	return result ;
}

static int fxi_fopen (INSTANCE * my, int * params)
{
        static char * ops[] = { "rb0", "w+b0", "wb0", "rb", "wb6" } ;
        int r ;

        if (params[1] < 0 || params[1] > 4)
                params[0] = 0 ;

	r = (int) file_open (string_get(params[0]), ops[params[1]]) ;
	string_discard (params[0]) ;
	return r ;
}

static int fxi_fclose (INSTANCE * my, int * params)
{
	file_close ((file *)params[0]) ;
	return 1 ;
}

static int fxi_fread (INSTANCE * my, int * params)
{
	return file_read ((file *)params[0], (void *)params[1], params[2]) ;
}

static int fxi_fwrite (INSTANCE * my, int * params)
{
	return file_write ((file *)params[0], (void *)params[1], params[2]) ;
}

static int fxi_fseek (INSTANCE * my, int * params)
{
	return file_seek ((file *)params[0], params[1], params[2]) ;
}

static int fxi_ftell (INSTANCE * my, int * params)
{
	return file_pos ((file *)params[0]) ;
}

static int fxi_filelength (INSTANCE * my, int * params)
{
	return file_size ((file *)params[0]) ;
}

static int fxi_fputs (INSTANCE * my, int * params)
{
	int r = file_puts ((file *)params[0], string_get(params[1])) ;
	string_discard(params[1]) ;
	return r ;
}

static int fxi_file (INSTANCE * my, int * params)
{
	char buffer[1030], *ptr = buffer ;
        int str = string_new("") ;
        file * f ;

        f = file_open (string_get(params[0]), "rb") ;
        if (!f) return 0 ;
        string_discard(params[0]) ;

	while (!file_eof(f))
	{
		if (!file_read (f, ptr, 1))
                        break ;
                ptr++ ;
                if (ptr == buffer+1024)
                {
                        *ptr = 0 ;
                        string_concat (str, buffer) ;
                        ptr = buffer ;
                }
	}
        *ptr = 0 ;
        string_concat (str, buffer) ;
	string_use (str) ;
        file_close (f) ;
	return str ;
}

static int fxi_fgets (INSTANCE * my, int * params)
{
	char buffer[1030] ;
	int str, str2 = 0, str3 ;
	int len, sigue ;

	for (;;)
	{
		file_gets ((file *)params[0], buffer, 1024) ;
		len = strlen(buffer) ;
		if (len > 1 && buffer[len-1] == '\n' && buffer[len-2] == '\\')
		{
			buffer[len-2] = 0 ;
			sigue = 1 ;
		}
		else	sigue = 0 ;

		str = string_new (buffer) ;
		if (str2)
		{
			str3 = string_add (str2, str) ;
			string_discard (str) ;
			string_discard (str2) ;
			str2 = str3 ;
		}
		else	str2 = str ;

		if (!sigue) break ;
	}
	string_use (str2) ;
	return str2 ;
}

static int fxi_feof (INSTANCE * my, int * params)
{
	return file_eof ((file *)params[0]) ;
}
/* Sonido */



/*
 *  FUNCTION : fxi_load_mod
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

static int fxi_load_mod (INSTANCE * my, int * params) {

	int var;

	const char * filename ;

	filename = string_get (params[0]) ;
	if (!filename) return (-1) ;

	var=(load_mod(filename));
	string_discard(params[0]);
	return (var);
}


/*
 *  FUNCTION : fxi_play_mod
 *
 *  Play a MOD
 *
 *  PARAMS:
 *      mod id;
 *      number of loops (-1 infinite loops)
 *
 *  RETURN VALUE:
 *
 *	-1 if there is any error
 *  0 if all goes ok
 */


static int fxi_play_mod (INSTANCE * my, int * params)
{
	if (params[0]<0) return (-1);
	return(play_mod(params[0],params[1]));

}


/*
 *  FUNCTION : fxi_unload_mod
 *
 *  Frees the resources from a MOD and unloads it
 *
 *  PARAMS:
 *      mod id;
 *
 *  RETURN VALUE:
 *
 *	-1 if there is any error
 *  0 if all goes ok
 */


static int fxi_unload_mod (INSTANCE * my, int * params)
{
	   if (params[0]<0) return (-1);
       return(unload_mod(params[0]));

}



/*
 *  FUNCTION : fxi_stop_mod
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

int fxi_stop_mod (INSTANCE * my, int * params)
{
	return(stop_mod());
}



/*
 *  FUNCTION : fxi_pause_mod
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


int fxi_pause_mod (INSTANCE * my, int * params)
{
	return(pause_mod());
}


/*
 *  FUNCTION : fxi_resume_mod
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


int fxi_resume_mod (INSTANCE * my, int * params)
{
	return(resume_mod());
}



/*
 *  FUNCTION : fxi_is_playing_mod
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


static int fxi_is_playing_mod(INSTANCE * my, int * params) {
    return (is_playing_mod());
}


/*
 *  FUNCTION : fxi_set_mod_volume
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


static int fxi_set_mod_volume (INSTANCE * my, int * params)
{
	return (set_mod_volume(params[0]));
}




/*
 *  FUNCTION : fxi_load_wav
 *
 *  Load a WAV from a file
 *
 *  PARAMS:
 *      file name
 *
 *  RETURN VALUE:
 *
 *		wav id
 *
 */




static int fxi_load_wav (INSTANCE * my, int * params)
{
	int var;

	const char * filename ;

	filename = string_get (params[0]) ;
	if (!filename) return (-1) ;

	var=(load_wav(filename));
	string_discard(params[0]);
	return (var);

}


/*
 *  FUNCTION : fxi_play_mod
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


static int fxi_play_wav (INSTANCE * my, int * params)
{
	return(play_wav(params[0],params[1]));
}



/*
 *  FUNCTION : fxi_unload_wav
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


static int fxi_unload_wav (INSTANCE * my, int * params)
{
	return(unload_wav(params[0]));
}


/*
 *  FUNCTION : fxi_stop_wav
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



static int fxi_stop_wav (INSTANCE * my, int * params)
{
	return(stop_wav(params[0]));
}



/*
 *  FUNCTION : fxi_pause_wav
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


static int fxi_pause_wav (INSTANCE * my, int * params)
{
	return (pause_wav(params[0]));
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

static int fxi_resume_wav (INSTANCE * my, int * params)
{
	return (resume_wav(params[0]));
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


static int fxi_is_playing_wav(INSTANCE * my, int * params) {
    return (is_playing_wav(params[0]));
}




/*
 *  FUNCTION : fxi_set_wav_volume
 *
 *  Set the volume for a wav playing (0-128)
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


static int fxi_set_wav_volume(INSTANCE * my, int * params) {
	return(set_wav_volume(params[0],params[1]));
}

/* Sonido de CD */

static int fxi_play_cd (INSTANCE * my, int * params)
{
	cd_play (params[0], params[1]) ;
	return 1 ;
}

static int fxi_is_playing_cd (INSTANCE * my, int * params)
{
	return cd_playing() ;
}

static int fxi_stop_cd (INSTANCE * my, int * params)
{
	cd_stop () ;
	return 1 ;
}

/* Mode 7 */

static int fxi_start_mode7 (INSTANCE * my, int * params)
{
	gr_mode7_start (params[0], params[1], params[2], params[3], params[4], params[5]) ;
	return 1 ;
}

static int fxi_stop_mode7 (INSTANCE * my, int * params)
{
	gr_mode7_stop (params[0]) ;
	return 1 ;
}

/* Scroll */

static int fxi_start_scroll (INSTANCE * my, int * params)
{
	gr_scroll_start (params[0], params[1], params[2], params[3], params[4], params[5]) ;
	return 1 ;
}

static int fxi_stop_scroll (INSTANCE * my, int * params)
{
	gr_scroll_stop (params[0]) ;
	return 1 ;
}

static int fxi_move_scroll (INSTANCE * my, int * params)
{
	gr_scroll_draw (params[0], 0) ;
	return 1 ;
}

/* Reproducción de FLI */

FLIC * current_fli = 0 ;
int    current_fli_x = 0 ;
int    current_fli_y = 0 ;

static int fxi_start_fli (INSTANCE * my, int * params)
{
	const char * str = string_get(params[0]) ;

	if (!str) return 0 ;
	if (current_fli) flic_destroy (current_fli) ;
	current_fli = flic_open (str) ;
	current_fli_x = params[1] ;
	current_fli_y = params[2] ;
	string_discard (params[0]) ;
	if (current_fli) return current_fli->header.frames ;
	return 1 ;
}

static int fxi_reset_fli (INSTANCE * my, int * params)
{
	if (current_fli) flic_reset (current_fli) ;
	return 1 ;
}

static int fxi_end_fli (INSTANCE * my, int * params)
{
	if (current_fli)
	{
		flic_destroy (current_fli) ;
		current_fli = 0 ;
	}
	return 1 ;
}

static int fxi_frame_fli (INSTANCE * my, int * params)
{
	return current_fli ? current_fli->current_frame != current_fli->header.frames-1 : 0 ;
}

/* Cadenas */

static int fxi_strlen (INSTANCE * my, int * params)
{
	const char * str = string_get(params[0]) ;
	int r = str ? strlen(str) : 0 ;
	string_discard (params[0]) ;
	return r ;
}

static int fxi_strupper (INSTANCE * my, int * params)
{
	int r = string_ucase(params[0]) ;
	string_discard (params[0]) ;
	string_use     (r) ;
	return r ;
}

static int fxi_strlower (INSTANCE * my, int * params)
{
	int r = string_lcase(params[0]) ;
	string_discard (params[0]) ;
	string_use     (r) ;
	return r ;
}

static int fxi_substr (INSTANCE * my, int * params)
{
	int r = string_substr(params[0],params[1],params[2]) ;
	string_discard (params[0]) ;
	string_use     (r) ;
	return r ;
}

static int fxi_strfind (INSTANCE * my, int * params)
{
	int r = string_find(params[0],params[1]) ;
	string_discard (params[0]) ;
	string_discard (params[1]) ;
	return r ;
}

static int fxi_itos (INSTANCE * my, int * params)
{
	int r = string_itoa (params[0]) ;
	string_use(r) ;
	return r ;
}

static int fxi_ftos (INSTANCE * my, int * params)
{
	int r = string_ftoa (*(float *)&params[0]) ;
	string_use(r) ;
	return r ;
}

static int fxi_stoi (INSTANCE * my, int * params)
{
	const char * str = string_get(params[0]) ;
	int r = str ? atoi(str) : 0 ;
	string_discard (params[0]) ;
	return r ;
}

static int fxi_stof (INSTANCE * my, int * params)
{
	const char * str = string_get(params[0]) ;
	float res = (float)(str ? atof(str) : 0 );
	string_discard (params[0]) ;
	return *(Sint32 *)&res ;
}

static int fxi_asc (INSTANCE * my, int * params)
{
	const Uint8 * str = string_get(params[0]) ;
	int r = str ? *str : 0 ;
	string_discard (params[0]) ;
	return r ;
}

static int fxi_chr (INSTANCE * my, int * params)
{
	char buffer[2] = " " ; int r ;
	buffer[0] = params[0] ;
	r = string_new (buffer) ;
	string_use (r) ;
	return r ;
}

/*
 * Dynamic memory
 */

#ifndef LINUX

/*
 *  WIN32 ONLY FUNCS
 */

/*
 *  FUNCTION : fxi_memory_free
 *
 *  Return number of free bytes (ONLY PHYSICAL MEMORY)
 *  NOTE: returned value is VOLATILE as mem is used by WM
 *  while fenix is running
 *
 *  PARAMS:
 *      no params
 *
 *  RETURN VALUE:
 *      pointer to a float value...
 *
 */

static int fxi_memory_free (INSTANCE * my, int * params)
{
	MEMORYSTATUS mem ;
	float result ;

	GlobalMemoryStatus(&mem) ;
	result = (float)mem.dwAvailPhys ;

	return *(int *)&result ;
}

/*
 *  FUNCTION : fxi_memory_total
 *
 *  Return total number of bytes of memory (ONLY PHYSICAL MEMORY)
 *
 *  PARAMS:
 *      no params
 *
 *  RETURN VALUE:
 *      pointer to a float value...
 *
 */

static int fxi_memory_total (INSTANCE * my, int * params)
{
	MEMORYSTATUS mem ;
	float result ;

	GlobalMemoryStatus(&mem) ;
	result = (float)mem.dwTotalPhys ;

	return *(int *)&result ;
}

/*
 * END OF WIN32 ONLY FUNCS
 */

#endif

#ifdef LINUX

/*
 * LINUX ONLY FUNCS
 */
static int fxi_memory_total (INSTANCE * my, int * params)
{
  struct sysinfo meminf;
  int fv;

  if(sysinfo(&meminf) == -1)
    return -1;

  if(!(fv = kernel_version_type()))
    return -1;

  if(fv == 1)
    return meminf.totalram * meminf.mem_unit;
  else
    return meminf.totalram;

  return -1;
}

/*
 * Obtiene la memoria libre
 */

static int fxi_memory_free (INSTANCE * my, int * params)
{
  struct sysinfo meminf;
  int fv;

  if(sysinfo(&meminf) == -1)
    return -1;

  if(!(fv = kernel_version_type()))
    return -1;

  if(fv == 1)
    return meminf.freeram * meminf.mem_unit;
  else
    return meminf.freeram;

  return -1;
}

/*
 * Tipo de version (posterior o anterior a KERNELC_V_*)
 */

int kernel_version_type(void)
{
  struct utsname sysinf;
  int kernel_v[3];
  int i,t,fv = 0;

  if(uname(&sysinf) == -1)
    return -1;

  bzero((int*)kernel_v, sizeof(int)*3);

  for(i=0, t=0; i<=2; i++) {
    if(sysinf.release[t]){
      kernel_v[i] = atoi(&sysinf.release[t]);
      while(sysinf.release[++t] && sysinf.release[t] != '.');
      t++;
    }
  }

  if(!fv && kernel_v[0] > KERNELC_V_1) fv = 1;
  if(!fv && kernel_v[0] < KERNELC_V_1) fv = 2;
  if(!fv && kernel_v[1] > KERNELC_V_2) fv = 1;
  if(!fv && kernel_v[1] < KERNELC_V_2) fv = 2;
  if(!fv && kernel_v[2] > KERNELC_V_3) fv = 1;
  if(!fv && kernel_v[2] < KERNELC_V_3) fv = 2;

  return fv;
}

/*
 * END OF LINUX ONLY FUNCS
 */

#endif

static int fxi_memcopy (INSTANCE * my, int * params)
{
	memmove ((void *)params[0], (void *)params[1], params[2]) ;
	return 1 ;
}

static int fxi_memset (INSTANCE * my, int * params)
{
	memset ((void *)params[0], params[1], params[2]) ;
	return 1 ;
}

static int fxi_memsetw (INSTANCE * my, int * params)
{
	Uint16 * ptr = (Uint16 *)params[0] ;
	const Uint16 b = params[1] ;
	int n ;

	for (n = params[2] ; n ; n--) *ptr++ = b ;
	return 1 ;
}

static int fxi_alloc (INSTANCE * my, int * params)
{
	void * ptr = malloc (params[0]) ;
	if (!ptr) gr_error ("ALLOC: no hay memoria libre suficiente") ;
	return (int)ptr ;
}

static int fxi_realloc (INSTANCE * my, int * params)
{
	void * ptr = realloc ((void *)params[0], params[1]) ;
	if (!ptr) gr_error ("REALLOC: no hay memoria libre suficiente") ;
	return (int)ptr ;
}

static int fxi_free (INSTANCE * my, int * params)
{
	free ((void *)params[0]) ;
	return 1 ;
}

/* Hora del día */

static int fxi_time (INSTANCE * my, int * params)
{
	return time(0) ;
}

/*
 *  FUNCTION : fxi_ftime
 *
 *  Returns parts of the date
 *
 *  PARAMS:
 *      no params
 *
 *  RETURN VALUE:
 *      pointer to a float value...
 *
 */

static int
fxi_ftime (INSTANCE * my, int * params)
{
	char buffer[128] ;
	char * format ;
	struct tm * t ;
	int ret ;
	time_t tim ;
	char * base ;

#ifndef LINUX
	/* aux buffer to make all changes... */
	char aux[128] ;
	unsigned char pos ;

#endif

	format = base = strdup(string_get(params[0])) ;

#ifndef LINUX
	/* Addapting win32 strftime formats to linux formats */
	/* HEAVY PATCH... :( */
	pos=0 ;
	while (*format && pos<127) {
		switch (*format) {
			case '%': /* MIGHT NEED CONVERSION... */
				aux[pos] = *format ;
				pos++ ;
				format++ ;
				switch (*format) {
					case 'e':	aux[pos] = '#' ;
								pos++ ;
								aux[pos] = 'd' ;
								break ;
					case 'l':   aux[pos] = '#' ;
								pos++ ;
								aux[pos] = 'I' ;
								break ;
					case 'k':	aux[pos]='#' ;
								pos++ ;
								aux[pos] = 'H' ;
								break ;
					case 'P':	aux[pos] = 'p' ;
								break ;

					case 'C':	aux[pos++] = '%' ;
								aux[pos++] = *format ;
								aux[pos++] = '%' ;
								aux[pos] = 'Y' ;
								break ;

					case 'u':	aux[pos++] = '%' ;
								aux[pos++] = *format ;
								aux[pos++] = '%' ;
								aux[pos] = 'w' ;
								break ;

					case '%':	//MUST BE %%%% TO KEEP 2 IN POSTPROCESS
								aux[pos++] = '%' ;
								aux[pos++] = '%' ;
								aux[pos] = '%' ;
								break ;

					default:	aux[pos] = *format ;
								break ;
				}
				break ;

			default: aux[pos] = *format ;
					 break ;
			}
		format++ ;
		pos++ ;
	}
	aux[pos]=0 ;
	free(base) ;
	format = aux ;
#endif

	tim = (time_t) params[1] ;
	t = localtime(&tim) ;
	strftime (buffer, 128, format, t) ;
	string_discard(params[0]) ;

#ifndef LINUX
	/* win32 postprocess */
	aux[0] = '\0' ;
	format = buffer ;
	pos = 0 ;
	while (*format) {
		switch (*format) {
			case '%':	format++ ;
						switch (*format) {
							case 'u':	format++ ;
										if (*format=='0') *format='7' ;
										aux[pos] = *format ;
										break ;

							case 'C':	format++ ;
										aux[pos] = *format ;
										pos++ ;
										format++ ;
										aux[pos] = *format ;
										format++ ;
										format++ ;
										break ;

							default:	aux[pos] = *format ;
										break ;
						}
						break ;
			default:	aux[pos] = *format ;
						break ;
		}
		format++ ;
		pos++;
	}
	aux[pos] = '\0' ;
	strcpy(buffer,aux) ;
#endif

	ret = string_new(buffer) ;
	string_use(ret) ;
	return ret ;
}


/* Window Manager */


static int fxi_set_title (INSTANCE * my, int * params) {

	apptitle = strdup(string_get(params[0])) ;
	return 1 ;
}

static int fxi_set_icon (INSTANCE * my, int * params) {

	icono = bitmap_get(params[0],params[1]) ;
	return 1 ;
}


/*
 * Auxiliary QSort functions
 *
 */

double GetData(Uint8 *Data,int pos,int *params){
	int i; Uint8 b; Uint16 w; float f;
	if(params[4]==1){ return Data[pos*params[1]+params[3]];}
	if(params[4]==2){ memcpy(&w,&Data[pos*params[1]+params[3]],params[4]); return w;}
	if(params[4]==4 && params[5]==0){ memcpy(&i,&Data[pos*params[1]+params[3]],params[4]); return i;}
	if(params[4]==4 && params[5]==1){ memcpy(&f,&Data[pos*params[1]+params[3]],params[4]); return f;}
	return 1 ;
}

void QuickSort(Uint8 *Data,int inf, int sup, int *params)
{
	register izq,der;
	double mitad;
	Uint8* x=(Uint8*)malloc(params[1]);
	izq=inf;
	der=sup;
	mitad=GetData(Data,(izq+der)>>1,params);
	do
	{
		while(GetData(Data,izq,params) < mitad && izq<sup)
			izq++;
		while(mitad<GetData(Data,der,params) && der>inf)
			der--;

		if(izq<=der)
		{
			memcpy(x,&Data[izq*params[1]],params[1]);
			memcpy(&Data[izq*params[1]],&Data[der*params[1]],params[1]);
			memcpy(&Data[der*params[1]],x,params[1]);
			izq++;
			der--;
		}
	}while(izq<=der);

	if(inf<der)
		QuickSort(Data,inf,der,params);

	if(izq<sup)
		QuickSort(Data,izq,sup,params);
}

/*
 *	QSort:
 */

static int fxi_quicksort(INSTANCE *my, int *params){ //punteroalarray,tamañodato,numdatos,offsetadatoordenador,tamañodatoaordenar,tipodato(int,float)
	Uint8 *Data=(Uint8 *)params[0];
	QuickSort(Data,0,params[2]-1,params);
	return 1 ;
}

static int fxi_filter (INSTANCE *my, int *params){ //fpg,map,tabla10

	GRAPH * map = bitmap_get (params[0], params[1]), *map2=bitmap_clone(map);
	int *tabla=(int*)params[2];
	int x,y,i,j;
	int r,g,b,r2,g2,b2,c;
	float r1,g1,b1;

	if (map->depth != 16)
		gr_error ("Intento de usar filter con un gráfico de 8 bits") ;

	r1=0;g1=0;b1=0,c=0;
	for(i=1;i<map->width-1;i++)
	{
		for(j=1;j<map->height-1;j++)
		{
			for(y=j-1;y<j+2;y++)
			{
				for(x=i-1;x<i+2;x++)
				{
					gr_get_rgb(gr_get_pixel(map,x,y),&r2,&g2,&b2);
					r1+=(float)(r2*tabla[c]);g1+=(float)(g2*tabla[c]);b1+=(float)(b2*tabla[c]);
					c++;
				}
			}
			r1/=tabla[9];g1/=tabla[9];b1/=tabla[9];
			r=r1;g=g1;b=b1;

			if(r<0)r=0;
			if(g<0)g=0;
			if(b<0)b=0;

			if(!r && !g && !b)
				c=0;
			else
				c=gr_rgb(r,g,b);
			gr_put_pixel(map2,i,j,c);
			r1=0;g1=0;b1=0;c=0;
		}
	}
	memcpy(map->data,map2->data,map->height*map->width*2);
	bitmap_destroy(map2);

	return 1 ;
}

static int fxi_blur (INSTANCE *my, int *params) // fpg,map,tipo
{

	GRAPH * map = bitmap_get (params[0], params[1]), *map2;

	int x,y,i,j,c;
	int r,g,b,r2,g2,b2;

	if (map->depth != 16)
		gr_error ("Intento de usar blur con un gráfico de 8 bits") ;

	switch(params[2])
	{

		case 0:
			//METODO 1 "RAPIDO" izq y arriba
			for(i=0;i<map->width;i++)
				for(j=0;j<map->height;j++)
				{
					gr_get_rgb(gr_get_pixel(map,i,j),&r,&g,&b);
					if(!r && !g && !b)continue;
					if(i>0){
						gr_get_rgb(gr_get_pixel(map,i-1,j),&r2,&g2,&b2);
						r+=r2;g+=g2;b+=b2;
					}
					if(j>0){
						gr_get_rgb(gr_get_pixel(map,i,j-1),&r2,&g2,&b2);
						r+=r2;g+=g2;b+=b2;
					}
					r/=3;g/=3;b/=3;
					gr_put_pixel(map,i,j,gr_rgb(r,g,b));
				}
			break;

		case 1:
			// METODO2 LENTO 3x3
			r=0;g=0;b=0;c=0;
			for(i=0;i<map->width;i++)
				for(j=0;j<map->height;j++)
				{
					gr_get_rgb(gr_get_pixel(map,j,i),&r,&g,&b);
					if(!r && !g && !b)continue;
					for(x=i-1;x<i+2;x++)
					{
						for(y=j-1;y<j+2;y++)
						{
							if(x<0 || x>map->width-1 || y<0 || y>map->height-1)continue;
							gr_get_rgb(gr_get_pixel(map,x,y),&r2,&g2,&b2);
							r+=r2;g+=g2;b+=b2;
							c++;
						}
					}
					r/=c;g/=c;b/=c;
					gr_put_pixel(map,i,j,gr_rgb(r,g,b));
					r=0;g=0;b=0;c=0;
				}
			break;

		case 2:
			// METODO3 aun mas LENTO 5x5
			r=0;g=0;b=0;c=0;
			for(i=0;i<map->width;i++)
			{
				for(j=0;j<map->height;j++)
				{
					gr_get_rgb(gr_get_pixel(map,j,i),&r,&g,&b);
					if(!r && !g && !b)continue;
					for(x=i-2;x<i+3;x++)
					{
						for(y=j-2;y<j+3;y++)
						{
							if(x<0 || x>map->width-1 || y<0 || y>map->height-1)continue;
							gr_get_rgb(gr_get_pixel(map,x,y),&r2,&g2,&b2);
							r+=r2;g+=g2;b+=b2;
							c++;
						}
					}
					r/=c;g/=c;b/=c;
					gr_put_pixel(map,i,j,gr_rgb(r,g,b));
					r=0;g=0;b=0;c=0;
				}
			}
			break;

		case 3:
			// METODO4 5x5 mapa adicional
			map2 = bitmap_clone(map);
			r=0;g=0;b=0;c=0;
			for(i=0;i<map->width;i++)
			{
				for(j=0;j<map->height;j++)
				{
					gr_get_rgb(gr_get_pixel(map,j,i),&r,&g,&b);
					if(!r && !g && !b)continue;
					for(x=i-2;x<i+3;x++)
					{
						for(y=j-2;y<j+3;y++)
						{
							if(x<0 || x>map->width-1 || y<0 || y>map->height-1)continue;
							gr_get_rgb(gr_get_pixel(map,x,y),&r2,&g2,&b2);
							r+=r2;g+=g2;b+=b2;
							c++;
						}
					}
					r/=c;g/=c;b/=c;
					gr_put_pixel(map2,i,j,gr_rgb(r,g,b));
					r=0;g=0;b=0;c=0;
				}
			}
			memcpy(map->data,map2->data,map->height*map->width*2);
			bitmap_destroy(map2);
		break;
		default: break;

	}
	return 1 ;
}

static int fxi_grayscale (INSTANCE *my, int *params) //fpg,map,tipo
{
	GRAPH * map = bitmap_get (params[0], params[1]) ;
	int i,j,c;
	Uint8 r,g,b;

	if (!map) return -1 ;
    if (map->depth != 16)
		gr_error ("Intento de usar grayscale con un gráfico de 8 bits") ;

	for(i=0;i<map->height;i++)
		for(j=0;j<map->width;j++){
			gr_get_rgb(gr_get_pixel(map,j,i),&r,&g,&b);
			if(!r && !g && !b)continue;
			c=0.3*r + 0.59*g + 0.11*b;
			switch(params[2]){
				case 0: // RGB
					c=gr_rgb(c,c,c);
					break;
				case 1: // R
					c=gr_rgb(c,0,0);
					break;
				case 2: // G
					c=gr_rgb(0,c,0);
					break;
				case 3: // B
					c=gr_rgb(0,0,c);
					break;
				case 4: // RG
					c=gr_rgb(c,c,0);
					break;
				case 5: // RB
					c=gr_rgb(c,0,c);
					break;
				case 6: // GB
					c=gr_rgb(0,c,c);
					break;
				default:
					c=gr_rgb(r,g,b);
			}
			gr_put_pixel(map,j,i,c);
		}

	return 1 ;
}

static int fxi_rgbscale (INSTANCE *my, int *params) //fpg, map, r, g, b
{
	GRAPH * map = bitmap_get (params[0], params[1]) ;
	int i,j,c;
	Uint8 r,g,b;

	if (!map) return -1 ;
	if (map->depth != 16)
		gr_error ("Intento de usar rgbscale con un gráfico de 8 bits") ;

	for(i=0;i<map->height;i++)
		for(j=0;j<map->width;j++){
			gr_get_rgb(gr_get_pixel(map,j,i),&r,&g,&b);
			if(!r && !g && !b)continue;
			c=0.3*r + 0.59*g + 0.11*b;
			c=gr_rgb(c**(float *)(&params[2]),c**(float *)(&params[3]),c**(float *)(&params[4]));
			gr_put_pixel(map,j,i,c);
		}
	return 1 ;
}

/* ---------------------------------------------------------------------- */

#include "sysprocs.h"

static SYSPROC * sysproc_tab[256+MAX_SYSPROCS] ;

int sysproc_add (char * name, char * paramtypes, int type, void * func)
{
	static SYSPROC * last = 0 ;
	static int sysproc_count = 0 ;

	if (!last)
	{
		last = sysprocs ;
		sysproc_count++ ;
		while (last[1].func) last++, sysproc_count++ ;
	}
	last[1].code = last[0].code + 1 ;
	last[1].name = name ;
	last[1].paramtypes = paramtypes ;
	last[1].params = strlen(paramtypes) ;
	last[1].type = type ;
	last[1].func = func ;
	last[1].id   = 0 ;
	last++ ;
	sysproc_count++ ;
	if (sysproc_count == MAX_SYSPROCS)
		gr_error ("Demasiadas funciones del sistema") ;
	last[1].func = 0 ;
	return last->code ;
}

SYSPROC * sysproc_get (int code)
{
	return sysproc_tab[code] ;
}

void sysproc_init()
{
	SYSPROC         * proc = sysprocs ;
	void            * library ;
	dlfunc          RegisterFunctions ;
	char            * filename;
	int             n ;

	for (n = 0 ; n < dcb.NImports ; n++)
	{
        filename = string_get(dcb.imports[n]) ;

        library  = dlopen ( filename, RTLD_NOW | RTLD_GLOBAL) ;
		if (!library) gr_error (dlerror()) ;

		RegisterFunctions = dlsym (library, "RegisterFunctions") ;
		if (!RegisterFunctions) gr_error("Error in %s", filename) ;

        (*RegisterFunctions)(fnc_export, sysproc_add) ;
	}

	while (proc->func)
	{
		sysproc_tab[proc->code] = proc ;
		proc++ ;
	}
}
