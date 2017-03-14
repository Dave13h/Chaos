/***************************************************************
*  input_ncurses.c                                 #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  ncurses input routines                          #.....#     *
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

extern bool     forceupdate;
extern WINDOW   *window;

/* -==========-
     Key Notes :
    -==========-

    Convert NCURSES keycodes to our own internal enum'd key codes. This
    will help with multiple input toolkits with hopefully minimal amount
    of coding required. We take the input toolkits keysyms and return our
    own values which are declared CE_KEYS (input_common.h).

    No point in psudo-mapping standard keys such as '1' or 'a' as the
    handler function looks at the ascii value as a 'char' for comparision.
*/
/*
##########################
#..ncurses_special_key().#
##########################
*/
static int ncurses_special_key(int key)
{
    switch (key) {
        case KEY_ENTER:
        case 10:
            return CE_RETURN;
        case KEY_EXIT:
            return CE_ESCAPE;

        case KEY_BACKSPACE:
            return CE_BACKSPACE;
        case KEY_IL:
            return CE_INSERT;
        case KEY_DC:
            return CE_DELETE;
        case KEY_HOME:
            return CE_HOME;
        case KEY_END:
            return CE_END;
        case KEY_NPAGE:
            return CE_PGUP;
        case KEY_PPAGE:
            return CE_PGDN;

        case KEY_UP:                /* Cursor Movement */
            return CE_UP;
        case KEY_DOWN:
            return CE_DOWN;
        case KEY_LEFT:
            return CE_LEFT;
        case KEY_RIGHT:
            return CE_RIGHT;

        case KEY_A1:                /*  -- Diagonals */
            return CE_DOWNLEFT;
        case KEY_A3:
            return CE_DOWNRIGHT;
        case KEY_B2:
            return CE_ACTION;
        case KEY_C1:
            return CE_UPLEFT;
        case KEY_C3:
            return CE_UPRIGHT;

        case KEY_F(1):              /* Function Keys */
            return CE_F1;
        case KEY_F(2):
            return CE_F2;
        case KEY_F(3):
            return CE_F3;
        case KEY_F(4):
            return CE_F4;
        case KEY_F(5):
            return CE_F5;
        case KEY_F(6):
            return CE_F6;
        case KEY_F(7):
            return CE_F7;
        case KEY_F(8):
            return CE_F8;
        case KEY_F(9):
            return CE_F9;
        case KEY_F(10):
            return CE_F10;
        case KEY_F(11):
            return CE_F11;
        case KEY_F(12):
            return CE_F12;

        case 9:
            return CE_TAB;
    }
    return 0;
}


/*
###########################
#..checkforncursesinput().#
###########################
*/
int checkforncursesinput(void)
{
    int key;
    char *kname;

    key = wgetch(window);
    kname = (char *) keyname(key);

    /* Skip mouse events */
    if(key == KEY_MOUSE)
        return 0;

    /* Escaped keys we care about */
    if( strncmp(kname,"^?",strlen(kname)) == 0 ||
        strncmp(kname,"^H",strlen(kname)) == 0){
        return CE_BACKSPACE;
    }

    if(key == 9 || key > 122)
        return ncurses_special_key(key);

    return key;
}
