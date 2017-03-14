/***************************************************************
*  magic_spell_attrib.c                            #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  magic spell attribute functions/routines        #.....#     *
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
#include "history.h"
#include "chaos.h"
#include "magic_spell_attrib.h"
#include "sound_common.h"

extern world                myworld;
extern player               myplayers[MAX_PLAYERS];
extern spells               myspells[MAX_SPELLS];
extern monster              mymonsters[MAX_MONSTERS];
extern tree                 mytrees[MAX_TREES];
extern magic_spell_attrib   mymagic_spell_attrib[MAX_MAGIC_SPELL_ATTRIB];

extern char log_message[LOGMSGLEN];

extern char infobar_text[255];
extern bool beepmsg;

extern int arenas[MAX_ARENAS][2];

/*
#############################
#..spell_attrib_raisedead().#
#############################
*/
static bool spell_attrib_raisedead(void){
    int layer_top = itemstack_top(myworld.cursor[0],myworld.cursor[1]);
    char histtext[255];

    memset(histtext,'\0',sizeof(histtext));

    if(myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top]].dead){
        myspells[myplayers[myworld.current_player - 1].spells[myplayers[myworld.current_player - 1].selected_spell]].beencast = true;

        sprintf(histtext,"%s Raised %s from the dead",
            myplayers[myworld.current_player - 1].name,
            mymonsters[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top]].id].name
        );

        return true;
    }

    sprintf(histtext,"%s Failed to raise a dead %s",
        myplayers[myworld.current_player - 1].name,
        mymonsters[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top]].id].name
    );

    sprintf(infobar_text,"Target is not Dead");
    beepmsg = true;
    CE_beep();
    return false;
}

/*
##############################
#..spell_attrib_subversion().#
##############################
*/
static bool spell_attrib_subversion(void){
    int magic_res, castroll;
    int layer_top = itemstack_top(myworld.cursor[0],myworld.cursor[1]);
    int itemid = myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top]].id;
    int cur_x = myworld.cursor[0] - 1;
    int cur_y = myworld.cursor[1] - 1;
    int i,j,um_top_layer;
    char histtext[255];

    memset(histtext,'\0',sizeof(histtext));

    /* Can't subvert non-monster item */
    if(myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top]].spell_type != SPELL_MONSTER){
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Cant subvert item [type was %d]",
            myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top]].spell_type)
        );
        sprintf(infobar_text,"Can't Subvert Non-Monster Spell");
        beepmsg = true;
        CE_beep();
        myspells[myplayers[myworld.current_player - 1].spells[myplayers[myworld.current_player - 1].selected_spell]].beencast = false;
        return false;
    }

    magic_res = mymonsters[itemid].magic_resistance;
    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Monster %d's Magic resistance is %d",itemid,magic_res));

    if(is_mounted(myworld.cursor[0],myworld.cursor[1])){
        magic_res +=
            myplayers[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top]].player_id].magic_resistance;

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Target is mounted adding player's (%d) Magic resistance : %d (total : %d)",
            myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top]].player_id,
            myplayers[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top]].player_id].magic_resistance,
            magic_res)
        );
    }

    srand(time(NULL));
    castroll = rand()%9;
    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Casting roll was %d",castroll));

    myspells[myplayers[myworld.current_player - 1].spells[myplayers[myworld.current_player - 1].selected_spell]].beencast = true;

    if(castroll >= magic_res){
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Subversion was successfull resistance was less than or equal to castroll %d < %d",
            magic_res,castroll)
        );

        sprintf(infobar_text,"Successful");
        beepmsg = true;

        if(is_mounted(myworld.cursor[0],myworld.cursor[1])){
            /* Find a safe place to move mounted creature */
            for(i=0;i<3;i++){
                for(j=0;j<3;j++){
                    if((cur_x+i >= 0 && cur_y+j >=0) &&
                        (cur_x+i < arenas[myworld.arenasize][0]) &&
                        (cur_y+j < arenas[myworld.arenasize][1])){

                        um_top_layer = itemstack_top(cur_x+i,cur_y+j);

                        if(myworld.layout[cur_x+i][cur_y+j][um_top_layer] == 0 ||
                            (myspells[myworld.layout[cur_x+i][cur_y+j][um_top_layer]].dead &&
                            !myspells[myworld.layout[cur_x+i][cur_y+j][um_top_layer]].illusion)
                        ){

                            if(myworld.layout[cur_x+i][cur_y+j][um_top_layer] > 0){
                                um_top_layer++;
                            }

                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                log_message,
                                "Found suitible position for moving subverted mount @ %dx%dx%d",
                                cur_x+i,cur_y+j,um_top_layer)
                            );

                            /* Set new pos */
                            myworld.layout[cur_x+i][cur_y+j][um_top_layer] = myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top];

                            myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top]].current_pos[0] = cur_x+i;
                            myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top]].current_pos[1] = cur_y+j;
                            myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top]].current_pos[2] = um_top_layer;

                            /* Unset old pos */
                            myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top] = 0;

                            /* Move Cursor to new position for 'change owner' */
                            myworld.cursor[0] = cur_x+i;
                            myworld.cursor[1] = cur_y+j;

                            return true;
                        }
                    }
                }
            }
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                log_message,
                "Failed to find suitible position for moving subverted mount")
            );
        }
        sprintf(histtext,"%s Successfully Subverts %s's %s",
            myplayers[myworld.current_player - 1].name,
            myplayers[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top]].player_id].name,
            mymonsters[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top]].id].name
        );
        history_add(histtext);

        return true;
    } else {
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Subversion was unsuccessfull resistance was greater than castroll %d > %d",
            magic_res,castroll)
        );

        sprintf(histtext,"%s Fails to Subvert %s's %s",
            myplayers[myworld.current_player - 1].name,
            myplayers[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top]].player_id].name,
            mymonsters[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top]].id].name
        );
        history_add(histtext);

        sprintf(infobar_text,"Failed");
        beepmsg = true;
        return false;
    }
}

