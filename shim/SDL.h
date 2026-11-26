/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * A very small SDL 1.2 look-alike for the Fenix 0.80 interpreter (fxi) on the SF3500.
 *
 * fxi was written against SDL 1.2. The SF3500 has no SDL video or input device: frames go to the
 * device's display driver and the pad is read from shared memory (see sf3500/). So instead of
 * porting an SDL 1.2 build, this header declares just the subset of the SDL 1.2 API that fxi uses,
 * and shim/fxsdl.c implements it directly on those two things. Only the audio path uses real SDL
 * (SDL2 + SDL2_mixer, in shim/fxsdl_audio.c), because SDL_mixer does the sample mixing and the
 * tracker music.
 *
 * Every function is renamed with a macro (SDL_Init -> fxsdl_Init), so nothing here can collide
 * with the real SDL2 symbols that the audio side links against.
 */
#ifndef FXSDL_H
#define FXSDL_H

#include "SDL_types.h"
#include "SDL_keysym.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDL_MAJOR_VERSION 1
#define SDL_MINOR_VERSION 2
#define SDL_PATCHLEVEL    15

typedef struct { Uint8 major, minor, patch; } SDL_version;

/* ---- video ---------------------------------------------------------------------------------- */

typedef struct { Uint8 r, g, b, unused; } SDL_Color;
typedef struct { int ncolors; SDL_Color *colors; } SDL_Palette;

typedef struct {
	SDL_Palette *palette;
	Uint8  BitsPerPixel, BytesPerPixel;
	Uint8  Rloss, Gloss, Bloss, Aloss;
	Uint8  Rshift, Gshift, Bshift, Ashift;
	Uint32 Rmask, Gmask, Bmask, Amask;
	Uint32 colorkey;
	Uint8  alpha;
} SDL_PixelFormat;

typedef struct SDL_Surface {
	Uint32 flags;
	SDL_PixelFormat *format;
	int w, h;
	Uint16 pitch;
	void *pixels;
	int offset;
} SDL_Surface;

#define SDL_SWSURFACE   0x00000000
#define SDL_HWSURFACE   0x00000001
#define SDL_SRCCOLORKEY 0x00001000
#define SDL_HWPALETTE   0x20000000
#define SDL_DOUBLEBUF   0x40000000
#define SDL_FULLSCREEN  0x80000000

#define SDL_LOGPAL 0x01
#define SDL_PHYSPAL 0x02

/* ---- init ----------------------------------------------------------------------------------- */

#define SDL_INIT_AUDIO    0x00000010
#define SDL_INIT_VIDEO    0x00000020
#define SDL_INIT_CDROM    0x00000100
#define SDL_INIT_JOYSTICK 0x00000200

#define SDL_ENABLE  1
#define SDL_DISABLE 0

/* ---- events --------------------------------------------------------------------------------- */

typedef enum {
	SDL_NOEVENT = 0,
	SDL_KEYDOWN = 2,
	SDL_KEYUP = 3,
	SDL_MOUSEMOTION = 4,
	SDL_MOUSEBUTTONDOWN = 5,
	SDL_MOUSEBUTTONUP = 6,
	SDL_JOYAXISMOTION = 7,
	SDL_JOYBUTTONDOWN = 10,
	SDL_JOYBUTTONUP = 11,
	SDL_QUIT = 12
} SDL_EventType;

#define SDL_PRESSED  1
#define SDL_RELEASED 0

typedef struct { SDLKey sym; SDLMod mod; Uint16 unicode; Uint8 scancode; } SDL_keysym;

typedef struct { Uint8 type, which, state; SDL_keysym keysym; } SDL_KeyboardEvent;
typedef struct { Uint8 type, state; Uint16 x, y; Sint16 xrel, yrel; } SDL_MouseMotionEvent;
typedef struct { Uint8 type, button, state; Uint16 x, y; } SDL_MouseButtonEvent;
typedef struct { Uint8 type, which, axis; Sint16 value; } SDL_JoyAxisEvent;
typedef struct { Uint8 type, which, button, state; } SDL_JoyButtonEvent;

typedef union {
	Uint8 type;
	SDL_KeyboardEvent key;
	SDL_MouseMotionEvent motion;
	SDL_MouseButtonEvent button;
	SDL_JoyAxisEvent jaxis;
	SDL_JoyButtonEvent jbutton;
} SDL_Event;

/* ---- CD audio: there is no CD drive on the device; every call reports "no disc" -------------- */

typedef struct SDL_CD SDL_CD;
typedef enum { CD_TRAYEMPTY, CD_STOPPED, CD_PLAYING, CD_PAUSED, CD_ERROR = -1 } CDstatus;
#define CD_INDRIVE(status) ((int)(status) > 0)

