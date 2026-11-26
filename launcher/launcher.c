/* SPDX-License-Identifier: MIT
 *
 * Dead Mobsters launcher for the SF3500: Launch Game, Stage Select, Settings, Exit Game.
 *
 * It is a program of its own. run.sh starts it, reads what it wrote to launch.txt, runs the game,
 * and starts it again when the game ends. Its pictures are drawn from the game's own files, read
 * at run time: the title from deadmob.fpg, each stage's first screen from that stage's .fpg.
 *
 * It uses the same display, sound and gamepad layer as the game, and shares dm_settings with it.
 * The canvas is 480x360: 4:3, and padded to 16:9 it is 640 wide, the most the driver accepts.
 *
 * Test hooks: SF3500_KEYSCRIPT="30:D,40:A:4" (frame:button[:frames]; U D L R, A, B, T Start,
 * X Select), SF3500_SHOT_PREFIX + SF3500_SHOT_FRAMES, SF3500_QUIT_FRAME.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include "dm_settings.h"
#include "font_data.h"
#include "../sf3500/hwdisp.h"
#include "../sf3500/hwaudio.h"
#include "../sf3500/joykey.h"

#define W 480
#define H 360
#define FRAME_MS 33

typedef unsigned short u16;

static u16 canvas[W * H];
static unsigned frames = 0;

/* ---- colours ----------------------------------------------------------------------------- */

#define RGB(r, g, b) ((u16)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#define BLACK   RGB(0, 0, 0)
#define WHITE   RGB(240, 240, 240)
#define GOLD    RGB(255, 208, 72)
#define RED     RGB(200, 16, 16)
#define DIM     RGB(112, 112, 124)
#define HINT    RGB(168, 168, 180)
#define PANEL   RGB(28, 22, 26)
#define EDGE    RGB(96, 40, 40)

/* ---- drawing ----------------------------------------------------------------------------- */

static void fill(int x, int y, int w, int h, u16 c)
{
	int i, j;
	for (j = y < 0 ? 0 : y; j < y + h && j < H; j++)
		for (i = x < 0 ? 0 : x; i < x + w && i < W; i++)
			canvas[j * W + i] = c;
}

static void frame_rect(int x, int y, int w, int h, int t, u16 c)
{
	fill(x, y, w, t, c);
	fill(x, y + h - t, w, t, c);
	fill(x, y, t, h, c);
	fill(x + w - t, y, t, h, c);
}

static void blend(int x, int y, u16 c, int a)
{
	u16 under;
	int r, g, b;
	if (x < 0 || y < 0 || x >= W || y >= H || a <= 0) return;
	if (a >= 255) { canvas[y * W + x] = c; return; }
	under = canvas[y * W + x];
	r = (((c >> 11) & 31) * a + ((under >> 11) & 31) * (255 - a)) / 255;
	g = (((c >> 5) & 63) * a + ((under >> 5) & 63) * (255 - a)) / 255;
	b = ((c & 31) * a + (under & 31) * (255 - a)) / 255;
	canvas[y * W + x] = (u16)((r << 11) | (g << 5) | b);
}

static const struct glyph *glyph_of(const struct font *f, char c)
{
	if (c < 32 || c > 126) c = '?';
	return &f->glyphs[c - 32];
}

static int text_w(const struct font *f, const char *s)
{
	int w = 0;
	for (; *s; s++) w += glyph_of(f, *s)->advance;
	return w;
}

static void text_raw(const struct font *f, int x, int y, const char *s, u16 c)
{
	int i, j;
	for (; *s; s++) {
		const struct glyph *g = glyph_of(f, *s);
		for (j = 0; j < f->height; j++)
			for (i = 0; i < g->width; i++)
				blend(x + g->left + i, y + j, c, f->alpha[g->at + j * g->width + i]);
		x += g->advance;
	}
}

/* With a dark shadow; align: 0 left, 1 centre, 2 right of x. */
static void text(const struct font *f, int x, int y, int align, const char *s, u16 c)
{
	if (align) x -= align == 1 ? text_w(f, s) / 2 : text_w(f, s);
	text_raw(f, x + 2, y + 2, s, BLACK);
	text_raw(f, x, y, s, c);
}

