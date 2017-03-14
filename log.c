/***************************************************************
*  log.c                                           #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*                                                  #.....#     *
*  Internal Logging Functions                      #######     *
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

int logging_level = 0;
char log_message[LOGMSGLEN];
FILE *logfile;

extern world                myworld;
extern history_log          myhistory[MAX_HISTORY];
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

extern int arenas[MAX_ARENAS][2];

/*
########################
#..set_logging_level().#
########################
*/
void set_logging_level (int level)
{
    switch(level){
        case LOG_NONE:
        case LOG_ALL:
        case LOG_ERROR:
        case LOG_WARNING:
        case LOG_NOTICE:
        case LOG_DEBUG:
            logging_level = level;
            break;
        default :
            logging_level = LOG_NOTICE;
            break;
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,sprintf(log_message,
        "Debug Logging set to %d",logging_level));
}

/*
###################
#..open_logfile().#
###################
*/
void open_logfile (void)
{
    time_t tval;
    struct tm *now;
    char logfilename[26];

    memset(logfilename,'\0',sizeof(logfilename));

    tval = time(NULL);
    now = localtime(&tval);
    sprintf(logfilename,"chaos.%02d%02d%02d_%02d%02d%02d.log",
        now->tm_year, now->tm_mon, now->tm_mday,
        now->tm_hour, now->tm_min, now->tm_sec);

    logfile=fopen(logfilename,"a+");
}

/*
####################
#..close_logfile().#
####################
*/
void close_logfile (void)
{
    if(logfile==NULL)
        return;
    fclose(logfile);
}

/*
##################
#..console_log().#
##################
*/
void console_log (char *file, const char *func, int line,
    int log_level, int messagelen)
{
    time_t tval;
    struct tm *now;
    char level_text[12];

    if(logfile == NULL){
        set_logging_level(LOG_NONE);
        return;
    }

    if(log_level == LOG_NONE || messagelen < 1)
        return;

    if (log_level <= logging_level || log_level == LOG_ALL) {
        memset(level_text,'\0',sizeof(level_text));
        tval = time(NULL);
        now = localtime(&tval);

        switch(log_level){
            case LOG_ALL:
                sprintf(level_text,"ALL");
                break;
            case LOG_ERROR:
                sprintf(level_text,"ERROR");
                break;
            case LOG_WARNING:
                sprintf(level_text,"WARNING");
                break;
            case LOG_NOTICE:
                sprintf(level_text,"NOTICE");
                break;
            case LOG_DEBUG:
                sprintf(level_text,"DEBUG");
                break;
            default :
                sprintf(level_text,"UNKNOWN");
                break;
        }

        /*printf("[%d:%d:%d][%s] %s @ %s():%d - %s\n",now->tm_hour, now->tm_min,
            now->tm_sec,level_text,file,func,line,log_message);*/

        fprintf(logfile,"[%02d:%02d:%02d][%s] %s @ %s():%d - %s\n",now->tm_hour, now->tm_min,
            now->tm_sec,level_text,file,func,line,log_message);
    }
}

/*
################
#..dump_grid().#
################
*/
void dump_grid (void)
{
    int i,j,k;
    time_t tval;
    struct tm *now;

    tval = time(NULL);
    now = localtime(&tval);
    fprintf(logfile,"[%d:%d:%d] Dumping grid info:\n",now->tm_hour, now->tm_min, now->tm_sec);

    for(k=0;k<MAX_LAYERS;k++){
        fprintf(logfile,"Layer %d :\n",k);
        for(j=0;j<arenas[myworld.arenasize][1];j++){
            for(i=0;i<arenas[myworld.arenasize][0];i++){
                if(myworld.cursor[0] == i && myworld.cursor[1] == j){
                    fprintf(logfile,"[%d] ",myworld.layout[i][j][k]);
                }else{
                    fprintf(logfile,"%d ",myworld.layout[i][j][k]);
                }
            }
            fprintf(logfile,"\n");
        }
    }

    fprintf(logfile,"[%d:%d:%d] Finished Dumping\n",now->tm_hour, now->tm_min, now->tm_sec);
}

