/***************************************************************
*  display_ncurses.c                               #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  draw routines for ncurses                       #.....#     *
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
#include "display_ncurses.h"

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

short int ncurses_cursor_mode;
static int ncurses_cursors[NCURSES_CURSORS] = {
        A_STANDOUT,
        A_UNDERLINE,
        A_REVERSE,
        A_BOLD
};

static int CE_COLORS_NCURSES[][2] = {
    [CE_C_LGREY] = {COLOR_WHITE,A_NORMAL},
    [CE_C_GREY] = {COLOR_BLACK,A_DIM},
    [CE_C_DGREY] = {COLOR_WHITE,A_DIM},

    [CE_C_LVIOLET] = {COLOR_MAGENTA,A_NORMAL},
    [CE_C_VIOLET] = {COLOR_MAGENTA,A_DIM},
    [CE_C_DVIOLET] = {COLOR_MAGENTA,A_DIM},

    [CE_C_LWHITE] = {COLOR_WHITE,A_BOLD},
    [CE_C_WHITE] = {COLOR_WHITE,A_NORMAL},
    [CE_C_DWHITE] = {COLOR_WHITE,A_DIM},

    [CE_C_LBLUE] = {COLOR_BLUE,A_NORMAL},
    [CE_C_BLUE] = {COLOR_BLUE,A_DIM},
    [CE_C_DBLUE] = {COLOR_BLUE,A_DIM},

    [CE_C_LGREEN] = {COLOR_GREEN,A_BOLD},
    [CE_C_GREEN] = {COLOR_GREEN,A_NORMAL},
    [CE_C_DGREEN] = {COLOR_GREEN,A_DIM},

    [CE_C_LRED] = {COLOR_RED,A_BOLD},
    [CE_C_RED] = {COLOR_RED,A_NORMAL},
    [CE_C_DRED] = {COLOR_RED,A_DIM},

    [CE_C_LORANGE] = {COLOR_YELLOW,A_DIM},
    [CE_C_ORANGE] = {COLOR_YELLOW,A_DIM},
    [CE_C_DORANGE] = {COLOR_YELLOW,A_DIM},

    [CE_C_LYELLOW] = {COLOR_YELLOW,A_BOLD},
    [CE_C_YELLOW] = {COLOR_YELLOW,A_NORMAL},
    [CE_C_DYELLOW] = {COLOR_YELLOW,A_DIM},

    [CE_C_LBROWN] = {COLOR_YELLOW,A_DIM},
    [CE_C_BROWN] = {COLOR_YELLOW,A_DIM},
    [CE_C_DBROWN] = {COLOR_YELLOW,A_DIM},

    [CE_C_LPURPLE] = {COLOR_MAGENTA,A_BOLD},
    [CE_C_PURPLE] = {COLOR_MAGENTA,A_NORMAL},
    [CE_C_DPURPLE] = {COLOR_MAGENTA,A_DIM},

    [CE_C_LCYAN] = {COLOR_CYAN,A_BOLD},
    [CE_C_CYAN] = {COLOR_CYAN,A_NORMAL},
    [CE_C_DCYAN] = {COLOR_CYAN,A_DIM}
};

static int CE_MULTI_COLORS_NCURSES[][6] = {
    [CE_C_MULTI1] = {CE_C_YELLOW,CE_C_VIOLET,CE_C_BLUE,CE_C_GREEN,CE_C_RED,CE_C_CYAN},
    [CE_C_MULTI2] = {CE_C_RED,CE_C_LRED,CE_C_DRED,CE_C_RED,CE_C_LRED,CE_C_DRED},
    [CE_C_MULTI3] = {CE_C_GREEN,CE_C_LGREEN,CE_C_DGREEN,CE_C_GREEN,CE_C_LGREEN,CE_C_DGREEN}
};

static short int anicolor;

WINDOW *window;
static bool ncurses_color;

unsigned short int highlight_arena[MAX_X][MAX_Y];
bool view_highlight_arena;

#ifdef WITH_NET
extern int mycolor;
extern int mypid;
extern bool net_enable;
extern bool net_dedicated;
extern int net_port;
#endif

/*
###################
#..init_ncurses().#
###################
*/
void init_ncurses (void)
{
    if(!initscr()){
        find_front_end();
        return;
    }
    raw();                      /* We control the input */
    noecho();                   /* We control showing the input */
    curs_set(0);                /* Invisible cursor */
    window = newwin(NCURSES_HEIGHT, NCURSES_WIDTH, 0, 0);
    nodelay(window, TRUE);      /* getch() won't block */
    keypad(window, TRUE);       /* Give us our function keys */

    ncurses_cursor_mode = 0;

    mousemask(ALL_MOUSE_EVENTS, NULL); /* Steal Mouse Events */

    if(has_colors()){
        start_color();
        console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
            "Terminal Supports Colour!"));

        use_default_colors();

        init_pair(COLOR_BLACK, COLOR_BLACK, -1);
        init_pair(COLOR_RED, COLOR_RED, -1);
        init_pair(COLOR_GREEN, COLOR_GREEN, -1);
        init_pair(COLOR_YELLOW, COLOR_YELLOW, -1);
        init_pair(COLOR_BLUE, COLOR_BLUE, -1);
        init_pair(COLOR_MAGENTA, COLOR_MAGENTA, -1);
        init_pair(COLOR_CYAN, COLOR_CYAN, -1);
        init_pair(COLOR_WHITE, COLOR_WHITE, -1);

        ncurses_color = true;
    } else {
        console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
            "Terminal Doesn't Support Colour!"));
        ncurses_color = false;
    }
}

/*
#######################
#..shutdown_ncurses().#
#######################
*/
void shutdown_ncurses(void)
{
    curs_set(1);            /* Restore cursor */
    wclear(window);
    delwin(window);
    endwin();
}

/*
###########################
#..ncurses_changecursor().#
###########################
*/
void ncurses_changecursor(void)
{
    ncurses_cursor_mode++;
    if(ncurses_cursor_mode >= NCURSES_CURSORS)
         ncurses_cursor_mode = 0;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
        sprintf(log_message, "Cursor Mode = %d",ncurses_cursor_mode));
}

/*
###########################
#..enable_color_ncurses().#
###########################
*/
static void enable_color_ncurses (int color)
{
    if(color == CE_C_MULTI1 || color == CE_C_MULTI2 || color == CE_C_MULTI3){
        anicolor = rand()%6;
        color = CE_MULTI_COLORS_NCURSES[color][anicolor];
    }

    wattron(window,COLOR_PAIR(CE_COLORS_NCURSES[color][0]));
    wattron(window,CE_COLORS_NCURSES[color][1]);
}

/*
############################
#..disable_color_ncurses().#
############################
*/
static void disable_color_ncurses (int color)
{
    if(color == CE_C_MULTI1 || color == CE_C_MULTI2 || color == CE_C_MULTI3)
        color = CE_MULTI_COLORS_NCURSES[color][anicolor];

    wattroff(window,COLOR_PAIR(CE_COLORS_NCURSES[color][0]));
    wattroff(window,CE_COLORS_NCURSES[color][1]);
}

