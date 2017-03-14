/***************************************************************
*  sound_common.c                                  #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  common sound routines                           #.....#     *
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

#ifdef WITH_X11
    #include "sound_x11.h"
#endif

#ifdef WITH_NCURSES
    #include "sound_ncurses.h"
#endif

extern int frontend_mode;
extern char log_message[LOGMSGLEN];

/*
###########
#..beep().#
###########
*/
void CE_beep(void)
{
    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
        log_message,"BEEP!"));

    switch(frontend_mode){
    #ifdef WITH_X11
        case FE_X11:
            beep_x11();
            break;
    #endif
    #ifdef WITH_NCURSES
        case FE_NCURSES:
            beep_ncurses();
            break;
    #endif
    }
}
