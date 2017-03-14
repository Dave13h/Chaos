/***************************************************************
*  magic_special.c                                 #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  special magic functions/routines                #.....#     *
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
#include "history.h"
#include "log.h"
#include "chaos.h"
#include "magic_special.h"
#include "sound_common.h"

extern world                myworld;
extern player               myplayers[MAX_PLAYERS];
extern spells               myspells[MAX_SPELLS];
extern magic_special        mymagic_special[MAX_MAGIC_SPECIAL];
extern monster              mymonsters[MAX_MONSTERS];

extern char log_message[LOGMSGLEN];

/*
#########################
#..special_disbelieve().#    // assoc_func == 1
#########################
*/
static void special_disbelieve(void)
{
    int top_layer = itemstack_top(myworld.cursor[0],myworld.cursor[1]);
    char histtext[255];

    memset(histtext,'\0',sizeof(histtext));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
        log_message,"Do disbelieve stuff"));

    if(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] > 0 &&
        myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][0]].player_id != (myworld.current_player-1)
        && !myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][0]].dead){

        sprintf(histtext,"%s Casts Disbelieve", myplayers[myworld.current_player - 1].name);
        history_add(histtext);

        memset(histtext,'\0',sizeof(histtext));

        if(myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].illusion){

            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                log_message,"Creature is an illusion"));

            sprintf(histtext,"%s's %s was an illusion!",
                myplayers[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][0]].player_id].name,
                mymonsters[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][0]].id].name);
            history_add(histtext);

            creature_death(
                myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer],0,0);
        } else {
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                log_message,"Creature is not an illusion"));

            sprintf(histtext,"%s's %s is not an illusion!",
                myplayers[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][0]].player_id].name,
                mymonsters[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][0]].id].name);
            history_add(histtext);
        }

    } else {
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "No creature under cursor, or player is selecting his own creature"));
        CE_beep();
    }
}

/*
############################
#..lookupspecialfunction().#
############################
*/
void lookupspecialfunction(int spellid)
{
    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Looking up special spell %d",spellid));

    switch(mymagic_special[spellid].assoc_func){
        case 1:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Found special spell [%d] function %d",spellid,
                mymagic_special[spellid].assoc_func)
            );
            special_disbelieve();
            break;
        default:
            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "Failed to find special spell [%d] function %d",spellid,
                mymagic_special[spellid].assoc_func)
            );
            break;
    }
}
