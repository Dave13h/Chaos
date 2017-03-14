/***************************************************************
*  main.h                                          #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  main.c headers                                  #.....#     *
*                                                  #######     *
*                                                              *
***************************************************************/

/* Don't forget to set FRONTENDS in globals.h! */
enum FRONTEND {
    FE_NONE,
    FE_X11,
    FE_NCURSES,
    FE_HILDON
};

/*
###############
#..Prototypes.#
###############
*/
void kill_chaos(void);
void shutdown_chaos(void);
void find_front_end(void);
