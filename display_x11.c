/***************************************************************
*  display_x11.c                                   #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  draw routines for X11                           #.....#     *
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
#include "display_common.h"
#include "display_x11.h"

extern world                myworld;
extern player               myplayers[MAX_PLAYERS];
extern spells               myspells[MAX_SPELLS];
extern monster              mymonsters[MAX_MONSTERS];
extern tree                 mytrees[MAX_TREES];
extern wall                 mywalls[MAX_WALLS];
extern blob                 myblobs[MAX_BLOBS];
extern magic_special        mymagic_special[MAX_MAGIC_SPECIAL];
extern magic_upgrade        mymagic_upgrade[MAX_MAGIC_UPGRADE];
extern magic_balance        mymagic_balance[MAX_MAGIC_BALANCE];
extern magic_spell_attrib   mymagic_spell_attrib[MAX_MAGIC_SPELL_ATTRIB];
extern magic_ranged         mymagic_ranged[MAX_MAGIC_RANGED];

extern char infobar_text[255];
extern bool beepmsg;

extern char log_message[LOGMSGLEN];

extern char input_buffer[255];

extern int arenas[MAX_ARENAS][2];

extern history_log myhistory[MAX_HISTORY];
extern int history_count;

extern chat_log mychat[MAX_CHAT];
extern int chat_count;
extern char chat_buffer[MAX_CHATMSG_LENGTH];

static char *CE_COLORS_X11[] = {
    [CE_C_LGREY] = "A/A/A",
    [CE_C_GREY] = "7/7/7",
    [CE_C_DGREY] = "3/3/3",

    [CE_C_LVIOLET] = "F/0/C",
    [CE_C_VIOLET] = "B/0/F",
    [CE_C_DVIOLET] = "6/0/C",

    [CE_C_LWHITE] = "F/F/F",
    [CE_C_WHITE] = "F/F/F",
    [CE_C_DWHITE] = "A/A/A",

    [CE_C_LBLUE] = "0/0/8",
    [CE_C_BLUE] = "5/5/F",
    [CE_C_DBLUE] = "A/A/F",

    [CE_C_LGREEN] = "0/8/0",
    [CE_C_GREEN] = "0/F/0",
    [CE_C_DGREEN] = "9/F/9",

    [CE_C_LRED] = "F/6/6",
    [CE_C_RED] = "F/0/0",
    [CE_C_DRED] = "F/9/9",

    [CE_C_LORANGE] = "F/9/3",
    [CE_C_ORANGE] = "F/6/0",
    [CE_C_DORANGE] = "A/3/0",

    [CE_C_LYELLOW] = "F/E/7",
    [CE_C_YELLOW] = "F/E/0",
    [CE_C_DYELLOW] = "A/8/0",

    [CE_C_LBROWN] = "F/9/6",
    [CE_C_BROWN] = "9/6/3",
    [CE_C_DBROWN] = "6/3/0",

    [CE_C_LPURPLE] = "9/9/E",
    [CE_C_PURPLE] = "9/3/C",
    [CE_C_DPURPLE] = "6/0/9",

    [CE_C_LCYAN] = "6/F/F",
    [CE_C_CYAN] = "3/A/A",
    [CE_C_DCYAN] = "0/9/9"
};

static int CE_MULTI_COLORS_X11[][6] = {
    [CE_C_MULTI1] = {CE_C_YELLOW,CE_C_VIOLET,CE_C_BLUE,CE_C_GREEN,CE_C_RED,CE_C_CYAN},
    [CE_C_MULTI2] = {CE_C_RED,CE_C_LRED,CE_C_DRED,CE_C_RED,CE_C_LRED,CE_C_DRED},
    [CE_C_MULTI3] = {CE_C_GREEN,CE_C_LGREEN,CE_C_DGREEN,CE_C_GREEN,CE_C_LGREEN,CE_C_DGREEN}
};

Display*    display;
Window      win;
Atom WM_DELETE_WINDOW;
static GC   gc;

static XVisualInfo  visual_info;
static int          default_screen;
static int          default_depth;
static Colormap     default_cmap;
static XFontStruct  *font_info;
static XColor       fontcolor;
static Pixmap       pixbuff;

#ifdef WITH_NET
extern int mycolor;
extern int mypid;
extern bool net_enable;
extern bool net_dedicated;
extern int net_port;
#endif

/*
####################
#..listfonts_x11().#
####################
*/
void listfonts_x11(void) {
    char **fonts;
    int font_c;
    fonts = XListFonts(display,"*",1000,&font_c);
    for(--font_c;font_c>0;font_c--)
        console_log(__FILE__,__func__,__LINE__,LOG_ERROR,
            sprintf(log_message,"   %s",fonts[font_c]));
    XFreeFontNames(fonts);
}

/*
###############
#..init_x11().#
###############
*/
void init_x11 (void)
{
    static char *visual_class[] = {
        "StaticGray",
        "GrayScale",
        "StaticColor",
        "PseudoColor",
        "TrueColor",
        "DirectColor"
    };

    Visual *default_visual;
    XSetWindowAttributes swa;
    XWMHints *input_hints;
    char *fontname = "9x15";
    int i = 5;
    const char * env_display = getenv("DISPLAY");

    /* Failed to get DISPLAY EnvVar */
    if(env_display == NULL){
        find_front_end();
        return;
    }

    /* Failed to get valid display? */
    if((display = XOpenDisplay(env_display)) == NULL){
        find_front_end();
        return;
    }

    default_screen = DefaultScreen(display);
    default_visual = DefaultVisual(display,default_screen);
    default_depth  = DefaultDepth(display,default_screen);
    default_cmap   = DefaultColormap(display,default_screen);

    if (default_depth < 8) {
        console_log(__FILE__,__func__,__LINE__,LOG_ERROR, sprintf(log_message,
            "ERROR: Screen Depth of %d is too low [8bpp or higher required]",default_depth));
        find_front_end();
        return;
    }

    while(!XMatchVisualInfo(display,default_screen,default_depth,i--,&visual_info)){
        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
            "Can't find visual, trying next..."));
    }

    if(i<0){
        console_log(__FILE__,__func__,__LINE__,LOG_ERROR, sprintf(log_message,
            "ERROR: Couldn't find a suitable visual!"));
        find_front_end();
        return;
    }

    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
        "Setting up X server"));
    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
        "Using Screen => 0.%d",visual_info.screen));
    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
        "Using visual => %s",visual_class[++i]));
    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
        "Using depth => %d",visual_info.depth));

    visual_info.visual  = default_visual;

    swa.background_pixel = BlackPixel(display,default_screen);
    swa.event_mask = ExposureMask|StructureNotifyMask|KeyPressMask|
                        ButtonPressMask|ButtonReleaseMask;
    swa.colormap = default_cmap;

    win = XCreateWindow(
        display,
        RootWindow(display, visual_info.screen),
        0, 0,   /* Spawn window at x,y (who cares the xserver chooses anyway) */
        WIN_WIDTH, WIN_HEIGHT,
        0, /* Border Width (again the xserver knows best) */
        visual_info.depth,
        InputOutput,
        visual_info.visual,
        CWBackPixel|CWEventMask|CWColormap, /* Value Masks */
        &swa
    );

    WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", True);
    XSetWMProtocols(display, win, &WM_DELETE_WINDOW, 1);

    XStoreName(display, win, "Chaos");
    XMapWindow(display, win);

    /* Make sure we have input focus! */
    if((input_hints = XGetWMHints(display, win)) == NULL){
        XWMHints input_hints = {.input = True, .flags = InputHint};
        XSetWMHints(display, win, &input_hints);
    } else {
        input_hints->input = True;
        input_hints->flags |= InputHint;
        XSetWMHints(display, win, input_hints);
    }
    XFree(input_hints);

    /* Set up some Fonty Goodness */
    if ((font_info = XLoadQueryFont(display,fontname)) == NULL) {
        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
            "ERROR: Can't get font => 9x15"));

        char *fontname = "6x13";
        if ((font_info = XLoadQueryFont(display,fontname)) == NULL) {
            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "ERROR: Can't get font => 6x13"));

            listfonts_x11();
            shutdown_x11();
            find_front_end();
        }
    }
}

/*
###################
#..shutdown_x11().#
###################
*/
void shutdown_x11(void) {
    if(font_info != NULL)
        XUnloadFont(display,font_info->fid);
    XDestroyWindow(display, win);
    XCloseDisplay(display);
}

/*
#######################
#..enable_color_x11().#
#######################
*/
static void enable_color_x11 (int color)
{
    char itemcolor[10];

    if(color == CE_C_MULTI1 || color == CE_C_MULTI2 || color == CE_C_MULTI3){
        color = CE_MULTI_COLORS_X11[color][rand()%6];
    }

    sprintf(itemcolor,"rgb:%s",CE_COLORS_X11[color]);

    if(XParseColor(display, default_cmap, itemcolor, &fontcolor)){
            XAllocColor(display, default_cmap,&fontcolor);
            XSetForeground(display, gc, fontcolor.pixel);
    } else {
            XSetForeground(display, gc, WhitePixel(display,visual_info.screen));
            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "Could not Allocate Colour (%s), Falling back to White",itemcolor));
    }
}

