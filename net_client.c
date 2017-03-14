/***************************************************************
*  net_client.c                                    #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  Network Client routines                         #.....#     *
*                                                  #######     *
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
#include "input_common.h"
#include "display_common.h"
#include "sound_common.h"
#include "net.h"
#include "net_client.h"

extern int frontend_mode;
extern bool forceupdate;

extern char infobar_text[255];
extern bool beepmsg;

extern int logging_level;
extern char log_message[LOGMSGLEN];

extern world                myworld;
extern spells               myspells[MAX_SPELLS];
extern player               myplayers[MAX_PLAYERS];
extern monster              mymonsters[MAX_MONSTERS];
extern tree                 mytrees[MAX_TREES];
extern wall                 mywalls[MAX_WALLS];
extern blob                 myblobs[MAX_BLOBS];
extern magic_special        mymagic_special[MAX_MAGIC_SPECIAL];
extern magic_upgrade        mymagic_upgrade[MAX_MAGIC_UPGRADE];
extern magic_balance        mymagic_balance[MAX_MAGIC_BALANCE];
extern magic_spell_attrib   mymagic_spell_attrib[MAX_MAGIC_SPELL_ATTRIB];
extern magic_ranged         mymagic_ranged[MAX_MAGIC_RANGED];

extern chat_log mychat[MAX_CHAT];
extern int chat_count;

extern history_log myhistory[MAX_HISTORY];
extern int history_count;

extern int view_mode;

extern int arenas[MAX_ARENAS][2];

extern bool net_enable;
extern bool net_dedicated;
extern bool net_connected;
extern bool net_wait;
extern int net_port;

int server_socket;
int mycolor;
int mypid;

char partialpacket[NET_MAX_PACKET_SIZE];

/*
######################
#..init_net_client().#
######################
*/
void init_net_client (void)
{
    int clearsock = 1;

    mycolor=0;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,&clearsock,sizeof(int));

    memset(partialpacket,'\0',sizeof(partialpacket));

    console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
        "Net - Net Client Running"));
}

/*
##############################
#..net_server_disconnected().#
##############################
*/
static void net_serverdisconnected(void)
{

    console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
        "Net - Net Connection to Server Lost"));

    /* Clean up networking */
    server_socket = 0;
    net_connected = false;
    net_wait = false;
    init_net_client();

    /* Clean up input */
    empty_buffer();
    chat_empty_buffer();

    /* Clean up world */
    init_world();
    view_mode = CE_VIEW_NORMAL;
    forceupdate = true;

    if(!beepmsg){
        /* Tell the Client */
        sprintf(infobar_text,"Lost Connection to Server");
        beepmsg = true;
    }
}

/*
#############################
#..net_handle_client_data().#
#############################
Gotos:
    NEXTPACKET : Gibberish or end of data.
*/
static void net_handle_client_data (char data[NET_MAX_PACKET_SIZE])
{
    ce_net_request newdata;

    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
        "Net - Incoming Data from %d : %s (Len %d)",server_socket,data,(int)strlen(data)));

    /* Convert data to request struct */
    newdata = net_get_request(data);

    /* Handle 'types' */
    switch(newdata.type){

        /* ===================== Incomming == */
        case CE_NET_PING:
            console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
                "Net - Ping from Socket %d",server_socket));
            net_return_ping(newdata.data);
            break;

        case CE_NET_GAMESTATE:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Net - Game State data from Socket %d",server_socket));
            net_recv_gamestate(newdata.data);
            break;

        case CE_NET_ARENALAYOUT:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Net - Arena Layout data from Socket %d",server_socket));
            net_recv_arenalayout(newdata.data);
            break;

        case CE_NET_PLAYERSTATE:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Net - Player State data from Socket %d",server_socket));
            net_recv_playerstate(newdata.data);
            break;

        case CE_NET_NEWPLAYER:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Net - New Player data from Socket %d",server_socket));
            net_recv_newplayer(newdata.data);
            break;

        case CE_NET_SPELLLIST:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Net - Spell List data from Socket %d",server_socket));
            net_recv_spelllist(newdata.data);
            break;

        case CE_NET_SPELLDATA:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Net - Spell Data from Socket %d",server_socket));
            net_recv_spelldata(newdata.data);
            break;

        case CE_NET_HISTORYLOG:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Net - History Log data from Socket %d",server_socket));
            net_recv_historylog(newdata.data);
            break;

        case CE_NET_CHATLOG:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Net - Chat Log data from Socket %d",server_socket));
            net_recv_chatlog(newdata.data);
            break;

        case CE_NET_READY:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Net - Ready Return from Socket %d",server_socket));
            break;

        case CE_NET_SELECTEDSPELL:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Net - Selected Spell Return from Socket %d",server_socket));
            net_recv_selectedspell(newdata.data);
            break;

        case CE_NET_ACTION:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Net - Action Return from Socket %d",server_socket));
            net_recv_actionreturn(newdata.data);
            break;

        case CE_NET_ENDTURN:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Net - End Turn Return from Socket %d",server_socket));
            break;

        case CE_NET_BEEPMSG:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Net - Recv Beep Msg from Socket %d",server_socket));
            net_recv_beepmsg(newdata.data);
            break;

        /* ===================== Unknown/Bad == */

        case CE_NET_BADCRC:
        case CE_NET_INVALID:
        default:
            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "Net - Unrecongised Data from Socket %d : %c",server_socket,data[0]));
            //net_invalid(server_socket);
            break;
    }
}

/*
#######################
#..net_check_client().#
#######################

XXX FIXME :
    Ok, this is fairly bad but is working for now.
    Issue is that the client might recv() less than the full packet of data
    from the server when it is checking, so we have a global buffer (partialpacket)
    to hold left overs. That's the good part.

    The bad news is that this is a bit of a memory juggling clusterfuck, which could
    be resolved with some nice malloc() routines.

    The ugly news is that strtok in our net_unserialise() function likes to fuck
    around with the data we are passing to it to sort hense the juggling the data
    before passing it in, if that doesn't happen the array we pass to it gets fucked
    ...Proper fucked

*/
void net_check_client (void)
{
    int nbytes, r;
    char data_in[NET_MAX_PACKET_SIZE];
    ce_net_serial_data packets;
    struct pollfd ufds[1];
    int i=0;
    int totallength=0;
    int partiallen=0;

    ufds[0].fd = server_socket;
    ufds[0].events = POLLIN;
    ufds[0].revents = POLLIN;

    r = poll(ufds,1,NET_CLIENT_POLL_TIMEOUT);

    /* No Dataz Waiting */
    if(r < 1)
        return;

    memset(data_in,'\0',sizeof(data_in));

    if ((nbytes = recv(server_socket, data_in, sizeof(data_in), 0)) <= 0) {
        if (nbytes == 0) {
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                sprintf(log_message,"Net - Socket %d Disconnected",server_socket));

            close(server_socket);
            net_serverdisconnected();
            return;
        }
    }
    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
        sprintf(log_message,"Net - received %d bytes of data from server",nbytes));

    /* Deal with it */
    if(nbytes > 0){
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
            sprintf(log_message,"Net - Data From Server - %s",data_in));

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
            sprintf(log_message,"Net - Data in Partial Packet - %s",partialpacket));

        sprintf(partialpacket,"%s%s",partialpacket,data_in);

        partiallen = strlen(partialpacket);

        /* Oh shit, dupe the data before sending to net_serialise() */
        memset(data_in,'\0',sizeof(data_in));
        memcpy(data_in,partialpacket,strlen(partialpacket));

        packets = net_unserialise(data_in,";");

        if(strlen(partialpacket) > 0 && partialpacket[partiallen-1] != ';')
            packets.len--;

        /* Handle Good Packets */
        for(i=0;i<packets.len;i++){
            totallength += strlen(packets.data[i])+1;
            net_handle_client_data(packets.data[i]);
        }

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
            sprintf(log_message,"Net - data_in len : %d - total processed packets len : %d - partial packet len - %d",
            (int)strlen(data_in),totallength,partiallen
        ));

        /* More data left? */
        if(partiallen > totallength){
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                sprintf(log_message,"Net - Incomplete Data Packet (unused %d/%d) - %s",
                (partiallen - totallength),(int)strlen(partialpacket),data_in)
            );

            memset(data_in,'\0',sizeof(data_in));
            memcpy(data_in,partialpacket,strlen(partialpacket));
            memset(partialpacket,'\0',sizeof(partialpacket));
            strncpy(partialpacket,&data_in[totallength],(partiallen - totallength));

            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                sprintf(log_message,"Net - Left over Data in Partial Packet - %s",partialpacket));

        } else {
            memset(partialpacket,'\0',sizeof(partialpacket));
        }
    }
}

