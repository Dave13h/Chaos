/***************************************************************
*  input_common.c                                  #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  Common input routines                           #.....#     *
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
#include "chat.h"
#include "generate.h"
#include "display_common.h"
#include "sound_common.h"
#include "input_common.h"

#ifdef WITH_X11
    #include "input_x11.h"
#endif

#ifdef WITH_NCURSES
    #include "input_ncurses.h"
    #include "display_ncurses.h"
#endif

#ifdef WITH_NET
#include "net.h"
#include "net_client.h"
#include "input_net.h"
extern bool net_enable;
extern bool net_dedicated;
extern bool net_connected;
extern bool net_wait;
extern int server_socket;
#endif

extern char log_message[LOGMSGLEN];
extern int frontend_mode;
extern bool forceupdate;

extern int view_mode;
extern bool view_highlight_arena;

extern char infobar_text[255];
extern bool beepmsg;

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

extern int arenas[MAX_ARENAS][2];

char input_buffer[255];
extern char chat_buffer[MAX_CHATMSG_LENGTH];

/* -=================-
     General Keycodes :
   -=================-
    We should only care about keys within the ascii ranges of :
    32      : space
    48-57   : 0-9
    65-90   : A-Z
    97-122  : a-z
*/

/*
###################
#..empty_buffer().#
###################
*/
void empty_buffer (void)
{
    memset(input_buffer,'\0',sizeof(input_buffer));
}

/*
#################
#..init_input().#
#################
*/
void init_input (void)
{
    /* Blank the buffer */
    empty_buffer();
}

/*
##################
#..lock_cursor().#
##################
*/
bool lock_cursor (void) {

    /* selected item and not in ranged attack mode */
    if(myworld.selected_item[0] > 0 && (myworld.submode == 0 || myworld.submode == 5)){
        if(checkadjacent()){    /* Not engaged to fight */
            return true;
        } else {
            /* Is flying creature/player */
            if(can_fly(myspells[myworld.selected_item[0]].current_pos[0],
                    myspells[myworld.selected_item[0]].current_pos[1])){
                return false;
            } else {
                return true;
            }
        }
    } else if(myworld.submode == 4){    /* Dismount mode */
        return true;
    }

    return false;
}

/*
##################
#..move_cursor().#
##################

 - This function will be called from multiple places during
    the input routine. Simple in/decriments the worlds cursor
    position.

*/
bool move_cursor (int key) {
    bool ret=true;
    if(myworld.mode == CE_WORLD_MODE_CASTING || (myworld.mode == CE_WORLD_MODE_MOVE &&
        (myworld.submode == 4 || myworld.submode == 1)) ||
        myworld.selected_item[0] == 0 || myworld.selected_item[1] > 0 ||
        checkadjacent()){

        if(key == 'a' || key == CE_LEFT){
            myworld.cursor[0] -= 1;
            if(myworld.selected_item[0] && lock_cursor()) {moveitem(-1,0);}

        } else if (key == 'd' || key == CE_RIGHT) {
            myworld.cursor[0] += 1;
            if(myworld.selected_item[0] && lock_cursor()) {moveitem(1,0);}

        } else if (key == 'w' || key == CE_UP) {
            myworld.cursor[1] -= 1;
            if(myworld.selected_item[0] && lock_cursor()) {moveitem(0,-1);}

        } else if (key == 'x' || key == CE_DOWN) {
            myworld.cursor[1] += 1;
            if(myworld.selected_item[0] && lock_cursor()) {moveitem(0,1);}

        } else if (key == 'q' || key == CE_UPLEFT) {
            myworld.cursor[0] -= 1;
            myworld.cursor[1] -= 1;
            if(myworld.selected_item[0] && lock_cursor()) {moveitem(-1,-1);}

        } else if (key == 'e' || key == CE_UPRIGHT) {
            myworld.cursor[0] += 1;
            myworld.cursor[1] -= 1;
            if(myworld.selected_item[0] && lock_cursor()) {moveitem(1,-1);}

        } else if (key == 'z' || key == CE_DOWNLEFT) {
            myworld.cursor[0] -= 1;
            myworld.cursor[1] += 1;
            if(myworld.selected_item[0] && lock_cursor()) {moveitem(-1,1);}

        } else if (key == 'c' || key == CE_DOWNRIGHT) {
            myworld.cursor[0] += 1;
            myworld.cursor[1] += 1;
            if(myworld.selected_item[0] && lock_cursor()) {moveitem(1,1);}

        } else {
            ret = false;
        }
    } else {
        ret = false;
    }

    if(view_highlight_arena && myworld.mode == CE_WORLD_MODE_CASTING)
        checklos();
    return ret;
}

/*
###########################
#..process_input_common().#
###########################
- Global keys that are the same on local/net modes
*/
bool process_input_common (int key) {

    switch(key){
        case CE_BACKSPACE:
        case CE_DELETE:
            if(view_mode == CE_VIEW_CHAT){
                if(strlen(chat_buffer) > 0)
                    sprintf(chat_buffer, "%.*s", (int)strlen(chat_buffer)-1, chat_buffer);
                else
                    CE_beep();
            } else {
                if(strlen(input_buffer) > 0)
                    sprintf(input_buffer, "%.*s", (int)strlen(input_buffer)-1, input_buffer);
                else
                    CE_beep();
            }
            break;

    case CE_F4:     /* Show Hightlight Arena Info */
            if(view_highlight_arena){
                view_highlight_arena = false;
            }else{
                view_highlight_arena = true;
            }
            console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
                "view_highlight_arena is : %d",view_highlight_arena));
        break;

        case CE_F5:     /* Normal View */
            view_mode = CE_VIEW_NORMAL;
            break;

        case CE_F6:     /* History View */
            view_mode = CE_VIEW_HISTORY;
            break;

    #ifdef WITH_NET
        case CE_F7:     /* Chat View */
            view_mode = CE_VIEW_CHAT;
        break;
    #endif

        case CE_F8:     /* Debug View */
            view_mode = CE_VIEW_DEBUG;
            break;

    #ifdef WITH_NCURSES
        case CE_F9:     /* Change Cursor Display (ncurses) */
            ncurses_changecursor();
            break;
    #endif

        case CE_F10:    /* Quit/shutdown */
            shutdown_chaos();
            break;

        case CE_F11:    /* Dump grid to debug out */
            dump_grid();
            break;

        case CE_F12:    /* Dump spells to debug out */
            dump_spells();
            break;
        default:
            return false;
    }
    return true;
}

