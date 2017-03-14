/***************************************************************
*  magic_ranged.c                                  #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  magic ranged functions/routines                 #.....#     *
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
#include "magic_ranged.h"
#include "sound_common.h"

extern world                myworld;
extern player               myplayers[MAX_PLAYERS];
extern spells               myspells[MAX_SPELLS];
extern monster              mymonsters[MAX_MONSTERS];
extern magic_ranged         mymagic_ranged[MAX_MAGIC_RANGED];

extern char log_message[LOGMSGLEN];

extern char infobar_text[255];
extern bool beepmsg;

extern int arenas[MAX_ARENAS][2];

/*
##############################
#..spell_ranged_justice().#
##############################
*/
static bool spell_ranged_justice(void){
    int magic_res, castroll,creature,s;
    int top_layer = itemstack_top(myworld.cursor[0],myworld.cursor[1]);
    int itemid = myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].id;
    int targplayer = myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]-10;

    if(myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].spell_type == SPELL_PLAYER){
        magic_res = myplayers[itemid].magic_resistance;
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Player %d's Magic resistance is %d",itemid,magic_res));
    } else {
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Cant Justice a non-player item [type was %d]",
            myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].spell_type)
        );
        return false;
    }

    srand(time(NULL));
    castroll = rand()%9;
    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Casting roll was %d",castroll));

    if(castroll >= magic_res){

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Justice was successfull resistance was less than or equal to castroll %d < %d",
            magic_res,castroll)
        );

        for (s=1; s<myplayers[targplayer].total_spells+1; s++){
            creature = myplayers[targplayer].spells[s];
            if(myspells[creature].beencast && !myspells[creature].dead &&
                myspells[creature].spell_type == SPELL_MONSTER){

                console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
                    "Unsetting creature (%s)[spellid %d] at %dx%d layer : %d",
                    mymonsters[myspells[creature].id].name,creature,
                    myspells[creature].current_pos[0],
                    myspells[creature].current_pos[1],
                    myspells[creature].current_pos[2])
                );

                myworld.layout[myspells[creature].current_pos[0]][myspells[creature].current_pos[1]][myspells[creature].current_pos[2]] = 0;

                /* Item above corpse ? */
                if(myspells[creature].dead &&
                    myworld.layout[myspells[creature].current_pos[0]][myspells[creature].current_pos[1]][myspells[creature].current_pos[2]+1] > 0){

                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                        "Item (%d) above old Spell (%d) at %dx%d layer : %d",
                        myworld.layout[myspells[creature].current_pos[0]][myspells[creature].current_pos[1]][myspells[creature].current_pos[2]+1],
                        s,
                        myspells[creature].current_pos[0],
                        myspells[creature].current_pos[1],
                        myspells[creature].current_pos[2])
                    );

                    /* Move top creature down */
                    myworld.layout[myspells[creature].current_pos[0]][myspells[creature].current_pos[1]][myspells[creature].current_pos[2]] =
                        myworld.layout[myspells[creature].current_pos[0]][myspells[creature].current_pos[1]][myspells[creature].current_pos[2]+1];

                    /* Update spells pos*/
                    myspells[myworld.layout[myspells[creature].current_pos[0]][myspells[creature].current_pos[1]][myspells[creature].current_pos[2]+1]].current_pos[2]--;

                    /* Unset old Pos */
                    myworld.layout[myspells[creature].current_pos[0]][myspells[creature].current_pos[1]][myspells[creature].current_pos[2]+1] = 0;

                    /* There is a mounted monster above? */
                    if(myworld.layout[myspells[creature].current_pos[0]][myspells[creature].current_pos[1]][2] > 0){

                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Mounted Item (%d) above old Spell (%d) at %dx%d layer : %d",
                            myworld.layout[myspells[creature].current_pos[0]][myspells[creature].current_pos[1]][2],
                            s,
                            myspells[creature].current_pos[0],
                            myspells[creature].current_pos[1],
                            myspells[creature].current_pos[2])
                        );

                        /* Move top creature down */
                        myworld.layout[myspells[creature].current_pos[0]][myspells[creature].current_pos[1]][1] =
                            myworld.layout[myspells[creature].current_pos[0]][myspells[creature].current_pos[1]][2];

                        /* Update spells pos*/
                        myspells[myworld.layout[myspells[creature].current_pos[0]][myspells[creature].current_pos[1]][2]].current_pos[2]--;

                        /* Unset old Pos */
                        myworld.layout[myspells[creature].current_pos[0]][myspells[creature].current_pos[1]][2] = 0;

                    }
                }

                myspells[creature].dead = true;

            } else {
                console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
                    "Uncast creature (%s)[spellid %d]",
                    mymonsters[myspells[creature].id].name,creature)
                );
            }
        }

        sprintf(infobar_text,"Success");
        beepmsg = true;

        return true;
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Justice was unsuccessfull resistance was greater than castroll %d > %d",
        magic_res,castroll)
    );
    sprintf(infobar_text,"Failed");
    beepmsg = true;
    return false;
}