/*
======================================================================= Requests
*/

/*
######################
#..net_return_ping().#
######################
*/
void net_return_ping (char ping[256])
{
    char packet[NET_MAX_PACKET_SIZE],data[MAX_CHATMSG_LENGTH];

    memset(data,'\0',sizeof(data));
    memset(packet,'\0',sizeof(packet));

    sprintf(data,"%s",ping);

    sprintf(packet,"%d|%d|%s|%s",CE_NET_PING,(int)strlen(data),data,net_gencrc(data));

    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
        "Net - Returning Ping to Server : %s (Packed Data Len %d)",
        packet,(int)strlen(data))
    );

    net_send(server_socket, packet);
}

/*
##########################
#..net_connecttoserver().#
##########################
*/
bool net_connecttoserver(char address[256])
{
    struct sockaddr_in server_addr;
    struct hostent *he;
    int flags, n;
    int error = 0;
    socklen_t len;
    fd_set rset, wset;
    struct timeval tval;

    if(!valid_ipaddress(address) && !valid_input(address)){
        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
            "Net - Invalid Server address : %s (Packed Data Len %d)",
            address,(int)strlen(address))
        );
        sprintf(infobar_text,"Invalid Server Address");
        beepmsg = true;
        return false;
    }

    if((he=gethostbyname(address)) == NULL){
        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
            "Net - Failed to Lookup Server at : %s (Packed Data Len %d)",
            address,(int)strlen(address))
        );
        return false;
    }

    /* Set non-blocking */
    flags = fcntl(server_socket,F_SETFL,0);
    fcntl(server_socket,F_SETFL,flags|O_NONBLOCK);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(net_port);
    server_addr.sin_addr = *((struct in_addr *)he->h_addr);

    memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Attempting to Connect to Server at : %s (Packed Data Len %d)",
        address,(int)strlen(address))
    );

    if((n=connect(server_socket,(struct sockaddr *)&server_addr,sizeof(server_addr))) < 0) {
        if(errno != EINPROGRESS) {
            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "Net - Failed to Connect to Server at : %s (Packed Data Len %d) errno : %d",
                address,(int)strlen(address),errno)
            );

            /* Tell the Client */
            sprintf(infobar_text,"Failed to connect to Server");
            beepmsg = true;
            return false;
        }
    }

    if(n==0)
        goto CONNECTED;

    /* Connect is taking longer than expected but hasn't failed.. */
    FD_ZERO(&rset);
    FD_SET(server_socket,&rset);
    wset = rset;
    tval.tv_sec = 2;    /* 2 seconds timeout */
    tval.tv_usec = 0;

    if((n=select(server_socket+1,&rset,&wset,NULL,&tval))==0){
        close(server_socket);   /* Timedout */
        errno = ETIMEDOUT;

        /* Tell the Client */
        sprintf(infobar_text,"Failed to connect to Server");
        beepmsg = true;
        return false;
    }

    if(FD_ISSET(server_socket, &rset) || FD_ISSET(server_socket,&wset)){
        len = sizeof(error);
        if(getsockopt(server_socket,SOL_SOCKET,SO_ERROR,&error,&len) < 0)
            return false; /* Solaris pending error */
    } else {
        return false;
    }

    /* Huge Success! */
    CONNECTED:

    /* Return socket to normal */
    fcntl(server_socket,F_SETFL,flags);

    /* Something went wrong? */
    if(error){
        close(server_socket);
        errno=error;

        /* Tell the Client */
        sprintf(infobar_text,"Failed to connect to Server");
        beepmsg = true;

        return false;
    }

    net_connected = true;
    myworld.submode++;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Connected to Server at : %s (Packed Data Len %d)",
        address,(int)strlen(address))
    );

    /* Redraw Display */
    forceupdate = true;

    return true;
}

/*
###################
#..net_joingame().#
###################
*/
void net_joingame (char name[256])
{
    char packet[NET_MAX_PACKET_SIZE],data[NET_MAX_PACKET_SIZE];

    memset(data,'\0',sizeof(data));
    memset(packet,'\0',sizeof(packet));

    sprintf(data,"%s~%d",name,mycolor);

    sprintf(packet,"%d|%d|%s|%s",CE_NET_NEWPLAYER,(int)strlen(data),data,net_gencrc(data));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Join Game Command sent to Server : %s (Packed Data Len %d)",
        packet,(int)strlen(data))
    );

    net_send(server_socket, packet);

    /* Tell the frontend to wait for Network */
    net_wait = true;
}

/*
#######################
#..net_send_chatmsg().#
#######################
*/
void net_send_chatmsg (char msg[MAX_CHATMSG_LENGTH])
{
    char packet[NET_MAX_PACKET_SIZE],data[MAX_CHATMSG_LENGTH];

    memset(data,'\0',sizeof(data));
    memset(packet,'\0',sizeof(packet));

    sprintf(data,"%s",msg);

    sprintf(packet,"%d|%d|%s|%s",CE_NET_CHATMSG,(int)strlen(data),data,net_gencrc(data));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Chat Msg sent to Server : %s (Packed Data Len %d)",
        packet,(int)strlen(data))
    );

    net_send(server_socket, packet);
}

/*
#####################
#..net_send_ready().#
#####################
*/
void net_send_ready (void)
{
    char packet[NET_MAX_PACKET_SIZE],data[MAX_CHATMSG_LENGTH];

    memset(data,'\0',sizeof(data));
    memset(packet,'\0',sizeof(packet));

    sprintf(data,"0");

    sprintf(packet,"%d|%d|%s|%s",CE_NET_READY,(int)strlen(data),data,net_gencrc(data));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Ready sent to Server : %s (Packed Data Len %d)",
        packet,(int)strlen(data))
    );

    net_send(server_socket, packet);
}

/*
#############################
#..net_send_selectedspell().#
#############################
*/
void net_send_selectedspell (int spid, bool illusion)
{
    char packet[NET_MAX_PACKET_SIZE],data[MAX_CHATMSG_LENGTH];

    /* Don't send anything if dead */
    if(isspelldead(myplayers[mypid].id+10))
        return;

    memset(data,'\0',sizeof(data));
    memset(packet,'\0',sizeof(packet));

    sprintf(data,"%d~%d",spid,illusion);

    sprintf(packet,"%d|%d|%s|%s",CE_NET_SELECTEDSPELL,(int)strlen(data),data,net_gencrc(data));

    console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
        "Net - Ready sent to Server : %s (Packed Data Len %d)",
        packet,(int)strlen(data))
    );

    net_send(server_socket, packet);
}

