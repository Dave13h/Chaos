/***************************************************************
*  log.h                                           #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  log.c headers                                   #.....#     *
*                                                  #######     *
*                                                              *
***************************************************************/

enum LOG_RATING {
    LOG_NONE = -1,
    LOG_ERROR,
    LOG_WARNING,
    LOG_NOTICE,
    LOG_DEBUG,
    LOG_ALL
};

/*
###############
#..Prototypes.#
###############
*/
void set_logging_level(int level);
void open_logfile(void);
void close_logfile(void);
void console_log(char *file,const char *func, int line, int log_level, int messagelen);
void dump_grid(void);
void dump_spells(void);
