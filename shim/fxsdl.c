/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Video, input and timing for fxi on the SF3500, behind the SDL 1.2 look-alike in SDL.h.
 *
 *   video   fxi draws into an 8 or 16 bit surface that lives here; "flipping" converts it to
 *           RGB565 (8-bit frames through the palette) and hands it to the display driver.
 *   input   the gamepad is read from the front-end's shared memory and turned into SDL 1.2 key
 *           state and key events. Select + Start quits.
 *   timing  SDL_GetTicks / SDL_Delay on the monotonic clock.
 *
 * Added by the port (the game has neither):
 *   pause       Start, while a level is being played. The game stands still inside its own input
 *               poll, its clock does not advance, sound is held, and the last frame is shown dimmed.
 *   intro skip  the logo and title sequence must be watched through once (this writes the file
 *               "intro_seen" next to the game). On later runs a prompt is drawn and Start skips.
 *
 * Test hooks (cost nothing unless the variable is set):
 *   SF3500_KEYSCRIPT="60:E,120:S:30"   press Enter at frame 60, Space at frame 120 for 30 frames
 *                                      (keys: E enter, S space, L/R/U/D arrows, C escape, F fire (Alt),
 *                                      K Ctrl, W the game's next-level cheat; pad buttons: A, T Start,
 *                                      N R1, P L1, J the B button, H D-pad up;
 *                                      3 frames by default)
 *   SF3500_SHOT_PREFIX=p               with SF3500_SHOT_FRAMES="100,200" writes p_<frame>.ppm
 *   SF3500_QUIT_FRAME=n                exits cleanly after n presented frames
 *   SF3500_PAUSE_TEST_POLLS=n          a pause ends by itself after n polls (50 ms each)
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "SDL.h"
#include "mobsters.h"
#include "../sf3500/joykey.h"
#include "../sf3500/hwdisp.h"
#include "../sf3500/hwaudio.h"

extern int  fxsdl_audio_init(void);
extern void fxsdl_audio_quit(void);
extern void fxsdl_audio_pause(int on);

/* ------------------------------------------------------------------------------------------- */
/* timing                                                                                      */
/* ------------------------------------------------------------------------------------------- */

static struct timespec t0;
static int t0_set = 0;
static long long paused_us = 0;      /* time spent paused, hidden from the game */

static long long now_us(void)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	if (!t0_set) { t0 = t; t0_set = 1; }
	return (long long)(t.tv_sec - t0.tv_sec) * 1000000LL + (t.tv_nsec - t0.tv_nsec) / 1000 - paused_us;
}

Uint32 SDL_GetTicks(void) { return (Uint32)(now_us() / 1000); }

void SDL_Delay(Uint32 ms)
{
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (long)(ms % 1000) * 1000000L;
	nanosleep(&ts, NULL);
}

/* ------------------------------------------------------------------------------------------- */
/* init / quit                                                                                 */
/* ------------------------------------------------------------------------------------------- */

static int video_up = 0;
static int quitting = 0;

static const SDL_version linked = { SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL };
const SDL_version *SDL_Linked_Version(void) { return &linked; }

int SDL_Init(Uint32 flags)
{
	now_us();
	if ((flags & SDL_INIT_VIDEO) && !video_up) {
		joykey_init();
		if (hwdisp_init() != 0)
			fprintf(stderr, "fxsdl: no display driver, frames are not shown\n");
		video_up = 1;
	}
	if (flags & SDL_INIT_AUDIO) {
		if (fxsdl_audio_init() < 0) return -1;
	}
	return 0;
}

void SDL_Quit(void)
{
	if (quitting) return;
	quitting = 1;
	fxsdl_audio_quit();
	hwaudio_deinit();
	hwdisp_deinit();
}

/* ------------------------------------------------------------------------------------------- */
/* video                                                                                       */
/* ------------------------------------------------------------------------------------------- */

