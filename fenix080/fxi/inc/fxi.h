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
 * FILE        : fxi.h
 * DESCRIPTION : Base includes in FXI and some vars and const defines
 *
 * HISTORY:
 *
 */

/*
 *  VERSION
 */

#define VERSION "FXI 0.80 " __DATE__ " " __TIME__

/*
 *  HEADER FILES
 */

#include <SDL.h>
#include "files.h"
#include "xctype.h"

/*
 *  CONSTANTS
 */

#ifndef M_PI
#define M_PI           3.14159265358979323846  /* pi */
#endif

#include "typedef.h"
#if 0
#define TYPE_DWORD  0
#define TYPE_WORD   1
#define TYPE_BYTE   2
#define TYPE_STRING 3
#define TYPE_FLOAT  4
#define TYPE_POINTER 19
#endif

/*
 *  GLOBAL VARIABLES
 */
extern int debug ;              /* 1 = Actovate debug options       */
extern int fxi ;                /* 1 = EXE module is FXI.EXE        */
extern int enable_16bits ;      /* 1 = 16bpp MODE on                */
extern int enable_filtering ;   /* 1 = 16bpp filter MODE on         */

/*
 *  DEBUG FLAGS
 */

extern int report_audio ;
extern int report_string ;
extern int report_graphics ;

#include "strings.h"
#include "offsets.h"
#include "pslang.h"
#include "instance.h"
#include "i_procdef.h"
#include "grlib.h"
#include "sound.h"
#include "flic.h"

extern void   gr_error      (const char *fmt, ...) ;
extern void   do_exit       () ;
extern int    dcb_load      (const char * filename) ;
extern char * getid         (unsigned int code) ;
extern int    path_find     (GRAPH * bitmap, int sx, int sy, int dx, int dy, int options) ;
extern int    path_get      (int * x, int * y) ;
extern int    path_set_wall (int n) ;
