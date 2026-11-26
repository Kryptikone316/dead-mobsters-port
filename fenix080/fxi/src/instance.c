/*
 *  Fenix - Videogame compiler/interpreter
 *  Current release       : FENIX - PROJECT 1.0 - R 0.8
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
 * HISTORY:  0.80 - Solved leak problem in instance_destroy()
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "fxi.h"

/* ---------------------------------------------------------------------- */
/* Módulo de gestión de instancias, con las funciones de incialización y  */
/* destrucción, duplicado, etc.                                           */
/* ---------------------------------------------------------------------- */

INSTANCE * first_instance = 0 ;
INSTANCE * last_instance  = 0 ;

static int instance_maxid =  FIRST_INSTANCE_ID-2 ;

int instance_getid()
{
	instance_maxid += 2 ;
	return instance_maxid ;
}

INSTANCE * instance_duplicate (INSTANCE * father)
{
	INSTANCE * r, * brother ;
	int n ;

	r = (INSTANCE *) malloc (sizeof(INSTANCE)) ;
	assert (r != 0) ;

	r->pridata = (int *) malloc (father->private_size + 4) ;
	r->locdata = (int *) malloc (local_size + 4) ;
	r->code    = father->code ;
	r->codeptr = father->codeptr ;
	r->proc    = father->proc ;

	r->private_size = father->private_size ;

	memcpy (r->pridata, father->pridata, father->private_size) ;
	memcpy (r->locdata, father->locdata, local_size) ;

	/* Actualiza las cuentas de uso de las cadenas */

	for (n = 0 ; n < r->proc->string_count ; n++)
		string_use (PRIDWORD(r, r->proc->strings[n])) ;

	for (n = 0 ; n < local_strings ; n++)
		string_use (LOCDWORD(r, localstr[n])) ;

	/* Inicializa datos DIV */

	/* Crea el proceso clónico como si lo hubiera llamado el padre */
	/* No sé si es eso lo que hace DIV */

	LOCDWORD(r, PROCESS_ID)   = instance_getid() ;
	LOCDWORD(r, FATHER)       = LOCDWORD(father,PROCESS_ID) ;
	LOCDWORD(r, SON)          = 0 ;
	LOCDWORD(r, SMALLBRO)     = 0 ;
	LOCDWORD(r, BIGBRO)       = LOCDWORD(father,SON) ;

	brother = instance_get (LOCDWORD(father, SON)) ;
	if (brother) LOCDWORD(brother, SMALLBRO) = LOCDWORD(r,PROCESS_ID) ;

	LOCDWORD(father, SON)     = LOCDWORD(r,PROCESS_ID) ;

	/* Añade la instancia al final de la lista */

	r->next    = 0 ;
	r->prev    = last_instance ;
	if (r->prev) r->prev->next = r ;

	last_instance = r ;
	if (!first_instance) first_instance = r ;
	
	return r ;
}

INSTANCE * instance_new (PROCDEF * proc, INSTANCE * father)
{
	INSTANCE * r ;
	INSTANCE * brother ;
	int n ;

	r = (INSTANCE *) malloc (sizeof(INSTANCE)) ;
	assert (r != 0) ;

	r->pridata = (int *) malloc (proc->private_size + 4) ;
	r->locdata = (int *) malloc (local_size + 4) ;
	r->code    = proc->code ;
	r->codeptr = proc->code ;
	r->proc    = proc ;

	r->private_size = proc->private_size ;

	memcpy (r->pridata, proc->pridata, proc->private_size) ;
	memcpy (r->locdata, localdata, local_size) ;

	/* Inicializa datos DIV */

	LOCDWORD(r, PROCESS_ID)   = instance_getid() ;
	LOCDWORD(r, PROCESS_TYPE) = proc->typeid ;

	if (father)
	{
		LOCDWORD(r, FATHER)     = LOCDWORD(father, PROCESS_ID) ;
		brother = instance_get (LOCDWORD(father, SON)) ;
		if (brother)
		{
			LOCDWORD(r, BIGBRO)         = LOCDWORD(brother, PROCESS_ID) ;
			LOCDWORD(brother, SMALLBRO) = LOCDWORD(r, PROCESS_ID) ;
		}

		LOCDWORD(father, SON) = LOCDWORD(r, PROCESS_ID) ;
	}

	/* Cuenta los usos de las variables tipo cadena */

	for (n = 0 ; n < proc->string_count ; n++)
		string_use (PRIDWORD(r, proc->strings[n])) ;

	for (n = 0 ; n < local_strings ; n++)
		string_use (LOCDWORD(r, localstr[n])) ;

	/* Añade la instancia al final de la lista */

	r->next    = 0 ;
	r->prev    = last_instance ;
	if (r->prev) r->prev->next = r ;

	last_instance = r ;
	if (!first_instance) first_instance = r ;
	
	return r ;
}

