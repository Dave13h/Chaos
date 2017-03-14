/***************************************************************
*  generate.c                                      #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  generate functions/routines                     #.....#     *
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
#include "display_common.h"

extern world                myworld;
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

extern char log_message[LOGMSGLEN];

int arenas[MAX_ARENAS][2] = {{0,0},{20,8},{42,16},{64,20}};

int monsters_count = 0;
int trees_count = 0;
int walls_count = 0;
int blobs_count = 0;
int magic_special_count = 0;
int magic_upgrade_count = 0;
int magic_balance_count = 0;
int magic_spell_attrib_count = 0;
int magic_ranged_count = 0;

#ifdef WITH_NET
#include "net.h"
extern bool net_enable;
extern bool net_dedicated;
#endif

/*
###################
#..get_ce_color().#
###################
    NOTES :
        - XXX Disgusting, there MUST be a better way? =/
*/
static int get_ce_color (char *colortext)
{
    if(!strcasecmp(colortext,"lgrey"))      return CE_C_LGREY;
    if(!strcasecmp(colortext,"grey"))       return CE_C_GREY;
    if(!strcasecmp(colortext,"dgrey"))      return CE_C_DGREY;

    if(!strcasecmp(colortext,"lviolet"))    return CE_C_LVIOLET;
    if(!strcasecmp(colortext,"violet"))     return CE_C_VIOLET;
    if(!strcasecmp(colortext,"dviolet"))    return CE_C_DVIOLET;

    if(!strcasecmp(colortext,"lwhite"))     return CE_C_LWHITE;
    if(!strcasecmp(colortext,"white"))      return CE_C_WHITE;
    if(!strcasecmp(colortext,"dwhite"))     return CE_C_DWHITE;

    if(!strcasecmp(colortext,"lblue"))      return CE_C_LBLUE;
    if(!strcasecmp(colortext,"blue"))       return CE_C_BLUE;
    if(!strcasecmp(colortext,"dblue"))      return CE_C_DBLUE;

    if(!strcasecmp(colortext,"lgreen"))     return CE_C_LGREEN;
    if(!strcasecmp(colortext,"green"))      return CE_C_GREEN;
    if(!strcasecmp(colortext,"dgreen"))     return CE_C_DGREEN;

    if(!strcasecmp(colortext,"lred"))       return CE_C_LRED;
    if(!strcasecmp(colortext,"red"))        return CE_C_RED;
    if(!strcasecmp(colortext,"dred"))       return CE_C_DRED;

    if(!strcasecmp(colortext,"lorange"))    return CE_C_LORANGE;
    if(!strcasecmp(colortext,"orange"))     return CE_C_ORANGE;
    if(!strcasecmp(colortext,"dorange"))    return CE_C_DORANGE;

    if(!strcasecmp(colortext,"lyellow"))    return CE_C_LYELLOW;
    if(!strcasecmp(colortext,"yellow"))     return CE_C_YELLOW;
    if(!strcasecmp(colortext,"dyellow"))    return CE_C_DYELLOW;

    if(!strcasecmp(colortext,"lbrown"))     return CE_C_LBROWN;
    if(!strcasecmp(colortext,"brown"))      return CE_C_BROWN;
    if(!strcasecmp(colortext,"dbrown"))     return CE_C_DBROWN;

    if(!strcasecmp(colortext,"lpurple"))    return CE_C_LPURPLE;
    if(!strcasecmp(colortext,"purple"))     return CE_C_PURPLE;
    if(!strcasecmp(colortext,"dpurple"))    return CE_C_DPURPLE;

    if(!strcasecmp(colortext,"lcyan"))      return CE_C_LCYAN;
    if(!strcasecmp(colortext,"cyan"))       return CE_C_CYAN;
    if(!strcasecmp(colortext,"dcyan"))      return CE_C_DCYAN;

    if(!strcasecmp(colortext,"multi1"))     return CE_C_MULTI1;
    if(!strcasecmp(colortext,"multi2"))     return CE_C_MULTI2;
    if(!strcasecmp(colortext,"multi3"))     return CE_C_MULTI3;

    /* Nothing found in this monstrosity, default to white */
    return CE_C_WHITE;
}

/*
#######################
#..generatemonsters().#
#######################
*/
static int generate_monsters (void)
{
    FILE *datafile;
    char filename[32] = "data/monsters.dat";
    char buf[256];
    char *field;
    int i;
    int j = 1;

    if((datafile = fopen(filename,"rb")) == NULL){
        console_log(__FILE__,__func__,__LINE__,LOG_ERROR, sprintf(log_message,
            "FATAL : Failed to open file : %s",filename));
        exit(1);
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Opening file for reading => %s",filename));
    while (fgets(buf, sizeof(buf), datafile) != NULL) {
        i = 0;
        buf[strlen(buf)-1] = '\0';          /* Strip  from end of line  */
        if(buf[0] != '#'){                  /* Ignore # comment lines       */
            field = strtok(buf,",");
            while(field != NULL){
                switch(i){
                    case 0:
                        mymonsters[j].id = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymonsters[%d].id => %d", j,
                            mymonsters[j].id)
                        );
                        break;
                    case 1:
                        memset(mymonsters[j].name,'\0',sizeof(mymonsters[j].name));
                        strcpy(mymonsters[j].name,field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymonsters[%d].name => %s", j,
                            mymonsters[j].name)
                        );
                        break;
                    case 2:
                        memset(mymonsters[j].disp,'\0',sizeof(mymonsters[j].disp));
                        strcpy(mymonsters[j].disp,field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymonsters[%d].disp => %s", j,
                            mymonsters[j].disp)
                        );
                        break;
                    case 3:
                        mymonsters[j].color = get_ce_color(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymonsters[%d].color => %d (dat file => %s)", j,
                            mymonsters[j].color,field)
                        );
                        break;
                    case 4:
                        mymonsters[j].attack = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymonsters[%d].attack => %d",
                            j, mymonsters[j].attack)
                        );
                        break;
                    case 5:
                        mymonsters[j].ranged_damage = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymonsters[%d].ranged_damage => %d",
                            j, mymonsters[j].ranged_damage)
                        );
                        break;
                    case 6:
                        mymonsters[j].ranged_range = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymonsters[%d].ranged_range => %d",
                            j, mymonsters[j].ranged_range)
                        );
                        break;
                    case 7:
                        mymonsters[j].defense = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymonsters[%d].defense => %d",
                            j, mymonsters[j].defense)
                        );
                        break;
                    case 8:
                        mymonsters[j].move_range = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymonsters[%d].move_range => %d",
                            j, mymonsters[j].move_range)
                        );
                        break;
                    case 9:
                        mymonsters[j].dex = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymonsters[%d].dex => %d", j,
                            mymonsters[j].dex)
                        );
                        break;
                    case 10:
                        mymonsters[j].magic_resistance = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymonsters[%d].magic_resistance => %d",
                            j, mymonsters[j].magic_resistance)
                        );
                        break;
                    case 11:
                        mymonsters[j].casting_prob = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymonsters[%d].casting_prob => %d",
                            j, mymonsters[j].casting_prob)
                        );
                        break;
                    case 12:
                        if(atoi(field) == 1){
                            mymonsters[j].flight = true;
                        } else {
                            mymonsters[j].flight = false;
                        }
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymonsters[%d].flight => %d",
                            j, mymonsters[j].flight)
                        );
                        break;
                    case 13:
                        if(atoi(field) == 1){
                            mymonsters[j].mount = true;
                        } else {
                            mymonsters[j].mount = false;
                        }
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymonsters[%d].mount => %d",
                            j, mymonsters[j].mount)
                        );
                        break;
                    case 14:
                        if(atoi(field) == 1){
                            mymonsters[j].undead = true;
                        } else {
                            mymonsters[j].undead = false;
                        }
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymonsters[%d].undead => %d",
                            j, mymonsters[j].undead)
                        );
                        break;
                    case 15:
                        mymonsters[j].balance = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymonsters[%d].balance => %d",
                            j, mymonsters[j].balance)
                        );
                        break;
                    default:
                        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(
                            log_message,
                            "Warning unused field (%d) from datafile row %d. Contained (%s)",
                            i,j,field)
                        );
                        break;
                }
                i++;
                field = strtok(NULL,",");
            }
            j++;
        }
    }
    fclose(datafile);
    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Total Monsters created => %d",j-1));
    return j-1;
}