/*
####################
#..net_send_move().#
####################
*/
void net_send_move (int cur_x, int cur_y, int to_x, int to_y)
{
    char packet[NET_MAX_PACKET_SIZE],data[MAX_CHATMSG_LENGTH];

    /* Don't send anything if dead */
    if(isspelldead(myplayers[mypid].id+10))
        return;

    memset(data,'\0',sizeof(data));
    memset(packet,'\0',sizeof(packet));

    sprintf(data,"%d~%d~%d~%d",cur_x,cur_y,to_x,to_y);

    sprintf(packet,"%d|%d|%s|%s",CE_NET_MOVE,(int)strlen(data),data,net_gencrc(data));

    console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
        "Net - Action sent to Server : %s (Packed Data Len %d)",
        packet,(int)strlen(data))
    );

    net_send(server_socket, packet);
}

/*
#####################
#..net_send_mount().#
#####################
*/
void net_send_mount (int dir, int action)
{
    char packet[NET_MAX_PACKET_SIZE],data[MAX_CHATMSG_LENGTH];

    memset(data,'\0',sizeof(data));
    memset(packet,'\0',sizeof(packet));

    sprintf(data,"%d~%d",dir,action);

    sprintf(packet,"%d|%d|%s|%s",CE_NET_MOUNT,(int)strlen(data),data,net_gencrc(data));

    console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
        "Net - Mount sent to Server : %s (Packed Data Len %d)",
        packet,(int)strlen(data))
    );

    net_send(server_socket, packet);
}

/*
######################
#..net_send_action().#
######################
*/
void net_send_action (int action, int cur_x, int cur_y)
{
    char packet[NET_MAX_PACKET_SIZE],data[MAX_CHATMSG_LENGTH];

    memset(data,'\0',sizeof(data));
    memset(packet,'\0',sizeof(packet));

    sprintf(data,"%d~%d~%d",action,cur_x,cur_y);

    sprintf(packet,"%d|%d|%s|%s",CE_NET_ACTION,(int)strlen(data),data,net_gencrc(data));

    console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
        "Net - Action sent to Server : %s (Packed Data Len %d)",
        packet,(int)strlen(data))
    );

    net_send(server_socket, packet);
}

/*
#######################
#..net_send_endturn().#
#######################
*/
void net_send_endturn (void)
{
    char packet[NET_MAX_PACKET_SIZE],data[MAX_CHATMSG_LENGTH];

    memset(data,'\0',sizeof(data));
    memset(packet,'\0',sizeof(packet));

    sprintf(data,"0");

    sprintf(packet,"%d|%d|%s|%s",CE_NET_ENDTURN,(int)strlen(data),data,net_gencrc(data));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - End Turn sent to Server : %s (Packed Data Len %d)",
        packet,(int)strlen(data))
    );

    net_send(server_socket, packet);
}

/*
#######################
#..net_client_color().#
#######################
*/
void net_client_color (void)
{
    mycolor++;
    if(mycolor > MAX_PLAYER_COLORS)
        mycolor = 0;
}

/*
======================================================================== Returns
*/

/*
#########################
#..net_recv_gamestate().#
#########################
Keys :
    0  - mode
    1  - submode
    2  - total spells
    3  - total players
    4  - current player
    5  - selected item.id
    6  - selected item.range
    7  - arena size
    8  - balance
    9  - cursor x_position
    10 - cursor y_position
*/
void net_recv_gamestate (char data[NET_MAX_PACKET_SIZE])
{
    char *d = data;
    char *newdata = NULL;
    world newworld;
    int i=0;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Processing gamestate"));

    newdata = strtok(d,"~");
    while(newdata != NULL){
        switch(i){
            case 0:
                newworld.mode = atoi(newdata);
                break;
            case 1:
                newworld.submode = atoi(newdata);
                break;
            case 2:
                newworld.total_spells = atoi(newdata);
                break;
            case 3:
                newworld.players = atoi(newdata);
                break;
            case 4:
                newworld.current_player = atoi(newdata);
                break;
            case 5:
                newworld.selected_item[0] = atoi(newdata);
                break;
            case 6:
                newworld.selected_item[1] = atoi(newdata);
                break;
            case 7:
                newworld.arenasize = atoi(newdata);
                break;
            case 8:
                newworld.balance = atoi(newdata);
                break;
            case 9:
                newworld.cursor[0] = atoi(newdata);
                break;
            case 10:
                newworld.cursor[1] = atoi(newdata);
                break;
            default:
                /* Too much data, BAD! */
                goto BADDATA;
        }

        i++;

        /* Move to Next field */
        newdata = strtok(NULL,"~");
    }

    if(i!=11){
        BADDATA:
        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
            "Net - Bad field count : %d fields (expected 11)",i+1));
        return;
    }

    /* Assume Data is good. Only copy in values we have! */
    myworld.mode = newworld.mode;
    myworld.submode = newworld.submode;
    myworld.total_spells = newworld.total_spells;
    myworld.players = newworld.players;
    myworld.current_player = newworld.current_player;
    myworld.arenasize = newworld.arenasize;
    myworld.balance = newworld.balance;

    /* If player is dead, don't overwrite local cursor system */
    if(!isspelldead(myplayers[mypid].id+10)){
        myworld.selected_item[0] = newworld.selected_item[0];
        myworld.selected_item[1] = newworld.selected_item[1];
        myworld.cursor[0] = newworld.cursor[0];
        myworld.cursor[1] = newworld.cursor[1];
    }

    /* Unlock Network */
    net_wait = false;
    forceupdate = true;
}

/*
###########################
#..net_recv_arenalayout().#
###########################
*/
void net_recv_arenalayout (char data[NET_MAX_PACKET_SIZE])
{
    char *d = data;
    char *newdata = NULL;
    unsigned short int layout[MAX_X][MAX_Y][MAX_LAYERS];
    int i=0;
    int j=0;
    int l=0;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Processing gamestate"));

    newdata = strtok(d,"~");
    while(newdata != NULL){

        layout[i][j][l] = atoi(newdata);

        l++;
        if(l>=MAX_LAYERS){
            l=0;
            j++;
            if(j>=arenas[myworld.arenasize][1]){
                j=0;
                i++;
                if(i>arenas[myworld.arenasize][0]){
                    console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                        "Net - Too much data"));
                    return;
                }
            }
        }

        /* Move to Next field */
        newdata = strtok(NULL,"~");
    }

    for(i=0;i<arenas[myworld.arenasize][0];i++){
        for(j=0;j<arenas[myworld.arenasize][1];j++){
            for(l=0;l<MAX_LAYERS;l++){
                myworld.layout[i][j][l] = layout[i][j][l];
            }
        }
    }

    /* Unlock Network */
    net_wait = false;
}

/*
###########################
#..net_recv_playerstate().#
###########################
Values :
    id~name~readystate~color
*/
void net_recv_playerstate (char data[NET_MAX_PACKET_SIZE])
{
    char *newdata = NULL;
    player newplayer;
    int i=0;
    int p=0;
    ce_net_serial_data players;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Processing Player States"));

    players = net_unserialise(data,",");

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Total Players in State : %d",players.len));

    for(p=0;p<players.len;p++){
        i=0;
        newdata = strtok((char *)players.data[p],"~");
        while(newdata != NULL){
            switch(i){
                case 0:
                    newplayer.id = atoi(newdata);
                    break;
                case 1:
                    memset(newplayer.name,'\0',sizeof(newplayer.name));
                    sprintf(newplayer.name,"%s",newdata);
                    break;
                case 2:
                    newplayer.ready = atoi(newdata);
                    break;
                case 3:
                    newplayer.color = atoi(newdata);
                    break;
                case 4:
                    newplayer.ping = atoi(newdata);
                    break;
                default:
                    /* Too much data, BAD! */
                    goto BADDATA;
            }
            i++;

            /* Move to Next field */
            newdata = strtok(NULL,"~");
        }

        /* Assume this is a blank player, clean the player table just in case */
        if(i==4){
            newplayer.socket = 0;
            myplayers[p].id = 0;
            memset(myplayers[p].name,'\0',sizeof(myplayers[p].name));
            myplayers[p].ready = 0;
            myplayers[p].color = 0;
            myplayers[p].ping = 0;
            continue;

        /* Can't be trusted */
        }else if(i!=5){
            BADDATA:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Net - Bad field count for player %d : %d fields (expected 5)",p,i+1));
            continue;
        }

        /* Client doesn't need to know */
        newplayer.socket = 0;

        /* Only store supplied data! */
        myplayers[p].id = newplayer.id;
        memset(myplayers[p].name,'\0',sizeof(myplayers[p].name));
        sprintf(myplayers[p].name,"%s",newplayer.name);
        myplayers[p].ready = newplayer.ready;
        myplayers[p].color = newplayer.color;
        myplayers[p].ping = newplayer.ping;
    }

    forceupdate = true;
}

