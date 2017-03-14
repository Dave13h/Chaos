/***************************************************************
*  input_net.c                                     #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  Network input routines                          #.....#     *
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
#include "generate.h"
#include "chat.h"
#include "display_common.h"
#include "sound_common.h"
#include "input_common.h"
#include "input_net.h"

#ifdef WITH_NCURSES
    #include "input_ncurses.h"
    #include "display_ncurses.h"
#endif

#include "net.h"
#include "net_client.h"
#include "net_server.h"
extern bool net_enable;
extern bool net_dedicated;
extern bool net_connected;
extern bool net_wait;
extern int server_socket;

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

extern char input_buffer[255];
extern char chat_buffer[MAX_CHATMSG_LENGTH];

extern int mypid;

/*
###############################
#..process_input_net_server().#
###############################
*/
void process_input_net_server (int key)
{
    int max_inputsize; /* input length limit */
    int i,p;

    /* -------------------------
        Make safe input limits
    ------------------------- */
    switch(myworld.mode){
        case CE_WORLD_MODE_SETUP: /*-------- Setup Mode --------*/
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

    /* Chat mode has it's own seperate buffer */
    if(view_mode == CE_VIEW_CHAT)
        max_inputsize = MAX_CHATMSG_LENGTH-1;

    /* Check for common key */
    if(process_input_common(key))
        return;

    /* ---------------
        Handle 'key'
    --------------- */
    switch (key){
        case CE_RETURN:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "input is : %s",input_buffer));

            /* Chat Mode */
            if(view_mode == CE_VIEW_CHAT){
                chat_add(-1,chat_buffer);
                chat_empty_buffer();
                net_wait = false; /* Let the server send an update */
                break;
            }

            switch(myworld.mode){
                case CE_WORLD_MODE_SETUP: /*-------- Setup world --------*/
                    switch(myworld.submode){

                        case 0: /*-------- Number of players --------*/

                            if(atoi(input_buffer) > 0 && atoi(input_buffer) < MAX_PLAYERS){
                                myworld.players = atoi(input_buffer);

                                /* clean some entries for socket field */
                                for (p=0;p<myworld.players;p++){
                                    myplayers[p].socket = 0;
                                }

                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                    log_message,"myworld.players set to : %d",
                                    atoi(input_buffer))
                                );

                                empty_buffer();
                                myworld.submode=3;

                            } else {
                                console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(
                                    log_message,"input is out of range!"));
                                CE_beep();
                            }
                            break;

                        case 1: /*-------- Player names --------*/
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

                                /* Tell the Clients */
                                net_gamestate(CE_NET_SOCKET_ALL);
                                net_arenalayout(CE_NET_SOCKET_ALL);
                                net_playerstate(CE_NET_SOCKET_ALL);
                                net_historylog(CE_NET_SOCKET_ALL);
                                net_spelllist(CE_NET_SOCKET_ALL);
                                for(i=0;i<MAX_SPELL_TYPES+1;i++)
                                    net_spelldata(CE_NET_SOCKET_ALL,i);

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

                case CE_WORLD_MODE_SELECTSPELLS:/*-------- Spell Selection      --------*/
                case CE_WORLD_MODE_CASTING:     /*-------- Casting Round        --------*/
                case CE_WORLD_MODE_MOVE:        /*-------- Movement Round       --------*/
                case CE_WORLD_MODE_POSTROUND:   /*-------- Grow/animate walls   --------*/
                case CE_WORLD_MODE_ENDGAME:     /*-------- Game End             --------*/
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
        /* Chat Mode */
        if(view_mode == CE_VIEW_CHAT){
            if((int)strlen(chat_buffer) < max_inputsize){
                sprintf(chat_buffer, "%s%c", chat_buffer,key);
            } else {
                console_log(__FILE__,__func__,__LINE__,LOG_WARNING,
                    sprintf(log_message,"chat buffer full!"));
                CE_beep();
            }
            break;
        }

        switch(myworld.mode){
            case CE_WORLD_MODE_SELECTSPELLS:/*-------- Spell Selection      --------*/
            case CE_WORLD_MODE_CASTING:     /*-------- Casting Round        --------*/
            case CE_WORLD_MODE_MOVE:        /*-------- Movement Round       --------*/
            case CE_WORLD_MODE_POSTROUND:   /*-------- Grow/animate walls   --------*/
            case CE_WORLD_MODE_ENDGAME:     /*-------- Game End             --------*/
                break;

            default:    /* Fill buffer until full */
                if((int)strlen(input_buffer) < max_inputsize){
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
######################
#..move_cursor_net().#
######################

 - This function will be called from multiple places during
    the input routine. Simple in/decriments the worlds cursor
    position.

*/
static bool move_cursor_net (int key)
{
    bool ret=true;
    if(myworld.mode == CE_WORLD_MODE_CASTING || (myworld.mode == CE_WORLD_MODE_MOVE &&
        (myworld.submode == 4 || myworld.submode == 5 || myworld.submode == 1)) ||
        myworld.selected_item[0] == 0 || myworld.selected_item[1] > 0 ||
        checkadjacent()){

        if(key == 'a' || key == CE_LEFT){
            myworld.cursor[0] -= 1;
            if(myworld.selected_item[0] && lock_cursor()) {net_send_move(myworld.cursor[0],myworld.cursor[1],-1,0);}

        } else if (key == 'd' || key == CE_RIGHT) {
            myworld.cursor[0] += 1;
            if(myworld.selected_item[0] && lock_cursor()) {net_send_move(myworld.cursor[0],myworld.cursor[1],1,0);}

        } else if (key == 'w' || key == CE_UP) {
            myworld.cursor[1] -= 1;
            if(myworld.selected_item[0] && lock_cursor()) {net_send_move(myworld.cursor[0],myworld.cursor[1],0,-1);}

        } else if (key == 'x' || key == CE_DOWN) {
            myworld.cursor[1] += 1;
            if(myworld.selected_item[0] && lock_cursor()) {net_send_move(myworld.cursor[0],myworld.cursor[1],0,1);}

        } else if (key == 'q' || key == CE_UPLEFT) {
            myworld.cursor[0] -= 1;
            myworld.cursor[1] -= 1;
            if(myworld.selected_item[0] && lock_cursor()) {net_send_move(myworld.cursor[0],myworld.cursor[1],-1,-1);}

        } else if (key == 'e' || key == CE_UPRIGHT) {
            myworld.cursor[0] += 1;
            myworld.cursor[1] -= 1;
            if(myworld.selected_item[0] && lock_cursor()) {net_send_move(myworld.cursor[0],myworld.cursor[1],1,-1);}

        } else if (key == 'z' || key == CE_DOWNLEFT) {
            myworld.cursor[0] -= 1;
            myworld.cursor[1] += 1;
            if(myworld.selected_item[0] && lock_cursor()) {net_send_move(myworld.cursor[0],myworld.cursor[1],-1,1);}

        } else if (key == 'c' || key == CE_DOWNRIGHT) {
            myworld.cursor[0] += 1;
            myworld.cursor[1] += 1;
            if(myworld.selected_item[0] && lock_cursor()) {net_send_move(myworld.cursor[0],myworld.cursor[1],1,1);}

        } else {
            ret = false;
        }
    } else {
        ret = false;
    }

    if(view_highlight_arena && myworld.mode==2)
        checklos();
    return ret;
}

/*
###############################
#..process_input_net_client().#
###############################
*/
void process_input_net_client (int key)
{
    int max_inputsize = 1;          /* input length limit */
    int top_layer;
    int p, cursoritem;

    /* -------------------------
        Make safe input limits
    ------------------------- */
    switch(myworld.mode){
        case CE_WORLD_MODE_SETUP: /*-------- Setup Mode --------*/
            switch(myworld.submode){

                case 0: /*-------- Server IP Address --------*/
                    max_inputsize = 15;
                    break;

                case 1: /*-------- Player names --------*/
                    max_inputsize = MAX_PLAYERNAME;
                    break;
            }
            break;

        default : /*-------- All other modes are single key events --------*/
            max_inputsize = 1;
            break;
    }

    /* Chat mode has it's own seperate buffer */
    if(view_mode == CE_VIEW_CHAT)
        max_inputsize = MAX_CHATMSG_LENGTH-1;


    /* Check for common key */
    if(process_input_common(key))
        return;

    /* ---------------
       Handle 'key'
    --------------- */
    switch (key){

        case CE_TAB:
            if(view_mode == CE_VIEW_CHAT)
                break;

            switch(myworld.mode){
                case CE_WORLD_MODE_SETUP: /*-------- Setup world --------*/
                    switch(myworld.submode){
                        case 1: /*-------- Player names --------*/
                            net_client_color();
                            break;

                        case 0:     /*-------- Connect to Server IP     --------*/
                        case 2:     /*-------- Waiting for Players      --------*/
                        case 3:     /*-------- Waiting for Players      --------*/
                        default :   /*-------- Invalid Game SubMode?    --------*/
                            CE_beep();
                            break;
                    }
                    break;

                case CE_WORLD_MODE_SELECTSPELLS:/*-------- Spell Selection      --------*/
                case CE_WORLD_MODE_CASTING:     /*-------- Casting Round        --------*/
                case CE_WORLD_MODE_MOVE:        /*-------- Movement Round       --------*/
                case CE_WORLD_MODE_POSTROUND:   /*-------- Grow/animate walls   --------*/
                case CE_WORLD_MODE_ENDGAME:     /*-------- Game End             --------*/
                default :   /*-------- Invalid Game Mode?   --------*/
                    CE_beep();
                    break;
            }
        break;

        case CE_RETURN:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "input is : %s",input_buffer));

            /* Chat Mode */
            if(view_mode == CE_VIEW_CHAT){
                net_send_chatmsg(chat_buffer);
                chat_empty_buffer();
                break;
            }

            switch(myworld.mode){
                case CE_WORLD_MODE_SETUP: /*-------- Setup world --------*/
                    switch(myworld.submode){

                        case 0: /*-------- Connect to Server IP --------*/

                            if((int)strlen(input_buffer) > 6)
                                if(net_connecttoserver(input_buffer))
                                    empty_buffer();
                            break;

                        case 1: /*-------- Player names --------*/

                            /* Net client sends player name to server */
                            if(net_enable && net_connected){
                                if((int)strlen(input_buffer) > 0){
                                    net_joingame(input_buffer);
                                } else {
                                    console_log(__FILE__,__func__,__LINE__,LOG_WARNING,
                                        sprintf(log_message,"input is out of range!"));
                                    CE_beep();
                                }
                            }

                            break;

                        case 2:   /*-------- Waiting for Players --------*/
                        case 3:   /*-------- Waiting for Players --------*/
                        default : /*-------- Invalid Game SubMode? --------*/
                            console_log(__FILE__,__func__,__LINE__,LOG_ERROR, sprintf(
                                log_message,"Invalid or unimplimented game submode! (%d)",
                                myworld.submode)
                            );
                            CE_beep();
                            break;
                    }
                    break;

                case CE_WORLD_MODE_SELECTSPELLS:/*-------- Spell Selection      --------*/
                case CE_WORLD_MODE_CASTING:     /*-------- Casting Round        --------*/
                case CE_WORLD_MODE_MOVE:        /*-------- Movement Round       --------*/
                case CE_WORLD_MODE_POSTROUND:   /*-------- Grow/animate walls   --------*/
                case CE_WORLD_MODE_ENDGAME:     /*-------- Game End             --------*/
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
        /* Chat Mode */
        if(view_mode == CE_VIEW_CHAT){
            if((int)strlen(chat_buffer) < max_inputsize){
                sprintf(chat_buffer, "%s%c", chat_buffer,key);
            } else {
                console_log(__FILE__,__func__,__LINE__,LOG_WARNING,
                    sprintf(log_message,"chat buffer full!"));
                CE_beep();
            }
            break;
        }

        /* World doesn't 'exist' during setup */
        if(myworld.mode > CE_WORLD_MODE_SETUP){
            top_layer = itemstack_top(myworld.cursor[0],myworld.cursor[1]);
            cursoritem = myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer];
        }

        switch(myworld.mode){
            case CE_WORLD_MODE_SELECTSPELLS: /*-------- Spell Selection --------*/
                switch(myworld.submode){
                    case 0: /* Main Menu */
                        if(key == '1'){
                            myworld.submode = 1;
                        }else if(key == '2'){
                            myworld.submode = 3;
                        }else if(key == '3'){
                            /* Move cursor to players position */
                            myworld.cursor[0] = myspells[mypid+10].current_pos[0];
                            myworld.cursor[1] = myspells[mypid+10].current_pos[1];

                            myworld.submode = 5;
                        }else if(key == '0'){
                            /* Tell Server we have selected a Spell and wait for response */
                            net_send_selectedspell(myplayers[mypid].selected_spell,
                                myspells[myplayers[mypid].spells[myplayers[mypid].selected_spell]].illusion);
                            net_wait=true;

                        }else{
                            CE_beep();
                            console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                                log_message,"key input : %c[%d]",key,key));
                        }
                        break;
                    case 1:  /* Select spell to view details */
                    case 3:  /* Select spell */
                        if((key - 96) > 0 &&
                            (key - 96) < (myplayers[mypid].total_spells+1) &&
                            !myspells[myplayers[mypid].spells[(key - 96)]].beencast)
                        {
                            /* Store Chosen Spell */
                            myplayers[mypid].selected_spell = (key - 96);

                            /* Just viewing spell? */
                            if(myworld.submode == 1){
                                myworld.submode = 2;

                            /* Chosing spell */
                            } else if(myworld.submode == 3 &&
                                myspells[myplayers[mypid].spells[myplayers[mypid].selected_spell]].spell_type == SPELL_MONSTER){

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

                            console_log(__FILE__,__func__,__LINE__,LOG_ALL,
                                sprintf(log_message,"Marking spell %d (main spell id %d) as an illusion",
                                    myplayers[mypid].selected_spell,myplayers[mypid].spells[myplayers[mypid].selected_spell]));

                            myspells[myplayers[mypid].spells[myplayers[mypid].selected_spell]].illusion = true;
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
                                if(!isspelldead(mypid+10)){
                                    myworld.submode = 0;
                                    myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
                                    myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];
                                } else {
                                    CE_beep();
                                    sprintf(infobar_text,"You are Dead");
                                    beepmsg = true;
                                }
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

            case CE_WORLD_MODE_CASTING: /*-------- Casting Round --------*/

                /* Dead players can look around*/
                if(isspelldead(myplayers[mypid].id+10)){
                    if(!move_cursor(key) && myworld.submode != 2)
                        myworld.submode = 5;
                    return;
                }

                if(mypid != (myworld.current_player-1))
                    break;

                if(!move_cursor(key)){
                    p = myworld.current_player - 1;

                    /* Skip Go */
                    if(key == '0'){
                        net_send_endturn();
                        net_wait=true;

                     /* Attempt Cast*/
                    } else if (key == 's' || key == CE_ACTION || key == 32) {
                        net_send_action(CE_NET_ACTION_CAST,myworld.cursor[0],myworld.cursor[1]);
                    }
                }
                break;

            case CE_WORLD_MODE_MOVE: /*-------- Movement Round --------*/

                /* Dead players can look around*/
                if(isspelldead(myplayers[mypid].id+10)){
                    if(!move_cursor(key) && myworld.submode != 2)
                        myworld.submode = 5;
                    return;
                }

                if(mypid != (myworld.current_player-1))
                    break;

                switch(myworld.submode){
                    case 2:/* Want to mount creature? */
                        if (key == 'y')
                            net_send_mount(1,1);
                        else if (key == 'n')
                            net_send_mount(1,0);
                        else
                            CE_beep();
                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                            sprintf(log_message,"Want to mount creature input : %c",key));
                        break;

                    case 3: /* Want to dismount? */
                        if (key == 'y')
                            net_send_mount(2,1);
                        else if (key == 'n')
                            net_send_mount(2,0);
                        else
                            CE_beep();
                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                            sprintf(log_message,"Want to dismount creature input : %c",key));
                        break;

                    case 6: /* View item return */
                        myworld.submode = 0;
                        break;

                    default:
                        if(!move_cursor_net(key)){
                            if(key == '0'){
                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                    sprintf(log_message,"Player %d : Ending Turn",
                                    myworld.current_player)
                                );

                                net_send_endturn();

                            } else if (key == 'k' && myworld.selected_item[0] > 0){

                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                    sprintf(log_message,"Player %d : Ending Turn",
                                    myworld.current_player)
                                );

                                net_send_action(CE_NET_ACTION_DESELECT,myworld.cursor[0],myworld.cursor[1]);

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

                                net_send_action(CE_NET_ACTION_SELECT,myworld.cursor[0],myworld.cursor[1]);

                            } else {
                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                    sprintf(log_message,"Invalid Key : %c", key));
                                CE_beep();
                            }
                        }
                    break;
                }
                break;

            case CE_WORLD_MODE_POSTROUND: /*-------- Grow/animate walls --------*/
                break;

            case CE_WORLD_MODE_ENDGAME: /*-------- Game End --------*/
                break;

            default:

                /* Waiting for Players screen, SPACE to toggle ready */
                if(myworld.mode == CE_WORLD_MODE_SETUP && myworld.submode == 3 &&
                    view_mode == CE_VIEW_NORMAL && key == 32){
                    net_send_ready();
                    break;
                }

                /* Fill buffer until full */
                if((int)strlen(input_buffer) < max_inputsize){
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