/*
#####################
#..generate_trees().#
#####################
*/
static int generate_trees (void)
{
    FILE *datafile;
    char filename[32] = "data/trees.dat";
    char buf[256];
    char *field;
    int i;
    int j = 1;

    if((datafile = fopen(filename,"rb")) == NULL){
        console_log(__FILE__,__func__,__LINE__,LOG_ERROR, sprintf(log_message,
            "FATAL : Failed to open file : %s",filename));
        exit(1);
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Opening file for reading => %s",filename));
    while (fgets(buf, sizeof(buf), datafile) != NULL) {
        i = 0;
        buf[strlen(buf)-1] = '\0';          /* Strip  from end of line  */
        if(buf[0] != '#'){                  /* Ignore # comment lines       */
            field = strtok(buf,",");
            while(field != NULL){
                switch(i){
                    case 0:
                        mytrees[j].id = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mytrees[%d].id => %d", j, mytrees[j].id)
                        );
                        break;
                    case 1:
                        memset(mytrees[j].name,'\0',sizeof(mytrees[j].name));
                        strcpy(mytrees[j].name,field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mytrees[%d].name => %s", j,
                            mytrees[j].name)
                        );
                        break;
                    case 2:
                        memset(mytrees[j].disp,'\0',sizeof(mytrees[j].disp));
                        strcpy(mytrees[j].disp,field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mytrees[%d].disp => %s", j,
                            mytrees[j].disp)
                        );
                        break;
                    case 3:
                        mytrees[j].color = get_ce_color(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mytrees[%d].color => %d (dat file => %s)", j,
                            mytrees[j].color,field)
                        );
                        break;
                    case 4:
                        memset(mytrees[j].description,'\0',sizeof(mytrees[j].description));
                        strcpy(mytrees[j].description,field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mytrees[%d].description => %s",
                            j, mytrees[j].description)
                        );
                        break;
                    case 5:
                        mytrees[j].attack = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mytrees[%d].attack => %d", j,
                            mytrees[j].attack)
                        );
                        break;
                    case 6:
                        mytrees[j].defense = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mytrees[%d].defense => %d", j,
                            mytrees[j].defense)
                        );
                        break;
                    case 7:
                        mytrees[j].casting_prob = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mytrees[%d].casting_prob => %d",
                            j, mytrees[j].casting_prob)
                        );
                        break;
                    case 8:
                        mytrees[j].casting_range = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mytrees[%d].casting_range => %d",
                            j, mytrees[j].casting_range)
                        );
                        break;
                    case 9:
                        mytrees[j].assoc_func = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mytrees[%d].assoc_func => %d",
                            j, mytrees[j].assoc_func)
                        );
                        break;
                    case 10:
                        if(atoi(field) == 1){
                            mytrees[j].mount = true;
                        } else {
                            mytrees[j].mount = false;
                        }
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mytrees[%d].mount => %d", j,
                            mytrees[j].mount)
                        );
                        break;
                    case 11:
                        if(atoi(field) == 1){
                            mytrees[j].genspells = true;
                        } else {
                            mytrees[j].genspells = false;
                        }
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mytrees[%d].genspells => %d", j,
                            mytrees[j].genspells)
                        );
                        break;
                    case 12:
                        mytrees[j].turnlimit = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mytrees[%d].turnlimit => %d", j,
                            mytrees[j].turnlimit)
                        );
                        break;
                    case 13:
                        mytrees[j].uses = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mytrees[%d].uses => %d", j,
                            mytrees[j].uses)
                        );
                        break;
                    case 14:
                        mytrees[j].balance = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mytrees[%d].balance => %d", j,
                            mytrees[j].balance)
                        );
                        break;
                    default:
                        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(
                            log_message,
                            "Warning unused field (%d) from datafile row %d. Contained (%s)",
                            i,j,field)
                        );
                        break;
                }
                i++;
                field = strtok(NULL,",");
            }
            j++;
        }
    }
    fclose(datafile);

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Total Tree Spells Generated => %d",j-1));
    return j-1;
}