/*
########################
#..disable_color_x11().#
########################
*/
static void disable_color_x11 (void)
{
    XSetForeground(display, gc, WhitePixel(display,visual_info.screen));
    XSetBackground(display, gc, BlackPixel(display,visual_info.screen));
}

/*
#######################
#..cleanbuffers_x11().#
#######################
*/
static void cleanbuffers_x11 (void)
{
    gc = XCreateGC(display, win, 0, (XGCValues*) NULL);
    pixbuff = XCreatePixmap(display, win, WIN_WIDTH, WIN_HEIGHT, default_depth);

    XSetFont(display, gc, font_info->fid);

    XSetForeground(display, gc, BlackPixel(display,visual_info.screen));
    XSetBackground(display, gc, BlackPixel(display,visual_info.screen));

    /* Clear Pixmap buffer to black */
    XFillRectangle(display, pixbuff, gc, 0, 0, WIN_WIDTH, WIN_HEIGHT);

    XSetForeground(display, gc, WhitePixel(display,visual_info.screen));
}

/*
#######################
#..flushbuffers_x11().#
#######################
*/
static void flushbuffers_x11 (void)
{
    XCopyArea(display, pixbuff, win, gc, 0, 0, WIN_WIDTH, WIN_HEIGHT, 0, 0);

    XFlush(display);    /* Show us =) */

    /* Clean up on isle 3 */
    XFreePixmap(display, pixbuff);
    XFreeGC(display,gc);
}

/*
#########################
#..drawplayerlist_x11().#
#########################
*/
static void drawplayerlist_x11 (bool withsockets)
{
    int i;
    int pingcolor=-1;
    int sobump=0;
    char readyflag[2];
    char infotext[64];

    if(withsockets){
        XDrawString(display, pixbuff, gc, 180, 120,"| id | So | P | R | Player", 26);
        XDrawString(display, pixbuff, gc, 180, 135,
            "|----|----|---|---|-----------------------------", 48);
        sobump=45;
    } else {
        XDrawString(display, pixbuff, gc, 180, 120,"| id | P | R | Player", 21);
        XDrawString(display, pixbuff, gc, 180, 135,
            "|----|---|---|-----------------------------", 43);
    }

    /* Draw player list */
    for(i=0;i<myworld.players;i++){
        readyflag[0] = ' ';
        if(myplayers[i].ready)
            readyflag[0] = '*';

        /* Player ID */
        memset(infotext,'\0',sizeof(infotext));
        sprintf(infotext,"| %d  |",myplayers[i].id);
        XDrawString(display, pixbuff, gc, 180, 150+(i*15), infotext, strlen(infotext));

        if(withsockets){
            /* Player Socket */
            memset(infotext,'\0',sizeof(infotext));
            sprintf(infotext," %d  |",myplayers[i].socket);
            XDrawString(display, pixbuff, gc, 234, 150+(i*15), infotext, strlen(infotext));
        }

        /* Ping */
        if(strlen(myplayers[i].name) > 0){
            switch(myplayers[i].ping){
                case 4:
                case 3:
                    pingcolor = CE_C_GREEN;
                    break;
                case 2:
                    pingcolor = CE_C_CYAN;
                    break;
                case 1:
                    pingcolor = CE_C_RED;
                    break;
                case 0:
                default:
                    pingcolor = CE_C_DGREY;
                    break;
            }
            enable_color_x11(pingcolor);
            XDrawString(display, pixbuff, gc, 234+sobump, 150+(i*15), " * ", 3);
            disable_color_x11();
        }

        /* Ready Flag */
        memset(infotext,'\0',sizeof(infotext));
        sprintf(infotext," %c ",readyflag[0]);
        XDrawString(display, pixbuff, gc, 261+sobump, 150+(i*15), "|",1);
        if(myplayers[i].ready)
            enable_color_x11(CE_C_YELLOW);
        XDrawString(display, pixbuff, gc, 270+sobump, 150+(i*15), infotext, strlen(infotext));
        if(myplayers[i].ready)
            disable_color_x11();
        XDrawString(display, pixbuff, gc, 297+sobump, 150+(i*15), "|",1);

        /* Player Name */
        enable_color_x11(myplayers[i].color);
        XDrawString(display, pixbuff, gc, 315+sobump, 150+(i*15), myplayers[i].name,
            strlen(myplayers[i].name));
        disable_color_x11();
    }
}

/*
######################
#..drawhistory_x11().#
######################
*/
void drawhistory_x11 (void)
{
    int i;

    cleanbuffers_x11();

    XStoreName(display, win, "Chaos - Game History");

    /* Draw Border around the screen */
    for(i=0;i<(WIN_WIDTH/9);i++){
        XDrawString(display, pixbuff, gc, i*9, 15, "#", 1);
        XDrawString(display, pixbuff, gc, i*9, 360, "#", 1);
        if(i<(WIN_HEIGHT/15)){
            XDrawString(display, pixbuff, gc, 0, i*15, "#", 1);
            XDrawString(display, pixbuff, gc, 711, i*15, "#", 1);
        }
    }

    XDrawString(display, pixbuff, gc, text_offset(12), 80, "Game History", 12);

    for(i=0;i<history_count;i++){

        enable_color_x11(CE_C_ORANGE);

        XDrawString(display, pixbuff, gc, 81,(i*15)+110, "[",1);
        XDrawString(display, pixbuff, gc, 90,(i*15)+110, myhistory[i].datetime,
            strlen(myhistory[i].datetime));
        XDrawString(display, pixbuff, gc, 162,(i*15)+110, "]",1);

        disable_color_x11();

        XDrawString(display, pixbuff, gc, 180,(i*15)+110, myhistory[i].message,
            strlen(myhistory[i].message));
    }

    flushbuffers_x11();
}

/*
###################
#..drawchat_x11().#
###################
*/
void drawchat_x11 (void)
{
    int i;

    cleanbuffers_x11();

    XStoreName(display, win, "Chaos - Player Chat");

    /* Draw Border around the screen */
    for(i=0;i<(WIN_WIDTH/9);i++){
        XDrawString(display, pixbuff, gc, i*9, 15, "#", 1);
        XDrawString(display, pixbuff, gc, i*9, 360, "#", 1);
        if(i<(WIN_HEIGHT/15)){
            XDrawString(display, pixbuff, gc, 0, i*15, "#", 1);
            XDrawString(display, pixbuff, gc, 711, i*15, "#", 1);
        }
    }

    XDrawString(display, pixbuff, gc, text_offset(11), 80, "Player Chat", 11);

    for(i=0;i<chat_count;i++){

        XDrawString(display, pixbuff, gc, 27,(i*15)+110, "[",1);

        enable_color_x11(CE_C_ORANGE);
        XDrawString(display, pixbuff, gc, 36,(i*15)+110, mychat[i].datetime,
            strlen(mychat[i].datetime));
        disable_color_x11();

        XDrawString(display, pixbuff, gc, 108,(i*15)+110, "]<",2);
        XDrawString(display, pixbuff, gc, 197,(i*15)+110, ">",1);

        disable_color_x11();

        if(mychat[i].player_id == -1){
            enable_color_x11(CE_C_YELLOW);
            XDrawString(display, pixbuff, gc, 126,(i*15)+110, "Server",6);
        } else {
            enable_color_x11(myplayers[mychat[i].player_id].color);
            XDrawString(display, pixbuff, gc, 126,(i*15)+110,
                myplayers[mychat[i].player_id].name,
                strlen(myplayers[mychat[i].player_id].name));
        }

        disable_color_x11();

        XDrawString(display, pixbuff, gc, 216,(i*15)+110, mychat[i].message,
            strlen(mychat[i].message));
    }

    /* Show typed chat buffer */
    enable_color_x11(CE_C_RED);
    XDrawString(display, pixbuff, gc, 18, 345, "Message: ", 9);
    disable_color_x11();
    XDrawString(display, pixbuff, gc, 99, 345, chat_buffer, strlen(chat_buffer));

    flushbuffers_x11();
}

