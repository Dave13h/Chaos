/***************************************************************
*  input_x11.c                                     #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  X11 input routines                              #.....#     *
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

extern Display* display;
extern Window win;
extern bool forceupdate;
extern Atom WM_DELETE_WINDOW;

/* -==========-
     Key Notes :
    -==========-

    Convert xkeysym keycodes to our own internal enum'd key codes. This
    will help with multiple input toolkits with hopefully minimal amount
    of coding required. We take the input toolkits keysyms and return our
    own values which are declared CE_KEYS (input_common.h).

    No point in psudo-mapping standard keys such as '1' or 'a' as the
    handler function looks at the ascii value as a 'char' for comparision.
*/
/*
######################
#..x11_special_key().#
######################
*/
static int x11_special_key(int key)
{
    switch (key) {
        case XK_Return:
        case XK_KP_Enter:
        case XK_Linefeed:
            return CE_RETURN;
        case XK_Escape:
            return CE_ESCAPE;

        case XK_BackSpace:
            return CE_BACKSPACE;
        case XK_Insert:
            return CE_INSERT;
        case XK_Delete:
            return CE_DELETE;
        case XK_Home:
            return CE_HOME;
        case XK_End:
            return CE_END;
        case XK_Page_Up:
            return CE_PGUP;
        case XK_Page_Down:
            return CE_PGDN;

        case XK_Up:                 /* Cursor Movement */
        case XK_KP_8:
            return CE_UP;
        case XK_Down:
        case XK_KP_2:
            return CE_DOWN;
        case XK_Left:
        case XK_KP_4:
            return CE_LEFT;
        case XK_Right:
        case XK_KP_6:
            return CE_RIGHT;

        case XK_KP_1:               /*  -- Diagonals */
            return CE_DOWNLEFT;
        case XK_KP_3:
            return CE_DOWNRIGHT;
        case XK_KP_5:
            return CE_ACTION;
        case XK_KP_7:
            return CE_UPLEFT;
        case XK_KP_9:
            return CE_UPRIGHT;

        case XK_F1:                 /* Function Keys */
            return CE_F1;
        case XK_F2:
            return CE_F2;
        case XK_F3:
            return CE_F3;
        case XK_F4:
            return CE_F4;
        case XK_F5:
            return CE_F5;
        case XK_F6:
            return CE_F6;
        case XK_F7:
            return CE_F7;
        case XK_F8:
            return CE_F8;
        case XK_F9:
            return CE_F9;
        case XK_F10:
            return CE_F10;
        case XK_F11:
            return CE_F11;
        case XK_F12:
            return CE_F12;

        case XK_Tab:
            return CE_TAB;
    }
    return 0;
}

/*
    -=====================-
     Event Handler Notes :
   -=====================-

    Because X11/xlib uses xevents for all input/events the functions
    called from input_common.c are piped to a single xevents checker
    function instead of individual functions. This means we dont have
    to deal with callback functions required by XNextEvent and such
    to see what the next type of event is coming as this would interupt
    the SIGAlrm timers... which wouldnt be good =F
*/
/*
######################
#..checkforxevents().#
######################
*/
int checkforxevents(void)
{
    XEvent event;
    KeySym key;
    char text[2];

    memset(text,'\0',sizeof(text));

    if (XPending(display)){
        XNextEvent(display,&event);
        switch (event.type) {
            case Expose:
                XResizeWindow(display, win, WIN_WIDTH, WIN_HEIGHT);
                forceupdate = true;
                break;
            case ClientMessage:
                if (event.xclient.data.l[0] == WM_DELETE_WINDOW) {
                    shutdown_chaos();
                }
                break;
            case KeyPress:
                XLookupString(&event.xkey,text,255,&key,0);
                if(key > 122){
                    return x11_special_key(key);
                }
                return key;
        }
    }
    return 0;
}