/* The red arrow the game's own menu uses, pointing right (or left). */
static void arrow(int x, int cy, int size, int left, u16 c)
{
	int i, j;
	for (i = 0; i < size; i++) {
		int half = size - 1 - i;
		for (j = -half; j <= half; j++)
			blend(left ? x + size - 1 - i : x + i, cy + j, c, 255);
	}
}

/* ---- the game's graphics ----------------------------------------------------------------- */

struct image { int w, h; u16 *px; unsigned char *opaque; };

static unsigned char *read_file(const char *name, size_t *size)
{
	FILE *f = fopen(name, "rb");
	unsigned char *d = NULL;
	long n;
	if (!f) return NULL;
	if (fseek(f, 0, SEEK_END) == 0 && (n = ftell(f)) > 0 && fseek(f, 0, SEEK_SET) == 0) {
		d = (unsigned char *)malloc((size_t)n);
		if (d && fread(d, 1, (size_t)n, f) != (size_t)n) { free(d); d = NULL; }
		*size = (size_t)n;
	}
	fclose(f);
	return d;
}

static int le32(const unsigned char *p)
{
	return (int)(p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned)p[3] << 24));
}

/* An 8-bit FPG: 8 bytes of magic, 768 of palette (6 bits a channel), 576 of gamma ranges, then
 * one record per graphic: code, length, name[32], file[12], width, height, points, point data,
 * pixels. Colour 0 is transparent. lut (may be NULL) is applied to the palette. */
static int load_graphic(const unsigned char *d, size_t size, int code, struct image *out,
                        const unsigned char *lut)
{
	const unsigned char *pal = d + 8;
	size_t at = 8 + 768 + 576;

	memset(out, 0, sizeof(*out));
	if (!d || size < at || memcmp(d, "fpg", 3) != 0) return -1;
	while (at + 64 <= size) {
		int rec = le32(d + at);
		int w = le32(d + at + 52), h = le32(d + at + 56), points = le32(d + at + 60) & 0xfff;
		size_t pixels = at + 64 + 4 * (size_t)points;
		int i;

		if (w <= 0 || h <= 0 || w > 4096 || h > 4096 || pixels + (size_t)w * h > size) break;
		if (rec != code) { at = pixels + (size_t)w * h; continue; }
		out->px = (u16 *)malloc((size_t)w * h * 2);
		out->opaque = (unsigned char *)malloc((size_t)w * h);
		if (!out->px || !out->opaque) return -1;
		out->w = w; out->h = h;
		for (i = 0; i < w * h; i++) {
			int v = d[pixels + i], k, rgb[3];
			for (k = 0; k < 3; k++) {
				rgb[k] = pal[v * 3 + k] * 4;
				if (rgb[k] > 255) rgb[k] = 255;
				if (lut) rgb[k] = lut[rgb[k]];
			}
			out->px[i] = RGB(rgb[0], rgb[1], rgb[2]);
			out->opaque[i] = (v != 0);
		}
		return 0;
	}
	return -1;
}

static void free_image(struct image *im) { free(im->px); free(im->opaque); memset(im, 0, sizeof(*im)); }

/* Draws the part (sx,sy,sw,sh) of an image scaled into (dx,dy,dw,dh), nearest pixel. keyed:
 * leave transparent pixels alone; otherwise they come out black. */
static void blit(const struct image *im, int sx, int sy, int sw, int sh,
                 int dx, int dy, int dw, int dh, int keyed)
{
	int x, y;
	if (!im->px) return;
	for (y = 0; y < dh; y++) {
		int v = sy + y * sh / dh;
		for (x = 0; x < dw; x++) {
			int u = sx + x * sw / dw, X = dx + x, Y = dy + y;
			if (u < 0 || v < 0 || u >= im->w || v >= im->h || X < 0 || Y < 0 || X >= W || Y >= H) continue;
			if (im->opaque[v * im->w + u]) canvas[Y * W + X] = im->px[v * im->w + u];
			else if (!keyed) canvas[Y * W + X] = BLACK;
		}
	}
}

/* deadmob.fpg: the title picture and the two mobsters walking. */
#define WALK_FRAMES 8
#define WALKER_H   37
static struct image title_art, walker[2][WALK_FRAMES];
/* the colour values as the file has them, kept to redraw the picture test */
static unsigned char *mob_file = NULL;
static size_t mob_file_size = 0;

