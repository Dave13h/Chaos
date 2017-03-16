/***************************************************************
*  chaos.c                                         #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*                                                  #.....#     *
*  Game Code                                       #######     *
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
#include "chaos.h"
#include "log.h"
#include "history.h"
#include "timer.h"
#include "generate.h"
#include "input_common.h"
#include "display_common.h"
#include "sound_common.h"
#include "tree.h"
#include "wall.h"
#include "blob.h"
#include "magic_special.h"
#include "magic_upgrade.h"
#include "magic_balance.h"
#include "magic_spell_attrib.h"
#include "magic_ranged.h"

extern int frontend_mode;
extern bool forceupdate;

extern int logging_level;
extern char log_message[LOGMSGLEN];

extern bool beepmsg;
extern char infobar_text[255];

#ifdef WITH_NET
#include "net.h"
#include "net_server.h"
extern bool net_enable;
extern bool net_dedicated;
extern int net_port;
extern bool net_wait;
#endif

world                   myworld;
player                  myplayers[MAX_PLAYERS];
spells                  myspells[MAX_SPELLS];
monster                 mymonsters[MAX_MONSTERS];
tree                    mytrees[MAX_TREES];
wall                    mywalls[MAX_WALLS];
blob                    myblobs[MAX_BLOBS];
magic_special           mymagic_special[MAX_MAGIC_SPECIAL];
magic_upgrade           mymagic_upgrade[MAX_MAGIC_UPGRADE];
magic_balance           mymagic_balance[MAX_MAGIC_BALANCE];
magic_spell_attrib      mymagic_spell_attrib[MAX_MAGIC_SPELL_ATTRIB];
magic_ranged            mymagic_ranged[MAX_MAGIC_RANGED];

extern int arenas[MAX_ARENAS][2];

extern int monsters_count;
extern int trees_count;
extern int walls_count;
extern int blobs_count;
extern int magic_special_count;
extern int magic_upgrade_count;
extern int magic_balance_count;
extern int magic_spell_attrib_count;
extern int magic_ranged_count;

extern history_log myhistory[MAX_HISTORY];
extern chat_log mychat[MAX_CHAT];

extern unsigned short int highlight_arena[MAX_X][MAX_Y];
extern bool view_highlight_arena;

/*
========================================================================= Arena =
*/

/*
####################
#..itemstack_top().#
####################
*/
int itemstack_top (int x, int y)
{
    int s;
    for(s=MAX_LAYERS-1;s>0;s--){
        if(myworld.layout[x][y][s] > 0){
            console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
                "Top layer is %d => itemid %d",s,myworld.layout[x][y][s]));
            return s;
        }
    }
    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
        "Top layer is %d => itemid %d",s,myworld.layout[x][y][s]));
    return s;
}

/*
####################
#..checkdistance().#
####################
*/
int checkdistance (void)
{
    int start_x;
    int start_y;
    int dest_x = myworld.cursor[0];
    int dest_y = myworld.cursor[1];
    int x,y,totaldist;

    if(myworld.mode == 2){
        start_x = myspells[myworld.current_player+9].current_pos[0];
        start_y = myspells[myworld.current_player+9].current_pos[1];
    } else {
        start_x = myspells[myworld.selected_item[0]].current_pos[0];
        start_y = myspells[myworld.selected_item[0]].current_pos[1];
    }

    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
        "Start_x = %d / Start_y = %d / Dest_x = %d / Dest_y = %d",
            start_x,start_y,dest_x,dest_y)
    );

    x = abs(start_x - dest_x);
    y = abs(start_y - dest_y);

    x = x * x;
    y = y * y;

    totaldist = sqrt(x+y);

    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
        "distance = %d",totaldist));

    return (totaldist);
}

#ifdef NEWLOS

/*
###############
#..checklos().#
###############
*/
bool checklos (void)
{
    int start_x;
    int start_y;
    int dest_x = myworld.cursor[0];
    int dest_y = myworld.cursor[1];
    int x,y,i,j,ratio,top_layer;
    int ratiotmp = 0;
    int swap;
    bool blocked = false;
    bool loshack = false;

    clear_highlights();

    if(myworld.mode == 2){
        start_x = myspells[myworld.current_player+9].current_pos[0];
        start_y = myspells[myworld.current_player+9].current_pos[1];
    } else {
        start_x = myspells[myworld.selected_item[0]].current_pos[0];
        start_y = myspells[myworld.selected_item[0]].current_pos[1];
    }

    LOSHACK:

    x = abs(start_x - dest_x);
    y = abs(start_y - dest_y);

    if(y == 0 || x == 0){
        ratio = 0;
    } else {
        if(x>y){
            ratio = abs(x / y);
        }else{
            ratio = abs(y / x);
        }
    }

    console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
        "start x = %d / start y = %d / dest x = %d / dest y = %d / dist x = %d"
        " / dist y = %d / ratio = %d",start_x,start_y,dest_x,dest_y,x,y,ratio));

    j = start_y;

    if(start_x < dest_x){   /* x dir is down */

        console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
                "Direction is down"));

        for(i=start_x+1;i<dest_x;i++){

            if(start_y < dest_y && ratiotmp == ratio){
                j++;
                ratiotmp = 0;
            } else if(start_y > dest_y && ratiotmp == ratio){
                j--;
                ratiotmp = 0;
            }

            top_layer = itemstack_top(i,j);
            highlight_arena[i][j]=true;

            console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
                "Checking for item at %dx%d layer %d",i,j,top_layer));

            if(myworld.layout[i][j][top_layer] > 0 &&
                !isspelldead(myworld.layout[i][j][top_layer])){

                console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
                    "Found blocking item %d at %dx%d layer %d",
                    myworld.layout[i][j][top_layer],i,j,top_layer)
                );

                blocked = true;
            }
            ratiotmp++;
        }


    } else if(start_x > dest_x){ /* x dir is up */

        console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
                "Direction is up"));

        for(i=start_x-1;i>dest_x;i--){

            if(start_y < dest_y && ratiotmp == ratio){
                j++;
                ratiotmp = 0;
            } else if(start_y > dest_y && ratiotmp == ratio){
                j--;
                ratiotmp = 0;
            }

            top_layer = itemstack_top(i,j);
            highlight_arena[i][j]=true;

            console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
                "Checking for item at %dx%d layer %d",i,j,top_layer));

            if(myworld.layout[i][j][top_layer] > 0 &&
                !isspelldead(myworld.layout[i][j][top_layer])){

                console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
                    "Found blocking item %d at %dx%d layer %d",
                    myworld.layout[i][j][top_layer],i,j,top_layer)
                );
                blocked = true;
            }
            ratiotmp++;
        }

    } else {    /* x dir doesnt change */

        i = start_x;

        if(start_y < dest_y){   /* y dir is right */

            console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
                "Direction is right"));

            for(j=start_y+1;j<dest_y;j++){

                top_layer = itemstack_top(i,j);
                highlight_arena[i][j]=true;

                console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
                    "Checking for item at %dx%d layer %d",i,j,top_layer));

                if(myworld.layout[i][j][top_layer] > 0 &&
                    !isspelldead(myworld.layout[i][j][top_layer])){

                    console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
                        "Found blocking item %d at %dx%d layer %d",
                        myworld.layout[i][j][top_layer],i,j,top_layer)
                    );

                    blocked = true;
                }
            }

        } else if (start_y > dest_y){   /* y dir is left */

            console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
                "Direction is left"));

            for(j=start_y-1;j>dest_y;j--){

                top_layer = itemstack_top(i,j);
                highlight_arena[i][j]=true;

                console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
                    "Checking for item at %dx%d layer %d",i,j,top_layer));

                if(myworld.layout[i][j][top_layer] > 0 &&
                    !isspelldead(myworld.layout[i][j][top_layer])){

                    console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
                        "Found blocking item %d at %dx%d layer %d",
                        myworld.layout[i][j][top_layer],i,j,top_layer)
                    );

                    blocked = true;
                }
            }
        }
    }

    /* OH NOES, WE ARE GOING BACKWARDZ! */
    if(blocked && !loshack){
        console_log(__FILE__,__func__,__LINE__,LOG_ALL,
            sprintf(log_message,"Reverse LOS direction!"));

        swap=start_x;
        start_x=dest_x;
        dest_x=swap;

        swap=start_y;
        start_y=dest_y;
        dest_y=swap;

        blocked = false;
        loshack = true;
        goto LOSHACK;
    }

    return !blocked;
}

#else

/*
###############
#..checklos().#
###############
*/
bool checklos (void)
{
    int start_x;
    int start_y;
    int dest_x = myworld.cursor[0];
    int dest_y = myworld.cursor[1];
    int x,y,i,j,ratio,top_layer;
    int ratiotmp = 0;
    bool blocked = false;

    clear_highlights();

    if(myworld.mode == 2){
        start_x = myspells[myworld.current_player+9].current_pos[0];
        start_y = myspells[myworld.current_player+9].current_pos[1];
    } else {
        start_x = myspells[myworld.selected_item[0]].current_pos[0];
        start_y = myspells[myworld.selected_item[0]].current_pos[1];
    }

    x = abs(start_x - dest_x);
    y = abs(start_y - dest_y);

    if(y == 0 || x == 0){
        ratio = 0;
    } else {
        if(x>y){
            ratio = abs(x / y);
        }else{
            ratio = abs(y / x);
        }
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "start x = %d / start y = %d / dest x = %d / dest y = %d / dist x = %d"
        " / dist y = %d / ratio = %d",
        start_x,start_y,dest_x,dest_y,x,y,ratio)
    );

    j = start_y;

    if(start_x < dest_x){   /* x dir is down */

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Direction is down"));

        for(i=start_x+1;i<dest_x;i++){

            if(start_y < dest_y && ratiotmp == ratio){
                j++;
                ratiotmp = 0;
            } else if(start_y > dest_y && ratiotmp == ratio){
                j--;
                ratiotmp = 0;
            }

            top_layer = itemstack_top(i,j);
            highlight_arena[i][j]=true;

            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Checking for item at %dx%d layer %d",i,j,top_layer));

            if(myworld.layout[i][j][top_layer] > 0 &&
                !isspelldead(myworld.layout[i][j][top_layer])){

                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Found blocking item %d at %dx%d layer %d",
                    myworld.layout[i][j][top_layer],i,j,top_layer)
                );

                blocked = true;
            }
            ratiotmp++;
        }


    } else if(start_x > dest_x){ /* x dir is up */

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Direction is up"));

        for(i=start_x-1;i>dest_x;i--){

            if(start_y < dest_y && ratiotmp == ratio){
                j++;
                ratiotmp = 0;
            } else if(start_y > dest_y && ratiotmp == ratio){
                j--;
                ratiotmp = 0;
            }

            top_layer = itemstack_top(i,j);
            highlight_arena[i][j]=true;

            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Checking for item at %dx%d layer %d",i,j,top_layer));

            if(myworld.layout[i][j][top_layer] > 0 &&
                !isspelldead(myworld.layout[i][j][top_layer])){

                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Found blocking item %d at %dx%d layer %d",
                    myworld.layout[i][j][top_layer],i,j,top_layer)
                );
                blocked = true;
            }
            ratiotmp++;
        }

    } else {    /* x dir doesnt change */

        i = start_x;

        if(start_y < dest_y){   /* y dir is right */

            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Direction is right"));

            for(j=start_y+1;j<dest_y;j++){

                top_layer = itemstack_top(i,j);
                highlight_arena[i][j]=true;

                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Checking for item at %dx%d layer %d",i,j,top_layer));

                if(myworld.layout[i][j][top_layer] > 0 &&
                    !isspelldead(myworld.layout[i][j][top_layer])){

                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                        "Found blocking item %d at %dx%d layer %d",
                        myworld.layout[i][j][top_layer],i,j,top_layer)
                    );

                    blocked = true;
                }
            }

        } else if (start_y > dest_y){   /* y dir is left */

            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Direction is left"));

            for(j=start_y-1;j>dest_y;j--){

                top_layer = itemstack_top(i,j);
                highlight_arena[i][j]=true;

                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Checking for item at %dx%d layer %d",i,j,top_layer));

                if(myworld.layout[i][j][top_layer] > 0 &&
                    !isspelldead(myworld.layout[i][j][top_layer])){

                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                        "Found blocking item %d at %dx%d layer %d",
                        myworld.layout[i][j][top_layer],i,j,top_layer)
                    );

                    blocked = true;
                }
            }
        }
    }

    return !blocked;
}

#endif

