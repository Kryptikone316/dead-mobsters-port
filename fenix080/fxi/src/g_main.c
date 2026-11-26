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
 * HISTORY: 0.76 - Patched DRAW_INSTANCE so XGRAPH does not rotate graphic
 *			0.76 - Patched DRAW_INSTANCE_AT so XGRAPH does not rotate graphic
 *			0.75 - Added icon WM capabilities on 8/16 bpp
 *			0.75 - New Keyboard system about 60/70% done
 *			0.74 - Added some more SDLK symbols to list
 *			0.74 - Added title WM capabilities... working in icon
 *			0.72 - Added SDLK_8 to keylistg
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <assert.h>

#include <SDL.h>
#include <SDL_keysym.h>

#include "fxi.h"


SDL_Surface * screen ;

GRAPH * background ;
GRAPH * scrbitmap = 0 ;
GRAPH * background_8bits ;
GRAPH * icono = 0 ;

int     background_8bits_used ;
int     enable_16bits = 0 ;
int		enable_filtering = 0 ;
int		background_is_black = 1 ;

char *	apptitle ;

int		scr_width = 320 ;
int		scr_height = 200 ;

int		scr_initialized = 0 ;

int		full_screen = 1 ;
int		double_buffer = 0 ;

static int scrbitmap_is_fake = 0 ;
static int show_console = 0 ;

int report_graphics = 0 ;

/* ---------------------------------------------------------------------- */
/* Gestión de regiones                                                    */
/* ---------------------------------------------------------------------- */

REGION    regions[32] ;

#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

void region_define (int region, int x, int y, int width, int height)
{
	if (region < 1 || region > 31)
		return ;

	regions[region].x = MAX (x, 0) ;
	regions[region].y = MAX (y, 0) ;
	regions[region].x2 = MIN (scr_width,  x+width)  - 1 ;
	regions[region].y2 = MIN (scr_height, y+height) - 1 ;
}

void region_union (REGION * dest, REGION * b)
{
	dest->x  = MAX(dest->x,  b->x) ;
	dest->y  = MAX(dest->y,  b->y) ;
	dest->y2 = MIN(dest->y2, b->y2) ;
	dest->x2 = MIN(dest->x2, b->x2) ;
}

int region_is_empty (REGION * a)
{
	return (a->x2 < a->x) || (a->y2 < a->y) ;
}

int region_is_out (REGION * a, REGION * b)
{
	return (b->x > a->x2 || b->y > a->y2 || b->x2 < a->x || b->y2 < a->y);
}

/* ---------------------------------------------------------------------- */
/* Gestión de eventos (ratón, teclado)                                    */
/* ---------------------------------------------------------------------- */

static int sdl_equiv[SDLK_LAST+1] ; 
key_equiv key_table[127] ;  /* Now we have a search table with equivs */ 
unsigned char * keystate ;		  /* Pointer to key states */

static int equivs[] =
{
	SDLK_ESCAPE,		1,
	SDLK_1,				2,
	SDLK_KP1,			2,
	SDLK_2,				3,
	SDLK_KP2,			3,
	SDLK_3,				4,
	SDLK_KP3,			4,
	SDLK_4,				5,
	SDLK_KP4,			5,
	SDLK_5,				6,
	SDLK_KP5,			6,
	SDLK_6,				7,
	SDLK_KP6,			7,
	SDLK_7,				8,
	SDLK_KP7,			8,
	SDLK_8,				9,
	SDLK_KP8,			9,
	SDLK_9,				10,
	SDLK_KP9,			10,
	SDLK_0,				11,
	SDLK_KP0,			11,
	SDLK_MINUS,			12,
	SDLK_PLUS,			13,
	SDLK_BACKSPACE,		14,
	SDLK_TAB,			15,
	SDLK_q,				16,
	SDLK_w,				17,
	SDLK_e,				18,
	SDLK_r,				19,
	SDLK_t,				20,
	SDLK_y,				21,
	SDLK_u,				22,
	SDLK_i,				23,
	SDLK_o,				24,
	SDLK_p,				25,
	SDLK_LEFTBRACKET,	26,
	SDLK_RIGHTBRACKET,	27,
	SDLK_RETURN,		28,
	SDLK_KP_ENTER,		28,
	SDLK_LCTRL,			29,
	SDLK_RCTRL,			29,
	SDLK_a,				30,
	SDLK_s,				31,
	SDLK_d,				32,
	SDLK_f,				33,
	SDLK_g,				34,
	SDLK_h,				35,
	SDLK_j,				36,
	SDLK_k,				37,
	SDLK_l,				38,
	SDLK_SEMICOLON,		39,
	SDLK_CARET,			40,
	SDLK_QUOTE,			41,	
	SDLK_LSHIFT,		42,	
	SDLK_BACKSLASH,		43,
	SDLK_z,				44,
	SDLK_x,				45,
	SDLK_c,				46,
	SDLK_v,				47,
	SDLK_b,				48,
	SDLK_n,				49,
	SDLK_m,				50,
	SDLK_COMMA,			51,
	SDLK_PERIOD,		52,
	SDLK_KP_PERIOD,		52,
	SDLK_SLASH,			53,
	SDLK_KP_DIVIDE,		53,
	SDLK_RSHIFT,		54,
/*	SDLK_PRINT,			55,*/
	SDLK_KP_MULTIPLY,	55,
	SDLK_LALT,			56,
	SDLK_RALT,			56,
	SDLK_RMETA,			56,
	SDLK_LMETA,			56,
	SDLK_SPACE,			57,
	SDLK_CAPSLOCK,		58,
	SDLK_F1,			59,
	SDLK_F2,			60,
	SDLK_F3,			61,
	SDLK_F4,			62,
	SDLK_F5,			63,
	SDLK_F6,			64,
	SDLK_F7,			65,
	SDLK_F8,			66,
	SDLK_F9,			67,
	SDLK_F10,			68,
	SDLK_NUMLOCK,		69,
	SDLK_SCROLLOCK,		70,
	SDLK_HOME,			71,
	SDLK_UP,			72,
	SDLK_PAGEUP,		73,
	SDLK_KP_MINUS,	    74,
	SDLK_LEFT,			75,
	SDLK_RIGHT,			77,
	SDLK_KP_PLUS,		78,
	SDLK_END,			79,
	SDLK_DOWN,			80,
	SDLK_PAGEDOWN,		81,
	SDLK_INSERT,		82,
	SDLK_DELETE,		83,
	SDLK_F11,			87,
	SDLK_F12,			88,
	SDLK_LESS,			89,
	SDLK_EQUALS,		90,
	SDLK_GREATER,		91,
	SDLK_QUESTION,		92,
	-1, -1
} ;

