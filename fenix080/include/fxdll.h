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

#define FNXDLL

#include <SDL.h>
#include <grlib_st.h>
#include <files_st.h>
#include <typedef_st.h>
#include <xctype_st.h>
#include <flic_st.h>
#include <i_procdef_st.h>
#include <offsets.h>
#include <pslang.h>

/*
 * Declaracion de funciones
 */

/*****************************************************
 * fxi.h
 *****************************************************/

int * _debug ;            /* 1 = Actovate debug options       */
int * _fxi ;              /* 1 = EXE module is FXI.EXE        */
int * _enable_16bits ;    /* 1 = 16bpp MODE on                */
int * _enable_filtering ; /* 1 = 16bpp filter MODE on         */

#define debug              ( * _debug             )
#define fxi                ( * _fxi               )
#define enable_16bits      ( * _enable_16bits     )
#define enable_filtering   ( * _enable_filtering  )

int * _report_audio ;
int * _report_string ;
int * _report_graphics ;

#define report_audio        (* _report_audio    )
#define report_string       (* _report_string   )
#define report_graphics     (* _report_graphics )

void   (* gr_error      )(const char *fmt, ...) ;
void   (* do_exit       )() ;
int    (* dcb_load      )(const char * filename) ;
char * (* getid         )(unsigned int code) ;
int    (* path_find     )(GRAPH * bitmap, int sx, int sy, int dx, int dy, int options) ;
int    (* path_get      )(int * x, int * y) ;
int    (* path_set_wall )(int n) ;

/*****************************************************
 * files.h
 *****************************************************/

file * (*file_open       )(const char * filename, char * mode) ;
int    (*file_read       )(file * fp, void * buffer, int len) ;
int    (*file_write      )(file * fp, void * buffer, int len) ;
int    (*file_gets       )(file * fp, char * buffer, int len) ;
int    (*file_puts       )(file * fp, const char * buffer) ;
int    (*file_size       )(file * fp) ;
int    (*file_pos        )(file * fp) ;
int    (*file_seek       )(file * fp, int pos, int where) ;
void   (*file_addp       )(const char * path) ;
void   (*file_close      )(file * fp) ;
int    (*file_exists     )(char * filename) ;
void   (*file_add_xfile  )(file * fp, long offset, char * name, int size) ;
int    (*file_eof        )(file * fp) ;
FILE * (*file_fp         )(file * fp) ;


/*****************************************************
 * strings.h
 *****************************************************/

void         (*string_init    )() ;
const char * (*string_get     )(int code) ;
void         (*string_dump    )() ;
void         (*string_load    )(file *) ;
void         (*string_save    )(file *) ;

int          (*string_new     )(const char * ptr) ;
void         (*string_use     )(int code) ;
void         (*string_discard )(int code) ;
int          (*string_add     )(int code1, int code2) ;
int          (*string_compile )(const char * * source) ;
int          (*string_itoa    )(int n) ;
int          (*string_ftoa    )(float n) ;
int          (*string_ptoa    )(void * n) ;
int          (*string_comp    )(int code1, int code2) ;
int          (*string_char    )(int n, int nchar) ;
int          (*string_substr  )(int code, int first, int last) ;
int          (*string_find    )(int code1, int code2) ;
int          (*string_ucase   )(int code1) ;
int          (*string_lcase   )(int code1) ;
void         (*string_coalesce)() ;
int          (*string_concat  )(int code1, char * str2) ;

/*****************************************************
 * xctype.h
 *****************************************************/

int           * _dos_chars ;		/* 1 = Código fuente en caracteres MS-DOS */
char          ** _c_type ;
unsigned char ** _c_upper ;
unsigned char ** _c_lower ;

unsigned char (*convert    )(unsigned char c) ;
void          (*init_c_type)() ;

unsigned char ** _dos_to_win ;
unsigned char ** _win_to_dos ;

#define dos_chars    (* _dos_chars )
#define c_type       (* _c_type    )
#define c_upper      (* _c_upper   )
#define c_lower      (* _c_lower   )

#define dos_to_win   (* _dos_to_win)
#define win_to_dos   (* _win_to_dos)

/*
 * typedef.h
 */

TYPEDEF      (*typedef_new      )(BASETYPE type) ;
TYPEDEF      (*typedef_enlarge  )(TYPEDEF base) ;
TYPEDEF      (*typedef_reduce   )(TYPEDEF base) ;
int          (*typedef_size     )(TYPEDEF t) ;
int          (*typedef_subsize  )(TYPEDEF t, int c) ;
void         (*typedef_describe )(char * buffer, TYPEDEF t) ;
TYPEDEF      (*typedef_pointer  )(TYPEDEF to) ;

TYPEDEF    * (*typedef_by_name  )(int code) ;
void         (*typedef_name     )(TYPEDEF t, int code) ;

/*****************************************************
 * flic.h
 *****************************************************/