/*
####################
#..checkadjacent().#
####################
*/
bool checkadjacent (void)
{
    int cur_x = myspells[myworld.selected_item[0]].current_pos[0] - 1;
    int cur_y = myspells[myworld.selected_item[0]].current_pos[1] - 1;
    int i,j,top_layer;
    bool enemy_found = false;

    for(i=0;i<3;i++){
        for(j=0;j<3;j++){
            if((cur_x+i >= 0 && cur_y+j >=0) &&
                (cur_x+i < arenas[myworld.arenasize][0]) &&
                (cur_y+j < arenas[myworld.arenasize][1])){

                top_layer = itemstack_top(cur_x+i,cur_y+j);

                if(myworld.layout[cur_x+i][cur_y+j][top_layer] > 0 &&
                    myspells[myworld.layout[cur_x+i][cur_y+j][top_layer]].player_id !=(myworld.current_player-1)
                    && myspells[myworld.layout[cur_x+i][cur_y+j][top_layer]].current_defense > 0
                    && !isspelldead(myworld.layout[cur_x+i][cur_y+j][top_layer])){

                    switch(myspells[myworld.layout[cur_x+i][cur_y+j][top_layer]].spell_type){

                        case SPELL_PLAYER:
                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                log_message,
                                "Found enemy player [id %d / name %s] at %dx%d",
                                myworld.layout[cur_x+i][cur_y+j][top_layer],
                                myplayers[myspells[myworld.layout[cur_x+i][cur_y+j][top_layer]].id].name,
                                cur_x+i,cur_y+j)
                            );
                            break;

                        case SPELL_TREE:
                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                log_message,"Found enemy tree [id %d / name %s] at %dx%d",
                                myworld.layout[cur_x+i][cur_y+j][top_layer],
                                mytrees[myspells[myworld.layout[cur_x+i][cur_y+j][top_layer]].id].name,
                                cur_x+i,cur_y+j)
                            );
                            break;

                        case SPELL_WALL:
                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                log_message,"Found enemy wall [id %d / name %s] at %dx%d",
                                myworld.layout[cur_x+i][cur_y+j][top_layer],
                                mywalls[myspells[myworld.layout[cur_x+i][cur_y+j][top_layer]].id].name,
                                cur_x+i,cur_y+j)
                            );
                            break;

                        case SPELL_BLOB:
                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                log_message,"Found enemy blob [id %d / name %s] at %dx%d",
                                myworld.layout[cur_x+i][cur_y+j][top_layer],
                                myblobs[myspells[myworld.layout[cur_x+i][cur_y+j][top_layer]].id].name,
                                cur_x+i,cur_y+j)
                            );
                            /* Blobs don't block during movement turn*/
                            if(myworld.mode == 3)
                                continue;
                            break;

                        case SPELL_MONSTER:
                        default:
                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                log_message,
                                "Found enemy monster [id %d / name %s] belonging to player %d at %dx%d",
                                myworld.layout[cur_x+i][cur_y+j][top_layer],
                                mymonsters[myspells[myworld.layout[cur_x+i][cur_y+j][top_layer]].id].name,
                                myspells[myworld.layout[cur_x+i][cur_y+j][top_layer]].player_id,
                                cur_x+i,cur_y+j)
                            );
                            break;
                    }

                    enemy_found = true;
                }
            } else {
                console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
                    "ignoring tile outside of arena at %dx%d",cur_x+i,cur_y+j));
            }
        }
    }

    return enemy_found;
}

/*
########################
#..checkadjacent_any().#
########################
*/
bool checkadjacent_any (int cur_x, int cur_y)
{
    int i,j,top_layer;

    for(i=0;i<3;i++){
        for(j=0;j<3;j++){
            if((cur_x+i >= 0 && cur_y+j >=0) &&
                (cur_x+i < arenas[myworld.arenasize][0]) &&
                (cur_y+j < arenas[myworld.arenasize][1])){

                top_layer = itemstack_top(cur_x+i,cur_y+j);

                if(myworld.layout[cur_x+i][cur_y+j][top_layer] > 0 &&
                    !isspelldead(myworld.layout[cur_x+i][cur_y+j][top_layer-1])){

                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                        "Found Item (%d) at %dx%d",
                        myworld.layout[cur_x+i][cur_y+j][top_layer],cur_x+i,cur_y+j)
                    );

                    // Casting a Wall next to another wall?
                    if(myworld.mode == 2 &&
                        myspells[myworld.layout[cur_x+i][cur_y+j][top_layer]].spell_type == SPELL_WALL){

                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Found wall at %dx%d (skipping check)",cur_x+i,cur_y+j));

                        continue;
                    }

                    return true;
                } else {
                    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
                        "Clear pos at %dx%d",cur_x+i,cur_y+j));
                }

            } else {
                console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
                    "ignoring tile outside of arena at %dx%d",cur_x+i,cur_y+j));
            }
        }
    }

    return false;
}

/*
=================================================================== Can/Has/Is? =
*/

/*
##############
#..can_fly().#
##############
*/
bool can_fly (int x, int y)
{
    int current_layer = itemstack_top(x,y);
    int itemid = myworld.layout[x][y][current_layer];

    switch(myspells[itemid].spell_type){
        case SPELL_PLAYER:
            if(myplayers[myspells[itemid].id].flight)
                return true;
            break;

        case SPELL_MONSTER:
            if(mymonsters[myspells[itemid].id].flight)
                return true;
            break;
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Item %d can't fly",myworld.layout[x][y][current_layer]));

    return false;
}

/*
#######################
#..has_rangedattack().#
#######################
*/
static bool has_rangedattack (int x, int y)
{
    int current_layer = itemstack_top(x,y);
    int itemid = myworld.layout[x][y][current_layer];

    switch(myspells[itemid].spell_type){

        case SPELL_PLAYER:
            if(myplayers[myspells[itemid].id].ranged_range > 0)
                return true;
            break;

        case SPELL_MONSTER:
            if(mymonsters[myspells[itemid].id].ranged_range > 0)
                return true;
            break;

        case SPELL_MAGIC_UPGRADE:
            if(mymagic_upgrade[myspells[itemid].id].ranged_range > 0)
                return true;
            break;
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Item %d doesn't have a ranged attack",
        myworld.layout[x][y][current_layer])
    );

    return false;
}

/*
###############
#..is_mount().#
###############
*/
bool is_mount (int x, int y)
{
    int current_layer = itemstack_top(x,y);
    int itemid = myworld.layout[x][y][current_layer];

    switch(myspells[itemid].spell_type){

        case SPELL_MONSTER:
            if(mymonsters[myspells[itemid].id].mount)
                return true;
            break;

        case SPELL_TREE:
            if(mytrees[myspells[itemid].id].mount)
                return true;
            break;

        case SPELL_WALL:
            if(mywalls[myspells[itemid].id].mount)
                return true;
            break;
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Item %d isn't a mount",myworld.layout[x][y][current_layer]));

    return false;
}

/*
#################
#..is_mounted().#
#################
*/
bool is_mounted (int x, int y)
{
    int current_layer = itemstack_top(x,y);

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Checking if Item %d is mounted (%dx%dx%d)",
        myworld.layout[x][y][current_layer],x,y,current_layer)
    );

    /* Only item at x/y, obviously not mounted */
    if(current_layer < 1)
        return false;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Layer count = %d",current_layer));

    /* Not a player above? */
    if(myspells[myworld.layout[x][y][current_layer-1]].spell_type != SPELL_PLAYER)
        return false;

    /* Not owner above? */
    if(myspells[myworld.layout[x][y][current_layer-1]].id != myspells[myworld.layout[x][y][current_layer]].player_id)
        return false;

    /* Standing on a corpse */
    if(isspelldead(myworld.layout[x][y][current_layer-1]))
        return false;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Item %d is mounted (%dx%dx%d) (mounter is id %d) ",
        myworld.layout[x][y][current_layer],x,y,current_layer,
        myworld.layout[x][y][current_layer-1])
    );

    return true;
}

/*
======================================================================= Actions =
*/

/*
#################
#..mount_item().#
#################
*/
void mount_item (int x, int y)
{
    int current_layer = itemstack_top(x,y);
    int p = myworld.current_player + 9;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Moving creature %d up to layer %d",
        myworld.layout[x][y][current_layer],
        current_layer+1)
    );
    myworld.layout[x][y][current_layer+1] = myworld.layout[x][y][current_layer];
    myspells[myworld.layout[x][y][current_layer+1]].current_pos[2] = current_layer+1;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Moving player %d to old layer %d",myspells[p].id,current_layer));
    myworld.layout[x][y][current_layer] = p;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Unsetting old pos %dx%d layer : %d",
        myspells[myworld.selected_item[0]].current_pos[0],
        myspells[myworld.selected_item[0]].current_pos[1],
        myspells[myworld.selected_item[0]].current_pos[2])
    );
    myworld.layout[myspells[p].current_pos[0]][myspells[p].current_pos[1]][myspells[p].current_pos[2]] = 0;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Updating player pos %dx%d layer : %d",x,y,current_layer));
    myspells[p].current_pos[0] = x;
    myspells[p].current_pos[1] = y;
    myspells[p].current_pos[2] = current_layer;

    /* Dont let mounted creature move this turn after being mounted */
    myspells[myworld.layout[x][y][current_layer+1]].beenmoved = true;
    myspells[myworld.layout[x][y][current_layer+1]].skipround = true;
}

/*
###################
#..unmount_item().#
###################
*/
static void unmount_item (int old_x, int old_y)
{
    int current_layer = itemstack_top(old_x,old_y);
    int new_layer = itemstack_top(myworld.cursor[0],myworld.cursor[1]);
    int p = myworld.current_player + 9;

    if(is_mount(myworld.cursor[0],myworld.cursor[1]) &&
        !is_mounted(myworld.cursor[0],myworld.cursor[1])){

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Player %d is dismounting to remount another spell",myspells[p].id));
        mount_item(myworld.cursor[0],myworld.cursor[1]);

    } else {

        if(isspelldead(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][new_layer])){
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Moving player %d to new layer %d (on corpse)",myspells[p].id,
                current_layer+1)
            );
            myworld.layout[myworld.cursor[0]][myworld.cursor[1]][new_layer+1] = p;
        } else {
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Moving player %d to new layer %d",myspells[p].id,current_layer));
            myworld.layout[myworld.cursor[0]][myworld.cursor[1]][new_layer] = p;
        }

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Updating player pos %dx%d layer : %d",myworld.cursor[0],
            myworld.cursor[1],new_layer)
        );

        myspells[p].current_pos[0] = myworld.cursor[0];
        myspells[p].current_pos[1] = myworld.cursor[1];
        myspells[p].current_pos[2] = new_layer;
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Moving mounted creature %d down to layer %d...",
        myworld.layout[old_x][old_y][current_layer],current_layer-1)
    );
    myworld.layout[old_x][old_y][current_layer-1] = myworld.selected_item[0];
    myspells[myworld.selected_item[0]].current_pos[2]--;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Unsetting old mounted creature pos %dx%d layer : %d",old_x,old_y,current_layer));
    myworld.layout[old_x][old_y][current_layer] = 0;

    /* Dont let unmounted item move this turn after unmounting */
    myspells[p].beenmoved = false;
    myspells[myworld.selected_item[0]].beenmoved = true;

    myworld.selected_item[0] = 0;
    myworld.selected_item[1] = 0;
}

/*
#################
#..mount_move().#
#################
*/
static void mount_move (int old_x, int old_y)
{
    int current_layer = itemstack_top(myworld.cursor[0],myworld.cursor[1]);
    int old_layer = itemstack_top(old_x,old_y);
    int p = myworld.current_player + 9;
    int x = myspells[p].current_pos[0];
    int y = myspells[p].current_pos[1];
    int z = myspells[p].current_pos[2];

    /* Target Area has corpse.. */
    if(isspelldead(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][0]) &&
        !myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][0]].illusion &&
        myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][0]].spell_type != SPELL_PLAYER){

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "New space has corpse of [%d]",
            myworld.layout[myworld.cursor[0]][myworld.cursor[1]][0])
        );
        current_layer = 1;
    }

    /* Move mounted creature */
    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Moving creature %d up to layer %d",
        myworld.layout[old_x][old_y][current_layer],
        current_layer+1)
    );
    myworld.layout[myworld.cursor[0]][myworld.cursor[1]][current_layer+1] =
        myworld.layout[old_x][old_y][old_layer];

    myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][current_layer+1]].current_pos[0] =
        myworld.cursor[0];
    myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][current_layer+1]].current_pos[1] =
        myworld.cursor[1];
    myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][current_layer+1]].current_pos[2] =
        current_layer+1;

    myworld.layout[old_x][old_y][old_layer] = 0;

    /* Move player */
    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Moving player %d to new layer %d",p,current_layer));

    myworld.layout[myworld.cursor[0]][myworld.cursor[1]][current_layer] =
        myworld.layout[x][y][z];

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Unsetting old pos %dx%d layer : %d",x,y,z));

    myworld.layout[x][y][z] = 0;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Updating player pos %dx%d layer : %d",myworld.cursor[0],
        myworld.cursor[1],current_layer)
    );
    myspells[p].current_pos[0] = myworld.cursor[0];
    myspells[p].current_pos[1] = myworld.cursor[1];
    myspells[p].current_pos[2] = current_layer;
}


