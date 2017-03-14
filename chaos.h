/***************************************************************
*  chaos.h                                         #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  chaos.c headers                                 #.....#     *
*                                                  #######     *
*                                                              *
***************************************************************/
enum CE_WORLD_MODE {
    CE_WORLD_MODE_SETUP = 0,
    CE_WORLD_MODE_SELECTSPELLS,
    CE_WORLD_MODE_CASTING,
    CE_WORLD_MODE_MOVE,
    CE_WORLD_MODE_POSTROUND,
    CE_WORLD_MODE_ENDGAME
};

enum SPELL_TYPES {
    SPELL_NULL = -1,
    SPELL_PLAYER,
    SPELL_MONSTER,
    SPELL_MAGIC_RANGED,
    SPELL_TREE,
    SPELL_MAGIC_SPECIAL,
    SPELL_MAGIC_UPGRADE,
    SPELL_MAGIC_ATTRIB,
    SPELL_MAGIC_BALANCE,
    SPELL_WALL,
    SPELL_BLOB
};

/*
###############
#..Prototypes.#
###############
*/

/* Arena */
int checkdistance(void);
int itemstack_top(int x, int y);
bool checklos(void);
bool checkadjacent(void);
bool checkadjacent_any(int cur_x, int cur_y);

/* Can/Has/Is */
bool can_fly(int x, int y);
bool is_mount(int x, int y);
bool is_mounted(int x, int y);

/* Actions */
void mount_item(int x, int y);
void moveitem(int x, int y) ;
bool creature_attack(int old_x, int old_y);
void creature_death (int spellid, int old_x, int old_y);

/* Spells */
void cast_spell(void);

/* World */
void init_world(void);
void reset_world(void);
void skipdeadplayers(void);
bool allplayersready(void);
void setplayersready(bool readystate);
bool isspelldead(int spellid);

/* Game Loop */
void gameloop_local(void);