/*
#########################
#..net_recv_newplayer().#
#########################
Return format :
    0 - result (-2=malformed / -1=wrong mode / 0=server full / 1=success)
    1 - player id
*/
void net_recv_newplayer (char data[NET_MAX_PACKET_SIZE])
{
    char *d = data;
    char *newdata = NULL;
    int status=-2;
    int i=0;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Processing New Player Request Return"));

    newdata = strtok(d,"~");
    while(newdata != NULL){
        switch(i){
            case 0:
                status = atoi(newdata);
                break;
            case 1:
                /* Player id */
                mypid = atoi(newdata);
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Net - My Player id : %d",mypid));
                break;
            default:
                /* Too much data, BAD! */
                goto BADDATA;
        }

        i++;

        /* Move to Next field */
        newdata = strtok(NULL,"~");
    }

    if(i!=2){
        BADDATA:
        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
            "Net - Received too much data : %d fields (expected 2)",i+1));
        return;
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - New Player Status : %d",status));

    switch(status){
        case -2: /* Re-enter name */
            myworld.mode = 0;
            myworld.submode = 1;
            sprintf(infobar_text,"Invalid Name");
            beepmsg = true;
            break;
        case -1: /* Wrong Mode */
            myworld.mode = 0;
            myworld.submode = 1;
            sprintf(infobar_text,"Connection Failed : Game in Progress");
            beepmsg = true;
            net_serverdisconnected();
            break;
        case 0: /* Server Full */
            myworld.mode = 0;
            myworld.submode = 1;
            sprintf(infobar_text,"Connection Failed : Server is Full");
            beepmsg = true;
            net_serverdisconnected();
            break;
        case 1: /* Success, wait for players */
            myworld.mode = 0;
            myworld.submode = 2;
            break;
        default:
            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "Net - Unknown return : %d ",status));
            break;
    }

    net_wait = false;
    forceupdate = true;
}

/*
#########################
#..net_recv_spelllist().#
#########################
*/
void net_recv_spelllist (char data[NET_MAX_PACKET_SIZE])
{
    char *newdata = NULL;
    spells newentry;
    int i=0;
    int e=0;
    int spellid;

    ce_net_serial_data newitems;

    console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
        "Net - Processing Spell list (mode %d / sub %d)",myworld.mode,myworld.submode));

    newitems = net_unserialise(data,",");

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Total entries in list : %d",newitems.len));

    for(e=0;e<newitems.len;e++){
        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
            "Net - Spell List - Processing : %d",e));

        i=0;
        newdata = strtok((char *)newitems.data[e],"~");
        while(newdata != NULL){
            switch(i){
                case 0:
                    spellid = atoi(newdata);
                    break;
                case 1:
                    newentry.id = atoi(newdata);
                    break;
                case 2:
                    newentry.player_id = atoi(newdata);
                    break;
                case 3:
                    newentry.spell_type = atoi(newdata);
                    break;
                case 4:
                    newentry.current_pos[0] = atoi(newdata);
                    break;
                case 5:
                    newentry.current_pos[1] = atoi(newdata);
                    break;
                case 6:
                    newentry.current_pos[2] = atoi(newdata);
                    break;
                case 7:
                    newentry.current_defense = atoi(newdata);
                    break;
                case 8:
                    newentry.illusion = atoi(newdata);
                    break;
                case 9:
                    newentry.beencast = atoi(newdata);
                    break;
                case 10:
                    newentry.dead = atoi(newdata);
                    break;
                case 11:
                    newentry.undead = atoi(newdata);
                    break;
                case 12:
                    newentry.beenmoved = atoi(newdata);
                    break;
                case 13:
                    newentry.genspells = atoi(newdata);
                    break;
                case 14:
                    newentry.turnlimit = atoi(newdata);
                    break;
                case 15:
                    newentry.uses = atoi(newdata);
                    break;
                case 16:
                    newentry.last_killed = atoi(newdata);
                    break;
                case 17:
                    newentry.skipround = atoi(newdata);
                    break;
                default:
                    /* Too much data, BAD! */
                    goto BADDATA;
            }
            i++;

            /* Move to Next field */
            newdata = strtok(NULL,"~");
        }

        if(i!=18){
            BADDATA:
            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "Net - Bad field count for Spell List %d : %d fields (expected 18)",e,i+1));
            continue;
        }

        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
            "Net - Spell List - Safe Entry : %d",e));

        /* Good Data */
        myspells[spellid] = newentry;
    }
}