/*
#####################
#..generate_walls().#
#####################
*/
static int generate_walls (void)
{
    FILE *datafile;
    char filename[32] = "data/walls.dat";
    char buf[256];
    char *field;
    int i;
    int j = 1;

    if((datafile = fopen(filename,"rb")) == NULL){
        console_log(__FILE__,__func__,__LINE__,LOG_ERROR, sprintf(log_message,
            "FATAL : Failed to open file : %s",filename));
        exit(1);
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Opening file for reading => %s",filename));
    while (fgets(buf, sizeof(buf), datafile) != NULL) {
        i = 0;
        buf[strlen(buf)-1] = '\0';          /* Strip  from end of line  */
        if(buf[0] != '#'){                  /* Ignore # comment lines       */
            field = strtok(buf,",");
            while(field != NULL){
                switch(i){
                    case 0:
                        mywalls[j].id = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mywalls[%d].id => %d", j, mywalls[j].id)
                        );
                        break;
                    case 1:
                        memset(mywalls[j].name,'\0',sizeof(mywalls[j].name));
                        strcpy(mywalls[j].name,field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mywalls[%d].name => %s", j,
                            mywalls[j].name)
                        );
                        break;
                    case 2:
                        memset(mywalls[j].disp,'\0',sizeof(mywalls[j].disp));
                        strcpy(mywalls[j].disp,field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mywalls[%d].disp => %s", j,
                            mywalls[j].disp)
                        );
                        break;
                    case 3:
                        mywalls[j].color = get_ce_color(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mywalls[%d].color => %d (dat file => %s)", j,
                            mywalls[j].color,field)
                        );
                        break;
                    case 4:
                        memset(mywalls[j].description,'\0',sizeof(mywalls[j].description));
                        strcpy(mywalls[j].description,field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mywalls[%d].description => %s",
                            j, mywalls[j].description)
                        );
                        break;
                    case 5:
                        mywalls[j].attack = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mywalls[%d].attack => %d", j,
                            mywalls[j].attack)
                        );
                        break;
                    case 6:
                        mywalls[j].defense = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mywalls[%d].defense => %d", j,
                            mywalls[j].defense)
                        );
                        break;
                    case 7:
                        mywalls[j].casting_prob = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mywalls[%d].casting_prob => %d",
                            j, mywalls[j].casting_prob)
                        );
                        break;
                    case 8:
                        mywalls[j].casting_range = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mywalls[%d].casting_range => %d",
                            j, mywalls[j].casting_range)
                        );
                        break;
                    case 9:
                        mywalls[j].assoc_func = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mywalls[%d].assoc_func => %d",
                            j, mywalls[j].assoc_func)
                        );
                        break;
                    case 10:
                        if(atoi(field) == 1){
                            mywalls[j].mount = true;
                        } else {
                            mywalls[j].mount = false;
                        }
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mywalls[%d].mount => %d", j,
                            mywalls[j].mount)
                        );
                        break;
                    case 11:
                        if(atoi(field) == 1){
                            mywalls[j].genspells = true;
                        } else {
                            mywalls[j].genspells = false;
                        }
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mywalls[%d].genspells => %d", j,
                            mywalls[j].genspells)
                        );
                        break;
                    case 12:
                        mywalls[j].turnlimit = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mywalls[%d].turnlimit => %d", j,
                            mywalls[j].turnlimit)
                        );
                        break;
                    case 13:
                        mywalls[j].uses = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mywalls[%d].uses => %d", j,
                            mywalls[j].uses)
                        );
                        break;
                    case 14:
                        mywalls[j].balance = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mywalls[%d].balance => %d", j,
                            mywalls[j].balance)
                        );
                        break;
                    default:
                        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(
                            log_message,
                            "Warning unused field (%d) from datafile row %d. Contained (%s)",
                            i,j,field)
                        );
                        break;
                }
                i++;
                field = strtok(NULL,",");
            }
            j++;
        }
    }
    fclose(datafile);

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Total Wall Spells Generated => %d",j-1));
    return j-1;
}

/*
#####################
#..generate_blobs().#
#####################
*/
static int generate_blobs (void)
{
    FILE *datafile;
    char filename[32] = "data/blobs.dat";
    char buf[256];
    char *field;
    int i;
    int j = 1;

    if((datafile = fopen(filename,"rb")) == NULL){
        console_log(__FILE__,__func__,__LINE__,LOG_ERROR, sprintf(log_message,
            "FATAL : Failed to open file : %s",filename));
        exit(1);
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Opening file for reading => %s",filename));
    while (fgets(buf, sizeof(buf), datafile) != NULL) {
        i = 0;
        buf[strlen(buf)-1] = '\0';          /* Strip  from end of line  */
        if(buf[0] != '#'){                  /* Ignore # comment lines       */
            field = strtok(buf,",");
            while(field != NULL){
                switch(i){
                    case 0:
                        myblobs[j].id = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to myblobs[%d].id => %d", j, myblobs[j].id)
                        );
                        break;
                    case 1:
                        memset(myblobs[j].name,'\0',sizeof(myblobs[j].name));
                        strcpy(myblobs[j].name,field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to myblobs[%d].name => %s", j,
                            myblobs[j].name)
                        );
                        break;
                    case 2:
                        memset(myblobs[j].disp,'\0',sizeof(myblobs[j].disp));
                        strcpy(myblobs[j].disp,field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to myblobs[%d].disp => %s", j,
                            myblobs[j].disp)
                        );
                        break;
                    case 3:
                        myblobs[j].color = get_ce_color(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to myblobs[%d].color => %d (dat file => %s)", j,
                            myblobs[j].color,field)
                        );
                        break;
                    case 4:
                        memset(myblobs[j].description,'\0',sizeof(myblobs[j].description));
                        strcpy(myblobs[j].description,field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to myblobs[%d].description => %s",
                            j, myblobs[j].description)
                        );
                        break;
                    case 5:
                        myblobs[j].attack = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to myblobs[%d].attack => %d", j,
                            myblobs[j].attack)
                        );
                        break;
                    case 6:
                        myblobs[j].defense = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to myblobs[%d].defense => %d", j,
                            myblobs[j].defense)
                        );
                        break;
                    case 7:
                        myblobs[j].casting_prob = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to myblobs[%d].casting_prob => %d",
                            j, myblobs[j].casting_prob)
                        );
                        break;
                    case 8:
                        myblobs[j].casting_range = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to myblobs[%d].casting_range => %d",
                            j, myblobs[j].casting_range)
                        );
                        break;
                    case 9:
                        myblobs[j].assoc_func = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to myblobs[%d].assoc_func => %d",
                            j, myblobs[j].assoc_func)
                        );
                        break;
                    case 10:
                        myblobs[j].autospawn = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to myblobs[%d].autospawn => %d", j,
                            myblobs[j].autospawn)
                        );
                        break;
                    case 11:
                        myblobs[j].devourer = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to myblobs[%d].devourer => %d", j,
                            myblobs[j].devourer)
                        );
                        break;
                    case 12:
                        myblobs[j].turnlimit = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to myblobs[%d].turnlimit => %d", j,
                            myblobs[j].turnlimit)
                        );
                        break;
                    case 13:
                        myblobs[j].uses = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to myblobs[%d].uses => %d", j,
                            myblobs[j].uses)
                        );
                        break;
                    case 14:
                        myblobs[j].balance = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to myblobs[%d].balance => %d", j,
                            myblobs[j].balance)
                        );
                        break;
                    default:
                        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(
                            log_message,
                            "Warning unused field (%d) from datafile row %d. Contained (%s)",
                            i,j,field)
                        );
                        break;
                }
                i++;
                field = strtok(NULL,",");
            }
            j++;
        }
    }
    fclose(datafile);

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Total Blob Spells Generated => %d",j-1));
    return j-1;
}