/*
#####################
#..creature_death().#
#####################
*/
void creature_death (int spellid, int old_x, int old_y)
{
    int s;
    int top_layer = itemstack_top(myworld.cursor[0],myworld.cursor[1]);
    int old_layer = itemstack_top(old_x,old_y);
    int top_layer_item = myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer];
    int below_layer_item = myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer-1];
    char histtext[255];

    memset(histtext,'\0',sizeof(histtext));

    myspells[spellid].dead = true;

    /* If the killed spell was a tree/wall then set it to an
        illusion so it doesnt create a corpse */
    if(myspells[top_layer_item].spell_type == SPELL_TREE
        || myspells[top_layer_item].spell_type == SPELL_WALL)
        myspells[spellid].illusion = true;

    /* Player killed, slaughter his minions */
    if (myspells[spellid].spell_type == SPELL_PLAYER) {

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Player %s has been defeated, death to his minions!",
            myplayers[myspells[spellid].player_id].name)
        );

        sprintf(histtext,"%s has been defeated!", myplayers[myspells[spellid].player_id].name);
        history_add(histtext);

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Current world balance to %d",myworld.balance));

        for(s=100; s<myworld.total_spells+101; s++){

            if(myspells[spellid].player_id == myspells[s].player_id){
                if(!myspells[s].beencast){
                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                        "Uncast Spell [%d]", s));
                    continue;
                }

                /* If Monster or Tree or wall or blob unset its grid position*/
                if(myspells[s].spell_type == SPELL_MONSTER || myspells[s].spell_type == SPELL_TREE ||
                    myspells[s].spell_type == SPELL_WALL || myspells[s].spell_type == SPELL_BLOB) {

                    /* Only remove monsters or living spells from arena */
                    if(myspells[s].spell_type == SPELL_MONSTER || !isspelldead(s)){
                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Unsetting Spell (%d) at %dx%d layer : %d",s,
                            myspells[s].current_pos[0],
                            myspells[s].current_pos[1],
                            myspells[s].current_pos[2])
                        );

                        myworld.layout[myspells[s].current_pos[0]][myspells[s].current_pos[1]][myspells[s].current_pos[2]] = 0;
                    }

                    /* Monster Type above corpse ? */
                    if( isspelldead(s) && !(myspells[s].spell_type == SPELL_TREE ||
                        myspells[s].spell_type == SPELL_WALL || myspells[s].spell_type == SPELL_BLOB) &&
                        myworld.layout[myspells[s].current_pos[0]][myspells[s].current_pos[1]][myspells[s].current_pos[2]+1] > 0){

                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Item (%d) above old Spell (%d) at %dx%d layer : %d",
                            myworld.layout[myspells[s].current_pos[0]][myspells[s].current_pos[1]][myspells[s].current_pos[2]+1],
                            s,
                            myspells[s].current_pos[0],
                            myspells[s].current_pos[1],
                            myspells[s].current_pos[2])
                        );

                        /* Move top creature down */
                        myworld.layout[myspells[s].current_pos[0]][myspells[s].current_pos[1]][myspells[s].current_pos[2]] =
                            myworld.layout[myspells[s].current_pos[0]][myspells[s].current_pos[1]][myspells[s].current_pos[2]+1];

                        /* Update spells pos*/
                        myspells[myworld.layout[myspells[s].current_pos[0]][myspells[s].current_pos[1]][myspells[s].current_pos[2]+1]].current_pos[2]--;

                        /* Unset old Pos */
                        myworld.layout[myspells[s].current_pos[0]][myspells[s].current_pos[1]][myspells[s].current_pos[2]+1] = 0;

                        /* There is a mounted monster above? */
                        if(myworld.layout[myspells[s].current_pos[0]][myspells[s].current_pos[1]][2] > 0){

                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                                "Mounted Item (%d) above old Spell (%d) at %dx%d layer : %d",
                                myworld.layout[myspells[s].current_pos[0]][myspells[s].current_pos[1]][2],
                                s,
                                myspells[s].current_pos[0],
                                myspells[s].current_pos[1],
                                myspells[s].current_pos[2])
                            );

                            /* Move top creature down */
                            myworld.layout[myspells[s].current_pos[0]][myspells[s].current_pos[1]][1] =
                                myworld.layout[myspells[s].current_pos[0]][myspells[s].current_pos[1]][2];

                            /* Update spells pos*/
                            myspells[myworld.layout[myspells[s].current_pos[0]][myspells[s].current_pos[1]][2]].current_pos[2]--;

                            /* Unset old Pos */
                            myworld.layout[myspells[s].current_pos[0]][myspells[s].current_pos[1]][2] = 0;

                        }

                    }

                }
                myspells[s].dead = true;

            } else {
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Not my Spell [%d]", s));
            }
        }

        /* Unset player's position */
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Removing player from world at %dx%dx%d",myspells[spellid].current_pos[0],
            myspells[spellid].current_pos[1],myspells[spellid].current_pos[2])
        );
        myworld.layout[myspells[spellid].current_pos[0]][myspells[spellid].current_pos[1]][myspells[spellid].current_pos[2]] = 0;
    } else {

        /* History log */
        switch(myspells[spellid].spell_type){
            case SPELL_BLOB:
                sprintf(histtext,"%s's %s has been destroyed",
                    myplayers[myspells[spellid].player_id].name,
                    myblobs[myspells[spellid].id].name
                );
                break;

            case SPELL_WALL:
                sprintf(histtext,"%s's %s has been destroyed",
                    myplayers[myspells[spellid].player_id].name,
                    mywalls[myspells[spellid].id].name
                );
                break;
            case SPELL_TREE:
                sprintf(histtext,"%s's %s has been destroyed",
                    myplayers[myspells[spellid].player_id].name,
                    mytrees[myspells[spellid].id].name
                );
                break;
            case SPELL_MONSTER:
                sprintf(histtext,"%s's %s has been destroyed",
                    myplayers[myspells[spellid].player_id].name,
                    mymonsters[myspells[spellid].id].name
                );
                break;

            default:
                sprintf(histtext,"%s's spell has been destroyed",
                    myplayers[myspells[spellid].player_id].name
                );
                break;
        }
        history_add(histtext);
    }

    /* Creature killed creature */
    if(myspells[spellid].spell_type == SPELL_MONSTER &&
        myspells[myworld.selected_item[0]].spell_type == SPELL_MONSTER &&
        !myspells[spellid].illusion){

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Setting creatures last killed to %d",spellid));

        myspells[myworld.selected_item[0]].last_killed = spellid;
    }

    /*
    ###################
    #..Movement.Round.#
    ###################
    */
    if(myworld.mode == CE_WORLD_MODE_CASTING || myworld.mode == CE_WORLD_MODE_MOVE){
        /*
        #################
        #..Melee.attack.#
        #################
        */
        if(myworld.mode == CE_WORLD_MODE_MOVE && (myworld.submode == 0 || myworld.submode == 5)){

            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                log_message,"Melee Attack! (layer : %d)",top_layer));

            if(top_layer > 0){

                /* Killed a mounted creature? */
                if(!isspelldead(below_layer_item) &&
                    is_mounted(myworld.cursor[0],myworld.cursor[1])){

                    /* Was mounted creature real and not a tree/wall? */
                    if(!myspells[top_layer_item].illusion &&
                        myspells[top_layer_item].spell_type != SPELL_WALL &&
                        myspells[top_layer_item].spell_type != SPELL_TREE){

                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Item was mounted and not an illusion, moving corpse to bottom"));

                        myworld.layout[myworld.cursor[0]][myworld.cursor[1]][0] =
                            myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer];

                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Forcing player %d to layer 1",
                            myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][0]].player_id)
                        );

                        myworld.layout[myworld.cursor[0]][myworld.cursor[1]][1] =
                            myspells[top_layer_item].player_id + 10;

                        myworld.layout[myworld.cursor[0]][myworld.cursor[1]][2] = 0;

                        myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][0]].current_pos[2] = 0;
                        myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][1]].current_pos[2] = 1;

                    } else {
                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Item was mounted and an illusion or a tree/wall, unsetting position"));

                        myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] = 0;
                    }

                } else {

                    /* Dont bother updating dead players info, hes dead why would he care? */
                    if(myspells[spellid].spell_type != SPELL_PLAYER && myspells[spellid].spell_type != SPELL_TREE &&
                        myspells[spellid].spell_type != SPELL_WALL && myspells[spellid].spell_type != SPELL_BLOB){

                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Fixing piled corpses : new corpse %d  [layer %d] => old corpse %d [layer %d]",
                            myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer-1],
                            top_layer-1,
                            myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer],
                            top_layer)
                        );

                        /* Move new corpse to old corpse layer */
                        myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer-1] =
                            myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer];

                        /* update dead spells layer pos */
                        myspells[below_layer_item].current_pos[2] = top_layer-1;

                    }

                    /* Is attacking creature mounted? */
                    if(is_mounted(old_x,old_y)){

                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Attacking Creature is mounted"));

                        mount_move(old_x,old_y);

                    } else {
                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Attacking Creature is not mounted"));
                    }

                    /* Set attacker to new pos if not a tree */
                    if(myspells[myworld.selected_item[0]].spell_type != SPELL_TREE &&
                        myspells[myworld.selected_item[0]].spell_type != SPELL_WALL){

                        top_layer = itemstack_top(myworld.cursor[0],myworld.cursor[1]);
                        myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] = myworld.selected_item[0];
                        myworld.layout[old_x][old_y][old_layer] = 0;

                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Setting : %dx%d to id[%d]",myworld.cursor[0],myworld.cursor[1],
                            myworld.selected_item[0])
                        );

                        myspells[myworld.selected_item[0]].current_pos[0] = myworld.cursor[0];
                        myspells[myworld.selected_item[0]].current_pos[1] = myworld.cursor[1];
                        myspells[myworld.selected_item[0]].current_pos[2] = top_layer;

                    }
                }
            } else {

                /* Trees/walls dont move after they kill */
                if(myspells[myworld.selected_item[0]].spell_type != SPELL_TREE &&
                    myspells[myworld.selected_item[0]].spell_type != SPELL_WALL){

                    /* Is attacking creature mounted? */
                    if(is_mounted(old_x,old_y)){

                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Attacking Creature (@ %dx%d) is mounted",old_x,old_y));

                        mount_move(old_x,old_y);

                    } else {
                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Attacking Creature is not mounted"));

                        if(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] > 0 &&
                            !myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].illusion){

                            top_layer++;
                        }

                        myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] = myworld.selected_item[0];

                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Setting : %dx%dx%d to id[%d]",myworld.cursor[0],myworld.cursor[1],
                            top_layer,myworld.selected_item[0])
                        );

                        myspells[myworld.selected_item[0]].current_pos[0] = myworld.cursor[0];
                        myspells[myworld.selected_item[0]].current_pos[1] = myworld.cursor[1];
                        myspells[myworld.selected_item[0]].current_pos[2] = top_layer;

                    }

                    myworld.layout[old_x][old_y][itemstack_top(old_x,old_y)] = 0;
                }
            }
        /*
        ##################
        #..Ranged.attack.#
        ##################
        */
        } else {
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Ranged Attack!"));

            /* Player killed */
            if(myspells[spellid].spell_type == SPELL_PLAYER){

                myworld.layout[myworld.cursor[0]][myworld.cursor[1]][itemstack_top(myworld.cursor[0],myworld.cursor[1])] = 0;

                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Setting : %dx%d to blank",myworld.cursor[0],myworld.cursor[1]));
            }

            if(!myspells[top_layer_item].illusion){

                /* Killed a mounted creature? */
                if(is_mounted(myworld.cursor[0],myworld.cursor[1])){

                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                        "Item was mounted and not an illusion, moving corpse to bottom"));

                    myworld.layout[myworld.cursor[0]][myworld.cursor[1]][0] =
                        myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer];

                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                        "Forcing player %d to layer 1",
                        myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][0]].player_id)
                    );

                    myworld.layout[myworld.cursor[0]][myworld.cursor[1]][1] =
                        myspells[top_layer_item].player_id + 10;

                    myworld.layout[myworld.cursor[0]][myworld.cursor[1]][2] = 0;
                    myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][0]].current_pos[2] = 0;
                    myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][1]].current_pos[2] = 1;

                } else {

                    /* Died on a Corpse? */
                    if(top_layer > 0 && myspells[top_layer_item].spell_type != SPELL_TREE &&
                        myspells[top_layer_item].spell_type != SPELL_WALL && myspells[top_layer_item].spell_type != SPELL_BLOB){

                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Fixing piled corpses : new corpse %d  [layer %d] => old corpse %d [layer %d]",
                            myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer-1],top_layer-1,
                            myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer],top_layer)
                        );

                        /* Move new corpse to old corpse layer */
                        myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer-1] =
                            myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer];

                        /* update dead spells layer pos */
                        myspells[below_layer_item].current_pos[2] = top_layer-1;
                    }

                    if(myspells[top_layer_item].spell_type == SPELL_TREE ||
                        myspells[top_layer_item].spell_type == SPELL_WALL ||
                        myspells[top_layer_item].spell_type == SPELL_BLOB){

                        myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] = 0;
                    }

                    /* else - do nothing as the renderer will draw the creature
                        on its current layer as a ~ symbol */

                }
            } else {
                myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] = 0;
            }
        }
    /*
    ###############
    #..Grown.Over.#
    ###############
    */
    } else {

        /* Don't bother updating dead players info, hes dead why would he care?
         and Blobs don't leave corpses */
        if(myspells[spellid].spell_type != SPELL_PLAYER && myspells[spellid].spell_type != SPELL_BLOB){

            /* Was on a corpse? */
            if(top_layer){

                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Fixing piled corpses : new corpse %d  [layer %d] => old corpse %d [layer %d]",
                    myworld.layout[myspells[spellid].current_pos[0]][myspells[spellid].current_pos[1]][myspells[spellid].current_pos[2]-1],
                    top_layer-1,
                    myworld.layout[myspells[spellid].current_pos[0]][myspells[spellid].current_pos[1]][myspells[spellid].current_pos[2]],
                    top_layer)
                );

                /* Move new corpse to old corpse layer */
                myworld.layout[myspells[spellid].current_pos[0]][myspells[spellid].current_pos[1]][myspells[spellid].current_pos[2]-1] =
                    myworld.layout[myspells[spellid].current_pos[0]][myspells[spellid].current_pos[1]][myspells[spellid].current_pos[2]];

                myspells[spellid].current_pos[2] = top_layer;

                /* update dead spells layer pos */
                myspells[below_layer_item].current_pos[2] = top_layer-1;
            }
        }
    }
}

