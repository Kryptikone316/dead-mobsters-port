/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Everything the port layer knows about Dead Mobsters' insides. The game's files are never
 * changed, and neither is its code in memory. What is written, while the game runs:
 *   - the main process's "how long until I run again" counter, to end the intro's two waits early
 *     (the game then runs its own end-of-intro code);
 *   - for the title menu: the menu variable, and the text area of the three loaded menu pictures;
 *   - for Stage Select: the game's own "start level" and "continue" variables, the ones its Game
 *     Over screen uses.
 *
 * The positions below are instruction offsets in mobsters.dcb's main process (tools/dcbdis.py
 * prints them). They are checked against the loaded program first; if they do not match, every
 * function here reports "elsewhere" and the features stay off.
 */
#include <stdio.h>
#include <string.h>

#include "fxi.h"
#include "mobsters.h"

/* A process waiting in a FRAME statement has its code pointer on the instruction after it. */
#define WAIT_INTRO_LOGO     166     /* FRAME(16000): the company logo, 160 frames            */
#define WAIT_INTRO_TITLE    267     /* FRAME(84000): the title sequence, 840 frames          */
#define MENU_FIRST          320     /* the title menu loop's waits lie in this range         */
#define MENU_LAST           425
#define WAIT_IN_LEVEL       5278    /* the once-per-frame wait of the level loop             */

#define G_LEVEL_DONE        1844    /* set when a level (or a story card) is finished        */
#define G_HEALTH            1688    /* below 0: game over                                    */
#define G_WEAPON            1644    /* current weapon, 1 to 4                                */
#define G_LEVEL             1684    /* the level being shown                                 */
#define G_MENU              1604    /* title menu entry: 1 START 1, 2 START 2, 3 EXIT. The  */
                                    /* game takes the character from it                      */
#define G_SKIP_MENU         1656    /* 1: go on without the title menu (set by "continue")   */
#define G_START_LEVEL       1660    /* the level a new game starts at (set by "continue")    */
#define G_FPG               1664    /* the library number of deadmob.fpg                     */

static const struct { int at, wait; } checks[] = {
	{ WAIT_INTRO_LOGO, 16000 }, { WAIT_INTRO_TITLE, 84000 }, { WAIT_IN_LEVEL, 100 },
};

static INSTANCE *main_instance(void)
{
	INSTANCE *i;
	if (!mainproc || !globaldata) return NULL;
	for (i = first_instance; i; i = i->next)
		if (i->proc == mainproc) return i;
	return NULL;
}

static int program_matches(void)
{
	static int result = -1;
	unsigned n;
	if (result >= 0) return result;
	result = 1;
	for (n = 0; n < sizeof(checks) / sizeof(checks[0]); n++) {
		int at = checks[n].at;
		if ((at + 1) * (int)sizeof(int) > mainproc->code_size
		    || mainproc->code[at - 3] != MN_PUSH
		    || mainproc->code[at - 2] != checks[n].wait
		    || mainproc->code[at - 1] != MN_FRAME) {
			result = 0;
			break;
		}
	}
	if (!result)
		fprintf(stderr, "mobsters: this is not the expected mobsters.dcb; pause and intro skip are off\n");
	return result;
}

enum mob_place mob_where(void)
{
	INSTANCE *m = main_instance();
	int at;
	if (!m || !program_matches()) return MOB_ELSEWHERE;
	at = (int)(m->codeptr - m->code);
	if (at == WAIT_INTRO_LOGO || at == WAIT_INTRO_TITLE) return MOB_INTRO;
	if (at >= MENU_FIRST && at <= MENU_LAST) return MOB_MENU;
	if (at == WAIT_IN_LEVEL && !GLODWORD(G_LEVEL_DONE) && GLODWORD(G_HEALTH) >= 0) return MOB_IN_LEVEL;
	return MOB_ELSEWHERE;
}

void mob_end_intro_wait(void)
{
	INSTANCE *m = main_instance();
	if (!m || mob_where() != MOB_INTRO) return;
	/* The engine takes 100 off after this and runs the process once the counter is below 100.
	 * Leaving exactly 100 means "run on the next frame" without shortening the wait after it. */
	if (LOCDWORD(m, FRAME_PERCENT) > 100) LOCDWORD(m, FRAME_PERCENT) = 100;
}

int mob_weapon(void)
{
	if (!globaldata) return 0;
	return GLODWORD(G_WEAPON);
}

int mob_level(void) { return globaldata ? GLODWORD(G_LEVEL) : 0; }
int mob_alive(void) { return globaldata ? GLODWORD(G_HEALTH) >= 0 : 0; }

void mob_menu_select(int on_exit, int mobster)
{
	if (mob_where() == MOB_MENU) GLODWORD(G_MENU) = on_exit ? 3 : mobster;
}

void mob_request_stage(int level, int mobster)
{
	if (mob_where() != MOB_INTRO) return;
	GLODWORD(G_SKIP_MENU) = 1;
	GLODWORD(G_START_LEVEL) = level;
	GLODWORD(G_MENU) = mobster;
}

int mob_stage_cleared(int level)
{
	INSTANCE *m = main_instance();
	if (!m || !program_matches()) return 0;
	return (int)(m->codeptr - m->code) == WAIT_IN_LEVEL && GLODWORD(G_LEVEL_DONE)
	       && GLODWORD(G_HEALTH) >= 0 && GLODWORD(G_LEVEL) == level;
}

/* ---- the title menu --------------------------------------------------------------------- */

/* The three menu pictures (graphs 15, 16, 17: the arrow on START 1, START 2, EXIT) have their
 * entries replaced by PLAY GAME and EXIT, in memory. Letters are drawn like the originals: 1
 * pixel white strokes, 8 high, with a dark outline; the arrow and both colours are taken from
 * graph 15 itself. Because it is part of the picture, it fades and dims like the rest. */
