/***************************************************************
*  display_common.h                                #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  common draw routine headers                     #.....#     *
*                                                  #######     *
*                                                              *
***************************************************************/

/* Update globals.h with MAX_PLAYER_COLORS!
    - only count non-light/dark colors
    - exclude the multi's
*/
enum CE_COLORS {
    CE_C_LGREY,     CE_C_GREY,      CE_C_DGREY,
    CE_C_LVIOLET,   CE_C_VIOLET,    CE_C_DVIOLET,
    CE_C_LWHITE,    CE_C_WHITE,     CE_C_DWHITE,
    CE_C_LBLUE,     CE_C_BLUE,      CE_C_DBLUE,
    CE_C_LGREEN,    CE_C_GREEN,     CE_C_DGREEN,
    CE_C_LRED,      CE_C_RED,       CE_C_DRED,
    CE_C_LORANGE,   CE_C_ORANGE,    CE_C_DORANGE,
    CE_C_LYELLOW,   CE_C_YELLOW,    CE_C_DYELLOW,
    CE_C_LBROWN,    CE_C_BROWN,     CE_C_DBROWN,
    CE_C_LPURPLE,   CE_C_PURPLE,    CE_C_DPURPLE,
    CE_C_LCYAN,     CE_C_CYAN,      CE_C_DCYAN,
    CE_C_MULTI1,    CE_C_MULTI2,    CE_C_MULTI3
};

enum CE_VIEWS {
    CE_VIEW_NORMAL = 1,
    CE_VIEW_HISTORY,
    CE_VIEW_CHAT,
    CE_VIEW_DEBUG
};

/*
###############
#..Prototypes.#
###############
*/
int text_offset(int strlength);
void clear_highlights(void);
void init_display(void);
void update_infobar(void);
void drawscene(void);
void shutdown_display(void);