/*
#########################
#..net_recv_spelldata().#
#########################
    This will receive the data then work
    out which func to call to handle it.
    net_recv_data_<TYPE>().
*/
void net_recv_spelldata (char data[NET_MAX_PACKET_SIZE])
{
    ce_net_serial_data newitems;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Processing Spell Data"));

    newitems = net_unserialise(data,",");

    console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
        "Net - Total entries in Spell Data list : %d (spell type %d)",
        newitems.len-1,atoi(newitems.data[0]))
    );

    if(newitems.len < 1){
        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
            "Net - Nothing to Process : %d entries", newitems.len));
        return;
    }

    if((int)strlen(newitems.data[0])>2){
        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
            "Net - Absurd Spell Type : %s", newitems.data[0]));
        return;
    }

    switch(atoi(newitems.data[0])){
        case SPELL_PLAYER:
            net_recv_data_player(newitems.data,newitems.len);
            break;

        case SPELL_MONSTER:
            net_recv_data_monster(newitems.data,newitems.len);
            break;

        case SPELL_MAGIC_RANGED:
            net_recv_data_magic_ranged(newitems.data,newitems.len);
            break;

        case SPELL_TREE:
            net_recv_data_tree(newitems.data,newitems.len);
            break;

        case SPELL_MAGIC_SPECIAL:
            net_recv_data_magic_special(newitems.data,newitems.len);
            break;

        case SPELL_MAGIC_UPGRADE:
            net_recv_data_magic_upgrade(newitems.data,newitems.len);
            break;

        case SPELL_MAGIC_ATTRIB:
            net_recv_data_magic_attrib(newitems.data,newitems.len);
            break;

        case SPELL_MAGIC_BALANCE:
            net_recv_data_magic_balance(newitems.data,newitems.len);
            break;

        case SPELL_WALL:
            net_recv_data_wall(newitems.data,newitems.len);
            break;

        case SPELL_BLOB:
            net_recv_data_blob(newitems.data,newitems.len);
            break;

        /* Unknown Spell Type / Bad Request */
        default:
            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "Net - Invalid Spell Type Request : %d (original request : %s) (Packed Data Len %d)",
                atoi(newitems.data[0]),newitems.data[0],(int)strlen(newitems.data[0]))
            );
            break;
    }
    forceupdate = true;
}
/*
###########################
#..net_recv_data_player().#
###########################
*/
void net_recv_data_player (char data[MAX_SPELLS][NET_MAX_SERIAL_CHUNK], int total_entries)
{
    char *newdata = NULL;
    char spelldata[MAX_SPELLS*3], upgradedata[MAX_MAGIC_UPGRADE*3];
    ce_net_serial_data spells, upgrades;
    player newentry;
    int i=0;
    int e=0;
    int s=0;
    int playerid;

    /* Skip first entry, was spell type header! */
    for(e=1;e<total_entries;e++){
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Player Data - Processing : %d (%s)",e,data[e]));

        i=0;
        memset(upgradedata,'\0',sizeof(upgradedata));
        memset(spelldata,'\0',sizeof(spelldata));

        newdata = strtok((char *)data[e],"~");
        while(newdata != NULL){
            switch(i){
                case 0:
                    playerid = atoi(newdata);
                    break;
                case 1:
                    newentry.id = atoi(newdata);
                    break;
                case 2:
                    newentry.socket = atoi(newdata);
                    break;
                case 3:
                    newentry.ready = atoi(newdata);
                    break;
                case 4:
                    memset(newentry.name,'\0',sizeof(newentry.name));
                    sprintf(newentry.name,"%s",newdata);
                    break;
                case 5:
                    memset(newentry.disp,'\0',sizeof(newentry.disp));
                    sprintf(newentry.disp,"%s",newdata);
                    break;
                case 6:
                    newentry.color = atoi(newdata);
                    break;
                case 7:
                    newentry.ping = atoi(newdata);
                    break;
                case 8:
                    sprintf(spelldata,"%s",newdata);
                    break;
                case 9:
                    newentry.total_spells = atoi(newdata);
                    break;
                case 10:
                    newentry.selected_spell = atoi(newdata);
                    break;
                case 11:
                    newentry.defense = atoi(newdata);
                    break;
                case 12:
                    newentry.attack = atoi(newdata);
                    break;
                case 13:
                    newentry.ranged_damage = atoi(newdata);
                    break;
                case 14:
                    newentry.ranged_range = atoi(newdata);
                    break;
                case 15:
                    newentry.dex = atoi(newdata);
                    break;
                case 16:
                    newentry.move_range = atoi(newdata);
                    break;
                case 17:
                    newentry.magic_resistance = atoi(newdata);
                    break;
                case 18:
                    newentry.flight = atoi(newdata);
                    break;
                case 19:
                    newentry.attack_undead = atoi(newdata);
                    break;
                case 20:
                    sprintf(upgradedata,"%s",newdata);
                    break;
                default:
                    /* Too much data, BAD! */
                    goto BADDATA;
            }
            i++;

            /* Move to Next field  */
            newdata = strtok(NULL,"~");
        }

        /* Spell List */
        for(s=0;s<MAX_PLAYER_SPELLS;s++)
            newentry.spells[s] = 0;

        spells = net_unserialise(spelldata,":");
        console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
            "Net - Player Data - Unserialised Spells : %d ",spells.len));

        if(spells.len > 0){
            for(s=0;s<spells.len;s++){
                console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
                    "Net - Spell %d item id in spells : %s ",s+1,spells.data[s]));
                newentry.spells[s+1] = atoi(spells.data[s]);
            }
        }

        /* Upgrade List */
        for(s=0;s<MAX_MAGIC_UPGRADE;s++)
            newentry.upgrades[s] = 0;

        upgrades = net_unserialise(upgradedata,":");
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Player Data - Unserialised Upgrades : %d ",upgrades.len));

        if(upgrades.len > 0){
            for(s=0;s<upgrades.len;s++){
                console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
                    "Net - Upgrade item id in upgrades : %s ",upgrades.data[s]));
                newentry.upgrades[s] = atoi(upgrades.data[s]);
            }
        }

        /* Upgrade field could be empty */
        if(i!=20 && i!=21){
            BADDATA:
            console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
                "Net - Bad field count for Player List %d : %d fields (expected 20/21)",e,i+1));
            continue;
        }

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Spell List - Safe Entry : %d",e));

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Player Data - Total Spells : %d",newentry.total_spells));

        /* Good Data */
        myplayers[playerid] = newentry;
    }
}

/*
############################
#..net_recv_data_monster().#
############################
*/
void net_recv_data_monster (char data[MAX_SPELLS][NET_MAX_SERIAL_CHUNK], int total_entries)
{
    char *newdata = NULL;
    monster newentry;
    int i=0;
    int e=0;
    int monsterid;

    /* Skip first entry, was spell type header! */
    for(e=1;e<total_entries;e++){
        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
            "Net - Monster Data - Processing : %d",e));

        i=0;

        newdata = strtok((char *)data[e],"~");
        while(newdata != NULL){
            switch(i){
                case 0:
                    monsterid = atoi(newdata);
                    break;
                case 1:
                    newentry.id = atoi(newdata);
                    break;
                case 2:
                    memset(newentry.name,'\0',sizeof(newentry.name));
                    sprintf(newentry.name,"%s",newdata);
                    break;
                case 3:
                    memset(newentry.disp,'\0',sizeof(newentry.disp));
                    sprintf(newentry.disp,"%s",newdata);
                    break;
                case 4:
                    newentry.color = atoi(newdata);
                    break;
                case 5:
                    newentry.attack = atoi(newdata);
                    break;
                case 6:
                    newentry.ranged_damage = atoi(newdata);
                    break;
                case 7:
                    newentry.ranged_range = atoi(newdata);
                    break;
                case 8:
                    newentry.defense = atoi(newdata);
                    break;
                case 9:
                    newentry.move_range = atoi(newdata);
                    break;
                case 10:
                    newentry.dex = atoi(newdata);
                    break;
                case 11:
                    newentry.magic_resistance = atoi(newdata);
                    break;
                case 12:
                    newentry.casting_prob = atoi(newdata);
                    break;
                case 13:
                    newentry.flight = atoi(newdata);
                    break;
                case 14:
                    newentry.mount = atoi(newdata);
                    break;
                case 15:
                    newentry.undead = atoi(newdata);
                    break;
                case 16:
                    newentry.balance = atoi(newdata);
                    break;
                default:
                    /* Too much data, BAD! */
                    goto BADDATA;
            }
            i++;

            /* Move to Next field  */
            newdata = strtok(NULL,"~");
        }

        if(i!=17){
            BADDATA:
            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "Net - Bad field count for Monster List %d : %d fields (expected 17)",e,i+1));
            continue;
        }

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Spell List - Safe Entry : %d",e));

        /* Good Data */
        mymonsters[monsterid] = newentry;
    }
}