/*
####################
#..drawdebug_x11().#
####################
*/
void drawdebug_x11 (void)
{
    int i;
    char debugtext[64];

    cleanbuffers_x11();

    XStoreName(display, win, "Chaos - Debug View");

    /* Draw Border around the screen */
    for(i=0;i<(WIN_WIDTH/9);i++){
        XDrawString(display, pixbuff, gc, i*9, 15, "#", 1);
        XDrawString(display, pixbuff, gc, i*9, 360, "#", 1);
        if(i<(WIN_HEIGHT/15)){
            XDrawString(display, pixbuff, gc, 0, i*15, "#", 1);
            XDrawString(display, pixbuff, gc, 711, i*15, "#", 1);
        }
    }

    XDrawString(display, pixbuff, gc, text_offset(10), 80, "Debug View", 10);

    XDrawString(display, pixbuff, gc, 90, 125, "World Data", 10);

    enable_color_x11(CE_C_LORANGE);
    XDrawString(display, pixbuff, gc, 90, 140, "[Balance]", 9);
    disable_color_x11();
    memset(debugtext,'\0',sizeof(debugtext));
    sprintf(debugtext,"%d",myworld.balance);
    XDrawString(display, pixbuff, gc, 270, 140, debugtext, strlen(debugtext));

    enable_color_x11(CE_C_LORANGE);
    XDrawString(display, pixbuff, gc, 90, 155, "[Players]", 9);
    disable_color_x11();
    memset(debugtext,'\0',sizeof(debugtext));
    sprintf(debugtext,"%d",myworld.players);
    XDrawString(display, pixbuff, gc, 270, 155, debugtext, strlen(debugtext));

    enable_color_x11(CE_C_LORANGE);
    XDrawString(display, pixbuff, gc, 90, 170, "[Total Spells]", 14);
    disable_color_x11();
    memset(debugtext,'\0',sizeof(debugtext));
    sprintf(debugtext,"%d",myworld.total_spells);
    XDrawString(display, pixbuff, gc, 270, 170, debugtext, strlen(debugtext));

    enable_color_x11(CE_C_LORANGE);
    XDrawString(display, pixbuff, gc, 90, 185, "[Current Player]", 16);
    disable_color_x11();
    memset(debugtext,'\0',sizeof(debugtext));
    sprintf(debugtext,"%d",myworld.current_player);
    XDrawString(display, pixbuff, gc, 270, 185, debugtext, strlen(debugtext));

    enable_color_x11(CE_C_LORANGE);
    XDrawString(display, pixbuff, gc, 90, 200, "[Selected Item]", 15);
    disable_color_x11();
    memset(debugtext,'\0',sizeof(debugtext));
    sprintf(debugtext,"%d (Turns left: %d)",
        myworld.selected_item[0],myworld.selected_item[1]);
    XDrawString(display, pixbuff, gc, 270, 200, debugtext, strlen(debugtext));

    enable_color_x11(CE_C_LORANGE);
    XDrawString(display, pixbuff, gc, 90, 215, "[Mode]", 6);
    disable_color_x11();
    memset(debugtext,'\0',sizeof(debugtext));
    sprintf(debugtext,"%d",myworld.mode);
    XDrawString(display, pixbuff, gc, 270, 215, debugtext, strlen(debugtext));

    enable_color_x11(CE_C_LORANGE);
    XDrawString(display, pixbuff, gc, 90, 230, "[Submode]", 9);
    disable_color_x11();
    memset(debugtext,'\0',sizeof(debugtext));
    sprintf(debugtext,"%d",myworld.submode);
    XDrawString(display, pixbuff, gc, 270, 230, debugtext, strlen(debugtext));

    flushbuffers_x11();
}

/*
#########################
#..drawfindserver_x11().#
#########################
*/
void drawfindserver_x11 (void)
{
    int i,c;

    cleanbuffers_x11();

    switch(myworld.submode){

        /* Connect to IP */
        case 0:
            XStoreName(display, win, "Chaos - Find Server");

            XDrawString(display, pixbuff, gc, text_offset(23), 140, "Connect to Server by IP", 23);
            XDrawString(display, pixbuff, gc, text_offset(strlen(input_buffer)), 180,
                input_buffer, strlen(input_buffer));

            /* Messages about Server */
            if(beepmsg){
                enable_color_x11(CE_C_RED);
                XDrawString(display, pixbuff, gc, text_offset(strlen(infobar_text)), 350,
                    infobar_text, strlen(infobar_text));
                disable_color_x11();
            }
            break;

        /* Enter Name */
        case 1:
            XStoreName(display, win, "Chaos - Player Setup");

            XDrawString(display, pixbuff, gc, text_offset(22), 110, "Enter Your Player Name", 22);
            XDrawString(display, pixbuff, gc, text_offset(strlen(input_buffer)), 150,
                input_buffer, strlen(input_buffer));

            /* Choose Player Colour */
            XDrawString(display, pixbuff, gc, text_offset(32), 210,
                "Tab to choose your player colour", 32);

            c=1;
            for(i=0;i<MAX_PLAYER_COLORS+1;i++){
                enable_color_x11(c);
                XDrawString(display, pixbuff, gc, 245+(c*7), 240, "@", 1);
                if(mycolor==i){
                    disable_color_x11();
                    XDrawString(display, pixbuff, gc, 245+(c*7),255,"^",1);
                }
                disable_color_x11();
                c+=3;
            }

            /* Messages about Server */
            if(beepmsg){
                enable_color_x11(CE_C_RED);
                XDrawString(display, pixbuff, gc, text_offset(strlen(infobar_text)), 350,
                    infobar_text, strlen(infobar_text));
                disable_color_x11();
            }
            break;

        /* Waiting for Players */
        case 3:
            XStoreName(display, win, "Chaos - Waiting for Players");
            XDrawString(display, pixbuff, gc, text_offset(19), 60, "Waiting for Players", 19);

            drawplayerlist_x11(false);

            enable_color_x11(CE_C_LRED);

            if(beepmsg)
                XDrawString(display, pixbuff, gc, text_offset(strlen(infobar_text)), 335,
                    infobar_text, strlen(infobar_text));

            XDrawString(display, pixbuff, gc, text_offset(27), 335,
                "Press Space to toggle Ready", 27);

            disable_color_x11();
            break;
    }

    flushbuffers_x11();
}

/*
############################
#..drawwaitforserver_x11().#
############################
*/
void drawwaitforserver_x11 (void)
{
    cleanbuffers_x11();

    /* Waiting for other players */
    if(myworld.mode == CE_WORLD_MODE_SELECTSPELLS){
        XStoreName(display, win, "Chaos - Waiting for Players");
        XDrawString(display, pixbuff, gc, text_offset(19), 60, "Waiting for Players", 19);

        drawplayerlist_x11(false);

    } else {
        XDrawString(display, pixbuff, gc, text_offset(19), 164, "Waiting for Server", 18);
    }

    if(beepmsg){
        enable_color_x11(CE_C_LRED);
        XDrawString(display, pixbuff, gc, text_offset(strlen(infobar_text)), 335,
                    infobar_text, strlen(infobar_text));
        disable_color_x11();
    }

    flushbuffers_x11();
}