static void load_art(void)
{
	int m, i;
	mob_file = read_file("deadmob.fpg", &mob_file_size);
	if (!mob_file) { fprintf(stderr, "launcher: cannot read deadmob.fpg\n"); return; }
	load_graphic(mob_file, mob_file_size, 15, &title_art, NULL);
	for (m = 0; m < 2; m++)
		for (i = 0; i < WALK_FRAMES; i++)
			load_graphic(mob_file, mob_file_size, 200 + 8 * m + i, &walker[m][i], NULL);
}

/* A stage's first screen: its sky (graphic 3) with the start of its scenery (graphic 1). */
static const char *stage_file[DM_AREAS][DM_STAGES_PER_AREA] = {
	{ "01.fpg", "02.fpg", "b1.fpg" }, { "03.fpg", "04.fpg", "b2.fpg" },
	{ "05.fpg", "06.fpg", "b3.fpg" }, { "07.fpg", "08.fpg", "b4.fpg" },
};

struct scene { struct image sky, ground; };

static void load_scene(struct scene *s, const char *name, const unsigned char *lut)
{
	size_t size = 0;
	unsigned char *d = read_file(name, &size);
	free_image(&s->sky);
	free_image(&s->ground);
	if (!d) return;
	load_graphic(d, size, 3, &s->sky, lut);
	load_graphic(d, size, 1, &s->ground, lut);
	free(d);
}

static void draw_scene(const struct scene *s, int dx, int dy, int dw, int dh)
{
	fill(dx, dy, dw, dh, BLACK);
	blit(&s->sky, 0, 0, 320, 240, dx, dy, dw, dh, 1);
	blit(&s->ground, 0, 0, 320, 240, dx, dy, dw, dh, 1);
}

/* ---- sound ------------------------------------------------------------------------------- */

static Mix_Chunk *menu_music = NULL, *shot = NULL;
static int audio_up = 0;

static void postmix(void *udata, Uint8 *stream, int len)
{
	(void)udata;
	hwaudio_write((const short *)stream, len / 4);
}

static void sound_start(const struct dm_settings *s)
{
	SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);     /* pacing only; hwaudio drives the device */
	if (SDL_Init(SDL_INIT_AUDIO) < 0 || Mix_OpenAudio(HWAUDIO_RATE, AUDIO_S16SYS, 2, 2048) < 0) {
		fprintf(stderr, "launcher: no audio: %s\n", SDL_GetError());
		return;
	}
	audio_up = 1;
	hwaudio_init();
	Mix_SetPostMix(postmix, NULL);
	menu_music = Mix_LoadWAV("lowmenu.wav");
	shot = Mix_LoadWAV("hgun.wav");
	if (menu_music) {
		Mix_VolumeChunk(menu_music, dm_mix_volume(s->music));
		Mix_PlayChannel(0, menu_music, -1);
	}
}

static void sound_stop(void)
{
	if (!audio_up) return;
	audio_up = 0;
	Mix_HaltChannel(-1);
	Mix_CloseAudio();
	hwaudio_deinit();
}

static void music_volume_changed(const struct dm_settings *s)
{
	if (menu_music) Mix_VolumeChunk(menu_music, dm_mix_volume(s->music));
}

static void play_test_shot(const struct dm_settings *s)
{
	if (!shot) return;
	Mix_VolumeChunk(shot, dm_mix_volume(s->sound));
	Mix_PlayChannel(1, shot, 0);
}

/* ---- pad ---------------------------------------------------------------------------------- */

#define BIT(b) (1u << (b))

#define SCRIPT_MAX 128
static struct { unsigned frame, hold, pad; } script[SCRIPT_MAX];
static int script_n = 0;

static void load_script(void)
{
	const char *e = getenv("SF3500_KEYSCRIPT");
	while (e && *e && script_n < SCRIPT_MAX) {
		char *end;
		unsigned long frame = strtoul(e, &end, 10), hold = 3;
		unsigned pad = 0;
		if (end == e || *end != ':') break;
		switch (end[1]) {
		case 'U': pad = BIT(JK_UP); break;
		case 'D': pad = BIT(JK_DOWN); break;
		case 'L': pad = BIT(JK_LEFT); break;
		case 'R': pad = BIT(JK_RIGHT); break;
		case 'A': pad = BIT(JK_A); break;
		case 'B': pad = BIT(JK_B); break;
		case 'T': pad = BIT(JK_START); break;
		case 'X': pad = BIT(JK_SELECT); break;
		}
		e = end + 2;
		if (*e == ':') hold = strtoul(e + 1, (char **)&e, 10);
		script[script_n].frame = (unsigned)frame;
		script[script_n].hold = (unsigned)hold;
		script[script_n].pad = pad;
		script_n++;
		if (*e != ',') break;
		e++;
	}
}