/*
#############################
#..drawplayerlist_ncurses().#
#############################
*/
static void drawplayerlist_ncurses (bool withsockets)
{
    int i;
    int pingcolor=-1;
    int sobump=0;
    char readyflag[2];
    char infotext[64];

    if(withsockets){
        mvwprintw(window,8,20,"| id | So | P | R | Player");
        mvwprintw(window,9,20,"|----|----|---|---|-----------------------------");
        sobump=5;
    } else {
        mvwprintw(window,8,20,"| id | P | R | Player");
        mvwprintw(window,9,20,"|----|---|---|-----------------------------");
    }

    /* Draw player list */
    for(i=0;i<myworld.players;i++){
        readyflag[0] = ' ';
        if(myplayers[i].ready)
            readyflag[0] = '*';

        /* Player ID */
        memset(infotext,'\0',sizeof(infotext));
        sprintf(infotext,"| %d  |",myplayers[i].id);
        mvwprintw(window,(i+10),20,infotext);

        /* Player Socket */
        if(withsockets){
            memset(infotext,'\0',sizeof(infotext));
            sprintf(infotext," %d  |",myplayers[i].socket);
            mvwprintw(window,(i+10),26,infotext);
        }

        /* Ping */
        if(strlen(myplayers[i].name) > 0){
            if(ncurses_color){
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
                if(pingcolor>0)
                    enable_color_ncurses(pingcolor);
            }
            mvwprintw(window,(i+10),26+sobump," * ");
            if(ncurses_color && pingcolor > 0)
                disable_color_ncurses(pingcolor);
        }

        /* Ready Flag */
        memset(infotext,'\0',sizeof(infotext));
        sprintf(infotext," %c ",readyflag[0]);
        mvwprintw(window,(i+10),29+sobump,"|");
        if(myplayers[i].ready && ncurses_color)
            enable_color_ncurses(CE_C_YELLOW);
        mvwprintw(window,(i+10),30+sobump,infotext);
        if(myplayers[i].ready && ncurses_color)
            disable_color_ncurses(CE_C_YELLOW);
        mvwprintw(window,(i+10),33+sobump,"|");

        /* Player Name */
        if(ncurses_color)
            enable_color_ncurses(myplayers[i].color);
        mvwprintw(window,(i+10),35+sobump,myplayers[i].name);
        if(ncurses_color)
            disable_color_ncurses(myplayers[i].color);
    }
}

/*
##########################
#..drawhistory_ncurses().#
##########################
*/
void drawhistory_ncurses (void)
{
    int i;

    wclear(window);

    /* Draw Border around the screen */
    for(i=0;i<NCURSES_WIDTH;i++){
        mvwprintw(window,i,0, "#");
        mvwprintw(window,i,NCURSES_WIDTH-1, "#");
        mvwprintw(window,0,i, "#");
        mvwprintw(window,NCURSES_HEIGHT-1,i, "#");
    }

    mvwprintw(window,3,text_offset(12), "Game History");

    for(i=0;i<history_count;i++){
        if(ncurses_color)
            enable_color_ncurses(CE_C_ORANGE);

        mvwprintw(window,6+i,9, "[");
        mvwprintw(window,6+i,10, myhistory[i].datetime);
        mvwprintw(window,6+i,18, "]");

        if(ncurses_color)
            disable_color_ncurses(CE_C_ORANGE);

        mvwprintw(window,6+i,20, myhistory[i].message);
    }

    wrefresh(window);
}

/*
#######################
#..drawchat_ncurses().#
#######################
*/
void drawchat_ncurses (void)
{
    int i;

    wclear(window);

    /* Draw Border around the screen */
    for(i=0;i<NCURSES_WIDTH;i++){
        mvwprintw(window,i,0, "#");
        mvwprintw(window,i,NCURSES_WIDTH-1, "#");
        mvwprintw(window,0,i, "#");
        mvwprintw(window,NCURSES_HEIGHT-1,i, "#");
    }

    mvwprintw(window,3,text_offset(12), "Player Chat");

    for(i=0;i<chat_count;i++){
        mvwprintw(window,6+i,3, "[");
        if(ncurses_color)
            enable_color_ncurses(CE_C_ORANGE);

        mvwprintw(window,6+i,4, mychat[i].datetime);

        if(ncurses_color)
            disable_color_ncurses(CE_C_ORANGE);

        mvwprintw(window,6+i,12, "]<");

        if(ncurses_color){
            if(mychat[i].player_id == -1)
                enable_color_ncurses(CE_C_LYELLOW);
            else
                enable_color_ncurses(myplayers[mychat[i].player_id].color);
        }

        if(mychat[i].player_id == -1)
            mvwprintw(window,6+i,14, "Server");
        else
            mvwprintw(window,6+i,14, myplayers[mychat[i].player_id].name);

        if(ncurses_color){
            if(mychat[i].player_id == -1)
                disable_color_ncurses(CE_C_LYELLOW);
            else
                disable_color_ncurses(myplayers[mychat[i].player_id].color);
        }
        mvwprintw(window,6+i,24, ">");

        mvwprintw(window,6+i,26, mychat[i].message);
    }

    /* Show typed chat buffer */
    if(ncurses_color)
        enable_color_ncurses(CE_C_RED);
    mvwprintw(window,22,2,"Message: ");
    if(ncurses_color)
        disable_color_ncurses(CE_C_RED);
    mvwprintw(window,22,12,chat_buffer);

    wrefresh(window);
}

/*
########################
#..drawdebug_ncurses().#
########################
*/
void drawdebug_ncurses (void)
{
    int i;
    char debugtext[64];

    wclear(window);

    /* Draw Border around the screen */
    for(i=0;i<NCURSES_WIDTH;i++){
        mvwprintw(window,i,0, "#");
        mvwprintw(window,i,NCURSES_WIDTH-1, "#");
        mvwprintw(window,0,i, "#");
        mvwprintw(window,NCURSES_HEIGHT-1,i, "#");
    }

    mvwprintw(window,3,text_offset(12), "Debug View");

    mvwprintw(window,5,10, "World Data");

    if(ncurses_color)
        enable_color_ncurses(CE_C_ORANGE);
    mvwprintw(window,6,10, "[Balance]");
    if(ncurses_color)
        disable_color_ncurses(CE_C_ORANGE);
    memset(debugtext,'\0',sizeof(debugtext));
    sprintf(debugtext,"%d",myworld.balance);
    mvwprintw(window,6,28, debugtext);

    if(ncurses_color)
        enable_color_ncurses(CE_C_ORANGE);
    mvwprintw(window,7,10, "[Players]");
    if(ncurses_color)
        disable_color_ncurses(CE_C_ORANGE);
    memset(debugtext,'\0',sizeof(debugtext));
    sprintf(debugtext,"%d",myworld.players);
    mvwprintw(window,7,28, debugtext);

    if(ncurses_color)
        enable_color_ncurses(CE_C_ORANGE);
    mvwprintw(window,8,10, "[Total Spells]");
    if(ncurses_color)
        disable_color_ncurses(CE_C_ORANGE);
    memset(debugtext,'\0',sizeof(debugtext));
    sprintf(debugtext,"%d",myworld.total_spells);
    mvwprintw(window,8,28, debugtext);

    if(ncurses_color)
        enable_color_ncurses(CE_C_ORANGE);
    mvwprintw(window,9,10, "[Current Player]");
    if(ncurses_color)
        disable_color_ncurses(CE_C_ORANGE);
    memset(debugtext,'\0',sizeof(debugtext));
    sprintf(debugtext,"%d",myworld.current_player);
    mvwprintw(window,9,28, debugtext);

    if(ncurses_color)
        enable_color_ncurses(CE_C_ORANGE);
    mvwprintw(window,10,10, "[Selected Item]");
    if(ncurses_color)
        disable_color_ncurses(CE_C_ORANGE);
    memset(debugtext,'\0',sizeof(debugtext));
    sprintf(debugtext,"%d (Turns left: %d)",
        myworld.selected_item[0],myworld.selected_item[1]);
    mvwprintw(window,10,28, debugtext);

    if(ncurses_color)
        enable_color_ncurses(CE_C_ORANGE);
    mvwprintw(window,11,10, "[Mode]");
    if(ncurses_color)
        disable_color_ncurses(CE_C_ORANGE);
    memset(debugtext,'\0',sizeof(debugtext));
    sprintf(debugtext,"%d",myworld.mode);
    mvwprintw(window,11,28, debugtext);

    if(ncurses_color)
        enable_color_ncurses(CE_C_ORANGE);
    mvwprintw(window,12,10, "[Submode]");
    if(ncurses_color)
        disable_color_ncurses(CE_C_ORANGE);
    memset(debugtext,'\0',sizeof(debugtext));
    sprintf(debugtext,"%d",myworld.submode);
    mvwprintw(window,12,28, debugtext);

    wrefresh(window);
}