FLIC * (*flic_open     )(const char * filename) ;
void   (*flic_destroy  )(FLIC * flic) ;
FLIC * (*flic_do_frame )(FLIC * flic) ;
void   (*flic_reset    )(FLIC * flic) ;

FLIC *  *_current_fli   ;
int     *_current_fli_x ;
int     *_current_fli_y ;

#define current_fli     (*_current_fli   )
#define current_fli_x   (*_current_fli_x )
#define current_fli_y   (*_current_fli_y )

/*
 * grlib.h
 */

int              * _scr_initialized ;
REGION          ** _regions ;
GRAPH           ** _background ;
GRAPH           ** _scrbitmap ;
GRLIB           ** _syslib ;
GRAPH           ** _icono ;
int              * _last_frame_ms ;
int             ** _joy_x,
                ** _joy_y,
                ** _joy_b;
char            ** _apptitle ;
key_equiv       ** _key_table ;
unsigned char   ** _keystate ;
int              * _keytab_initialized ;

int              * _scr_width ;
int              * _scr_height ;

int              * _full_screen ;
int              * _double_buffer ;

#define scr_initialized      ( * _scr_initialized    )
#define regions              ( * _regions            )
#define background           ( * _background         )
#define scrbitmap            ( * _scrbitmap          )
#define syslib               ( * _syslib             )
#define icono                ( * _icono              )
#define last_frame_ms        ( * _last_frame_ms      )
#define joy_x                ( * _joy_x              )
#define joy_y                ( * _joy_y              )
#define joy_b                ( * _joy_b              )
#define apptitle             ( * _apptitle           )
#define key_table            ( * _key_table          )
#define keystate             ( * _keystate           )
#define keytab_initialized   ( * _keytab_initialized )
#define scr_width            ( * _scr_width          )
#define scr_height           ( * _scr_height         )
#define full_screen          ( * _full_screen        )
#define double_buffer        ( * _double_buffer      )

/* Inicialización y control de tiempo */
/* ---------------------------------- */

void (*gr_init           )(int w, int h) ;
void (*gr_set_fps        )(int fps, int max_jump) ;
void (*gr_wait_frame     )() ;
void (*gr_advance_timers )() ;
void (*gr_draw_frame     )() ;
void (*keytab_init       )() ;
void (*keytab_free       )() ;

/* Paleta de colores */
/* ----------------- */

SDL_Color   ** _vpalette ;
SDL_Color   ** _palette ;

Uint16      ** _colorequiv ;       /* Equivalencia paleta -> pantalla   */
Uint16      ** _colorghost ;       /* Deja un color a 50% de intensidad */

GRAPH       ** _background_8bits ;
int          * _background_8bits_used ;
int          * _background_is_black ;

Uint8      *** _trans_table ;           /* Tabla de transparencias 8 bits        */
int          * _trans_table_updated ;   /* 1 = La tabla es utilizable            */

void  (*gr_make_trans_table)() ;

int          * _palette_loaded ;        /* ¿Se ha cargado ya la paleta inicial ? */
int          * _palette_changed ;       /* Poner a 1 cuando se cambien colores   */
int          * _fade_on ;               /* ¿Hay un fade activo?                  */
int          * _fade_step ;             /* Si lo hay, posición (0=off)           */

int   (*gr_read_pal           )(file * file) ;
void  (*gr_refresh_palette    )() ;
void  (*gr_fade_init          )(int r, int g, int b, int speed, int dir) ;
void  (*gr_fade_step          )() ;
void  (*gr_roll_palette       )(int color0, int num, int inc) ;
int   (*gr_find_nearest_color )(int r, int g, int b) ;
void  (*gr_set_rgb            )(int c, int r, int g, int b) ;
int   (*gr_rgb                )(int r, int g, int b) ;
void  (*gr_get_rgb            )(int color, int *r, int *g, int *b) ;
void  (*gr_set_colors         )(int color, int num, Uint8 * pal) ;
void  (*gr_get_colors         )(int color, int num, Uint8 * pal) ;

#define vpalette                     (* _vpalette                )
#define palette                      (* _palette                 )

#define colorequiv                   (* _colorequiv              )
#define colorghost                   (* _colorghost              )

#define background_8bits             (* _background_8bits        )
#define background_8bits_used        (* _background_8bits_used   )
#define background_is_black          (* _background_is_black     )

#define trans_table                  (* _trans_table             )
#define trans_table_updated          (* _trans_table_updated     )

#define palette_loaded               (* _palette_loaded          )
#define palette_changed              (* _palette_changed         )
#define fade_on                      (* _fade_on                 )
#define fade_step                    (* _fade_step               )

/* Blend ops */
/* --------- */

