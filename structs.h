/***************************************************************
*  structs.h                                       #######     *
*  chaos-eng(c) int13h 2005-2008                   #.....#     *
*                                                  #..@..#     *
*  data structures include                         #.....#     *
*                                                  #######     *
*                                                              *
***************************************************************/

#include "globals.h"

/* static monster info pointed to from spells.id */
typedef struct {
    unsigned short int id;
    char name[33];
    char disp[2];
    short int color;
    short int attack;                           /* 0 = no attack turn */
    short int ranged_damage;
    short int ranged_range;
    short int defense;                          /* Hit Points / 0 = indestructable */
    short int move_range;
    short int dex;
    short int magic_resistance;
    unsigned short int casting_prob;
    bool flight;                                /* Flight? */
    bool mount;
    bool undead;                                /* Undead? */
    short int balance;                          /* 0 = Neutral / -1 = chaos / 1 = law */
} monster;

/* static tree info pointed to from spells.id */
typedef struct {
    unsigned short int id;
    char name[33];
    char disp[2];
    short int color;
    char description[255];
    short int attack;                           /* 0 = no attack turn */
    short int defense;                          /* Hit Points / 0 = indestructable */
    unsigned short int casting_prob;
    unsigned short int casting_range;
    unsigned short int assoc_func;              /* Function to handle spell */
    bool mount;
    bool autospawn;                             /* if true then the game chooses spawn locations */
    bool genspells;                             /* Can generate spells when mounted */
    short int turnlimit;                        /* lives for x turns, 0 = unlimited / -1 random */
    short int uses;
    short int balance;                          /* 0 = Neutral / -1 = chaos / 1 = law */
} tree;

/* static wall info pointed to from spells.id */
typedef struct {
    unsigned short int id;
    char name[33];
    char disp[2];
    short int color;
    char description[255];
    short int attack;                           /* 0 = no attack turn */
    short int defense;                          /* Hit Points / 0 = indestructable */
    unsigned short int casting_prob;
    unsigned short int casting_range;
    unsigned short int assoc_func;              /* Function to handle spell */
    bool mount;
    bool autospawn;                             /* if true then the game chooses spawn locations */
    bool genspells;                             /* Can generate spells when mounted */
    short int turnlimit;                        /* lives for x turns, 0 = unlimited / -1 random */
    short int uses;
    short int balance;                          /* 0 = Neutral / -1 = chaos / 1 = law */
} wall;

/* static blob info pointed to from spells.id */
typedef struct {
    unsigned short int id;
    char name[33];
    char disp[2];
    short int color;
    char description[255];
    short int attack;                           /* 0 = no attack turn */
    short int defense;                          /* Hit Points / 0 = indestructable */
    unsigned short int casting_prob;
    unsigned short int casting_range;
    unsigned short int assoc_func;              /* Function to handle spell */
    bool autospawn;                             /* if true then the game chooses spawn locations */
    unsigned short int devourer;                /* 0 = no remains / 1 = remains left */
    short int turnlimit;                        /* lives for x turns, 0 = unlimited / -1 random */
    short int uses;
    short int balance;                          /* 0 = Neutral / -1 = chaos / 1 = law */
} blob;

/* static info for spells that are enabled by default for all
    players and are ordered by id at the top of spell list */
typedef struct {
    unsigned short int id;
    char name[33];
    char description[255];
    unsigned short int assoc_func;              /* Function to handle spell */
    unsigned short int casting_range;
    unsigned short int casting_prob;
} magic_special;

/* static info for spells that affect world balance */
typedef struct {
    unsigned short int id;
    char name[33];
    char description[255];
    unsigned short int casting_prob;
    short int balance;
} magic_balance;

/* static info for spells are applied to players */
typedef struct {
    unsigned short int id;
    char name[33];
    char disp[2];
    short int color;
    char description[255];
    short int attack;
    short int ranged_damage;
    short int ranged_range;
    short int defense;
    short int move_range;
    bool flight;
    bool attack_undead;
    unsigned short int casting_prob;
    short int balance;
} magic_upgrade;