static SDL_Surface screen_surf;
static SDL_PixelFormat screen_fmt;
static SDL_Palette screen_pal;
static SDL_Color pal[256];
static unsigned short pal565[256];
static unsigned short *out565 = NULL;
static unsigned frames = 0;

static unsigned short pack565(int r, int g, int b)
{
	return (unsigned short)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

static void palette_changed(int first, int n)
{
	int i;
	for (i = first; i < first + n && i < 256; i++)
		pal565[i] = pack565(pal[i].r, pal[i].g, pal[i].b);
}

SDL_Surface *SDL_SetVideoMode(int width, int height, int bpp, Uint32 flags)
{
	int bytes;

	if (width <= 0 || height <= 0 || (bpp != 8 && bpp != 16)) {
		fxsdl_set_error("video mode not supported");
		return NULL;
	}
	bytes = bpp / 8;

	free(screen_surf.pixels);
	free(out565);
	out565 = NULL;
	memset(&screen_surf, 0, sizeof(screen_surf));
	memset(&screen_fmt, 0, sizeof(screen_fmt));

	screen_surf.pitch = (Uint16)((width * bytes + 3) & ~3);
	screen_surf.pixels = calloc(1, (size_t)screen_surf.pitch * height);
	if (!screen_surf.pixels) { fxsdl_set_error("out of memory"); return NULL; }
	screen_surf.w = width;
	screen_surf.h = height;
	screen_surf.flags = (flags & (SDL_DOUBLEBUF | SDL_FULLSCREEN | SDL_HWPALETTE)) | SDL_HWSURFACE;
	screen_surf.format = &screen_fmt;

	screen_fmt.BitsPerPixel = (Uint8)bpp;
	screen_fmt.BytesPerPixel = (Uint8)bytes;
	if (bpp == 16) {
		screen_fmt.Rmask = 0xF800; screen_fmt.Gmask = 0x07E0; screen_fmt.Bmask = 0x001F;
		screen_fmt.Rshift = 11; screen_fmt.Gshift = 5; screen_fmt.Bshift = 0;
		screen_fmt.Rloss = 3; screen_fmt.Gloss = 2; screen_fmt.Bloss = 3;
	} else {
		screen_pal.ncolors = 256;
		screen_pal.colors = pal;
		screen_fmt.palette = &screen_pal;
		out565 = (unsigned short *)malloc((size_t)width * height * 2);
		if (!out565) { fxsdl_set_error("out of memory"); return NULL; }
	}
	fprintf(stderr, "fxsdl: video mode %dx%d, %d bit%s\n", width, height, bpp,
	        (screen_surf.flags & SDL_DOUBLEBUF) ? ", double buffer" : "");
	return &screen_surf;
}

int SDL_LockSurface(SDL_Surface *s) { (void)s; return 0; }
void SDL_UnlockSurface(SDL_Surface *s) { (void)s; }

int SDL_SetColors(SDL_Surface *s, SDL_Color *colors, int first, int n)
{
	int i;
	if (!s || !colors || s != &screen_surf || screen_fmt.BitsPerPixel != 8) return 0;
	if (first < 0 || first + n > 256) return 0;
	for (i = 0; i < n; i++) pal[first + i] = colors[i];
	palette_changed(first, n);
	return 1;
}

int SDL_SetPalette(SDL_Surface *s, int flags, SDL_Color *colors, int first, int n)
{
	(void)flags;
	return SDL_SetColors(s, colors, first, n);
}

Uint32 SDL_MapRGB(const SDL_PixelFormat *fmt, Uint8 r, Uint8 g, Uint8 b)
{
	if (fmt && fmt->palette) {
		/* nearest entry of the palette */
		int i, best = 0, bestd = 1 << 30;
		for (i = 0; i < fmt->palette->ncolors; i++) {
			int dr = fmt->palette->colors[i].r - r, dg = fmt->palette->colors[i].g - g, db = fmt->palette->colors[i].b - b;
			int d = dr * dr + dg * dg + db * db;
			if (d < bestd) { bestd = d; best = i; if (!d) break; }
		}
		return (Uint32)best;
	}
	return pack565(r, g, b);
}

SDL_Surface *SDL_CreateRGBSurfaceFrom(void *pixels, int w, int h, int depth, int pitch,
                                      Uint32 rm, Uint32 gm, Uint32 bm, Uint32 am)
{
	/* Only fxi's window icon uses this; the device has no window, so a header is enough. */
	SDL_Surface *s = (SDL_Surface *)calloc(1, sizeof(*s));
	SDL_PixelFormat *f = (SDL_PixelFormat *)calloc(1, sizeof(*f));
	if (!s || !f) { free(s); free(f); return NULL; }
	f->BitsPerPixel = (Uint8)depth; f->BytesPerPixel = (Uint8)(depth / 8);
	f->Rmask = rm; f->Gmask = gm; f->Bmask = bm; f->Amask = am;
	s->format = f; s->w = w; s->h = h; s->pitch = (Uint16)pitch; s->pixels = pixels;
	return s;
}

void SDL_FreeSurface(SDL_Surface *s)
{
	if (!s || s == &screen_surf) return;
	free(s->format);
	free(s);
}

int SDL_SetColorKey(SDL_Surface *s, Uint32 flag, Uint32 key)
{
	if (s && s->format) s->format->colorkey = key;
	(void)flag;
	return 0;
}

void SDL_WM_SetCaption(const char *title, const char *icon) { (void)title; (void)icon; }
void SDL_WM_SetIcon(SDL_Surface *icon, Uint8 *mask) { (void)icon; (void)mask; }
int  SDL_ShowCursor(int toggle) { (void)toggle; return 0; }
void SDL_WarpMouse(Uint16 x, Uint16 y) { (void)x; (void)y; }

/* ---- frame dumps and the frame counter for tests ------------------------------------------- */

static void write_ppm(const char *path, const unsigned short *rgb565, int w, int h, int pitch_px)
{
	FILE *f = fopen(path, "wb");
	int x, y;
	if (!f) return;
	fprintf(f, "P6\n%d %d\n255\n", w, h);
	for (y = 0; y < h; y++)
		for (x = 0; x < w; x++) {
			unsigned short p = rgb565[y * pitch_px + x];
			unsigned char c[3];
			c[0] = (unsigned char)(((p >> 11) & 31) * 255 / 31);
			c[1] = (unsigned char)(((p >> 5) & 63) * 255 / 63);
			c[2] = (unsigned char)((p & 31) * 255 / 31);
			fwrite(c, 1, 3, f);
		}
	fclose(f);
}

/* ---- text drawn by the port over finished frames (never into the game's own picture) -------- */

/* 5x7 letters for the pause screen and the skip prompt (rows top to bottom, bit 4 is the left column) */
static const struct { char c; unsigned char r[7]; } font5x7[] = {
	{ 'A', { 14, 17, 17, 31, 17, 17, 17 } }, { 'C', { 14, 17, 16, 16, 16, 17, 14 } },
	{ 'D', { 30, 17, 17, 17, 17, 17, 30 } }, { 'E', { 31, 16, 16, 30, 16, 16, 31 } },
	{ 'I', { 14,  4,  4,  4,  4,  4, 14 } }, { 'K', { 17, 18, 20, 24, 20, 18, 17 } },
	{ 'L', { 16, 16, 16, 16, 16, 16, 31 } }, { 'M', { 17, 27, 21, 21, 17, 17, 17 } },
	{ 'O', { 14, 17, 17, 17, 17, 17, 14 } }, { 'P', { 30, 17, 17, 30, 16, 16, 16 } },
	{ 'Q', { 14, 17, 17, 17, 21, 18, 13 } }, { 'R', { 30, 17, 17, 30, 20, 18, 17 } },
	{ 'S', { 15, 16, 16, 14,  1,  1, 30 } }, { 'T', { 31,  4,  4,  4,  4,  4,  4 } },
	{ 'U', { 17, 17, 17, 17, 17, 17, 14 } }, { '+', {  0,  4,  4, 31,  4,  4,  0 } },
	{ ':', {  0,  4,  4,  0,  4,  4,  0 } }, { 0, { 0 } }
};

static int text_width(const char *text, int scale) { return (int)strlen(text) * 6 * scale - scale; }

static void draw_text(unsigned short *buf, int w, int h, int x0, int y0, int scale,
                      const char *text, unsigned short colour)
{
	int i, k, r, c, dx, dy;
	for (i = 0; text[i]; i++)
		for (k = 0; font5x7[k].c; k++) {
			if (font5x7[k].c != text[i]) continue;
			for (r = 0; r < 7; r++)
				for (c = 0; c < 5; c++) {
					if (!(font5x7[k].r[r] & (16 >> c))) continue;
					for (dy = 0; dy < scale; dy++)
						for (dx = 0; dx < scale; dx++) {
							int x = x0 + (i * 6 + c) * scale + dx, y = y0 + r * scale + dy;
							if (x >= 0 && x < w && y >= 0 && y < h) buf[y * w + x] = colour;
						}
				}
		}
}

/* Text with a one-pixel dark shadow, centred on cx. */
static void draw_label(unsigned short *buf, int w, int h, int cx, int y, int scale,
                       const char *text, unsigned short colour)
{
	int x = cx - text_width(text, scale) / 2;
	draw_text(buf, w, h, x + 1, y + 1, scale, text, 0x0000);
	draw_text(buf, w, h, x, y, scale, text, colour);
}

#define SKIP_TEXT "PRESS START TO SKIP"

static void draw_skip_prompt(unsigned short *buf, int w, int h)
{
	int tw = text_width(SKIP_TEXT, 1), x = (w - tw) / 2, y = h - 15, i, j;
	for (j = y - 4; j < y + 11; j++)                 /* a dark box: the logo screen is light grey */
		for (i = x - 6; i < x + tw + 6; i++)
			if (i >= 0 && i < w && j >= 0 && j < h) buf[j * w + i] = 0x0000;
	draw_text(buf, w, h, x, y, 1, SKIP_TEXT, 0xFFFF);
}

/* The last presented frame, dimmed, with the pause text. Only the 8-bit mode the game uses. */
static void present_paused(void)
{
	static unsigned short *buf = NULL;
	static int cap = 0;
	int w = screen_surf.w, h = screen_surf.h, n = w * h, i;

	if (!out565 || n <= 0) return;
	if (n > cap) {
		free(buf);
		buf = (unsigned short *)malloc((size_t)n * 2);
		cap = buf ? n : 0;
	}
	if (!buf) return;
	for (i = 0; i < n; i++) buf[i] = (unsigned short)((out565[i] >> 2) & 0x39E7);   /* a quarter as bright */
	draw_label(buf, w, h, w / 2, h / 2 - 34, 4, "PAUSED", 0xFFFF);
	draw_label(buf, w, h, w / 2, h / 2 + 14, 2, "START: RESUME", 0xFFE0);
	draw_label(buf, w, h, w / 2, h / 2 + 36, 1, "SELECT + START: QUIT", 0xC618);
	hwdisp_present(buf, w, h, w * 2);
	if (getenv("SF3500_SHOT_PREFIX")) {
		char path[512];
		snprintf(path, sizeof(path), "%s_paused.ppm", getenv("SF3500_SHOT_PREFIX"));
		write_ppm(path, buf, w, h, w);
	}
}

static int frame_listed(const char *list, unsigned frame)
{
	const char *p = list;
	while (p && *p) {
		char *end;
		unsigned long v = strtoul(p, &end, 10);
		if (end == p) break;
		if (v == frame) return 1;
		p = (*end == ',') ? end + 1 : end;
	}
	return 0;
}

static int intro_seen = -1;           /* -1: not looked yet */
static int skipping = 0;

static void present(void)
{
	static long long last_us = 0, sum_us = 0, present_us = 0;
	static unsigned count = 0;
	long long a, b;
	const void *src;
	int pitch;
	static const char *shot_prefix = (const char *)-1, *shot_frames = NULL, *quit_frame = NULL;

	if (shot_prefix == (const char *)-1) {
		shot_prefix = getenv("SF3500_SHOT_PREFIX");
		shot_frames = getenv("SF3500_SHOT_FRAMES");
		quit_frame = getenv("SF3500_QUIT_FRAME");
	}

	a = now_us();
	if (screen_fmt.BitsPerPixel == 8) {
		const unsigned char *s = (const unsigned char *)screen_surf.pixels;
		unsigned short *d = out565;
		int x, y;
		for (y = 0; y < screen_surf.h; y++) {
			const unsigned char *row = s + (size_t)y * screen_surf.pitch;
			for (x = 0; x < screen_surf.w; x++) *d++ = pal565[row[x]];
		}
		if (intro_seen == 1 && !skipping && mob_where() == MOB_INTRO)
			draw_skip_prompt(out565, screen_surf.w, screen_surf.h);
		src = out565; pitch = screen_surf.w * 2;
	} else {
		src = screen_surf.pixels; pitch = screen_surf.pitch;
	}
	hwdisp_present(src, screen_surf.w, screen_surf.h, pitch);
	b = now_us();

	frames++;
	if (shot_prefix && shot_frames && frame_listed(shot_frames, frames)) {
		char path[512];
		snprintf(path, sizeof(path), "%s_%u.ppm", shot_prefix, frames);
		write_ppm(path, (const unsigned short *)src, screen_surf.w, screen_surf.h, pitch / 2);
	}

	/* One log line about every 250 frames: achieved frame rate and the time spent in the driver. */
	if (last_us) { sum_us += a - last_us; present_us += b - a; count++; }
	last_us = a;
	if (count >= 250) {
		fprintf(stderr, "fxsdl: perf frames=%u avg=%lld us (%.1f fps) present=%lld us\n",
		        frames, sum_us / count, 1e6 * count / (double)sum_us, present_us / count);
		sum_us = present_us = 0; count = 0;
	}
	if (quit_frame && frames >= (unsigned)atoi(quit_frame)) {
		fprintf(stderr, "fxsdl: SF3500_QUIT_FRAME reached\n");
		SDL_Quit();
		exit(0);
	}
}

void SDL_Flip(SDL_Surface *s) { if (s == &screen_surf) present(); }

void SDL_UpdateRect(SDL_Surface *s, Sint32 x, Sint32 y, Uint32 w, Uint32 h)
{
	(void)x; (void)y; (void)w; (void)h;
	if (s == &screen_surf) present();
}

/* ------------------------------------------------------------------------------------------- */
/* input                                                                                       */
/* ------------------------------------------------------------------------------------------- */

static Uint8 keystate[SDLK_LAST];
static Uint8 prevstate[SDLK_LAST];
static int unicode_on = 0;

/* Up to two keys per button. Dead Mobsters reads: arrows, Space or Ctrl (jump), Alt (fire; the
 * original readme says Ctrl, but the game code tests Alt), 1-4 (weapon), Enter and Escape. */
static struct { SDLKey sym; Uint16 uni; } const jk_key[JK_COUNT][2] = {
	/* JK_LEFT   */ { { SDLK_LEFT,   0 } },
	/* JK_RIGHT  */ { { SDLK_RIGHT,  0 } },
	/* JK_UP     */ { { SDLK_UP,     0 },   { SDLK_SPACE,  ' ' } },  /* menu; also jumps */
	/* JK_DOWN   */ { { SDLK_DOWN,   0 } },                          /* menu */
	/* JK_A      */ { { SDLK_LALT,   0 },   { SDLK_RETURN, 13 } },   /* fire; confirm in menus */
	/* JK_B      */ { { SDLK_SPACE,  ' ' } },                        /* jump */
	/* JK_L1     */ { { SDLK_UNKNOWN, 0 } },                         /* previous weapon, below */
	/* JK_R1     */ { { SDLK_UNKNOWN, 0 } },                         /* next weapon, below */
	/* JK_X      */ { { SDLK_UNKNOWN, 0 } },                         /* next weapon, below */
	/* JK_Y      */ { { SDLK_LALT,   0 } },                          /* fire */
	/* JK_START  */ { { SDLK_RETURN, 13 } },                         /* start / continue */
	/* JK_SELECT */ { { SDLK_ESCAPE, 0 } },                          /* back to the menu; there, leaves */
};

/* ---- weapon cycling --------------------------------------------------------------------- */

/* The game only has direct keys (1-4). To step through the weapons, ask which one is current and
 * press the number key of its neighbour. */
#define WEAPONS 4
#define WEAPON_KEY_FRAMES 2

static SDLKey weapon_key = SDLK_UNKNOWN;
static unsigned weapon_key_until = 0;

static void cycle_weapon(int step)
{
	int cur = mob_weapon();
	if (cur < 1 || cur > WEAPONS) return;
	cur = (cur - 1 + step + WEAPONS) % WEAPONS + 1;
	weapon_key = (SDLKey)(SDLK_1 + cur - 1);
	weapon_key_until = frames + WEAPON_KEY_FRAMES;
}

/* ---- scripted keys for headless tests ------------------------------------------------------- */

#define SCRIPT_MAX 256
static struct { unsigned frame, hold, pad; SDLKey sym; } script[SCRIPT_MAX];
static int script_n = 0, script_loaded = 0;

static void load_script(void)
{
	const char *e = getenv("SF3500_KEYSCRIPT");
	script_loaded = 1;
	while (e && *e && script_n < SCRIPT_MAX) {
		char *end;
		unsigned long frame = strtoul(e, &end, 10);
		unsigned long hold = 3;
		SDLKey sym = SDLK_UNKNOWN;
		unsigned pad = 0;
		if (end == e || *end != ':') break;
		switch (end[1]) {
		case 'E': sym = SDLK_RETURN; break;
		case 'S': sym = SDLK_SPACE; break;
		case 'L': sym = SDLK_LEFT; break;
		case 'R': sym = SDLK_RIGHT; break;
		case 'U': sym = SDLK_UP; break;
		case 'D': sym = SDLK_DOWN; break;
		case 'C': sym = SDLK_ESCAPE; break;
		case 'F': sym = SDLK_LALT; break;
		case 'K': sym = SDLK_LCTRL; break;
		case 'W': sym = SDLK_l; break;          /* the game's own "next level" cheat, for tests */
		case 'N': pad = 1u << JK_R1; break;     /* these press pad buttons, not keys */
		case 'P': pad = 1u << JK_L1; break;
		case 'T': pad = 1u << JK_START; break;
		case 'A': pad = 1u << JK_A; break;
		case 'J': pad = 1u << JK_B; break;     /* the jump button */
		case 'H': pad = 1u << JK_UP; break;
		}
		e = end + 2;
		if (*e == ':') hold = strtoul(e + 1, (char **)&e, 10);
		script[script_n].frame = (unsigned)frame;
		script[script_n].hold = (unsigned)hold;
		script[script_n].sym = sym;
		script[script_n].pad = pad;
		script_n++;
		if (*e == ',') e++; else break;
	}
}

#define EVQ 64
static SDL_Event evq[EVQ];
static int ev_head = 0, ev_tail = 0;

static void push_key(int down, SDLKey sym, Uint16 uni)
{
	SDL_Event *e = &evq[ev_tail];
	int next = (ev_tail + 1) % EVQ;
	if (next == ev_head) return; /* full: drop */
	memset(e, 0, sizeof(*e));
	e->key.type = down ? SDL_KEYDOWN : SDL_KEYUP;
	e->key.state = down ? SDL_PRESSED : SDL_RELEASED;
	e->key.keysym.sym = sym;
	e->key.keysym.unicode = unicode_on ? uni : 0;
	e->key.keysym.mod = KMOD_NONE;
	ev_tail = next;
}

/* Pad buttons: the device's, plus any a test script is holding. */
static unsigned int pad_bits(void)
{
	unsigned int bits = joykey_read();
	int i;
	for (i = 0; i < script_n; i++)
		if (frames >= script[i].frame && frames < script[i].frame + script[i].hold)
			bits |= script[i].pad;
	return bits;
}

/* Standing SF3500 exit combo: Select + Start. */
static void quit_if_combo(unsigned int bits)
{
	if ((bits & (1u << JK_SELECT)) && (bits & (1u << JK_START))) {
		fprintf(stderr, "fxsdl: Select+Start, leaving\n");
		SDL_Quit();
		exit(0);
	}
}

/* ---- intro skip ------------------------------------------------------------------------- */

#define INTRO_FLAG "intro_seen"       /* in the game's folder; delete it to force the intro again */

static void intro_update(enum mob_place where)
{
	if (intro_seen < 0) intro_seen = (access(INTRO_FLAG, F_OK) == 0);
	if (skipping) {
		if (where == MOB_INTRO) mob_end_intro_wait();
		else if (where == MOB_MENU) skipping = 0;
	}
	/* Reaching the title menu is what counts as having watched the intro. */
	if (where == MOB_MENU && !intro_seen) {
		FILE *f = fopen(INTRO_FLAG, "w");
		intro_seen = 1;
		if (!f) { fprintf(stderr, "fxsdl: cannot write %s\n", INTRO_FLAG); return; }
		fputs("The intro has been watched once. Delete this file to make it unskippable again.\n", f);
		fflush(f);
		fsync(fileno(f));          /* the handheld is often just switched off */
		fclose(f);
		fprintf(stderr, "fxsdl: intro watched, skipping is available from now on\n");
	}
}

/* ---- pause -------------------------------------------------------------------------------- */

/* Does not return until the player resumes: fxi polls input once a frame, so the game waits here. */
static void pause_game(void)
{
	static int test_polls = -2;
	long long began = now_us();
	int start_was_down = 1, polls = 0;

	if (test_polls == -2) {
		const char *e = getenv("SF3500_PAUSE_TEST_POLLS");
		test_polls = e ? atoi(e) : -1;
	}
	fprintf(stderr, "fxsdl: pause on (frame %u)\n", frames);
	fxsdl_audio_pause(1);
	for (;;) {
		unsigned int bits;
		int down;
		if (polls % 10 == 0) present_paused();          /* now, and twice a second after */
		SDL_Delay(50);
		polls++;
		bits = pad_bits();
		quit_if_combo(bits);
		down = (bits & (1u << JK_START)) != 0;
		if (down && !start_was_down) break;
		start_was_down = down;
		if (test_polls >= 0 && polls >= test_polls) break;
	}
	fxsdl_audio_pause(0);
	paused_us += now_us() - began;                      /* the game's clock did not move */
	fprintf(stderr, "fxsdl: pause off\n");
}

static void refresh_input(void)
{
	static unsigned int prevbits = 0;
	static unsigned int start_before = 0;
	static int swallow_start = 0;
	unsigned int bits, start_now;
	enum mob_place where;
	Uint8 now[SDLK_LAST];
	Uint16 uni[SDLK_LAST];
	int i, new_press;

	if (!script_loaded) load_script();

	bits = pad_bits();
	quit_if_combo(bits);

	start_now = bits & (1u << JK_START);
	new_press = start_now && !start_before && !(bits & (1u << JK_SELECT));
	start_before = start_now;

	where = mob_where();
	if (new_press && where == MOB_IN_LEVEL) {
		pause_game();
		bits = pad_bits();                          /* the Start press that resumed */
		start_before = bits & (1u << JK_START);
		swallow_start = 1;
	} else if (new_press && where == MOB_INTRO && intro_seen == 1) {
		fprintf(stderr, "fxsdl: intro skipped\n");
		skipping = 1;
		swallow_start = 1;
	}
	intro_update(where);

	/* A Start press used for pause or skip must not also reach the game as Enter (it would
	 * pick an entry in the title menu), so it is hidden until the button is let go. */
	if (swallow_start) {
		if (bits & (1u << JK_START)) bits &= ~(1u << JK_START);
		else swallow_start = 0;
	}

	memset(now, 0, sizeof(now));
	memset(uni, 0, sizeof(uni));
	for (i = 0; i < JK_COUNT; i++) {
		int k;
		if (!(bits & (1u << i))) continue;
		for (k = 0; k < 2; k++) {
			if (jk_key[i][k].sym == SDLK_UNKNOWN) continue;
			now[jk_key[i][k].sym] = 1;
			uni[jk_key[i][k].sym] = jk_key[i][k].uni;
		}
	}

	{
		unsigned int pressed = bits & ~prevbits;
		prevbits = bits;
		if (pressed & ((1u << JK_R1) | (1u << JK_X))) cycle_weapon(+1);
		if (pressed & (1u << JK_L1)) cycle_weapon(-1);
	}
	for (i = 0; i < script_n; i++) {
		if (frames >= script[i].frame && frames < script[i].frame + script[i].hold && script[i].sym != SDLK_UNKNOWN) {
			now[script[i].sym] = 1;
			uni[script[i].sym] = (script[i].sym == SDLK_SPACE) ? ' ' : (script[i].sym == SDLK_RETURN) ? 13 : 0;
		}
	}

	if (weapon_key != SDLK_UNKNOWN) {
		if (frames < weapon_key_until) now[weapon_key] = 1;
		else weapon_key = SDLK_UNKNOWN;
	}

	for (i = 0; i < SDLK_LAST; i++) {
		if (now[i] != prevstate[i]) push_key(now[i], (SDLKey)i, uni[i]);
		prevstate[i] = now[i];
		keystate[i] = now[i];
	}
}

int SDL_PollEvent(SDL_Event *event)
{
	if (ev_head == ev_tail) refresh_input();
	if (ev_head == ev_tail) return 0;
	if (event) *event = evq[ev_head];
	ev_head = (ev_head + 1) % EVQ;
	return 1;
}

Uint8 *SDL_GetKeyState(int *numkeys)
{
	if (numkeys) *numkeys = SDLK_LAST;
	return keystate;
}

SDLMod SDL_GetModState(void) { return KMOD_NONE; }
int SDL_EnableUNICODE(int enable) { int old = unicode_on; if (enable >= 0) unicode_on = enable; return old; }

int SDL_NumJoysticks(void) { return 0; }
int SDL_JoystickEventState(int state) { return state; }

/* ---- CD audio: none ---------------------------------------------------------------------- */

SDL_CD *SDL_CDOpen(int drive) { (void)drive; return NULL; }
void SDL_CDClose(SDL_CD *cd) { (void)cd; }
CDstatus SDL_CDStatus(SDL_CD *cd) { (void)cd; return CD_TRAYEMPTY; }
int SDL_CDPlayTracks(SDL_CD *cd, int a, int b, int c, int d) { (void)cd; (void)a; (void)b; (void)c; (void)d; return -1; }
int SDL_CDStop(SDL_CD *cd) { (void)cd; return -1; }