/*
################################
#..lookupspellattribfunction().#
################################
*/
static bool lookupspellattribfunction(int spellid)
{
    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Looking up spell attribute spell %d",spellid));

    switch(mymagic_spell_attrib[spellid].assoc_func){
        case 1:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Found spell attribute spell [%d] function %d",spellid,
                mymagic_spell_attrib[spellid].assoc_func)
            );

            return spell_attrib_subversion();
        case 2:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Found spell attribute spell [%d] function %d",spellid,
                mymagic_spell_attrib[spellid].assoc_func)
            );

            return spell_attrib_raisedead();
        default:
            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "Failed to find spell attribute spell [%d] function %d",spellid,
                mymagic_spell_attrib[spellid].assoc_func)
            );
            break;
    }
    return false;
}

/*
########################
#..applymagicbalance().#
########################
*/
void mod_spell_attributes(int spellid)
{
    int layer_top = itemstack_top(myworld.cursor[0],myworld.cursor[1]);

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Applying Modifier %s",mymagic_spell_attrib[spellid].name));

    /*
    #########################
    #..Check.for.assoc_func.#
    #########################
    */
    if(mymagic_spell_attrib[spellid].assoc_func == 0 ||
        (mymagic_spell_attrib[spellid].assoc_func > 0 &&
        lookupspellattribfunction(spellid))){

        /*
        ##################
        #..change_alive?.#
        ##################
        */
        if(mymagic_spell_attrib[spellid].change_alive &&
            myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top]].dead){

            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Resurecting creature %s",
                mymonsters[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top]].id].name)
            );

            myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top]].dead = false;
            myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top]].beenmoved = true;
        }

        /*
        #################
        #..turn_undead?.#
        #################
        */
        if(mymagic_spell_attrib[spellid].turn_undead){
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Turning creature undead => %s",
                mymonsters[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top]].id].name)
            );

            myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top]].undead = true;
        }

        /*
        ##################
        #..change_owner?.#
        ##################
        */
        if(mymagic_spell_attrib[spellid].change_owner){
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Changing owner to %d",myworld.current_player-1));

            /* Subverted item might not be where cursor was */
            layer_top = itemstack_top(myworld.cursor[0],myworld.cursor[1]);

            myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][layer_top]].player_id =
                myworld.current_player-1;
        }
    } else {
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Assoc funciton returned false"));
    }
}