/*
######################
#..creature_attack().#
######################
*/
bool creature_attack (int old_x, int old_y)
{

    int attack, defense, dex, dexroll, attackroll;
    bool attacker_undead = false;
    bool defender_undead = false;
    int top_layer = itemstack_top(myworld.cursor[0],myworld.cursor[1]);
    int old_top_layer = itemstack_top(old_x,old_y);

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "creature at %dx%d attacking creature at %dx%d",old_x,old_y,
        myworld.cursor[0],myworld.cursor[1])
    );

    /*
    ######################
    #..Get.attack.values.#
    ######################
    */
    if(myspells[myworld.layout[old_x][old_y][old_top_layer]].spell_type == SPELL_PLAYER){
        attack = myplayers[myspells[myworld.layout[old_x][old_y][old_top_layer]].player_id].attack;
        dex = myplayers[myspells[myworld.layout[old_x][old_y][old_top_layer]].player_id].dex;
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "attacking player (%s) has attack value of %d and dex value of %d",
            myplayers[myspells[myworld.layout[old_x][old_y][old_top_layer]].player_id].name,
            attack,dex)
        );

        /*
        ######################
        #..Is.player.undead?.#
        ######################
        */
        if(myspells[myworld.layout[old_x][old_y][old_top_layer]].undead){
            attacker_undead = true;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "attacking player (%s) is undead",
                myplayers[myspells[myworld.layout[old_x][old_y][old_top_layer]].player_id].name)
            );

        } else if(myplayers[myspells[myworld.layout[old_x][old_y][old_top_layer]].player_id].attack_undead){
            attacker_undead = true;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "attacking player (%s) can attack undead",
                myplayers[myspells[myworld.layout[old_x][old_y][old_top_layer]].player_id].name)
            );

        } else {
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "attacking player (%s) is not undead",
                myplayers[myspells[myworld.layout[old_x][old_y][old_top_layer]].player_id].name)
            );
        }

    } else if(myspells[myworld.layout[old_x][old_y][old_top_layer]].spell_type == SPELL_TREE){
        attack = mytrees[myspells[myworld.layout[old_x][old_y][old_top_layer]].id].attack;
        dex = 0;
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "attacking Tree (%s) has attack value of %d and dex value of %d",
            mytrees[myspells[myworld.layout[old_x][old_y][old_top_layer]].id].name,
            attack,dex)
        );

    } else if(myspells[myworld.layout[old_x][old_y][old_top_layer]].spell_type == SPELL_WALL){
        attack = mywalls[myspells[myworld.layout[old_x][old_y][old_top_layer]].id].attack;
        dex = 0;
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "attacking Wall (%s) has attack value of %d and dex value of %d",
            mywalls[myspells[myworld.layout[old_x][old_y][old_top_layer]].id].name,
            attack,dex)
        );

    } else {
        /*
        #######################
        #..Was.ranged.attack?.#
        #######################
        */
        if(myworld.submode == 1){
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Using ranged attack"));
            attack = mymonsters[myspells[myworld.layout[old_x][old_y][old_top_layer]].id].attack;
        } else {
            attack = mymonsters[myspells[myworld.layout[old_x][old_y][old_top_layer]].id].attack;
        }

        dex = mymonsters[myspells[myworld.layout[old_x][old_y][old_top_layer]].id].dex;
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "attacking creature (%s) has attack value of %d and dex value of %d",
            mymonsters[myspells[myworld.layout[old_x][old_y][old_top_layer]].id].name,
            attack,dex)
        );

        /*
        ########################
        #..Is.creature.undead?.#
        ########################
        */
        if(myspells[myworld.layout[old_x][old_y][old_top_layer]].undead){
            attacker_undead = true;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "attacking creature is undead"));
        } else {
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "attacking creature is not undead"));
        }
    }

    /*
    #######################
    #..Get.defense.values.#
    #######################
    */
    defense = myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].current_defense;

    if(myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].spell_type == SPELL_PLAYER){
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "defending player (%s) has defense value of %d [MAX %d]",
            myplayers[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].player_id].name,
            defense,
            myplayers[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].player_id].defense)
        );

        /*
        ######################
        #..Is.player.undead?.#
        ######################
        */
        if(myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].undead){
            defender_undead = true;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "defending player is undead"));
        } else {
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "defending player is not undead"));
        }

    } else if(myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].spell_type == SPELL_TREE){
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "defending tree (%s) has defense value of %d [MAX %d]",
            mytrees[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].player_id].name,
            defense,
            mytrees[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].player_id].defense)
        );

    } else if(myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].spell_type == SPELL_WALL){
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "defending wall (%s) has defense value of %d [MAX %d]",
            mywalls[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].player_id].name,
            defense,
            mywalls[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].player_id].defense)
        );

        if(defense == 0){
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "defending wall (%s) is invulnerable!",
                mywalls[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].player_id].name)
            );
            return false;
        }

    } else if(myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].spell_type == SPELL_BLOB){
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "defending blob (%s) has defense value of %d [MAX %d]",
            myblobs[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].player_id].name,
            defense,
            myblobs[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].player_id].defense)
        );

        if(defense == 0){
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "defending blob (%s) is invulnerable!",
                myblobs[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].player_id].name)
            );
            return false;
        }

    } else {
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "defending creature (%s) has defense value of %d [MAX %d]",
            mymonsters[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].id].name,
            defense,
            mymonsters[myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].id].defense)
        );
    }

    /*
    ########################
    #..Is.creature.undead?.#
    ########################
    */
    if(myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].undead){
        defender_undead = true;
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "defending creature is undead"));
    } else {
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "defending creature is not undead"));
    }

    if ((defender_undead && attacker_undead) || (!defender_undead)){
        srand(time(NULL));
        attackroll = (rand()%attack)+1;
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "attackroll was %d",attackroll));

        myspells[myworld.selected_item[0]].beenmoved = true;

        srand(time(NULL)+10);
        dexroll = (rand()%10)+1;
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "dexroll was %d",dexroll));

        if(dex >= dexroll){
            srand(time(NULL)+1000);
            dex = (rand()%dexroll)+1;

            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "dex to be added roll was %d",dex));

            attackroll += dex;

            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "dexroll was successful, new attack value %d",attack));
        } else {
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "dexroll was unsuccessful"));
        }

        if (attackroll >= defense){
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Successful attack (%d vs %d)",attackroll,defense));

            creature_death(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer],
             old_x, old_y);

            return true;
        } else {
            myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].current_defense -= attackroll;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Unsuccessful attack (%d vs %d)",attackroll,defense));
        }

    } else {
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Unsuccessful attack target is undead"));

        myspells[myworld.selected_item[0]].beenmoved = false;

        sprintf(infobar_text,"Target is Undead");
        beepmsg = true;
    }

    return false;
}

