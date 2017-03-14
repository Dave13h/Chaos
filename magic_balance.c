/***************************************************************
*  magic_balance.c                                 #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  magic balance functions/routines                #.....#     *
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
#include "magic_balance.h"

extern world                myworld;
extern player               myplayers[MAX_PLAYERS];
extern spells               myspells[MAX_SPELLS];
extern magic_balance        mymagic_balance[MAX_MAGIC_BALANCE];

extern char log_message[LOGMSGLEN];

/*
########################
#..applymagicbalance().#
########################
*/
void applymagicbalance(int spellid)
{
    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Enabling balance modifier %s",mymagic_balance[spellid].name));
}