/*
##############################
#..generate_magic_special().#
##############################
*/
static int generate_magic_special (void)
{
    FILE *datafile;
    char filename[32] = "data/magic_special.dat";
    char buf[256];
    char *field;
    int i;
    int j = 1;

    if((datafile = fopen(filename,"rb")) == NULL){
        console_log(__FILE__,__func__,__LINE__,LOG_ERROR, sprintf(log_message,
            "FATAL : Failed to open file : %s",filename));
        exit(1);
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Opening file for reading => %s",filename));

    while (fgets(buf, sizeof(buf), datafile) != NULL) {
        i = 0;
        buf[strlen(buf)-1] = '\0';          /* Strip  from end of line  */
        if(buf[0] != '#'){                  /* Ignore # comment lines       */
            field = strtok(buf,",");
            while(field != NULL){
                switch(i){
                    case 0:
                        mymagic_special[j].id = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymagic_special[%d].id => %d",
                            j, mymagic_special[j].id)
                        );
                        break;
                    case 1:
                        memset(mymagic_special[j].name,'\0',
                            sizeof(mymagic_special[j].name));

                        strcpy(mymagic_special[j].name,field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymagic_special[%d].name => %s",
                            j, mymagic_special[j].name)
                        );
                        break;
                    case 2:
                        memset(mymagic_special[j].description,'\0',
                            sizeof(mymagic_special[j].description));
                        strcpy(mymagic_special[j].description,field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_special[%d].description => %s",
                            j, mymagic_special[j].description)
                        );
                        break;
                    case 3:
                        mymagic_special[j].assoc_func = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_special[%d].assoc_func => %d", j,
                            mymagic_special[j].assoc_func)
                        );
                        break;
                    case 4:
                        mymagic_special[j].casting_prob = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_special[%d].casting_prob => %d", j,
                            mymagic_special[j].casting_prob)
                        );
                        break;
                    case 5:
                        mymagic_special[j].casting_range = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_special[%d].casting_range => %d",
                            j, mymagic_special[j].casting_range)
                        );
                        break;
                    default:
                        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(
                            log_message,
                            "Warning unused field (%d) from datafile row %d. Contained (%s)",
                            i,j,field)
                        );
                        break;
                }
                i++;
                field = strtok(NULL,",");
            }
            j++;
        }
    }
    fclose(datafile);

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Total Magic Special Spells Generated => %d",j-1));
    return j-1;
}

/*
##############################
#..generate_magic_balance().#
##############################
*/
static int generate_magic_balance (void)
{
    FILE *datafile;
    char filename[32] = "data/magic_balance.dat";
    char buf[256];
    char *field;
    int i;
    int j = 1;

    if((datafile = fopen(filename,"rb")) == NULL){
        console_log(__FILE__,__func__,__LINE__,LOG_ERROR, sprintf(log_message,
            "FATAL : Failed to open file : %s",filename));
        exit(1);
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Opening file for reading => %s",filename));

    while (fgets(buf, sizeof(buf), datafile) != NULL) {
        i = 0;
        buf[strlen(buf)-1] = '\0';          /* Strip  from end of line  */
        if(buf[0] != '#'){                  /* Ignore # comment lines       */
            field = strtok(buf,",");
            while(field != NULL){
                switch(i){
                    case 0:
                        mymagic_balance[j].id = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymagic_balance[%d].id => %d",
                            j, mymagic_balance[j].id)
                        );
                        break;
                    case 1:
                        memset(mymagic_balance[j].name,'\0',
                            sizeof(mymagic_balance[j].name));

                        strcpy(mymagic_balance[j].name,field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymagic_balance[%d].name => %s",
                            j, mymagic_balance[j].name)
                        );
                        break;
                    case 2:
                        memset(mymagic_balance[j].description,'\0',
                            sizeof(mymagic_balance[j].description));

                        strcpy(mymagic_balance[j].description,field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_balance[%d].description => %s",
                            j, mymagic_balance[j].description)
                        );
                        break;
                    case 3:
                        mymagic_balance[j].balance = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymagic_balance[%d].balance => %d",
                            j, mymagic_balance[j].balance)
                        );
                        break;
                    case 4:
                        mymagic_balance[j].casting_prob = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_balance[%d].casting_prob => %d",
                            j, mymagic_balance[j].casting_prob)
                        );
                        break;
                    default:
                        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(
                            log_message,
                            "Warning unused field (%d) from datafile row %d. Contained (%s)",
                            i,j,field)
                        );
                        break;
                }
                i++;
                field = strtok(NULL,",");
            }
            j++;
        }
    }
    fclose(datafile);

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Total Magic Balance Spells Generated => %d",j-1));
    return j-1;
}