/* ---- functions ------------------------------------------------------------------------------ */

#define SDL_Init                   fxsdl_Init
#define SDL_Quit                   fxsdl_Quit
#define SDL_GetError               fxsdl_GetError
#define SDL_Linked_Version         fxsdl_Linked_Version
#define SDL_GetTicks               fxsdl_GetTicks
#define SDL_Delay                  fxsdl_Delay
#define SDL_PollEvent              fxsdl_PollEvent
#define SDL_GetKeyState            fxsdl_GetKeyState
#define SDL_GetModState            fxsdl_GetModState
#define SDL_EnableUNICODE          fxsdl_EnableUNICODE
#define SDL_WarpMouse              fxsdl_WarpMouse
#define SDL_ShowCursor             fxsdl_ShowCursor
#define SDL_NumJoysticks           fxsdl_NumJoysticks
#define SDL_JoystickEventState     fxsdl_JoystickEventState
#define SDL_SetVideoMode           fxsdl_SetVideoMode
#define SDL_Flip                   fxsdl_Flip
#define SDL_UpdateRect             fxsdl_UpdateRect
#define SDL_LockSurface            fxsdl_LockSurface
#define SDL_UnlockSurface          fxsdl_UnlockSurface
#define SDL_FreeSurface            fxsdl_FreeSurface
#define SDL_CreateRGBSurfaceFrom   fxsdl_CreateRGBSurfaceFrom
#define SDL_SetPalette             fxsdl_SetPalette
#define SDL_SetColors              fxsdl_SetColors
#define SDL_SetColorKey            fxsdl_SetColorKey
#define SDL_MapRGB                 fxsdl_MapRGB
#define SDL_WM_SetCaption          fxsdl_WM_SetCaption
#define SDL_WM_SetIcon             fxsdl_WM_SetIcon
#define SDL_CDOpen                 fxsdl_CDOpen
#define SDL_CDClose                fxsdl_CDClose
#define SDL_CDStatus               fxsdl_CDStatus
#define SDL_CDPlayTracks           fxsdl_CDPlayTracks
#define SDL_CDStop                 fxsdl_CDStop

int          SDL_Init(Uint32 flags);
void         SDL_Quit(void);
const char * SDL_GetError(void);
const SDL_version * SDL_Linked_Version(void);
Uint32       SDL_GetTicks(void);
void         SDL_Delay(Uint32 ms);
int          SDL_PollEvent(SDL_Event *event);
Uint8 *      SDL_GetKeyState(int *numkeys);
SDLMod       SDL_GetModState(void);
int          SDL_EnableUNICODE(int enable);
void         SDL_WarpMouse(Uint16 x, Uint16 y);
int          SDL_ShowCursor(int toggle);
int          SDL_NumJoysticks(void);
int          SDL_JoystickEventState(int state);
SDL_Surface *SDL_SetVideoMode(int width, int height, int bpp, Uint32 flags);
void         SDL_Flip(SDL_Surface *screen);
void         SDL_UpdateRect(SDL_Surface *screen, Sint32 x, Sint32 y, Uint32 w, Uint32 h);
int          SDL_LockSurface(SDL_Surface *surface);
void         SDL_UnlockSurface(SDL_Surface *surface);
void         SDL_FreeSurface(SDL_Surface *surface);
SDL_Surface *SDL_CreateRGBSurfaceFrom(void *pixels, int width, int height, int depth, int pitch,
                                      Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
int          SDL_SetPalette(SDL_Surface *surface, int flags, SDL_Color *colors, int firstcolor, int ncolors);
int          SDL_SetColors(SDL_Surface *surface, SDL_Color *colors, int firstcolor, int ncolors);
int          SDL_SetColorKey(SDL_Surface *surface, Uint32 flag, Uint32 key);
Uint32       SDL_MapRGB(const SDL_PixelFormat *fmt, Uint8 r, Uint8 g, Uint8 b);
void         SDL_WM_SetCaption(const char *title, const char *icon);
void         SDL_WM_SetIcon(SDL_Surface *icon, Uint8 *mask);
SDL_CD *     SDL_CDOpen(int drive);
void         SDL_CDClose(SDL_CD *cdrom);
CDstatus     SDL_CDStatus(SDL_CD *cdrom);
int          SDL_CDPlayTracks(SDL_CD *cdrom, int start_track, int start_frame, int ntracks, int nframes);
int          SDL_CDStop(SDL_CD *cdrom);

/* Internal: used by fxsdl_audio.c, which is compiled against the real SDL2 headers instead of this one. */
void fxsdl_set_error(const char *msg);

#ifdef __cplusplus
}
#endif

#endif /* FXSDL_H */