/*
##################
#..dump_spells().#
##################
*/
void dump_spells (void)
{
    int i,s;
    time_t tval;
    struct tm *now;

    tval = time(NULL);
    now = localtime(&tval);
    fprintf(logfile,"[%d:%d:%d] Dumping Spell info:\n",now->tm_hour, now->tm_min, now->tm_sec);

    fprintf(logfile,"-------------------\nPlayers (total %d) :\n",myworld.players);
    for(i=10;i<myworld.players+10;i++){
        fprintf(logfile,"---- Player Spell %d ----\n",i);
        fprintf(logfile,"[%d].id = %d\n",i,myspells[i].id);
        fprintf(logfile,"[%d].player_id = %d\n",i,myspells[i].player_id);
        fprintf(logfile,"[%d].spell_type = %d\n",i,myspells[i].spell_type);
        fprintf(logfile,"[%d].current_pos = %dx%dx%d\n",i,
            myspells[i].current_pos[0],
            myspells[i].current_pos[1],
            myspells[i].current_pos[2]);
        fprintf(logfile,"[%d].current_defense = %d\n",i,myspells[i].current_defense);
        fprintf(logfile,"[%d].illusion = %d\n",i,myspells[i].illusion);
        fprintf(logfile,"[%d].beencast = %d\n",i,myspells[i].beencast);
        fprintf(logfile,"[%d].dead = %d\n",i,myspells[i].dead);
        fprintf(logfile,"[%d].undead = %d\n",i,myspells[i].undead);
        fprintf(logfile,"[%d].beenmoved = %d\n",i,myspells[i].beenmoved);
        fprintf(logfile,"[%d].uses = %d\n",i,myspells[i].uses);
        fprintf(logfile,"[%d].last_killed = %d\n",i,myspells[i].last_killed);

        fprintf(logfile,"---- Player Struct Item %d ----\n",i-10);
        fprintf(logfile,"[%d].id = %d\n",i-10,myplayers[i-10].id);
        fprintf(logfile,"[%d].socket = %d\n",i-10,myplayers[i-10].socket);
        fprintf(logfile,"[%d].ready = %d\n",i-10,myplayers[i-10].ready);
        fprintf(logfile,"[%d].name = %s\n",i-10,myplayers[i-10].name);
        fprintf(logfile,"[%d].ready = %d\n",i-10,myplayers[i-10].ready);
        fprintf(logfile,"[%d].color = %d\n",i-10,myplayers[i-10].color);
        fprintf(logfile,"[%d].ping = %d\n",i-10,myplayers[i-10].ping);
        fprintf(logfile,"[%d].total_spells = %d\n",i-10,myplayers[i-10].total_spells);
        fprintf(logfile,"[%d].selected_spell = %d\n",i-10,myplayers[i-10].selected_spell);
        fprintf(logfile,"[%d].defense = %d\n",i-10,myplayers[i-10].defense);
        fprintf(logfile,"[%d].attack = %d\n",i-10,myplayers[i-10].attack);
        fprintf(logfile,"[%d].ranged_damage = %d\n",i-10,myplayers[i-10].ranged_damage);
        fprintf(logfile,"[%d].dex = %d\n",i-10,myplayers[i-10].dex);
        fprintf(logfile,"[%d].move_range = %d\n",i-10,myplayers[i-10].move_range);
        fprintf(logfile,"[%d].magic_resistance = %d\n",i-10,myplayers[i-10].magic_resistance);
        fprintf(logfile,"[%d].flight = %d\n",i-10,myplayers[i-10].flight);
        fprintf(logfile,"[%d].attack_undead = %d\n",i-10,myplayers[i-10].attack_undead);
        fprintf(logfile,"[%d].spells = ",i-10);
        for(s=1;s<myplayers[i-10].total_spells+1;s++)
            fprintf(logfile,"%d ",myplayers[i-10].spells[s]);
        fprintf(logfile,"\n");
        fprintf(logfile,"[%d].upgrades = ",i-10);
        for(s=0;s<MAX_MAGIC_UPGRADE;s++)
            if(myplayers[i-10].upgrades[s] != 0)
                fprintf(logfile,"%d ",myplayers[i-10].upgrades[s]);
        fprintf(logfile,"\n");
    }
    fprintf(logfile,"-------------------\nSpells (total %d) :\n",myworld.total_spells);
    for(i=100;i<myworld.total_spells+100;i++){
        fprintf(logfile,"---- Spell %d ----\n",i);
        fprintf(logfile,"[%d].id = %d\n",i,myspells[i].id);
        fprintf(logfile,"[%d].player_id = %d\n",i,myspells[i].player_id);
        fprintf(logfile,"[%d].spell_type = %d\n",i,myspells[i].spell_type);
        fprintf(logfile,"[%d].current_pos = %dx%dx%d\n",i,
            myspells[i].current_pos[0],
            myspells[i].current_pos[1],
            myspells[i].current_pos[2]);
        fprintf(logfile,"[%d].current_defense = %d\n",i,myspells[i].current_defense);
        fprintf(logfile,"[%d].illusion = %d\n",i,myspells[i].illusion);
        fprintf(logfile,"[%d].beencast = %d\n",i,myspells[i].beencast);
        fprintf(logfile,"[%d].dead = %d\n",i,myspells[i].dead);
        fprintf(logfile,"[%d].undead = %d\n",i,myspells[i].undead);
        fprintf(logfile,"[%d].beenmoved = %d\n",i,myspells[i].beenmoved);
        fprintf(logfile,"[%d].turnlimit = %d\n",i,myspells[i].turnlimit);
        fprintf(logfile,"[%d].uses = %d\n",i,myspells[i].uses);
        fprintf(logfile,"[%d].last_killed = %d\n",i,myspells[i].last_killed);
    }
    fprintf(logfile,"[%d:%d:%d] Finished Dumping\n",now->tm_hour, now->tm_min, now->tm_sec);
}
