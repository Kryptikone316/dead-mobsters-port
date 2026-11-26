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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "fxi.h"
#include "dcb.h"

/* ---------------------------------------------------------------------- */
/* Gestor de cadenas, solo para FXI                                      */
/* ---------------------------------------------------------------------- */

#define STRING_MAX 1024

int report_string = 0 ;

char * string_mem = 0 ;
int    string_allocated = 0 ;
int    string_used = 0 ;

char * string_ptr[1024] ;
int    string_uct[1024] ;
char   string_dontfree[1024] ;
int    string_count = 1 ;

void string_init ()
{
	string_mem = (char *) malloc (4096) ;
	string_allocated = 4096 ;
	string_used = 0 ;
	string_count = 1 ;
	string_ptr[0] = string_mem ;
	string_mem[0] = 0 ;
}

void string_alloc (int bytes)
{
	string_mem = (char *) realloc (string_mem, string_allocated += bytes) ;
	if (!string_mem) gr_error ("string_alloc: sin memoria\n") ;
}

void string_dump ()
{
	int i ;
	gr_con_printf ("[STRING] ---- Dumping %d strings ----\n", string_count) ;
	for (i = 0 ; i < string_count ; i++)
	{
		if (string_uct[i] == 0)
		{
			if (string_ptr[i]) 
			{
                                if (!string_dontfree[i])
                                        free(string_ptr[i]) ;
				string_ptr[i] = 0 ;
			}
			continue ;
		}
		gr_con_printf ("[STRING] %4d [%4d]: {%s}\n", i, string_uct[i], string_ptr[i]) ;
	}
}


const char * string_get (int code)
{
	assert (code < string_count && code >= 0) ;
	return string_ptr[code] ;
}

void string_load (file * fp)
{
	int string_offset[STRING_MAX], n ;

	string_count = dcb.NStrings ;
	string_used  = dcb.SText ;
	file_seek (fp, dcb.OStrings, SEEK_SET) ;
	file_read (fp, &string_offset, 4 * string_count) ;
	if (string_used > string_allocated)
	{
		string_allocated = string_used ;
		string_mem = (char *) realloc (string_mem, string_used) ;
	}
	file_seek (fp, dcb.OText, SEEK_SET) ;
	file_read (fp, string_mem, string_used) ;

	for (n = 0 ; n < string_count ; n++)
	{
		string_ptr[n] = string_mem + string_offset[n] ;
		string_uct[n] = 2 ;
	}
}

void string_use (int code)
{
	string_uct[code]++ ;
        if (report_string)
                gr_con_printf ("[STRING] String %d used (count: %d)\n", 
                                code, string_uct[code]) ;
}

void string_discard (int code)
{
	if (string_uct[code] == 0)
	{
                if (report_string)
                        gr_con_printf ("[STRING] String %d released "
                                "but already discarted\n", code) ;
		return ;
	}

	string_uct[code]-- ;
        if (report_string)
                gr_con_printf ("[STRING] String %d released (count: %d)\n", 
                        code, string_uct[code]) ;
	if (string_uct[code] < 1 && code > 0)
	{
                if (report_string)
                        gr_con_printf ("[STRING] String %d released "
                                "and discarted\n", code) ;
                if (!string_dontfree[code])
                        free(string_ptr[code]) ;
                if (report_string && string_dontfree[code])
                        gr_con_printf ("[STRING] (Memory don't freed - special string)\n") ;
		string_ptr[code] = 0 ;
		if (code == string_count-1)
		{
			while (string_count > 1 && 
			       string_ptr[string_count-1] == 0)
				string_count--;
		}
	}
}

void string_coalesce()
{
	int n ;

	if (string_count < STRING_MAX/2)
		return ;

	for (n = 1 ; n < STRING_MAX ; n++)
	{
		if (!string_uct[n])
		{
                        if (!string_dontfree[n])
                                free (string_ptr[n]) ;
			string_ptr[n] = 0 ;
			string_uct[n] = 0 ;
		}
	}

	while (string_count > 1 && string_ptr[string_count-1] == 0)
		string_count--;
}

int string_getid ()
{
	int n ;

	if (string_count < STRING_MAX)
		return string_count++ ;

	for (n = 1 ; n < STRING_MAX ; n++)
		if (!string_ptr[n]) return n ;

	for (n = 1 ; n < STRING_MAX ; n++)
		if (!string_uct[n]) return n ;

	gr_error ("Demasiados textos en uso") ;
        return 0 ;
}

int string_new (const char * ptr)
{
	char * str = strdup(ptr) ;
	int    id ;

	assert (str) ;
	id = string_getid() ;

        if (report_string)
                gr_con_printf ("[STRING] String %d created: \"%s\"\n", id, str) ;

	string_ptr[id] = str ;
	string_uct[id] = 0 ;
        string_dontfree[id] = 0 ;
	return id ;
}

void string_internal (int code)
{
        assert (code >= 0 && code < STRING_MAX) ;
        string_dontfree[code] = 1 ;
}

