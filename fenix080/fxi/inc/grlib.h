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

#ifndef __GRLIB_H
#define __GRLIB_H

#include "instance_st.h"
#include "grlib_st.h"

/* -------------------------------------------------------------------- */
/* Librería gráfica                                                     */
/* -------------------------------------------------------------------- */

extern int              scr_initialized ;
extern REGION           regions[32] ;
extern GRAPH            * background ;
extern GRAPH            * scrbitmap ;
extern GRLIB            * syslib ;
extern GRAPH            * icono ;
extern int              last_frame_ms ;
extern int              joy_x[], joy_y[], joy_b[] ;
extern char             * apptitle ;
extern key_equiv        key_table[] ;
extern unsigned char    * keystate ;
extern int              keytab_initialized ;

extern int              scr_width ;
extern int              scr_height ;

extern int              full_screen ;
extern int              double_buffer ;

/* Inicialización y control de tiempo */
/* ---------------------------------- */

extern void gr_init           (int w, int h) ;
extern void gr_set_fps        (int fps, int max_jump) ;
extern void gr_wait_frame     () ;
extern void gr_advance_timers () ;
extern void gr_draw_frame     () ;
extern void keytab_init       () ;
extern void keytab_free       () ;

/* Paleta de colores */
/* ----------------- */

extern SDL_Color    vpalette[256] ;
extern SDL_Color    palette[256] ;

extern Uint16       colorequiv[256]   ;     /* Equivalencia paleta -> pantalla   */
extern Uint16       colorghost[65536] ;     /* Deja un color a 50% de intensidad */

extern GRAPH      * background_8bits ;
extern int          background_8bits_used ;
extern int          background_is_black ;

extern Uint8        trans_table[256][256] ; /* Tabla de transparencias 8 bits        */
extern int          trans_table_updated ;   /* 1 = La tabla es utilizable            */

extern void         gr_make_trans_table() ;

extern int          palette_loaded ;        /* ¿Se ha cargado ya la paleta inicial ? */
extern int          palette_changed ;       /* Poner a 1 cuando se cambien colores   */
extern int          fade_on ;               /* ¿Hay un fade activo?                  */
extern int          fade_step ;             /* Si lo hay, posición (0=off)           */

extern int          gr_read_pal           (file * file) ;
extern void         gr_refresh_palette    () ;
extern void         gr_fade_init          (int r, int g, int b, int speed, int dir) ;
extern void         gr_fade_step          () ;
extern void         gr_roll_palette       (int color0, int num, int inc) ;
extern int          gr_find_nearest_color (int r, int g, int b) ;
extern void         gr_set_rgb            (int c, int r, int g, int b) ;
extern int          gr_rgb                (int r, int g, int b) ;
extern void         gr_get_rgb            (int color, int *r, int *g, int *b) ;
extern void         gr_set_colors         (int color, int num, Uint8 * pal) ;
extern void         gr_get_colors         (int color, int num, Uint8 * pal) ;

/* Blend ops */
/* --------- */

extern Sint16 * blend_create       () ;
extern void     blend_free         () ;
extern void     blend_init         (Sint16 * blend) ;
extern void     blend_translucency (Sint16 * blend, double ammount) ;
extern void     blend_intensity    (Sint16 * blend, double ammount) ;
extern void     blend_tint         (Sint16 * blend, double ammount, Uint8 cr, Uint8 cg, Uint8 cb) ;
extern void     blend_swap         (Sint16 * blend) ;
extern void     blend_assign       (GRAPH * bitmap, Sint16 * blend) ;
extern void     blend_apply        (GRAPH * bitmap, Sint16 * blend) ;

/* Gestión de bitmaps y librerías de gráficos */
/* ------------------------------------------ */

extern int      gr_load_map       (const char * filename) ;
extern int      gr_load_png       (const char * filename) ;
extern int      gr_load_pcx       (const char * filename) ;
extern int      gr_load_fpg       (const char * filename) ;
extern int      gr_load_pal       (const char * filename) ;

extern int      grlib_new        () ;
extern GRLIB *  grlib_get        (int libid) ;
extern void     grlib_destroy    (int libid) ;
extern int      grlib_add_map    (GRLIB * lib, GRAPH * map) ;
extern int      grlib_unload_map (GRLIB * lib, int mapcode) ;

extern GRAPH * bitmap_new        (int code, int w, int h, int depth) ;
extern GRAPH * bitmap_clone      (GRAPH * this) ;
extern GRAPH * bitmap_new_syslib (int w, int h, int depth) ;
extern GRAPH * bitmap_get        (int libid, int mapcode) ;
extern void    bitmap_destroy    (GRAPH * map) ;
extern void    bitmap_add_cpoint (GRAPH *map, int x, int y) ;
extern void    bitmap_analize    (GRAPH * bitmap) ;
extern void    bitmap_animate    (GRAPH * bitmap) ;
extern void    bitmap_animate_to (GRAPH * bitmap, int pos, int speed) ;

