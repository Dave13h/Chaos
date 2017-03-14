/***************************************************************
*  display_ncurses.h                               #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  draw routines headers for ncurses               #.....#     *
*                                                  #######     *
*                                                              *
***************************************************************/

/*
###############
#..Prototypes.#
###############
*/
void init_ncurses(void);
void shutdown_ncurses(void);
void ncurses_changecursor(void);

void drawhistory_ncurses(void);
void drawchat_ncurses(void);
void drawdebug_ncurses(void);
void drawfindserver_ncurses(void);
void drawwaitforserver_ncurses(void);
void drawserver_ncurses(void);
void drawscene_ncurses(void);
