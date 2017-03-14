/***************************************************************
*  magic_upgrade.c                                 #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  magic upgrade functions/routines                #.....#     *
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
#include "magic_upgrade.h"
#include "sound_common.h"

extern world                myworld;
extern spells               myspells[MAX_SPELLS];
extern player               myplayers[MAX_PLAYERS];
extern magic_upgrade        mymagic_upgrade[MAX_MAGIC_UPGRADE];
extern char log_message[LOGMSGLEN];

/*
#######################
#..unstack_upgrades().#
#######################
*/
static void unstack_upgrades (int playerid)
{
    int max_move_range = 0;
    int u;

    for(u=0;u<MAX_MAGIC_UPGRADE;u++){
        if(myplayers[playerid].upgrades[u] > 0){
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Checking move_range for spell %d - %d",
                myplayers[playerid].upgrades[u],
                mymagic_upgrade[myplayers[playerid].upgrades[u]].move_range));

            if(mymagic_upgrade[myplayers[playerid].upgrades[u]].move_range > max_move_range)
                max_move_range = mymagic_upgrade[myplayers[playerid].upgrades[u]].move_range;
        }
    }

    max_move_range++;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Unstacking Player.moverange. Was : %d - Now : %d",
            myplayers[playerid].move_range,max_move_range));

    myplayers[playerid].move_range = max_move_range;
}

/*
########################
#..applymagicupgrade().#
########################
*/
void applymagicupgrade(int spellid)
{
    int p = myworld.current_player-1;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Applying upgrade %d to player %d",spellid,p));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "player attack was %d",myplayers[p].attack));

    if((myplayers[p].attack + mymagic_upgrade[spellid].attack) > 9){
        myplayers[p].attack = 9;
    } else if ((myplayers[p].attack + mymagic_upgrade[spellid].attack) < 0){
        myplayers[p].attack = 0;
    } else {
        myplayers[p].attack += mymagic_upgrade[spellid].attack;
    }
    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "player attack is now %d",myplayers[p].attack));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "player ranged_damage was %d",myplayers[p].ranged_damage));

    if((myplayers[p].ranged_damage + mymagic_upgrade[spellid].ranged_damage) > 9){
        myplayers[p].ranged_damage = 9;
    } else if ((myplayers[p].ranged_damage + mymagic_upgrade[spellid].ranged_damage) < 0){
        myplayers[p].ranged_damage = 0;
    } else {
        myplayers[p].ranged_damage += mymagic_upgrade[spellid].ranged_damage;
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "player ranged_damage is now %d",myplayers[p].ranged_damage));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "player ranged_range was %d",myplayers[p].ranged_range));

    if((myplayers[p].ranged_range + mymagic_upgrade[spellid].ranged_range) > 9){
        myplayers[p].ranged_range = 9;
    } else if ((myplayers[p].ranged_range + mymagic_upgrade[spellid].ranged_range) < 0){
        myplayers[p].ranged_range = 0;
    } else {
        myplayers[p].ranged_range += mymagic_upgrade[spellid].ranged_range;
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "player ranged_range is now %d",myplayers[p].ranged_range));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "player defense was %d",myplayers[p].defense));

    if((myplayers[p].defense + mymagic_upgrade[spellid].defense) > 9){
        myplayers[p].defense = 9;
    } else if ((myplayers[p].defense + mymagic_upgrade[spellid].defense) < 0){
        myplayers[p].defense = 0;
    } else {
        myplayers[p].defense += mymagic_upgrade[spellid].defense;
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "player defense is now %d",myplayers[p].defense));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "player move_range was %d",myplayers[p].move_range));

    if((myplayers[p].move_range + mymagic_upgrade[spellid].move_range) > 9){
        myplayers[p].move_range = 9;
    } else if ((myplayers[p].move_range + mymagic_upgrade[spellid].move_range) < 0){
        myplayers[p].move_range = 1;
    } else {
        myplayers[p].move_range += mymagic_upgrade[spellid].move_range;
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "player move_range is now %d",myplayers[p].move_range));

    if(mymagic_upgrade[spellid].flight){

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "player now has flight ability"));

        myplayers[p].flight = true;
    }

    if(mymagic_upgrade[spellid].attack_undead){

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "player now has attack undead ability"));

        myplayers[p].attack_undead = true;
    }

    unstack_upgrades(p);
}