/*
#################################
#..net_recv_data_magic_ranged().#
#################################
*/
void net_recv_data_magic_ranged (char data[MAX_SPELLS][NET_MAX_SERIAL_CHUNK], int total_entries)
{
    char *newdata = NULL;
    magic_ranged newentry;
    int i=0;
    int e=0;
    int rangedid;

    /* Skip first entry, was spell type header! */
    for(e=1;e<total_entries;e++){
        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
            "Net - Magic Ranged Data - Processing : %d",e));

        i=0;

        newdata = strtok((char *)data[e],"~");
        while(newdata != NULL){
            switch(i){
                case 0:
                    rangedid = atoi(newdata);
                    break;
                case 1:
                    newentry.id = atoi(newdata);
                    break;
                case 2:
                    memset(newentry.name,'\0',sizeof(newentry.name));
                    sprintf(newentry.name,"%s",newdata);
                    break;
                case 3:
                    memset(newentry.disp,'\0',sizeof(newentry.disp));
                    sprintf(newentry.disp,"%s",newdata);
                    break;
                case 4:
                    newentry.color = atoi(newdata);
                    break;
                case 5:
                    memset(newentry.description,'\0',sizeof(newentry.description));
                    sprintf(newentry.description,"%s",newdata);
                    break;
                case 6:
                    newentry.assoc_func = atoi(newdata);
                    break;
                case 7:
                    newentry.uses = atoi(newdata);
                    break;
                case 8:
                    newentry.ranged_damage = atoi(newdata);
                    break;
                case 9:
                    newentry.ranged_range = atoi(newdata);
                    break;
                case 10:
                    newentry.beam = atoi(newdata);
                    break;
                case 11:
                    newentry.casting_prob = atoi(newdata);
                    break;
                case 12:
                    newentry.balance = atoi(newdata);
                    break;
                default:
                    /* Too much data, BAD! */
                    goto BADDATA;
            }
            i++;

            /* Move to Next field  */
            newdata = strtok(NULL,"~");
        }

        if(i!=13){
            BADDATA:
            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "Net - Bad field count for Magic Ranged List %d : %d fields (expected 13)",e,i+1));
            continue;
        }

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Spell List - Safe Entry : %d",e));

        /* Good Data */
        mymagic_ranged[rangedid] = newentry;
    }
}

/*
#########################
#..net_recv_data_tree().#
#########################
*/
void net_recv_data_tree (char data[MAX_SPELLS][NET_MAX_SERIAL_CHUNK], int total_entries)
{
    char *newdata = NULL;
    tree newentry;
    int i=0;
    int e=0;
    int treeid;

    /* Skip first entry, was spell type header! */
    for(e=1;e<total_entries;e++){
        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
            "Net - Tree Data - Processing : %d",e));

        i=0;

        newdata = strtok((char *)data[e],"~");
        while(newdata != NULL){
            switch(i){
                case 0:
                    treeid = atoi(newdata);
                    break;
                case 1:
                    newentry.id = atoi(newdata);
                    break;
                case 2:
                    memset(newentry.name,'\0',sizeof(newentry.name));
                    sprintf(newentry.name,"%s",newdata);
                    break;
                case 3:
                    memset(newentry.disp,'\0',sizeof(newentry.disp));
                    sprintf(newentry.disp,"%s",newdata);
                    break;
                case 4:
                    newentry.color = atoi(newdata);
                    break;
                case 5:
                    memset(newentry.description,'\0',sizeof(newentry.description));
                    sprintf(newentry.description,"%s",newdata);
                    break;
                case 6:
                    newentry.attack = atoi(newdata);
                    break;
                case 7:
                    newentry.defense = atoi(newdata);
                    break;
                case 8:
                    newentry.casting_prob = atoi(newdata);
                    break;
                case 9:
                    newentry.casting_range = atoi(newdata);
                    break;
                case 10:
                    newentry.assoc_func = atoi(newdata);
                    break;
                case 11:
                    newentry.mount = atoi(newdata);
                    break;
                case 12:
                    newentry.autospawn = atoi(newdata);
                    break;
                case 13:
                    newentry.genspells = atoi(newdata);
                    break;
                case 14:
                    newentry.turnlimit = atoi(newdata);
                    break;
                case 15:
                    newentry.uses = atoi(newdata);
                    break;
                case 16:
                    newentry.balance = atoi(newdata);
                    break;
                default:
                    /* Too much data, BAD! */
                    goto BADDATA;
            }
            i++;

            /* Move to Next field  */
            newdata = strtok(NULL,"~");
        }

        if(i!=17){
            BADDATA:
            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "Net - Bad field count for Tree List %d : %d fields (expected 17)",e,i+1));
            continue;
        }

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Spell List - Safe Entry : %d",e));

        /* Good Data */
        mytrees[treeid] = newentry;
    }
}

/*
##################################
#..net_recv_data_magic_special().#
##################################
*/
void net_recv_data_magic_special (char data[MAX_SPELLS][NET_MAX_SERIAL_CHUNK], int total_entries)
{
    char *newdata = NULL;
    magic_special newentry;
    int i=0;
    int e=0;
    int specialid;

    /* Skip first entry, was spell type header! */
    for(e=1;e<total_entries;e++){
        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
            "Net - Magic Special Data - Processing : %d",e));

        i=0;

        newdata = strtok((char *)data[e],"~");
        while(newdata != NULL){
            switch(i){
                case 0:
                    specialid = atoi(newdata);
                    break;
                case 1:
                    newentry.id = atoi(newdata);
                    break;
                case 2:
                    memset(newentry.name,'\0',sizeof(newentry.name));
                    sprintf(newentry.name,"%s",newdata);
                    break;
                case 3:
                    memset(newentry.description,'\0',sizeof(newentry.description));
                    sprintf(newentry.description,"%s",newdata);
                    break;
                case 4:
                    newentry.assoc_func = atoi(newdata);
                    break;
                case 5:
                    newentry.casting_range = atoi(newdata);
                    break;
                case 6:
                    newentry.casting_prob = atoi(newdata);
                    break;
                default:
                    /* Too much data, BAD! */
                    goto BADDATA;
            }
            i++;

            /* Move to Next field  */
            newdata = strtok(NULL,"~");
        }

        if(i!=7){
            BADDATA:
            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "Net - Bad field count for Magic Special List %d : %d fields (expected 7)",e,i+1));
            continue;
        }

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Spell List - Safe Entry : %d",e));

        /* Good Data */
        mymagic_special[specialid] = newentry;
    }
}

/*
##################################
#..net_recv_data_magic_upgrade().#
##################################
*/
void net_recv_data_magic_upgrade (char data[MAX_SPELLS][NET_MAX_SERIAL_CHUNK], int total_entries)
{
    char *newdata = NULL;
    magic_upgrade newentry;
    int i=0;
    int e=0;
    int upgradeid;

    /* Skip first entry, was spell type header! */
    for(e=1;e<total_entries;e++){
        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
            "Net - Magic Upgrade Data - Processing : %d",e));

        i=0;

        newdata = strtok((char *)data[e],"~");
        while(newdata != NULL){
            switch(i){
                case 0:
                    upgradeid = atoi(newdata);
                    break;
                case 1:
                    newentry.id = atoi(newdata);
                    break;
                case 2:
                    memset(newentry.name,'\0',sizeof(newentry.name));
                    sprintf(newentry.name,"%s",newdata);
                    break;
                case 3:
                    memset(newentry.disp,'\0',sizeof(newentry.disp));
                    sprintf(newentry.disp,"%s",newdata);
                    break;
                case 4:
                    newentry.color = atoi(newdata);
                    break;
                case 5:
                    memset(newentry.description,'\0',sizeof(newentry.description));
                    sprintf(newentry.description,"%s",newdata);
                    break;
                case 6:
                    newentry.attack = atoi(newdata);
                    break;
                case 7:
                    newentry.ranged_damage = atoi(newdata);
                    break;
                case 8:
                    newentry.ranged_range = atoi(newdata);
                    break;
                case 9:
                    newentry.defense = atoi(newdata);
                    break;
                case 10:
                    newentry.move_range = atoi(newdata);
                    break;
                case 11:
                    newentry.flight = atoi(newdata);
                    break;
                case 12:
                    newentry.attack_undead = atoi(newdata);
                    break;
                case 13:
                    newentry.balance = atoi(newdata);
                    break;
                case 14:
                    newentry.casting_prob = atoi(newdata);
                    break;
                default:
                    /* Too much data, BAD! */
                    goto BADDATA;
            }
            i++;

            /* Move to Next field  */
            newdata = strtok(NULL,"~");
        }

        if(i!=15){
            BADDATA:
            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "Net - Bad field count for Magic Upgrade List %d : %d fields (expected 15)",e,i+1));
            continue;
        }

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Spell List - Safe Entry : %d",e));

        /* Good Data */
        mymagic_upgrade[upgradeid] = newentry;
    }
}

