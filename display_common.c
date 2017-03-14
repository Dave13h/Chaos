/***************************************************************
*  display_common.c                                #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  common draw routines                            #.....#     *
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

#ifdef WITH_X11
    #include "display_x11.h"
#endif

#ifdef WITH_NCURSES
    #include "display_ncurses.h"
#endif

#ifdef WITH_HILDON
    #include "display_hildon.h"
#endif

extern int frontend_mode;
extern bool forceupdate;

int view_mode = 1;

char infobar_text[255];
bool beepmsg = false;

extern int logging_level;
extern char log_message[LOGMSGLEN];

#ifdef WITH_NET
#include "net.h"
extern bool net_enable;
extern bool net_dedicated;
extern bool net_connected;
extern bool net_wait;
extern int net_port;
#endif

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

unsigned short int highlight_arena[MAX_X][MAX_Y];
bool view_highlight_arena;

/*
##################
#..text_offset().#
##################
*/
int text_offset(int strlength){
    switch(frontend_mode){
        case FE_X11:
        case FE_HILDON:
            return (WIN_WIDTH/2) - ((strlength*9) /2);
        case FE_NCURSES:
            return (NCURSES_WIDTH/2) - ((strlength) /2);
    }
    return 0;
}

/*
#######################
#..clear_highlights().#
#######################
*/
void clear_highlights (void) {
    int x,y;
    for(x=0;x<MAX_X;x++)
        for(y=0;y<MAX_Y;y++)
            highlight_arena[x][y]=0;
}

/*
###################
#..init_display().#
###################
*/
void init_display(void)
{
    switch(frontend_mode){
    #ifdef WITH_X11
        case FE_X11:
            init_x11();
            return;
    #endif
    #ifdef WITH_NCURSES
        case FE_NCURSES:
            init_ncurses();
            return;
    #endif
    #ifdef WITH_HILDON
        case FE_HILDON:
            init_hildon();
            return;
    #endif
    }

    /* Nothing init'd */
    find_front_end();

    clear_highlights();
    view_highlight_arena=false;
}

