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
 * HISTORY: 0.74 - Added apptitle and icono for WM functions
 *
 */

#ifndef __GRLIB_ST_H
#define __GRLIB_ST_H

/* -------------------------------------------------------------------- */
/* Librería gráfica                                                     */
/* -------------------------------------------------------------------- */

/* Tipos y datos globales */

typedef struct _cpoint
{
    short int x ;
    short int y ;
}
CPOINT ;

typedef struct _animation
{
    int     frames ;
    int     length ;
    int     pos ;
    int     speed ;
    int     remaining ;
    int     last_frame ;
    Sint16  * order ;
}
ANIMATION ;

typedef struct _bitmap
{
    int         width ;
    int         pitch ;
    int         height ;
    int         code ;
    int         depth ;
    char        name[32] ;
    void        * data ;
    CPOINT      * cpoints ;
    int         flags ;
    Uint32      offset ;

    ANIMATION * animation ;
    Sint16    * blend_table ;

    int         modified ;
    int         info_flags ;
}
GRAPH ;

#define F_NCPOINTS  0x0FFF
#define F_ANIMATION 0x1000

#define GI_NOCOLORKEY 1

typedef struct _font
{
    GRAPH   * chars[256] ;
    int     voffset[256] ;
    int     width  [256] ;
}
FONT ;

typedef struct _grlib
{
    GRAPH   ** maps ;
    int     map_count ;
    int     map_reserved ;
}
GRLIB ;

typedef struct _clipregion
{
    int x ;
    int y ;
    int x2 ;    /* Inclusive */
    int y2 ;
}
REGION ;

typedef struct _keyequiv {
    int                 sdlk_equiv ;
    struct _keyequiv    * next ;
} key_equiv ;

#define region_get(n) ( ((n)>=0 && (n)<=31) ? &regions[n] : &regions[0] )

/* Bajo nivel */
/* ---------- */

/* Flags para gr_blit */
#define B_HMIRROR       0x01
#define B_VMIRROR       0x02
#define B_TRANSLUCENT   0x04
#define B_NOCOLORKEY    0x80

#endif