/*
#############################
#..spell_ranged_vengeance().#
#############################
*/
static bool spell_ranged_vengeance(void){

    int top_layer = itemstack_top(myworld.cursor[0],myworld.cursor[1]);
    int itemid = myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer];
    int last_killed = myspells[itemid].last_killed;
    int cur_x = myworld.cursor[0] - 1;
    int cur_y = myworld.cursor[1] - 1;
    int i,j,um_top_layer;

    if((myspells[itemid].spell_type == SPELL_MONSTER || myspells[itemid].spell_type == SPELL_TREE ||
        myspells[itemid].spell_type == SPELL_BLOB)&& !myspells[itemid].dead){

        if(last_killed > 0 && !myspells[myspells[last_killed].player_id].dead
             && !myspells[myspells[last_killed].player_id].illusion){

            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Resurecting creatures last killed item : %d",last_killed));

            /* resurect last killed creature */
            myspells[last_killed].dead = false;
            myspells[last_killed].beenmoved = false;

            /* remove old corpse if it exists */
            if (myworld.layout[myspells[last_killed].current_pos[0]][myspells[last_killed].current_pos[1]][myspells[last_killed].current_pos[2]] == last_killed){
                myworld.layout[myspells[last_killed].current_pos[0]][myspells[last_killed].current_pos[1]][myspells[last_killed].current_pos[2]] = 0;

                /* Item above corpse ? */
                if(myspells[last_killed].dead &&
                    myworld.layout[myspells[last_killed].current_pos[0]][myspells[last_killed].current_pos[1]][myspells[last_killed].current_pos[2]+1] > 0){

                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                        "Item (%d) above old Spell (%d) at %dx%d layer : %d",
                        myworld.layout[myspells[last_killed].current_pos[0]][myspells[last_killed].current_pos[1]][myspells[last_killed].current_pos[2]+1],
                        last_killed,
                        myspells[last_killed].current_pos[0],
                        myspells[last_killed].current_pos[1],
                        myspells[last_killed].current_pos[2])
                    );

                    /* Move top creature down */
                    myworld.layout[myspells[last_killed].current_pos[0]][myspells[last_killed].current_pos[1]][myspells[last_killed].current_pos[2]] =
                        myworld.layout[myspells[last_killed].current_pos[0]][myspells[last_killed].current_pos[1]][myspells[last_killed].current_pos[2]+1];

                    /* Update spells pos*/
                    myspells[myworld.layout[myspells[last_killed].current_pos[0]][myspells[last_killed].current_pos[1]][myspells[last_killed].current_pos[2]+1]].current_pos[2]--;

                    /* Unset old Pos */
                    myworld.layout[myspells[last_killed].current_pos[0]][myspells[last_killed].current_pos[1]][myspells[last_killed].current_pos[2]+1] = 0;

                    /* There is a mounted monster above? */
                    if(myworld.layout[myspells[last_killed].current_pos[0]][myspells[last_killed].current_pos[1]][2] > 0){

                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Mounted Item (%d) above old Spell (%d) at %dx%d layer : %d",
                            myworld.layout[myspells[last_killed].current_pos[0]][myspells[last_killed].current_pos[1]][2],
                            last_killed,
                            myspells[last_killed].current_pos[0],
                            myspells[last_killed].current_pos[1],
                            myspells[last_killed].current_pos[2])
                        );

                        /* Move top creature down */
                        myworld.layout[myspells[last_killed].current_pos[0]][myspells[last_killed].current_pos[1]][1] =
                            myworld.layout[myspells[last_killed].current_pos[0]][myspells[last_killed].current_pos[1]][2];

                        /* Update spells pos*/
                        myspells[myworld.layout[myspells[last_killed].current_pos[0]][myspells[last_killed].current_pos[1]][2]].current_pos[2]--;

                        /* Unset old Pos */
                        myworld.layout[myspells[last_killed].current_pos[0]][myspells[last_killed].current_pos[1]][2] = 0;

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
                                            "Found suitible position for moving resurected creature @ %dx%dx%d",
                                            cur_x+i,cur_y+j,um_top_layer)
                                        );

                                        /* Move Cursor to new position for 'change owner' */
                                        myworld.cursor[0] = cur_x+i;
                                        myworld.cursor[1] = cur_y+j;

                                    }
                                }
                            }
                        }
                    }
                }
            }

            console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
                "Victim Creatures Previous pos was %dx%d%d",
                myspells[itemid].current_pos[0],
                myspells[itemid].current_pos[1],
                myspells[itemid].current_pos[2])
            );

            /* Was target creature standing on resurected creature? */
            if(top_layer > 0 && myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer-1] == 0)
                top_layer--;

            /* remove target creature */
            myspells[itemid].dead = true;
            myspells[itemid].illusion = true; /* oi oi cheeky! hide it from view =P */
            myworld.layout[myspells[itemid].current_pos[0]][myspells[itemid].current_pos[1]][myspells[itemid].current_pos[2]] = 0;
            myspells[itemid].current_pos[0] = 0;
            myspells[itemid].current_pos[1] = 0;
            myspells[itemid].current_pos[2] = 0;

            /* set resurected creatures new positions */
            myspells[last_killed].current_pos[0] = myworld.cursor[0];
            myspells[last_killed].current_pos[1] = myworld.cursor[1];
            myspells[last_killed].current_pos[2] = top_layer;
            myspells[last_killed].player_id = myworld.current_player-1;
            myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] = last_killed;
            itemid = last_killed;

            console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
                "Resurected Creatures new pos is %dx%dx%d",
                myspells[last_killed].current_pos[0],
                myspells[last_killed].current_pos[1],
                myspells[last_killed].current_pos[2]
                ));

            sprintf(infobar_text,"Success");
            beepmsg = true;
            return true;

        } else {

            sprintf(infobar_text,"Creature hasn't killed or last creatures owner is dead");
            beepmsg = true;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Creature hasnt killed yet or last killed creatures owner is dead, wasted spell"));
        }
    } else {
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Using Vengeance against player or dead creature"));
    }

    sprintf(infobar_text,"Failed");
    beepmsg = true;
    return false;
}

