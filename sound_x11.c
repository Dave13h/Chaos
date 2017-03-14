/***************************************************************
*  sound_x11.c                                     #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  X11 sound routines                              #.....#     *
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
#include "display_x11.h"

extern Display* display;

/*
###############
#..beep_x11().#
###############
*/
void beep_x11(void)
{
    XBell(display,100);
}