/* Regiones */
/* -------- */

extern void region_define   (int region, int x, int y, int width, int height) ;
extern void region_union    (REGION * a, REGION * b) ;
extern int  region_is_empty (REGION * a) ;
extern int  region_is_out   (REGION * a, REGION * b) ;

/* Alto nivel */
/* ---------- */

extern void draw_instance_at (INSTANCE * i, REGION * r, int x, int y) ;
extern void draw_instance    (INSTANCE * i) ;
extern void get_instance_bbox(INSTANCE * i) ;
extern GRAPH * get_graph     (INSTANCE * i) ;
extern void scroll_region    (int nscroll, REGION * r) ;

/* Textos */
/* ------ */

extern int      gr_font_load       (char * filename) ;
extern int      gr_font_new8x8     (char * chardata, char color_on, char color_off) ;
extern int      gr_font_systemfont (char * chardata) ;
extern void     gr_font_destroy    (int fontid) ;

extern int      gr_text_new        (int fontid, int x, int y, int centered, const char * text) ;
extern int      gr_text_new_var    (int fontid, int x, int y, int centered, const void * var, int type) ;
extern void     gr_text_move       (int textid, int x, int y) ;
extern void     gr_text_destroy    (int textid) ;
extern int      gr_text_width      (int fontid, const unsigned char * text) ;
extern int      gr_text_height     (int fontid, const unsigned char * text) ;
extern void     gr_text_put        (GRAPH * dest, int fontid, int x, int y, const unsigned char * text) ;

extern GRAPH  * gr_text_bitmap     (int fontid, const char * text, int centered) ;

extern void     gr_text_put_all    () ;

/* Bajo nivel */
/* ---------- */

/* Las funciones gráficas admiten dest=0 para referirse a la pantalla.
 * Para poder usar esta funcionalidad, debe estar bloqueada antes */

extern int  gr_lock_screen   () ;
extern void gr_unlock_screen () ;

/* Primitivas gráficas */

extern void gr_clear     (GRAPH * dest) ;
extern void gr_clear_as  (GRAPH * dest, int color) ;
extern void gr_put_pixel (GRAPH * dest, int x, int y, int color) ;
extern int  gr_get_pixel (GRAPH * dest, int x, int y) ;

extern void gr_setcolor  (int c) ;
extern void gr_vline     (GRAPH * dest, REGION * clip, int x, int y, int h) ;
extern void gr_hline     (GRAPH * dest, REGION * clip, int x, int y, int w) ;
extern void gr_line      (GRAPH * dest, REGION * clip, int x, int y, int w, int h) ;
extern void gr_box       (GRAPH * dest, REGION * clip, int x, int y, int w, int h) ;
extern void gr_rectangle (GRAPH * dest, REGION * clip, int x, int y, int w, int h) ;
extern void gr_circle    (GRAPH * dest, REGION * clip, int x, int y, int r) ;
extern void gr_fcircle   (GRAPH * dest, REGION * clip, int x, int y, int r) ;

/* Bitmaps */

extern void gr_blit          (GRAPH * dest, REGION * clip, int x, int y, int flags, GRAPH * gr) ;
extern void gr_get_bbox      (REGION * dest, REGION * clip, int x, int y, int angle, int scale, GRAPH * gr) ;
extern void gr_rotated_blit  (GRAPH * dest, REGION * clip, int x, int y, int flags, int angle, int scale, GRAPH * gr) ;

/* Scroll */
/* ------ */

extern void gr_scroll_start         (int n, int fileid, int graphid, int backid, int region, int flags) ;
extern void gr_scroll_stop          (int n) ;
extern void gr_scroll_draw          (int n, int do_drawing) ;

extern int  gr_scroll_active        (int n) ;
extern int  gr_scroll_is_fullscreen () ;

/* Modo 7 */
/* ------ */

extern void gr_mode7_start  (int n, int fileid, int inid, int outid, int region, int inclination) ;
extern void gr_mode7_stop   (int n) ;
extern void gr_mode7_draw   (int n) ;
extern int  gr_mode7_active (int n) ;

/* Consola del sistema */
/* ------------------- */

extern void gr_sys_color   (int cfg, int cbg) ;
extern void gr_sys_puts    (GRAPH * map, int x, int y, Uint8 * str, int len) ;
extern void gr_sys_putchar (GRAPH * map, int ox, int oy, Uint8 c) ;

extern void gr_con_printf  (const char *fmt, ...) ;
extern void gr_con_putline (char * text) ;
extern void gr_con_show    (int doit) ;
extern void gr_con_draw    () ;
extern void gr_con_getkey  (int key, int sym) ;
extern void gr_con_scroll  (int direction) ;
extern void gr_con_do      (const char * command) ;

#endif