static unsigned pad_now(void)
{
	unsigned bits = joykey_read();
	int i;
	for (i = 0; i < script_n; i++)
		if (frames >= script[i].frame && frames < script[i].frame + script[i].hold) bits |= script[i].pad;
	return bits;
}

/* New presses this frame; held directions repeat. Nothing counts until every button has been
 * let go once: the Select + Start that ended a game is still down when the launcher starts. */
static unsigned pad_pressed(unsigned bits)
{
	static unsigned previous = 0;
	static int armed = 0;
	static unsigned held_since = 0;
	const unsigned dirs = BIT(JK_UP) | BIT(JK_DOWN) | BIT(JK_LEFT) | BIT(JK_RIGHT);
	unsigned pressed;

	if (!armed) {
		if (!bits) armed = 1;
		previous = bits;
		return 0;
	}
	pressed = bits & ~previous;
	if ((bits & dirs) != (previous & dirs)) held_since = frames;
	else if ((bits & dirs) && frames - held_since >= 12 && (frames - held_since) % 4 == 0)
		pressed |= bits & dirs;
	previous = bits;
	return pressed;
}

/* ---- screens ------------------------------------------------------------------------------ */

enum screen { SCREEN_MAIN, SCREEN_STAGES, SCREEN_SETTINGS };

static struct dm_settings settings;
static enum screen screen = SCREEN_MAIN;
static int unlocked = 0;
static int main_sel = 0, set_sel = 0, area_sel = 0, stage_sel = 0;
static unsigned note_until = 0;

static const char *main_items[4] = { "LAUNCH GAME", "STAGE SELECT", "SETTINGS", "EXIT GAME" };
static const char *area_names[DM_AREAS] = { "The Suburbs", "The Sewers", "Scaffolding", "The Factory" };
static const char *stage_names[DM_STAGES_PER_AREA] = { "Stage 1", "Stage 2", "Boss" };

enum {
	SET_MOBSTER, SET_MUSIC, SET_SOUND, SET_BRIGHTNESS, SET_CONTRAST, SET_GAMMA, SET_RESET, SET_BACK,
	SET_ROWS
};
static const char *set_names[SET_ROWS] = {
	"Mobster", "Music", "Sound effects", "Brightness", "Contrast", "Gamma", "Reset to defaults", "Back"
};

static struct scene stage_view;
static int stage_view_of = -1;
static struct scene test_view;
static int test_view_stale = 1;
static struct image test_walker[2][WALK_FRAMES];

static void draw_main(void)
{
	int i;
	/* the title and its subtitle, from the game's own menu picture (rows 24 to 136) */
	blit(&title_art, 20, 24, 280, 112, 30, 14, 420, 168, 0);
	for (i = 0; i < 4; i++) {
		int y = 202 + i * 31;
		int locked = (i == 1 && !unlocked);
		u16 c = locked ? DIM : (i == main_sel ? GOLD : WHITE);
		text(&font_item, W / 2, y, 1, main_items[i], c);
		if (i == main_sel) arrow(W / 2 - text_w(&font_item, main_items[i]) / 2 - 20, y + 11, 8, 0, RED);
		if (locked)
			text(&font_small, W / 2 + text_w(&font_item, main_items[i]) / 2 + 10, y + 4, 0, "LOCKED", DIM);
	}
	if (frames < note_until)
		text(&font_small, W / 2, 336, 1, "Finish the whole game once to unlock Stage Select.", GOLD);
	else
		text(&font_small, W / 2, 336, 1, "A: choose", HINT);
}

