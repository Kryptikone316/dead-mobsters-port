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

#endif
