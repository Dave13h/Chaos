/***************************************************************
*  history.c                                       #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*                                                  #.....#     *
*  History Log Functions                           #######     *
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
#include "chaos.h"
#include "history.h"

#ifdef WITH_NET
#include "net.h"
#include "net_server.h"
extern bool net_enable;
extern bool net_dedicated;
#endif

extern int logging_level;
extern char log_message[LOGMSGLEN];

history_log myhistory[MAX_HISTORY];
int history_count = 0;

/*
##################
#..history_pop().#
##################
*/
static void history_pop (void)
{
    int i;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,sprintf(log_message,
        "History Full, Shifting Items Up"));

    for(i=0;i<MAX_HISTORY;i++)
        myhistory[i]=myhistory[i+1];

    history_count = MAX_HISTORY-1;
}

/*
##################
#..history_add().#
##################
*/
void history_add (char text[255])
{
    time_t tval = time(NULL);
    struct tm *now;

    now = localtime(&tval);

    if(strlen(text)<1)
        return;

    if(history_count >= MAX_HISTORY)
        history_pop();

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,sprintf(log_message,
        "History[%d] add : %02d:%02d:%02d - %s",history_count, now->tm_hour,
        now->tm_min, now->tm_sec,text));

    memset(myhistory[history_count].datetime,'\0',sizeof(myhistory[history_count].datetime));
    memset(myhistory[history_count].message,'\0',sizeof(myhistory[history_count].message));

    sprintf(myhistory[history_count].datetime,"%02d:%02d:%02d",now->tm_hour,
        now->tm_min,now->tm_sec);

    sprintf(myhistory[history_count].message,"%s",text);

    if(history_count<MAX_HISTORY)
        history_count++;

#ifdef WITH_NET
    /* Send chat to all clients */
    if(net_enable && net_dedicated)
        net_historylog(CE_NET_SOCKET_ALL);
#endif
}