/*
####################
#..drawscene_x11().#
####################
*/
void drawscene_x11 (void)
{
    int i,j,u,p;
    int cursor_x, cursor_y, top_layer,icon_offset;
    int offset_x = (((MAX_X/2)*9) - (arenas[myworld.arenasize][0]/2)*9)+126;
    int offset_y = (((MAX_Y/2)*15) - (arenas[myworld.arenasize][1]/2)*15)+30;
    int itemid, spellid;
    int casting_prob;
    int itemcolor = -1;
    int pingcolor = -1;
    int pid=myworld.current_player-1;

    char item[2];
    char infotext[64];
    char letter[28] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ\0";
    char balance_text[8];
    char balance_rating[20];

    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
        "offset_x : %d / offset_y : %d",offset_x,offset_y));

    memset(item,'\0',sizeof(item));
    memset(infotext,'\0',sizeof(infotext));

    #if WITH_NET
    if(net_enable)
        pid = mypid;
    #endif

    /*
    ####################
    #.Update.Title.Bar.#
    ####################
    */
    switch(myworld.mode){
        case 0:
            XStoreName(display, win, "Chaos - Setup");
            break;
        case 1:
            XStoreName(display, win, "Chaos - Spell Selection");
            break;
        case 2:
            XStoreName(display, win, "Chaos - Spell Casting");
            break;
        case 3:
            XStoreName(display, win, "Chaos - Movement Turn");
            break;
        case 5:
            XStoreName(display, win, "Chaos - Game Over");
            break;
        default:
            XStoreName(display, win, "Chaos");
            break;
    }

    /*
    ####################
    #..Prepare.Buffers.#
    ####################
    */
    cleanbuffers_x11();

    /*
    ##################
    #..Setup.Screens.#
    ##################
    */
    if(myworld.mode == CE_WORLD_MODE_SETUP) {

        /* Draw Border around the screen */
        for(i=0;i<(WIN_WIDTH/9);i++){
            XDrawString(display, pixbuff, gc, i*9, 15, "#", 1);
            XDrawString(display, pixbuff, gc, i*9, 360, "#", 1);
            if(i<(WIN_HEIGHT/15)){
                XDrawString(display, pixbuff, gc, 0, i*15, "#", 1);
                XDrawString(display, pixbuff, gc, 711, i*15, "#", 1);
            }
        }

        /* Draw setup screen title */
        switch(myworld.submode){
            case 0: /* Number of players */
                XDrawString(display, pixbuff, gc, 280, 140, "Number of Players?", 18);
                break;
            case 1: /* Player names */
                sprintf(infotext,"Player %d Name",myworld.current_player);
                XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 140,
                    infotext, strlen(infotext));
                break;
            case 2: /* Arena size */
                XDrawString(display, pixbuff, gc, 280, 140, "Arena Size?", 11);
                sprintf(infotext,"1 = [%dx%d] | 2 = [%dx%d] | 3 = [%dx%d]",
                    arenas[1][0],arenas[1][1],
                    arenas[2][0],arenas[2][1],
                    arenas[3][0],arenas[3][1]
                );
                XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 155,
                    infotext, strlen(infotext));
                memset(infotext,'\0',sizeof(infotext));
                break;
        }

        /* Draw buffer contents so user can see what they are typing */
        XDrawString(display, pixbuff, gc, text_offset(strlen(input_buffer)), 180,
            input_buffer, strlen(input_buffer));

    /*
    ####################
    #..Spell.Selection.#
    ####################
    */
    }else if((myworld.mode == CE_WORLD_MODE_SELECTSPELLS ||
        (myworld.mode == CE_WORLD_MODE_MOVE && myworld.submode == 6))
        && myworld.submode != 5 && myworld.arenasize > 0){ /* Spell selection stage */

        for(i=0;i<(WIN_WIDTH/9);i++){
            XDrawString(display, pixbuff, gc, i*9, 15, "#", 1);
            XDrawString(display, pixbuff, gc, i*9, 360, "#", 1);
            if(i<(WIN_HEIGHT/15)){
                XDrawString(display, pixbuff, gc, 0, i*15, "#", 1);
                XDrawString(display, pixbuff, gc, 711, i*15, "#", 1);
            }
        }

        /*
        #################################
        #..Draw.Player.Name.and.Balance.#
        #################################
        */
        memset(infotext,'\0',sizeof(infotext));
        memset(balance_text,'\0',sizeof(balance_text));
        memset(balance_rating,'\0',sizeof(balance_rating));

        if(myworld.balance > 0){
            sprintf(balance_text,"%s","Law");
            memset(balance_rating,'*',myworld.balance);
            itemcolor = CE_C_DBLUE;
        } else if (myworld.balance < 0) {
            sprintf(balance_text,"%s","Chaos");
            memset(balance_rating,'*',
                myworld.balance - (myworld.balance + myworld.balance));
            itemcolor = CE_C_RED;
        } else {
            sprintf(balance_text,"%s","Neutral");
            itemcolor = -1;
        }

        if(itemcolor >= 0)
            enable_color_x11(itemcolor);

        sprintf(infotext,"%s - %s %s",myplayers[pid].name,
            balance_text,balance_rating);
        XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 80, infotext,
            strlen(infotext));

        if(itemcolor >= 0)
            disable_color_x11();

        /*
        ########################
        #..List.players.spells.#
        ########################
        */
        if(myworld.submode == 1 || myworld.submode == 3){

            console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
                "Listing players spells [total : %d]",
                myplayers[pid].total_spells)
            );

            for(i=1;i<myplayers[pid].total_spells+1;i++){

                itemid = myspells[myplayers[pid].spells[i]].id;
                spellid = myplayers[pid].spells[i];
                /* Default White Text */
                disable_color_x11();
                /*
                ########################
                #..List.special.spells.#
                ########################
                */
                if(myspells[spellid].spell_type == SPELL_MAGIC_SPECIAL){
                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"%c - %s",letter[i-1],
                        mymagic_special[itemid].name);
                    XDrawString(display, pixbuff, gc, 250, 100+((i)*15), infotext,strlen(infotext));
                /*
                ########################
                #..List.monster.spells.#
                ########################
                */
                } else {
                    if (!myspells[spellid].beencast){

                         /* clear to white */
                        itemcolor = -1;
                        disable_color_x11();

                        switch(myspells[spellid].spell_type){
                            case SPELL_BLOB:
                                memset(infotext,'\0',sizeof(infotext));
                                sprintf(infotext,"%c - %s",letter[i-1],myblobs[itemid].name);
                                if(myblobs[itemid].balance > 0){
                                    itemcolor = CE_C_BLUE;
                                } else if (myblobs[itemid].balance < 0){
                                    itemcolor = CE_C_RED;
                                }
                                break;

                            case SPELL_WALL:
                                memset(infotext,'\0',sizeof(infotext));
                                sprintf(infotext,"%c - %s",letter[i-1],mywalls[itemid].name);
                                if(mywalls[itemid].balance > 0){
                                    itemcolor = CE_C_BLUE;
                                } else if (mywalls[itemid].balance < 0){
                                    itemcolor = CE_C_RED;
                                }
                                break;

                            case SPELL_MAGIC_BALANCE:
                                memset(infotext,'\0',sizeof(infotext));
                                sprintf(infotext,"%c - %s",letter[i-1],mymagic_balance[itemid].name);
                                if(mymagic_balance[itemid].balance > 0){
                                    itemcolor = CE_C_BLUE;
                                } else if (mymagic_balance[itemid].balance < 0){
                                    itemcolor = CE_C_RED;
                                }
                                break;

                            case SPELL_MAGIC_ATTRIB:
                                memset(infotext,'\0',sizeof(infotext));
                                sprintf(infotext,"%c - %s",letter[i-1],mymagic_spell_attrib[itemid].name);
                                if(mymagic_spell_attrib[itemid].balance > 0){
                                    itemcolor = CE_C_BLUE;
                                } else if (mymagic_spell_attrib[itemid].balance < 0){
                                    itemcolor = CE_C_RED;
                                }
                                break;

                            case SPELL_MAGIC_UPGRADE:
                                memset(infotext,'\0',sizeof(infotext));
                                sprintf(infotext,"%c - %s",letter[i-1],mymagic_upgrade[itemid].name);
                                if(mymagic_upgrade[itemid].balance > 0){
                                    itemcolor = CE_C_BLUE;
                                } else if (mymagic_upgrade[itemid].balance < 0){
                                    itemcolor = CE_C_RED;
                                }
                                break;

                            case SPELL_TREE:
                                memset(infotext,'\0',sizeof(infotext));
                                sprintf(infotext,"%c - %s",letter[i-1],mytrees[itemid].name);
                                if(mytrees[itemid].balance > 0){
                                    itemcolor = CE_C_BLUE;
                                } else if (mytrees[itemid].balance < 0){
                                    itemcolor = CE_C_RED;
                                }
                                break;

                            case SPELL_MAGIC_RANGED:
                                memset(infotext,'\0',sizeof(infotext));
                                sprintf(infotext,"%c - %s",letter[i-1],mymagic_ranged[itemid].name);
                                if(mymagic_ranged[itemid].balance > 0){
                                    itemcolor = CE_C_BLUE;
                                } else if (mymagic_ranged[itemid].balance < 0){
                                    itemcolor = CE_C_RED;
                                }
                                break;

                            case SPELL_MONSTER:
                            default:
                                memset(infotext,'\0',sizeof(infotext));
                                sprintf(infotext,"%c - %s",letter[i-1],mymonsters[itemid].name);
                                if(mymonsters[itemid].balance > 0){
                                    itemcolor = CE_C_BLUE;
                                } else if (mymonsters[itemid].balance < 0){
                                    itemcolor = CE_C_RED;
                                }
                                break;
                        }

                        if(itemcolor >= 0)
                            enable_color_x11(itemcolor);

                        /* Draw that badboy... */
                        XDrawString(display, pixbuff, gc, 250, 100+((i)*15), infotext,strlen(infotext));

                        if(itemcolor >= 0)
                            disable_color_x11();
                    }
                }
            }

        /*
        ######################
        #..Spell.Info.Screen.#
        ######################
        */
        } else if(myworld.submode == 2 || myworld.submode == 6){

            if(myworld.submode == 2) {
                itemid = myplayers[pid].spells[myplayers[pid].selected_spell];
                spellid = myspells[itemid].id;
            } else {    /* Submode 6 ( view arena -> view item) */
                itemid = myworld.layout[myworld.cursor[0]][myworld.cursor[1]][itemstack_top(myworld.cursor[0],myworld.cursor[1])];
                spellid = myspells[itemid].id;
            }

            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Displaying item info for item %d [spell %d]",itemid,spellid));

            switch(myspells[itemid].spell_type) {

                case SPELL_PLAYER:
                    sprintf(infotext,"Player Info for %s",myplayers[spellid].name);
                    XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 100,
                        infotext,strlen(infotext));

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Attack     %d",myplayers[spellid].attack);
                    XDrawString(display, pixbuff, gc, 225, 130, infotext,strlen(infotext));

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Defense    %d",myplayers[spellid].defense);
                    XDrawString(display, pixbuff, gc, 225, 145, infotext,strlen(infotext));

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Ranged Range    %d",myplayers[spellid].ranged_range);
                    XDrawString(display, pixbuff, gc, 369, 130, infotext,strlen(infotext));

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Ranged Damage   %d",myplayers[spellid].ranged_damage);
                    XDrawString(display, pixbuff, gc, 369, 145, infotext,strlen(infotext));

                    if(myplayers[spellid].flight){
                        enable_color_x11(CE_C_LBLUE);
                        memset(infotext,'\0',sizeof(infotext));
                        sprintf(infotext,"Flight");
                        XDrawString(display, pixbuff, gc, 210, 170, infotext,strlen(infotext));
                        disable_color_x11();
                    }
                    break;

                case SPELL_MONSTER:
                    sprintf(infotext,"Monster Info for %s",mymonsters[spellid].name);
                    XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 100,
                        infotext,strlen(infotext));

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Attack     %d",mymonsters[spellid].attack);
                    XDrawString(display, pixbuff, gc, 225, 130, infotext,strlen(infotext));

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Defense    %d",mymonsters[spellid].defense);
                    XDrawString(display, pixbuff, gc, 225, 145, infotext,strlen(infotext));

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Ranged Range    %d",mymonsters[spellid].ranged_range);
                    XDrawString(display, pixbuff, gc, 369, 130, infotext,strlen(infotext));

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Ranged Damage   %d",mymonsters[spellid].ranged_damage);
                    XDrawString(display, pixbuff, gc, 369, 145, infotext,strlen(infotext));

                    if(mymonsters[spellid].flight){
                        enable_color_x11(CE_C_LBLUE);
                        memset(infotext,'\0',sizeof(infotext));
                        sprintf(infotext,"Flight");
                        XDrawString(display, pixbuff, gc, 225, 170, infotext,strlen(infotext));
                        disable_color_x11();
                    }

                    if(myspells[itemid].undead){
                        enable_color_x11(CE_C_DRED);
                        memset(infotext,'\0',sizeof(infotext));
                        sprintf(infotext,"Undead");
                        XDrawString(display, pixbuff, gc, 333, 170, infotext,strlen(infotext));
                        disable_color_x11();
                    }

                    if(mymonsters[spellid].mount){
                        enable_color_x11(CE_C_DGREEN);
                        memset(infotext,'\0',sizeof(infotext));
                        sprintf(infotext,"Mount");
                        XDrawString(display, pixbuff, gc, 441, 170, infotext,strlen(infotext));
                        disable_color_x11();
                    }

                    XSetForeground(display, gc, WhitePixel(display,visual_info.screen));

                    memset(infotext,'\0',sizeof(infotext));
                    if(mymonsters[spellid].balance > 0){
                        sprintf(infotext,"Balance Law-%d",mymonsters[spellid].balance);
                        enable_color_x11(CE_C_DBLUE);
                    } else if (mymonsters[spellid].balance < 0) {
                        sprintf(infotext,"Balance Chaos-%d",mymonsters[spellid].balance -
                            (mymonsters[spellid].balance + mymonsters[spellid].balance));
                        enable_color_x11(CE_C_RED);
                    } else {
                        sprintf(infotext,"Balance Neutral");
                    }

                    XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 190,
                        infotext,strlen(infotext));

                    disable_color_x11();

                    if(myworld.submode == 6) {

                        memset(infotext,'\0',sizeof(infotext));
                        sprintf(infotext,"Belongs to : %s",
                            myplayers[myspells[itemid].player_id].name);
                        XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)),
                            220, infotext,strlen(infotext));

                    } else {

                        /* Law spell / Law world */
                        if(myworld.balance > 0 && mymonsters[spellid].balance > 0){
                            casting_prob = (mymonsters[spellid].casting_prob + myworld.balance)+1;

                        /* Chaos spell / Chaos world */
                        } else if (myworld.balance < 0 && mymonsters[spellid].balance < 0){
                            casting_prob = (mymonsters[spellid].casting_prob +
                                (myworld.balance - (myworld.balance + myworld.balance)))+1;

                        } else {    /* Neutral World */
                            casting_prob = mymonsters[spellid].casting_prob+1;
                        }

                        memset(infotext,'\0',sizeof(infotext));
                        sprintf(infotext,"Casting Probability %d0%%",casting_prob);
                        XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 205,
                            infotext,strlen(infotext));
                    }
                    break;

                case SPELL_MAGIC_RANGED:
                    sprintf(infotext,"Spell Info for %s",mymagic_ranged[spellid].name);
                    XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 100,
                        infotext,strlen(infotext));

                    /* Description is too long for the screen,
                        find a nice place to split it */
                    if(strlen(mymagic_ranged[spellid].description) > 70){

                        j = 50;

                        /* Find a nice place to split the string at,
                            find a space after 50 chars */
                        for(i=50;i<strlen(mymagic_ranged[spellid].description);i++){
                            if(strncmp(&mymagic_ranged[spellid].description[i]," ",1) == 0){
                                j = i;
                                i = strlen(mymagic_ranged[spellid].description);
                            }
                        }

                        /* Display part 1 */
                        memset(infotext,'\0',sizeof(infotext));
                        strncpy(infotext,mymagic_ranged[spellid].description,j);
                        XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 130,
                            infotext,strlen(infotext));

                        /* Display the rest of the string */
                        memset(infotext,'\0',sizeof(infotext));
                        strcpy(infotext,&mymagic_ranged[spellid].description[j]);
                        XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 145,
                            infotext,strlen(infotext));

                    } else {

                        memset(infotext,'\0',sizeof(infotext));
                        sprintf(infotext,"%s",mymagic_ranged[spellid].description);
                        XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 130,
                            infotext,strlen(infotext));
                    }

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Casting Probability %d0%%",
                        mymagic_ranged[spellid].casting_prob);
                    XDrawString(display, pixbuff, gc, 256, 190, infotext,strlen(infotext));
                    break;

                case SPELL_TREE:
                    sprintf(infotext,"Spell Info for %s",mytrees[spellid].name);
                    XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 100,
                        infotext,strlen(infotext));

                    /* Description is too long for the screen,
                        find a nice place to split it */
                    if(strlen(mytrees[spellid].description) > 70){

                        j = 50;

                        /* Find a nice place to split the string at,
                            find a space after 50 chars */
                        for(i=50;i<strlen(mytrees[spellid].description);i++){
                            if(strncmp(&mytrees[spellid].description[i]," ",1) == 0){
                                j = i;
                                i = strlen(mytrees[spellid].description);
                            }
                        }

                        /* Display part 1 */
                        memset(infotext,'\0',sizeof(infotext));
                        strncpy(infotext,mytrees[spellid].description,j);
                        XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 130,
                            infotext,strlen(infotext));

                        /* Display the rest of the string */
                        memset(infotext,'\0',sizeof(infotext));
                        strcpy(infotext,&mytrees[spellid].description[j]);
                        XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 145,
                            infotext,strlen(infotext));

                    } else {

                        memset(infotext,'\0',sizeof(infotext));
                        sprintf(infotext,"%s",mytrees[spellid].description);
                        XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 130,
                            infotext,strlen(infotext));
                    }

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Casting Probability %d0%%",
                        mytrees[spellid].casting_prob);
                    XDrawString(display, pixbuff, gc, 256, 190, infotext,strlen(infotext));
                    break;

                case SPELL_MAGIC_SPECIAL:
                    sprintf(infotext,"Spell Info for %s",mymagic_special[spellid].name);
                    XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 100,
                        infotext,strlen(infotext));

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"%s",mymagic_special[spellid].description);
                    XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 130,
                        infotext,strlen(infotext));

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Casting Probability %d0%%",
                        mymagic_special[spellid].casting_prob);
                    XDrawString(display, pixbuff, gc, 256, 190, infotext,strlen(infotext));
                    break;

                case SPELL_MAGIC_UPGRADE:
                    sprintf(infotext,"Spell Info for %s",mymagic_upgrade[spellid].name);
                    XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 100,
                        infotext,strlen(infotext));

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"%s",mymagic_upgrade[spellid].description);
                    XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 130,
                        infotext,strlen(infotext));

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Casting Probability %d0%%",
                        mymagic_upgrade[spellid].casting_prob);
                    XDrawString(display, pixbuff, gc, 256, 190, infotext,strlen(infotext));
                    break;

                case SPELL_MAGIC_ATTRIB:
                    sprintf(infotext,"Spell Info for %s",
                        mymagic_spell_attrib[spellid].name);
                    XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 100,
                        infotext,strlen(infotext));

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"%s",mymagic_spell_attrib[spellid].description);
                    XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 130,
                        infotext,strlen(infotext));

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Casting Probability %d0%%",
                        mymagic_spell_attrib[spellid].casting_prob);
                    XDrawString(display, pixbuff, gc, 256, 190, infotext,strlen(infotext));
                    break;

                case SPELL_MAGIC_BALANCE:
                    sprintf(infotext,"Spell Info for %s",mymagic_balance[spellid].name);
                    XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 100,
                        infotext,strlen(infotext));

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"%s",mymagic_balance[spellid].description);
                    XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 130,
                        infotext,strlen(infotext));

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Casting Probability %d0%%",
                        mymagic_balance[spellid].casting_prob);
                    XDrawString(display, pixbuff, gc, 256, 190, infotext,strlen(infotext));
                    break;

                case SPELL_WALL:
                    sprintf(infotext,"Spell Info for %s",mywalls[spellid].name);
                    XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 100,
                        infotext,strlen(infotext));

                    /* Description is too long for the screen,
                        find a nice place to split it */
                    if(strlen(mywalls[spellid].description) > 70){

                        j = 50;

                        /* Find a nice place to split the string at,
                            find a space after 50 chars */
                        for(i=50;i<strlen(mywalls[spellid].description);i++){
                            if(strncmp(&mywalls[spellid].description[i]," ",1) == 0){
                                j = i;
                                i = strlen(mywalls[spellid].description);
                            }
                        }

                        /* Display part 1 */
                        memset(infotext,'\0',sizeof(infotext));
                        strncpy(infotext,mywalls[spellid].description,j);
                        XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 130,
                            infotext,strlen(infotext));

                        /* Display the rest of the string */
                        memset(infotext,'\0',sizeof(infotext));
                        strcpy(infotext,&mywalls[spellid].description[j]);
                        XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 145,
                            infotext,strlen(infotext));

                    } else {

                        memset(infotext,'\0',sizeof(infotext));
                        sprintf(infotext,"%s",mywalls[spellid].description);
                        XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 130,
                            infotext,strlen(infotext));
                    }

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Casting Probability %d0%%",
                        mywalls[spellid].casting_prob);
                    XDrawString(display, pixbuff, gc, 256, 190, infotext,strlen(infotext));
                    break;

                case SPELL_BLOB:
                    sprintf(infotext,"Spell Info for %s",myblobs[spellid].name);
                    XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 100,
                        infotext,strlen(infotext));

                    /* Description is too long for the screen,
                        find a nice place to split it */
                    if(strlen(myblobs[spellid].description) > 70){

                        j = 50;

                        /* Find a nice place to split the string at,
                            find a space after 50 chars */
                        for(i=50;i<strlen(myblobs[spellid].description);i++){
                            if(strncmp(&myblobs[spellid].description[i]," ",1) == 0){
                                j = i;
                                i = strlen(myblobs[spellid].description);
                            }
                        }

                        /* Display part 1 */
                        memset(infotext,'\0',sizeof(infotext));
                        strncpy(infotext,myblobs[spellid].description,j);
                        XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 130,
                            infotext,strlen(infotext));

                        /* Display the rest of the string */
                        memset(infotext,'\0',sizeof(infotext));
                        strcpy(infotext,&myblobs[spellid].description[j]);
                        XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 145,
                            infotext,strlen(infotext));

                    } else {

                        memset(infotext,'\0',sizeof(infotext));
                        sprintf(infotext,"%s",myblobs[spellid].description);
                        XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 130,
                            infotext,strlen(infotext));
                    }

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Casting Probability %d0%%",
                        myblobs[spellid].casting_prob);
                    XDrawString(display, pixbuff, gc, 256, 190, infotext,strlen(infotext));
                    break;

                /* Unknown Item */
                default:
                    XDrawString(display, pixbuff, gc, 220, 240, "Unknown Spell",12);
                    console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(
                        log_message,"Couldnt get info for Spell id : %d [type %d]",
                        myspells[itemid].id,
                        myspells[itemid].spell_type)
                    );
                    break;
            }

            XDrawString(display, pixbuff, gc, 239, 240, "[Press any key to continue]",27);

        /*
        #################
        #..Is.Illusion?.#
        #################
        */
        } else if(myworld.submode == 4){
            XDrawString(display, pixbuff, gc, 256, 135, "Cast as illusion? (y/n)",23);
        /*
        ##############
        #..Main.Menu.#
        ##############
        */
        } else {
            XDrawString(display, pixbuff, gc, 256, 120, "1. View Spells",14);
            XDrawString(display, pixbuff, gc, 256, 135, "2. Select Spells",16);
            XDrawString(display, pixbuff, gc, 256, 150, "3. View Arena",13);
            XDrawString(display, pixbuff, gc, 256, 165, "0. End Turn",11);

            /* Messages from Server? */
            if(beepmsg){
                enable_color_x11(CE_C_LRED);
                XDrawString(display, pixbuff, gc, text_offset(strlen(infobar_text)), 335,
                    infobar_text, strlen(infobar_text));
                disable_color_x11();
            }
        }

    /*
    ####################
    #..Game.End.Screen.#
    ####################
    */
    } else if (myworld.mode == CE_WORLD_MODE_ENDGAME) {
        sprintf(infotext,"Game over %s Wins!",myplayers[myworld.current_player].name);
        XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 180,
            infotext, strlen(infotext));
    /*
    #################
    #..Arena.Screen.#
    #################
    */
    } else {
        /* Draw player list */
        for(i=0;i<myworld.players;i++){
            /*
            #############################
            #..Highlight.Current.Player.#
            #############################
            */
            if(i == myworld.current_player -1 && myworld.mode != CE_WORLD_MODE_SELECTSPELLS){

                itemcolor = CE_C_YELLOW;

            /*
            #########################
            #..Grey.Out.Dead.Player.#
            #########################
            */
            } else if (myspells[i+10].dead) {

                itemcolor = CE_C_GREY;

            /*
            ####################
            #..Inactive.Player.#
            ####################
            */
            } else {
                itemcolor = -1;
            }

            if(itemcolor >= 0)
                enable_color_x11(itemcolor);

            XDrawString(display, pixbuff, gc, 0, 15+(i*30), myplayers[i].name,
                strlen(myplayers[i].name));

            XDrawString(display, pixbuff, gc, 0, 30+(i*30), "[",1);

            if(itemcolor >= 0)
                disable_color_x11();

            /*
            #############
            #..Ping.Box.#
            #############
            */

            memset(infotext,'\0',sizeof(infotext));
            for(p=0;p<myplayers[i].ping+1;p++)
                sprintf(infotext,"%s*",infotext);

            switch(strlen(infotext)){
                case 4:
                    pingcolor = CE_C_GREEN;
                    break;
                case 3:
                    pingcolor = CE_C_ORANGE;
                    break;
                case 2:
                    pingcolor = CE_C_YELLOW;
                    break;
                case 1:
                    pingcolor = CE_C_RED;
                    break;
                case 0:
                default:
                    pingcolor = -1;
                    break;
            }

            if(pingcolor >= 0)
                enable_color_x11(pingcolor);

            XDrawString(display, pixbuff, gc, 9, 30+(i*30), infotext,strlen(infotext));

            if(pingcolor >= 0)
                disable_color_x11();

            if(itemcolor >= 0)
                enable_color_x11(itemcolor);

            XDrawString(display, pixbuff, gc, 45, 30+(i*30), "][",2);

            XDrawString(display, pixbuff, gc, 108, 30+(i*30), "]", 1);

            if(itemcolor >= 0)
                disable_color_x11();

            /*
            ############################
            #..Upgrade.Icons.(first 5).#
            ############################
            */
            icon_offset = 0;
            for(u=0;u<5;u++){
                if(myplayers[i].upgrades[u] > 0){
                    enable_color_x11(mymagic_upgrade[myplayers[i].upgrades[u]].color);
                    strcpy(infotext,&mymagic_upgrade[myplayers[i].upgrades[u]].disp[0]);
                    XDrawString(display, pixbuff, gc, 63+(icon_offset*9), 30+(i*30), infotext, 1);
                    icon_offset++;
                }
            }

            disable_color_x11();
        }

        for(i=0;i<MAX_X+2;i++){
            for(j=0;j<MAX_Y+2;j++){
                if(
                    (i<((MAX_X/2) - (arenas[myworld.arenasize][0]/2)+1) ||
                        i>((MAX_X/2) + (arenas[myworld.arenasize][0]/2)))
                    ||
                    (j<((MAX_Y/2) - (arenas[myworld.arenasize][1]/2)+1) ||
                        j>((MAX_Y/2) + (arenas[myworld.arenasize][1]/2)))
                ){
                    XDrawString(display, pixbuff, gc, 117+(i*9), 15+(j*15), "#", 1);
                }
            }
        }

        /*
        ######################
        #..Draw.Battleground.#
        ######################
        */
        for(i=0;i<arenas[myworld.arenasize][0];i++){
            for(j=0;j<arenas[myworld.arenasize][1];j++){
                top_layer = itemstack_top(i,j);
                if(myworld.layout[i][j][top_layer] > 0){

                    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                        log_message,"Found : %d @ %d %d (type = %d)",
                        myworld.layout[i][j][top_layer],i,j,
                        myspells[myworld.layout[i][j][top_layer]].spell_type)
                    );

                    itemcolor = -1;
                    disable_color_x11();

                    switch(myspells[myworld.layout[i][j][top_layer]].spell_type){

                        case SPELL_PLAYER:
                            itemcolor = myplayers[myspells[myworld.layout[i][j][top_layer]].id].color;
                            if(myspells[myworld.layout[i][j][top_layer]].dead){
                                itemcolor = -1;
                                strcpy(item,".");
                            } else {
                                strcpy(item,&myplayers[myspells[myworld.layout[i][j][top_layer]].id].disp[0]);
                            }
                            break;

                        case SPELL_MONSTER:
                            itemcolor = mymonsters[myspells[myworld.layout[i][j][top_layer]].id].color;

                            if(myspells[myworld.layout[i][j][top_layer]].dead &&
                                !myspells[myworld.layout[i][j][top_layer]].illusion){

                                strcpy(item,"~");

                            } else if(myspells[myworld.layout[i][j][top_layer]].dead &&
                                myspells[myworld.layout[i][j][top_layer]].illusion){

                                itemcolor = -1;
                                strcpy(item,".");

                            } else {
                                strcpy(item,
                                    &mymonsters[myspells[myworld.layout[i][j][top_layer]].id].disp[0]);
                            }
                            break;

                        case SPELL_TREE:
                            itemcolor = mytrees[myspells[myworld.layout[i][j][top_layer]].id].color;

                            if(myspells[myworld.layout[i][j][top_layer]].dead){
                                itemcolor = -1;
                                strcpy(item,".");
                            } else {
                                strcpy(item,
                                    &mytrees[myspells[myworld.layout[i][j][top_layer]].id].disp[0]);
                            }
                            break;

                        case SPELL_WALL:
                            itemcolor = mywalls[myspells[myworld.layout[i][j][top_layer]].id].color;

                            if(myspells[myworld.layout[i][j][top_layer]].dead){
                                itemcolor = -1;
                                strcpy(item,".");
                            } else {
                                strcpy(item,
                                    &mywalls[myspells[myworld.layout[i][j][top_layer]].id].disp[0]);
                            }
                            break;

                        case SPELL_BLOB:
                            itemcolor = myblobs[myspells[myworld.layout[i][j][top_layer]].id].color;

                            if(myspells[myworld.layout[i][j][top_layer]].dead){
                                itemcolor = -1;
                                strcpy(item,".");
                            } else {
                                strcpy(item,
                                    &myblobs[myspells[myworld.layout[i][j][top_layer]].id].disp[0]);
                            }
                            break;

                        /* Floor? */
                        default:
                            disable_color_x11();
                            break;
                    }

                    if(itemcolor >= 0)
                        enable_color_x11(itemcolor);

                } else {
                    disable_color_x11();
                    strcpy(item,".");
                }
                XDrawString(display, pixbuff, gc, offset_x + (i*9), offset_y + (j*15), item, 1);
            }
        }

        /*
        ################
        #..Draw.Cursor.#
        ################
        */
        XSetForeground(display, gc, WhitePixel(display,visual_info.screen));

        if(myworld.cursor[0] > arenas[myworld.arenasize][0]-1) {
            myworld.cursor[0] = arenas[myworld.arenasize][0]-1;
        } else if (myworld.cursor[0] < 0) {
            myworld.cursor[0] = 0;
        }

        if(myworld.cursor[1] > arenas[myworld.arenasize][1]-1) {
            myworld.cursor[1] = arenas[myworld.arenasize][1]-1;
        } else if (myworld.cursor[1] < 0) {
            myworld.cursor[1] = 0;
        }

        cursor_x = (myworld.cursor[0] * 9) + offset_x;
        cursor_y = (myworld.cursor[1] * 15) + (offset_y - 15);

        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
            "cursor at %d,%d",cursor_x,cursor_y));

        /* Engaged to fight but not ranged attacking */
        if(myworld.selected_item[0] > 0 && checkadjacent() &&
            myworld.submode != 1){

            enable_color_x11(CE_C_RED);
            XDrawRectangle(display, pixbuff, gc, cursor_x, cursor_y, 9, 15);
            disable_color_x11();

        } else {

            if(myworld.selected_item[0] > 0 &&
                (myworld.submode == 0 || myworld.submode == 5) &&
                can_fly(myspells[myworld.selected_item[0]].current_pos[0],
                        myspells[myworld.selected_item[0]].current_pos[1])) {

                XDrawLine(display, pixbuff, gc, cursor_x, cursor_y,
                    cursor_x + 9, cursor_y + 15);
                XDrawLine(display, pixbuff, gc, cursor_x + 9, cursor_y,
                    cursor_x, cursor_y + 15);

            } else if (myworld.selected_item[0] > 0 && myworld.submode == 1) {

                enable_color_x11(CE_C_RED);

                XDrawLine(display, pixbuff, gc, cursor_x, cursor_y,
                    cursor_x + 9, cursor_y + 15);
                XDrawLine(display, pixbuff, gc, cursor_x + 9, cursor_y,
                    cursor_x, cursor_y + 15);

                disable_color_x11();
            } else {

                XDrawRectangle(display, pixbuff, gc, cursor_x, cursor_y, 9, 15);
            }
        }

        /*
        ##################
        #..Draw.Info.Bar.#
        ##################
        */
        if(!beepmsg)
            update_infobar();

        XDrawString(display, pixbuff, gc, 117, 350, infobar_text, strlen(infobar_text));
    }

    /* Push buffers to screen */
    flushbuffers_x11();
}