/*
################################
#..lookuprangedspellfunction().#
################################
*/
static bool lookuprangedspellfunction(int spellid)
{
    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
        log_message,"Looking up spell ranged spell %d",spellid));

    switch(mymagic_ranged[spellid].assoc_func){
        case 2:

            console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                log_message,"Found spell ranged spell [%d] function %d",
                spellid,mymagic_ranged[spellid].assoc_func)
            );

            return spell_ranged_justice();
        case 3:

            console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
                "Found spell ranged spell [%d] function %d",spellid,
                mymagic_ranged[spellid].assoc_func)
            );

            return spell_ranged_vengeance();
        default:

            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "Failed to find spell ranged spell [%d] function %d",spellid,
                mymagic_ranged[spellid].assoc_func)
            );
            break;
    }
    return false;
}

/*
#######################
#..castmagic_ranged().#
#######################
*/
void castmagic_ranged(int spellid)
{
    int top_layer = itemstack_top(myworld.cursor[0],myworld.cursor[1]);
    int attack = 0;

    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
        "Casting magic ranged spell : %s",mymagic_ranged[spellid].name));

    /*
    #########################
    #..Check.for.assoc_func.#
    #########################
    */
    if(mymagic_ranged[spellid].assoc_func < 2 ||
        (mymagic_ranged[spellid].assoc_func > 1 &&
        lookuprangedspellfunction(spellid))){

        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
            log_message,"Spell Successful"));

        /* You can't kill what cannot be hurt! */
        if(myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].current_defense == 0){
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Unsuccessful attack Target is invincible or was just resurected!")
            );
            return;
        }

        if(mymagic_ranged[spellid].ranged_damage > 0){

            if(mymagic_ranged[spellid].assoc_func == 1){
                srand(time(NULL));
                attack = rand()%mymagic_ranged[spellid].ranged_damage;
            } else if(mymagic_ranged[spellid].assoc_func == 3 &&
                myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].spell_type == SPELL_MONSTER){

                attack = 0;

            } else {
                attack = mymagic_ranged[spellid].ranged_damage;
            }

            if(attack >= myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].current_defense){

                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Successful attack (%d vs %d)",attack,
                    myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].current_defense)
                );

                creature_death(
                    myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer],
                    myspells[myworld.current_player+9].current_pos[0],
                    myspells[myworld.current_player+9].current_pos[1]
                );

            } else {
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Unsuccessful attack (%d vs %d)",attack,
                    myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].current_defense)
                );

                myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].current_defense -= attack;
            }

        }
    } else {
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
            log_message,"Unsuccessful"));
    }
}