/*
#####################
#..update_infobar().#
#####################
*/
void update_infobar (void)
{
    int p = myworld.current_player - 1;
    int top_layer = itemstack_top(myworld.cursor[0],myworld.cursor[1]);
    int cursoritem = myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer];
    int spellid = myplayers[p].spells[myplayers[p].selected_spell];

    /*
    #####################
    #..Round/Game.Setup.#
    #####################
    */
    if(myworld.mode == CE_WORLD_MODE_SELECTSPELLS){
        /*
        ###############
        #..View.Arena.#
        ###############
        */
        if(myworld.submode == 5){
            if(cursoritem > 0 && !(myspells[cursoritem].dead && myspells[cursoritem].illusion)){
                switch(myspells[cursoritem].spell_type) {
                case SPELL_PLAYER:
                    sprintf(infobar_text,"%s",myplayers[myspells[cursoritem].id].name);
                    break;

                case SPELL_MONSTER:
                    sprintf(infobar_text,"%s",mymonsters[myspells[cursoritem].id].name);

                    /* Is mounted? */
                    if(top_layer > 0 && mymonsters[myspells[cursoritem].id].mount &&
                        !myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer-1]].dead &&
                        myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer-1]].spell_type == SPELL_PLAYER){
                        sprintf(infobar_text,"%s [MOUNTED]",infobar_text);
                    }

                    /* Is dead? */
                    if(myspells[cursoritem].dead){
                        sprintf(infobar_text,"%s [DEAD]",infobar_text);
                    }

                    break;

                case SPELL_TREE:
                    sprintf(infobar_text,"%s",mytrees[myspells[cursoritem].id].name);

                    /* Is mounted? */
                    if(top_layer > 0 && mytrees[myspells[cursoritem].id].mount &&
                        !myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer-1]].dead &&
                        myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer-1]].spell_type == SPELL_PLAYER){
                        sprintf(infobar_text,"%s [MOUNTED]",infobar_text);
                    }

                    break;

                case SPELL_WALL:
                    sprintf(infobar_text,"%s",mywalls[myspells[cursoritem].id].name);

                    /* Is mounted? */
                    if(top_layer > 0 && mywalls[myspells[cursoritem].id].mount &&
                        !myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer-1]].dead &&
                        myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer-1]].spell_type == SPELL_PLAYER){
                        sprintf(infobar_text,"%s [MOUNTED]",infobar_text);
                    }

                    break;

                case SPELL_BLOB:
                    sprintf(infobar_text,"%s",myblobs[myspells[cursoritem].id].name);
                    break;

                default:
                    sprintf(infobar_text,"Nothing");
                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                        "Info bar - unknown spell type (%d) or no spell selected",
                        myspells[spellid].spell_type)
                    );
                    break;
                }
                /* If not current players spell (label) who's it is */
                if(myspells[cursoritem].spell_type > 0 && myspells[cursoritem].spell_type < 10 &&
                    myspells[cursoritem].player_id != p){
                    sprintf(infobar_text,"%s (%s)",infobar_text,myplayers[myspells[cursoritem].player_id].name);
                }
            } else {
                sprintf(infobar_text,"Nothing");
            }
        }
    /*
    #######################
    #..Spell.Casting.Turn.#
    #######################
    */
    } else if (myworld.mode == CE_WORLD_MODE_CASTING) {
        if(myplayers[p].selected_spell > 0){
            switch(myspells[spellid].spell_type) {
                case SPELL_MONSTER:
                    sprintf(infobar_text,"%s",mymonsters[myspells[spellid].id].name);
                    break;

                case SPELL_MAGIC_RANGED:
                    sprintf(infobar_text,"%s",mymagic_ranged[myspells[spellid].id].name);
                    if(mymagic_ranged[myspells[spellid].id].uses > 0){
                        sprintf(infobar_text,"%s [Uses left : %d]",infobar_text,myspells[spellid].uses);
                    }
                    break;

                case SPELL_TREE:
                    sprintf(infobar_text,"%s",mytrees[myspells[spellid].id].name);
                    if(mytrees[myspells[spellid].id].uses > 0){
                        sprintf(infobar_text,"%s [Uses left : %d]",infobar_text,myspells[spellid].uses);
                    }
                    break;

                case SPELL_MAGIC_SPECIAL:
                    /*
                    ##################################
                    #..Casting.Special.Spell.On.item.#
                    ##################################
                    */
                    if(cursoritem > 0){
                        switch(myspells[cursoritem].spell_type){
                            case 0:
                                sprintf(infobar_text,"%s - %s",mymagic_special[myspells[spellid].id].name,
                                myplayers[myspells[cursoritem].id].name);
                                break;
                            case 1:
                                sprintf(infobar_text,"%s - %s (%s)",mymagic_special[myspells[spellid].id].name,
                                mymonsters[myspells[cursoritem].id].name,
                                myplayers[myspells[cursoritem].player_id].name);

                                /* Is mounted? */
                                if(top_layer > 0 && mymonsters[myspells[cursoritem].id].mount &&
                                    myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer-1]].spell_type == SPELL_PLAYER){
                                    sprintf(infobar_text,"%s [MOUNTED]",infobar_text);
                                }
                                break;
                            default:
                                console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                                    "Info bar - casting special spell on unknown spell type %d",
                                    myspells[cursoritem].spell_type)
                                );
                                break;
                        }
                    } else {
                        sprintf(infobar_text,"%s",mymagic_special[myspells[spellid].id].name);
                    }
                    break;

                case SPELL_MAGIC_UPGRADE:
                    /*
                    ##################################
                    #..Casting.Upgrade.Spell.On.item.#
                    ##################################
                    */
                    if(cursoritem > 0){
                        switch(myspells[cursoritem].spell_type){
                            case 0:
                                sprintf(infobar_text,"%s - %s",mymagic_upgrade[myspells[spellid].id].name,
                                myplayers[myspells[cursoritem].id].name);
                                break;
                            case 1:
                                sprintf(infobar_text,"%s - %s (%s)",mymagic_upgrade[myspells[spellid].id].name,
                                    mymonsters[myspells[cursoritem].id].name,
                                    myplayers[myspells[cursoritem].player_id].name);

                                /* Is mounted? */
                                if(top_layer > 0 && mymonsters[myspells[cursoritem].id].mount &&
                                    myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer-1]].spell_type == SPELL_PLAYER){
                                    sprintf(infobar_text,"%s [MOUNTED]",infobar_text);
                                }
                                break;
                            default:
                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                                    "Info bar - casting upgrade spell on unknown spell type %d",
                                    myspells[cursoritem].spell_type)
                                );
                                break;
                        }
                    } else {
                        sprintf(infobar_text,"%s",
                            mymagic_upgrade[myspells[spellid].id].name);
                    }
                    break;

                case SPELL_MAGIC_ATTRIB:
                    sprintf(infobar_text,"%s",
                        mymagic_spell_attrib[myspells[spellid].id].name);
                    break;

                case SPELL_MAGIC_BALANCE:
                    sprintf(infobar_text,"%s",
                        mymagic_balance[myspells[spellid].id].name);
                    break;

                case SPELL_WALL:
                    sprintf(infobar_text,"%s",mywalls[myspells[spellid].id].name);
                    if(mywalls[myspells[spellid].id].uses > 0){
                        sprintf(infobar_text,"%s [Uses left : %d]",infobar_text,myspells[spellid].uses);
                    }
                    break;

                case SPELL_BLOB:
                    sprintf(infobar_text,"%s",myblobs[myspells[spellid].id].name);
                    if(myblobs[myspells[spellid].id].uses > 0){
                        sprintf(infobar_text,"%s [Uses left : %d]",infobar_text,myspells[spellid].uses);
                    }
                    break;

                default:
                    sprintf(infobar_text,"Unknown Spell Type");
                    console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                        "Info bar - unknown spell type %d",myspells[spellid].spell_type));
                    break;
            }
        }
    /*
    ##################
    #..Movement.Turn.#
    ##################
    */
    } else if (myworld.mode == CE_WORLD_MODE_MOVE) {
        /*
        ###################
        #..Spell.Selected.#
        ###################
        */
        if(myworld.selected_item[0] > 0){

            /*
            ###########
            #..Mount?.#
            ###########
            */
            if(myworld.submode == 2){
                switch(myspells[cursoritem].spell_type){
                    case SPELL_TREE:
                        sprintf(infobar_text,"Mount Tree? (y/n)");
                        break;

                    case SPELL_WALL:
                        sprintf(infobar_text,"Enter Castle? (y/n)");
                        break;

                    default:
                        sprintf(infobar_text,"Mount Creature? (y/n)");
                        break;
                }
            } else if(myworld.submode == 3){
                switch(myspells[cursoritem].spell_type){
                    case SPELL_TREE:
                        sprintf(infobar_text,"Dismount Tree? (y/n)");
                        break;

                    case SPELL_WALL:
                        sprintf(infobar_text,"Leave Castle? (y/n)");
                        break;

                    default:
                        sprintf(infobar_text,"Dismount Creature? (y/n)");
                        break;
                }
            } else if(myworld.submode == 4){
                sprintf(infobar_text,"Dismount direction?");
            } else {

                switch(myspells[myworld.selected_item[0]].spell_type) {
                    case SPELL_PLAYER:
                        if((myworld.submode == 0 || myworld.submode == 5) && !checkadjacent()){
                            if(myplayers[myspells[myworld.selected_item[0]].id].flight){
                                sprintf(infobar_text,"%s (Flying : %d)",
                                    myplayers[myspells[myworld.selected_item[0]].id].name,
                                    myworld.selected_item[1]
                                );
                            } else {
                                sprintf(infobar_text,"%s (Moves Left : %d)",
                                    myplayers[myspells[myworld.selected_item[0]].id].name,
                                    myworld.selected_item[1]
                                );
                            }
                        } else if (myworld.submode == 1) {
                            sprintf(infobar_text,"%s (Ranged Attack : %d)",
                                myplayers[myspells[myworld.selected_item[0]].id].name,
                                myplayers[myspells[myworld.selected_item[top_layer]].id].ranged_range
                            );
                        } else {
                            sprintf(infobar_text,"%s (Engaged to Enemy)",
                                myplayers[myspells[cursoritem].id].name);
                        }
                    break;

                    case SPELL_TREE:
                        if((myworld.submode == 0 || myworld.submode == 5) && !checkadjacent()){
                            sprintf(infobar_text,"%s",mytrees[myspells[myworld.selected_item[0]].id].name);
                        } else {
                            sprintf(infobar_text,"%s (Engaged to Enemy)",
                                mytrees[myspells[cursoritem].id].name);
                        }
                        break;

                    case SPELL_WALL:
                        if((myworld.submode == 0 || myworld.submode == 5) && !checkadjacent()){
                            sprintf(infobar_text,"%s",mywalls[myspells[myworld.selected_item[0]].id].name);
                        } else {
                            sprintf(infobar_text,"%s (Engaged to Enemy)",
                                mywalls[myspells[cursoritem].id].name);
                        }
                        break;

                    case SPELL_BLOB:
                        if((myworld.submode == 0 || myworld.submode == 5) && !checkadjacent()){
                            sprintf(infobar_text,"%s",myblobs[myspells[myworld.selected_item[0]].id].name);
                        } else {
                            sprintf(infobar_text,"%s (Engaged to Enemy)",
                                myblobs[myspells[cursoritem].id].name);
                        }
                        break;

                    case SPELL_MONSTER:
                    default:
                        if((myworld.submode == 0 || myworld.submode == 5) && !checkadjacent()){
                            if(mymonsters[myspells[myworld.selected_item[0]].id].flight){
                                sprintf(infobar_text,"%s (Flying : %d)",
                                    mymonsters[myspells[myworld.selected_item[0]].id].name,
                                    myworld.selected_item[1]
                                );
                            } else {
                                sprintf(infobar_text,"%s (Moves Left : %d)",
                                    mymonsters[myspells[myworld.selected_item[0]].id].name,
                                    myworld.selected_item[1]
                                );
                            }
                        } else if (myworld.submode == 1) {
                            sprintf(infobar_text,"%s (Ranged Attack : %d)",
                                mymonsters[myspells[myworld.selected_item[0]].id].name,
                                mymonsters[myspells[myworld.selected_item[0]].id].ranged_range
                            );
                        } else {
                            sprintf(infobar_text,"%s (Engaged to Enemy)",
                                mymonsters[myspells[cursoritem].id].name);
                        }
                        break;
                }
            }
        /*
        ###############
        #..View.Arena.#
        ###############
        */
        } else if(cursoritem > 0 && myworld.selected_item[0] == 0 &&
            !(myspells[cursoritem].dead && myspells[cursoritem].illusion)){

            switch(myspells[cursoritem].spell_type) {
                case SPELL_PLAYER:
                    sprintf(infobar_text,"%s",myplayers[myspells[cursoritem].id].name);
                    break;

                case SPELL_MONSTER:
                    sprintf(infobar_text,"%s",mymonsters[myspells[cursoritem].id].name);

                    /* Is mounted? */
                    if(top_layer > 0 && mymonsters[myspells[cursoritem].id].mount &&
                        !myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer-1]].dead &&
                        myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer-1]].spell_type == SPELL_PLAYER){
                        sprintf(infobar_text,"%s [MOUNTED]",infobar_text);
                    }

                    /* Is dead? */
                    if(myspells[cursoritem].dead){
                        sprintf(infobar_text,"%s [DEAD]",infobar_text);
                    }

                    break;

                case SPELL_TREE:
                    sprintf(infobar_text,"%s",mytrees[myspells[cursoritem].id].name);

                    /* Is mounted? */
                    if(top_layer > 0 && mytrees[myspells[cursoritem].id].mount &&
                        !myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer-1]].dead &&
                        myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer-1]].spell_type == SPELL_PLAYER){
                        sprintf(infobar_text,"%s [MOUNTED]",infobar_text);
                    }

                    break;

                case SPELL_WALL:
                    sprintf(infobar_text,"%s",mywalls[myspells[cursoritem].id].name);

                    /* Is mounted? */
                    if(top_layer > 0 && mywalls[myspells[cursoritem].id].mount &&
                        !myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer-1]].dead &&
                        myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer-1]].spell_type == SPELL_PLAYER){
                        sprintf(infobar_text,"%s [MOUNTED]",infobar_text);
                    }

                    break;

                case SPELL_BLOB:
                    sprintf(infobar_text,"%s",myblobs[myspells[cursoritem].id].name);
                    break;

                default:
                    sprintf(infobar_text,"Nothing");
                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                        "Info bar - unknown spell type (%d) or no spell selected",
                        myspells[spellid].spell_type)
                    );
                    break;
                }

                /* If not current players spell (label) who's it is */
                if(myspells[cursoritem].spell_type > 0 && myspells[cursoritem].spell_type < 10 &&
                    myspells[cursoritem].player_id != p){
                    sprintf(infobar_text,"%s (%s)",infobar_text,myplayers[myspells[cursoritem].player_id].name);
                }
        } else {
            sprintf(infobar_text,"Nothing");
        }
    }
}


