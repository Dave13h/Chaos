/***************************************************************
*  sound_hildon.c                                  #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  Hildon sound routines                           #.....#     *
*                                                  #######     *
*                                                              *
***************************************************************/

/*
############
#..Globals.#
############
*/
#include "globals.h"
#include "structs.h"
#include "main.h"
#include "log.h"
#include "chaos.h"
#include "display_hildon.h"

extern HildonWindow *window;

/*
##################
#..beep_hildon().#
##################
*/
void beep_hildon(void)
{
    gdk_window_beep((GdkWindow *) window);
}