/*
###############
#..moveitem().#
###############
*/
void moveitem (int x, int y)
{
    int top_layer = itemstack_top(myworld.cursor[0],myworld.cursor[1]);
    int old_pos_x,old_pos_y,old_pos_layer;
    bool was_mounted = false;

    /* Something to fight near by? */
    if(myworld.submode != 4 && checkadjacent()){
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Enemy Found!"));

        if(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] > 0 &&
            myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].player_id != (myworld.current_player-1)
            && !isspelldead(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer])){

            if(creature_attack(myspells[myworld.selected_item[0]].current_pos[0],
                myspells[myworld.selected_item[0]].current_pos[1])){

                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Successful attack"));
            } else {
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Unsuccessful attack"));

                CE_beep();
            }

            myworld.cursor[0] = myspells[myworld.selected_item[0]].current_pos[0];
            myworld.cursor[1] = myspells[myworld.selected_item[0]].current_pos[1];

            /*
            ############################
            #..Check.for.ranged.combat.#
            ############################
            */
            if(has_rangedattack(myspells[myworld.selected_item[0]].current_pos[0],myspells[myworld.selected_item[0]].current_pos[1])){

                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Mode - Ranged Attack"));

                myworld.submode = 1;
            } else {
                if(myspells[myworld.selected_item[0]].beenmoved){
                    myworld.selected_item[0] = 0;
                    myworld.selected_item[1] = 0;
                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                        "Item is out of moves"));
                }
            }

        } else {

            myworld.cursor[0] -= x;
            myworld.cursor[1] -= y;
            CE_beep();
        }
    } else {
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,"No Enemies Found!"));

        /*
        ####################
        #..Mount.Creature?.#
        ####################
        */
        if (myworld.submode != 4 && myworld.submode != 5 &&
            myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].player_id == (myworld.current_player-1) &&
            myspells[myworld.selected_item[0]].spell_type == SPELL_PLAYER &&
            is_mount(myworld.cursor[0],myworld.cursor[1]) &&
            !isspelldead(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer])){

            /* Got back to game loop for input/drawscene */
            myworld.submode = 2;
        /*
        #######################
        #..Dismount.Creature?.#
        #######################
        */
        }else if (myworld.submode == 4) {

            /* Is space clear to dismount or are you dismounting to another mount? */
            if((myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] == 0 ||
                isspelldead(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]) ||
                is_mount(myworld.cursor[0],myworld.cursor[1])) &&
                (myworld.cursor[0] < myspells[myworld.current_player+9].current_pos[0]+2 &&
                myworld.cursor[0] > myspells[myworld.current_player+9].current_pos[0]-2) &&
                (myworld.cursor[1] < myspells[myworld.current_player+9].current_pos[1]+2 &&
                myworld.cursor[1] > myspells[myworld.current_player+9].current_pos[1]-2)) {

                unmount_item(myworld.cursor[0]-x,myworld.cursor[1]-y);
                myworld.submode = 0;

            } else {

                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Cant dismount to %dx%d layer : %d", myworld.cursor[0],
                    myworld.cursor[1], top_layer)
                );

                CE_beep();
            }

            /* ALWAYS move cursor back to original place */
            myworld.cursor[0] = myworld.cursor[0]-x;
            myworld.cursor[1] = myworld.cursor[1]-y;

        } else {

            /*
            #####################
            #..Flying.Creatures.#
            #####################
            */
            if(can_fly(myspells[myworld.selected_item[0]].current_pos[0],
                    myspells[myworld.selected_item[0]].current_pos[1])){

                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Flying creature landing..."));

                /* Is target area empty? */
                if(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] == 0 ||
                    isspelldead(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer])){

                    old_pos_x = myspells[myworld.selected_item[0]].current_pos[0];
                    old_pos_y = myspells[myworld.selected_item[0]].current_pos[1];
                    old_pos_layer = myspells[myworld.selected_item[0]].current_pos[2];

                    if(is_mounted(old_pos_x,old_pos_y)){

                        mount_move(old_pos_x,old_pos_y);

                    } else {
                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,"Item is not mounted"));

                        top_layer = itemstack_top(myspells[myworld.selected_item[0]].current_pos[0],
                            myspells[myworld.selected_item[0]].current_pos[1]);

                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                            log_message,"unsetting : %dx%d layer : %d",
                            myspells[myworld.selected_item[0]].current_pos[0],
                            myspells[myworld.selected_item[0]].current_pos[1],top_layer)
                        );

                        myworld.layout[myspells[myworld.selected_item[0]].current_pos[0]][myspells[myworld.selected_item[0]].current_pos[1]][top_layer] = 0;

                        top_layer = itemstack_top(myworld.cursor[0],myworld.cursor[1]);

                        if(isspelldead(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]))
                            top_layer++;

                        myspells[myworld.selected_item[0]].current_pos[0] = myworld.cursor[0];
                        myspells[myworld.selected_item[0]].current_pos[1] = myworld.cursor[1];
                        myspells[myworld.selected_item[0]].current_pos[2] = top_layer;

                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
                            "Setting : %dx%d layer : %d to [%d]",myworld.cursor[0],
                            myworld.cursor[1], top_layer, myworld.selected_item[0])
                        );

                        myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] = myworld.selected_item[0];
                    }

                    /*
                    ####################
                    #..Check.for.enemy.#
                    ####################
                    */
                    if(!checkadjacent()){
                        /*
                        ############################
                        #..Check.for.ranged.combat.#
                        ############################
                        */
                        if(has_rangedattack(myspells[myworld.selected_item[0]].current_pos[0],
                            myspells[myworld.selected_item[0]].current_pos[1])){

                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                log_message,"Mode - Ranged Attack"));

                            myworld.submode = 1;

                        } else {
                            myworld.selected_item[0] = 0;
                            myworld.selected_item[1] = 0;
                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                log_message,"Item is out of moves"));
                        }
                    }

                /* Something to Fight */
                } else if(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] > 0 &&
                    myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].player_id != (myworld.current_player-1) &&
                    !isspelldead(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer])){

                    old_pos_x = myspells[myworld.selected_item[0]].current_pos[0];
                    old_pos_y = myspells[myworld.selected_item[0]].current_pos[1];
                    old_pos_layer = myspells[myworld.selected_item[0]].current_pos[2];

                    if(creature_attack(myspells[myworld.selected_item[0]].current_pos[0],myspells[myworld.selected_item[0]].current_pos[1])){

                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,"Successful attack (from %dx%d)",old_pos_x,old_pos_y));

                        was_mounted = is_mounted(old_pos_x,old_pos_y);

                        if(isspelldead(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][itemstack_top(myworld.cursor[0],myworld.cursor[1])]) ||
                            myworld.layout[myworld.cursor[0]][myworld.cursor[1]][itemstack_top(myworld.cursor[0],myworld.cursor[1])] == myworld.selected_item[0]){

                            /* Is creature mounted? */
                            if(was_mounted){
                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                    sprintf(log_message,"Item is mounted"));

                                mount_move(old_pos_x,old_pos_y);
                            } else {
                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                    sprintf(log_message,"Item is not mounted"));

                                myspells[myworld.selected_item[0]].current_pos[0] = myworld.cursor[0];
                                myspells[myworld.selected_item[0]].current_pos[1] = myworld.cursor[1];
                                myspells[myworld.selected_item[0]].current_pos[2] = top_layer+1;
                            }

                        } else {
                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                log_message,
                                "Attacked Mounted Creature, reset cursor to attacks original pos.")
                            );

                            myworld.cursor[0] = myspells[myworld.selected_item[0]].current_pos[0];
                            myworld.cursor[1] = myspells[myworld.selected_item[0]].current_pos[1];
                        }

                        /* If flying creature kills something by flight,
                            check to see if there is something next to it to fight. */
                        if(myworld.submode == 0 &&
                            has_rangedattack(myspells[myworld.selected_item[0]].current_pos[0],
                                myspells[myworld.selected_item[0]].current_pos[1])){

                            myspells[myworld.selected_item[0]].beenmoved = false;
                            myworld.submode++;

                        } else {
                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                log_message,"No range attack, ending turn. Type = %d ",
                                myspells[myworld.selected_item[0]].spell_type)
                            );
                        }

                    } else {
                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Unsuccessful attack"));

                        myworld.cursor[0] = myspells[myworld.selected_item[0]].current_pos[0];
                        myworld.cursor[1] = myspells[myworld.selected_item[0]].current_pos[1];
                        CE_beep();
                    }

                    /*
                    ############################
                    #..Check.for.ranged.combat.#
                    ############################
                    */
                    if(has_rangedattack(myspells[myworld.selected_item[0]].current_pos[0],
                        myspells[myworld.selected_item[0]].current_pos[1])){

                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                            log_message,"Mode - Ranged Attack"));

                        myworld.submode = 1;
                    } else {
                        if(myspells[myworld.selected_item[0]].beenmoved){
                            myworld.selected_item[0] = 0;
                            myworld.selected_item[1] = 0;
                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                log_message,"Item is out of moves"));
                        }
                    }
                } else {
                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                        "Target area [%dx%d] is not empty/attackable",myworld.cursor[0],
                        myworld.cursor[1])
                    );

                    myworld.cursor[0] -= x;
                    myworld.cursor[1] -= y;
                    CE_beep();
                }

            /*
            ######################
            #..Walking.Creatures.#
            ######################
            */
            } else {

                if (((myworld.cursor[0] >= 0) && (myworld.cursor[1] >= 0)) &&
                    (myworld.cursor[0] < arenas[myworld.arenasize][0]) &&
                    (myworld.cursor[1] < arenas[myworld.arenasize][1])){

                     /* Is target area empty? */
                    if(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] == 0 ||
                        isspelldead(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]) ){

                        old_pos_x = myspells[myworld.selected_item[0]].current_pos[0];
                        old_pos_y = myspells[myworld.selected_item[0]].current_pos[1];
                        old_pos_layer = myspells[myworld.selected_item[0]].current_pos[2];
                        top_layer = itemstack_top(myworld.cursor[0],myworld.cursor[1]);

                        if(is_mounted(old_pos_x,old_pos_y)){

                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                log_message,
                                "Found mount at old pos! - old layer pos : %d - monster.player_id : %d - Current Player_id : %d - Item above id : %d",
                                old_pos_layer,
                                myspells[myworld.selected_item[0]].player_id,
                                myworld.current_player-1,
                                myspells[myworld.layout[old_pos_x][old_pos_y][old_pos_layer-1]].id)
                            );

                            mount_move(old_pos_x,old_pos_y);

                        } else {

                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                log_message,
                                "Item is not mounted : layer : %d / old pos id : %d / my owner : %d",
                                old_pos_layer,
                                myworld.current_player,
                                myspells[myworld.selected_item[0]].player_id)
                            );


                            old_pos_layer = itemstack_top(old_pos_x,old_pos_y);
                            myworld.layout[old_pos_x][old_pos_y][old_pos_layer] = 0;
                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                                "unsetting : %dx%d Layer : %d",myworld.cursor[0] - x,
                                myworld.cursor[1] - y,old_pos_layer)
                            );

                            if(isspelldead(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer])){
                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                                    "Item [layer %d] standing on a corpse [layer %d]",
                                    top_layer-1,top_layer)
                                );

                                top_layer++;
                            }

                            myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] = myworld.selected_item[0];
                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                log_message,"Setting : %dx%d layer : %d to [%d]",
                                myworld.cursor[0],myworld.cursor[1], top_layer,
                                myworld.selected_item[0])
                            );

                            myspells[myworld.selected_item[0]].current_pos[0] = myworld.cursor[0];
                            myspells[myworld.selected_item[0]].current_pos[1] = myworld.cursor[1];
                            myspells[myworld.selected_item[0]].current_pos[2] = top_layer;

                        }

                        /* Decrease items move count */
                        myworld.selected_item[1]--;

                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                            log_message,"Item has %d moves left",
                            myworld.selected_item[1])
                        );


                        /*
                        ####################
                        #..Check.for.enemy.#
                        ####################
                        */
                        if(myworld.selected_item[1] < 1 && !checkadjacent()){
                            /*
                            ############################
                            #..Check.for.ranged.combat.#
                            ############################
                            */
                            if(has_rangedattack(myspells[myworld.selected_item[0]].current_pos[0],
                                myspells[myworld.selected_item[0]].current_pos[1])){

                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                    sprintf(log_message,"Mode - Ranged Attack"));

                                    myworld.submode = 1;
                                } else {
                                    myworld.selected_item[0] = 0;
                                    myworld.selected_item[1] = 0;
                                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                        sprintf(log_message,"Item is out of moves"));
                            }
                        }
                    } else {
                        /*
                        #######################
                        #..Force.Attack.Blob?.#
                        #######################
                        */
                        if(myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].spell_type == SPELL_BLOB &&
                            myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].current_defense > 0){

                            if(creature_attack(myspells[myworld.selected_item[0]].current_pos[0],
                                myspells[myworld.selected_item[0]].current_pos[1])){

                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                                    "Successful attack"));
                            } else {
                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                                    "Unsuccessful attack"));
                                CE_beep();
                            }

                            myworld.cursor[0] = myspells[myworld.selected_item[0]].current_pos[0];
                            myworld.cursor[1] = myspells[myworld.selected_item[0]].current_pos[1];

                            /*
                            ############################
                            #..Check.for.ranged.combat.#
                            ############################
                            */
                            if(has_rangedattack(myspells[myworld.selected_item[0]].current_pos[0],myspells[myworld.selected_item[0]].current_pos[1])){

                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                                    "Mode - Ranged Attack"));

                                myworld.submode = 1;
                            } else {
                                if(myspells[myworld.selected_item[0]].beenmoved){
                                    myworld.selected_item[0] = 0;
                                    myworld.selected_item[1] = 0;
                                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                                        "Item is out of moves"));
                                }
                            }

                        } else {
                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                log_message,
                                "Target area [%dx%d] is not empty/attackable",
                                myworld.cursor[0],
                                myworld.cursor[1])
                            );

                            myworld.cursor[0] -= x;
                            myworld.cursor[1] -= y;
                            CE_beep();
                        }
                    }
                } else {
                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                        "Target area [%dx%d] out of bounds!",myworld.cursor[0],
                        myworld.cursor[1])
                    );

                    myworld.cursor[0] -= x;
                    myworld.cursor[1] -= y;
                    CE_beep();
                }
            }   /* endif flying/walking creatures */
        }   /* endif want to mount? */
    }   /* end if checkadjacent() */
}

/*
======================================================================== Spells =
*/

/*
###################
#..givenewspell().#
###################
*/
static void givenewspell (int player_id)
{
    int newspellid = myworld.total_spells+100;
    int newspelltype = rand()%MAX_SPELL_TYPES+1;

    /* Can't give a SPELL_MAGIC_SPECIAL Spell */
    while(newspelltype == SPELL_MAGIC_SPECIAL)
        newspelltype = rand()%MAX_SPELL_TYPES+1;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Total spells before adding new spell %d, new starting point %d",
        myworld.total_spells,myworld.total_spells+101));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "New spell type rand() was %d",newspelltype));

    myspells[newspellid].id = newspellid;
    myspells[newspellid].player_id = player_id;
    myspells[newspellid].dead = false;
    myspells[newspellid].illusion = false;
    myspells[newspellid].beencast = false;
    myspells[newspellid].beenmoved = false;
    myspells[newspellid].turnlimit = 0;
    myspells[newspellid].genspells = false;
    myspells[newspellid].spell_type = newspelltype;
    myspells[newspellid].last_killed = 0;

    switch(newspelltype){

        case SPELL_BLOB:
            myspells[newspellid].id = rand()%blobs_count+1;
            myspells[newspellid].current_defense = myblobs[myspells[newspellid].id].defense;
            myspells[newspellid].uses = myblobs[myspells[newspellid].id].uses;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Spell %d added for player %d [type %d] : %d => %s", newspellid,
                myspells[newspellid].player_id, myspells[newspellid].spell_type,
                myspells[newspellid].id, myblobs[myspells[newspellid].id].name)
            );
            break;

        case SPELL_WALL:
            myspells[newspellid].id = rand()%walls_count+1;
            myspells[newspellid].current_defense = mywalls[myspells[newspellid].id].defense;
            if(mywalls[myspells[newspellid].id].turnlimit == -1) {
                myspells[newspellid].turnlimit = rand()%ROUNDTURN_EXPIRE_RAND+1;
            } else {
                /* Add 1 as it will expire on turn 1 */
                myspells[newspellid].turnlimit = mywalls[myspells[newspellid].id].turnlimit+1;
            }
            myspells[newspellid].uses = mywalls[myspells[newspellid].id].uses;
            myspells[newspellid].genspells = mywalls[myspells[newspellid].id].genspells;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Spell %d added for player %d [type %d] : %d => %s", newspellid,
                myspells[newspellid].player_id, myspells[newspellid].spell_type,
                myspells[newspellid].id, mywalls[myspells[newspellid].id].name)
            );
            break;

        case SPELL_MAGIC_BALANCE:
            myspells[newspellid].id = rand()%magic_balance_count+1;
            myspells[newspellid].current_defense = 0;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Spell %d added for player %d [type %d] : %d => %s", newspellid,
                myspells[newspellid].player_id, myspells[newspellid].spell_type,
                myspells[newspellid].id, mymagic_balance[myspells[newspellid].id].name)
            );
            break;

        case SPELL_MAGIC_ATTRIB:
            myspells[newspellid].id = rand()%magic_spell_attrib_count+1;
            myspells[newspellid].current_defense = 0;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Spell %d added for player %d  [type %d] : %d => %s", newspellid,
                myspells[newspellid].player_id, myspells[newspellid].spell_type,
                myspells[newspellid].id, mymagic_spell_attrib[myspells[newspellid].id].name)
            );
            break;

        case SPELL_MAGIC_UPGRADE:
            myspells[newspellid].id = rand()%magic_upgrade_count+1;
            myspells[newspellid].current_defense = 0;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Spell %d added for player %d [type %d] : %d => %s", newspellid,
                myspells[newspellid].player_id,myspells[newspellid].spell_type,
                myspells[newspellid].id, mymagic_upgrade[myspells[newspellid].id].name)
            );
            break;

        case SPELL_TREE:
            myspells[newspellid].id = rand()%trees_count+1;
            myspells[newspellid].current_defense = mytrees[myspells[newspellid].id].defense;
            if(mytrees[myspells[newspellid].id].turnlimit == -1) {
                mytrees[newspellid].turnlimit = rand()%ROUNDTURN_EXPIRE_RAND+1;
            } else {
                mytrees[newspellid].turnlimit = mytrees[myspells[newspellid].id].turnlimit+1; /* Add 1 as it will expire on turn 1 */
            }
            myspells[newspellid].uses = mytrees[myspells[newspellid].id].uses;
            myspells[newspellid].genspells = mytrees[myspells[newspellid].id].genspells;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Spell %d added for player %d [type %d] : %d => %s", newspellid,
                myspells[newspellid].player_id, myspells[newspellid].spell_type,
                myspells[newspellid].id, mytrees[myspells[newspellid].id].name)
            );
            break;

        case SPELL_MAGIC_RANGED:
            myspells[newspellid].id = rand()%magic_ranged_count+1;
            myspells[newspellid].current_defense = 0;
            myspells[newspellid].uses = mymagic_ranged[myspells[newspellid].id].uses;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Spell %d added for player %d [type %d] : %d => %s", newspellid,
                myspells[newspellid].player_id, myspells[newspellid].spell_type,
                myspells[newspellid].id, mymagic_ranged[myspells[newspellid].id].name)
            );
            break;

        case SPELL_MONSTER:
        default:
            myspells[newspellid].spell_type = SPELL_MONSTER;
            myspells[newspellid].id = rand()%monsters_count+1;
            myspells[newspellid].current_defense = mymonsters[myspells[newspellid].id].defense;
            if(mymonsters[myspells[newspellid].id].undead){ myspells[newspellid].undead = true;}
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Spell %d added for player %d [type %d] : %d => %s", newspellid,
                myspells[newspellid].player_id, myspells[newspellid].spell_type,
                myspells[newspellid].id, mymonsters[myspells[newspellid].id].name)
            );
            break;
    }

    sprintf(infobar_text,"%s received a new spell!",myplayers[myspells[newspellid].player_id].name);
    beepmsg = true;
    history_add(infobar_text);

    myworld.total_spells++;
    myplayers[player_id].total_spells++;
    myplayers[player_id].spells[myplayers[player_id].total_spells] = newspellid;

    #ifdef WITH_NET
    if(net_enable){
        net_beepmsg(CE_NET_SOCKET_ALL);
        net_wait = false;
    }
    #endif

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Total Spells in world : %d. Total Spells for player %d : %d", myworld.total_spells,
        myspells[newspellid].player_id, myplayers[player_id].total_spells)
    );
}

