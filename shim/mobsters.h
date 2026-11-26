/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef MOBSTERS_H
#define MOBSTERS_H

enum mob_place {
	MOB_ELSEWHERE,      /* story cards, level changes, game over, or an unknown program */
	MOB_INTRO,          /* the logo and title sequence shown once after starting        */
	MOB_MENU,           /* the title menu                                               */
	MOB_IN_LEVEL        /* a level is being played                                      */
};

enum mob_place mob_where(void);
void mob_end_intro_wait(void);     /* does nothing outside MOB_INTRO */
int  mob_weapon(void);             /* 1 to 4 */
int  mob_level(void);              /* the game's level number, story cards included */
int  mob_alive(void);              /* not game over */

/* The title menu, shown as PLAY GAME / EXIT. */
void mob_prepare_menu(void);       /* redraws the three loaded menu pictures, once */
void mob_menu_select(int on_exit, int mobster);

/* Stage Select: call during the intro. The game then goes straight to this level. */
void mob_request_stage(int level, int mobster);
int  mob_stage_cleared(int level); /* the level is complete and the player alive */

#endif