int keytab_initialized = 0 ;

void add_key_equiv(int equiv, int keyconst) {
  
	key_equiv * curr = &key_table[keyconst] ;	
	if (curr->next != NULL) {
		while (curr->next!=NULL) 
			curr=curr->next ;
	}
	if (curr->sdlk_equiv!=0) {
		curr->next = malloc(sizeof(key_equiv)) ;
		curr = curr->next ;
		curr->next = NULL ;
		}
	curr->sdlk_equiv = equiv ;
}

/* FREE used key_equivs... note base 127 equivs are static not allocated... */
void keytab_free() {

	int i ;
	key_equiv * aux ;
	key_equiv * curr = key_table ;

	for (i=0;i<127;i++) {
		if (curr->next!=NULL) {
			curr = curr->next ;
			while (curr->next!=NULL) {
				aux = curr ;
				curr = curr->next;
				free(aux) ;
			}
			free(curr) ;
		}
	}
}
void keytab_init()
{
	int * ptr = equivs ;

	memset (sdl_equiv, 0, sizeof(sdl_equiv));
	memset (key_table,  0, sizeof(key_table)) ;

	while (*ptr != -1)
	{
		sdl_equiv[*ptr] = ptr[1] ;
		add_key_equiv(ptr[0],ptr[1]) ;
		ptr += 2 ;
	}
	keystate = SDL_GetKeyState(NULL) ;
	SDL_EnableUNICODE(1);
	keytab_initialized = 1 ;
}

#define MAX_JOYSTICKS 8
int joy_x[8] = { 0, 0, 0, 0, 0, 0, 0, 0 } ;
int joy_y[8] = { 0, 0, 0, 0, 0, 0, 0, 0 } ;
int joy_b[8] = { 0, 0, 0, 0, 0, 0, 0, 0 } ;