Sint16 * (*blend_create       )() ;
void     (*blend_free         )() ;
void     (*blend_init         )(Sint16 * blend) ;
void     (*blend_translucency )(Sint16 * blend, double ammount) ;
void     (*blend_intensity    )(Sint16 * blend, double ammount) ;
void     (*blend_tint         )(Sint16 * blend, double ammount, Uint8 cr, Uint8 cg, Uint8 cb) ;
void     (*blend_swap         )(Sint16 * blend) ;
void     (*blend_assign       )(GRAPH * bitmap, Sint16 * blend) ;
void     (*blend_apply        )(GRAPH * bitmap, Sint16 * blend) ;

/* Gestión de bitmaps y librerías de gráficos */
/* ------------------------------------------ */

int      (*gr_load_map      )(const char * filename) ;
int      (*gr_load_png      )(const char * filename) ;
int      (*gr_load_pcx      )(const char * filename) ;
int      (*gr_load_fpg      )(const char * filename) ;
int      (*gr_load_pal      )(const char * filename) ;

int      (*grlib_new        )() ;
GRLIB *  (*grlib_get        )(int libid) ;
void     (*grlib_destroy    )(int libid) ;
int      (*grlib_add_map    )(GRLIB * lib, GRAPH * map) ;
int      (*grlib_unload_map )(GRLIB * lib, int mapcode) ;

GRAPH * (*bitmap_new        )(int code, int w, int h, int depth) ;
GRAPH * (*bitmap_clone      )(GRAPH * this) ;
GRAPH * (*bitmap_new_syslib )(int w, int h, int depth) ;
GRAPH * (*bitmap_get        )(int libid, int mapcode) ;
void    (*bitmap_destroy    )(GRAPH * map) ;
void    (*bitmap_add_cpoint )(GRAPH *map, int x, int y) ;
void    (*bitmap_analize    )(GRAPH * bitmap) ;
void    (*bitmap_animate    )(GRAPH * bitmap) ;
void    (*bitmap_animate_to )(GRAPH * bitmap, int pos, int speed) ;

/* Regiones */
/* -------- */

void (*region_define   )(int region, int x, int y, int width, int height) ;
void (*region_union    )(REGION * a, REGION * b) ;
int  (*region_is_empty )(REGION * a) ;
int  (*region_is_out   )(REGION * a, REGION * b) ;

/* Alto nivel */
/* ---------- */

void     (*draw_instance_at  )(INSTANCE * i, REGION * r, int x, int y) ;
void     (*draw_instance     )(INSTANCE * i) ;
void     (*get_instance_bbox )(INSTANCE * i) ;
GRAPH  * (*get_graph         )(INSTANCE * i) ;
void     (*scroll_region     )(int nscroll, REGION * r) ;

/* Textos */
/* ------ */

int      (*gr_font_load       )(char * filename) ;
int      (*gr_font_new8x8     )(char * chardata, char color_on, char color_off) ;
int      (*gr_font_systemfont )(char * chardata) ;
void     (*gr_font_destroy    )(int fontid) ;

int      (*gr_text_new        )(int fontid, int x, int y, int centered, const char * text) ;
int      (*gr_text_new_var    )(int fontid, int x, int y, int centered, const void * var, int type) ;
void     (*gr_text_move       )(int textid, int x, int y) ;
void     (*gr_text_destroy    )(int textid) ;
int      (*gr_text_width      )(int fontid, const unsigned char * text) ;
int      (*gr_text_height     )(int fontid, const unsigned char * text) ;
void     (*gr_text_put        )(GRAPH * dest, int fontid, int x, int y, const unsigned char * text) ;

GRAPH  * (*gr_text_bitmap     )(int fontid, const char * text, int centered) ;

void     (*gr_text_put_all    )() ;

/* Bajo nivel */
/* ---------- */

/* Las funciones gráficas admiten dest=0 para referirse a la pantalla.
* Para poder usar esta funcionalidad, debe estar bloqueada antes */

int  (*gr_lock_screen   )() ;
void (*gr_unlock_screen )() ;

/* Primitivas gráficas */

void (*gr_clear     )(GRAPH * dest) ;
void (*gr_clear_as  )(GRAPH * dest, int color) ;
void (*gr_put_pixel )(GRAPH * dest, int x, int y, int color) ;
int  (*gr_get_pixel )(GRAPH * dest, int x, int y) ;

void (*gr_setcolor  )(int c) ;
void (*gr_vline     )(GRAPH * dest, REGION * clip, int x, int y, int h) ;
void (*gr_hline     )(GRAPH * dest, REGION * clip, int x, int y, int w) ;
void (*gr_line      )(GRAPH * dest, REGION * clip, int x, int y, int w, int h) ;
void (*gr_box       )(GRAPH * dest, REGION * clip, int x, int y, int w, int h) ;
void (*gr_rectangle )(GRAPH * dest, REGION * clip, int x, int y, int w, int h) ;
void (*gr_circle    )(GRAPH * dest, REGION * clip, int x, int y, int r) ;
void (*gr_fcircle   )(GRAPH * dest, REGION * clip, int x, int y, int r) ;