/*
################
#..drawscene().#
################
*/
void drawscene(void)
{
    if(forceupdate){
        switch(frontend_mode){
        #ifdef WITH_X11
            case FE_X11:
                switch(view_mode){
                #ifdef WITH_NET
                    case CE_VIEW_CHAT:
                        drawchat_x11();
                        break;
                #endif
                    case CE_VIEW_HISTORY:
                        drawhistory_x11();
                        break;
                    case CE_VIEW_DEBUG:
                        drawdebug_x11();
                        break;
                    case CE_VIEW_NORMAL:
                    default:
                #ifdef WITH_NET
                        if(!net_dedicated && net_wait)
                            drawwaitforserver_x11();
                        else if(net_dedicated)
                            drawserver_x11();
                        else if(net_enable && myworld.mode == CE_WORLD_MODE_SETUP)
                            drawfindserver_x11();
                        else
                #endif
                            drawscene_x11();
                        break;
                }
                break;
        #endif
        #ifdef WITH_NCURSES
            case FE_NCURSES:

                switch(view_mode){
                #ifdef WITH_NET
                    case CE_VIEW_CHAT:
                        drawchat_ncurses();
                        break;
                #endif
                    case CE_VIEW_HISTORY:
                        drawhistory_ncurses();
                        break;
                    case CE_VIEW_DEBUG:
                        drawdebug_ncurses();
                        break;
                    case CE_VIEW_NORMAL:
                    default:
                #ifdef WITH_NET
                        if(!net_dedicated && net_wait)
                            drawwaitforserver_ncurses();
                        else if(net_dedicated)
                            drawserver_ncurses();
                        else if(net_enable && myworld.mode == CE_WORLD_MODE_SETUP)
                            drawfindserver_ncurses();
                        else
                #endif
                            drawscene_ncurses();
                        break;
                }
                break;
        #endif
        #ifdef WITH_HILDON
            case FE_HILDON:
                /*
                    Note : For Hildon, we draw all the views at the same time as
                        they are all drawn on different 'slides' so the user can
                        'flick' between them instead of keypressing to change view.
                */
                #ifdef WITH_NET
                drawchat_hildon();
                #endif
                drawhistory_hildon();
                #ifdef WITH_NET
                if(!net_dedicated && net_wait)
                    drawwaitforserver_hildon();
                else if(net_dedicated)
                    drawserver_hildon();
                else if(net_enable && myworld.mode == CE_WORLD_MODE_SETUP)
                    drawfindserver_hildon();
                else
                #endif
                    drawscene_hildon();
                break;
        #endif
        }
        forceupdate = false;
    }
}

/*
#######################
#..shutdown_display().#
#######################
*/
void shutdown_display(void)
{
    switch(frontend_mode){
    #ifdef WITH_X11
        case FE_X11:
            shutdown_x11();
            break;
    #endif
    #ifdef WITH_NCURSES
        case FE_NCURSES:
            shutdown_ncurses();
            break;
    #endif
    #ifdef WITH_HILDON
        case FE_HILDON:
            shutdown_hildon();
            break;
    #endif
    }
}