/*
##################################
#..generate_magic_spell_attrib().#
##################################
*/
static int generate_magic_spell_attrib (void)
{
    FILE *datafile;
    char filename[32] = "data/magic_spell_attrib.dat";
    char buf[256];
    char *field;
    int i;
    int j = 1;

    if((datafile = fopen(filename,"rb")) == NULL){
        console_log(__FILE__,__func__,__LINE__,LOG_ERROR, sprintf(log_message,"FATAL : Failed to open file : %s",filename));
        exit(1);
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,"Opening file for reading => %s",filename));
    while (fgets(buf, sizeof(buf), datafile) != NULL) {
        i = 0;
        buf[strlen(buf)-1] = '\0';          /* Strip  from end of line  */
        if(buf[0] != '#'){                  /* Ignore # comment lines       */
            field = strtok(buf,",");
            while(field != NULL){
                switch(i){
                    case 0:
                        mymagic_spell_attrib[j].id = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_spell_attrib[%d].id => %d", j,
                            mymagic_spell_attrib[j].id)
                        );
                        break;
                    case 1:
                        memset(mymagic_spell_attrib[j].name,'\0',
                            sizeof(mymagic_spell_attrib[j].name));

                        strcpy(mymagic_spell_attrib[j].name,field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_spell_attrib[%d].name => %s", j,
                            mymagic_spell_attrib[j].name)
                        );
                        break;
                    case 2:
                        memset(mymagic_spell_attrib[j].description,'\0',
                            sizeof(mymagic_spell_attrib[j].description));

                        strcpy(mymagic_spell_attrib[j].description,field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_spell_attrib[%d].description => %s",
                            j, mymagic_spell_attrib[j].description)
                        );
                        break;
                    case 3:
                        mymagic_spell_attrib[j].assoc_func = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_spell_attrib[%d].assoc_func => %d",
                            j, mymagic_spell_attrib[j].assoc_func)
                        );
                        break;
                    case 4:
                        if(atoi(field) == 1){
                            mymagic_spell_attrib[j].change_alive = true;
                        } else {
                            mymagic_spell_attrib[j].change_alive = false;
                        }
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_spell_attrib[%d].change_alive => %d",
                            j, mymagic_spell_attrib[j].change_alive)
                        );
                        break;
                    case 5:
                        if(atoi(field) == 1){
                            mymagic_spell_attrib[j].turn_undead = true;
                        } else {
                            mymagic_spell_attrib[j].turn_undead = false;
                        }
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_spell_attrib[%d].turn_undead => %d",
                            j, mymagic_spell_attrib[j].turn_undead)
                        );
                        break;
                    case 6:
                        if(atoi(field) == 1){
                            mymagic_spell_attrib[j].change_owner = true;
                        } else {
                            mymagic_spell_attrib[j].change_owner = false;
                        }
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_spell_attrib[%d].change_owner => %d",
                            j, mymagic_spell_attrib[j].change_owner)
                        );
                        break;
                    case 7:
                        mymagic_spell_attrib[j].balance = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_spell_attrib[%d].balance => %d", j,
                            mymagic_spell_attrib[j].balance)
                        );
                        break;
                    case 8:
                        mymagic_spell_attrib[j].casting_prob = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_spell_attrib[%d].casting_prob => %d",
                            j, mymagic_spell_attrib[j].casting_prob)
                        );
                        break;
                    case 9:
                        mymagic_spell_attrib[j].casting_range = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_spell_attrib[%d].casting_range => %d",
                            j, mymagic_spell_attrib[j].casting_range)
                        );
                        break;
                    default:
                        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(
                            log_message,
                            "Warning unused field (%d) from datafile row %d. Contained (%s)",
                            i,j,field)
                        );
                        break;
                }
                i++;
                field = strtok(NULL,",");
            }
            j++;
        }
    }
    fclose(datafile);

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Total Magic Spell Attribute Spells Generated => %d",j-1));
    return j-1;
}

/*
#############################
#..generate_magic_upgrade().#
#############################
*/
static int generate_magic_upgrade (void)
{
    FILE *datafile;
    char filename[32] = "data/magic_upgrade.dat";
    char buf[256];
    char *field;
    int i;
    int j = 1;

    if((datafile = fopen(filename,"rb")) == NULL){
        console_log(__FILE__,__func__,__LINE__,LOG_ERROR, sprintf(log_message,
            "FATAL : Failed to open file : %s",filename));
        exit(1);
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Opening file for reading => %s",filename));

    while (fgets(buf, sizeof(buf), datafile) != NULL) {
        i = 0;
        buf[strlen(buf)-1] = '\0';          /* Strip  from end of line  */
        if(buf[0] != '#'){                  /* Ignore # comment lines       */
            field = strtok(buf,",");
            while(field != NULL){
                switch(i){
                    case 0:
                        mymagic_upgrade[j].id = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymagic_upgrade[%d].id => %d",
                            j, mymagic_upgrade[j].id)
                        );
                        break;
                    case 1:
                        memset(mymagic_upgrade[j].name,'\0',
                            sizeof(mymagic_upgrade[j].name));

                        strcpy(mymagic_upgrade[j].name,field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymagic_upgrade[%d].name => %s",
                            j, mymagic_upgrade[j].name)
                        );
                        break;
                    case 2:
                        memset(mymagic_upgrade[j].disp,'\0',sizeof(mymagic_upgrade[j].disp));
                        strcpy(mymagic_upgrade[j].disp,field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymagic_upgrade[%d].disp => %s",
                            j, mymagic_upgrade[j].disp)
                        );
                        break;
                    case 3:
                        mymagic_upgrade[j].color = get_ce_color(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymagic_upgrade[%d].color => %d (dat file => %s)", j,
                            mymagic_upgrade[j].color,field)
                        );
                        break;
                    case 4:
                        memset(mymagic_upgrade[j].description,'\0',
                            sizeof(mymagic_upgrade[j].description));

                        strcpy(mymagic_upgrade[j].description,field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_upgrade[%d].description => %s", j,
                            mymagic_upgrade[j].description)
                        );
                        break;
                    case 5:
                        mymagic_upgrade[j].attack = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymagic_upgrade[%d].attack => %d",
                            j, mymagic_upgrade[j].attack)
                        );
                        break;
                    case 6:
                        mymagic_upgrade[j].ranged_damage = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_upgrade[%d].ranged_damage => %d",
                            j, mymagic_upgrade[j].ranged_damage)
                        );
                        break;
                    case 7:
                        mymagic_upgrade[j].ranged_range = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_upgrade[%d].ranged_range => %d", j,
                            mymagic_upgrade[j].ranged_range)
                        );
                        break;
                    case 8:
                        mymagic_upgrade[j].defense = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_upgrade[%d].defense => %d", j,
                            mymagic_upgrade[j].defense)
                        );
                        break;
                    case 9:
                        mymagic_upgrade[j].move_range = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_upgrade[%d].move_range => %d", j,
                            mymagic_upgrade[j].move_range)
                        );
                        break;
                    case 10:
                        if(atoi(field) == 1){
                            mymagic_upgrade[j].flight = true;
                        } else {
                            mymagic_upgrade[j].flight = false;
                        }
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_upgrade[%d].flight => %d", j,
                            mymagic_upgrade[j].flight)
                        );
                        break;
                    case 11:
                        if(atoi(field) == 1){
                            mymagic_upgrade[j].attack_undead = true;
                        } else {
                            mymagic_upgrade[j].attack_undead = false;
                        }
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_upgrade[%d].attack_undead => %d", j,
                            mymagic_upgrade[j].attack_undead)
                        );
                        break;
                    case 12:
                        mymagic_upgrade[j].balance = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_upgrade[%d].balance => %d", j,
                            mymagic_upgrade[j].balance)
                        );
                        break;
                    case 13:
                        mymagic_upgrade[j].casting_prob = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_upgrade[%d].casting_prob => %d", j,
                            mymagic_upgrade[j].casting_prob)
                        );
                        break;
                    default:
                        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(
                            log_message,
                            "Warning unused field (%d) from datafile row %d. Contained (%s)",
                            i,j,field)
                        );
                        break;
                }
                i++;
                field = strtok(NULL,",");
            }
            j++;
        }
    }
    fclose(datafile);

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Total Magic Upgrade Spells Generated => %d",j-1));
    return j-1;
}