static void do_events ()
{
	SDL_Event e ;
	SDLMod m ;
	int k,asc ;
	int pressed ;
	key_equiv * curr ;

	int keypress ;
	static struct
	{
		int ascii ;
		int scancode ;
	}
	keyring [64] ;
	static int keyring_start = 0, keyring_tail = 0 ;

	static int last_mouse_x = -1, last_mouse_y = -1 ;

	if (!keytab_initialized) keytab_init() ;
	
	/* Actualizar eventos */
	
	keypress = 0 ;
	m = SDL_GetModState() ;

	/* El cambio de mouse.x/y afecta directamente al ratón */

	if (last_mouse_x != -1 && 
		(GLODWORD(MOUSEX) != last_mouse_x ||
		 GLODWORD(MOUSEY) != last_mouse_y))
	{
		SDL_WarpMouse ((Uint16)GLODWORD(MOUSEX), (Uint16)GLODWORD(MOUSEY)) ;
	}

	/* Procesa los eventos pendientes */
	/* Reset ascii and scancode if last key was released... */	
	/* must check all the linked equivs */

	keystate = SDL_GetKeyState(NULL) ;
	pressed = 0 ;
	if (GLODWORD(SCANCODE)!=0) {
		curr = &key_table[GLODWORD(SCANCODE)] ;
		while (curr!=NULL && pressed == 0) {
			if (keystate[curr->sdlk_equiv]) pressed = 1 ;
			curr=curr->next ;
		}
	}
	if (pressed==0) {
		GLODWORD(ASCII) = 0 ;
		GLODWORD(SCANCODE) = 0 ;
	}
	while (SDL_PollEvent (&e)) 
	{
		switch (e.type)
		{
			case SDL_MOUSEMOTION:
				GLODWORD(MOUSEX) = e.motion.x ;
				GLODWORD(MOUSEY) = e.motion.y ;
				break ;

			case SDL_JOYAXISMOTION:
				if (e.jaxis.which < MAX_JOYSTICKS && 
					e.jaxis.axis == 0)
				{
					joy_x[e.jaxis.which] = e.motion.x ;
					joy_y[e.jaxis.which] = e.motion.y ;
				}
				break ;

			case SDL_MOUSEBUTTONDOWN:
				if (e.button.button == 1) GLODWORD(MOUSELEFT)   = 1 ;
				if (e.button.button == 2) GLODWORD(MOUSEMIDDLE) = 1 ;
				if (e.button.button == 3) GLODWORD(MOUSERIGHT)  = 1 ;
				break ;

			case SDL_JOYBUTTONDOWN:
				if (e.jbutton.which < MAX_JOYSTICKS)
				{
					joy_b[e.jbutton.which] |= 
						(1 << e.jbutton.button) ;
				}
				break ;

			case SDL_MOUSEBUTTONUP:
				if (e.button.button == 1) GLODWORD(MOUSELEFT)   = 0 ;
				if (e.button.button == 2) GLODWORD(MOUSEMIDDLE) = 0 ;
				if (e.button.button == 3) GLODWORD(MOUSERIGHT)  = 0 ;
				break ;

			case SDL_JOYBUTTONUP:
				if (e.jbutton.which < MAX_JOYSTICKS)
				{
					joy_b[e.jbutton.which] &= 
						~(1 << e.jbutton.button) ;
				}
				break ;

			case SDL_KEYDOWN:

				/* Teclas del sistema ALT+... */

				if (e.key.keysym.sym == SDLK_d && (m & KMOD_LALT))
					instance_dump_all() ;
				if (e.key.keysym.sym == SDLK_x && (m & KMOD_LALT))
					must_exit = 1 ;

				/* Consola del sistema */

				if (e.key.keysym.sym == SDLK_c && (m & KMOD_LALT))
					show_console = !show_console ;
				if (show_console && e.key.keysym.sym == SDLK_PAGEUP)
				{
					gr_con_scroll (-1) ;
					break ;
				}
				else if (show_console && e.key.keysym.sym == SDLK_PAGEDOWN)
				{
					gr_con_scroll (1) ;
					break ;
				}
				else if (show_console && !(m & KMOD_LALT)) 
                                {
                                        gr_con_getkey (e.key.keysym.unicode,
                                                       e.key.keysym.sym) ;
					break ;
                                }

				/* Almacena la pulsación de la tecla */
				if (e.key.keysym.unicode) {
					if ((e.key.keysym.unicode & 0xFF80) == 0) {
						k =  sdl_equiv[e.key.keysym.unicode & 0x7F];
						/* NOTE: must skip asc trans for UCASE */
						if (k<0x61 && k>0x7A) {
							if (sdl_equiv[e.key.keysym.sym]!= k ) {
								// Add Unicode translation
								sdl_equiv[e.key.keysym.sym] = k ;
								add_key_equiv(e.key.keysym.sym,k) ;
							}
						} else {
							k = sdl_equiv[ e.key.keysym.sym ];
						}
					}
				} else {
					k = sdl_equiv[ e.key.keysym.sym ];
				}				
				m = e.key.keysym.mod ;

                if (!keypress) {
					GLODWORD(SCANCODE) = k ;
					if (e.key.keysym.unicode) {
						asc = win_to_dos[e.key.keysym.unicode & 0x7F] ;
					    /*ascii mayusculas */
					    if (asc >= 0X61 && GLODWORD(ASCII) <= 0X7A && (m & KMOD_LSHIFT || m & KMOD_RSHIFT || keystate[SDLK_CAPSLOCK]))
							asc -= 0x20 ;
					} else {
						asc = 0 ; /* NON PRINTABLE OR EXTENDED NOT IMPLEMENTED YET*/
					}
					GLODWORD(ASCII) = asc ;
					keypress = 1 ;
				} else {
					keyring[keyring_tail].scancode = k ;
					if (e.key.keysym.unicode) {
						asc = win_to_dos[e.key.keysym.unicode & 0x7F] ;
					    /*ascii mayusculas */
					    if (asc >= 0X61 && GLODWORD(ASCII) <= 0X7A && (m & KMOD_LSHIFT || m & KMOD_RSHIFT || keystate[SDLK_CAPSLOCK]))
							asc -= 0x20 ;
					} else {
						asc = 0 ; /* NON PRINTABLE */
					}
					keyring[keyring_tail].ascii = asc ;
					if (++keyring_tail == 64) keyring_tail = 0 ;
				}

				break ;

			case SDL_KEYUP:
				/* Do nothing, fenix is key_up unsensitive */
				break ;

			case SDL_QUIT:
				do_exit (-1) ;
		}
	}

	if (!keypress && keyring_start != keyring_tail)
	{
		GLODWORD(ASCII)    = keyring[keyring_start].ascii ;
		GLODWORD(SCANCODE) = keyring[keyring_start].scancode ;
		if (++keyring_start == 64) keyring_start = 0 ;
	}

	/* Now actualized every frame... */
	GLODWORD(SHIFTSTATUS) = 
		(m & KMOD_LSHIFT)                    ? 1 : 0 +
		(m & KMOD_RSHIFT)                    ? 2 : 0 +
		(m & KMOD_RCTRL) || (m & KMOD_LCTRL) ? 4 : 0 +
		(m & KMOD_LALT)  || (m & KMOD_RALT)  ? 8 : 0  ;

	last_mouse_x = GLODWORD(MOUSEX) ;
	last_mouse_y = GLODWORD(MOUSEY) ;
}


/* ---------------------------------------------------------------------- */
/* Inicialización y controles de tiempo                                   */
/* ---------------------------------------------------------------------- */

int last_frame_ms = 0 ;
int frame_count = 0 ;

static int last_frame_ticks = 0 ;
static int next_frame_ticks = 0 ;
static int frame_ms = 40 ;
static int max_jump = 2 ;
static int current_jump = 0 ;
static int jump = 0 ;
static int timer_advance = 0 ;
static int FPS_count = 0 ;
static int FPS_curr  = 0 ;
static int FPS_init  = 0 ;

void gr_set_fps(int fps, int jump)
{
	frame_ms = fps ? 1000 / fps : 1 ;
	max_jump = jump ;

	next_frame_ticks = SDL_GetTicks() + frame_ms ;
}

void gr_advance_timers()
{
	int * timer, i ;

	timer = &GLODWORD(TIMER) ;
	for (i = 0 ; i < 10 ; i++)
		timer[i] += timer_advance/10 ;
}