/* Bitmaps */

void (*gr_blit          )(GRAPH * dest, REGION * clip, int x, int y, int flags, GRAPH * gr) ;
void (*gr_get_bbox      )(REGION * dest, REGION * clip, int x, int y, int angle, int scale, GRAPH * gr) ;
void (*gr_rotated_blit  )(GRAPH * dest, REGION * clip, int x, int y, int flags, int angle, int scale, GRAPH * gr) ;

/* Scroll */
/* ------ */

void (*gr_scroll_start         )(int n, int fileid, int graphid, int backid, int region, int flags) ;
void (*gr_scroll_stop          )(int n) ;
void (*gr_scroll_draw          )(int n, int do_drawing) ;

int  (*gr_scroll_active        )(int n) ;
int  (*gr_scroll_is_fullscreen )() ;

/* Modo 7 */
/* ------ */

void (*gr_mode7_start  )(int n, int fileid, int inid, int outid, int region, int inclination) ;
void (*gr_mode7_stop   )(int n) ;
void (*gr_mode7_draw   )(int n) ;
int  (*gr_mode7_active )(int n) ;

/* Consola del sistema */
/* ------------------- */

void (*gr_sys_color   )(int cfg, int cbg) ;
void (*gr_sys_puts    )(GRAPH * map, int x, int y, Uint8 * str, int len) ;
void (*gr_sys_putchar )(GRAPH * map, int ox, int oy, Uint8 c) ;

void (*gr_con_printf  )(const char *fmt, ...) ;
void (*gr_con_putline )(char * text) ;
void (*gr_con_show    )(int doit) ;
void (*gr_con_draw    )() ;
void (*gr_con_getkey  )(int key, int sym) ;
void (*gr_con_scroll  )(int direction) ;
void (*gr_con_do      )(const char * command) ;

/*****************************************************
 * i_procdef.h
 *****************************************************/
int *_local_strings ;
int ** _localstr ;

#define local_strings   (* _local_strings )
#define localstr        (* _localstr      )

PROCDEF **_mainproc ;
PROCDEF *(*procdef_get)(int n) ;

#define mainproc    (*_mainproc)

SYSPROC   * (*sysproc_get )(int code) ;
int         (*sysproc_add )(char * name, char * paramtypes, int type, void * func) ;
void        (*sysproc_init)() ;

/*****************************************************
 * instance.h
 *****************************************************/

void     **_globaldata ;
void     **_localdata ;

int       *_local_size ;

INSTANCE **_first_instance ;
INSTANCE **_last_instance ;

#define globaldata      (*_globaldata     )
#define localdata       (*_localdata      )

#define local_size      (*_local_size     )

#define first_instance  (*_first_instance )
#define last_instance   (*_last_instance  )

/*****************************************************
 * sound.h
 *****************************************************/

void       (*sound_init          )();
void       (*sound_close         )();

int        (*load_mod            )(const char * filename) ;
int        (*play_mod            )(int id , int loops);
int        (*unload_mod          )(int id);
int        (*stop_mod            )();
int        (*pause_mod           )();
int        (*resume_mod          )();
int        (*is_playing_mod      )();
int        (*set_mod_volume      )(int volume);

int        (*load_wav            )(const char * filename) ;
int        (*play_wav            )(int id , int loops);
int        (*unload_wav          )(int id);
int        (*stop_wav            )(int id);
int        (*pause_wav           )(int id);
int        (*resume_wav          )(int id);
int        (*is_playing_wav      )(int id);
int        (*set_wav_volume      )(int id,int volume);

void       (*ini_musiccd         )();
void       (*cd_play             )(int track, int continuous) ;
void       (*cd_stop             )() ;
int        (*cd_playing          )() ;

/*****************************************************
 * instance.h
 *****************************************************/
int * _must_exit ;
#define must_exit   (*_must_exit )

int        (*instance_getid       )() ;
INSTANCE * (*instance_get         )(int id) ;
INSTANCE * (*instance_getfather   )(INSTANCE * i) ;
INSTANCE * (*instance_getson      )(INSTANCE * i) ;
INSTANCE * (*instance_getbigbro   )(INSTANCE * i) ;
INSTANCE * (*instance_getsmallbro )(INSTANCE * i) ;
INSTANCE * (*instance_new         )(PROCDEF * proc, INSTANCE * father) ;
INSTANCE * (*instance_duplicate   )(INSTANCE * i) ;
void       (*instance_destroy     )(INSTANCE * r) ;
void       (*instance_dump        )(INSTANCE * father, int indent) ;
void       (*instance_dump_all    )() ;

/* Las siguientes funciones son el punto de entrada del intérprete */
int        (*instance_go          )(INSTANCE * r) ;
void       (*instance_go_all      )() ;