/*
#############################
#..drawfindserver_ncurses().#
#############################
*/
void drawfindserver_ncurses (void)
{
    int i,c;

    wclear(window);

    switch(myworld.submode){

        /* Connect to IP */
        case 0:
            mvwprintw(window,6,text_offset(23), "Connect to Server by IP");
            mvwprintw(window,10,text_offset(strlen(input_buffer)), input_buffer);

            /* Messages about Server */
            if(beepmsg){
                if(ncurses_color)
                    enable_color_ncurses(CE_C_RED);
                mvwprintw(window,22,text_offset(strlen(infobar_text)),infobar_text);
                if(ncurses_color)
                    disable_color_ncurses(CE_C_RED);
            }
            break;

        /* Enter Name */
        case 1:
            mvwprintw(window,6,text_offset(22), "Enter Your Player Name");
            mvwprintw(window,8,text_offset(strlen(input_buffer)), input_buffer);

            /* Choose Player Colour if front end Supports it */
            if(ncurses_color){
                mvwprintw(window,13,text_offset(32), "Tab to choose your player colour");
                c=1;
                for(i=0;i<MAX_PLAYER_COLORS+1;i++){
                    enable_color_ncurses(c);
                    mvwprintw(window,15,23+c,"@");
                    if(mycolor==i){
                        disable_color_ncurses(c);
                        mvwprintw(window,16,23+c,"^");
                        enable_color_ncurses(c);
                    }
                    disable_color_ncurses(c);
                    c+=3;
                }
            }

            /* Messages From Server */
            if(beepmsg){
                if(ncurses_color)
                    enable_color_ncurses(CE_C_RED);
                mvwprintw(window,22,text_offset(strlen(infobar_text)),infobar_text);
                if(ncurses_color)
                    disable_color_ncurses(CE_C_RED);
            }
            break;

        /* Waiting for Players */
        case 3:
            mvwprintw(window,3,text_offset(19), "Waiting for Players");

            drawplayerlist_ncurses(false);

            if(ncurses_color)
                enable_color_ncurses(CE_C_RED);
            if(beepmsg)
                mvwprintw(window,21,text_offset(strlen(infobar_text)),infobar_text);
            mvwprintw(window,22,text_offset(27),"Press Space to toggle Ready");
            if(ncurses_color)
                disable_color_ncurses(CE_C_RED);
            break;
    }

    wrefresh(window);
}

/*
################################
#..drawwaitforserver_ncurses().#
################################
*/
void drawwaitforserver_ncurses (void)
{
    wclear(window);

    /* Waiting for other players */
    if(myworld.mode == CE_WORLD_MODE_SELECTSPELLS){
        mvwprintw(window,3,text_offset(19), "Waiting for Players");

        drawplayerlist_ncurses(false);

    } else {
        mvwprintw(window,12,text_offset(18), "Waiting for Server");
    }

    if(beepmsg){
        if(ncurses_color)
                enable_color_ncurses(CE_C_RED);
        mvwprintw(window,22,text_offset(strlen(infobar_text)),infobar_text);
        if(ncurses_color)
                disable_color_ncurses(CE_C_RED);
    }

    wrefresh(window);
}

