/***************************************************************
*  input_hildon.c                                  #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  Hildon input routines                           #.....#     *
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
#include "input_common.h"
#include "display_common.h"
#include "display_hildon.h"

static int hildonkey=0;

/* -==========-
     Key Notes :
    -==========-

    Ok, so Hildon/GTK+ is a little different to X11 or Ncurses in as much as
    GTK want so control everything internally instead of letting our app run
    and ask GTK for input when we want. So basically what we need to do here
    is have GTK run in the background and when it gets a 'signal' for something
    happening (like a key_press_event), we run a function to capture that and
    store it in a global (hildonkey) until we want to read it.

    This might create some issues like missing a keypress if the signal fires
    more than once before we check the global. Time will tell.

*/
/*
#########################
#..hildon_special_key().#
#########################
*/
static int hildon_special_key(int key)
{
    switch (key) {
        case 65293:
            return CE_RETURN;
        case 65307:
            return CE_ESCAPE;

        case 65288:
            return CE_BACKSPACE;

        case 65362:                 /* Cursor Movement */
            return CE_UP;
        case 65364:
            return CE_DOWN;
        case 65361:
            return CE_LEFT;
        case 65363:
            return CE_RIGHT;

        case 65470:                 /* Function Keys */
            return CE_F1;
        case 65471:
            return CE_F2;
        case 65472:
            return CE_F3;
        case 65473:
            return CE_F4;
        case 65474:
            return CE_F5;
        case 65475:
            return CE_F6;
        case 65476:
            return CE_F7;
        case 65477:
            return CE_F8;
        case 65478:
            return CE_F9;
        case 65479:
            return CE_F10;
        case 65480:
            return CE_F11;
        case 65481:
            return CE_F12;
    }
    return 0;
}

/*
###################
#..gethildonkey().#
###################
*/
void gethildonkey (GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    hildonkey = event->keyval;
    if(hildonkey > 122)
        hildonkey = hildon_special_key(event->keyval);
}

/*
##########################
#..checkforhildoninput().#
##########################
*/
int checkforhildoninput(void)
{
    int sendkey=hildonkey;

    hildonkey=0; /* Clear the global store */

    doevents_hildon();

    return sendkey;
}