/*****************************************************
 * Otros
 *****************************************************/

#ifndef WIN32
    #define FENIX_MainDLL       void
#else
    #define FENIX_MainDLL       void __stdcall
#endif

#define COMMON_PARAMS   void *(*FENIX_import)(char *name),\
                        void (*FENIX_export)(char * name, char * paramtypes, int type, void * func)

#define FENIX_DLLImport     \
    file_open                   = FENIX_import ( "file_open"                       ); \
    file_read                   = FENIX_import ( "file_read"                       ); \
    file_write                  = FENIX_import ( "file_write"                      ); \
    file_gets                   = FENIX_import ( "file_gets"                       ); \
    file_puts                   = FENIX_import ( "file_puts"                       ); \
    file_size                   = FENIX_import ( "file_size"                       ); \
    file_pos                    = FENIX_import ( "file_pos"                        ); \
    file_seek                   = FENIX_import ( "file_seek"                       ); \
    file_addp                   = FENIX_import ( "file_addp"                       ); \
    file_close                  = FENIX_import ( "file_close"                      ); \
    file_exists                 = FENIX_import ( "file_exists"                     ); \
    file_add_xfile              = FENIX_import ( "file_add_xfile"                  ); \
    file_eof                    = FENIX_import ( "file_eof"                        ); \
    file_fp                     = FENIX_import ( "file_fp"                         ); \
    string_init                 = FENIX_import ( "string_init"                     ); \
    string_get                  = FENIX_import ( "string_get"                      ); \
    string_dump                 = FENIX_import ( "string_dump"                     ); \
    string_load                 = FENIX_import ( "string_load"                     ); \
    string_new                  = FENIX_import ( "string_new"                      ); \
    string_use                  = FENIX_import ( "string_use"                      ); \
    string_discard              = FENIX_import ( "string_discard"                  ); \
    string_add                  = FENIX_import ( "string_add"                      ); \
    string_itoa                 = FENIX_import ( "string_itoa"                     ); \
    string_ftoa                 = FENIX_import ( "string_ftoa"                     ); \
    string_ptoa                 = FENIX_import ( "string_ptoa"                     ); \
    string_comp                 = FENIX_import ( "string_comp"                     ); \
    string_char                 = FENIX_import ( "string_char"                     ); \
    string_substr               = FENIX_import ( "string_substr"                   ); \
    string_find                 = FENIX_import ( "string_find"                     ); \
    string_ucase                = FENIX_import ( "string_ucase"                    ); \
    string_lcase                = FENIX_import ( "string_lcase"                    ); \
    string_coalesce             = FENIX_import ( "string_coalesce"                 ); \
    string_concat               = FENIX_import ( "string_concat"                   ); \
    _dos_chars                  = FENIX_import ( "dos_chars"                       ); \
    _c_type                     = FENIX_import ( "c_type"                          ); \
    _c_upper                    = FENIX_import ( "c_upper"                         ); \
    _c_lower                    = FENIX_import ( "c_lower"                         ); \
    convert                     = FENIX_import ( "convert"                         ); \
    init_c_type                 = FENIX_import ( "init_c_type"                     ); \
    _dos_to_win                 = FENIX_import ( "dos_to_win"                      ); \
    _win_to_dos                 = FENIX_import ( "win_to_dos"                      ); \
    flic_open                   = FENIX_import ( "flic_open"                       ); \
    flic_destroy                = FENIX_import ( "flic_destroy"                    ); \
    flic_do_frame               = FENIX_import ( "flic_do_frame"                   ); \
    flic_reset                  = FENIX_import ( "flic_reset"                      ); \
    _current_fli                = FENIX_import ( "current_fli"                     ); \
    _current_fli_x              = FENIX_import ( "current_fli_x"                   ); \
    _current_fli_y              = FENIX_import ( "current_fli_y"                   ); \
    _debug                      = FENIX_import ( "debug"                           ); \
    _fxi                        = FENIX_import ( "fxi"                             ); \
    _enable_16bits              = FENIX_import ( "enable_16bits"                   ); \
    _enable_filtering           = FENIX_import ( "enable_filtering"                ); \
    _report_string              = FENIX_import ( "report_string"                   ); \
    _report_graphics            = FENIX_import ( "report_graphics"                 ); \
    gr_error                    = FENIX_import ( "gr_error"                        ); \
    do_exit                     = FENIX_import ( "do_exit"                         ); \
    dcb_load                    = FENIX_import ( "dcb_load"                        ); \
    getid                       = FENIX_import ( "getid"                           ); \
    path_find                   = FENIX_import ( "path_find"                       ); \
    path_get                    = FENIX_import ( "path_get"                        ); \
    path_set_wall               = FENIX_import ( "path_set_wall"                   ); \
    _scr_initialized            = FENIX_import ( "scr_initialized"                 ); \
    _regions                    = FENIX_import ( "regions"                         ); \
    _background                 = FENIX_import ( "background"                      ); \
    _scrbitmap                  = FENIX_import ( "scrbitmap"                       ); \
    _syslib                     = FENIX_import ( "syslib"                          ); \
    _icono                      = FENIX_import ( "icono"                           ); \
    _last_frame_ms              = FENIX_import ( "last_frame_ms"                   ); \
    _joy_x                      = FENIX_import ( "joy_x"                           ); \
    _joy_y                      = FENIX_import ( "joy_y"                           ); \
    _joy_b                      = FENIX_import ( "joy_b"                           ); \
    _apptitle                   = FENIX_import ( "apptitle"                        ); \
    _key_table                  = FENIX_import ( "key_table"                       ); \
    _keystate                   = FENIX_import ( "keystate"                        ); \
    _keytab_initialized         = FENIX_import ( "keytab_initialized"              ); \
    _scr_width                  = FENIX_import ( "scr_width"                       ); \
    _scr_height                 = FENIX_import ( "scr_height"                      ); \
    _full_screen                = FENIX_import ( "full_screen"                     ); \
    _double_buffer              = FENIX_import ( "double_buffer"                   ); \
    gr_init                     = FENIX_import ( "gr_init"                         ); \
    gr_set_fps                  = FENIX_import ( "gr_set_fps"                      ); \
    gr_wait_frame               = FENIX_import ( "gr_wait_frame"                   ); \
    gr_advance_timers           = FENIX_import ( "gr_advance_timers"               ); \
    gr_draw_frame               = FENIX_import ( "gr_draw_frame"                   ); \
    keytab_init                 = FENIX_import ( "keytab_init"                     ); \
    keytab_free                 = FENIX_import ( "keytab_free"                     ); \
    _vpalette                   = FENIX_import ( "vpalette"                        ); \
    _palette                    = FENIX_import ( "palette"                         ); \
    _colorequiv                 = FENIX_import ( "colorequiv"                      ); \
    _colorghost                 = FENIX_import ( "colorghost"                      ); \
    _background_8bits           = FENIX_import ( "background_8bits"                ); \
    _background_8bits_used      = FENIX_import ( "background_8bits_used"           ); \
    _background_is_black        = FENIX_import ( "background_is_black"             ); \
    _trans_table                = FENIX_import ( "trans_table"                     ); \
    _trans_table_updated        = FENIX_import ( "trans_table_updated"             ); \
    gr_make_trans_table         = FENIX_import ( "gr_make_trans_table"             ); \
    _palette_loaded             = FENIX_import ( "palette_loaded"                  ); \
    _palette_changed            = FENIX_import ( "palette_changed"                 ); \
    _fade_on                    = FENIX_import ( "fade_on"                         ); \
    _fade_step                  = FENIX_import ( "fade_step"                       ); \
    gr_read_pal                 = FENIX_import ( "gr_read_pal"                     ); \
    gr_refresh_palette          = FENIX_import ( "gr_refresh_palette"              ); \
    gr_fade_init                = FENIX_import ( "gr_fade_init"                    ); \
    gr_fade_step                = FENIX_import ( "gr_fade_step"                    ); \
    gr_roll_palette             = FENIX_import ( "gr_roll_palette"                 ); \
    gr_find_nearest_color       = FENIX_import ( "gr_find_nearest_color"           ); \
    gr_set_rgb                  = FENIX_import ( "gr_set_rgb"                      ); \
    gr_rgb                      = FENIX_import ( "gr_rgb"                          ); \
    gr_get_rgb                  = FENIX_import ( "gr_get_rgb"                      ); \
    gr_set_colors               = FENIX_import ( "gr_set_colors"                   ); \
    gr_get_colors               = FENIX_import ( "gr_get_colors"                   ); \
    blend_create                = FENIX_import ( "blend_create"                    ); \
    blend_free                  = FENIX_import ( "blend_free"                      ); \
    blend_init                  = FENIX_import ( "blend_init"                      ); \
    blend_translucency          = FENIX_import ( "blend_translucency"              ); \
    blend_intensity             = FENIX_import ( "blend_intensity"                 ); \
    blend_tint                  = FENIX_import ( "blend_tint"                      ); \
    blend_swap                  = FENIX_import ( "blend_swap"                      ); \
    blend_assign                = FENIX_import ( "blend_assign"                    ); \
    blend_apply                 = FENIX_import ( "blend_apply"                     ); \
    gr_load_map                 = FENIX_import ( "gr_load_map"                     ); \
    gr_load_png                 = FENIX_import ( "gr_load_png"                     ); \
    gr_load_pcx                 = FENIX_import ( "gr_load_pcx"                     ); \
    gr_load_fpg                 = FENIX_import ( "gr_load_fpg"                     ); \
    gr_load_pal                 = FENIX_import ( "gr_load_pal"                     ); \
    grlib_new                   = FENIX_import ( "grlib_new"                       ); \
    grlib_get                   = FENIX_import ( "grlib_get"                       ); \
    grlib_destroy               = FENIX_import ( "grlib_destroy"                   ); \
    grlib_add_map               = FENIX_import ( "grlib_add_map"                   ); \
    grlib_unload_map            = FENIX_import ( "grlib_unload_map"                ); \
    bitmap_new                  = FENIX_import ( "bitmap_new"                      ); \
    bitmap_clone                = FENIX_import ( "bitmap_clone"                    ); \
    bitmap_new_syslib           = FENIX_import ( "bitmap_new_syslib"               ); \
    bitmap_get                  = FENIX_import ( "bitmap_get"                      ); \
    bitmap_destroy              = FENIX_import ( "bitmap_destroy"                  ); \
    bitmap_add_cpoint           = FENIX_import ( "bitmap_add_cpoint"               ); \
    bitmap_analize              = FENIX_import ( "bitmap_analize"                  ); \
    bitmap_animate              = FENIX_import ( "bitmap_animate"                  ); \
    bitmap_animate_to           = FENIX_import ( "bitmap_animate_to"               ); \
    region_define               = FENIX_import ( "region_define"                   ); \
    region_union                = FENIX_import ( "region_union"                    ); \
    region_is_empty             = FENIX_import ( "region_is_empty"                 ); \
    region_is_out               = FENIX_import ( "region_is_out"                   ); \
    draw_instance_at            = FENIX_import ( "draw_instance_at"                ); \
    draw_instance               = FENIX_import ( "draw_instance"                   ); \
    get_instance_bbox           = FENIX_import ( "get_instance_bbox"               ); \
    get_graph                   = FENIX_import ( "get_graph"                       ); \
    scroll_region               = FENIX_import ( "scroll_region"                   ); \
    gr_font_load                = FENIX_import ( "gr_font_load"                    ); \
    gr_font_new8x8              = FENIX_import ( "gr_font_new8x8"                  ); \
    gr_font_systemfont          = FENIX_import ( "gr_font_systemfont"              ); \
    gr_font_destroy             = FENIX_import ( "gr_font_destroy"                 ); \
    gr_text_new                 = FENIX_import ( "gr_text_new"                     ); \
    gr_text_new_var             = FENIX_import ( "gr_text_new_var"                 ); \
    gr_text_move                = FENIX_import ( "gr_text_move"                    ); \
    gr_text_destroy             = FENIX_import ( "gr_text_destroy"                 ); \
    gr_text_width               = FENIX_import ( "gr_text_width"                   ); \
    gr_text_height              = FENIX_import ( "gr_text_height"                  ); \
    gr_text_put                 = FENIX_import ( "gr_text_put"                     ); \
    gr_text_bitmap              = FENIX_import ( "gr_text_bitmap"                  ); \
    gr_text_put_all             = FENIX_import ( "gr_text_put_all"                 ); \
    gr_lock_screen              = FENIX_import ( "gr_lock_screen"                  ); \
    gr_unlock_screen            = FENIX_import ( "gr_unlock_screen"                ); \
    gr_clear                    = FENIX_import ( "gr_clear"                        ); \
    gr_clear_as                 = FENIX_import ( "gr_clear_as"                     ); \
    gr_put_pixel                = FENIX_import ( "gr_put_pixel"                    ); \
    gr_get_pixel                = FENIX_import ( "gr_get_pixel"                    ); \
    gr_setcolor                 = FENIX_import ( "gr_setcolor"                     ); \
    gr_vline                    = FENIX_import ( "gr_vline"                        ); \
    gr_hline                    = FENIX_import ( "gr_hline"                        ); \
    gr_line                     = FENIX_import ( "gr_line"                         ); \
    gr_box                      = FENIX_import ( "gr_box"                          ); \
    gr_rectangle                = FENIX_import ( "gr_rectangle"                    ); \
    gr_circle                   = FENIX_import ( "gr_circle"                       ); \
    gr_fcircle                  = FENIX_import ( "gr_fcircle"                      ); \
    gr_blit                     = FENIX_import ( "gr_blit"                         ); \
    gr_get_bbox                 = FENIX_import ( "gr_get_bbox"                     ); \
    gr_rotated_blit             = FENIX_import ( "gr_rotated_blit"                 ); \
    gr_scroll_start             = FENIX_import ( "gr_scroll_start"                 ); \
    gr_scroll_stop              = FENIX_import ( "gr_scroll_stop"                  ); \
    gr_scroll_draw              = FENIX_import ( "gr_scroll_draw"                  ); \
    gr_scroll_active            = FENIX_import ( "gr_scroll_active"                ); \
    gr_scroll_is_fullscreen     = FENIX_import ( "gr_scroll_is_fullscreen"         ); \
    gr_mode7_start              = FENIX_import ( "gr_mode7_start"                  ); \
    gr_mode7_stop               = FENIX_import ( "gr_mode7_stop"                   ); \
    gr_mode7_draw               = FENIX_import ( "gr_mode7_draw"                   ); \
    gr_mode7_active             = FENIX_import ( "gr_mode7_active"                 ); \
    gr_sys_color                = FENIX_import ( "gr_sys_color"                    ); \
    gr_sys_puts                 = FENIX_import ( "gr_sys_puts"                     ); \
    gr_sys_putchar              = FENIX_import ( "gr_sys_putchar"                  ); \
    gr_con_printf               = FENIX_import ( "gr_con_printf"                   ); \
    gr_con_putline              = FENIX_import ( "gr_con_putline"                  ); \
    gr_con_show                 = FENIX_import ( "gr_con_show"                     ); \
    gr_con_draw                 = FENIX_import ( "gr_con_draw"                     ); \
    gr_con_getkey               = FENIX_import ( "gr_con_getkey"                   ); \
    gr_con_scroll               = FENIX_import ( "gr_con_scroll"                   ); \
    gr_con_do                   = FENIX_import ( "gr_con_do"                       ); \
    _local_strings              = FENIX_import ( "local_strings"                   ); \
    _localstr                   = FENIX_import ( "localstr"                        ); \
    _mainproc                   = FENIX_import ( "mainproc"                        ); \
    procdef_get                 = FENIX_import ( "procdef_get"                     ); \
    sysproc_get                 = FENIX_import ( "sysproc_get"                     ); \
    sysproc_add                 = FENIX_import ( "sysproc_add"                     ); \
    sysproc_init                = FENIX_import ( "sysproc_init"                    ); \
    _globaldata                 = FENIX_import ( "globaldata"                      ); \
    _localdata                  = FENIX_import ( "localdata"                       ); \
    _local_size                 = FENIX_import ( "local_size"                      ); \
    _first_instance             = FENIX_import ( "first_instance"                  ); \
    _last_instance              = FENIX_import ( "last_instance"                   ); \
    _must_exit                  = FENIX_import ( "must_exit"                       ); \
    instance_getid              = FENIX_import ( "instance_getid"                  ); \
    instance_get                = FENIX_import ( "instance_get"                    ); \
    instance_getfather          = FENIX_import ( "instance_getfather"              ); \
    instance_getson             = FENIX_import ( "instance_getson"                 ); \
    instance_getbigbro          = FENIX_import ( "instance_getbigbro"              ); \
    instance_getsmallbro        = FENIX_import ( "instance_getsmallbro"            ); \
    instance_new                = FENIX_import ( "instance_new"                    ); \
    instance_duplicate          = FENIX_import ( "instance_duplicate"              ); \
    instance_destroy            = FENIX_import ( "instance_destroy"                ); \
    instance_dump               = FENIX_import ( "instance_dump"                   ); \
    instance_dump_all           = FENIX_import ( "instance_dump_all"               ); \
    instance_go                 = FENIX_import ( "instance_go"                     ); \
    instance_go_all             = FENIX_import ( "instance_go_all"                 ); \
    sound_init                  = FENIX_import ( "sound_init"                      ); \
    sound_close                 = FENIX_import ( "sound_close"                     ); \
    load_mod                    = FENIX_import ( "load_mod"                        ); \
    play_mod                    = FENIX_import ( "play_mod"                        ); \
    unload_mod                  = FENIX_import ( "unload_mod"                      ); \
    stop_mod                    = FENIX_import ( "stop_mod"                        ); \
    pause_mod                   = FENIX_import ( "pause_mod"                       ); \
    resume_mod                  = FENIX_import ( "resume_mod"                      ); \
    is_playing_mod              = FENIX_import ( "is_playing_mod"                  ); \
    set_mod_volume              = FENIX_import ( "set_mod_volume"                  ); \
    load_wav                    = FENIX_import ( "load_wav"                        ); \
    play_wav                    = FENIX_import ( "play_wav"                        ); \
    unload_wav                  = FENIX_import ( "unload_wav"                      ); \
    stop_wav                    = FENIX_import ( "stop_wav"                        ); \
    pause_wav                   = FENIX_import ( "pause_wav"                       ); \
    resume_wav                  = FENIX_import ( "resume_wav"                      ); \
    is_playing_wav              = FENIX_import ( "is_playing_wav"                  ); \
    set_wav_volume              = FENIX_import ( "set_wav_volume"                  ); \
    ini_musiccd                 = FENIX_import ( "ini_musiccd"                     ); \
    cd_play                     = FENIX_import ( "cd_play"                         ); \
    cd_stop                     = FENIX_import ( "cd_stop"                         ); \
    cd_playing                  = FENIX_import ( "cd_playing"                      );
                                                                                      