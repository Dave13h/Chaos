/***************************************************************
*  blob.c                                          #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*                                                  #.....#     *
*  Blob functions                                  #######     *
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
#include "blob.h"

extern world                myworld;
extern player               myplayers[MAX_PLAYERS];
extern spells               myspells[MAX_SPELLS];
extern blob                 myblobs[MAX_BLOBS];

extern int arenas[MAX_ARENAS][2];
extern char log_message[LOGMSGLEN];

/*
#########################
#..lookupblobfunction().#
#########################
*/
void lookupblobfunction(int spellid)
{
    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Looking up blob spell %d",spellid));

    switch(myblobs[spellid].assoc_func){
        default:
            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "Failed to find special spell [%d] function %d",spellid,
                myblobs[spellid].assoc_func)
            );
            break;
    }
}

/*
################
#..grow_blob().#
################
*/
void grow_blob(int spellid)
{
    int p = myspells[spellid].player_id;
    int i;
    int top_layer, newspellid, growrand;
    int spawncursor[2];
    int worldcursor[2] = {myworld.cursor[0],myworld.cursor[1]};
    int startspawnpos = 0;
    int spawnpos[8][2] = {
        {-1,-1},
        {-1,0},
        {-1,1},
        {0,-1},
        /* Don't spawn on self*/
        {0,1},
        {1,-1},
        {1,0},
        {1,1}
    };

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Growing blob %d at %dx%d",spellid,
        myspells[spellid].current_pos[0],myspells[spellid].current_pos[1])
    );

    /* Pick a random direction to start loop from */
    startspawnpos = rand()%8;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Starting direction is %d [%dx%d]",startspawnpos,
        spawnpos[startspawnpos][0],spawnpos[startspawnpos][1]));

    /* Keep going until all posibilities are exhausted */
    for(i=0;i<8;i++){

        myworld.cursor[0] = spawncursor[0] = myspells[spellid].current_pos[0] + spawnpos[startspawnpos][0];
        myworld.cursor[1] = spawncursor[1] = myspells[spellid].current_pos[1] + spawnpos[startspawnpos][1];

        if((spawncursor[0] >= 0 && spawncursor[1] >=0) &&
            (spawncursor[0] < arenas[myworld.arenasize][0]) &&
            (spawncursor[1] < arenas[myworld.arenasize][1])){

            top_layer = itemstack_top(spawncursor[0],spawncursor[1]);

            /* Don't grow friendly blobs */
            if(myspells[myworld.layout[spawncursor[0]][spawncursor[1]][top_layer]].spell_type == SPELL_BLOB &&
                myspells[myworld.layout[spawncursor[0]][spawncursor[1]][top_layer]].player_id == p){
                console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
                    "Can't attack friendly Blobs spell at %dx%dx%d!",
                    spawncursor[0],spawncursor[1],top_layer));
                continue;
            }

            growrand = rand()%7+1;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Grow Blob Roll = %d (min needed %d)",growrand,BLOB_GROW_CHANCE));

            if(growrand >= BLOB_GROW_CHANCE){

                top_layer = itemstack_top(spawncursor[0],spawncursor[1]);

                if(myworld.layout[spawncursor[0]][spawncursor[1]][top_layer] > 0){
                    if(!myspells[myworld.layout[spawncursor[0]][spawncursor[1]][top_layer]].dead &&
                        myspells[myworld.layout[spawncursor[0]][spawncursor[1]][top_layer]].current_defense == 0){
                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Can't attack invulnerable spell at %dx%dx%d!",
                            spawncursor[0],spawncursor[1],top_layer));
                        continue;
                    }

                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                        "Found something to eat at %dx%dx%d! - spell %d",
                        spawncursor[0],spawncursor[1],top_layer,
                        myworld.layout[spawncursor[0]][spawncursor[1]][top_layer])
                    );

                    /* Go over corpses */
                    if(myspells[myworld.layout[spawncursor[0]][spawncursor[1]][top_layer]].dead){
                        top_layer++;

                    } else if(is_mounted(spawncursor[0],spawncursor[1]) || myspells[myworld.layout[spawncursor[0]][spawncursor[1]][top_layer]].spell_type == SPELL_PLAYER){
                        /* Grown over a Player! */
                        creature_death(myspells[myworld.layout[spawncursor[0]][spawncursor[1]][top_layer]].player_id+10, spawncursor[0], spawncursor[1]);

                        /* Did we eat our own player? */
                        if(myspells[myworld.layout[spawncursor[0]][spawncursor[1]][top_layer]].player_id == p)
                            return;

                    } else {
                        /* Don't eat, just sit on */
                        if(myblobs[myspells[spellid].id].devourer == 1){
                            top_layer++;
                        } else {
                            /* Grown over a creature */
                            creature_death(myworld.layout[spawncursor[0]][spawncursor[1]][top_layer], spawncursor[0], spawncursor[1]);
                            /* Check for corpses */
                            top_layer = itemstack_top(spawncursor[0],spawncursor[1]);
                        }
                    }
                }

                myworld.total_spells++;
                newspellid = myworld.total_spells+99;

                myspells[newspellid].id = myspells[spellid].id;
                myspells[newspellid].player_id = p;
                myspells[newspellid].spell_type = SPELL_BLOB;
                myspells[newspellid].undead = false;
                myspells[newspellid].current_defense = myblobs[myspells[spellid].id].defense;
                myspells[newspellid].dead = false;
                myspells[newspellid].uses = 0;

                if(myblobs[myspells[newspellid].id].turnlimit == -1) {
                    myspells[newspellid].turnlimit = rand()%ROUNDTURN_EXPIRE_RAND+1;
                } else {
                    myspells[newspellid].turnlimit = myblobs[myspells[newspellid].id].turnlimit;
                    if(myspells[newspellid].turnlimit > 0){ /* Add 1 as it will expire on turn 1 */
                        myspells[newspellid].turnlimit++;
                    }
                }

                myspells[newspellid].illusion = myspells[spellid].illusion;
                myspells[newspellid].beencast = true;
                myspells[newspellid].beenmoved = true;
                myspells[newspellid].genspells = false;

                myspells[newspellid].current_pos[0] = spawncursor[0];
                myspells[newspellid].current_pos[1] = spawncursor[1];
                myspells[newspellid].current_pos[2] = top_layer;
                myworld.layout[spawncursor[0]][spawncursor[1]][top_layer] = newspellid;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "New Child Blob at %dx%dx%d will expire in %d turns!",
                    myspells[newspellid].current_pos[0],myspells[newspellid].current_pos[1],
                    myspells[newspellid].current_pos[2],myspells[newspellid].turnlimit));

                /* Return world cursor */
                myworld.cursor[0] = worldcursor[0];
                myworld.cursor[1] = worldcursor[1];

                /* One successful blob offspring is enough for this blob this round */
                return;
            }
        }

        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
            "Unsuitible casting place for Blob at %dx%d (Out of Bounds)",
            spawncursor[0],spawncursor[1])
        );

        startspawnpos++;
        if(startspawnpos > 8)
            startspawnpos = 0;
    }

    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
        "Ran out of growth attempts (%d)",i)
    );

    /* Return world cursor */
    myworld.cursor[0] = worldcursor[0];
    myworld.cursor[1] = worldcursor[1];
}