/*
############################
#..generate_magic_ranged().#
############################
*/
static int generate_magic_ranged (void)
{
    FILE *datafile;
    char filename[32] = "data/magic_ranged.dat";
    char buf[256];
    char *field;
    int i;
    int j = 1;

    if((datafile = fopen(filename,"rb")) == NULL){
        console_log(__FILE__,__func__,__LINE__,LOG_ERROR, sprintf(log_message,
            "FATAL : Failed to open file : %s",filename));
        exit(1);
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Opening file for reading => %s",filename));

    while (fgets(buf, sizeof(buf), datafile) != NULL) {
        i = 0;
        buf[strlen(buf)-1] = '\0';          /* Strip  from end of line  */
        if(buf[0] != '#'){                  /* Ignore # comment lines       */
            field = strtok(buf,",");
            while(field != NULL){
                switch(i){
                    case 0:
                        mymagic_ranged[j].id = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymagic_ranged[%d].id => %d", j,
                            mymagic_ranged[j].id)
                        );
                        break;
                    case 1:
                        memset(mymagic_ranged[j].name,'\0',
                            sizeof(mymagic_ranged[j].name));

                        strcpy(mymagic_ranged[j].name,field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymagic_ranged[%d].name => %s",
                            j, mymagic_ranged[j].name)
                        );
                        break;
                    case 2:
                        memset(mymagic_ranged[j].disp,'\0',
                            sizeof(mymagic_ranged[j].disp));
                        strcpy(mymagic_ranged[j].disp,field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymagic_ranged[%d].disp => %s",
                            j, mymagic_ranged[j].disp)
                        );
                        break;
                    case 3:
                        mymagic_ranged[j].color = get_ce_color(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymagic_ranged[%d].color => %d (dat file => %s)", j,
                            mymagic_ranged[j].color,field)
                        );
                        break;
                    case 4:
                        memset(mymagic_ranged[j].description,'\0',
                            sizeof(mymagic_ranged[j].description));

                        strcpy(mymagic_ranged[j].description,field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_ranged[%d].description => %s", j,
                            mymagic_ranged[j].description)
                        );
                        break;
                    case 5:
                        mymagic_ranged[j].assoc_func = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_ranged[%d].assoc_func => %d", j,
                            mymagic_ranged[j].assoc_func)
                        );
                        break;
                    case 6:
                        mymagic_ranged[j].ranged_damage = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_ranged[%d].ranged_damage => %d", j,
                            mymagic_ranged[j].ranged_damage)
                        );
                        break;
                    case 7:
                        mymagic_ranged[j].ranged_range = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_ranged[%d].ranged_range => %d", j,
                            mymagic_ranged[j].ranged_range)
                        );
                        break;
                    case 8:
                        mymagic_ranged[j].uses = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymagic_ranged[%d].uses => %d",
                            j, mymagic_ranged[j].uses)
                        );
                        break;
                    case 9:
                        if(atoi(field) == 1){
                            mymagic_ranged[j].beam = true;
                        } else {
                            mymagic_ranged[j].beam = false;
                        }
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,"Added data to mymagic_ranged[%d].beam => %d",
                            j, mymagic_ranged[j].beam)
                        );
                        break;
                    case 10:
                        mymagic_ranged[j].balance = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_ranged[%d].balance => %d", j,
                            mymagic_ranged[j].balance)
                        );
                        break;
                    case 11:
                        mymagic_ranged[j].casting_prob = atoi(field);
                        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
                            log_message,
                            "Added data to mymagic_ranged[%d].casting_prob => %d", j,
                            mymagic_ranged[j].casting_prob)
                        );
                        break;
                    default:
                        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(
                            log_message,
                            "Warning unused field (%d) from datafile row %d. Contained (%s)",
                            i,j,field)
                        );
                        break;
                }
                i++;
                field = strtok(NULL,",");
            }
            j++;
        }
    }
    fclose(datafile);

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Total Magic Ranged Spells Generated => %d",j-1));
    return j-1;
}