INSTANCE * instance_get (int id)
{
	INSTANCE * i = first_instance ;

	while (i)
	{
		if (LOCDWORD(i, PROCESS_ID) == id) break ;
		i = i->next ;
	}
	return i ;
}

INSTANCE * instance_getfather (INSTANCE * i)
{
	return instance_get (LOCDWORD(i, FATHER)) ;
}

INSTANCE * instance_getson (INSTANCE * i)
{
	return instance_get (LOCDWORD(i, SON)) ;
}

INSTANCE * instance_getbigbro (INSTANCE * i)
{
	return instance_get (LOCDWORD(i, BIGBRO)) ;
}

INSTANCE * instance_getsmallbro (INSTANCE * i)
{
	return instance_get (LOCDWORD(i, SMALLBRO)) ;
}

void instance_destroy_all (INSTANCE * except)
{
	INSTANCE * i, * next ;

	i = first_instance ;
	while (i)
	{
		next = i->next ;
		if (i != except)
			instance_destroy (i) ;
		i = next ;
	}
}

void instance_destroy (INSTANCE * r)
{
	INSTANCE * father, * brother, * son, * prime ;
	int n ;

	/* Actualiza la cuenta de referencia de las variables tipo string */

	for (n = 0 ; n < r->proc->string_count ; n++)
		string_discard (PRIDWORD(r, r->proc->strings[n])) ;

	for (n = 0 ; n < local_strings ; n++)
		string_discard (LOCDWORD(r, localstr[n])) ;

	/* Quita la instancia de la lista */

	if (r->prev) r->prev->next  = r->next ;
	else         first_instance = r->next ;

	if (r->next) r->next->prev = r->prev ;
	else         last_instance = r->prev ;

	if (first_instance == r)
		first_instance = r->next ;

	/* Actualiza árbol DIV */

	brother = instance_get (LOCDWORD(r,BIGBRO)) ;
	if (brother) LOCDWORD(brother,SMALLBRO) = LOCDWORD(r,SMALLBRO) ;
	father = instance_get (LOCDWORD(r,FATHER)) ;
	if (father)
	{
		if (LOCDWORD(father, SON) == LOCDWORD(r, PROCESS_ID))
		{
			if (brother)
				LOCDWORD(father,SON) = LOCDWORD(brother,PROCESS_ID) ;
			else
				LOCDWORD(father,SON) = 0 ;
		}
	}

	brother = instance_get (LOCDWORD(r,SMALLBRO)) ;
	if (brother) LOCDWORD(brother,BIGBRO) = LOCDWORD(r,BIGBRO) ;

	son = instance_get (LOCDWORD(r,SON)) ;
	if (son)
	{
		/* Busca el primer y el último hijo */

		brother = son ;
		while (LOCDWORD(son,BIGBRO))
		{
			son = instance_get (LOCDWORD(son,BIGBRO)) ;
			assert (son != 0) ;
		}
		while (LOCDWORD(brother,SMALLBRO))
		{
			brother = instance_get (LOCDWORD(son,SMALLBRO)) ;
			assert (brother != 0) ;
		}

		/* Los procesos han quedado huérfanos; asígnalos al padre */

		prime = first_instance ;
		assert (prime != 0) ;

		while (son)
		{
			LOCDWORD(son,FATHER) = LOCDWORD(r,FATHER) ;
			son = instance_get (LOCDWORD(son,SMALLBRO)) ;
		}
	}
	// Added liberation of locadata... removes leak problem
	free (r->locdata) ;
	free (r->pridata) ;
	free (r) ;
}