int string_concat (int code1, char * str2)
{
	char * str1 ;

	assert (code1 < string_count && code1 >= 0) ;

        str1 = string_ptr[code1] ;
	assert (str1) ;

	str1 = (char *) realloc(str1, strlen(str1) + strlen(str2) + 1) ;
	assert (str1) ;

	strcat (str1, str2) ;

	string_ptr[code1] = str1 ;
	return code1 ;
}

int string_add (int code1, int code2)
{
	const char * str1 = string_get(code1) ;
	const char * str2 = string_get(code2) ;
	char * str3 ;
	int id ;

	assert (str1) ;
	assert (str2) ;

	str3 = (char *) malloc(strlen(str1) + strlen(str2) + 1) ;
	assert (str3) ;

	strcpy (str3, str1) ;
	strcat (str3, str2) ;
	id = string_getid() ;

	string_ptr[id] = str3 ;
	string_uct[id] = 0 ;
	return id ;
}

int string_ptoa (void * n)
{
	char * str ;
	int id ;

	str = (char *) malloc(64) ;
	assert (str)  ;

	sprintf (str, "%p", n) ;

	id = string_getid() ;
	string_ptr[id] = str ;
	string_uct[id] = 0 ;
	return id ;
}

int string_ftoa (float n)
{
	char * str, * ptr ;
	int id ;

	str = (char *) malloc(64) ;
	assert (str)  ;

	sprintf (str, "%f", n) ;
	ptr = str + strlen(str) - 1 ;
	while (ptr >= str)
	{
		if (*ptr != '0') break ;
		*ptr-- = 0 ;
	}
	if (ptr >= str && *ptr == '.') *ptr = 0 ;
	if (*str == 0) strcpy (str, "0") ;

	id = string_getid() ;
	string_ptr[id] = str ;
	string_uct[id] = 0 ;
	return id ;
}

int string_itoa (int n)
{
	char * str ;
	int id ;

	str = (char *) malloc(8) ;
	assert (str)  ;

	sprintf (str, "%d", n) ;

	id = string_getid() ;
	string_ptr[id] = str ;
	string_uct[id] = 0 ;
	return id ;
}

int string_comp (int code1, int code2)
{
	const char * str1 = string_get(code1) ;
	const char * str2 = string_get(code2) ;

	return strcmp (str1, str2) ;
}

int string_char (int n, int nchar)
{
	const char * str = string_get(n) ;
	int          len ;
	char buffer[2] ;

	assert (str) ;
	len = strlen(str) ;
	if (nchar >= len)
		nchar = len ;
	if (nchar < 0)
		nchar = len + nchar ;
	if (nchar < 0)
		nchar = len ;
	buffer[0] = str[nchar] ;
	buffer[1] = 0 ;
	return string_new (buffer) ;
}

int string_substr (int code, int first, int last)
{
	const char * str = string_get(code) ;
	char       * ptr ;
	int          len, n ;

	assert (str) ;
	len = strlen(str) ;
	if (first < 0) first = len + first ;
	if (last  < 0) last  = len + last ;
	if (first < 0) first = 0 ;
	if (last  < 0) { return string_new("") ; }
	if (first > len) first = len ;
	if (last  > len) last  = len ;
	if (first > last)
	{
		n = first ;
		first = last ;
		last = n ;
	}

	ptr = (char *)malloc(last-first+2) ;
	n   = string_getid() ;
	memcpy (ptr, str+first, last-first+1) ;
	ptr[last-first+1] = 0 ;
	string_ptr[n]   = ptr ;
	string_uct[n]   = 0 ;
	return n ;
}

int string_find (int code1, int code2)
{
	const char * str1 = string_get(code1) ;
	const char * str2 = string_get(code2) ;
	int pos, len ;

	assert (str1 && str2) ;
	len = strlen(str2) ;
	for (pos = 0 ; str1[pos] ; pos++)
	{
		if (memcmp(str1+pos, str2, len) == 0)
			return pos ;
	}
	return -1 ;
}

int string_ucase (int code)
{
	const char * str = string_get(code) ;
	char       * bptr, * ptr ;
	int          id ;

	assert (str) ;
	bptr = (char *)malloc(strlen(str)+1) ;
	assert (bptr) ;

	for (ptr = bptr; *str ; ptr++, str++) 
		*ptr = TOUPPER(*str) ;
	*ptr = 0 ;
	id = string_getid() ;
	string_ptr[id] = bptr ;
	string_uct[id] = 0 ;
	return id ;
}

int string_lcase (int code)
{
	const char * str = string_get(code) ;
	char       * bptr, * ptr ;
	int          id ;

	assert (str) ;
	bptr = (char *)malloc(strlen(str)+1) ;
	assert (bptr) ;

	for (ptr = bptr; *str ; ptr++, str++) 
		*ptr = TOLOWER(*str) ;
	*ptr = 0 ;
	id = string_getid() ;
	string_ptr[id] = bptr ;
	string_uct[id] = 0 ;
	return id ;
}