/*
#################
#..cast_spell().#
#################
*/
void cast_spell (void)
{
    int p = myworld.current_player - 1;
    int u;
    int s = myplayers[p].spells[myplayers[p].selected_spell];
    int castroll = 0;
    int castprob = 0;
    int balanceroll, balancechance;
    int top_layer = itemstack_top(myworld.cursor[0],myworld.cursor[1]);
    char histtext[255];

    memset(histtext,'\0',sizeof(histtext));

    switch(myspells[s].spell_type){

        case SPELL_BLOB:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Attempting to cast spell : %s [Bal %d]",myblobs[myspells[s].id].name,
                myblobs[myspells[s].id].balance)
            );

            srand(time(NULL));
            castroll = rand()%9;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Casting roll was %d",castroll));

            castprob = myblobs[myspells[s].id].casting_prob;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Spell Success probability is %d",castprob));

            /*
            ###########################
            #..Balance.bonus/negative.#
            ###########################
            */
            /* Law Spell in a Law World */
            if(myworld.balance > 0 && myblobs[myspells[s].id].balance > 0) {

                balancechance = myworld.balance;
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Bonus [Law Spell in a Law World] : %d",balanceroll));
                castprob += balanceroll;

            /* Law Spell in a Chaos World   */
            } else if(myworld.balance < 0 && myblobs[myspells[s].id].balance > 0) {

                balancechance = myworld.balance;
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Negative [Law Spell in a Chaos World] : %d",balanceroll));
                castprob -= balanceroll;

            /* Chaos Spell in a Chaos World */
            } else if(myworld.balance < 0 && myblobs[myspells[s].id].balance < 0) {

                balancechance = myworld.balance - (myworld.balance + myworld.balance);
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Bonus [Chaos Spell in a Chaos World] : %d",balanceroll));
                castprob += balanceroll;

            /* Chaos Spell in a Law World   */
            } else if(myworld.balance > 0 && myblobs[myspells[s].id].balance < 0) {

                balancechance = myworld.balance - (myworld.balance + myworld.balance);
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Negative [Chaos Spell in a Law World] : %d",balanceroll));
                castprob -= balanceroll;
            }

            /*
            ##########################
            #..Success/Fail.Casting?.#
            ##########################
            */
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Spell uses left : %d (max : %d)",myspells[s].uses,
                myblobs[myspells[s].id].uses)
            );

            if(castprob >= castroll || myspells[s].uses < myblobs[myspells[s].id].uses){
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Spell Casting Successful (%d vs %d) or multiple uses availible",
                    castprob,castroll)
                );

                /* Fire eats corpses */
                if(isspelldead(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]) &&
                    myblobs[myspells[s].id].devourer != 0){
                    top_layer++;
                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                        "Spell Casting on top of corpse (layer %d)",
                        top_layer)
                    );
                }

                myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] = s;

                myspells[s].current_pos[0] = myworld.cursor[0];
                myspells[s].current_pos[1] = myworld.cursor[1];
                myspells[s].current_pos[2] = top_layer;

                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Player %d spell [%d] Spawned Blob [%d]: %s (%s) [bal %d] @ %dx%dx%d (owner : %d)",
                    p, s, myspells[s].id,myblobs[myspells[s].id].name,
                    myblobs[myspells[s].id].disp,
                    myblobs[myspells[s].id].balance,myworld.cursor[0],myworld.cursor[1],
                    top_layer,myspells[s].player_id)
                );

                sprintf(infobar_text,"Successful");
                beepmsg = true;

                sprintf(histtext,"%s Successfully Casts %s",
                    myplayers[p].name,myblobs[myspells[s].id].name);
                history_add(histtext);

                myspells[s].beenmoved = true;

            } else {
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Failed to cast spell (%d vs %d)",castprob,castroll));
                myspells[s].beencast = true;
                myspells[s].dead = true;
                myspells[s].uses = 0;

                sprintf(infobar_text,"Failed");
                beepmsg = true;

                sprintf(histtext,"%s Fails to Cast %s",
                    myplayers[p].name,myblobs[myspells[s].id].name);
                history_add(histtext);
            }
            myspells[s].beencast = true;
            break;

        case SPELL_WALL:
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Attempting to cast spell : %s [Bal %d]",mywalls[myspells[s].id].name,
                mywalls[myspells[s].id].balance)
            );

            srand(time(NULL));
            castroll = rand()%9;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Casting roll was %d",castroll));

            castprob = mywalls[myspells[s].id].casting_prob;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Spell Success probability is %d",castprob));

            /*
            ###########################
            #..Balance.bonus/negative.#
            ###########################
            */
            /* Law Spell in a Law World */
            if(myworld.balance > 0 && mywalls[myspells[s].id].balance > 0) {

                balancechance = myworld.balance;
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Bonus [Law Spell in a Law World] : %d",balanceroll));
                castprob += balanceroll;

            /* Law Spell in a Chaos World   */
            } else if(myworld.balance < 0 && mywalls[myspells[s].id].balance > 0) {

                balancechance = myworld.balance;
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Negative [Law Spell in a Chaos World] : %d",balanceroll));
                castprob -= balanceroll;

            /* Chaos Spell in a Chaos World */
            } else if(myworld.balance < 0 && mywalls[myspells[s].id].balance < 0) {

                balancechance = myworld.balance - (myworld.balance + myworld.balance);
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Bonus [Chaos Spell in a Chaos World] : %d",balanceroll));
                castprob += balanceroll;

            /* Chaos Spell in a Law World   */
            } else if(myworld.balance > 0 && mywalls[myspells[s].id].balance < 0) {

                balancechance = myworld.balance - (myworld.balance + myworld.balance);
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Negative [Chaos Spell in a Law World] : %d",balanceroll));
                castprob -= balanceroll;
            }

            /*
            ##########################
            #..Success/Fail.Casting?.#
            ##########################
            */
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Spell uses left : %d (max : %d)",myspells[s].uses,
                mywalls[myspells[s].id].uses)
            );

            if(castprob >= castroll || myspells[s].uses < mywalls[myspells[s].id].uses){
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Spell Casting Successful (%d vs %d) or multiple uses availible",
                    castprob,castroll)
                );

                /* Assoc Function? */
                if(mywalls[myspells[s].id].assoc_func > 0){
                    lookupwallfunction(mywalls[myspells[s].id].assoc_func);
                } else {
                    myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] = s;

                    myspells[s].current_pos[0] = myworld.cursor[0];
                    myspells[s].current_pos[1] = myworld.cursor[1];
                    myspells[s].current_pos[2] = top_layer;

                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                        "Player %d spell [%d] Spawned Wall [%d]: %s (%s) [bal %d] @ %dx%d (owner : %d)",
                        p,  s,  myspells[s].id, mywalls[myspells[s].id].name,
                        mywalls[myspells[s].id].disp,
                        mywalls[myspells[s].id].balance, myworld.cursor[0],myworld.cursor[1],
                        myspells[s].player_id)
                    );

                    sprintf(infobar_text,"Successful");
                    beepmsg = true;
                    myspells[s].beenmoved = true;
                    myspells[s].beencast = true;

                    sprintf(histtext,"%s Successfully Casts %s",
                        myplayers[p].name,mywalls[myspells[s].id].name);
                    history_add(histtext);
                }

            } else {
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Failed to cast spell (%d vs %d)",castprob,castroll));
                myspells[s].beencast = true;
                myspells[s].dead = true;
                myspells[s].uses = 0;

                sprintf(infobar_text,"Failed");
                beepmsg = true;

                sprintf(histtext,"%s Fails to Cast %s",
                    myplayers[p].name,mywalls[myspells[s].id].name);
                history_add(histtext);
            }
            break;

        case SPELL_MAGIC_BALANCE:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Attempting to cast spell : %s [Bal %d]",
                mymagic_balance[myspells[s].id].name,
                mymagic_balance[myspells[s].id].balance)
            );

            srand(time(NULL));
            castroll = rand()%9;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Casting roll was %d",castroll));

            castprob = mymagic_balance[myspells[s].id].casting_prob;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Spell Success probability is %d",castprob));

            /*
            ###########################
            #..Balance.bonus/negative.#
            ###########################
            */
            /* Law Spell in a Law World */
            if(myworld.balance > 0 && mymagic_balance[myspells[s].id].balance > 0) {

                balancechance = myworld.balance;
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Bonus [Law Spell in a Law World] : %d",
                    balanceroll)
                );

                castprob += balanceroll;

            /* Law Spell in a Chaos World   */
            } else if(myworld.balance < 0 && mymagic_balance[myspells[s].id].balance > 0) {

                balancechance = myworld.balance;
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Negative [Law Spell in a Chaos World] : %d",
                    balanceroll)
                );

                castprob -= balanceroll;

            /* Chaos Spell in a Chaos World */
            } else if(myworld.balance < 0 && mymagic_balance[myspells[s].id].balance < 0) {

                balancechance = myworld.balance - (myworld.balance + myworld.balance);
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Bonus [Chaos Spell in a Chaos World] : %d",
                    balanceroll)
                );

                castprob += balanceroll;

            /* Chaos Spell in a Law World */
            } else if(myworld.balance > 0 && mymagic_balance[myspells[s].id].balance < 0) {

                balancechance = myworld.balance - (myworld.balance + myworld.balance);
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Negative [Chaos Spell in a Law World] : %d",
                    balanceroll)
                );

                castprob -= balanceroll;
            }

            /*
            ##########################
            #..Success/Fail.Casting?.#
            ##########################
            */
            if(castprob >= castroll){

                sprintf(infobar_text,"Successful");
                beepmsg = true;

                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Spell Casting Successful (%d vs %d)",castprob,castroll));
                applymagicbalance(myspells[s].id);

                sprintf(histtext,"%s Successfully Casts %s",
                    myplayers[p].name,mymagic_balance[myspells[s].id].name);
                history_add(histtext);

            } else {
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Failed to cast spell (%d vs %d)",castprob,castroll));
                myspells[s].dead = true;

                sprintf(infobar_text,"Failed");
                beepmsg = true;

                sprintf(histtext,"%s Fails to Cast %s",
                    myplayers[p].name,mymagic_balance[myspells[s].id].name);
                history_add(histtext);
            }

            myspells[s].beencast = true;
            break;

        case SPELL_MAGIC_ATTRIB:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Attempting to cast spell : %s [Bal %d]",
                mymagic_spell_attrib[myspells[s].id].name,
                mymagic_spell_attrib[myspells[s].id].balance)
            );

            srand(time(NULL));
            castroll = rand()%7;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Casting roll was %d",castroll));

            castprob = mymagic_spell_attrib[myspells[s].id].casting_prob;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Spell Success probability is %d",castprob));

            /*
            ###########################
            #..Balance.bonus/negative.#
            ###########################
            */
            /* Law Spell in a Law World */
            if(myworld.balance > 0 && mymagic_spell_attrib[myspells[s].id].balance > 0) {

                balancechance = myworld.balance;
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Bonus [Law Spell in a Law World] : %d",
                    balanceroll)
                );

                castprob += balanceroll;

            /* Law Spell in a Chaos World   */
            } else if(myworld.balance < 0 && mymagic_spell_attrib[myspells[s].id].balance > 0) {

                balancechance = myworld.balance;
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Negative [Law Spell in a Chaos World] : %d",
                    balanceroll)
                );

                castprob -= balanceroll;

            /* Chaos Spell in a Chaos World */
            } else if(myworld.balance < 0 && mymagic_spell_attrib[myspells[s].id].balance < 0) {

                balancechance = myworld.balance - (myworld.balance + myworld.balance);
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Bonus [Chaos Spell in a Chaos World] : %d",
                        balanceroll)
                );

                castprob += balanceroll;

            /* Chaos Spell in a Law World */
            } else if(myworld.balance > 0 && mymagic_spell_attrib[myspells[s].id].balance < 0) {

                balancechance = myworld.balance - (myworld.balance + myworld.balance);
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Negative [Chaos Spell in a Law World] : %d",
                    balanceroll)
                );

                castprob -= balanceroll;
            }

            /*
            ##########################
            #..Success/Fail.Casting?.#
            ##########################
            */
            if(castprob >= castroll){
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Spell Casting Successful (%d vs %d)",castprob,castroll));

                mod_spell_attributes(myspells[s].id);

            } else {
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Failed to cast spell (%d vs %d)",castprob,castroll));
                myspells[s].dead = true;

                sprintf(infobar_text,"Failed");
                beepmsg = true;

                sprintf(histtext,"%s Fails to Cast %s",
                    myplayers[p].name,mymagic_spell_attrib[myspells[s].id].name);
                history_add(histtext);
            }
            break;

        case SPELL_MAGIC_UPGRADE:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Attempting to cast spell : %s [Bal %d]",
                mymagic_upgrade[myspells[s].id].name,
                mymagic_upgrade[myspells[s].id].balance)
            );

            srand(time(NULL));
            castroll = rand()%9;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Casting roll was %d",castroll));

            castprob = mymagic_upgrade[myspells[s].id].casting_prob;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Spell Success probability is %d",castprob));

            /*
            ###########################
            #..Balance.bonus/negative.#
            ###########################
            */
            /* Law Spell in a Law World */
            if(myworld.balance > 0 && mymagic_upgrade[myspells[s].id].balance > 0) {

                balancechance = myworld.balance;
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Bonus [Law Spell in a Law World] : %d",
                    balanceroll)
                );

                castprob += balanceroll;

            /* Law Spell in a Chaos World   */
            } else if(myworld.balance < 0 && mymagic_upgrade[myspells[s].id].balance > 0) {

                balancechance = myworld.balance;
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Negative [Law Spell in a Chaos World] : %d",
                    balanceroll)
                );

                castprob -= balanceroll;

            /* Chaos Spell in a Chaos World */
            } else if(myworld.balance < 0 && mymagic_upgrade[myspells[s].id].balance < 0) {

                balancechance = myworld.balance - (myworld.balance + myworld.balance);
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Bonus [Chaos Spell in a Chaos World] : %d",
                    balanceroll)
                );

                castprob += balanceroll;

            /* Chaos Spell in a Law World   */
            } else if(myworld.balance > 0 && mymagic_upgrade[myspells[s].id].balance < 0) {

                balancechance = myworld.balance - (myworld.balance + myworld.balance);
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Negative [Chaos Spell in a Law World] : %d",
                    balanceroll)
                );

                castprob -= balanceroll;
            }

            /*
            ##########################
            #..Success/Fail.Casting?.#
            ##########################
            */
            if(castprob >= castroll){
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Spell Casting Successful (%d vs %d)",castprob,castroll));

                for(u=0;u<MAX_MAGIC_UPGRADE;u++){
                    if(myplayers[p].upgrades[u] == 0){
                        myplayers[p].upgrades[u] = myspells[s].id;
                        applymagicupgrade(myspells[s].id);
                        u=MAX_MAGIC_UPGRADE;
                    } else if(myplayers[p].upgrades[u] == myspells[s].id){
                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Spell Upgrade already in use! skipping spell"));
                        u=MAX_MAGIC_UPGRADE;
                    }
                }

                sprintf(infobar_text,"Successful");
                beepmsg = true;

                sprintf(histtext,"%s Successfully Casts %s",
                    myplayers[p].name,mymagic_upgrade[myspells[s].id].name);
                history_add(histtext);
            } else {
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Failed to cast spell (%d vs %d)",castprob,castroll));
                myspells[s].dead = true;

                sprintf(infobar_text,"Failed");
                beepmsg = true;

                sprintf(histtext,"%s Fails to Cast %s",
                    myplayers[p].name,mymagic_upgrade[myspells[s].id].name);
                history_add(histtext);
            }

            myspells[s].beencast = true;
            break;

        case SPELL_MAGIC_SPECIAL:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Attempting to cast spell : %s",mymagic_special[myspells[s].id].name));
            lookupspecialfunction(myspells[s].id);
            break;

        case SPELL_TREE:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Attempting to cast spell : %s [Bal %d]",mytrees[myspells[s].id].name,
                mytrees[myspells[s].id].balance)
            );

            srand(time(NULL));
            castroll = rand()%9;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Casting roll was %d",castroll));

            castprob = mytrees[myspells[s].id].casting_prob;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Spell Success probability is %d",castprob));

            /*
            ###########################
            #..Balance.bonus/negative.#
            ###########################
            */
            /* Law Spell in a Law World */
            if(myworld.balance > 0 && mytrees[myspells[s].id].balance > 0) {

                balancechance = myworld.balance;
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Bonus [Law Spell in a Law World] : %d",balanceroll));
                castprob += balanceroll;

            /* Law Spell in a Chaos World   */
            } else if(myworld.balance < 0 && mytrees[myspells[s].id].balance > 0) {

                balancechance = myworld.balance;
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Negative [Law Spell in a Chaos World] : %d",balanceroll));
                castprob -= balanceroll;

            /* Chaos Spell in a Chaos World */
            } else if(myworld.balance < 0 && mytrees[myspells[s].id].balance < 0) {

                balancechance = myworld.balance - (myworld.balance + myworld.balance);
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Bonus [Chaos Spell in a Chaos World] : %d",balanceroll));
                castprob += balanceroll;

            /* Chaos Spell in a Law World   */
            } else if(myworld.balance > 0 && mytrees[myspells[s].id].balance < 0) {

                balancechance = myworld.balance - (myworld.balance + myworld.balance);
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Negative [Chaos Spell in a Law World] : %d",balanceroll));
                castprob -= balanceroll;
            }

            /*
            ##########################
            #..Success/Fail.Casting?.#
            ##########################
            */
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Spell uses left : %d (max : %d)",myspells[s].uses,
                mytrees[myspells[s].id].uses)
            );

            if(castprob >= castroll || myspells[s].uses < mytrees[myspells[s].id].uses){
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Spell Casting Successful (%d vs %d) or multiple uses availible",
                    castprob,castroll)
                );

                lookuptreefunction(myspells[s].id);

                /* History message in tree.c */

            } else {
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Failed to cast spell (%d vs %d)",castprob,castroll));
                myspells[s].beencast = true;
                myspells[s].dead = true;
                myspells[s].uses = 0;

                sprintf(infobar_text,"Failed");
                beepmsg = true;

                sprintf(histtext,"%s Fails to Cast %s",
                    myplayers[p].name,mytrees[myspells[s].id].name);
                history_add(histtext);
            }
            break;

        case SPELL_MAGIC_RANGED:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Attempting to cast spell : %s [Bal %d]",
                mymagic_ranged[myspells[s].id].name,mymagic_ranged[myspells[s].id].balance)
            );

            srand(time(NULL));
            castroll = rand()%9;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Casting roll was %d",castroll));

            castprob = mymagic_ranged[myspells[s].id].casting_prob;
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Spell Success probability is %d",castprob));

            /*
            ###########################
            #..Balance.bonus/negative.#
            ###########################
            */
            /* Law Spell in a Law World */
            if(myworld.balance > 0 && mymagic_ranged[myspells[s].id].balance > 0) {

                balancechance = myworld.balance;
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Bonus [Law Spell in a Law World] : %d",balanceroll));
                castprob += balanceroll;

            /* Law Spell in a Chaos World   */
            } else if(myworld.balance < 0 && mymagic_ranged[myspells[s].id].balance > 0) {

                balancechance = myworld.balance;
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Negative [Law Spell in a Chaos World] : %d",balanceroll));
                castprob -= balanceroll;

            /* Chaos Spell in a Chaos World */
            } else if(myworld.balance < 0 && mymagic_ranged[myspells[s].id].balance < 0) {

                balancechance = myworld.balance - (myworld.balance + myworld.balance);
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Bonus [Chaos Spell in a Chaos World] : %d",balanceroll));
                castprob += balanceroll;

            /* Chaos Spell in a Law World   */
            } else if(myworld.balance > 0 && mymagic_ranged[myspells[s].id].balance < 0) {

                balancechance = myworld.balance - (myworld.balance + myworld.balance);
                balanceroll = (rand()%balancechance+1)/2;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Balance Alignment Negative [Chaos Spell in a Law World] : %d",balanceroll));
                castprob -= balanceroll;
            }

            /*
            ##########################
            #..Success/Fail.Casting?.#
            ##########################
            */
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Spell uses left : %d (max : %d)",myspells[s].uses,
                mymagic_ranged[myspells[s].id].uses)
            );

            if(castprob >= castroll ||
                (mymagic_ranged[myspells[s].id].uses > 1 && myspells[s].uses < mymagic_ranged[myspells[s].id].uses)){

                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Spell Casting Successful (%d vs %d)",castprob,castroll));

                castmagic_ranged(myspells[s].id);

                myspells[s].uses--;

                if(myspells[s].uses < 1)
                    myspells[s].beencast = true;

                sprintf(histtext,"%s Casts %s",
                    myplayers[p].name,mymagic_ranged[myspells[s].id].name);
                history_add(histtext);

            } else {
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Failed to cast spell (%d vs %d)",castprob,castroll));

                sprintf(infobar_text,"Failed");
                beepmsg = true;

                sprintf(histtext,"%s Fails to Cast %s",
                    myplayers[p].name,mymagic_ranged[myspells[s].id].name);
                history_add(histtext);

                myspells[s].beencast = true;
                myspells[s].uses = 0;
            }
            break;

        case SPELL_MONSTER:
        default:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Attempting to cast spell : %s [Bal %d]",
                mymonsters[myspells[s].id].name,
                mymonsters[myspells[s].id].balance
            ));

            /*
            ######################
            #..Cast.as.Illusion?.#
            ######################
            */
            if(myspells[s].illusion) {

                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Spell : %s is being cast as an illusion, bypassing casting roll",
                    mymonsters[myspells[s].id].name)
                );
            /*
            ##########################################
            #..Not.illusion.roll.against.probability.#
            ##########################################
            */
            } else {
                srand(time(NULL));
                castroll = rand()%9;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Casting roll was %d",castroll));

                castprob = mymonsters[myspells[s].id].casting_prob;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Spell Success probability is %d",castprob));

                /*
                ###########################
                #..Balance.bonus/negative.#
                ###########################
                */

                /* Law Spell in a Law World */
                if(myworld.balance > 0 && mymonsters[myspells[s].id].balance > 0) {

                    balancechance = myworld.balance;
                    balanceroll = (rand()%balancechance+1)/2;
                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                        "Balance Alignment Bonus [Law Spell in a Law World] : %d",balanceroll));
                    castprob += balanceroll;

                /* Law Spell in a Chaos World   */
                } else if(myworld.balance < 0 && mymonsters[myspells[s].id].balance > 0) {

                    balancechance = myworld.balance;
                    balanceroll = (rand()%balancechance+1)/2;
                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                        "Balance Alignment Negative [Law Spell in a Chaos World] : %d",balanceroll));
                    castprob -= balanceroll;

                /* Chaos Spell in a Chaos World */
                } else if(myworld.balance < 0 && mymonsters[myspells[s].id].balance < 0) {

                    balancechance = myworld.balance - (myworld.balance + myworld.balance);
                    balanceroll = (rand()%balancechance+1)/2;
                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                        "Balance Alignment Bonus [Chaos Spell in a Chaos World] : %d",balanceroll));
                    castprob += balanceroll;

                /* Chaos Spell in a Law World   */
                } else if(myworld.balance > 0 && mymonsters[myspells[s].id].balance < 0) {

                    balancechance = myworld.balance - (myworld.balance + myworld.balance);
                    balanceroll = (rand()%balancechance+1)/2;
                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                        "Balance Alignment Negative [Chaos Spell in a Law World] : %d",balanceroll));
                    castprob -= balanceroll;

                }
            }

            /*
            ##########################
            #..Success/Fail.Casting?.#
            ##########################
            */
            if(castprob >= castroll || myspells[s].illusion){
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Spell Casting Successful (%d vs %d)",castprob,castroll));

                if(isspelldead(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer])){ top_layer++;}

                myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] = s;

                myspells[s].current_pos[0] = myworld.cursor[0];
                myspells[s].current_pos[1] = myworld.cursor[1];
                myspells[s].current_pos[2] = top_layer;

                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Player %d spell [%d] Spawned monster [%d]: %s (%s) [bal %d] @ %dx%d [move_range %d] (owner : %d)",
                    p,  s,  myspells[s].id, mymonsters[myspells[s].id].name,
                    mymonsters[myspells[s].id].disp,
                    mymonsters[myspells[s].id].balance, myworld.cursor[0], myworld.cursor[1],
                    mymonsters[myspells[s].id].move_range,  myspells[s].player_id)
                );

                sprintf(infobar_text,"Successful");
                beepmsg = true;

                sprintf(histtext,"%s Successfully Casts %s",
                    myplayers[p].name,mymonsters[myspells[s].id].name);
                history_add(histtext);

            } else {
                myspells[s].dead = true;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Failed to cast spell (%d vs %d)",castprob,castroll));
                sprintf(infobar_text,"Failed");
                beepmsg = true;

                sprintf(histtext,"%s Fails to Cast %s",
                    myplayers[p].name,mymonsters[myspells[s].id].name);
                history_add(histtext);
            }

            myspells[s].beencast = true;
            break;
    }

    if(myspells[s].spell_type != SPELL_MAGIC_RANGED && myspells[s].spell_type != SPELL_TREE
        && myspells[s].uses > 0 && myspells[s].beencast){

        myplayers[p].selected_spell = 0;
    }
}