/*
######################
#..generate_spells().#
######################
*/
void generate_spells (void)
{
    int total_spells = 0;
    int players[myworld.players],s,p;
    int c = 0;
    int magic_special_added = 0;

    monsters_count = generate_monsters();
    trees_count = generate_trees();
    walls_count = generate_walls();
    blobs_count = generate_blobs();
    magic_special_count = generate_magic_special();
    magic_upgrade_count = generate_magic_upgrade();
    magic_balance_count = generate_magic_balance();
    magic_spell_attrib_count = generate_magic_spell_attrib();
    magic_ranged_count = generate_magic_ranged();

    srand(time(NULL));

    /*
    #######################
    #.Spell.List.Defaults.#
    #######################
    */
    for (s=0;s<MAX_SPELLS;s++){
        myspells[s].id = 0;
        myspells[s].spell_type = SPELL_NULL;
    }

    /*
    #########################
    #..Player.Spell.Entries.#
    #########################
    */
    for (p=0;p<myworld.players;p++){

        players[p] = (MIN_PLAYER_SPELLS + (rand() %
            (MAX_PLAYER_SPELLS-MIN_PLAYER_SPELLS))-magic_special_count);

        total_spells += players[p];
        myplayers[p].total_spells = players[p];

        myspells[10+p].id = p;
        myspells[10+p].player_id = p;
        myspells[10+p].spell_type = SPELL_PLAYER;
        myspells[10+p].beenmoved = false;
        myspells[10+p].dead = false;
        myspells[10+p].undead = false;
        myspells[10+p].illusion = false;
        myspells[10+p].genspells = false;
        myspells[10+p].current_defense = myplayers[p].defense;

        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
            log_message,"Player %d needs %d spells", p, players[p]));
    }

    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
        log_message,"Total spells needed %d", total_spells));

    p=0;

    /*
    ###################
    #..Special.Spells.#
    ###################
    */
    if(magic_special_count > 0){

        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(
            log_message,"Loading %d Special Spells",magic_special_count));

        /* XXX FIX ME: Why the fuck does this loop start at 1 you prick. */
        for(s=1;s<magic_special_count+1;s++){
            myspells[50+s].id = s;
            myspells[50+s].player_id = s;
            myspells[50+s].spell_type = SPELL_MAGIC_SPECIAL;
            myspells[50+s].beenmoved = false;
            myspells[50+s].undead = false;
            myspells[50+s].illusion = false;
            myspells[50+s].genspells = false;
            for(p=0;p<myworld.players;p++){

                console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
                    "Assigning Default Special Spell %d (myspells[%d]) to player %d",
                    s,s+50,p)
                );

                myplayers[p].spells[magic_special_added+1] = 50+s;
                myplayers[p].total_spells++;
                players[p]++;
            }
            magic_special_added++;
            console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
                "Added Special Spell %d : %s",s,mymagic_special[s].name));
        }
        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
            "Finished loading Special Spells"));
    }

    p=0;
    c=magic_special_added+1;

    /*
    ###############
    #..Spell.Pool.#
    ###############
    */
    for (s=100;s<total_spells+100;s++){
        myspells[s].player_id = p;
        myspells[s].undead = false;
        myspells[s].turnlimit = 0;
        myspells[s].uses = 0;
        myspells[s].genspells = false;

        myspells[s].spell_type = rand()%MAX_SPELL_TYPES+1;
        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
            "Spell type rand() was %d",myspells[s].spell_type));

        switch(myspells[s].spell_type){

            case SPELL_BLOB:
                myspells[s].id = rand()%blobs_count+1;
                myspells[s].current_defense = myblobs[myspells[s].id].defense;
                myspells[s].uses = myblobs[myspells[s].id].uses;

                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Spell %d added for player %d  Spell %d [type %d] : %d => %s", s,
                    myspells[s].player_id, c,myspells[s].spell_type, myspells[s].id,
                    myblobs[myspells[s].id].name)
                );
                break;

            case SPELL_WALL:
                myspells[s].id = rand()%walls_count+1;
                myspells[s].current_defense = mywalls[myspells[s].id].defense;
                if(mywalls[myspells[s].id].turnlimit == -1) {
                    myspells[s].turnlimit = rand()%ROUNDTURN_EXPIRE_RAND+1;
                } else {
                    myspells[s].turnlimit = mywalls[myspells[s].id].turnlimit;
                    if(myspells[s].turnlimit > 0){ /* Add 1 as it will expire on turn 1 */
                        myspells[s].turnlimit++;
                    }
                }
                myspells[s].uses = mywalls[myspells[s].id].uses;
                myspells[s].genspells = mywalls[myspells[s].id].genspells;

                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Spell %d added for player %d  Spell %d [type %d] : %d => %s with turn limit of %d", s,
                    myspells[s].player_id, c,myspells[s].spell_type, myspells[s].id,
                    mywalls[myspells[s].id].name,myspells[s].turnlimit)
                );
                break;

            case SPELL_MAGIC_BALANCE:
                myspells[s].id = rand()%magic_balance_count+1;
                myspells[s].current_defense = 0;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Spell %d added for player %d  Spell %d [type %d] : %d => %s", s,
                    myspells[s].player_id, c,myspells[s].spell_type, myspells[s].id,
                    mymagic_balance[myspells[s].id].name)
                );
                break;

            case SPELL_MAGIC_ATTRIB:
                myspells[s].id = rand()%magic_spell_attrib_count+1;
                myspells[s].current_defense = 0;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Spell %d added for player %d  Spell %d [type %d] : %d => %s", s,
                    myspells[s].player_id, c,myspells[s].spell_type, myspells[s].id,
                    mymagic_spell_attrib[myspells[s].id].name)
                );
                break;

            case SPELL_MAGIC_UPGRADE:
                myspells[s].id = rand()%magic_upgrade_count+1;
                myspells[s].current_defense = 0;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Spell %d added for player %d  Spell %d [type %d] : %d => %s", s,
                    myspells[s].player_id, c,myspells[s].spell_type, myspells[s].id,
                    mymagic_upgrade[myspells[s].id].name)
                );
                break;

            case SPELL_TREE:
                myspells[s].id = rand()%trees_count+1;
                myspells[s].current_defense = mytrees[myspells[s].id].defense;
                if(mytrees[myspells[s].id].turnlimit == -1) {
                    myspells[s].turnlimit = rand()%ROUNDTURN_EXPIRE_RAND+1;
                } else {
                    myspells[s].turnlimit = mytrees[myspells[s].id].turnlimit;
                    if(myspells[s].turnlimit > 0){ /* Add 1 as it will expire on turn 1 */
                        myspells[s].turnlimit++;
                    }
                }
                myspells[s].uses = mytrees[myspells[s].id].uses;
                myspells[s].genspells = mytrees[myspells[s].id].genspells;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Spell %d added for player %d  Spell %d [type %d] : %d => %s with turn limit of %d", s,
                    myspells[s].player_id, c,myspells[s].spell_type, myspells[s].id,
                    mytrees[myspells[s].id].name,myspells[s].turnlimit)
                );
                break;

            case SPELL_MAGIC_RANGED:
                myspells[s].id = rand()%magic_ranged_count+1;
                myspells[s].current_defense = 0;
                myspells[s].uses = mymagic_ranged[myspells[s].id].uses;
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Spell %d added for player %d  Spell %d [type %d] : %d => %s", s,
                    myspells[s].player_id, c,myspells[s].spell_type, myspells[s].id,
                    mymagic_ranged[myspells[s].id].name)
                );
                break;

            case SPELL_MONSTER:
            default:
                myspells[s].spell_type = SPELL_MONSTER; /* 'Safe' type */
                myspells[s].id = rand()%monsters_count+1;
                myspells[s].current_defense = mymonsters[myspells[s].id].defense;
                if(mymonsters[myspells[s].id].undead){ myspells[s].undead = true;}
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Spell %d added for player %d  Spell %d [type %d] : %d => %s", s,
                    myspells[s].player_id, c,myspells[s].spell_type, myspells[s].id,
                    mymonsters[myspells[s].id].name)
                );
                break;
        }

        myspells[s].beencast = false;
        myspells[s].beenmoved = false;
        myspells[s].dead = false;
        myspells[s].last_killed = 0;
        myplayers[p].spells[c] = s;

        /* Default to real until 'casting/placement' time */
        myspells[s].illusion = false;

        c++;

        /* Start players spells from last special spell */
        if(c > players[p]){p++;c=magic_special_added+1;}

        myworld.total_spells++;
    }
    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Total spells in pool => %d",myworld.total_spells));
}