static void draw_stages(void)
{
	int a, s;
	text(&font_title, W / 2, 6, 1, "STAGE SELECT", RED);
	for (a = 0; a < DM_AREAS; a++) {
		int y = 54 + a * 36;
		text(&font_item, 18, y + 4, 0, area_names[a], a == area_sel ? GOLD : WHITE);
		for (s = 0; s < DM_STAGES_PER_AREA; s++) {
			int x = 176 + s * 98, on = (a == area_sel && s == stage_sel);
			fill(x, y, 92, 30, on ? EDGE : PANEL);
			frame_rect(x, y, 92, 30, on ? 2 : 1, on ? GOLD : EDGE);
			text(&font_item, x + 46, y + 4, 1, stage_names[s], on ? GOLD : WHITE);
		}
	}
	if (stage_view_of != area_sel * DM_STAGES_PER_AREA + stage_sel) {
		load_scene(&stage_view, stage_file[area_sel][stage_sel], NULL);
		stage_view_of = area_sel * DM_STAGES_PER_AREA + stage_sel;
	}
	frame_rect(159, 202, 162, 122, 1, EDGE);
	draw_scene(&stage_view, 160, 203, 160, 120);
	text(&font_small, W / 2, 336, 1, "A: play this stage      B: back", HINT);
}

static void bar(int x, int y, int value, int on)
{
	int i;
	for (i = 0; i < DM_VOLUME_MAX; i++)
		fill(x + i * 9, y + (i < value ? 0 : 5), 6, i < value ? 14 : 4, i < value ? (on ? GOLD : WHITE) : DIM);
}

/* The right-hand side: the chosen mobster and a picture test, both through the picture settings. */
static void draw_preview(void)
{
	const int px = 306, pw = 160;
	unsigned char lut[256];
	int i, step = (frames / 3) % WALK_FRAMES, m = settings.mobster - 1;

	if (test_view_stale) {
		int k, n;
		dm_colour_lut(&settings, lut);
		load_scene(&test_view, "01.fpg", lut);
		for (k = 0; k < 2; k++)
			for (n = 0; n < WALK_FRAMES; n++) {
				free_image(&test_walker[k][n]);
				if (mob_file)
					load_graphic(mob_file, mob_file_size, 200 + 8 * k + n, &test_walker[k][n], lut);
			}
		test_view_stale = 0;
	}
	frame_rect(px - 1, 53, pw + 2, 122, 1, EDGE);
	draw_scene(&test_view, px, 54, pw, 120);

	frame_rect(px - 1, 184, pw + 2, 98, 1, EDGE);
	fill(px, 185, pw, 96, PANEL);
	if (test_walker[m][step].px) {
		const struct image *w = &test_walker[m][step];
		/* the mobster fills the top WALKER_H rows of its frame; the rest is blank */
		blit(w, 0, 0, w->w, WALKER_H, px + pw / 2 - w->w, 185 + (96 - WALKER_H * 2) / 2,
		     w->w * 2, WALKER_H * 2, 1);
	}

	/* eight greys, black to white: with good settings every step can be told apart */
	dm_colour_lut(&settings, lut);
	for (i = 0; i < 8; i++) {
		int v = lut[i * 255 / 7];
		fill(px + i * 20, 291, 20, 20, RGB(v, v, v));
	}
	frame_rect(px - 1, 290, pw + 2, 22, 1, EDGE);
}

static void draw_settings(void)
{
	int i;
	char value[16];

	text(&font_title, W / 2, 6, 1, "SETTINGS", RED);
	for (i = 0; i < SET_ROWS; i++) {
		int y = 54 + i * 33, on = (i == set_sel);
		u16 c = on ? GOLD : WHITE;
		if (on) { fill(8, y - 3, 288, 29, PANEL); arrow(12, y + 11, 7, 0, RED); }
		text(&font_item, 26, y, 0, set_names[i], c);
		value[0] = 0;
		switch (i) {
		case SET_MOBSTER:    snprintf(value, sizeof(value), "%d", settings.mobster); break;
		case SET_MUSIC:      bar(198, y + 4, settings.music, on); break;
		case SET_SOUND:      bar(198, y + 4, settings.sound, on); break;
		case SET_BRIGHTNESS: snprintf(value, sizeof(value), settings.brightness ? "%+d" : "0", settings.brightness); break;
		case SET_CONTRAST:   snprintf(value, sizeof(value), settings.contrast ? "%+d" : "0", settings.contrast); break;
		case SET_GAMMA:      snprintf(value, sizeof(value), "%d.%d", settings.gamma / 10, settings.gamma % 10); break;
		}
		if (value[0]) {
			text(&font_item, 242, y, 1, value, c);
			if (on) { arrow(206, y + 11, 6, 1, RED); arrow(272, y + 11, 6, 0, RED); }
		}
	}
	draw_preview();
	text(&font_small, W / 2, 336, 1, "Left / Right: change      B: back", HINT);
}