void gr_wait_frame()
{
	int frame_ticks, delay ;

	if (!scr_initialized) gr_init(320,200) ;

	frame_ticks = SDL_GetTicks() ;
	timer_advance %= 10 ;
	timer_advance += frame_ticks - last_frame_ticks ;

#ifdef DEBUG_X
	printf ("%04d:%4dms/%-4dmax.fps  %d chan  %d\r", frame_count+1,
			frame_ticks - last_frame_ticks, 
			GLODWORD(FPS), sound_channels_used(),
			next_frame_ticks) ;
	fflush (stdout) ;
#endif

	last_frame_ms = frame_ticks - last_frame_ticks ;
	GLODWORD(SPEED_GAUGE) = (frame_ticks-last_frame_ticks)*100/frame_ms ;
	last_frame_ticks = frame_ticks ;

	if (!jump) 
		FPS_count++ ;
	if (timer_advance > 9)
		FPS_curr += (int)100.0/(timer_advance/10) ;
	if (FPS_count && FPS_init < frame_ticks-1000)
	{
		GLODWORD(FPS) = FPS_curr / FPS_count ;
		FPS_count = 0 ;
		FPS_curr = 0 ;
		FPS_init = frame_ticks ;
	}

	jump = 0 ;
	frame_count ++ ;

	if (frame_ticks > next_frame_ticks)
	{
		if (frame_ticks > next_frame_ticks + frame_ms)
			next_frame_ticks = frame_ticks + frame_ms ;
		else
			next_frame_ticks += frame_ms ;

		if (current_jump < max_jump)
		{
			current_jump++ ;
			last_frame_ticks = frame_ticks ;
			jump = 1 ;
		}
		else
		{
			//next_frame_ticks = frame_ticks + frame_ms ;
			current_jump = 0 ;
		}
	}
	else
	{
		delay = next_frame_ticks - frame_ticks  ;
		GLODWORD(SPEED_GAUGE) = 100 - delay*100/frame_ms ;
		next_frame_ticks += frame_ms ;

		while (delay > 20)
		{
			SDL_Delay (20) ;
			delay -= 20 ;
		}
		if (delay) SDL_Delay (delay) ;

		current_jump = 0 ;
	}

	string_coalesce() ;	/* Libera cadenas temporales */
}

/* Rutinas gráficas de alto nivel */

GRAPH * get_graph (INSTANCE * i)
{
	int * xgraph, c, a ;

	if (LOCDWORD(i,XGRAPH))
	{
		xgraph = (int *) LOCDWORD(i,XGRAPH) ;
		c      = *xgraph++ ;
		if (c)
		{
			a = LOCDWORD(i,ANGLE) % 360000 ;
			if (a < 0) a += 360000 ;
			c = xgraph[a*c/360000] ;
			return bitmap_get (LOCDWORD(i,FILEID), c) ;
		}
	}

	if (LOCDWORD(i,GRAPHID))
		return bitmap_get (LOCDWORD(i,FILEID), LOCDWORD(i,GRAPHID)) ;

	return 0 ;
}

void get_instance_bbox (INSTANCE * i)
{
	REGION dest, * r ;
	GRAPH * gr ;
	int x, y, n, * ptr ;

	gr = get_graph (i) ;

	if (!gr) return ;

	if (LOCDWORD(i,REGIONID) >= 0 && LOCDWORD(i,REGIONID) <= 31)
		r = &regions[LOCDWORD(i,REGIONID)] ;
	else
		r = &regions[0] ;

	x = LOCDWORD(i, COORDX) ;
	y = LOCDWORD(i, COORDY) ;
	if (LOCDWORD(i, RESOLUTION))
	{
		x /= LOCDWORD(i, RESOLUTION) ;
		y /= LOCDWORD(i, RESOLUTION) ;
	}

	gr_get_bbox (&dest, r, x, y, LOCDWORD(i,ANGLE), LOCDWORD(i,GRAPHSIZE), gr) ;

	if (LOCDWORD(i, CTYPE) == 1)	/* c_scroll */
	{
		n = 0 ;
		if (LOCDWORD(i, CFLAGS)) 
		{
			while (!(LOCDWORD(i, CFLAGS) & (1 << n))) n++ ;
			n++ ;
		}
		ptr = &GLODWORD(SCROLLS) + 20 * n ;
		dest.x  -= ptr[0] ;
		dest.y  -= ptr[1] ;
		dest.x2 -= ptr[0] ;
		dest.y2 -= ptr[1] ;
	}

	LOCDWORD(i, BOX_X0) = dest.x ;
	LOCDWORD(i, BOX_Y0) = dest.y ;
	LOCDWORD(i, BOX_X1) = dest.x2 ;
	LOCDWORD(i, BOX_Y1) = dest.y2 ;
}


void draw_instance_at (INSTANCE * proc_ptr, REGION * region, int x, int y)
{
	INSTANCE * i = (INSTANCE *) proc_ptr ;
	GRAPH * map ;


	map = get_graph (i) ;
	if (!map) return ;
	if (map->flags & F_ANIMATION) bitmap_animate(map) ;

	// PATCH - XGRAPH DOES NOT ROTATE DESTINATION GRAPHIC
	if (LOCDWORD(i,ANGLE) || LOCDWORD(i,GRAPHSIZE) != 100) {
		if (LOCDWORD(i,XGRAPH) && LOCDWORD(i,GRAPHSIZE) == 100) {
			printf ("DRAW INSTANCE AT: WE ARE HERE! # %d #",LOCDWORD(i,FLAGS)) ;
			gr_blit (0, region, x, y, LOCDWORD(i,FLAGS), map) ;
		} else {
			if (LOCDWORD(i,XGRAPH)) {
				gr_rotated_blit (0, region, x, y, LOCDWORD(i,FLAGS), 0, LOCDWORD(i,GRAPHSIZE), map) ;
			} else {
				gr_rotated_blit (0, region, x, y, LOCDWORD(i,FLAGS), LOCDWORD(i,ANGLE), LOCDWORD(i,GRAPHSIZE), map) ;
			}
		}
	} else {
		gr_blit (0, region, x, y, LOCDWORD(i,FLAGS), map) ;
	}
	
}