/*
####################
#..process_input().#
####################

 - Input that has passed the sanity check is no processed.

 - Process flow :

                  [key pressed]
                      |
             -------------------
             |                |
     [special key]     [normal key]
           |      .         |
          |     ....       |      PP      [fill buffer]
         |    ...@...     |      PPP           |
        | ~    ....      |                    | (world.mode = 0)
       | ~~~    .   [world.mode]---->[world.submode]
      |  ~~                                 | (world.mode > 0)
     |                                     |
[call special_func]      ZZ       [action/callfunction]
   |                    ZZZZ             |
  |                      ZZ             |
 ----------------------------------------
     kkkk              |
   kkkkkkkk     [clear buffer]
*/
static void process_input (int key)
{
    int max_inputsize;                  /* input length limit */
    int top_layer;
    int p, rangedistance, spid;
    int maxuses = 0;
    int attack_range = 0;
    int move_range = 0;
    int cursoritem;

    /* -------------------------
        Make safe input limits
    ------------------------- */
    switch(myworld.mode){
        case 0: /*-------- Setup Mode --------*/
            switch(myworld.submode){

                case 0: /*-------- Number of players --------*/
                    max_inputsize = 2;
                    break;

                case 1: /*-------- Player names --------*/
                    max_inputsize = MAX_PLAYERNAME;
                    break;

                case 2: /*-------- Size of arena --------*/
                    max_inputsize = 1;
                    break;
            }
            break;

        default : /*-------- All other modes are single key events --------*/
            max_inputsize = 1;
            break;
    }

    /* Check for common key */
    if(process_input_common(key))
        return;

    /* ---------------
        Handle 'key'
    --------------- */
    switch (key){
        case CE_TAB:
            /* Nothing in Local Mode */
            break;

        case CE_RETURN:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "input is : %s",input_buffer));

            switch(myworld.mode){
                case 0: /*-------- Setup world --------*/
                    switch(myworld.submode){

                        case 0: /*-------- Number of players --------*/

                            if(atoi(input_buffer) > 0 && atoi(input_buffer) < MAX_PLAYERS){
                                myworld.players = atoi(input_buffer);

                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                    log_message,"myworld.players set to : %d",
                                    atoi(input_buffer))
                                );

                                empty_buffer();
                                myworld.submode++;

                            } else {
                                console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(
                                    log_message,"input is out of range!"));
                                CE_beep();
                            }
                            break;

                        case 1: /*-------- Player names --------*/

                            /* Single/Local Player */
                            if(strlen(input_buffer) > 0){

                                memset(myplayers[myworld.current_player-1].name,'\0',
                                    sizeof(myplayers[myworld.current_player - 1].name));

                                myplayers[myworld.current_player-1].id = myworld.current_player-1;

                                sprintf(myplayers[myworld.current_player-1].name,"%s",
                                    input_buffer);

                                empty_buffer();

                                if(myworld.current_player >= myworld.players){
                                    generate_players();
                                    myworld.submode++;
                                } else {
                                    myworld.current_player++;
                                }
                            } else {
                                console_log(__FILE__,__func__,__LINE__,LOG_WARNING,
                                    sprintf(log_message,"input is out of range!"));
                                CE_beep();
                            }
                            break;

                        case 2: /*-------- Arena Size --------*/

                            if(atoi(input_buffer) > 0 &&
                                atoi(input_buffer) < (int)((sizeof(arenas) / sizeof(int)) / 2)){

                                myworld.arenasize = atoi(input_buffer);

                                empty_buffer();

                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                    sprintf(log_message,
                                        "myworld.arenasize set to : %d [max : %d]",
                                        atoi(input_buffer),
                                        (int)(sizeof(arenas) / sizeof(int)) / 2
                                    )
                                );

                                generate_arena();
                                generate_spells();

                                /* Setup world is complete, goto select spells mode */
                                myworld.mode = 1;
                                myworld.submode = 0;
                                myworld.current_player = 1;
                            } else {
                                console_log(__FILE__,__func__,__LINE__,LOG_WARNING,
                                    sprintf(log_message,"input is out of range!"));
                                CE_beep();
                            }
                            break;

                        default : /*-------- Invalid Game SubMode? --------*/
                            console_log(__FILE__,__func__,__LINE__,LOG_ERROR, sprintf(
                                log_message,"Invalid or unimplimented game submode! (%d)",
                                myworld.submode)
                            );
                            CE_beep();
                            break;
                    }
                    break;

                case 1: /*-------- Spell Selection      --------*/
                case 2: /*-------- Casting Round        --------*/
                case 3: /*-------- Movement Round       --------*/
                case 4: /*-------- Grow/animate walls   --------*/
                case 5: /*-------- Game End             --------*/
                    CE_beep();
                    break;

                default : /*-------- Invalid Game Mode? --------*/
                    console_log(__FILE__,__func__,__LINE__,LOG_ERROR, sprintf(
                        log_message,"Invalid or unimplimented game mode! (%d)",
                        myworld.mode)
                    );
                    CE_beep();
                    break;
            }
        break;

    default :

        /* world doesn't 'exist' during setup */
        if(myworld.mode > 0){
            top_layer = itemstack_top(myworld.cursor[0],myworld.cursor[1]);
            cursoritem = myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer];
        }

        switch(myworld.mode){
            case 1: /*-------- Spell Selection --------*/
                switch(myworld.submode){
                    case 0: /* Main Menu */
                        if(key == '1'){
                            myworld.submode = 1;
                        }else if(key == '2'){
                            myworld.submode = 3;
                        }else if(key == '3'){
                            /* Move cursor to players position */
                            myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
                            myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];

                            myworld.submode = 5;
                        }else if(key == '0'){
                            myworld.submode = 0;

                            console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                                log_message,"Player %d Selected Spell : %d",
                                myworld.current_player,
                                myplayers[myworld.current_player-1].selected_spell)
                            );

                            myworld.current_player++;
                            if(myworld.current_player > myworld.players){
                                myworld.current_player = 1;
                                myworld.mode++;

                                myworld.cursor[0] =
                                    myspells[myworld.current_player+9].current_pos[0];

                                myworld.cursor[1] =
                                    myspells[myworld.current_player+9].current_pos[1];
                            }
                        }else{
                            CE_beep();
                            console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                                log_message,"key input : %c[%d]",key,key));
                        }
                        break;
                    case 1:  /* Select spell to view details */
                    case 3:  /* Select spell */
                        if((key - 96) > 0 &&
                            (key - 96) < (myplayers[myworld.current_player - 1].total_spells+1) &&
                            !myspells[myplayers[myworld.current_player - 1].spells[(key - 96)]].beencast)
                        {
                            /* Store Chosen Spell */
                            myplayers[myworld.current_player - 1].selected_spell = (key - 96);

                            /* Just viewing spell? */
                            if(myworld.submode == 1){
                                myworld.submode = 2;

                            /* Chosing spell */
                            } else if(myworld.submode == 3 &&
                                myspells[myplayers[myworld.current_player - 1].spells[myplayers[myworld.current_player - 1].selected_spell]].spell_type == SPELL_MONSTER){

                                /* Ask is monster spell an illusion */
                                myworld.submode = 4;

                            /* Returning from Spell info screen */
                            } else {
                                /* Back to main menu */
                                myworld.submode = 0;
                            }

                        } else if (key == '0' || key == CE_ESCAPE) {
                            myworld.submode = 0;

                        } else {
                            CE_beep();

                        }
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"key input : %c[%d]",key,key));
                        empty_buffer();
                        break;

                    /* Spell Details */
                    case 2:
                        myworld.submode = 1;
                        break;

                    /* Illusion spell */
                    case 4:
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Asking if spell is an illusion.."));

                        if(key == 'y'){

                            console_log(__FILE__,__func__,__LINE__,LOG_DEBUG,
                                sprintf(log_message,"Marking spell as an illusion"));

                            myspells[myplayers[myworld.current_player - 1].spells[myplayers[myworld.current_player - 1].selected_spell]].illusion = true;
                            myworld.submode = 0;
                        }else if (key == 'n'){
                            myworld.submode = 0;
                        } else {
                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                log_message,"Invalid input %c (expecting 'y' or 'n')",key));
                            CE_beep();
                        }
                        break;

                    /* View Arena */
                    case 5:
                        if(!move_cursor(key)){
                            if(key == '0'){
                                myworld.submode = 0;
                                myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
                                myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];
                            } else if (key == 's' || key == 'i' || key == CE_ACTION || key == 32) {
                                if (myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] > 0){
                                        myworld.submode = 6;
                                } else {
                                    CE_beep();
                                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                        sprintf(log_message,"Looking at nothing @ %dx%d",
                                        myworld.cursor[0],myworld.cursor[1])
                                    );
                                }
                            }
                        }
                        break;

                    /* Item Info */
                    case 6:
                        myworld.submode = 5;
                        break;

                    /* Invalid Game SubMode */
                    default :
                        console_log(__FILE__,__func__,__LINE__,LOG_ERROR, sprintf(
                            log_message,"Invalid or unimplimented game submode! (%d)",
                            myworld.submode)
                        );
                        CE_beep();
                        break;
                }
                break;

            case 2: /*-------- Casting Round --------*/
                if(!move_cursor(key)){

                    p = myworld.current_player - 1;

                    if(key == '0'){
                        /* Don't kill multicasting spells */
                        spid = myplayers[p].spells[myplayers[p].selected_spell];
                        switch(myspells[myplayers[p].spells[myplayers[p].selected_spell]].spell_type){
                            case SPELL_BLOB:
                                maxuses = myblobs[myspells[spid].id].uses;
                                break;

                            case SPELL_WALL:
                                maxuses = mywalls[myspells[spid].id].uses;
                                break;

                            case SPELL_TREE:
                                maxuses = mytrees[myspells[spid].id].uses;
                                break;

                            default:
                                maxuses = 1;
                                break;
                        }

                        console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(
                            log_message,"Cancelling spell (%d) uses %d / max uses %d",spid,
                            myspells[myplayers[p].spells[myplayers[p].selected_spell]].uses,
                            maxuses)
                        );

                        if(maxuses == 1 || myspells[myplayers[p].spells[myplayers[p].selected_spell]].uses == maxuses)
                            myspells[myplayers[p].spells[myplayers[p].selected_spell]].dead = true;

                        myspells[myplayers[p].spells[myplayers[p].selected_spell]].beencast = true;

                        myplayers[p].selected_spell = 0;

                        myworld.current_player++;
                        skipdeadplayers();

                        if(myworld.current_player > myworld.players) {
                            myworld.mode = 3;
                            myworld.submode = 0;
                            myworld.current_player = 1;
                        }

                        myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
                        myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];
                    } else if (key == 's' || key == CE_ACTION || key == 32) {
                        switch(myspells[myplayers[p].spells[myplayers[p].selected_spell]].spell_type) {

                            case SPELL_BLOB:
                                if((myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] == 0 ||
                                    myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].dead) &&
                                    (myblobs[myspells[myplayers[p].spells[myplayers[p].selected_spell]].id].casting_range == 0 ||
                                    checkdistance() <= myblobs[myspells[myplayers[p].spells[myplayers[p].selected_spell]].id].casting_range)
                                    && checklos()){

                                    cast_spell();

                                    if(myspells[myplayers[p].spells[myplayers[p].selected_spell]].beencast){

                                        myworld.current_player++;
                                        skipdeadplayers();

                                        if(myworld.current_player > myworld.players) {
                                            myworld.mode = 3;
                                            myworld.submode = 0;
                                            myworld.current_player = 1;
                                        }
                                        myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
                                        myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];
                                    }
                                } else {
                                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                        log_message,
                                        "trying to cast too far from player[%dx%d] => cursor[%dx%d] | range is %d or Blocked",
                                        myspells[myworld.current_player+9].current_pos[0],
                                        myspells[myworld.current_player+9].current_pos[1],
                                        myworld.cursor[0],myworld.cursor[1],
                                        myblobs[myspells[myplayers[p].spells[myplayers[p].selected_spell]].id].casting_range)
                                    );
                                    CE_beep();
                                    sprintf(infobar_text,"Out of range or Invalid Spawn");
                                    beepmsg = true;
                                }
                                break;

                            case SPELL_WALL:
                                if((myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] == 0 ||
                                    myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].dead) &&
                                    (mywalls[myspells[myplayers[p].spells[myplayers[p].selected_spell]].id].casting_range == 0 ||
                                    checkdistance() <= mywalls[myspells[myplayers[p].spells[myplayers[p].selected_spell]].id].casting_range)
                                    && checklos()){

                                    cast_spell();

                                    if(myspells[myplayers[p].spells[myplayers[p].selected_spell]].beencast){

                                        myworld.current_player++;
                                        skipdeadplayers();

                                        if(myworld.current_player > myworld.players) {
                                            myworld.mode = 3;
                                            myworld.submode = 0;
                                            myworld.current_player = 1;
                                        }
                                        myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
                                        myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];
                                    }
                                } else {
                                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                        log_message,
                                        "trying to cast too far from player[%dx%d] => cursor[%dx%d] | range is %d or Blocked",
                                        myspells[myworld.current_player+9].current_pos[0],
                                        myspells[myworld.current_player+9].current_pos[1],
                                        myworld.cursor[0],myworld.cursor[1],
                                        mywalls[myspells[myplayers[p].spells[myplayers[p].selected_spell]].id].casting_range)
                                    );
                                    CE_beep();
                                    sprintf(infobar_text,"Out of range or Invalid Spawn");
                                    beepmsg = true;
                                }
                                break;

                            case SPELL_MAGIC_BALANCE:
                                cast_spell();

                                myworld.current_player++;
                                skipdeadplayers();

                                if(myworld.current_player > myworld.players) {
                                    myworld.mode = 3;
                                    myworld.submode = 0;
                                    myworld.current_player = 1;
                                }
                                myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
                                myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];
                                break;

                            case SPELL_MAGIC_ATTRIB:
                                if(mymagic_spell_attrib[myspells[myplayers[p].spells[myplayers[p].selected_spell]].id].casting_range == 0 ||
                                    checkdistance() <= mymagic_spell_attrib[myspells[myplayers[p].spells[myplayers[p].selected_spell]].id].casting_range){

                                    if(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] > 0){

                                        cast_spell();

                                        /* Spell was 'cast' */
                                        if(myspells[myplayers[p].spells[myplayers[p].selected_spell]].beencast){
                                            myworld.current_player++;
                                            skipdeadplayers();

                                            if(myworld.current_player > myworld.players) {
                                                myworld.mode = 3;
                                                myworld.submode = 0;
                                                myworld.current_player = 1;
                                            }

                                            myworld.cursor[0] =
                                                myspells[myworld.current_player+9].current_pos[0];

                                            myworld.cursor[1] =
                                                myspells[myworld.current_player+9].current_pos[1];
                                        }

                                    } else {
                                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                            sprintf(log_message,
                                            "trying to cast Spell Atrrib Spell on empty pos at %dx%d layer %d",
                                            myworld.cursor[0],myworld.cursor[1],top_layer)
                                        );
                                        CE_beep();
                                        sprintf(infobar_text,"Cannot Cast here");
                                        beepmsg = true;
                                    }
                                } else {
                                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                        log_message,
                                        "trying to cast too far from player[%dx%d] => cursor[%dx%d]",
                                        myspells[myworld.current_player+9].current_pos[0],
                                        myspells[myworld.current_player+9].current_pos[1],
                                        myworld.cursor[0],myworld.cursor[1])
                                    );
                                    CE_beep();
                                    sprintf(infobar_text,"Out of range");
                                    beepmsg = true;
                                }
                                break;

                            case SPELL_MAGIC_UPGRADE:
                                if(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] == myworld.current_player+9 ||
                                    (top_layer > 0 &&
                                    myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer-1] == myworld.current_player+9)
                                ){

                                    cast_spell();

                                    myworld.current_player++;
                                    skipdeadplayers();

                                    if(myworld.current_player > myworld.players) {
                                        myworld.mode = 3;
                                        myworld.submode = 0;
                                        myworld.current_player = 1;
                                    }
                                    myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
                                    myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];
                                } else {
                                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                            sprintf(log_message,
                                            "trying to cast upgrade on non-player owned player at %dx%d layer %d",
                                            myworld.cursor[0],myworld.cursor[1],top_layer)
                                        );
                                        CE_beep();
                                        sprintf(infobar_text,"Cannot Cast here");
                                        beepmsg = true;
                                }
                                break;

                            case SPELL_MAGIC_SPECIAL:
                                if(mymagic_special[myspells[myplayers[p].spells[myplayers[p].selected_spell]].id].casting_range == 0 ||
                                    checkdistance() <= mymagic_special[myspells[myplayers[p].spells[myplayers[p].selected_spell]].id].casting_range){

                                    cast_spell();

                                    myworld.current_player++;
                                    skipdeadplayers();

                                    if(myworld.current_player > myworld.players) {
                                        myworld.mode = 3;
                                        myworld.submode = 0;
                                        myworld.current_player = 1;
                                    }
                                    myworld.cursor[0] =
                                        myspells[myworld.current_player+9].current_pos[0];

                                    myworld.cursor[1] =
                                        myspells[myworld.current_player+9].current_pos[1];

                                } else {
                                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                        log_message,
                                        "trying to cast too far from player[%dx%d] => cursor[%dx%d]",
                                        myspells[myworld.current_player+9].current_pos[0],
                                        myspells[myworld.current_player+9].current_pos[1],
                                        myworld.cursor[0],myworld.cursor[1])
                                    );
                                    CE_beep();
                                    sprintf(infobar_text,"Out of range");
                                    beepmsg = true;
                                }
                                break;

                            case SPELL_TREE:
                                if(((myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] == 0 ||
                                    myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].dead) &&
                                    (mytrees[myspells[myplayers[p].spells[myplayers[p].selected_spell]].id].casting_range == 0 ||
                                    checkdistance() <= mytrees[myspells[myplayers[p].spells[myplayers[p].selected_spell]].id].casting_range)
                                    && checklos() && !checkadjacent_any(myworld.cursor[0] - 1,myworld.cursor[1] - 1)) ||
                                    mytrees[myspells[myplayers[p].spells[myplayers[p].selected_spell]].id].assoc_func == 1){

                                    cast_spell();

                                    if(myspells[myplayers[p].spells[myplayers[p].selected_spell]].beencast){

                                        myworld.current_player++;
                                        skipdeadplayers();

                                        if(myworld.current_player > myworld.players) {
                                            myworld.mode = 3;
                                            myworld.submode = 0;
                                            myworld.current_player = 1;
                                        }
                                        myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
                                        myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];
                                    }
                                } else {
                                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                        log_message,
                                        "trying to cast too far from player[%dx%d] => cursor[%dx%d] | range is %d or Blocked",
                                        myspells[myworld.current_player+9].current_pos[0],
                                        myspells[myworld.current_player+9].current_pos[1],
                                        myworld.cursor[0],myworld.cursor[1],
                                        mytrees[myspells[myplayers[p].spells[myplayers[p].selected_spell]].id].casting_range)
                                    );
                                    CE_beep();
                                    sprintf(infobar_text,"Out of range or Invalid Spawn");
                                    beepmsg = true;
                                }
                                break;

                            case SPELL_MAGIC_RANGED:
                                if(mymagic_ranged[myspells[myplayers[p].spells[myplayers[p].selected_spell]].id].ranged_range == 0 ||
                                    (checkdistance() <= mymagic_ranged[myspells[myplayers[p].spells[myplayers[p].selected_spell]].id].ranged_range &&
                                    checklos())){

                                    if(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] > 0 &&
                                        !myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].dead){

                                        if(myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].player_id != myplayers[myworld.current_player-1].id){
                                            cast_spell();

                                            if(myspells[myplayers[p].spells[myplayers[p].selected_spell]].beencast){
                                                myworld.current_player++;
                                                skipdeadplayers();

                                                if(myworld.current_player > myworld.players) {
                                                    myworld.mode = 3;
                                                    myworld.submode = 0;
                                                    myworld.current_player = 1;
                                                }
                                                myworld.cursor[0] =
                                                    myspells[myworld.current_player+9].current_pos[0];

                                                myworld.cursor[1] =
                                                    myspells[myworld.current_player+9].current_pos[1];
                                            }
                                        } else {
                                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                                sprintf(log_message,
                                                "Trying to attacking friendly at %dx%d layer %d",
                                                myworld.cursor[0],myworld.cursor[1],top_layer)
                                            );
                                            CE_beep();
                                        }
                                    } else {
                                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                            sprintf(log_message,
                                            "Nothing to attacking friendly at %dx%d layer %d",
                                            myworld.cursor[0],myworld.cursor[1],top_layer)
                                        );
                                        sprintf(infobar_text,"Nothing to attack");
                                        beepmsg = true;
                                    }
                                } else {
                                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                        sprintf(log_message,
                                        "trying to cast too far from player[%dx%d] => cursor[%dx%d]",
                                        myspells[myworld.current_player+9].current_pos[0],
                                        myspells[myworld.current_player+9].current_pos[1],
                                        myworld.cursor[0],myworld.cursor[1])
                                    );
                                    CE_beep();
                                    sprintf(infobar_text,"Out of range");
                                    beepmsg = true;
                                }
                                break;

                            case SPELL_MONSTER:
                            default:

                                if((myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] == 0 ||
                                    myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].dead) &&
                                    (myworld.cursor[0] < myspells[p+10].current_pos[0]+2 &&
                                    myworld.cursor[0] > myspells[p+10].current_pos[0]-2) &&
                                    (myworld.cursor[1] < myspells[p+10].current_pos[1]+2 &&
                                    myworld.cursor[1] > myspells[p+10].current_pos[1]-2)) {

                                    cast_spell();

                                    myworld.current_player++;
                                    skipdeadplayers();

                                    if(myworld.current_player > myworld.players) {
                                        myworld.mode = 3;
                                        myworld.submode = 0;
                                        myworld.current_player = 1;
                                    }
                                    myworld.cursor[0] =
                                        myspells[myworld.current_player+9].current_pos[0];

                                    myworld.cursor[1] =
                                        myspells[myworld.current_player+9].current_pos[1];
                                } else {
                                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                        sprintf(log_message,
                                        "trying to spawn too far from player[%dx%d] or ontop of another item => cursor[%dx%d]",
                                        myspells[p].current_pos[0],
                                        myspells[p].current_pos[1],
                                        myworld.cursor[0],myworld.cursor[1])
                                    );
                                    CE_beep();
                                    sprintf(infobar_text,"Out of range/Invalid Spawn");
                                    beepmsg = true;
                                }
                                break;
                        }
                    }
                }
                break;

            case 3: /*-------- Movement Round --------*/
                switch(myworld.submode){
                    case 2:/* Want to mount creature? */
                            if (key == 'y'){
                                mount_item(myworld.cursor[0],myworld.cursor[1]);
                                myworld.selected_item[0] = 0;
                                myworld.selected_item[1] = 0;
                                myworld.submode = 0;
                            } else if (key == 'n'){

                                myworld.cursor[0] =
                                    myspells[myworld.current_player+9].current_pos[0];

                                myworld.cursor[1] =
                                    myspells[myworld.current_player+9].current_pos[1];

                                myworld.submode = 0;
                            } else {
                                CE_beep();
                            }
                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                sprintf(log_message,"Want to mount creature input : %c",key));
                            break;
                    case 3: /* Want to dismount? */
                        if (key == 'y'){
                            myworld.submode = 4;
                        } else if (key == 'n'){
                            myworld.submode = 5;
                        } else {
                            CE_beep();
                        }
                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                            sprintf(log_message,"Want to dismount creature input : %c",key));
                        break;
                    case 6: /* View item return */
                        myworld.submode = 0;
                        break;
                    default:
                        if(!move_cursor(key)){
                            if(key == '0'){
                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                    sprintf(log_message,"Player %d : Ending Turn",
                                    myworld.current_player)
                                );

                                myworld.current_player++;
                                skipdeadplayers();

                                if(myworld.current_player > myworld.players) {
                                    myworld.mode = CE_WORLD_MODE_POSTROUND;

                                    sprintf(infobar_text,"End Round");
                                    beepmsg = true;

                                    myworld.current_player = 1;

                                } else {
                                    myworld.cursor[0] =
                                        myspells[myworld.current_player+9].current_pos[0];

                                    myworld.cursor[1] =
                                        myspells[myworld.current_player+9].current_pos[1];
                                }
                                myworld.submode = 0;
                                myworld.selected_item[0] = 0;
                                myworld.selected_item[1] = 0;


                            } else if (key == 'k' && myworld.selected_item[0] > 0){

                                /* End spell's turn but keep player in control */
                                myspells[myworld.selected_item[0]].beenmoved = true;

                                /* Have a ranged attack turn? */
                                if(myspells[myworld.selected_item[0]].spell_type == SPELL_PLAYER){
                                    attack_range =
                                        myplayers[myspells[myworld.selected_item[0]].id].ranged_range;
                                } else if(myspells[myworld.selected_item[0]].spell_type == SPELL_TREE){
                                    attack_range = 0;
                                } else {
                                    attack_range =
                                        mymonsters[myspells[myworld.selected_item[0]].id].ranged_range;
                                }

                                /* Go to range attack mode */
                                if(myworld.submode == 0 && attack_range > 0){
                                    myworld.submode++;
                                } else {
                                    myworld.selected_item[0] = 0;
                                }

                            } else if(key == 'i'){
                                if (myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] > 0){
                                    myworld.submode = 6;
                                } else {
                                    CE_beep();
                                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                        sprintf(log_message,"Looking at nothing @ %dx%d",
                                        myworld.cursor[0],myworld.cursor[1])
                                    );
                                }

                            } else if (key == 's' || key == CE_ACTION || key == 32) {

                                if((cursoritem > 0 && !myspells[cursoritem].dead) ||
                                    myworld.selected_item[0] > 0) {
                                    /*
                                    ################
                                    #..Select.item.#
                                    ################
                                    */
                                    if(myworld.selected_item[0] == 0) {
                                        /*
                                            This.. is proper ugly =F
                                            If...
                                                There is an item there
                                                Its yours
                                                its not dead
                                                If its a tree, is it worth selecting?
                                                Is not a Blob?
                                                (mount+is mounted / has attacking ability)
                                        */
                                        if(cursoritem > 0 &&
                                            myspells[cursoritem].player_id == myplayers[myworld.current_player-1].id &&
                                            !myspells[cursoritem].beenmoved &&
                                            myspells[cursoritem].spell_type != SPELL_BLOB &&
                                            (myspells[cursoritem].spell_type != SPELL_TREE ||
                                                (myspells[cursoritem].spell_type == SPELL_TREE &&
                                                mytrees[myspells[cursoritem].id].attack > 0) ||
                                                is_mounted(myworld.cursor[0],myworld.cursor[1])
                                            )
                                        ){

                                            /* Is item mounted? */
                                            if(myworld.submode == 0 && is_mounted(myworld.cursor[0],myworld.cursor[1])){

                                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                                    sprintf(log_message,
                                                    "Ask mounted player if they want to dismount")
                                                );

                                                myworld.submode = 3;

                                            }

                                            myspells[cursoritem].beenmoved = true;
                                            myworld.selected_item[0] = cursoritem;

                                            console_log(__FILE__,__func__,__LINE__,LOG_DEBUG,
                                                sprintf(
                                                log_message,"Item thinks it is at %dx%dx%d",
                                                myspells[myworld.selected_item[0]].current_pos[0],
                                                myspells[myworld.selected_item[0]].current_pos[1],
                                                myspells[myworld.selected_item[0]].current_pos[2])
                                            );

                                            switch(myspells[cursoritem].spell_type){
                                                case SPELL_PLAYER:
                                                    myworld.selected_item[1] =
                                                        myplayers[myspells[myworld.selected_item[0]].id].move_range;
                                                    break;

                                                case SPELL_TREE:
                                                case SPELL_WALL:
                                                case SPELL_BLOB:
                                                    myworld.selected_item[1] = 0;
                                                    break;

                                                case SPELL_MONSTER:
                                                default:
                                                    myworld.selected_item[1] =
                                                        mymonsters[myspells[myworld.selected_item[0]].id].move_range;
                                                    break;
                                            }

                                            if(can_fly(myspells[myworld.selected_item[0]].current_pos[0],
                                                    myspells[myworld.selected_item[0]].current_pos[1])){

                                                console_log(__FILE__,__func__,__LINE__,
                                                    LOG_DEBUG, sprintf(log_message,
                                                    "Item can fly %d place(s) this turn",
                                                    myworld.selected_item[1])
                                                );

                                            } else {
                                                console_log(__FILE__,__func__,__LINE__,
                                                    LOG_DEBUG, sprintf(log_message,
                                                    "Item can move %d place(s) this turn",
                                                    myworld.selected_item[1])
                                                );
                                            }
                                        } else {
                                            CE_beep();
                                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                                sprintf(log_message,
                                                "Item is out of moves or doesnt belong to you (beenmoved : %d)",
                                                myspells[cursoritem].beenmoved)
                                            );
                                            sprintf(infobar_text,"Spell doesn't belong to you or is out of moves");
                                            beepmsg = true;
                                        }

                                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                            sprintf(log_message,
                                            "Item (id[%d]type[%d]) on layer %d belongs to %d [you are (%d)]",
                                            cursoritem, myspells[cursoritem].spell_type, top_layer,
                                            myspells[cursoritem].player_id,
                                            myplayers[myworld.current_player-1].id)
                                        );

                                    } else {
                                        if(!checkadjacent() && (myworld.submode == 0 || myworld.submode == 5)){

                                            if(myspells[myworld.selected_item[0]].spell_type == SPELL_PLAYER){

                                                move_range =
                                                    myplayers[myspells[myworld.selected_item[0]].id].move_range;

                                                attack_range =
                                                    myplayers[myspells[myworld.selected_item[0]].id].ranged_range;

                                            } else if(myspells[myworld.selected_item[0]].spell_type == SPELL_TREE){

                                                move_range = 0;
                                                attack_range = 0;

                                            } else {

                                                move_range =
                                                    mymonsters[myspells[myworld.selected_item[0]].id].move_range;

                                                attack_range =
                                                    mymonsters[myspells[myworld.selected_item[0]].id].ranged_range;

                                            }

                                            /*
                                            #####################
                                            #..Flying.Creatures.#
                                            #####################
                                            */
                                            if(can_fly(myspells[myworld.selected_item[0]].current_pos[0],
                                                    myspells[myworld.selected_item[0]].current_pos[1])){

                                                rangedistance = checkdistance();

                                                if(rangedistance > move_range){

                                                    console_log(__FILE__,__func__,__LINE__,
                                                        LOG_DEBUG, sprintf(log_message,
                                                        "Trying to fly too far (cursor distance : %d / creature range %d)",
                                                        rangedistance,
                                                        mymonsters[myspells[myworld.selected_item[0]].id].move_range)
                                                    );

                                                    CE_beep();
                                                } else if (rangedistance < 1) {

                                                    console_log(__FILE__,__func__,__LINE__,
                                                        LOG_DEBUG, sprintf(log_message,
                                                        "Flying source and dest are the same. Ending turn.")
                                                    );

                                                    if(attack_range > 0){
                                                        myworld.submode = 1;
                                                    } else {
                                                        myworld.selected_item[0] = 0;
                                                        myworld.selected_item[1] = 0;
                                                    }
                                                } else {
                                                    moveitem(
                                                        myspells[myworld.selected_item[0]].current_pos[0],
                                                        myspells[myworld.selected_item[0]].current_pos[1]
                                                    );
                                                }
                                            /*
                                            ######################
                                            #..Walking.Creatures.#
                                            ######################
                                            */
                                            } else {
                                                if(attack_range > 0){
                                                    myworld.submode = 1;
                                                } else {
                                                    myworld.selected_item[0] = 0;
                                                    myworld.selected_item[1] = 0;
                                                }
                                                console_log(__FILE__,__func__,__LINE__,
                                                    LOG_DEBUG, sprintf(log_message,
                                                    "Source and dest are the same. Ending turn.")
                                                );
                                            }

                                        /*
                                        ##################
                                        #..Ranged.Attack.#
                                        ##################
                                        */
                                        } else if (myworld.submode == 1){

                                            if(myspells[cursoritem].player_id != myplayers[myworld.current_player-1].id){
                                                rangedistance = checkdistance();

                                                if(myspells[myworld.selected_item[0]].spell_type == SPELL_PLAYER){

                                                    attack_range =
                                                        myplayers[myspells[myworld.selected_item[0]].id].ranged_range;

                                                } else {

                                                    attack_range =
                                                        mymonsters[myspells[myworld.selected_item[0]].id].ranged_range;

                                                }

                                                if(rangedistance > attack_range){
                                                    console_log(__FILE__,__func__,__LINE__,
                                                        LOG_NOTICE, sprintf(log_message,
                                                        "Trying to attack too far (cursor distance : %d / creature range %d)",
                                                        rangedistance,mymonsters[myspells[myworld.selected_item[0]].id].ranged_range)
                                                    );
                                                    CE_beep();
                                                } else if (rangedistance < 1) {
                                                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                                        sprintf(log_message,
                                                        "Attack range source and dest are the same.")
                                                    );
                                                    CE_beep();
                                                } else {
                                                    if(checklos()){
                                                        if(cursoritem > 0 && !myspells[cursoritem].dead) {
                                                            if(creature_attack(myspells[myworld.selected_item[0]].current_pos[0],
                                                                myspells[myworld.selected_item[0]].current_pos[1])){

                                                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                                                    sprintf(log_message,"Successful attack"));

                                                            } else {

                                                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                                                    sprintf(log_message,"Unsuccessful attack"));
                                                                CE_beep();

                                                            }
                                                        } else {
                                                            console_log(__FILE__,__func__,__LINE__,
                                                                LOG_NOTICE, sprintf(log_message,
                                                                "Attacking nothing / wasted shot!")
                                                            );
                                                        }
                                                        myworld.submode = 0;
                                                        myworld.selected_item[0] = 0;
                                                        myworld.selected_item[1] = 0;
                                                    } else {
                                                        console_log(__FILE__,__func__,__LINE__,
                                                            LOG_NOTICE, sprintf(log_message,
                                                            "LOS failed, something blocking shot!")
                                                        );
                                                        CE_beep();
                                                        sprintf(infobar_text,"LOS Failed");
                                                        beepmsg = true;
                                                    }
                                                }
                                            } else if (cursoritem == 0){

                                                console_log(__FILE__,__func__,__LINE__,
                                                    LOG_NOTICE, sprintf(log_message,
                                                    "Noting to attack, wasting shot!")
                                                );
                                                myworld.submode = 0;
                                                myworld.selected_item[0] = 0;
                                                myworld.selected_item[1] = 0;
                                            } else {

                                                console_log(__FILE__,__func__,__LINE__,
                                                    LOG_NOTICE, sprintf(log_message,
                                                    "Trying to attack your own spell! %d vs %d",
                                                    myspells[cursoritem].player_id,
                                                    myplayers[myworld.current_player-1].id)
                                                );
                                                CE_beep();
                                            }

                                        } else {
                                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                                sprintf(log_message,"Engaged to creature!"));
                                            CE_beep();
                                        }
                                    }
                                } else {

                                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                        sprintf(log_message,
                                        "Nothing selected and nothing to select on current position [%dx%d]",
                                        myworld.cursor[0],myworld.cursor[1])
                                    );
                                    CE_beep();
                                }
                            } else {
                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                    sprintf(log_message,"Invalid Key : %c", key));
                                CE_beep();
                            }
                        }
                    break;
                }
                break;

            case 4: /*-------- Grow/animate walls --------*/
                break;

            case 5: /*-------- Game End --------*/
                break;

            default:    /* Fill buffer until full */
                if(strlen(input_buffer) < max_inputsize){
                    sprintf(input_buffer, "%s%c", input_buffer,key);
                } else {
                    console_log(__FILE__,__func__,__LINE__,LOG_WARNING,
                        sprintf(log_message,"input buffer full!"));
                    CE_beep();
                }
                break;
        }
    }
}

