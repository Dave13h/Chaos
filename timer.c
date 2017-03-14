/***************************************************************
*  timer.c                                         #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*                                                  #.....#     *
*  Timer setup and catcher code                    #######     *
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

#ifdef WITH_NET
#include "net.h"
#include "net_server.h"
#include "net_client.h"
extern bool net_enable;
extern bool net_dedicated;
extern bool net_wait;
#endif

extern world myworld;

extern bool forceupdate;
extern bool beepmsg;

extern char log_message[LOGMSGLEN];

long timer_ticks;
struct timeval last_snap, current_snap;

int msgdecay = 0;

/*
##################
#..catch_timer().#
##################
*/
static void catch_timer(int signum)
{
    timer_ticks++;
    gettimeofday(&current_snap,NULL);
    /*
    // Warning : Insense Logging even when logger function drops it.
    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
        "[%d] TICK %d! (started at %d)",current_snap.tv_sec,timer_ticks,
        last_snap.tv_sec)
    );
    */

    /* Engine Updates */
    if(current_snap.tv_sec - last_snap.tv_sec > 0){

        timer_ticks = 0;
        last_snap.tv_sec = current_snap.tv_sec;

        if(myworld.mode == CE_WORLD_MODE_CASTING
            || myworld.mode == CE_WORLD_MODE_MOVE
            || myworld.mode == CE_WORLD_MODE_POSTROUND)
            forceupdate = true; /* Allow animations to run */

        /* Info bar override timeout */
        if(beepmsg){
            msgdecay++;
            if(msgdecay > 1){
                beepmsg = false;
                msgdecay = 0;
            }
        }
    }

#ifdef WITH_NET
    /* Run Network Updates at twice the tick rate */
    if(net_enable && fmod(timer_ticks,NET_TICK_RATE_DIV) == 0){

        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
            "NET : TICK After %ld Engine Ticks!",timer_ticks));
        if(net_dedicated){
            net_check_server();
            forceupdate = true;

            /* Ping all clients once a second */
            if(timer_ticks == 0)
                net_send_ping(CE_NET_SOCKET_ALL);

        } else {
            net_check_client();
        }
    }
#endif
}


#ifdef POSIXTIMER

/*
##################
#..setup_timer().# // POSIX Timer
##################
*/
void setup_timer(void)
{
    struct itimerspec Timer1Settings;
    timer_t Timer1;
    struct sigaction act;

    /* Use high precision timer on IRIX boxes */
    #ifdef sgi
        clock_t timertype = CLOCK_SGI_FAST;
    #else
        clock_t timertype = CLOCK_REALTIME;
    #endif

    if(timer_create(timertype, NULL, &Timer1) < 0){
        console_log(__FILE__,__func__,__LINE__,LOG_ERROR, sprintf(log_message,
            "Cannot Create Timer."));
        kill_chaos();
    }

    Timer1Settings.it_interval.tv_sec = 0;
    Timer1Settings.it_interval.tv_nsec = (int) 1000000000 / TIMER_TICKS;
    Timer1Settings.it_value.tv_sec = 0;
    Timer1Settings.it_value.tv_nsec = (int) 1000000000 / TIMER_TICKS;

    if (timer_settime(Timer1, 0, &Timer1Settings, NULL) < 0){
        console_log(__FILE__,__func__,__LINE__,LOG_ERROR, sprintf(log_message,
            "Cannot SetTime Timer."));
        kill_chaos();
    }

    /* Callback function to call when timer interval expires */
    act.sa_handler = &catch_timer;
    act.sa_flags = 0;

    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGALRM);

    if (sigaction(SIGALRM, &act, NULL) == -1) {
        console_log(__FILE__,__func__,__LINE__,LOG_ERROR, sprintf(log_message,
            "Failed to SIGAction returned false."));
        kill_chaos();
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Using POSIX based timer @ %d ticks per second.",TIMER_TICKS));

}

#else

/*
##################
#..setup_timer().# // SIGALRM TIMER
##################
*/
void setup_timer(void)
{
    static time_t current_time;
    struct itimerval itv;
    struct sigaction act;

    itv.it_interval.tv_sec = 0;
    itv.it_interval.tv_usec = (int)(1000000 / TIMER_TICKS);
    itv.it_value.tv_sec = 0;
    itv.it_value.tv_usec = (int)(1000000 / TIMER_TICKS);

    memset (&act, 0, sizeof (act));

    act.sa_handler = &catch_timer;
    act.sa_flags = 0;

    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGALRM);

    if (sigaction(SIGALRM, &act, NULL) == -1) {
        console_log(__FILE__,__func__,__LINE__,LOG_ERROR, sprintf(log_message,
            "Cannot sigaction Timer."));
        kill_chaos();
    }

    if (setitimer(ITIMER_REAL, &itv, NULL) == -1) {
        console_log(__FILE__,__func__,__LINE__,LOG_ERROR, sprintf(log_message,
            "Cannot setitimer Timer."));
        kill_chaos();
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Using SIGARLM based timer @ %d ticks per second.",TIMER_TICKS));

    time(&current_time);
}

#endif