void draw_instance (INSTANCE * proc_ptr)
{
	INSTANCE * i = (INSTANCE *) proc_ptr ;
	GRAPH * map ;
	int x, y, r ;

	map = get_graph (i) ;
	if (!map) return ;
	if (map->flags & F_ANIMATION) bitmap_animate(map) ;

	r = LOCDWORD(i,REGIONID);
	if (r < 1 || r > 31) r = 0 ;

	x = LOCDWORD(i, COORDX) ;
	y = LOCDWORD(i, COORDY) ;
	if (LOCDWORD(i, RESOLUTION))
	{
		x /= LOCDWORD(i, RESOLUTION) ;
		y /= LOCDWORD(i, RESOLUTION) ;
	}
	// PATCHED FOR XGRAPH NOT RATATION GRAPHIC	
	if (LOCDWORD(i,ANGLE) || LOCDWORD(i,GRAPHSIZE) != 100) {		
		if (LOCDWORD(i,XGRAPH) && LOCDWORD(i,GRAPHSIZE) == 100) {
			gr_blit (0, &regions[r], x, y, LOCDWORD(i,FLAGS), map) ;
		} else {
			if (LOCDWORD(i,XGRAPH)) {
				gr_rotated_blit (0, &regions[r], x, y, LOCDWORD(i,FLAGS), 0, LOCDWORD(i,GRAPHSIZE), map) ;
			} else {
				gr_rotated_blit (0, &regions[r], x, y, LOCDWORD(i,FLAGS), LOCDWORD(i,ANGLE), LOCDWORD(i,GRAPHSIZE), map) ;
			}
		}
	} else {
		gr_blit (0, &regions[r], x, y, LOCDWORD(i,FLAGS), map) ;
	}
}

void draw_mouse (void * ptr)
{
	GRAPH * map ;
	int r ;

	map = bitmap_get (GLODWORD(MOUSEFILE), GLODWORD(MOUSEGRAPH)) ;

	if (!map) return ;
	if (map->flags & F_ANIMATION) bitmap_animate(map) ;

	r = GLODWORD(MOUSEREGION) ;
	if (r < 0 || r > 31) r = 0 ;
	
	if (GLODWORD(MOUSEANGLE) || GLODWORD(MOUSESIZE) != 100)
		gr_rotated_blit (0, &regions[r], GLODWORD(MOUSEX),
				 GLODWORD(MOUSEY), GLODWORD(MOUSEFLAGS),
				 GLODWORD(MOUSEANGLE), GLODWORD(MOUSESIZE), map) ;
	else
		gr_blit (0, &regions[r], GLODWORD(MOUSEX),
			 GLODWORD(MOUSEY), GLODWORD(MOUSEFLAGS), map) ;

}

/* Proceso principal de dibujo: llama a todas las funciones de dibujo,
 * ordenadas por coordenada Z, incluyendo dibujo de textos y demás */

typedef struct
{
	int z ;
	void (*draw)(void *) ;
	void * what ;
}
action ;

int compare_actions (const void * ptr1, const void * ptr2)
{
	action * a1 = (action *)ptr1 ;
	action * a2 = (action *)ptr2 ;

	return a2->z - a1->z ;
}

void draw_texts (void * ptr)
{
	gr_text_put_all() ;
}

void draw_mode7 (void * ptr)
{
	gr_mode7_draw ((int)ptr) ;
}

void draw_scroll (void * ptr)
{
	gr_scroll_draw ((int)ptr, 1) ;
}

void draw_fli (void * ptr)
{
	if ( (current_fli = flic_do_frame (current_fli)) )
	gr_blit (0, 0, current_fli_x + current_fli->bitmap->width/2, 
		       current_fli_y + current_fli->bitmap->height/2, 
		       0, current_fli->bitmap) ;
}

