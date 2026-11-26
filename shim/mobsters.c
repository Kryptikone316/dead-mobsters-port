/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Everything the port layer knows about Dead Mobsters' insides, for the pause and intro-skip
 * features and for weapon cycling. Neither the game's files nor its code in memory are changed.
 * The one thing written is the main process's "how long until I run again" counter, to end the
 * intro's two waits early; the game then runs its own end-of-intro code.
 *
 * The positions below are instruction offsets in mobsters.dcb's main process (tools/dcbdis.py
 * prints them). They are checked against the loaded program first; if they do not match, every
 * function here reports "elsewhere" and the features stay off.
 */
#include <stdio.h>

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
