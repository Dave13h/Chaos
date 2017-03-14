/***************************************************************
*  globals.h                                       #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  global defines/includes                         #.....#     *
*                                                  #######     *
*                                                              *
***************************************************************/

/* X11 */
#ifdef WITH_X11
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
    #include <X11/keysym.h> /* Special key defines */
    #include <X11/Xatom.h>
#endif

/* NCURSES */
#ifdef WITH_NCURSES
    #ifdef sgi
        #include <ncurses/ncurses.h>
    #else
        #include <ncurses.h>
    #endif
#endif

/* Hildon */
#ifdef WITH_HILDON
    #include <hildon/hildon-program.h>
    #include <gtk/gtkmain.h>
    #include <gtk/gtklabel.h>
#endif

/* Network */
#ifdef WITH_NET
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/poll.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <fcntl.h>
    #include <netdb.h>
#endif

#include <stdlib.h>     /* getenv() */
#include <stdio.h>
#include <string.h>

#if defined(__sun)
/* no stdbool, but ncurses has a version */
#define true 1
#define false 0
#else
#include <stdbool.h>
#endif

#include <errno.h>
#include <signal.h>     /* SIG Timer */
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <unistd.h>
#include <getopt.h>

#define RELEASENAME                     "BETA"
#define RELEASEVERSION                  "1.2-NET"
#define LOGMSGLEN                       65563
#define FRONTENDS                       3

#define WIN_WIDTH                       720
#define WIN_HEIGHT                      360

#define NCURSES_WIDTH                   80
#define NCURSES_HEIGHT                  24
#define NCURSES_CURSORS                 4

#define TIMER_TICKS                     75
#define MAX_X                           64
#define MAX_Y                           20
#define MAX_LAYERS                      3
#define MAX_ARENAS                      4

#define MAX_MONSTERS                    64
#define MAX_TREES                       64
#define MAX_WALLS                       64
#define MAX_BLOBS                       64

/* WARNING : Check generate_arena() spawn_points[]! */
#define MAX_PLAYERS                     13
#define MAX_PLAYER_COLORS               10

#define MAX_SPELLS                      448
#define MAX_MAGIC_SPECIAL               64
#define MAX_MAGIC_UPGRADE               64
#define MAX_MAGIC_BALANCE               64
#define MAX_MAGIC_SPELL_ATTRIB          64
#define MAX_MAGIC_RANGED                64

#define MAX_SPELL_TYPES                 9

#define MAX_HISTORY                     15
#define MAX_CHAT                        15
#define MAX_CHATMSG_LENGTH              32

#define MIN_PLAYER_SPELLS               8
#define MAX_PLAYER_SPELLS               16
#define MAX_PLAYER_ATTACK               6
#define MAX_PLAYER_DEFENSE              6
#define MAX_PLAYER_DEX                  9
#define MAX_PLAYER_MAGIC_RESISTANCE 9

#define MAX_PLAYERNAME                  10
#define NEWSPELL_CHANCE                 7
#define ROUNDTURN_EXPIRE_RAND           8
#define BLOB_GROW_CHANCE                6

#define NET_TICK_RATE_DIV               10
#define NET_CLIENT_POLL_TIMEOUT         50
#define NET_DEFAULT_PORT                13100
#define NET_PACKET_SEP                  "|"
#define NET_END_PACKET_SEP              ";"
#define NET_MAX_PACKET_SIZE             32768
#define NET_MAX_PACKET_QUEUE            32
#define NET_MAX_SERIAL_CHUNK            4096