void gr_draw_frame ()
{
	INSTANCE * i ;
	int a_reserved = 0 ;
	int a_count = 0 ;
	int a, n, step ;
	static action * alist = 0 ;

        if (jump)
        {
                do_events() ;		/* Recoge teclas y demás     */
                current_jump++ ;
                return ;
        }

        /* Actualiza la paleta */

	if (palette_changed)
		gr_refresh_palette() ;
	if (fade_on != 0) 
		gr_fade_step() ;

        GLODWORD(FADING) = (fade_on != 0) ;

        /* Bloquea el bitmap de pantalla */

	if (gr_lock_screen() < 0) return ;

	/* Dibuja el fondo, si no está tapado por planos de scroll */

	if (!gr_scroll_is_fullscreen())
	{
		if (enable_16bits && background_8bits_used)
		{
			Uint16 * ptr  = scrbitmap->data, * lptr ;
			Uint8  * orig = background_8bits->data ;

			for (a = 0 ; a < background_8bits->height ; a++)
			{
				lptr = ptr ;
				for (n = background_8bits->width ; n > 0 ; n--)
					*ptr++ = colorequiv[*orig++] ;
				ptr  = lptr + scrbitmap->width ;
			}
		}
		else if (enable_16bits)
		{
			Uint16 * ptr, * orig ;
			const int pixelsize = enable_16bits ? 2:1 ;

			ptr  = scrbitmap->data ;
			orig = background->data ;
			step = background->width * pixelsize ;
			if (background_is_black)
			{
				for (a = 0 ; a < background->height ; a++)
				{
					memset (ptr, 0, step) ;
					ptr  += scrbitmap->width ;
				}
			}
			else
			{
				for (a = 0 ; a < background->height ; a++)
				{
					memmove (ptr, orig, step) ;
					orig += background->width ;
					ptr  += scrbitmap->width ;
				}
			}
		}
		else
		{
			Uint8  * ptr, * orig ;
			const int pixelsize = 1 ;

			ptr  = scrbitmap->data ;
			orig = background->data ;
			step = background->width * pixelsize ;
			if (background_is_black)
			{
				for (a = 0 ; a < background->height ; a++)
				{
					memset (ptr, 0, step) ;
					ptr  += scrbitmap->width ;
				}
			}
			else
			{
				for (a = 0 ; a < background->height ; a++)
				{
					memcpy (ptr, orig, step) ;
					orig += background->width ;
					ptr  += scrbitmap->width ;
				}
			}
		}
	}

	/* Crea una lista de acciones donde añade todos los procesos */

	i = first_instance ;
	while (i)
	{
		if ((LOCDWORD(i,GRAPHID) || LOCDWORD(i,XGRAPH)) && LOCDWORD(i,CTYPE) == 0 &&
			(LOCDWORD(i,STATUS) == STATUS_RUNNING ||
			 LOCDWORD(i,STATUS) == STATUS_FROZEN))
		{
			if (a_reserved == a_count)
			{
				a_reserved += 16 ;
				alist = (action *) realloc (alist, sizeof(action) * a_reserved) ;
				assert (alist) ;
			}
			alist[a_count].z = LOCDWORD(i,COORDZ) ;
			alist[a_count].draw = ( void (*)(void *) ) draw_instance ;
			alist[a_count].what = i ;
			a_count++ ;
		}

		i = i->next ;
	}

	if (a_reserved <= a_count+13)
	{
		a_reserved = a_count+16 ;
		alist = (action *) realloc (alist, sizeof(action) * a_reserved) ;
		assert (alist) ;
	}

	/* Añade las acciones fijas internas, como dibujar textos */

	alist[a_count].z = GLODWORD(TEXTZ) ;
	alist[a_count].draw = draw_texts ;
	a_count++ ;

	if (GLODWORD(MOUSEGRAPH))
	{
		alist[a_count].z = GLODWORD(MOUSEZ) ;
		alist[a_count].draw = draw_mouse ;
		a_count++ ;
	}

	if (current_fli)
	{
		alist[a_count].z = 0 ;
		alist[a_count].draw = draw_fli ;
		a_count++ ;
	}

	/* Añade los planos de scroll que haya activos */

	for (n = 0 ; n < 10 ; n++)
	{
		if (gr_scroll_active(n))
		{
			alist[a_count].z = *(int *)(&GLODWORD(SCROLLS) + 10*n + 4) ;
			alist[a_count].draw = draw_scroll ;
			alist[a_count].what = (void *)n ;
			a_count++ ;
		}
	}

	for (n = 0 ; n < 10 ; n++)
	{
		if (gr_mode7_active(n))
		{
			alist[a_count].z = *(int *)(&GLODWORD(M7STRUCTS) + 10*n + 5) ;
			alist[a_count].draw = draw_mode7 ;
			alist[a_count].what = (void *)n ;
			a_count++ ;
		}
	}

	/* Ordena por Z la lista y ejecuta cada acción */

	qsort (alist, a_count, sizeof(action), compare_actions) ;

	for (a = 0 ; a < a_count ; a++)
		(*alist[a].draw) (alist[a].what) ;

        /* Visualiza la consola */

        gr_con_show(show_console) ;
        gr_con_draw() ;

	/* Actualiza la paleta y la pantalla */

	gr_unlock_screen() ;

	do_events() ;		/* Recoge teclas y demás     */
}

/* FUnción de inicialización de la librería gráfica */

static int screen_locked = 0 ;

int gr_lock_screen()
{
	if (screen_locked) return 1 ;
	screen_locked = 1 ;

	if (SDL_LockSurface (screen) < 0)
		return -1 ;

	if (screen->flags & SDL_DOUBLEBUF)
	{
		if (!scrbitmap)
		{
			scrbitmap = bitmap_new (0, screen->pitch, screen->h, 
					enable_16bits ? 16:8) ;
			bitmap_add_cpoint (scrbitmap, 0, 0) ;
			free (scrbitmap->data) ;
		}

		scrbitmap_is_fake = 1 ;
		scrbitmap->data   = screen->pixels ;
		scrbitmap->height = screen->h ;

		if (enable_16bits)
		{
			scrbitmap->width  = screen->pitch/2 ;
			scrbitmap->pitch  = screen->pitch/2 ;
			scrbitmap->depth  = 16 ;
		}
		else
		{
			scrbitmap->width  = screen->pitch ;
			scrbitmap->pitch  = screen->pitch ;
			scrbitmap->depth  = 8 ;
		}
	}
	else
	{
		scrbitmap_is_fake = 0 ;

		if (!scrbitmap) 
		{
			scrbitmap = bitmap_new (0, screen->w, screen->h, 
				enable_16bits ? 16:8) ;
			bitmap_add_cpoint (scrbitmap, 0, 0) ;
		}
	}

	return 1 ;
}

void gr_unlock_screen()
{
	int a ;

	if (!screen_locked) return ;
	screen_locked = 0 ;

	if (screen->flags & SDL_DOUBLEBUF)
	{
		SDL_UnlockSurface (screen) ;

		scrbitmap->data = 0 ;
		SDL_Flip(screen) ;
	}
	else
	{
		if (enable_16bits)
		{
			Uint16 * ptr, * orig ;

			orig = scrbitmap->data ;
			ptr  = screen->pixels ;

			if (enable_filtering)
			{
				int n;

				for (a = 0 ; a < screen->h ; a++)
				{
					ptr  = (Uint16 *)screen->pixels + screen->pitch * a / 2 ;

					for (n = 0 ; n < screen->w-1 ; n++)
					{
						*ptr = colorghost[orig[0]] 
						     + colorghost[orig[1]] ;

						ptr++, orig++ ;
					} 
					*ptr++ = *orig++ ;
				}
			}
			else
			{
				for (a = 0 ; a < screen->h ; a++)
				{
					memcpy (ptr, orig, screen->w * 2) ;
					orig += scrbitmap->width ;
					ptr  += (screen->pitch / 2) ;
				}
			}
		}
		else
		{
			Uint8 * ptr, * orig ;

			orig = scrbitmap->data ;
			ptr  = screen->pixels ;
			for (a = 0 ; a < screen->h ; a++)
			{
				memcpy (ptr, orig, screen->w) ;
				orig += scrbitmap->width ;
				ptr  += (screen->pitch) ;
			}
		}

		SDL_UnlockSurface (screen) ;
		SDL_UpdateRect (screen, 0, 0, 0, 0) ;
	}
}