/*
======================================================================== States =
*/

/*
###########################
#..getlivingplayercount().#
###########################
*/
static int getlivingplayercount (void)
{
    int p;
    int total = 0;

    for(p=0;p<myworld.players;p++){
        if(!isspelldead(p+10)){
            console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
                "Player %d is Alive",p));
            total++;
        } else {
            console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
                "Player %d is Dead",p));
        }
    }
    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
        "returning total players = %d",total));
    return total;
}

/*
###########################
#..getfirstlivingplayer().#
###########################
*/
static int getfirstlivingplayer (void)
{
    int p, alive;
    for(p=0;p<myworld.players;p++){
        if(!isspelldead(p+10)){
            alive = p+1;
            p = myworld.players;
        }
    }
    console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
        "First Living Player id = %d",alive));
    return alive;
}

/*
##########################
#..getlastlivingplayer().#
##########################
*/
static int getlastlivingplayer (void)
{
    int p,last;
    for(p=0;p<myworld.players;p++)
        if(!isspelldead(p+10))
            last = p;

    console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
        "Last Living Player id = %d",last));
    return last;
}

/*
######################
#..allplayersready().#
######################
*/
bool allplayersready (void)
{
    int p;
    for(p=0;p<myworld.players;p++)
        if(!myplayers[p].ready)
            return false;
    return true;
}