static int step_value(int v, int dir, int lo, int hi)
{
	v += dir;
	return v < lo ? lo : v > hi ? hi : v;
}

static void change_setting(int dir)
{
	struct dm_settings before = settings;
	switch (set_sel) {
	case SET_MOBSTER:    settings.mobster = settings.mobster == 1 ? 2 : 1; break;
	case SET_MUSIC:      settings.music = step_value(settings.music, dir, 0, DM_VOLUME_MAX); break;
	case SET_SOUND:      settings.sound = step_value(settings.sound, dir, 0, DM_VOLUME_MAX); break;
	case SET_BRIGHTNESS: settings.brightness = step_value(settings.brightness, dir, -DM_PICTURE_RANGE, DM_PICTURE_RANGE); break;
	case SET_CONTRAST:   settings.contrast = step_value(settings.contrast, dir, -DM_PICTURE_RANGE, DM_PICTURE_RANGE); break;
	case SET_GAMMA:      settings.gamma = step_value(settings.gamma, dir, DM_GAMMA_MIN, DM_GAMMA_MAX); break;
	default: return;
	}
	if (set_sel == SET_MUSIC) music_volume_changed(&settings);
	if (set_sel == SET_SOUND) play_test_shot(&settings);
	if (before.brightness != settings.brightness || before.contrast != settings.contrast
	    || before.gamma != settings.gamma)
		test_view_stale = 1;
}

static void save_settings(void)
{
	if (dm_settings_save(&settings) != 0) fprintf(stderr, "launcher: cannot save %s\n", DM_SETTINGS_FILE);
}

/* Handles this frame's presses. Returns the request for run.sh once there is one. */
static const char *handle(unsigned p)
{
	static char request[32];
	int ok = (p & (BIT(JK_A) | BIT(JK_START))) != 0;
	int back = (p & BIT(JK_B)) != 0;
	int up = (p & BIT(JK_UP)) != 0, down = (p & BIT(JK_DOWN)) != 0;
	int dir = (p & BIT(JK_RIGHT)) ? 1 : (p & BIT(JK_LEFT)) ? -1 : 0;

	switch (screen) {
	case SCREEN_MAIN:
		if (up) main_sel = (main_sel + 3) % 4;
		if (down) main_sel = (main_sel + 1) % 4;
		if (up || down) note_until = 0;
		if (!ok) break;
		if (main_sel == 0) return "play";
		if (main_sel == 1) {
			if (unlocked) screen = SCREEN_STAGES;
			else note_until = frames + 90;
		}
		if (main_sel == 2) { screen = SCREEN_SETTINGS; set_sel = 0; test_view_stale = 1; }
		if (main_sel == 3) return "exit";
		break;
	case SCREEN_STAGES:
		if (up) area_sel = (area_sel + DM_AREAS - 1) % DM_AREAS;
		if (down) area_sel = (area_sel + 1) % DM_AREAS;
		if (dir) stage_sel = (stage_sel + DM_STAGES_PER_AREA + dir) % DM_STAGES_PER_AREA;
		if (back) screen = SCREEN_MAIN;
		else if (ok) {
			snprintf(request, sizeof(request), "stage %d", dm_stage_level[area_sel][stage_sel]);
			return request;
		}
		break;
	case SCREEN_SETTINGS:
		if (up) set_sel = (set_sel + SET_ROWS - 1) % SET_ROWS;
		if (down) set_sel = (set_sel + 1) % SET_ROWS;
		if (dir) change_setting(dir);
		if (ok && set_sel == SET_MOBSTER) change_setting(1);
		if (ok && set_sel == SET_RESET) {
			dm_settings_default(&settings);
			music_volume_changed(&settings);
			test_view_stale = 1;
		}
		if (back || (ok && set_sel == SET_BACK)) { save_settings(); screen = SCREEN_MAIN; }
		break;
	}
	return NULL;
}

/* ---- main ---------------------------------------------------------------------------------- */