/* static info for spells that affect world balance */
typedef struct {
    unsigned short int id;
    char name[33];
    char description[255];
    unsigned short int assoc_func;
    bool change_alive;
    bool change_owner;
    bool turn_undead;
    unsigned short int casting_prob;
    unsigned short int casting_range;
    short int balance;
} magic_spell_attrib;

/* static info for magic ranged attack spells */
typedef struct {
    unsigned short int id;
    char name[33];
    char disp[2];
    short int color;
    char description[255];
    unsigned short int assoc_func;
    unsigned short int uses;
    short int ranged_damage;
    short int ranged_range;
    bool beam;
    unsigned short int casting_prob;
    short int balance;
} magic_ranged;

/* dynamic assigned global spell list */
typedef struct {
    unsigned short int id;
    unsigned short int player_id;
    short int spell_type;
    unsigned short int current_pos[3];
    unsigned short int current_defense;         /* Defense this attacking round :: */
    bool illusion;                              /*  reset at round end to monser.defense */
    bool beencast;
    bool dead;
    bool undead;
    bool beenmoved;                             /* Moved this turn */
    bool genspells;                             /* Can generate spells for mounted player? */
    short int turnlimit;                        /* lives for x turns, 0 = unlimited / -1 random */
    short int uses;                             /* How many times the spell can be cast */
    unsigned short int last_killed;
    bool skipround;                             /* Don't perform anything in current round */
} spells;

/*
#######
#..id.#
#######
    0-9     : Null
    10-*    : Players (* MAX_PLAYERS)
    50-99   : Magic_Special
    100+    : All other spells

###############
#..spell_type.#
###############
    0 = player
    1 = monster
    2 = magic_ranged
    3 = tree
    4 = magic_special
    5 = magic_upgrade
    6 = magic_attrib
    7 = magic_balance
    8 = wall
    9 = blob
*/

/* History item */
typedef struct {
    char datetime[21];
    char message[255];
} history_log;

/* Chat item */
typedef struct {
    char datetime[21];
    int player_id;
    char message[255];
} chat_log;

/* unique player specific info */
typedef struct {
    unsigned short int id;
    unsigned short int socket;
    bool ready;
    char name[11];
    char disp[2];
    short int color;
    short int ping; /* bad 0 ~ 3 good */
    unsigned short int spells[MAX_PLAYER_SPELLS];
    unsigned short int total_spells;
    unsigned short int selected_spell;
    short int defense;
    short int attack;
    short int ranged_damage;
    short int ranged_range;
    short int dex;
    short int move_range;
    short int magic_resistance;
    bool flight;
    bool attack_undead;
    unsigned short int upgrades[MAX_MAGIC_UPGRADE]; /* My upgrades stack */
} player;

typedef struct {
    short int balance;
    unsigned short int players;                     /* Number of players playing */
    unsigned short int total_spells;                /* Number of spells generated */
    unsigned short int current_player;              /* Current player */
    unsigned short int selected_item[2];            /* Mode 3 - selected [id][movement_range] */
    unsigned short int layout[MAX_X][MAX_Y][MAX_LAYERS];    /* [x][y][0] = layer(0->MAX_LAYERS) */
    unsigned short int arenasize;
    unsigned short int mode;
    unsigned short int submode;
    bool beenreset;
    short int cursor[2];                            /* Cursor position */
} world;

/*
##########
#..Modes.#
##########
0 = Setup World
1 = Select Spells
2 = Casting Round
3 = Move Round
4 = Grow/Animate/Postround
5 = Game Over

#############
#..Submodes.#
#############
Network Mode :
mode 0
    0 = Connect to IP
    1 = Enter Name (client)
    1 = Wait for Players (server)
    2 = Arena Size (server)
    3 = Wait for Players

Single Player:
mode 0
    0 = number of players
    1 = player names
    2 = arena size

mode 1
    0 = menu
    1 = spell list
    2 = spell detail
    3 = spell select
    4 = illusion?
    5 = view arena
    6 = item info

mode 3
    0 = item select
    1 = range attack
    2 = mount creature?
    3 = dismount creature?
    4 = dismount direction
    5 = move mounted creature
    6 = item info

mode 5
    0 = show winner
    1 = play again?
*/
