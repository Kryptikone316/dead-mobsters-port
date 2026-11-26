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

#ifndef __STRINGS_H
#define __STRINGS_H

/* Gestor de cadenas */

extern void         string_init    () ;
extern const char * string_get     (int code) ;
extern void         string_dump    () ;
extern void         string_load    (file *) ;
extern void         string_save    (file *) ;

extern int          string_new     (const char * ptr) ;
extern void         string_internal(int code) ;
extern void         string_use     (int code) ;
extern void         string_discard (int code) ;
extern int          string_add     (int code1, int code2) ;
extern int          string_compile (const char * * source) ;
extern int          string_itoa    (int n) ;
extern int          string_ftoa    (float n) ;
extern int          string_ptoa    (void * n) ;
extern int          string_comp    (int code1, int code2) ;
extern int          string_char    (int n, int nchar) ;
extern int          string_substr  (int code, int first, int last) ;
extern int          string_find    (int code1, int code2) ;
extern int          string_ucase   (int code1) ;
extern int          string_lcase   (int code1) ;
extern void         string_coalesce() ;

#endif
