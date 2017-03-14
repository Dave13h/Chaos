/***************************************************************
*  wall.c                                          #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*                                                  #.....#     *
*  Wall functions                                  #######     *
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
#include "wall.h"

extern world                myworld;
extern player               myplayers[MAX_PLAYERS];
extern spells               myspells[MAX_SPELLS];
extern wall                 mywalls[MAX_WALLS];

extern int arenas[MAX_ARENAS][2];
extern char log_message[LOGMSGLEN];

/*
################
#..cast_wall().#
################
*/
static void cast_wall (void)
{
    int p = myworld.current_player - 1;
    int spellid = myplayers[p].spells[myplayers[p].selected_spell];
    int top_layer = itemstack_top(myworld.cursor[0],myworld.cursor[1]);
    int newspellid;

    /* Original wall has been placed, start spawning new walls */
    if(myspells[spellid].uses < mywalls[myspells[spellid].id].uses){
        myworld.total_spells++;
        newspellid = myworld.total_spells+99;

        myspells[newspellid].id = myspells[spellid].id;
        myspells[newspellid].player_id = myworld.current_player - 1;
        myspells[newspellid].spell_type = SPELL_WALL;
        myspells[newspellid].undead = myspells[spellid].undead;
        myspells[newspellid].current_defense = 0;
        myspells[newspellid].dead = false;
        myspells[newspellid].uses = 0;
        myspells[newspellid].illusion = myspells[spellid].illusion;
        myspells[newspellid].beencast = true;
        myspells[newspellid].beenmoved = true;
        myspells[newspellid].current_pos[0] = myworld.cursor[0];
        myspells[newspellid].current_pos[1] = myworld.cursor[1];
        myspells[newspellid].current_pos[2] = top_layer;
        myspells[newspellid].turnlimit = myspells[spellid].turnlimit;
        myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] = newspellid;

    /* Use the main wall spell as the first wall */
    } else {
        myspells[spellid].current_pos[0] = myworld.cursor[0];
        myspells[spellid].current_pos[1] = myworld.cursor[1];
        myspells[spellid].current_pos[2] = top_layer;
        myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] = spellid;
        myspells[spellid].beenmoved = true;
    }

    myspells[spellid].uses--;

    if(myspells[spellid].uses < 1){
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Out of turns!"));
        myspells[spellid].beencast = true;
    }
}

/*
#########################
#..lookupwallfunction().#
#########################
*/
void lookupwallfunction(int spellid)
{
    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Looking up wall spell %d",spellid));

    switch(mywalls[spellid].assoc_func){
        case 1:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Found wall spell [%d] function %d",spellid,mywalls[spellid].assoc_func));

            cast_wall();
            break;
        default:
            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "Failed to find special spell [%d] function %d",spellid,
                mywalls[spellid].assoc_func)
            );
            break;
    }
}