static void write_ppm(const char *path)
{
	FILE *f = fopen(path, "wb");
	int i;
	if (!f) return;
	fprintf(f, "P6\n%d %d\n255\n", W, H);
	for (i = 0; i < W * H; i++) {
		u16 p = canvas[i];
		unsigned char c[3] = { (unsigned char)(((p >> 11) & 31) * 255 / 31),
		                       (unsigned char)(((p >> 5) & 63) * 255 / 63),
		                       (unsigned char)((p & 31) * 255 / 31) };
		fwrite(c, 1, 3, f);
	}
	fclose(f);
}

static int listed(const char *list, unsigned n)
{
	while (list && *list) {
		char *end;
		unsigned long v = strtoul(list, &end, 10);
		if (end == list) break;
		if (v == n) return 1;
		list = (*end == ',') ? end + 1 : end;
	}
	return 0;
}

static long now_ms(void)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (long)(t.tv_sec * 1000 + t.tv_nsec / 1000000);
}

static void pause_ms(long ms)
{
	struct timespec t = { ms / 1000, (ms % 1000) * 1000000L };
	if (ms > 0) nanosleep(&t, NULL);
}

static void select_stage(int level)
{
	int a, s;
	for (a = 0; a < DM_AREAS; a++)
		for (s = 0; s < DM_STAGES_PER_AREA; s++)
			if (dm_stage_level[a][s] == level) { area_sel = a; stage_sel = s; }
}

int main(int argc, char **argv)
{
	const char *shot_prefix = getenv("SF3500_SHOT_PREFIX"), *shot_frames = getenv("SF3500_SHOT_FRAMES");
	const char *quit_frame = getenv("SF3500_QUIT_FRAME");
	const char *request = NULL;
	FILE *done;
	long next;
	int i;

	dm_settings_load(&settings);
	done = fopen(DM_COMPLETED_FILE, "r");
	if (done) { unlocked = 1; fclose(done); }

	/* --stage <level>: coming back from a stage, so open Stage Select on it. */
	if (argc == 3 && !strcmp(argv[1], "--stage") && unlocked && dm_is_stage(atoi(argv[2]))) {
		select_stage(atoi(argv[2]));
		screen = SCREEN_STAGES;
		main_sel = 1;
	}

	load_script();
	joykey_init();
	if (hwdisp_init() != 0) fprintf(stderr, "launcher: no display driver, frames are not shown\n");
	load_art();
	sound_start(&settings);
	fprintf(stderr, "launcher: ready (stage select %s)\n", unlocked ? "unlocked" : "locked");

	next = now_ms();
	while (!request) {
		unsigned bits = pad_now(), pressed = pad_pressed(bits);

		/* the usual way out on this device */
		if ((bits & BIT(JK_SELECT)) && (pressed & (BIT(JK_SELECT) | BIT(JK_START))) && (bits & BIT(JK_START)))
			request = "exit";
		else
			request = handle(pressed);

		fill(0, 0, W, H, BLACK);
		if (screen == SCREEN_MAIN) draw_main();
		else if (screen == SCREEN_STAGES) draw_stages();
		else draw_settings();
		hwdisp_present(canvas, W, H, W * 2);

		frames++;
		if (shot_prefix && listed(shot_frames, frames)) {
			char path[512];
			snprintf(path, sizeof(path), "%s_%u.ppm", shot_prefix, frames);
			write_ppm(path);
		}
		if (quit_frame && frames >= (unsigned)atoi(quit_frame) && !request) request = "exit";

		next += FRAME_MS;
		if (next < now_ms()) next = now_ms();
		pause_ms(next - now_ms());
	}

	/* Settings changed and not yet saved (left with Select + Start) are kept too. */
	if (screen == SCREEN_SETTINGS) save_settings();

	/* Let go of the buttons before the next program reads them. */
	for (i = 0; i < 100 && joykey_read(); i++) pause_ms(20);

	{
		char line[40];
		snprintf(line, sizeof(line), "%s\n", request);
		if (dm_write_file(DM_REQUEST_FILE, line) != 0) fprintf(stderr, "launcher: cannot write %s\n", DM_REQUEST_FILE);
	}
	fprintf(stderr, "launcher: %s\n", request);
	sound_stop();
	hwdisp_deinit();
	SDL_Quit();
	return 0;
}