/* Paleta de colores por defecto */

static int defpal[] = 
{
        0,  0,  0,  16, 16, 16,  32, 32, 32,  48, 48, 48,
       64, 64, 64,  84, 84, 84, 100,100,100, 116,116,116,
      132,132,132, 148,148,148, 168,168,168, 184,184,184,
      200,200,200, 216,216,216, 232,232,232, 252,252,252,
       40,  0,  0,  72,  0,  0, 108,  0,  0, 144,  0,  0,
      180,  0,  0, 216,  0,  0, 252,  0,  0, 252, 32,  0,
      252, 64,  0, 252, 96,  0, 252,128,  0, 252,160,  0,
      252,180, 56, 252,200,112, 252,220,168, 252,240,224,
        0, 40,  0,   0, 60,  0,   0, 84,  0,   0,108,  0,
        0,132,  0,   0,156,  0,   0,180,  0,   0,204,  0,
        0,228,  0,   0,252,  0,  48,252, 32,  96,252, 64,
      144,252, 96, 192,252,132, 216,252,176, 240,252,220,
        0,  0, 40,   0,  0, 72,   0,  0,104,   0,  0,140,
        0,  0,172,   0,  0,208,   0,  0,252,   0, 48,252,
        0,100,252,   0,148,252,   0,200,252,   0,252,252,
       56,252,252, 112,252,252, 168,252,252, 224,252,252,
       28, 28,  0,  52, 52,  0,  76, 76,  0, 100,100,  0,
      124,124,  0, 152,152,  0, 176,176,  0, 200,200,  0,
      224,224,  0, 252,252,  0, 252,252, 36, 252,252, 72,
      252,252,108, 252,252,144, 252,252,180, 252,252,220,
       28,  0, 28,  52,  0, 52,  76,  0, 76, 100,  0,100,
      124,  0,124, 152,  0,152, 176,  0,176, 200,  0,200,
      224,  0,224, 252,  0,252, 252, 36,252, 252, 72,252,
      252,112,252, 252,148,252, 252,184,252, 252,224,252,
        0, 20, 20,   0, 40, 40,   0, 60, 60,   0, 80, 80,
        0,104,100,   0,124,120,   0,144,144,   0,164,164,
        0,188,184,   0,208,204,   0,228,224,   0,252,248,
       44,252,248,  92,252,248, 140,252,248, 188,252,248,
       24, 12,  0,  44, 24,  0,  64, 36,  0,  84, 48,  0,
      108, 60,  0, 128, 72,  0, 148, 84,  0, 168, 96,  0,
      192,112,  0, 196,128, 28, 204,144, 60, 212,160, 92,
      216,180,120, 224,196,152, 232,212,184, 240,232,216,
       24, 12, 12,  40, 20, 20,  60, 32, 32,  80, 44, 44,
       96, 52, 52, 116, 64, 64, 136, 76, 76, 156, 88, 88,
      176,104,104, 196,120,120, 216,136,136, 240,152,152,
      240,168,168, 244,188,188, 244,204,204, 248,224,224,
       24, 20, 12,  44, 36, 24,  68, 52, 36,  88, 72, 48,
      112, 88, 60, 132,104, 72, 156,124, 88, 172,140,100,
      188,156,112, 204,176,124, 220,192,136, 240,212,152,
      240,216,168, 244,224,188, 244,232,204, 248,240,224,
       32,  8,  0,  60, 16,  0,  88, 28,  0, 120, 36,  0,
      148, 48,  0, 176, 56,  0, 208, 68,  0, 216, 88,  0,
      224,112,  0, 232,136,  0, 240,160,  0, 252,184,  0,
      252,200, 56, 252,216,112, 252,232,168, 252,252,224,
       20, 12, 12,  36, 24, 24,  56, 36, 36,  72, 48, 48,
       92, 64, 64, 108, 76, 76, 128, 88, 88, 144,100,100,
      164,116,116, 172,132,132, 184,148,148, 192,164,164,
      204,180,180, 212,196,196, 224,212,212, 236,232,232,
       12, 20, 12,  24, 36, 24,  36, 56, 36,  48, 72, 48,
       64, 92, 64,  76,108, 76,  88,128, 88, 100,144,100,
      116,164,116, 132,172,132, 148,184,148, 164,192,164,
      180,204,180, 196,212,196, 212,224,212, 232,236,232,
       12, 12, 16,  24, 24, 32,  36, 36, 48,  48, 48, 64,
       64, 64, 80,  76, 76, 96,  88, 88,112, 100,100,128,
      116,116,148, 132,132,160, 148,148,172, 164,164,184,
      180,180,196, 196,196,208, 212,212,220, 232,232,236,
       40,  0,  0,  80,  0,  0, 124,  0,  0, 164,  0,  0,
      208,  0,  0, 252,  0,  0, 252, 40,  0, 252, 84,  0,
      252,124,  0, 252,168,  0, 252,208,  0, 252,252,  0,
      252,252, 44, 252,252, 92, 252,252,140, 252,252,188,
        0,  0,  0,   0,  0, 88,   0,  0,128,   0,  0,168,
        0,  0,208,   0,  0,248,  40,  0,248,  84,  0,248,
      124,  0,248, 168,  0,248, 208,  0,248, 252,  0,252,
      252, 52,252, 252,108,252, 252,164,252, 252,220,252
} ;

