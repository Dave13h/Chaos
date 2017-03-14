/***************************************************************
*  main.c                                          #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*                                                  #.....#     *
*  Main stuff happens here =]                      #######     *
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
#include "timer.h"
#include "chaos.h"
#include "generate.h"
#include "history.h"
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

int frontend_mode = FE_NONE;
bool forceupdate = true;

extern int logging_level;
extern char log_message[LOGMSGLEN];

#ifdef WITH_NET
#include "net.h"
#include "net_server.h"
#include "net_client.h"
extern bool net_enable;
extern bool net_dedicated;
extern bool net_connected;
extern bool net_wait;
extern int net_port;
#endif

/*
#####################
#..find_front_end().#
#####################
*/
void find_front_end(void)
{
    frontend_mode++;

    console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
        "Looking for next frontend (mode : %d)",frontend_mode));

    /* Run out of frontends to test? */
    if(frontend_mode > FRONTENDS)
        frontend_mode = FE_NONE;

    switch(frontend_mode){
        case FE_X11:
        case FE_NCURSES:
        case FE_HILDON:
            init_display();
            break;

        case FE_NONE:
        default:
            console_log(__FILE__,__func__,__LINE__,LOG_ERROR, sprintf(log_message,
                "ERROR: Failed to init() a working frontend"));
            printf("***No suitible frontend found***\n");
            shutdown_chaos();
            break;
    }
}

/*
####################
#..set_front_end().#
####################
*/
static void set_front_end(char *frontend)
{

    if(!strcasecmp(frontend,"x11")){
    #ifndef WITH_X11
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,sprintf(log_message,
            "Chaos has not been compiled with X11 Front End Support"));
    #else
        frontend_mode = FE_X11;
    #endif
    }

    if(!strcasecmp(frontend,"ncurses")){
    #ifndef WITH_NCURSES
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,sprintf(log_message,
            "Chaos has not been compiled with NCurses Front End Support"));
    #else
        frontend_mode = FE_NCURSES;
    #endif
    }

    if(!strcasecmp(frontend,"hildon")){
    #ifndef WITH_HILDON
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,sprintf(log_message,
            "Chaos has not been compiled with Hildon Front End Support"));
    #else
        frontend_mode = FE_HILDON;
    #endif
    }

    /* Still not found a front end? Guess! */
    if(frontend_mode == FE_NONE)
        find_front_end();
}

/*
#####################
#..shutdown_chaos().#
#####################
*/
void shutdown_chaos(void)
{
    shutdown_display();
    printf("***Shutting Down***\n");
    console_log(__FILE__,__func__,__LINE__,LOG_ALL,
        sprintf(log_message,"***Shutting Down***"));
    close_logfile();
    exit(1);
}

/*
#################
#..kill_chaos().#
#################
*/
void kill_chaos(void)
{
    printf("*************************\n");
    printf("*                       *\n");
    printf("* Fatal Error Occourred *\n");
    printf("*   Check the logfile   *\n");
    printf("*                       *\n");
    printf("*************************\n");
    close_logfile();
    exit(-1);
}

/*
###########
#..main().#
###########
*/
int main(int argc, char **argv)
{
    int carg;
    bool connecttoserver = false;
    char serveraddr[16];

    open_logfile();

    printf("***Chaos Console (%s %s RELEASE)***\n",RELEASEVERSION,RELEASENAME);
    console_log(__FILE__,__func__,__LINE__,LOG_ALL,sprintf(log_message,
        "***Chaos Console (%s %s RELEASE)***",RELEASEVERSION,RELEASENAME));

    frontend_mode = FE_NONE;

    /*
    ###########################
    #..Parse.commandline.args.#
    ###########################
        l = Logging Level
        f = Front End
        n = Networking
        d = Dedicated Server
        p = Network Port
        s = Connect Server IP
    */
    while ((carg = getopt (argc, argv, "l:f:p:s:nd")) != -1){
        switch (carg) {
            case 'l':
                set_logging_level(atoi(optarg));
                break;
            case 'f':
                set_front_end(optarg);
                break;
#ifdef WITH_NET
            case 'n':
                net_enable = true;
                break;
            case 'd':
                net_enable = true;
                net_dedicated = true;
                break;
            case 's':
                if(strlen(optarg) < 16 && !net_dedicated){
                    sprintf(serveraddr,"%s",optarg);
                    connecttoserver = true;
                }
                break;
            case 'p':
                net_port = atoi(optarg);
                break;
#endif
        }
    }

    /*
    ###############################
    #..Setup.and.connect.to.timer.#
    ###############################
    */
    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,sprintf(log_message,
        "Setting up timer..."));
    setup_timer();

#ifdef WITH_NET
    /*
    ###############
    #..Setup.Net..#
    ###############
    */
    if(net_enable){
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,sprintf(log_message,
            "Setting up Network Connections..."));

        if(net_dedicated)
            init_net_server();
        else
            init_net_client();
    }
#endif

    /*
    #################################
    #..Find.a.suitible.display.mode.#
    #################################
    */
    /* Guess? */
    if(frontend_mode == FE_NONE)
        find_front_end(); /* Inits display during tests */
    else
        init_display();

    init_input();

    /*
    #######################
    #..Default.world.vars.#
    #######################
    */
    init_world();

    history_add("Welcome to Chaos!");


#ifdef WITH_NET
    /* Net_client command line connect to server? */
    if(net_enable && !net_dedicated && connecttoserver)
        net_connecttoserver(serveraddr);
#endif

    /*
    ###############
    #..Game.Loops.#
    ###############
    */
    for(;;) {
#ifdef WITH_NET
        if(net_enable)
            if(net_dedicated)
                gameloop_net_server();
            else
                gameloop_net_client();
        else
#endif
            gameloop_local();

        check_input();
        drawscene();
        pause(); /* Wait for timer to fire */
    }
}