/*
#####################
#..generate_arena().#
#####################
*/
void generate_arena (void)
{
    int p = 10;
    int x = 1;
    int y = 1;
    int i,j,l;
    int indent = 2;
    int spawnmode = 1;
    int player_y;

    static int spawnpoints[13][3] = {
        {1,0,0},    /* 1 Player :: Never used but makes the numbers work =P */
        {2,0,0},    /* 2 Players    */
        {2,1,0},
        {2,2,0},
        {2,1,2},
        {3,3,0},
        {3,1,3},
        {2,4,2},
        {3,3,3},
        {3,4,3},
        {4,3,4},
        {4,4,4}     /* 12 Players   */
    };

    if(spawnpoints[myworld.players-1][1] > 0) {
        if(spawnpoints[myworld.players-1][2] > 0) {
            spawnmode = 3;
        } else {
            spawnmode = 2;
        }
    }

    if(myworld.players > 7){indent = 1;}

    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
        "Using spawn info {%d,%d,%d}",
        spawnpoints[myworld.players-1][0],spawnpoints[myworld.players-1][1],
        spawnpoints[myworld.players-1][2])
    );

    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
        "Using Spawn Mode %d",spawnmode));

    /* Blanking Arena */
    for(i=0;i<arenas[myworld.arenasize][0];i++)
        for(j=0;j<arenas[myworld.arenasize][1];j++)
            for(l=0;l<MAX_LAYERS;l++)
                myworld.layout[i][j][l] = 0;

    for(y=1;y<spawnmode+1;y++){
        if (y == 1 && spawnmode > 1) {
            player_y = indent;
        } else if ((y == 1 && spawnmode == 1) || (y == 2 && spawnmode == 3)) {
            player_y = arenas[myworld.arenasize][1] / 2;
        } else {
            player_y = arenas[myworld.arenasize][1] - (indent+1);
        }
        for(x=1;x<(spawnpoints[myworld.players-1][y-1]+1);x++){
            myspells[p].current_pos[1] = player_y;
            if(spawnpoints[myworld.players-1][y-1] == 2) {
                if(x == 1) {
                    myspells[p].current_pos[0] = indent;
                } else {
                    myspells[p].current_pos[0] = arenas[myworld.arenasize][0] - (indent+1);
                }
            } else if(spawnpoints[myworld.players-1][y-1] == 3) {
                if(x == 1) {
                    myspells[p].current_pos[0] = indent;
                } else if (x == 2) {
                    myspells[p].current_pos[0] = arenas[myworld.arenasize][0] / 2;
                } else {
                    myspells[p].current_pos[0] = arenas[myworld.arenasize][0] - (indent+1);
                }
            } else if(spawnpoints[myworld.players-1][y-1] == 4) {
                if(x == 1) {
                    myspells[p].current_pos[0] = indent;
                } else if (x == 2) {

                    myspells[p].current_pos[0] = ((arenas[myworld.arenasize][0] / 2) -
                        ((arenas[myworld.arenasize][0] / 3)/2));

                } else if (x == 3) {

                    myspells[p].current_pos[0] = ((arenas[myworld.arenasize][0] / 2) +
                        ((arenas[myworld.arenasize][0] / 3)/2));

                } else {
                    myspells[p].current_pos[0] = arenas[myworld.arenasize][0] -
                        (indent+1);
                }
            } else {
                myspells[p].current_pos[0] = arenas[myworld.arenasize][0] / 2;
            }
            myspells[p].current_pos[2] = 0;
            myworld.layout[myspells[p].current_pos[0]][myspells[p].current_pos[1]][0] = p;

            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Spawned player (%d) at %dx%d",p-9,myspells[p].current_pos[0],
                myspells[p].current_pos[1])
            );

            p++;
        }
    }
}

/*
#######################
#..generate_players().#
#######################
*/
void generate_players (void)
{
    int p,u;
    char histtext[255];

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Generating players static information"));

    srand(time(NULL));

    for (p=0;p<myworld.players;p++){
        memset(myplayers[p].disp,'\0',sizeof(myplayers[p].disp));
        strcpy(myplayers[p].disp,"@");

#ifdef WITH_NET
        /* Net Server gets colours from Client Join */
        if(!net_enable || !net_dedicated)
#endif
            myplayers[p].color = CE_C_LWHITE;

        myplayers[p].id = p;
        myplayers[p].selected_spell = 0;

        myplayers[p].attack = rand()%MAX_PLAYER_ATTACK+1;
        myplayers[p].defense = rand()%MAX_PLAYER_DEFENSE+1;
        myplayers[p].dex = rand()%MAX_PLAYER_DEX+1;
        myplayers[p].magic_resistance = rand()%MAX_PLAYER_MAGIC_RESISTANCE+1;

        myplayers[p].flight = false;
        myplayers[p].attack_undead = false;
        myplayers[p].ranged_range = 0;
        myplayers[p].ranged_damage = 0;
        myplayers[p].move_range = 1;

        myplayers[p].ping = 3;

        for(u=0;u<MAX_MAGIC_UPGRADE;u++){
            myplayers[p].upgrades[u] = 0;
        }

        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
            "Player %d added : %s [%d] [ATTACK %d | DEFENSE %d | DEX %d]=> %s",
            p, myplayers[p].disp, myplayers[p].color, myplayers[p].attack,
            myplayers[p].defense, myplayers[p].dex, myplayers[p].name)
        );

        memset(histtext,'\0',sizeof(histtext));
        sprintf(histtext,"Player %s Entered The Game",myplayers[p].name);
        history_add(histtext);
    }
    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Total Players created => %d",p));
}