/*
#################################
#..net_recv_data_magic_attrib().#
#################################
*/
void net_recv_data_magic_attrib (char data[MAX_SPELLS][NET_MAX_SERIAL_CHUNK], int total_entries)
{
    char *newdata = NULL;
    magic_spell_attrib newentry;
    int i=0;
    int e=0;
    int attribid;

    /* Skip first entry, was spell type header! */
    for(e=1;e<total_entries;e++){
        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
            "Net - Magic Spell Attrib Data - Processing : %d",e));

        i=0;

        newdata = strtok((char *)data[e],"~");
        while(newdata != NULL){
            switch(i){
                case 0:
                    attribid = atoi(newdata);
                    break;
                case 1:
                    newentry.id = atoi(newdata);
                    break;
                case 2:
                    memset(newentry.name,'\0',sizeof(newentry.name));
                    sprintf(newentry.name,"%s",newdata);
                    break;
                case 3:
                    memset(newentry.description,'\0',sizeof(newentry.description));
                    sprintf(newentry.description,"%s",newdata);
                    break;
                case 4:
                    newentry.assoc_func = atoi(newdata);
                    break;
                case 5:
                    newentry.change_alive = atoi(newdata);
                    break;
                case 6:
                    newentry.change_owner = atoi(newdata);
                    break;
                case 7:
                    newentry.turn_undead = atoi(newdata);
                    break;
                case 8:
                    newentry.casting_prob = atoi(newdata);
                    break;
                case 9:
                    newentry.casting_range = atoi(newdata);
                    break;
                case 10:
                    newentry.balance = atoi(newdata);
                    break;
                default:
                    /* Too much data, BAD! */
                    goto BADDATA;
            }
            i++;

            /* Move to Next field  */
            newdata = strtok(NULL,"~");
        }

        if(i!=11){
            BADDATA:
            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "Net - Bad field count for Magic Spell Attrib List %d : %d fields (expected 11)",e,i+1));
            continue;
        }

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Spell List - Safe Entry : %d",e));

        /* Good Data */
        mymagic_spell_attrib[attribid] = newentry;
    }
}

/*
##################################
#..net_recv_data_magic_balance().#
##################################
*/
void net_recv_data_magic_balance (char data[MAX_SPELLS][NET_MAX_SERIAL_CHUNK], int total_entries)
{
    char *newdata = NULL;
    magic_balance newentry;
    int i=0;
    int e=0;
    int balanceid;

    /* Skip first entry, was spell type header! */
    for(e=1;e<total_entries;e++){
        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
            "Net - Magic Balance Data - Processing : %d",e));

        i=0;

        newdata = strtok((char *)data[e],"~");
        while(newdata != NULL){
            switch(i){
                case 0:
                    balanceid = atoi(newdata);
                    break;
                case 1:
                    newentry.id = atoi(newdata);
                    break;
                case 2:
                    memset(newentry.name,'\0',sizeof(newentry.name));
                    sprintf(newentry.name,"%s",newdata);
                    break;
                case 3:
                    memset(newentry.description,'\0',sizeof(newentry.description));
                    sprintf(newentry.description,"%s",newdata);
                    break;
                case 4:
                    newentry.casting_prob = atoi(newdata);
                    break;
                case 5:
                    newentry.balance = atoi(newdata);
                    break;
                default:
                    /* Too much data, BAD! */
                    goto BADDATA;
            }
            i++;

            /* Move to Next field  */
            newdata = strtok(NULL,"~");
        }

        if(i!=6){
            BADDATA:
            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "Net - Bad field count for Magic Balance List %d : %d fields (expected 6)",e,i+1));
            continue;
        }

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Spell List - Safe Entry : %d",e));

        /* Good Data */
        mymagic_balance[balanceid] = newentry;
    }
}

/*
#########################
#..net_recv_data_wall().#
#########################
*/
void net_recv_data_wall (char data[MAX_SPELLS][NET_MAX_SERIAL_CHUNK], int total_entries)
{
    char *newdata = NULL;
    wall newentry;
    int i=0;
    int e=0;
    int wallid;

    /* Skip first entry, was spell type header! */
    for(e=1;e<total_entries;e++){
        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
            "Net - Wall Data - Processing : %d",e));

        i=0;

        newdata = strtok((char *)data[e],"~");
        while(newdata != NULL){
            switch(i){
                case 0:
                    wallid = atoi(newdata);
                    break;
                case 1:
                    newentry.id = atoi(newdata);
                    break;
                case 2:
                    memset(newentry.name,'\0',sizeof(newentry.name));
                    sprintf(newentry.name,"%s",newdata);
                    break;
                case 3:
                    memset(newentry.disp,'\0',sizeof(newentry.disp));
                    sprintf(newentry.disp,"%s",newdata);
                    break;
                case 4:
                    newentry.color = atoi(newdata);
                    break;
                case 5:
                    memset(newentry.description,'\0',sizeof(newentry.description));
                    sprintf(newentry.description,"%s",newdata);
                    break;
                case 6:
                    newentry.attack = atoi(newdata);
                    break;
                case 7:
                    newentry.defense = atoi(newdata);
                    break;
                case 8:
                    newentry.casting_prob = atoi(newdata);
                    break;
                case 9:
                    newentry.casting_range = atoi(newdata);
                    break;
                case 10:
                    newentry.assoc_func = atoi(newdata);
                    break;
                case 11:
                    newentry.mount = atoi(newdata);
                    break;
                case 12:
                    newentry.autospawn = atoi(newdata);
                    break;
                case 13:
                    newentry.genspells = atoi(newdata);
                    break;
                case 14:
                    newentry.turnlimit = atoi(newdata);
                    break;
                case 15:
                    newentry.uses = atoi(newdata);
                    break;
                case 16:
                    newentry.balance = atoi(newdata);
                    break;
                default:
                    /* Too much data, BAD! */
                    goto BADDATA;
            }
            i++;

            /* Move to Next field  */
            newdata = strtok(NULL,"~");
        }

        if(i!=17){
            BADDATA:
            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "Net - Bad field count for Wall List %d : %d fields (expected 17)",e,i+1));
            continue;
        }

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Spell List - Safe Entry : %d",e));

        /* Good Data */
        mywalls[wallid] = newentry;
    }
}

/*
#########################
#..net_recv_data_blob().#
#########################
*/
void net_recv_data_blob (char data[MAX_SPELLS][NET_MAX_SERIAL_CHUNK], int total_entries)
{
    char *newdata = NULL;
    blob newentry;
    int i=0;
    int e=0;
    int blobid;

    /* Skip first entry, was spell type header! */
    for(e=1;e<total_entries;e++){
        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
            "Net - Blob Data - Processing : %d",e));

        i=0;

        newdata = strtok((char *)data[e],"~");
        while(newdata != NULL){
            switch(i){
                case 0:
                    blobid = atoi(newdata);
                    break;
                case 1:
                    newentry.id = atoi(newdata);
                    break;
                case 2:
                    memset(newentry.name,'\0',sizeof(newentry.name));
                    sprintf(newentry.name,"%s",newdata);
                    break;
                case 3:
                    memset(newentry.disp,'\0',sizeof(newentry.disp));
                    sprintf(newentry.disp,"%s",newdata);
                    break;
                case 4:
                    newentry.color = atoi(newdata);
                    break;
                case 5:
                    memset(newentry.description,'\0',sizeof(newentry.description));
                    sprintf(newentry.description,"%s",newdata);
                    break;
                case 6:
                    newentry.attack = atoi(newdata);
                    break;
                case 7:
                    newentry.defense = atoi(newdata);
                    break;
                case 8:
                    newentry.casting_prob = atoi(newdata);
                    break;
                case 9:
                    newentry.casting_range = atoi(newdata);
                    break;
                case 10:
                    newentry.assoc_func = atoi(newdata);
                    break;
                case 11:
                    newentry.autospawn = atoi(newdata);
                    break;
                case 12:
                    newentry.devourer = atoi(newdata);
                    break;
                case 13:
                    newentry.turnlimit = atoi(newdata);
                    break;
                case 14:
                    newentry.uses = atoi(newdata);
                    break;
                case 15:
                    newentry.balance = atoi(newdata);
                    break;
                default:
                    /* Too much data, BAD! */
                    goto BADDATA;
            }
            i++;

            /* Move to Next field  */
            newdata = strtok(NULL,"~");
        }

        if(i!=16){
            BADDATA:
            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "Net - Bad field count for Blob List %d : %d fields (expected 16)",e,i+1));
            continue;
        }

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Spell List - Safe Entry : %d",e));

        /* Good Data */
        myblobs[blobid] = newentry;
    }
}


