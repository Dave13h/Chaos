/***************************************************************
*  tree.c                                          #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*                                                  #.....#     *
*  Tree functions                                  #######     *
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
#include "tree.h"

extern world  myworld;
extern player myplayers[MAX_PLAYERS];
extern spells myspells[MAX_SPELLS];
extern tree   mytrees[MAX_TREES];

extern int arenas[MAX_ARENAS][2];
extern char log_message[LOGMSGLEN];

/*
######################
#..cast_magic_tree().#
######################
*/
static void cast_magic_tree (void)
{
    int p = myworld.current_player - 1;
    int spellid = myplayers[p].spells[myplayers[p].selected_spell];
    int i = 1;
    int xpos, ypos;
    int top_layer, newspellid;
    int spawncursor[2];
    int spawndist = 2;
    int worldcursor[2] = {myworld.cursor[0],myworld.cursor[1]};
    char histtext[255];

    memset(histtext,'\0',sizeof(histtext));

    /* Circle outwards from the player spawning where 'possible' */
    for(spawndist=2;spawndist<mytrees[myspells[spellid].id].casting_range+1;spawndist++){

        for(ypos=-spawndist;ypos<spawndist+1;ypos++){
            for(xpos=-spawndist;xpos<spawndist+1;xpos++){

                myworld.cursor[0] = spawncursor[0] = myspells[p+10].current_pos[0] + xpos;
                myworld.cursor[1] = spawncursor[1] = myspells[p+10].current_pos[1] + ypos;

                if((spawncursor[0] >= 0 && spawncursor[1] >=0) &&
                    (spawncursor[0] < arenas[myworld.arenasize][0]) &&
                    (spawncursor[1] < arenas[myworld.arenasize][1])){

                    top_layer = itemstack_top(spawncursor[0],spawncursor[1]);

                    if(myworld.layout[spawncursor[0]][spawncursor[1]][top_layer] == 0 &&
                        !checkadjacent_any(spawncursor[0]-1,spawncursor[1]-1) && checklos()){

                        /* Original tree has been placed, start spawning new tress */
                        if(myspells[spellid].uses < mytrees[myspells[spellid].id].uses){
                            myworld.total_spells++;
                            newspellid = myworld.total_spells+99;

                            myspells[newspellid].id = myspells[spellid].id;
                            myspells[newspellid].player_id = myworld.current_player - 1;
                            myspells[newspellid].spell_type = SPELL_TREE;
                            myspells[newspellid].undead = myspells[spellid].undead;
                            myspells[newspellid].current_defense = mytrees[myspells[spellid].id].defense;
                            myspells[newspellid].dead = false;
                            myspells[newspellid].uses = 0;
                            if(mytrees[myspells[newspellid].id].turnlimit == -1) {
                                myspells[newspellid].turnlimit = rand()%ROUNDTURN_EXPIRE_RAND+1;
                            } else {
                                myspells[newspellid].turnlimit = mytrees[myspells[newspellid].id].turnlimit;
                                if(myspells[newspellid].turnlimit > 0){ /* Add 1 as it will expire on turn 1 */
                                    myspells[newspellid].turnlimit++;
                                }
                            }
                            myspells[newspellid].illusion = myspells[spellid].illusion;
                            myspells[newspellid].beencast = true;
                            myspells[newspellid].beenmoved = false;

                            myspells[newspellid].genspells = mytrees[myspells[spellid].id].genspells;

                            myspells[newspellid].current_pos[0] = spawncursor[0];
                            myspells[newspellid].current_pos[1] = spawncursor[1];
                            myspells[newspellid].current_pos[2] = top_layer;
                            myworld.layout[spawncursor[0]][spawncursor[1]][top_layer] = newspellid;

                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                                "New Child Tree will expire in %d turns!",myspells[newspellid].turnlimit));

                        /* Use the main tree spell as the first tree */
                        } else {
                            myspells[spellid].current_pos[0] = spawncursor[0];
                            myspells[spellid].current_pos[1] = spawncursor[1];
                            myspells[spellid].current_pos[2] = top_layer;

                            myworld.layout[spawncursor[0]][spawncursor[1]][top_layer] = spellid;

                            sprintf(histtext,"%s Successfully Casts %s",
                                myplayers[p].name,mytrees[myspells[spellid].id].name);
                            history_add(histtext);
                        }

                        myspells[spellid].uses--;

                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Found suitible casting place for Tree %d at %dx%d",i++,
                            spawncursor[0],spawncursor[1])
                        );

                        if(myspells[spellid].uses < 1)
                            goto outofturns; /* XXX: Dirty =( */

                    } else {
                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Unsuitible casting place for Tree %d at %dx%d (Blocking Item)",
                            i,spawncursor[0],spawncursor[1])
                        );
                    }
                } else {
                    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
                        "Unsuitible casting place for Tree %d at %dx%d (Out of Bounds)",
                        i,spawncursor[0],spawncursor[1])
                    );
                }
            }
        }
    }

    outofturns:

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Out of turns! Wasted %d Trees",myspells[spellid].uses));

    /* Force uses to 0 as if there are any left by now they are unpossible spawns */
    myspells[spellid].uses = 0;
    myspells[spellid].beencast = true;

    /* Return world cursor */
    myworld.cursor[0] = worldcursor[0];
    myworld.cursor[1] = worldcursor[1];
}