/*
######################
#..setplayersready().#
######################
*/
void setplayersready (bool readystate)
{
    int p;
    for(p=0;p<myworld.players;p++)
        if(!isspelldead(p+10))
            myplayers[p].ready=readystate;
}

/*
######################
#..skipdeadplayers().#
######################
*/
void skipdeadplayers (void)
{
    int p;
    char histtext[255];

    if(myworld.mode > CE_WORLD_MODE_SETUP && myworld.mode < CE_WORLD_MODE_ENDGAME){
        if(getlivingplayercount() > 1){
            for(p=myworld.current_player-1;p<myworld.players;p++){
                if(isspelldead(p+10)){

                    myworld.current_player++;
                    if(myworld.current_player > myworld.players) {
                        myworld.current_player = getfirstlivingplayer();

                        myworld.mode++;
                        if(myworld.mode > CE_WORLD_MODE_MOVE){

                            #ifdef WITH_NET
                            if(net_enable)
                                myworld.mode=CE_WORLD_MODE_POSTROUND;
                            #else
                            myworld.mode = CE_WORLD_MODE_SELECTSPELLS;
                            reset_world();
                            #endif
                        }
                        myworld.submode = 0;
                    }
                    myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
                    myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];

                    /* Tell Display to update */
                    forceupdate = true;
                    #ifdef WITH_NET
                    net_wait = false;
                    #endif
                    return;
                } else {

                    /* Found living player! */
                    myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
                    myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];

                    /* Tell Display to update */
                    forceupdate = true;
                    #ifdef WITH_NET
                    net_wait = false;
                    #endif
                    return;
                }
            }
        } else {    /* Game Over! */
            myworld.mode = CE_WORLD_MODE_ENDGAME;
            myworld.current_player = getlastlivingplayer();

            memset(histtext,'\0',sizeof(histtext));
            sprintf(histtext,"Game Over! %s has won the battle!",
                myplayers[myworld.current_player].name);
            history_add(histtext);

            /* Tell Display to update */
            forceupdate = true;
            #ifdef WITH_NET
            net_wait = false;
            #endif
        }
    }
}

/*
##################
#..isspelldead().#
##################
*/
bool isspelldead (int spellid)
{
    return myspells[spellid].dead;
}

/*
========================================================================= World =
*/

/*
#################
#..init_world().#
#################
*/
void init_world (void)
{
    int i,j,l;

    myworld.balance             = 0;
    myworld.mode                = 0;
    myworld.submode             = 0;
    myworld.players             = 0;
    myworld.current_player      = 1;
    myworld.arenasize           = 0;
    myworld.selected_item[0]    = 0;
    myworld.selected_item[1]    = 0;
    myworld.total_spells        = 0;
    myworld.beenreset           = false;

    monsters_count              = 0;
    trees_count                 = 0;
    walls_count                 = 0;
    blobs_count                 = 0;
    magic_special_count         = 0;
    magic_upgrade_count         = 0;
    magic_balance_count         = 0;
    magic_spell_attrib_count    = 0;
    magic_ranged_count          = 0;

    memset(myspells, 0, sizeof(spells)*MAX_SPELLS);
    memset(myplayers, 0, sizeof(player)*MAX_PLAYERS);
    memset(mymonsters, 0, sizeof(monster)*MAX_MONSTERS);
    memset(mytrees, 0, sizeof(tree)*MAX_TREES);
    memset(mywalls, 0, sizeof(wall)*MAX_WALLS);
    memset(myblobs, 0, sizeof(blob)*MAX_BLOBS);
    memset(mymagic_special, 0, sizeof(magic_special)*MAX_MAGIC_SPECIAL);
    memset(mymagic_upgrade, 0, sizeof(magic_upgrade)*MAX_MAGIC_UPGRADE);
    memset(mymagic_balance, 0, sizeof(magic_balance)*MAX_MAGIC_BALANCE);
    memset(mymagic_spell_attrib, 0, sizeof(magic_spell_attrib)*MAX_MAGIC_SPELL_ATTRIB);
    memset(mymagic_ranged, 0, sizeof(magic_ranged)*MAX_MAGIC_RANGED);

    memset(myhistory, 0, sizeof(history_log)*MAX_HISTORY);
    memset(mychat, 0, sizeof(chat_log)*MAX_CHAT);

    for(i=0;i<arenas[MAX_X][0];i++)
        for(j=0;j<arenas[MAX_Y][1];j++)
            for(l=0;l<MAX_LAYERS;l++)
                myworld.layout[i][j][l] = 0;
}

/*
##################
#..reset_world().#
##################
*/
void reset_world (void)
{
    int i;
    float balcalc;
    bool growthmsg = false;

    myworld.balance = 0;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Resetting Players"));

    for(i=10;i<myworld.players+11;i++){
        if(!isspelldead(i)){
            myplayers[i-10].selected_spell = 0;
            myspells[i].beenmoved = false;
        }
        myspells[i].current_defense = myplayers[myspells[i].player_id].defense;
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Resetting Spells"));

    for(i=100;i<myworld.total_spells+101;i++){
        if(!isspelldead(i) && myspells[i].beencast && myspells[i].spell_type != SPELL_PLAYER){
            switch(myspells[i].spell_type){

                case SPELL_MONSTER:
                    myspells[i].beenmoved = false;
                    myworld.balance += mymonsters[myspells[i].id].balance;

                    myspells[i].current_defense = mymonsters[myspells[i].id].defense;
                    break;

                case SPELL_TREE:
                    myspells[i].beenmoved = false;

                    /* Expire this round? */
                    if(myspells[i].turnlimit == 1 && is_mounted(myspells[i].current_pos[0],myspells[i].current_pos[1])){
                        myspells[i].dead = true;
                        myspells[i].illusion = true;

                        /* Give player a spell before expiring? */
                        if(myspells[i].genspells)
                            givenewspell(myspells[i].player_id);

                        myworld.layout[myspells[i].current_pos[0]][myspells[i].current_pos[1]][myspells[i].current_pos[2]] = 0;
                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Spell [%d] Expired",i));
                        continue;
                    } else if(myspells[i].turnlimit > 1 && is_mounted(myspells[i].current_pos[0],myspells[i].current_pos[1])){
                        myspells[i].turnlimit--;
                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Spell [%d] Will Expire in %d turn(s)",i,myspells[i].turnlimit));
                    }

                    /* Only take balance from 1 of the trees */
                    myworld.balance +=
                        mytrees[myspells[i].id].balance / mytrees[myspells[i].id].uses;

                    myspells[i].current_defense = mytrees[myspells[i].id].defense;
                    break;

                case SPELL_MAGIC_UPGRADE:
                    myworld.balance += mymagic_upgrade[myspells[i].id].balance;
                    break;

                case SPELL_MAGIC_BALANCE:
                    myworld.balance += mymagic_balance[myspells[i].id].balance;
                    break;

                case SPELL_WALL:
                    /* Expire this round? */
                    if(myspells[i].turnlimit == 1){
                        myspells[i].dead = true;
                        myspells[i].illusion = true;

                        /* Give player a spell before expiring? */
                        if(myspells[i].genspells)
                            givenewspell(myspells[i].player_id);

                        myworld.layout[myspells[i].current_pos[0]][myspells[i].current_pos[1]][myspells[i].current_pos[2]] = 0;
                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Spell [%d] Expired",i));
                        continue;
                    } else if(myspells[i].turnlimit > 1){
                        myspells[i].turnlimit--;
                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                            "Spell [%d] Will Expire in %d turn(s)",i,myspells[i].turnlimit));
                    }

                    /* Only take balance from 1 of the walls */
                    myworld.balance +=
                        mywalls[myspells[i].id].balance / mywalls[myspells[i].id].uses;

                    myspells[i].current_defense = mywalls[myspells[i].id].defense;
                    break;

                case SPELL_BLOB:
                    /* Only take balance from 1 of the blobs */
                    myworld.balance +=
                        myblobs[myspells[i].id].balance / myblobs[myspells[i].id].uses;

                    myspells[i].current_defense = myblobs[myspells[i].id].defense;

                    if(!myspells[i].beenmoved) {/* aka beengrown =P */
                        grow_blob(i);
                        growthmsg = true;
                    }

                    /* Let it grow next turn */
                    myspells[i].beenmoved = false;
                    break;

                /* Non-balance related spells */
                case SPELL_MAGIC_ATTRIB:
                case SPELL_MAGIC_RANGED:
                    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
                        "Skipping spell [id %d / type %d] - Nothing to reset",i,
                        myspells[i].spell_type)
                    );
                    break;

                default:
                    console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                        "Unknown spell type tried to be reset [id %d / type %d]",i,
                        myspells[i].spell_type)
                    );
                    break;
            }
        } else {
            console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
                "Uncast or dead spell not reset [id %d / type %d]",i,
                myspells[i].spell_type)
            );
        }

        /* Remove all skip round flags */
        myspells[i].skipround = false;
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "world balance before reconjubulation %d",myworld.balance));
    balcalc = myworld.balance;

    if(myworld.balance > 0)
        myworld.balance = ceil(balcalc/2);
    else if (myworld.balance < 0)
        myworld.balance = floor(balcalc/2);

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Reconjubulating world balance to %d",myworld.balance));

    if(growthmsg){
        sprintf(infobar_text,"Blobs Grown");
        beepmsg = true;
    }

    setplayersready(false);
}

/*
===================================================================== GameLoop =
*/

/*
#####################
#..gameloop_local().#
#####################
*/
void gameloop_local(void)
{
    /* During casting round, skip players with no spells to cast */
    if(myworld.mode == CE_WORLD_MODE_CASTING &&
        myplayers[myworld.current_player-1].selected_spell == 0){

        myworld.current_player++;
        skipdeadplayers();
        if(myworld.current_player > myworld.players) {
            myworld.mode = CE_WORLD_MODE_MOVE;
            myworld.submode = 0;
            myworld.current_player = 1;
        }
        myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
        myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];
        forceupdate = true;
    }

    /* Post Round Funcs */
    if(myworld.mode == CE_WORLD_MODE_POSTROUND){

        /* Waiting for a message to decay? */
        if(!beepmsg){

            /* Don't reset multiple times! */
            if(!myworld.beenreset){
                reset_world();
                myworld.beenreset = true;
            }

            /* As mad as this looks, this allows
                reset_world() to set beepmsg */
            if(!beepmsg){
                myworld.mode = CE_WORLD_MODE_SELECTSPELLS;
                myworld.beenreset = false;
            }
        }

        forceupdate = true;
    }
}