/*
##########################
#..net_recv_historylog().#
##########################
*/
void net_recv_historylog (char data[NET_MAX_PACKET_SIZE])
{
    char *newdata = NULL;
    history_log newentry;
    ce_net_serial_data newlog;
    int i=0;
    int e=0;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Processing history log"));

    newlog = net_unserialise(data,",");

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Total entries in log : %d",newlog.len));

    if(newlog.len < 0){
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - History Log - Nothing to Process"));
        return;
    }

    history_count = 0;

    for(e=0;e<newlog.len;e++){
        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
            "Net - History - Processing : %d",e));

        i=0;
        newdata = strtok((char *)newlog.data[e],"~");
        while(newdata != NULL){
            switch(i){
                case 0:
                    /* id field */
                    break;
                case 1:
                    memset(newentry.datetime,'\0',sizeof(newentry.datetime));
                    sprintf(newentry.datetime,"%s",newdata);
                    break;
                case 2:
                    memset(newentry.message,'\0',sizeof(newentry.message));
                    sprintf(newentry.message,"%s",newdata);
                    break;
                default:
                    /* Too much data, BAD! */
                    goto BADDATA;
            }
            i++;

            /* Move to Next field */
            newdata = strtok(NULL,"~");
        }

        if(i!=3){
            BADDATA:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Net - Bad field count for history log %d : %d fields (expected 3)",e,i+1));
            continue;
        }

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - History - Safe Entry : %d (%s / %s)",e,newentry.datetime,newentry.message));

        /* Good Data */
        myhistory[e] = newentry;
        history_count++;
    }

    forceupdate = true;
}

/*
#######################
#..net_recv_chatlog().#
#######################
*/
void net_recv_chatlog (char data[NET_MAX_PACKET_SIZE])
{
    char *newdata = NULL;
    chat_log newentry;
    ce_net_serial_data newlog;
    int i=0;
    int e=0;
    static bool firstrun = true;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Processing chat log"));

    newlog = net_unserialise(data,",");

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Total entries in log : %d",newlog.len));

    if(newlog.len < 0){
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Chat Log - Nothing to Process"));
        return;
    }

    chat_count = 0;

    for(e=0;e<newlog.len;e++){
        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
            "Net - Chat - Processing : %d",e));

        i=0;
        newdata = strtok((char *)newlog.data[e],"~");
        while(newdata != NULL){
            switch(i){
                case 0:
                    /* id field */
                    break;
                case 1:
                    memset(newentry.datetime,'\0',sizeof(newentry.datetime));
                    sprintf(newentry.datetime,"%s",newdata);
                    break;
                case 2:
                    newentry.player_id = atoi(newdata);
                    break;
                case 3:
                    memset(newentry.message,'\0',sizeof(newentry.message));
                    sprintf(newentry.message,"%s",newdata);
                    break;
                default:
                    /* Too much data, BAD! */
                    goto BADDATA;
            }
            i++;

            /* Move to Next field */
            newdata = strtok(NULL,"~");
        }

        if(i!=4){
            BADDATA:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Net - Bad field count for chat log %d : %d fields (expected 4)",e,i+1));
            continue;
        }

        console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
            "Net - Chat - Safe Entry : %d (%s / %d / %s)",e,newentry.datetime,
            newentry.player_id,newentry.message)
        );

        /* Good Data */
        mychat[e] = newentry;
        chat_count++;
    }

    if(!firstrun){
        sprintf(infobar_text,"New Chat Message(s) Received");
        beepmsg = true;
    }
    firstrun = false;

    forceupdate = true;
}

/*
#############################
#..net_recv_selectedspell().#
#############################
Return format :
    0 - result (-1=invalid mode)
*/
void net_recv_selectedspell (char data[NET_MAX_PACKET_SIZE])
{
    int status=atoi(data);

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Processing Selected Spell Return"));

    switch(status){
        case -1: /* Invalid Spell Selection */
            sprintf(infobar_text,"Invalid Spell Selection");
            beepmsg = true;
            break;
        default:
            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "Net - Unknown return : %d ",status));
            break;
    }

    net_wait = false;
    forceupdate = true;
}

/*
############################
#..net_recv_actionreturn().#
############################
*/
void net_recv_actionreturn (char data[NET_MAX_PACKET_SIZE])
{
    int status=atoi(data);

    console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
        "Net - Processing Action Return - %d",status));

    switch(status){
        case CE_NET_ACTION_FAIL_LOS:
            sprintf(infobar_text,"Not in line of sight");
            beepmsg = true;
            CE_beep();
            break;
        case CE_NET_ACTION_FAIL_OWNER:
            sprintf(infobar_text,"Target Does Not Belong To You");
            beepmsg = true;
            CE_beep();
            break;
        case CE_NET_ACTION_FAIL_DIST:
            sprintf(infobar_text,"Out of range");
            beepmsg = true;
            CE_beep();
            break;
        case CE_NET_ACTION_FAIL_INVALID:
            sprintf(infobar_text,"Invalid Move");
            beepmsg = true;
            CE_beep();
            break;
        case CE_NET_ACTION_ENGAGED:
            sprintf(infobar_text,"Engaged To Creature");
            beepmsg = true;
            CE_beep();
            break;
        default:
            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "Net - Unknown return : %d ",status));
            break;
    }

    net_wait = false;
    forceupdate = true;
}

/*
#######################
#..net_recv_beepmsg().#
#######################
*/
void net_recv_beepmsg (char data[NET_MAX_PACKET_SIZE])
{
    char *d = data;
    char *newdata = NULL;
    char newbeepmsg[255];
    int dobeep = FALSE;
    int i=0;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Processing BeepMsg"));

    newdata = strtok(d,"~");
    while(newdata != NULL){
        switch(i){
            case 0:
                memset(newbeepmsg,'\0',sizeof(newbeepmsg));
                sprintf(newbeepmsg,"%s",newdata);
                break;
            case 1:
                dobeep = atoi(newdata);
                break;
            default:
                /* Too much data, BAD! */
                goto BADDATA;
        }

        i++;

        /* Move to Next field */
        newdata = strtok(NULL,"~");
    }

    if(i!=2){
        BADDATA:
        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
            "Net - Bad field count : %d fields (expected 2)",i+1));
        return;
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - BeepMsg - Safe Entry : (%s / Beep - %d)",newdata,dobeep)
    );

    sprintf(infobar_text,"%s",newbeepmsg);
    beepmsg = dobeep;

    /* Unlock Network */
    net_wait = false;
    forceupdate = true;
}