static const struct { char c; const char *row[8]; } menu_font[] = {
	{ 'A', { "..#..", ".#.#.", ".#.#.", ".#.#.", "#...#", "#####", "#...#", "#...#" } },
	{ 'E', { "#####", "#....", "#....", "#####", "#....", "#....", "#....", "#####" } },
	{ 'G', { ".####.", "#....#", "#.....", "#.....", "#..###", "#....#", "#....#", ".####." } },
	{ 'I', { "#", "#", "#", "#", "#", "#", "#", "#" } },
	{ 'L', { "#....", "#....", "#....", "#....", "#....", "#....", "#....", "#####" } },
	{ 'M', { "#.....#", "##...##", "##...##", "#.#.#.#", "#.#.#.#", "#..#..#", "#..#..#", "#.....#" } },
	{ 'P', { "#####.", "#....#", "#....#", "#....#", "#####.", "#.....", "#.....", "#....." } },
	{ 'T', { "#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.." } },
	{ 'X', { "#....#", ".#..#.", ".#..#.", "..##..", "..##..", ".#..#.", ".#..#.", "#....#" } },
	{ 'Y', { "#.....#", ".#...#.", "..#.#..", "...#...", "...#...", "...#...", "...#...", "...#..." } },
	{ 0, { 0 } }
};

#define MENU_AREA_X0     100      /* what is cleared: the three original entries */
#define MENU_AREA_X1     220
#define MENU_AREA_Y0     155
#define MENU_AREA_Y1     198
#define ARROW_X          133      /* the arrow beside START 1 in graph 15 */
#define ARROW_W            9
#define ARROW_Y          158
#define ITEM_H            10
#define TEXT_X           134      /* left edge of the letters */
#define ARROW_GAP         11      /* from the arrow's left edge to the letters */
#define ITEM1_Y          159      /* top of the letters */
#define ITEM2_Y          175

static void draw_menu_text(Uint8 *pic, int pitch, int x, int y, const char *text, Uint8 white)
{
	int k, r, c;
	for (; *text; text++) {
		if (*text == ' ') { x += 4; continue; }
		for (k = 0; menu_font[k].c && menu_font[k].c != *text; k++) { }
		if (!menu_font[k].c) continue;
		for (r = 0; r < 8; r++)
			for (c = 0; menu_font[k].row[r][c]; c++)
				if (menu_font[k].row[r][c] == '#') pic[(y + r) * pitch + x + c] = white;
		x += (int)strlen(menu_font[k].row[0]) + 2;
	}
}

static void outline_menu_text(Uint8 *pic, int pitch, Uint8 background, Uint8 white, Uint8 dark)
{
	int x, y, dx, dy;
	for (y = MENU_AREA_Y0 + 1; y < MENU_AREA_Y1 - 1; y++)
		for (x = TEXT_X - 1; x < MENU_AREA_X1 - 1; x++) {
			if (pic[y * pitch + x] != background) continue;
			for (dy = -1; dy <= 1; dy++)
				for (dx = -1; dx <= 1; dx++)
					if (pic[(y + dy) * pitch + x + dx] == white) pic[y * pitch + x] = dark;
		}
}

void mob_prepare_menu(void)
{
	static int done = 0;
	GRAPH *g[3];
	Uint8 arrow[ITEM_H][ARROW_W], background, white = 0, dark = 0;
	int n, x, y;

	if (done || !globaldata || !program_matches()) return;
	for (n = 0; n < 3; n++) {
		g[n] = bitmap_get(GLODWORD(G_FPG), 15 + n);
		if (!g[n] || g[n]->width != 320 || g[n]->height != 240 || g[n]->depth != 8) return;
	}
	done = 1;

	{
		const Uint8 *p = (const Uint8 *)g[0]->data;
		int pitch = g[0]->pitch;
		background = p[0];
		/* the outline is the first coloured pixel of the first letter, the stroke the next other one */
		for (x = ARROW_X + ARROW_W; x < MENU_AREA_X1 && !dark; x++)
			if (p[(ARROW_Y + 3) * pitch + x] != background) dark = p[(ARROW_Y + 3) * pitch + x];
		for (; x < MENU_AREA_X1 && !white; x++)
			if (p[(ARROW_Y + 3) * pitch + x] != background && p[(ARROW_Y + 3) * pitch + x] != dark)
				white = p[(ARROW_Y + 3) * pitch + x];
		for (y = 0; y < ITEM_H; y++)
			memcpy(arrow[y], p + (ARROW_Y + y) * pitch + ARROW_X, ARROW_W);
	}
	if (!white || !dark) return;

	for (n = 0; n < 3; n++) {
		Uint8 *pic = (Uint8 *)g[n]->data;
		int pitch = g[n]->pitch;
		int arrow_y = (n == 2 ? ITEM2_Y : ITEM1_Y) - 1;      /* 15 and 16: PLAY GAME; 17: EXIT */

		for (y = MENU_AREA_Y0; y < MENU_AREA_Y1; y++)
			memset(pic + y * pitch + MENU_AREA_X0, background, MENU_AREA_X1 - MENU_AREA_X0);
		draw_menu_text(pic, pitch, TEXT_X, ITEM1_Y, "PLAY GAME", white);
		draw_menu_text(pic, pitch, TEXT_X, ITEM2_Y, "EXIT", white);
		outline_menu_text(pic, pitch, background, white, dark);
		for (y = 0; y < ITEM_H; y++)
			memcpy(pic + (arrow_y + y) * pitch + TEXT_X - ARROW_GAP, arrow[y], ARROW_W);
		g[n]->modified = 1;
	}
}