/*
########################
#..drawscene_ncurses().#
########################
*/
void drawscene_ncurses (void)
{
    int i,j,u,p;
    int cursor_x, cursor_y, top_layer,icon_offset;
    int offset_x = ((MAX_X/2) - (arenas[myworld.arenasize][0]/2))+1;
    int offset_y = ((MAX_Y/2) - (arenas[myworld.arenasize][1]/2))+1;
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

    wclear(window);

    /*
    ##################
    #..Setup.Screens.#
    ##################
    */
    if(myworld.mode == CE_WORLD_MODE_SETUP) {

        /* Draw Border around the screen */
        for(i=0;i<NCURSES_WIDTH;i++){
            mvwprintw(window,i,0, "#");
            mvwprintw(window,i,NCURSES_WIDTH-1, "#");
            mvwprintw(window,0,i, "#");
            mvwprintw(window,NCURSES_HEIGHT-1,i, "#");
        }

        /* Draw setup screen title */
        switch(myworld.submode){
            case 0: /* Number of players */
                mvwprintw(window,10,text_offset(18), "Number of Players?");
                break;
            case 1: /* Player names */
                sprintf(infotext,"Player %d Name",myworld.current_player);
                mvwprintw(window,10,text_offset(strlen(infotext)),infotext);
                memset(infotext,'\0',sizeof(infotext));
                break;
            case 2: /* Arena size */
                mvwprintw(window,9,(NCURSES_WIDTH/2)-8,"Arena Size?");
                sprintf(infotext,"1 = [%dx%d] | 2 = [%dx%d] | 3 = [%dx%d]",
                    arenas[1][0],arenas[1][1],
                    arenas[2][0],arenas[2][1],
                    arenas[3][0],arenas[3][1]
                );
                mvwprintw(window,10,text_offset(strlen(infotext)),infotext);
                memset(infotext,'\0',sizeof(infotext));
                break;
        }

        /* Draw buffer contents so user can see what they are typing */
        mvwprintw(window,12,text_offset(strlen(input_buffer)), input_buffer);

    /*
    ####################
    #..Spell.Selection.#
    ####################
    */
    } else if((myworld.mode == CE_WORLD_MODE_SELECTSPELLS ||
        (myworld.mode == CE_WORLD_MODE_MOVE && myworld.submode == 6))
        && myworld.submode != 5 && myworld.arenasize > 0){ /* Spell selection stage */

        /* Draw Border around the screen */
        for(i=0;i<NCURSES_WIDTH;i++){
            mvwprintw(window,i,0, "#");
            mvwprintw(window,i,NCURSES_WIDTH-1, "#");
            mvwprintw(window,0,i, "#");
            mvwprintw(window,NCURSES_HEIGHT-1,i, "#");
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
            itemcolor = CE_C_BLUE;
        } else if (myworld.balance < 0) {
            sprintf(balance_text,"%s","Chaos");
            memset(balance_rating,'*',
                myworld.balance - (myworld.balance + myworld.balance));
            itemcolor = CE_C_RED;
        } else {
            sprintf(balance_text,"%s","Neutral");
            itemcolor = -1;
        }

        if(ncurses_color && itemcolor >= 0)
            enable_color_ncurses(itemcolor);

        sprintf(infotext,"%s - %s %s",myplayers[pid].name,
            balance_text,balance_rating);

        mvwprintw(window,3,text_offset(strlen(infotext)),infotext);

        if(ncurses_color && itemcolor >= 0)
            disable_color_ncurses(itemcolor);

        /*
        ########################
        #..List.players.spells.#
        ########################
        */
        if(myworld.submode == 1 || myworld.submode == 3){

            console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
                "Listing player [%d] spells [total : %d]",pid,
                myplayers[pid].total_spells)
            );

            for(i=1;i<myplayers[pid].total_spells+1;i++){

                console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
                    "Spell %d (id %d)",i,myplayers[pid].spells[i])
                );


                itemid = myspells[myplayers[pid].spells[i]].id;
                spellid = myplayers[pid].spells[i];
                /* Default White Text */
                itemcolor = -1;
                /*
                ########################
                #..List.special.spells.#
                ########################
                */
                if(myspells[spellid].spell_type == SPELL_MAGIC_SPECIAL){
                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"%c - %s",letter[i-1],
                        mymagic_special[itemid].name);
                    mvwprintw(window,4+i,(NCURSES_WIDTH/2)-10,infotext);
                /*
                ########################
                #..List.monster.spells.#
                ########################
                */
                } else {
                    if (!myspells[spellid].beencast){
                        switch(myspells[spellid].spell_type){
                            case SPELL_BLOB:
                                memset(infotext,'\0',sizeof(infotext));
                                sprintf(infotext,"%c - %s",letter[i-1],myblobs[itemid].name);
                                if(ncurses_color){
                                    if(myblobs[itemid].balance > 0){
                                        itemcolor = CE_C_BLUE;
                                    } else if (myblobs[itemid].balance < 0){
                                        itemcolor = CE_C_RED;
                                    }
                                }
                                break;

                            case SPELL_WALL:
                                memset(infotext,'\0',sizeof(infotext));
                                sprintf(infotext,"%c - %s",letter[i-1],mywalls[itemid].name);
                                if(ncurses_color){
                                    if(mywalls[itemid].balance > 0){
                                        itemcolor = CE_C_BLUE;
                                    } else if (mywalls[itemid].balance < 0){
                                        itemcolor = CE_C_RED;
                                    }
                                }
                                break;

                            case SPELL_MAGIC_BALANCE:
                                memset(infotext,'\0',sizeof(infotext));
                                sprintf(infotext,"%c - %s",letter[i-1],mymagic_balance[itemid].name);
                                if(ncurses_color){
                                    if(mymagic_balance[itemid].balance > 0){
                                        itemcolor = CE_C_BLUE;
                                    } else if (mymagic_balance[itemid].balance < 0){
                                        itemcolor = CE_C_RED;
                                    }
                                }
                                break;

                            case SPELL_MAGIC_ATTRIB:
                                memset(infotext,'\0',sizeof(infotext));
                                sprintf(infotext,"%c - %s",letter[i-1],mymagic_spell_attrib[itemid].name);
                                if(ncurses_color){
                                    if(mymagic_spell_attrib[itemid].balance > 0){
                                        itemcolor = CE_C_BLUE;
                                    } else if (mymagic_spell_attrib[itemid].balance < 0){
                                        itemcolor = CE_C_RED;
                                    }
                                }
                                break;

                            case SPELL_MAGIC_UPGRADE:
                                memset(infotext,'\0',sizeof(infotext));
                                sprintf(infotext,"%c - %s",letter[i-1],mymagic_upgrade[itemid].name);
                                if(ncurses_color){
                                    if(mymagic_upgrade[itemid].balance > 0){
                                        itemcolor = CE_C_BLUE;
                                    } else if (mymagic_upgrade[itemid].balance < 0){
                                        itemcolor = CE_C_RED;
                                    }
                                }
                                break;

                            case SPELL_TREE:
                                memset(infotext,'\0',sizeof(infotext));
                                sprintf(infotext,"%c - %s",letter[i-1],mytrees[itemid].name);
                                if(ncurses_color){
                                    if(mytrees[itemid].balance > 0){
                                        itemcolor = CE_C_BLUE;
                                    } else if (mytrees[itemid].balance < 0){
                                        itemcolor = CE_C_RED;
                                    }
                                }
                                break;

                            case SPELL_MAGIC_RANGED:
                                memset(infotext,'\0',sizeof(infotext));
                                sprintf(infotext,"%c - %s",letter[i-1],mymagic_ranged[itemid].name);
                                if(ncurses_color){
                                    if(mymagic_ranged[itemid].balance > 0){
                                        itemcolor = CE_C_BLUE;
                                    } else if (mymagic_ranged[itemid].balance < 0){
                                        itemcolor = CE_C_RED;
                                    }
                                }
                                break;

                            case SPELL_MONSTER:
                            default:
                                memset(infotext,'\0',sizeof(infotext));
                                sprintf(infotext,"%c - %s",letter[i-1],mymonsters[itemid].name);
                                if(ncurses_color){
                                    if(mymonsters[itemid].balance > 0){
                                        itemcolor = CE_C_BLUE;
                                    } else if (mymonsters[itemid].balance < 0){
                                        itemcolor = CE_C_RED;
                                    }
                                }
                                break;

                        }

                        if(ncurses_color && itemcolor >= 0)
                            enable_color_ncurses(itemcolor);

                        /* Draw that badboy... */
                        mvwprintw(window,4+i,(NCURSES_WIDTH/2)-10,infotext);

                        /* Back to White! */
                        if(ncurses_color && itemcolor >= 0)
                            disable_color_ncurses(itemcolor);
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
                    mvwprintw(window,5,text_offset(strlen(infotext)),infotext);

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Attack     %d",myplayers[spellid].attack);
                    mvwprintw(window,7,25,infotext);

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Defense    %d",myplayers[spellid].defense);
                    mvwprintw(window,8,25,infotext);

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Ranged Range    %d",myplayers[spellid].ranged_range);
                    mvwprintw(window,7,40,infotext);

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Ranged Damage   %d",myplayers[spellid].ranged_damage);
                    mvwprintw(window,8,40,infotext);

                    if(myplayers[spellid].flight){
                        itemcolor = CE_C_RED;

                        memset(infotext,'\0',sizeof(infotext));
                        sprintf(infotext,"Flight");

                        if(ncurses_color && itemcolor >= 0)
                            enable_color_ncurses(itemcolor);
                        mvwprintw(window,10,25,infotext);
                        if(ncurses_color && itemcolor >= 0)
                            disable_color_ncurses(itemcolor);
                    }
                    break;

                case SPELL_MONSTER:
                    sprintf(infotext,"Monster Info for %s",mymonsters[spellid].name);
                    mvwprintw(window,5,text_offset(strlen(infotext)),infotext);

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Attack     %d",mymonsters[spellid].attack);
                    mvwprintw(window,7,25,infotext);

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Defense    %d",mymonsters[spellid].defense);
                    mvwprintw(window,8,25,infotext);

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Ranged Range    %d",mymonsters[spellid].ranged_range);
                    mvwprintw(window,7,40,infotext);

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Ranged Damage   %d",mymonsters[spellid].ranged_damage);
                    mvwprintw(window,8,40,infotext);

                    if(mymonsters[spellid].flight){
                        itemcolor = CE_C_RED;
                        memset(infotext,'\0',sizeof(infotext));
                        sprintf(infotext,"Flight");

                        if(ncurses_color && itemcolor >= 0)
                            enable_color_ncurses(itemcolor);
                        mvwprintw(window,10,25,infotext);
                        if(ncurses_color && itemcolor >= 0)
                            disable_color_ncurses(itemcolor);
                    }

                    if(myspells[itemid].undead){
                        itemcolor = CE_C_YELLOW;
                        memset(infotext,'\0',sizeof(infotext));
                        sprintf(infotext,"Undead");

                        if(ncurses_color && itemcolor >= 0)
                            enable_color_ncurses(itemcolor);
                        mvwprintw(window,10,38,infotext);
                        if(ncurses_color && itemcolor >= 0)
                            disable_color_ncurses(itemcolor);
                    }

                    if(mymonsters[spellid].mount){
                        itemcolor = CE_C_RED;
                        memset(infotext,'\0',sizeof(infotext));
                        sprintf(infotext,"Mount");

                        if(ncurses_color && itemcolor >= 0)
                            enable_color_ncurses(itemcolor);
                        mvwprintw(window,10,52,infotext);
                        if(ncurses_color && itemcolor >= 0)
                            disable_color_ncurses(itemcolor);
                    }

                    itemcolor = -1;

                    memset(infotext,'\0',sizeof(infotext));
                    if(mymonsters[spellid].balance > 0){
                        sprintf(infotext,"Balance Law-%d",mymonsters[spellid].balance);
                        itemcolor = CE_C_BLUE;
                    } else if (mymonsters[spellid].balance < 0) {
                        sprintf(infotext,"Balance Chaos-%d",mymonsters[spellid].balance -
                            (mymonsters[spellid].balance + mymonsters[spellid].balance));
                        itemcolor = CE_C_RED;
                    } else {
                        sprintf(infotext,"Balance Neutral");
                    }

                    if(ncurses_color && itemcolor >= 0)
                        enable_color_ncurses(itemcolor);

                    mvwprintw(window,12,text_offset(strlen(infotext)),infotext);

                    if(ncurses_color && itemcolor >= 0)
                        disable_color_ncurses(itemcolor);

                    itemcolor = -1;

                    if(myworld.submode == 6) {

                        memset(infotext,'\0',sizeof(infotext));
                        sprintf(infotext,"Belongs to : %s",
                            myplayers[myspells[itemid].player_id].name);
                        mvwprintw(window,13,text_offset(strlen(infotext)),infotext);

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
                        mvwprintw(window,13,text_offset(strlen(infotext)),infotext);
                    }
                    break;

                case SPELL_MAGIC_RANGED:
                    sprintf(infotext,"Spell Info for %s",mymagic_ranged[spellid].name);
                    mvwprintw(window,5,text_offset(strlen(infotext)),infotext);

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
                        mvwprintw(window,7,text_offset(strlen(infotext)),infotext);

                        /* Display the rest of the string */
                        memset(infotext,'\0',sizeof(infotext));
                        strcpy(infotext,&mymagic_ranged[spellid].description[j]);
                        mvwprintw(window,8,text_offset(strlen(infotext)),infotext);

                    } else {

                        memset(infotext,'\0',sizeof(infotext));
                        sprintf(infotext,"%s",mymagic_ranged[spellid].description);
                        mvwprintw(window,7,text_offset(strlen(infotext)),infotext);
                    }

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Casting Probability %d0%%",
                        mymagic_ranged[spellid].casting_prob);
                    mvwprintw(window,13,text_offset(strlen(infotext)),infotext);
                    break;

                case SPELL_TREE:
                    sprintf(infotext,"Spell Info for %s",mytrees[spellid].name);
                    mvwprintw(window,5,text_offset(strlen(infotext)),infotext);

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
                        mvwprintw(window,7,text_offset(strlen(infotext)),infotext);

                        /* Display the rest of the string */
                        memset(infotext,'\0',sizeof(infotext));
                        strcpy(infotext,&mytrees[spellid].description[j]);
                        mvwprintw(window,8,text_offset(strlen(infotext)),infotext);

                    } else {

                        memset(infotext,'\0',sizeof(infotext));
                        sprintf(infotext,"%s",mytrees[spellid].description);
                        mvwprintw(window,7,text_offset(strlen(infotext)),infotext);
                    }

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Casting Probability %d0%%",
                        mytrees[spellid].casting_prob);
                    mvwprintw(window,13,text_offset(strlen(infotext)),infotext);
                    break;

                case SPELL_MAGIC_SPECIAL:
                    sprintf(infotext,"Spell Info for %s",mymagic_special[spellid].name);
                    mvwprintw(window,5,text_offset(strlen(infotext)),infotext);

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"%s",mymagic_special[spellid].description);
                    mvwprintw(window,10,text_offset(strlen(infotext)),infotext);

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Casting Probability %d0%%",
                        mymagic_special[spellid].casting_prob);
                    mvwprintw(window,13,text_offset(strlen(infotext)),infotext);
                    break;

                case SPELL_MAGIC_UPGRADE:
                    sprintf(infotext,"Spell Info for %s",mymagic_upgrade[spellid].name);
                    mvwprintw(window,5,text_offset(strlen(infotext)),infotext);

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"%s",mymagic_upgrade[spellid].description);
                    mvwprintw(window,10,text_offset(strlen(infotext)),infotext);

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Casting Probability %d0%%",
                        mymagic_upgrade[spellid].casting_prob);
                    mvwprintw(window,13,text_offset(strlen(infotext)),infotext);
                    break;

                case SPELL_MAGIC_ATTRIB:
                    sprintf(infotext,"Spell Info for %s",
                        mymagic_spell_attrib[spellid].name);
                    mvwprintw(window,5,text_offset(strlen(infotext)),infotext);

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"%s",mymagic_spell_attrib[spellid].description);
                    mvwprintw(window,10,text_offset(strlen(infotext)),infotext);

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Casting Probability %d0%%",
                        mymagic_spell_attrib[spellid].casting_prob);
                    mvwprintw(window,13,text_offset(strlen(infotext)),infotext);
                    break;

                case SPELL_MAGIC_BALANCE:
                    sprintf(infotext,"Spell Info for %s",mymagic_balance[spellid].name);
                    mvwprintw(window,5,text_offset(strlen(infotext)),infotext);

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"%s",mymagic_balance[spellid].description);
                    mvwprintw(window,10,text_offset(strlen(infotext)),infotext);

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Casting Probability %d0%%",
                        mymagic_balance[spellid].casting_prob);
                    mvwprintw(window,13,text_offset(strlen(infotext)),infotext);
                    break;

                case SPELL_WALL:
                    sprintf(infotext,"Spell Info for %s",mywalls[spellid].name);
                    mvwprintw(window,5,text_offset(strlen(infotext)),infotext);

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
                        mvwprintw(window,7,text_offset(strlen(infotext)),infotext);

                        /* Display the rest of the string */
                        memset(infotext,'\0',sizeof(infotext));
                        strcpy(infotext,&mywalls[spellid].description[j]);
                        mvwprintw(window,8,text_offset(strlen(infotext)),infotext);

                    } else {

                        memset(infotext,'\0',sizeof(infotext));
                        sprintf(infotext,"%s",mywalls[spellid].description);
                        mvwprintw(window,7,text_offset(strlen(infotext)),infotext);
                    }

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Casting Probability %d0%%",
                        mywalls[spellid].casting_prob);
                    mvwprintw(window,13,text_offset(strlen(infotext)),infotext);
                    break;

                case SPELL_BLOB:
                    sprintf(infotext,"Spell Info for %s",myblobs[spellid].name);
                    mvwprintw(window,5,text_offset(strlen(infotext)),infotext);

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
                        mvwprintw(window,7,text_offset(strlen(infotext)),infotext);

                        /* Display the rest of the string */
                        memset(infotext,'\0',sizeof(infotext));
                        strcpy(infotext,&myblobs[spellid].description[j]);
                        mvwprintw(window,8,text_offset(strlen(infotext)),infotext);

                    } else {

                        memset(infotext,'\0',sizeof(infotext));
                        sprintf(infotext,"%s",myblobs[spellid].description);
                        mvwprintw(window,7,text_offset(strlen(infotext)),infotext);
                    }

                    memset(infotext,'\0',sizeof(infotext));
                    sprintf(infotext,"Casting Probability %d0%%",
                        myblobs[spellid].casting_prob);
                    mvwprintw(window,13,text_offset(strlen(infotext)),infotext);
                    break;

                /* Unknown Item */
                default:
                    mvwprintw(window,15,text_offset(strlen(infotext)),infotext);
                    console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(
                        log_message,"Couldnt get info for Spell id : %d [type %d]",
                        myspells[itemid].id,
                        myspells[itemid].spell_type)
                    );
                    break;
            }

            mvwprintw(window,16,text_offset(27),"[Press any key to continue]");

        /*
        #################
        #..Is.Illusion?.#
        #################
        */
        } else if(myworld.submode == 4){
            mvwprintw(window,10,(NCURSES_WIDTH/2)-10,"Cast as illusion? (y/n)");
        /*
        ##############
        #..Main.Menu.#
        ##############
        */
        } else {
            mvwprintw(window,8,(NCURSES_WIDTH/2)-8,"1. View Spells");
            mvwprintw(window,9,(NCURSES_WIDTH/2)-8,"2. Select Spells");
            mvwprintw(window,10,(NCURSES_WIDTH/2)-8,"3. View Arena");
            mvwprintw(window,11,(NCURSES_WIDTH/2)-8,"0. End Turn");

            /* Messages from Server? */
            if(beepmsg){
                if(ncurses_color)
                    enable_color_ncurses(CE_C_RED);
                mvwprintw(window,22,text_offset(strlen(infobar_text)),infobar_text);
                if(ncurses_color)
                    disable_color_ncurses(CE_C_RED);
            }
        }
    /*
    ####################
    #..Game.End.Screen.#
    ####################
    */
    } else if (myworld.mode == CE_WORLD_MODE_ENDGAME) {
        sprintf(infotext,"Game over %s Wins!",myplayers[myworld.current_player].name);
        mvwprintw(window,9, text_offset(strlen(infotext)),infotext);

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
                itemcolor = CE_C_WHITE;
            /*
            ####################
            #..Inactive.Player.#
            ####################
            */
            } else {
                itemcolor = -1;
            }

            if(ncurses_color && itemcolor >= 0)
                enable_color_ncurses(itemcolor);

            mvwprintw(window,(i*2),0,myplayers[i].name);

            mvwprintw(window,(i*2)+1,0,"[");

            if(ncurses_color && itemcolor >= 0)
                disable_color_ncurses(itemcolor);

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
                case 3:
                    pingcolor = CE_C_GREEN;
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

            if(ncurses_color && pingcolor >= 0)
                enable_color_ncurses(pingcolor);

            mvwprintw(window,(i*2)+1,1,infotext);

            if(ncurses_color && pingcolor >= 0)
                disable_color_ncurses(pingcolor);

            if(ncurses_color && itemcolor >= 0)
                enable_color_ncurses(itemcolor);

            mvwprintw(window,(i*2)+1,5,"][");
            mvwprintw(window,(i*2)+1,12,"]");

            if(ncurses_color && itemcolor >= 0)
                disable_color_ncurses(itemcolor);

            /*
            ############################
            #..Upgrade.Icons.(first 5).#
            ############################
            */
            icon_offset = 7;
            for(u=0;u<5;u++){
                if(myplayers[i].upgrades[u] > 0){
                    itemcolor = mymagic_upgrade[myplayers[i].upgrades[u]].color;

                    if(ncurses_color && itemcolor >= 0)
                        enable_color_ncurses(itemcolor);

                    strcpy(infotext,&mymagic_upgrade[myplayers[i].upgrades[u]].disp[0]);
                    mvwprintw(window,(i*2)+1, icon_offset,infotext);

                    if(ncurses_color && itemcolor >= 0)
                        enable_color_ncurses(itemcolor);

                    icon_offset++;
                }
            }

            if(ncurses_color && itemcolor >= 0)
                disable_color_ncurses(itemcolor);

            itemcolor = -1;
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

                    mvwprintw(window, j, 13+i, "#");
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

                    switch(myspells[myworld.layout[i][j][top_layer]].spell_type){

                        case SPELL_PLAYER:
                            if(myspells[myworld.layout[i][j][top_layer]].dead){
                                itemcolor = CE_C_WHITE;
                                strcpy(item,".");
                            } else {
                                itemcolor = myplayers[myspells[myworld.layout[i][j][top_layer]].id].color;
                                strcpy(item,&myplayers[myworld.layout[i][j][top_layer]-10].disp[0]);
                            }
                            break;

                        case SPELL_MONSTER:
                            itemcolor = mymonsters[myspells[myworld.layout[i][j][top_layer]].id].color;

                            if(myspells[myworld.layout[i][j][top_layer]].dead &&
                                !myspells[myworld.layout[i][j][top_layer]].illusion){

                                strcpy(item,"~");

                            } else if(myspells[myworld.layout[i][j][top_layer]].dead &&
                                myspells[myworld.layout[i][j][top_layer]].illusion){

                                itemcolor = CE_C_WHITE;
                                strcpy(item,".");

                            } else {
                                strcpy(item,
                                    &mymonsters[myspells[myworld.layout[i][j][top_layer]].id].disp[0]);
                            }
                            break;

                        case SPELL_TREE:
                            itemcolor = mytrees[myspells[myworld.layout[i][j][top_layer]].id].color;

                            if(myspells[myworld.layout[i][j][top_layer]].dead){
                                itemcolor = CE_C_WHITE;
                                strcpy(item,".");
                            } else {
                                strcpy(item,
                                    &mytrees[myspells[myworld.layout[i][j][top_layer]].id].disp[0]);
                            }
                            break;

                        case SPELL_WALL:
                            itemcolor = mywalls[myspells[myworld.layout[i][j][top_layer]].id].color;

                            if(myspells[myworld.layout[i][j][top_layer]].dead){
                                itemcolor = CE_C_WHITE;
                                strcpy(item,".");
                            } else {
                                strcpy(item,
                                    &mywalls[myspells[myworld.layout[i][j][top_layer]].id].disp[0]);
                            }
                            break;

                        case SPELL_BLOB:
                            itemcolor = myblobs[myspells[myworld.layout[i][j][top_layer]].id].color;

                            if(myspells[myworld.layout[i][j][top_layer]].dead){
                                itemcolor = CE_C_WHITE;
                                strcpy(item,".");
                            } else {
                                strcpy(item,
                                    &myblobs[myspells[myworld.layout[i][j][top_layer]].id].disp[0]);
                            }
                            break;

                        default:
                            if(ncurses_color && itemcolor >= 0)
                                disable_color_ncurses(itemcolor);
                            itemcolor = -1;
                            break;
                    }

                } else {
                    strcpy(item,".");

                    if(ncurses_color && itemcolor >= 0)
                        disable_color_ncurses(itemcolor);
                    itemcolor = -1;
                }

                if(ncurses_color && itemcolor >= 0)
                    enable_color_ncurses(itemcolor);

                mvwprintw(window, offset_y+j, offset_x+(13+i), item);

                if(ncurses_color && itemcolor >= 0)
                    disable_color_ncurses(itemcolor);
            }
        }

        /*
        ################
        #..Draw.Cursor.#
        ################
        */
        if(myworld.cursor[0] > arenas[myworld.arenasize][0]-1)
            myworld.cursor[0] = arenas[myworld.arenasize][0]-1;
        else if (myworld.cursor[0] < 0)
            myworld.cursor[0] = 0;

        if(myworld.cursor[1] > arenas[myworld.arenasize][1]-1)
            myworld.cursor[1] = arenas[myworld.arenasize][1]-1;
        else if (myworld.cursor[1] < 0)
            myworld.cursor[1] = 0;

        cursor_x = (myworld.cursor[0] + offset_x) + 13;
        cursor_y = myworld.cursor[1] + offset_y;

        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
            "cursor at %d,%d",cursor_x,cursor_y));

        /* Engaged to fight but not ranged attacking */
        if(myworld.selected_item[0] > 0 && checkadjacent() &&
            myworld.submode != 1){

            mvwchgat(window, cursor_y, cursor_x, 1, ncurses_cursors[ncurses_cursor_mode], COLOR_RED, NULL);

        } else {

            if(myworld.selected_item[0] > 0 &&
                (myworld.submode == 0 || myworld.submode == 5) &&
                can_fly(myspells[myworld.selected_item[0]].current_pos[0],
                        myspells[myworld.selected_item[0]].current_pos[1])) {

                mvwchgat(window, cursor_y, cursor_x, 1, ncurses_cursors[ncurses_cursor_mode], COLOR_BLUE, NULL);

            } else if (myworld.selected_item[0] > 0 && myworld.submode == 1) {

                mvwchgat(window, cursor_y, cursor_x, 1, ncurses_cursors[ncurses_cursor_mode], COLOR_RED, NULL);

            } else {

                mvwchgat(window, cursor_y, cursor_x, 1, ncurses_cursors[ncurses_cursor_mode], 0, NULL);
            }
        }

        /*
        ########################
        #..Draw.Highlight.Info.#
        ########################
        */
        if(view_highlight_arena)
            for(i=0;i<MAX_X;i++)
                for(j=0;j<MAX_Y;j++)
                    if(highlight_arena[i][j]){
                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Highlighting at %d,%d, cursor at %d, %d",(i+offset_x)+13,j+offset_y,cursor_x,cursor_y));
                        mvwchgat(window, (j+offset_y), (i+offset_x)+13, 1,
                            ncurses_cursors[ncurses_cursor_mode], COLOR_RED, NULL);
                    }

        /*
        ##################
        #..Draw.Info.Bar.#
        ##################
        */
        if(!beepmsg)
            update_infobar();

        mvwprintw(window,22,13,infobar_text);
    }

    wrefresh(window);
}