void gr_init(int width, int height)
{
	int n ;
	SDL_Surface *ico ;

	if (!scr_initialized) scr_initialized = 1 ;

	if (globaldata)
		enable_16bits = (GLODWORD(GRAPH_MODE) & 0x01) ? 1 : 0 ;

	/* Inicializa el modo gráfico */

	if (scr_initialized && scrbitmap)
	{
		if (scrbitmap_is_fake)
			free (scrbitmap) ;
		else
			bitmap_destroy (scrbitmap) ;

		scrbitmap = 0 ;
	}

	scr_width = width ;
	scr_height = height ;

	// Aqui las llamadas a SDL_WM funcs...
	SDL_WM_SetCaption(apptitle,"") ;
	// Icono...
	// Necesitamos crar una surface a partir de un MAP generico de 16x16...	
	if (icono) {				
		if (icono->depth == 8) {
			ico = SDL_CreateRGBSurfaceFrom(icono->data,32,32,8,32,0x00,0x00,0x00,0x00) ;
			SDL_SetPalette(ico, SDL_LOGPAL , palette, 0, 256);
		} else {
			ico = SDL_CreateRGBSurfaceFrom(icono->data,32,32,16,64,0xF800,0x07E0,0x001F,0x00) ;
		}
		printf("ICO %d\n", ico) ;
		SDL_SetColorKey(ico, SDL_SRCCOLORKEY, SDL_MapRGB(ico->format,0,0,0)) ;
		SDL_WM_SetIcon(ico, NULL);
		SDL_FreeSurface(ico) ;		
	}	

	screen = SDL_SetVideoMode (scr_width, scr_height, 
		  (enable_16bits ? 16:8), 
		  (full_screen   ? SDL_FULLSCREEN : 0) 
		| (double_buffer ? SDL_DOUBLEBUF : 0) 
		| SDL_HWSURFACE | SDL_HWPALETTE) ;

	if (!screen)
	{
		gr_con_printf ("Modo grafico %dx%d no disponible: %s\n",
				scr_width, scr_height, SDL_GetError()) ;

		do_exit(1);
	}

	if (enable_16bits)
	{
		if (screen->format->BytesPerPixel != 2)
		{
			printf ("Profundidad de color de 16 bits no soportada\n") ;
			do_exit(1) ;
		}
		for (n = 0 ; n < 65536 ; n++)
		{
			colorghost[n] = 
			     (((n & screen->format->Rmask) >> 1) & screen->format->Rmask) 
			   + (((n & screen->format->Gmask) >> 1) & screen->format->Gmask) 
			   + (((n & screen->format->Bmask) >> 1) & screen->format->Bmask) ;
		}
	}

        if (report_graphics)
                gr_con_printf ("[GRAPH] Graphic mode started - %dx%d - %s\n", 
		scr_width, scr_height, enable_16bits ? "16 bits":"8 bits") ;

	if (enable_16bits)
	{
                Uint32 m, Rbits = 0, Gbits = 0, Bbits = 0 ;

                for (m = screen->format->Rmask; m; m >>= 1) Rbits += (m&1) ;
                for (m = screen->format->Gmask; m; m >>= 1) Gbits += (m&1) ;
                for (m = screen->format->Bmask; m; m >>= 1) Bbits += (m&1) ;

                if (report_graphics)
                        gr_con_printf ("[GRAPH] RGB %d%d%d: "
                                "Masks R=0x%04X G=0x%04X B=0x%04X\n",
                                Rbits, Gbits, Bbits,
				screen->format->Rmask,
				screen->format->Gmask,
				screen->format->Bmask) ;
	}
	if (report_graphics && (screen->flags & SDL_DOUBLEBUF))
		gr_con_printf ("[GRAPH] Doble buffer activo\n") ;
	fflush(stdout) ;

	SDL_ShowCursor (0) ;

	regions[0].x  = 0 ;
	regions[0].y  = 0 ;
	regions[0].x2 = scr_width-1 ;
	regions[0].y2 = scr_height-1 ;

	/* Bitmaps de fondo */

	if (background) bitmap_destroy (background) ;
	background = bitmap_new (0, width, height, enable_16bits ? 16:8) ;
	assert (background) ;
	gr_clear (background) ;
	bitmap_add_cpoint (background, 0, 0) ;

	if (enable_16bits)
	{
		if (background_8bits) bitmap_destroy (background_8bits) ;
		background_8bits = bitmap_new (0, width, height, 8) ;
		assert (background_8bits) ;
		gr_clear (background_8bits) ;
		bitmap_add_cpoint (background_8bits, 0, 0) ;
		background_8bits_used = 0 ;
	}

	background_is_black = 1 ;

	/* Paleta de colores por defecto */

	if (!palette_loaded)
	{
		for (n = 0 ; n < 256 ; n++) 
			gr_set_rgb (n, defpal[n*3]/4, defpal[n*3+1]/4, 
					defpal[n*3+2]/4) ;
	}

	gr_refresh_palette() ;

	next_frame_ticks = SDL_GetTicks() + frame_ms ;

	scr_initialized = 2 ;
}

#ifndef LINUX
#include <windows.h>
#endif

void gr_error (const char *fmt, ...)
{
	char text[4000] ;

	va_list ap;
	va_start(ap, fmt);
	vsprintf(text, fmt, ap);
	va_end(ap);
	
#ifndef LINUX
	MessageBox (0, text, "FXI", MB_OK);
#else
	fprintf (stderr, "%s\n", text) ;
#endif
	do_exit (1) ;
}