/*
##################
#..check_input().#
##################

 - Ask current front end for input.
 - Pass to handler function.
 - Sanity check the input from frontend.
*/
void check_input(void)
{
    int key;

    switch(frontend_mode){
    #ifdef WITH_X11
        case FE_X11:
            key = checkforxevents();
            break;
    #endif
    #ifdef WITH_NCURSES
        case FE_NCURSES:
            key = checkforncursesinput();
            break;
    #endif
    #ifdef WITH_HILDON
        case FE_HILDON:
            key = checkforhildoninput();
            break;
    #endif
    }

    if(key > 0){
        if(key == 32){ /* Space bar */
            console_log(__FILE__,__func__,__LINE__,LOG_DEBUG,
                sprintf(log_message,"Space was pressed [%d]",key));

        } else if(key == 10){ /* Enter key */
            console_log(__FILE__,__func__,__LINE__,LOG_DEBUG,
                sprintf(log_message,"Enter was pressed [%d]",key));
            key = CE_RETURN;

        } else if (    /* Alpha-numeric */
            (key >= 48 && key <= 57) || /* Number */
            (key == 46) ||  /* . */
            (key >= 65 && key <= 90) || /* Upper Case Letter */
            (key >= 97 && key <= 122)       /* Lower Case Letter */
            ) {

            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                sprintf(log_message,"%c was pressed [%d]",key,key));

        /* XXX FIXME : How to findout how many items are in an enum? */
        } else if (key >= 13000 && key <= 13030) {  /* Special Key? */
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "%c was pressed [%d]",key,key));

        } else if(view_mode == CE_VIEW_CHAT) { /* Let anything through chat mode */
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "%c was pressed [%d]",key,key));

        } else {        /* Don't know, don't care */
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "invalid key pressed [%d]",key));
            key = 0;
        }

        beepmsg = false;

        /* Is key still valid? */
        if(key > 0){
            /* Force a Graphics update at next tick */
            forceupdate = true;
#ifdef WITH_NET
            /* Net Mode has its own process_input()s */
            if(net_enable)
                if(net_dedicated)
                    process_input_net_server(key);
                else
                    process_input_net_client(key);
            else
#endif
                process_input(key);
        }

    }

}

/*
##################
#..valid_input().#
##################
*/
bool valid_input (char input[256])
{
    char validchars[62] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";
    int i,j;
    bool valid;

    for(i=0;i<strlen(input);i++){
        valid = false;
        for(j=0;j<strlen(validchars);j++){
            if(input[i] == validchars[j]){
                valid = true;
                break;
            }
        }
        if(!valid){
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "invalid char [%c]",input[i]));
            return false;
        }
    }
    return true;
}