/*
#######################
#..cast_shadow_tree().#
#######################
*/
static void cast_shadow_tree (void)
{
    int p = myworld.current_player - 1;
    int spellid = myplayers[p].spells[myplayers[p].selected_spell];
    int top_layer = itemstack_top(myworld.cursor[0],myworld.cursor[1]);
    int newspellid;
    char histtext[255];

    memset(histtext,'\0',sizeof(histtext));

    /* Original tree has been placed, start spawning new trees */
    if(myspells[spellid].uses < mytrees[myspells[spellid].id].uses){
        myworld.total_spells++;
        newspellid = myworld.total_spells+99;

        myspells[newspellid].id = myspells[spellid].id;
        myspells[newspellid].player_id = myworld.current_player - 1;
        myspells[newspellid].spell_type = SPELL_TREE;
        myspells[newspellid].undead = myspells[spellid].undead;
        myspells[newspellid].current_defense = 0;
        myspells[newspellid].dead = false;
        if(mytrees[myspells[newspellid].id].turnlimit == -1) {
            myspells[newspellid].turnlimit = rand()%ROUNDTURN_EXPIRE_RAND+1;
        } else {
            myspells[newspellid].turnlimit = mytrees[myspells[newspellid].id].turnlimit;
            if(myspells[newspellid].turnlimit > 0) /* Add 1 as it will expire on turn 1 */
                myspells[newspellid].turnlimit++;
        }
        myspells[newspellid].uses = 0;
        myspells[newspellid].illusion = myspells[spellid].illusion;
        myspells[newspellid].beencast = true;
        myspells[newspellid].beenmoved = false;
        myspells[newspellid].current_pos[0] = myworld.cursor[0];
        myspells[newspellid].current_pos[1] = myworld.cursor[1];
        myspells[newspellid].current_pos[2] = top_layer;
        myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] = newspellid;

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "New Child Tree will expire in %d turns!",myspells[newspellid].turnlimit));

    /* Use the main tree spell as the first tree */
    } else {
        myspells[spellid].current_pos[0] = myworld.cursor[0];
        myspells[spellid].current_pos[1] = myworld.cursor[1];
        myspells[spellid].current_pos[2] = top_layer;
        myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] = spellid;

        sprintf(histtext,"%s Successfully Casts %s",
            myplayers[p].name,mytrees[myspells[spellid].id].name);
        history_add(histtext);
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
#..lookuptreefunction().#
#########################
*/
void lookuptreefunction(int spellid)
{
    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Looking up tree spell %d",spellid));

    switch(mytrees[spellid].assoc_func){
        case 1:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Found tree spell [%d] function %d",spellid,mytrees[spellid].assoc_func));

            cast_magic_tree();
            break;
        case 2:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Found tree spell [%d] function %d",spellid,mytrees[spellid].assoc_func));

            cast_shadow_tree();
            break;
        default:
            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "Failed to find special spell [%d] function %d",spellid,
                mytrees[spellid].assoc_func)
            );
            break;
    }
}