/*
#########################
#..drawserver_ncurses().#
#########################
*/
void drawserver_ncurses (void)
{
    int i,j,u,p;
    int cursor_x, cursor_y, top_layer, icon_offset;
    int offset_x = ((MAX_X/2) - (arenas[myworld.arenasize][0]/2))+1;
    int offset_y = ((MAX_Y/2) - (arenas[myworld.arenasize][1]/2))+1;
    int itemcolor = -1;
    int pingcolor = -1;

    char item[2];
    char infotext[64];

    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
        "offset_x : %d / offset_y : %d",offset_x,offset_y));

    memset(item,'\0',sizeof(item));
    memset(infotext,'\0',sizeof(infotext));

    wclear(window);

    /*
    ##################
    #..Setup.Screens.#
    ##################
    */
    if(myworld.mode == CE_WORLD_MODE_SETUP) {

        /* Draw Border around the screen */
        for(i=0;i<NCURSES_WIDTH;i++){
            mvwprintw(window,i,0, "#");
            mvwprintw(window,i,NCURSES_WIDTH-1, "#");
            mvwprintw(window,0,i, "#");
            mvwprintw(window,NCURSES_HEIGHT-1,i, "#");
        }

        /* Submode Title */
        switch(myworld.submode){
            case 0: /* Number of players */
                mvwprintw(window,10,text_offset(18), "Number of Players?");
                break;

            case 1: /* Player names */
            case 3: /* Waiting for players */
                mvwprintw(window,3,text_offset(18), "Player Setup Stage");

                drawplayerlist_ncurses(true);

                break;
            case 2: /* Arena size */
                mvwprintw(window,9,(NCURSES_WIDTH/2)-8,"Arena Size?");
                sprintf(infotext,"1 = [%dx%d] | 2 = [%dx%d] | 3 = [%dx%d]",
                    arenas[1][0],arenas[1][1],
                    arenas[2][0],arenas[2][1],
                    arenas[3][0],arenas[3][1]
                );
                mvwprintw(window,10,text_offset(strlen(infotext)),infotext);
                memset(infotext,'\0',sizeof(infotext));
                break;
        }

        if(myworld.submode != 1){
            /* Draw buffer contents so user can see what they are typing */
            mvwprintw(window,12,text_offset(strlen(input_buffer)), input_buffer);
        }
    } else if(myworld.mode == CE_WORLD_MODE_SELECTSPELLS){
        mvwprintw(window,3,text_offset(28), "Player Spell Selection Stage");

        drawplayerlist_ncurses(true);

    /*
    ####################
    #..Game.End.Screen.#
    ####################
    */
    } else if (myworld.mode == CE_WORLD_MODE_ENDGAME) {
        sprintf(infotext,"Game over %s Wins!",myplayers[myworld.current_player].name);
        mvwprintw(window,9, text_offset(strlen(infotext)),infotext);

    } else {

        /* Draw player list */
        for(i=0;i<myworld.players;i++){
            /*
            #############################
            #..Highlight.Current.Player.#
            #############################
            */
            if(i == myworld.current_player -1){
                itemcolor = CE_C_YELLOW;
            /*
            #########################
            #..Grey.Out.Dead.Player.#
            #########################
            */
            } else if (myspells[i+10].dead) {
                itemcolor = CE_C_WHITE;
            /*
            ####################
            #..Inactive.Player.#
            ####################
            */
            } else {
                itemcolor = -1;
            }

            if(ncurses_color && itemcolor >= 0)
                enable_color_ncurses(itemcolor);

            mvwprintw(window,(i*2),0,myplayers[i].name);

            mvwprintw(window,(i*2)+1,0,"[");

            if(ncurses_color && itemcolor >= 0)
                disable_color_ncurses(itemcolor);

            /*
            #############
            #..Ping.Box.#
            #############
            */

            memset(infotext,'\0',sizeof(infotext));
            for(p=0;p<myplayers[i].ping+1;p++){
                sprintf(infotext,"%s*",infotext);
            }
            switch(strlen(infotext)){
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
                    pingcolor = -1;
                    break;
            }

            if(ncurses_color && pingcolor >= 0)
                enable_color_ncurses(pingcolor);

            mvwprintw(window,(i*2)+1,1,infotext);

            if(ncurses_color && pingcolor >= 0)
                disable_color_ncurses(pingcolor);

            if(ncurses_color && itemcolor >= 0)
                enable_color_ncurses(itemcolor);

            mvwprintw(window,(i*2)+1,5,"][");
            mvwprintw(window,(i*2)+1,12,"]");

            if(ncurses_color && itemcolor >= 0)
                disable_color_ncurses(itemcolor);

            /*
            ############################
            #..Upgrade.Icons.(first 5).#
            ############################
            */
            icon_offset = 7;
            for(u=0;u<5;u++){
                if(myplayers[i].upgrades[u] > 0){
                    itemcolor = mymagic_upgrade[myplayers[i].upgrades[u]].color;

                    if(ncurses_color && itemcolor >= 0)
                        enable_color_ncurses(itemcolor);

                    strcpy(infotext,&mymagic_upgrade[myplayers[i].upgrades[u]].disp[0]);
                    mvwprintw(window,(i*2)+1, icon_offset,infotext);

                    if(ncurses_color && itemcolor >= 0)
                        enable_color_ncurses(itemcolor);

                    icon_offset++;
                }
            }

            if(ncurses_color && itemcolor >= 0)
                disable_color_ncurses(itemcolor);

            itemcolor = -1;
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

                    mvwprintw(window, j, 13+i, "#");
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

                    switch(myspells[myworld.layout[i][j][top_layer]].spell_type){

                        case SPELL_PLAYER:
                            if(myspells[myworld.layout[i][j][top_layer]].dead){
                                itemcolor = CE_C_WHITE;
                                strcpy(item,".");
                            } else {
                                itemcolor = myplayers[myspells[myworld.layout[i][j][top_layer]].id].color;
                                strcpy(item,&myplayers[myworld.layout[i][j][top_layer]-10].disp[0]);
                            }
                            break;

                        case SPELL_MONSTER:
                            itemcolor = mymonsters[myspells[myworld.layout[i][j][top_layer]].id].color;

                            if(myspells[myworld.layout[i][j][top_layer]].dead &&
                                !myspells[myworld.layout[i][j][top_layer]].illusion){

                                strcpy(item,"~");

                            } else if(myspells[myworld.layout[i][j][top_layer]].dead &&
                                myspells[myworld.layout[i][j][top_layer]].illusion){

                                itemcolor = CE_C_WHITE;
                                strcpy(item,".");

                            } else {
                                strcpy(item,
                                    &mymonsters[myspells[myworld.layout[i][j][top_layer]].id].disp[0]);
                            }
                            break;

                        case SPELL_TREE:
                            itemcolor = mytrees[myspells[myworld.layout[i][j][top_layer]].id].color;

                            if(myspells[myworld.layout[i][j][top_layer]].dead){
                                itemcolor = CE_C_WHITE;
                                strcpy(item,".");
                            } else {
                                strcpy(item,
                                    &mytrees[myspells[myworld.layout[i][j][top_layer]].id].disp[0]);
                            }
                            break;

                        case SPELL_WALL:
                            itemcolor = mywalls[myspells[myworld.layout[i][j][top_layer]].id].color;

                            if(myspells[myworld.layout[i][j][top_layer]].dead){
                                itemcolor = CE_C_WHITE;
                                strcpy(item,".");
                            } else {
                                strcpy(item,
                                    &mywalls[myspells[myworld.layout[i][j][top_layer]].id].disp[0]);
                            }
                            break;

                        case SPELL_BLOB:
                            itemcolor = myblobs[myspells[myworld.layout[i][j][top_layer]].id].color;

                            if(myspells[myworld.layout[i][j][top_layer]].dead){
                                itemcolor = CE_C_WHITE;
                                strcpy(item,".");
                            } else {
                                strcpy(item,
                                    &myblobs[myspells[myworld.layout[i][j][top_layer]].id].disp[0]);
                            }
                            break;

                        /* Floor? */
                        default:
                            if(ncurses_color && itemcolor >= 0)
                                disable_color_ncurses(itemcolor);
                            itemcolor = -1;
                            break;
                    }

                } else {
                    strcpy(item,".");

                    if(ncurses_color && itemcolor >= 0)
                        disable_color_ncurses(itemcolor);
                    itemcolor = -1;
                }

                if(ncurses_color && itemcolor >= 0)
                    enable_color_ncurses(itemcolor);

                mvwprintw(window, offset_y+j, offset_x+(13+i), item);

                if(ncurses_color && itemcolor >= 0)
                    disable_color_ncurses(itemcolor);
            }
        }

        /*
        ################
        #..Draw.Cursor.#
        ################
        */
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

        cursor_x = (myworld.cursor[0] + offset_x) + 13;
        cursor_y = myworld.cursor[1] + offset_y;

        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
            "cursor at %d,%d",cursor_x,cursor_y));

        /* Engaged to fight but not ranged attacking */
        if(myworld.selected_item[0] > 0 && checkadjacent() &&
            myworld.submode != 1){

            mvwchgat(window, cursor_y, cursor_x, 1, ncurses_cursors[ncurses_cursor_mode], COLOR_RED, NULL);

        } else {

            if(myworld.selected_item[0] > 0 &&
                (myworld.submode == 0 || myworld.submode == 5) &&
                can_fly(myspells[myworld.selected_item[0]].current_pos[0],
                        myspells[myworld.selected_item[0]].current_pos[1])) {

                mvwchgat(window, cursor_y, cursor_x, 1, ncurses_cursors[ncurses_cursor_mode], COLOR_BLUE, NULL);

            } else if (myworld.selected_item[0] > 0 && myworld.submode == 1) {

                mvwchgat(window, cursor_y, cursor_x, 1, ncurses_cursors[ncurses_cursor_mode], COLOR_RED, NULL);

            } else {

                mvwchgat(window, cursor_y, cursor_x, 1, ncurses_cursors[ncurses_cursor_mode], 0, NULL);
            }
        }

        /*
        ##################
        #..Draw.Info.Bar.#
        ##################
        */
        if(!beepmsg)
            update_infobar();

        mvwprintw(window,22,13,infobar_text);
    }
    wrefresh(window);
}
