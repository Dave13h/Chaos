/***************************************************************
*  chat.c                                          #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*                                                  #.....#     *
*  Chat Log Functions                              #######     *
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
#include "chat.h"
#ifdef WITH_NET
#include "net.h"
#include "net_server.h"
extern bool net_enable;
extern bool net_dedicated;
#endif

extern int logging_level;
extern char log_message[LOGMSGLEN];

extern char infobar_text[255];
extern bool beepmsg;

chat_log mychat[MAX_CHAT];
int chat_count = 0;

char chat_buffer[MAX_CHATMSG_LENGTH];

/*
########################
#..chat_empty_buffer().#
########################
*/
void chat_empty_buffer (void)
{
    memset(chat_buffer,'\0',sizeof(chat_buffer));
}

/*
###############
#..chat_pop().#
###############
*/
static void chat_pop (void)
{
    int i;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,sprintf(log_message,
        "Chat Full, Shifting Items Up"));

    for(i=0;i<MAX_CHAT;i++)
        mychat[i]=mychat[i+1];

    chat_count = MAX_CHAT-1;
}

/*
###############
#..chat_add().#
###############
*/
void chat_add (int player_id, char text[255])
{
    time_t tval = time(NULL);
    struct tm *now;

    now = localtime(&tval);

    if(strlen(text)<1){
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,sprintf(log_message,
            "Chat - MSG Too Short : Len %d",(int)strlen(text)));
        return;
    }

    if(chat_count >= MAX_CHAT)
        chat_pop();

    sprintf(infobar_text,"New Chat Message Received");
    beepmsg = true;

    /* Send chat to all clients */
    if(net_dedicated)
        net_chatlog(CE_NET_SOCKET_ALL);

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,sprintf(log_message,
        "Chat[%d] add : %02d:%02d:%02d - %s",chat_count, now->tm_hour,
        now->tm_min, now->tm_sec,text));

    memset(mychat[chat_count].datetime,'\0',sizeof(mychat[chat_count].datetime));
    memset(mychat[chat_count].message,'\0',sizeof(mychat[chat_count].message));

    sprintf(mychat[chat_count].datetime,"%02d:%02d:%02d",now->tm_hour,
        now->tm_min,now->tm_sec);

    mychat[chat_count].player_id = player_id;

    sprintf(mychat[chat_count].message,"%s",text);

    if(chat_count<MAX_CHAT)
        chat_count++;
#ifdef WITH_NET
    /* Send chat to all clients */
    if(net_dedicated)
        net_chatlog(CE_NET_SOCKET_ALL);
#endif
}