/*
#####################
#..drawserver_x11().#
#####################
*/
void drawserver_x11 (void)
{
    int i,j,u,p;
    int cursor_x, cursor_y, top_layer,icon_offset;
    int offset_x = (((MAX_X/2)*9) - (arenas[myworld.arenasize][0]/2)*9)+126;
    int offset_y = (((MAX_Y/2)*15) - (arenas[myworld.arenasize][1]/2)*15)+30;
    int itemcolor = -1;
    int pingcolor = -1;

    char item[2];
    char infotext[64];

    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
        "offset_x : %d / offset_y : %d",offset_x,offset_y));

    memset(item,'\0',sizeof(item));
    memset(infotext,'\0',sizeof(infotext));

    /*
    ####################
    #.Update.Title.Bar.#
    ####################
    */
    switch(myworld.mode){
        case 0:
            XStoreName(display, win, "Chaos - Setup");
            break;
        case 1:
            XStoreName(display, win, "Chaos - Spell Selection");
            break;
        case 2:
            XStoreName(display, win, "Chaos - Spell Casting");
            break;
        case 3:
            XStoreName(display, win, "Chaos - Movement Turn");
            break;
        case 5:
            XStoreName(display, win, "Chaos - Game Over");
            break;
        default:
            XStoreName(display, win, "Chaos");
            break;
    }

    /*
    ####################
    #..Prepare.Buffers.#
    ####################
    */
    cleanbuffers_x11();

    /*
    ##################
    #..Setup.Screens.#
    ##################
    */
    if(myworld.mode == CE_WORLD_MODE_SETUP) {

        /* Draw Border around the screen */
        for(i=0;i<(WIN_WIDTH/9);i++){
            XDrawString(display, pixbuff, gc, i*9, 15, "#", 1);
            XDrawString(display, pixbuff, gc, i*9, 360, "#", 1);
            if(i<(WIN_HEIGHT/15)){
                XDrawString(display, pixbuff, gc, 0, i*15, "#", 1);
                XDrawString(display, pixbuff, gc, 711, i*15, "#", 1);
            }
        }

        /* Draw setup screen title */
        switch(myworld.submode){
            case 0: /* Number of players */
                XDrawString(display, pixbuff, gc, 280, 140, "Number of Players?", 18);
                break;
            case 1: /* Player names */
            case 3: /* Waiting for players */
                XStoreName(display, win, "Chaos - Waiting for Players");
                XDrawString(display, pixbuff, gc, text_offset(19), 60, "Waiting for Players", 19);

                drawplayerlist_x11(true);

                break;
            case 2: /* Arena size */
                XDrawString(display, pixbuff, gc, 280, 140, "Arena Size?", 11);
                sprintf(infotext,"1 = [%dx%d] | 2 = [%dx%d] | 3 = [%dx%d]",
                    arenas[1][0],arenas[1][1],
                    arenas[2][0],arenas[2][1],
                    arenas[3][0],arenas[3][1]
                );
                XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 155,
                    infotext, strlen(infotext));
                memset(infotext,'\0',sizeof(infotext));
                break;
        }

        if(myworld.submode != 1){
            /* Draw buffer contents so user can see what they are typing */
            XDrawString(display, pixbuff, gc, text_offset(strlen(input_buffer)), 180,
                input_buffer, strlen(input_buffer));
        }

    /*
    ####################
    #..Spell.Selection.#
    ####################
    */
    }else if(myworld.mode == CE_WORLD_MODE_SELECTSPELLS){
        XStoreName(display, win, "Chaos - Waiting for Players");
        XDrawString(display, pixbuff, gc, text_offset(19), 60, "Waiting for Players", 19);

        drawplayerlist_x11(true);

    /*
    ####################
    #..Game.End.Screen.#
    ####################
    */
    } else if (myworld.mode == CE_WORLD_MODE_ENDGAME) {
        sprintf(infotext,"Game over %s Wins!",myplayers[myworld.current_player].name);
        XDrawString(display, pixbuff, gc, text_offset(strlen(infotext)), 180,
            infotext, strlen(infotext));
    /*
    #################
    #..Arena.Screen.#
    #################
    */
    } else {
        /* Draw player list */
        for(i=0;i<myworld.players;i++){
            /*
            #############################
            #..Highlight.Current.Player.#
            #############################
            */
            if(i == myworld.current_player -1 && myworld.mode != CE_WORLD_MODE_SELECTSPELLS){

                itemcolor = CE_C_YELLOW;

            /*
            #########################
            #..Grey.Out.Dead.Player.#
            #########################
            */
            } else if (myspells[i+10].dead) {

                itemcolor = CE_C_GREY;

            /*
            ####################
            #..Inactive.Player.#
            ####################
            */
            } else {
                itemcolor = -1;
            }

            if(itemcolor >= 0)
                enable_color_x11(itemcolor);

            XDrawString(display, pixbuff, gc, 0, 15+(i*30), myplayers[i].name,
                strlen(myplayers[i].name));

            XDrawString(display, pixbuff, gc, 0, 30+(i*30), "[",1);

            if(itemcolor >= 0)
                disable_color_x11();

            /*
            #############
            #..Ping.Box.#
            #############
            */

            memset(infotext,'\0',sizeof(infotext));
            for(p=0;p<myplayers[i].ping+1;p++)
                sprintf(infotext,"%s*",infotext);

            switch(strlen(infotext)){
                case 4:
                    pingcolor = CE_C_GREEN;
                    break;
                case 3:
                    pingcolor = CE_C_ORANGE;
                    break;
                case 2:
                    pingcolor = CE_C_YELLOW;
                    break;
                case 1:
                    pingcolor = CE_C_RED;
                    break;
                case 0:
                default:
                    pingcolor = -1;
                    break;
            }

            if(pingcolor >= 0)
                enable_color_x11(pingcolor);

            XDrawString(display, pixbuff, gc, 9, 30+(i*30), infotext,strlen(infotext));

            if(pingcolor >= 0)
                disable_color_x11();

            if(itemcolor >= 0)
                enable_color_x11(itemcolor);

            XDrawString(display, pixbuff, gc, 45, 30+(i*30), "][",2);

            XDrawString(display, pixbuff, gc, 108, 30+(i*30), "]", 1);

            if(itemcolor >= 0)
                disable_color_x11();

            /*
            ############################
            #..Upgrade.Icons.(first 5).#
            ############################
            */
            icon_offset = 0;
            for(u=0;u<5;u++){
                if(myplayers[i].upgrades[u] > 0){
                    enable_color_x11(mymagic_upgrade[myplayers[i].upgrades[u]].color);
                    strcpy(infotext,&mymagic_upgrade[myplayers[i].upgrades[u]].disp[0]);
                    XDrawString(display, pixbuff, gc, 63+(icon_offset*9), 30+(i*30), infotext, 1);
                    icon_offset++;
                }
            }

            disable_color_x11();
        }

        for(i=0;i<MAX_X+2;i++){
            for(j=0;j<MAX_Y+2;j++){
                if(
                    (i<((MAX_X/2) - (arenas[myworld.arenasize][0]/2)+1) ||
                        i>((MAX_X/2) + (arenas[myworld.arenasize][0]/2)))
                    ||
                    (j<((MAX_Y/2) - (arenas[myworld.arenasize][1]/2)+1) ||
                        j>((MAX_Y/2) + (arenas[myworld.arenasize][1]/2)))
                ){
                    XDrawString(display, pixbuff, gc, 117+(i*9), 15+(j*15), "#", 1);
                }
            }
        }

        /*
        ######################
        #..Draw.Battleground.#
        ######################
        */
        for(i=0;i<arenas[myworld.arenasize][0];i++){
            for(j=0;j<arenas[myworld.arenasize][1];j++){
                top_layer = itemstack_top(i,j);
                if(myworld.layout[i][j][top_layer] > 0){

                    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                        log_message,"Found : %d @ %d %d (type = %d)",
                        myworld.layout[i][j][top_layer],i,j,
                        myspells[myworld.layout[i][j][top_layer]].spell_type)
                    );

                    itemcolor = -1;
                    disable_color_x11();

                    switch(myspells[myworld.layout[i][j][top_layer]].spell_type){

                        case SPELL_PLAYER:
                            itemcolor = myplayers[myspells[myworld.layout[i][j][top_layer]].id].color;
                            if(myspells[myworld.layout[i][j][top_layer]].dead){
                                itemcolor = -1;
                                strcpy(item,".");
                            } else {
                                strcpy(item,&myplayers[myspells[myworld.layout[i][j][top_layer]].id].disp[0]);
                            }
                            break;

                        case SPELL_MONSTER:
                            itemcolor = mymonsters[myspells[myworld.layout[i][j][top_layer]].id].color;

                            if(myspells[myworld.layout[i][j][top_layer]].dead &&
                                !myspells[myworld.layout[i][j][top_layer]].illusion){

                                strcpy(item,"~");

                            } else if(myspells[myworld.layout[i][j][top_layer]].dead &&
                                myspells[myworld.layout[i][j][top_layer]].illusion){

                                itemcolor = -1;
                                strcpy(item,".");

                            } else {
                                strcpy(item,
                                    &mymonsters[myspells[myworld.layout[i][j][top_layer]].id].disp[0]);
                            }
                            break;

                        case SPELL_TREE:
                            itemcolor = mytrees[myspells[myworld.layout[i][j][top_layer]].id].color;

                            if(myspells[myworld.layout[i][j][top_layer]].dead){
                                itemcolor = -1;
                                strcpy(item,".");
                            } else {
                                strcpy(item,
                                    &mytrees[myspells[myworld.layout[i][j][top_layer]].id].disp[0]);
                            }
                            break;

                        case SPELL_WALL:
                            itemcolor = mywalls[myspells[myworld.layout[i][j][top_layer]].id].color;

                            if(myspells[myworld.layout[i][j][top_layer]].dead){
                                itemcolor = -1;
                                strcpy(item,".");
                            } else {
                                strcpy(item,
                                    &mywalls[myspells[myworld.layout[i][j][top_layer]].id].disp[0]);
                            }
                            break;

                        case SPELL_BLOB:
                            itemcolor = myblobs[myspells[myworld.layout[i][j][top_layer]].id].color;

                            if(myspells[myworld.layout[i][j][top_layer]].dead){
                                itemcolor = -1;
                                strcpy(item,".");
                            } else {
                                strcpy(item,
                                    &myblobs[myspells[myworld.layout[i][j][top_layer]].id].disp[0]);
                            }
                            break;

                        /* Floor? */
                        default:
                            disable_color_x11();
                            break;
                    }

                    if(itemcolor >= 0)
                        enable_color_x11(itemcolor);

                } else {
                    disable_color_x11();
                    strcpy(item,".");
                }
                XDrawString(display, pixbuff, gc, offset_x + (i*9), offset_y + (j*15), item, 1);
            }
        }

        /*
        ################
        #..Draw.Cursor.#
        ################
        */
        XSetForeground(display, gc, WhitePixel(display,visual_info.screen));

        if(myworld.cursor[0] > arenas[myworld.arenasize][0]-1) {
            myworld.cursor[0] = arenas[myworld.arenasize][0]-1;
        } else if (myworld.cursor[0] < 0) {
            myworld.cursor[0] = 0;
        }

        if(myworld.cursor[1] > arenas[myworld.arenasize][1]-1) {
            myworld.cursor[1] = arenas[myworld.arenasize][1]-1;
        } else if (myworld.cursor[1] < 0) {
            myworld.cursor[1] = 0;
        }

        cursor_x = (myworld.cursor[0] * 9) + offset_x;
        cursor_y = (myworld.cursor[1] * 15) + (offset_y - 15);

        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
            "cursor at %d,%d",cursor_x,cursor_y));

        /* Engaged to fight but not ranged attacking */
        if(myworld.selected_item[0] > 0 && checkadjacent() &&
            myworld.submode != 1){

            enable_color_x11(CE_C_RED);
            XDrawRectangle(display, pixbuff, gc, cursor_x, cursor_y, 9, 15);
            disable_color_x11();

        } else {

            if(myworld.selected_item[0] > 0 &&
                (myworld.submode == 0 || myworld.submode == 5) &&
                can_fly(myspells[myworld.selected_item[0]].current_pos[0],
                        myspells[myworld.selected_item[0]].current_pos[1])) {

                XDrawLine(display, pixbuff, gc, cursor_x, cursor_y,
                    cursor_x + 9, cursor_y + 15);
                XDrawLine(display, pixbuff, gc, cursor_x + 9, cursor_y,
                    cursor_x, cursor_y + 15);

            } else if (myworld.selected_item[0] > 0 && myworld.submode == 1) {

                enable_color_x11(CE_C_RED);

                XDrawLine(display, pixbuff, gc, cursor_x, cursor_y,
                    cursor_x + 9, cursor_y + 15);
                XDrawLine(display, pixbuff, gc, cursor_x + 9, cursor_y,
                    cursor_x, cursor_y + 15);

                disable_color_x11();
            } else {

                XDrawRectangle(display, pixbuff, gc, cursor_x, cursor_y, 9, 15);
            }
        }

        /*
        ##################
        #..Draw.Info.Bar.#
        ##################
        */
        if(!beepmsg)
            update_infobar();

        XDrawString(display, pixbuff, gc, 117, 350, infobar_text, strlen(infobar_text));
    }

    /* Push buffers to screen */
    flushbuffers_x11();
}
