/***************************************************************
*  net_server.c                                    #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  Network Server routines                         #.....#     *
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
#include "input_common.h"
#include "display_common.h"
#include "history.h"
#include "chat.h"
#include "net.h"
#include "net_server.h"

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
extern history_log myhistory[MAX_HISTORY];

extern int arenas[MAX_ARENAS][2];

extern bool net_enable;
extern bool net_dedicated;
extern bool net_connected;
extern bool net_wait;
extern int net_port;

fd_set master, read_fds;
struct sockaddr_in my_addr, their_addr;
socklen_t addrlen;
int fdmax, listener, newfd;
struct timeval tv;

/*
######################
#..init_net_server().#
######################
*/
void init_net_server (void)
{
    int clearsock = 1;

    tv.tv_sec = 0;
    tv.tv_usec = 50000;

    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    listener = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&clearsock,sizeof(int));

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(net_port);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    memset(my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero));

    console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
        "Net - Server running on IP %s Port %d",
        inet_ntoa(my_addr.sin_addr), htons(my_addr.sin_port)
    ));

    bind(listener, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
    listen(listener, MAX_PLAYERS);

    FD_SET(listener, &master);
    fdmax = listener;
}

/*
#############################
#..net_handle_server_data().#
#############################
Gotos:
    NEXTPACKET : Gibberish or end of data.
*/
static void net_handle_server_data (int socket, char data[NET_MAX_PACKET_SIZE])
{
    ce_net_request newdata;
    char *p = data;
    char *packet = NULL;
    char packets[NET_MAX_PACKET_QUEUE][NET_MAX_PACKET_SIZE];
    int i=0;
    int total_packets=0;

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Incoming Data from %d : %s (Len %d)",socket,data,(int)strlen(data)));

    /* Could be multiple packets in data, run get to the chopper! */
    packet = strtok(p,NET_END_PACKET_SEP);
    while(packet != NULL){
        /* Request is too small to be usable */
        if(strlen(packet) < 5)
            goto NEXTPACKET;

        sprintf(packets[total_packets],"%s",packet);
        total_packets++;

        /* Move to Next Packet */
        NEXTPACKET:
        packet = strtok(NULL,NET_END_PACKET_SEP);

    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Total Packets this 'tick' : %d",total_packets));

    /* Do stuff wit the usable packets */
    for(i=0;i<total_packets;i++){

        /* Convert data to request struct */
        newdata = net_get_request(packets[i]);

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Data type %d / Len %d / Data %s",
            newdata.type,newdata.len,newdata.data));

        /* Handle 'types' */
        switch(newdata.type){

            case CE_NET_PING:
                console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
                    "Net - Ping from Socket %d",socket));
                net_recv_ping(socket,newdata.data);
                break;

            /* ===================== Returns == */

            case CE_NET_GAMESTATE:
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Net - Game State request from Socket %d",socket));
                net_gamestate(socket);
                break;

            case CE_NET_ARENALAYOUT:
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Net - Arena Layout request from Socket %d",socket));
                net_arenalayout(socket);
                break;

            case CE_NET_PLAYERSTATE:
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Net - Player State request from Socket %d",socket));
                net_playerstate(socket);
                break;

            case CE_NET_SPELLLIST:
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Net - Spell List request from Socket %d",socket));
                net_spelllist(socket);
                break;

            case CE_NET_SPELLDATA:
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Net - Spell Data request from Socket %d",socket));
                net_spelldata(socket,atoi(newdata.data));
                break;

            case CE_NET_HISTORYLOG:
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Net - History Log Request from Socket %d",socket));
                net_historylog(socket);
                break;

            case CE_NET_CHATLOG:
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Net - Chat Log Request from Socket %d",socket));
                net_chatlog(socket);
                break;

            /* ===================== Commands == */

            case CE_NET_READY:
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Net - Set Player Ready State from Socket %d",socket));
                net_ready(socket);
                break;

            case CE_NET_NEWPLAYER:
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Net - New Player Request from Socket %d",socket));
                net_newplayer(socket,newdata.data);
                break;

            case CE_NET_CHATMSG:
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Net - Chat MSG Received from Socket %d",socket));
                net_chatmsg(socket,newdata.data);
                break;

            case CE_NET_SELECTEDSPELL:
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Net - Selected Spell Received from Socket %d",socket));
                net_recv_spellselection(socket,newdata.data);
                break;

            case CE_NET_MOVE:
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Net - Move Received from Socket %d",socket));
                net_recv_move(socket,newdata.data);
                break;

            case CE_NET_ACTION:
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Net - Action Received from Socket %d",socket));
                net_recv_action(socket,newdata.data);
                break;

            case CE_NET_MOUNT:
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Net - Mount Received from Socket %d",socket));
                net_recv_mount(socket,newdata.data);
                break;

            case CE_NET_ENDTURN:
                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Net - End turn from Socket %d",socket));
                net_recv_endturn(socket);
                break;


            /* ===================== Unknown/Bad == */

            case CE_NET_BADCRC:
            case CE_NET_INVALID:
            default:
                console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
                    "Net - Unrecongised Data Request from %d : %c",socket,data[0]));
                net_invalid(socket);
                break;
        }
    }
}

/*
#######################
#..net_check_server().#
#######################
*/
void net_check_server (void)
{
    int i, nbytes;
    char data_in[NET_MAX_PACKET_SIZE];
    read_fds = master;

    select(fdmax+1, &read_fds, NULL, NULL, &tv);

    for(i=0;i<=fdmax;i++) {
        if(FD_ISSET(i,&read_fds)){

            /* Listener Socket */
            if(i==listener){
                addrlen = sizeof(their_addr);
                newfd = accept(listener,(struct sockaddr*)&their_addr,&addrlen);
                FD_SET(newfd, &master);

                if (newfd > fdmax)
                    fdmax = newfd;

                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                    log_message,"Net - New Connection from %s on port %d",
                    inet_ntoa(their_addr.sin_addr),newfd
                ));

            /* Existing Sockets */
            } else {

                memset(data_in,'\0',sizeof(data_in));

                /* Incoming */
                if ((nbytes = recv(i, data_in, sizeof(data_in), 0)) <= 0) {

                    // No Dataz? Drop client
                    if (nbytes == 0) {
                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                            sprintf(log_message,"Net - Socket %d Disconnected",i));
                        net_player_disconnected(i);
                    }
                    close(i);
                    FD_CLR(i, &master);

                }

                /* Deal with it */
                if(nbytes > 0)
                    net_handle_server_data(i,data_in);

            }
        }
        memset(data_in, '\0', sizeof(data_in));
    }
}

/*
################################
#..net_getnextfreeplayerslot().#
################################
*/
int net_getnextfreeplayerslot (void)
{
    int i;

    for(i=0;i<myworld.players;i++)
        if(myplayers[i].socket == 0)
            return i;

    /* Server Full */
    return myworld.players+1;
}

/*
##############################
#..net_player_disconnected().#
##############################
*/
void net_player_disconnected (int socket)
{
    int pid;
    char histtext[255];

    if((pid = net_sockettopid(socket)) == -1)
        return;

    console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
        "Net - Player %d Disconnected from Socket %d",pid,socket));

    memset(histtext,'\0',sizeof(histtext));
    sprintf(histtext,"%s has left the server!", myplayers[pid].name);
    history_add(histtext);

    myplayers[pid].id = 0;
    myplayers[pid].socket = 0;
    myplayers[pid].ping = 0;
    memset(myplayers[pid].name,'\0',sizeof(myplayers[pid].name));
    myplayers[pid].ready = false;

    /* Unlock Game Loop */
    net_wait = false;
    forceupdate = true;
}

/*
======================================================================= Requests
*/

/*
####################
#..net_send_ping().#
####################
*/
void net_send_ping (int socket)
{
    char packet[NET_MAX_PACKET_SIZE],data[NET_MAX_PACKET_SIZE];

    memset(data,'\0',sizeof(data));
    memset(packet,'\0',sizeof(packet));

    sprintf(data,"%ld", (long)time(NULL));

    sprintf(packet,"%d|%d|%s|%s",CE_NET_PING,(int)strlen(data),data,net_gencrc(data));

    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
        "Net - %s (Packed Data Len %d)",packet,(int)strlen(data)));

    net_send(socket, packet);
}

/*
####################
#..net_recv_ping().#
####################
*/
void net_recv_ping (int socket, char data[256])
{
    int pid = net_sockettopid(socket);
    long diff;
    int newping = 3;

    diff = (long)time(NULL) - atol(data);

    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
        "Net - Got Ping Back %s from Socket %d (Packed Data Len %d)",
        data,socket,(int)strlen(data))
    );

    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
        "Net - Ping diff %ld from Socket %d",diff,socket));

    if(diff > 3)
        newping = 1;
    else if(diff > 1)
        newping = 2;

    /* Unlock Game Loop? */
    if(newping != myplayers[pid].ping){
        myplayers[pid].ping = newping;
        net_wait = false;
        forceupdate = true;
    }
}

/*
####################
#..net_gamestate().#
####################
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
void net_gamestate (int socket)
{
    char packet[NET_MAX_PACKET_SIZE],data[NET_MAX_PACKET_SIZE];

    memset(data,'\0',sizeof(data));
    memset(packet,'\0',sizeof(packet));

    sprintf(data,"%d~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d",
        myworld.mode,
        myworld.submode,
        myworld.total_spells,
        myworld.players,
        myworld.current_player,
        myworld.selected_item[0],
        myworld.selected_item[1],
        myworld.arenasize,
        myworld.balance,
        myworld.cursor[0],
        myworld.cursor[1]
    );

    sprintf(packet,"%d|%d|%s|%s",CE_NET_GAMESTATE,(int)strlen(data),data,net_gencrc(data));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - GameState Data : %s (Packed Data Len %d)",packet,(int)strlen(data)));

    net_send(socket, packet);
}

/*
######################
#..net_arenalayout().#
######################
*/
void net_arenalayout (int socket)
{
    char packet[NET_MAX_PACKET_SIZE],data[NET_MAX_PACKET_SIZE];
    int i,j,l;

    memset(data,'\0',sizeof(data));
    memset(packet,'\0',sizeof(packet));

    for(i=0;i<arenas[myworld.arenasize][0];i++)
        for(j=0;j<arenas[myworld.arenasize][1];j++)
            for(l=0;l<MAX_LAYERS;l++)
                sprintf(data,"%s%d~",data,myworld.layout[i][j][l]);

    /* Unset the last ~ */
    data[(int)strlen(data)-1] = '\0';

    sprintf(packet,"%d|%d|%s|%s",CE_NET_ARENALAYOUT,(int)strlen(data),data,net_gencrc(data));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Arena Layout Data : %s (Packed Data Len %d)",packet,(int)strlen(data)));

    net_send(socket, packet);
}

/*
######################
#..net_playerstate().#
######################
Values :
    id~name~readystate~color
*/
void net_playerstate (int socket)
{
    char packet[NET_MAX_PACKET_SIZE],data[NET_MAX_PACKET_SIZE];
    int i;

    memset(data,'\0',sizeof(data));
    memset(packet,'\0',sizeof(packet));

    for(i=0;i<myworld.players;i++){
        if(i==0){
            sprintf(data,"%d~%s~%d~%d~%d",
                myplayers[i].id,
                myplayers[i].name,
                myplayers[i].ready,
                myplayers[i].color,
                myplayers[i].ping
            );
        }else{
            sprintf(data,"%s,%d~%s~%d~%d~%d",
                data,
                myplayers[i].id,
                myplayers[i].name,
                myplayers[i].ready,
                myplayers[i].color,
                myplayers[i].ping
            );
        }
    }

    sprintf(packet,"%d|%d|%s|%s",CE_NET_PLAYERSTATE,(int)strlen(data),data,net_gencrc(data));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Player List Data : %s (Packed Data Len %d)",packet,(int)strlen(data)));

    net_send(socket, packet);
}

/*
####################
#..net_spelllist().#
####################
Values :
    id~name~readystate
*/
void net_spelllist (int socket)
{
    char packet[NET_MAX_PACKET_SIZE],data[NET_MAX_PACKET_SIZE];
    int i;
    bool usesep = FALSE;

    memset(data,'\0',sizeof(data));
    memset(packet,'\0',sizeof(packet));

    for(i=0;i<MAX_SPELLS;i++){

        /* Only send populated fields */
        if(myspells[i].spell_type == SPELL_NULL)
            continue;

        if(!usesep) {
            sprintf(data,"%d~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d",
                i,
                myspells[i].id,
                myspells[i].player_id,
                myspells[i].spell_type,
                myspells[i].current_pos[0],
                myspells[i].current_pos[1],
                myspells[i].current_pos[2],
                myspells[i].current_defense,
                myspells[i].illusion,
                myspells[i].beencast,
                myspells[i].dead,
                myspells[i].undead,
                myspells[i].beenmoved,
                myspells[i].genspells,
                myspells[i].turnlimit,
                myspells[i].uses,
                myspells[i].last_killed,
                myspells[i].skipround
            );
            usesep = TRUE;
        } else {
            sprintf(data,"%s,%d~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d",
                data,
                i,
                myspells[i].id,
                myspells[i].player_id,
                myspells[i].spell_type,
                myspells[i].current_pos[0],
                myspells[i].current_pos[1],
                myspells[i].current_pos[2],
                myspells[i].current_defense,
                myspells[i].illusion,
                myspells[i].beencast,
                myspells[i].dead,
                myspells[i].undead,
                myspells[i].beenmoved,
                myspells[i].genspells,
                myspells[i].turnlimit,
                myspells[i].uses,
                myspells[i].last_killed,
                myspells[i].skipround
            );
        }
    }

    sprintf(packet,"%d|%d|%s|%s",CE_NET_SPELLLIST,(int)strlen(data),data,net_gencrc(data));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Spell List Data : %s (Packed Data Len %d) to Socket %d",
        packet,(int)strlen(data),socket));

    net_send(socket, packet);
}

/*
####################
#..net_spelldata().#
####################
Values :
    spelltype~item1~item2~etc
*/
void net_spelldata (int socket, int spelltype)
{
    char packet[NET_MAX_PACKET_SIZE],data[NET_MAX_PACKET_SIZE];
    char spells[255],upgrades[255];
    int i,j;
    bool usesep = FALSE;

    if(spelltype < 0 || spelltype > MAX_SPELL_TYPES){
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Invalid Spell Type Request : %d",spelltype));
        net_invalid(socket);
        return;
    }

    memset(data,'\0',sizeof(data));
    memset(packet,'\0',sizeof(packet));

    /* First int is the spelltype */
    sprintf(data,"%d",spelltype);

    switch(spelltype){

        case SPELL_PLAYER:
            for(i=0;i<MAX_PLAYERS;i++){

                /* Skip blanks */
                if((int)strlen(myplayers[i].name) < 1)
                    continue;

                /* Spells id string */
                memset(spells,'\0',sizeof(spells));
                usesep = false;
                for(j=1;j<MAX_PLAYER_SPELLS;j++){
                    if(myplayers[i].spells[j] == 0)
                        continue;
                    if(usesep){
                        sprintf(spells,"%s:%d",spells,myplayers[i].spells[j]);
                    }else{
                        sprintf(spells,"%d",myplayers[i].spells[j]);
                        usesep = true;
                    }
                }

                /* Upgrades id string */
                memset(upgrades,'\0',sizeof(upgrades));
                usesep = false;
                for(j=0;j<MAX_MAGIC_UPGRADE;j++){
                    if(myplayers[i].upgrades[j] == 0)
                        continue;
                    if(usesep){
                        sprintf(upgrades,"%s:%d",upgrades,myplayers[i].upgrades[j]);
                    }else{
                        sprintf(upgrades,"%d",myplayers[i].upgrades[j]);
                        usesep = true;
                    }
                }

                /* Player Data String */
                sprintf(data,"%s,%d~%d~%d~%d~%s~%s~%d~%d~%s~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d~%s",
                    data,
                    i,
                    myplayers[i].id,
                    myplayers[i].socket,
                    myplayers[i].ready,
                    myplayers[i].name,
                    myplayers[i].disp,
                    myplayers[i].color,
                    myplayers[i].ping,
                    spells,
                    myplayers[i].total_spells,
                    myplayers[i].selected_spell,
                    myplayers[i].defense,
                    myplayers[i].attack,
                    myplayers[i].ranged_damage,
                    myplayers[i].ranged_range,
                    myplayers[i].dex,
                    myplayers[i].move_range,
                    myplayers[i].magic_resistance,
                    myplayers[i].flight,
                    myplayers[i].attack_undead,
                    upgrades
                );
            }
            break;

        case SPELL_MONSTER:
            for(i=0;i<MAX_MONSTERS;i++){

                /* Skip blanks */
                if((int)strlen(mymonsters[i].name) < 1)
                    continue;

                /* Monster Data String */
                sprintf(data,"%s,%d~%d~%s~%s~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d",
                    data,
                    i,
                    mymonsters[i].id,
                    mymonsters[i].name,
                    mymonsters[i].disp,
                    mymonsters[i].color,
                    mymonsters[i].attack,
                    mymonsters[i].ranged_damage,
                    mymonsters[i].ranged_range,
                    mymonsters[i].defense,
                    mymonsters[i].move_range,
                    mymonsters[i].dex,
                    mymonsters[i].magic_resistance,
                    mymonsters[i].casting_prob,
                    mymonsters[i].flight,
                    mymonsters[i].mount,
                    mymonsters[i].undead,
                    mymonsters[i].balance
                );
            }
            break;

        case SPELL_MAGIC_RANGED:
            for(i=0;i<MAX_MAGIC_RANGED;i++){

                /* Skip blanks */
                if((int)strlen(mymagic_ranged[i].name) < 1)
                    continue;

                /* Magic_Upgrade Data String */
                sprintf(data,"%s,%d~%d~%s~%s~%d~%s~%d~%d~%d~%d~%d~%d~%d",
                    data,
                    i,
                    mymagic_ranged[i].id,
                    mymagic_ranged[i].name,
                    mymagic_ranged[i].disp,
                    mymagic_ranged[i].color,
                    mymagic_ranged[i].description,
                    mymagic_ranged[i].assoc_func,
                    mymagic_ranged[i].uses,
                    mymagic_ranged[i].ranged_damage,
                    mymagic_ranged[i].ranged_range,
                    mymagic_ranged[i].beam,
                    mymagic_ranged[i].casting_prob,
                    mymagic_ranged[i].balance
                );
            }
            break;

        case SPELL_TREE:
            for(i=0;i<MAX_TREES;i++){

                /* Skip blanks */
                if((int)strlen(mytrees[i].name) < 1)
                    continue;

                /* Tree Data String */
                sprintf(data,"%s,%d~%d~%s~%s~%d~%s~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d",
                    data,
                    i,
                    mytrees[i].id,
                    mytrees[i].name,
                    mytrees[i].disp,
                    mytrees[i].color,
                    mytrees[i].description,
                    mytrees[i].attack,
                    mytrees[i].defense,
                    mytrees[i].casting_prob,
                    mytrees[i].casting_range,
                    mytrees[i].assoc_func,
                    mytrees[i].mount,
                    mytrees[i].autospawn,
                    mytrees[i].genspells,
                    mytrees[i].turnlimit,
                    mytrees[i].uses,
                    mytrees[i].balance
                );
            }
            break;

        case SPELL_MAGIC_SPECIAL:
            for(i=0;i<MAX_MAGIC_SPECIAL;i++){

                /* Skip blanks */
                if((int)strlen(mymagic_special[i].name) < 1)
                    continue;

                /* Magic Special Data String */
                sprintf(data,"%s,%d~%d~%s~%s~%d~%d~%d",
                    data,
                    i,
                    mymagic_special[i].id,
                    mymagic_special[i].name,
                    mymagic_special[i].description,
                    mymagic_special[i].assoc_func,
                    mymagic_special[i].casting_range,
                    mymagic_special[i].casting_prob
                );
            }
            break;

        case SPELL_MAGIC_UPGRADE:
            for(i=0;i<MAX_MAGIC_UPGRADE;i++){

                /* Skip blanks */
                if((int)strlen(mymagic_upgrade[i].name) < 1)
                    continue;

                /* Magic Upgrade Data String */
                sprintf(data,"%s,%d~%d~%s~%s~%d~%s~%d~%d~%d~%d~%d~%d~%d~%d~%d",
                    data,
                    i,
                    mymagic_upgrade[i].id,
                    mymagic_upgrade[i].name,
                    mymagic_upgrade[i].disp,
                    mymagic_upgrade[i].color,
                    mymagic_upgrade[i].description,
                    mymagic_upgrade[i].attack,
                    mymagic_upgrade[i].ranged_damage,
                    mymagic_upgrade[i].ranged_range,
                    mymagic_upgrade[i].defense,
                    mymagic_upgrade[i].move_range,
                    mymagic_upgrade[i].flight,
                    mymagic_upgrade[i].attack_undead,
                    mymagic_upgrade[i].balance,
                    mymagic_upgrade[i].casting_prob
                );
            }
            break;

        case SPELL_MAGIC_ATTRIB:
            for(i=0;i<MAX_MAGIC_SPELL_ATTRIB;i++){

                /* Skip blanks */
                if((int)strlen(mymagic_spell_attrib[i].name) < 1)
                    continue;

                /* Magic Attrib Data String */
                sprintf(data,"%s,%d~%d~%s~%s~%d~%d~%d~%d~%d~%d~%d",
                    data,
                    i,
                    mymagic_spell_attrib[i].id,
                    mymagic_spell_attrib[i].name,
                    mymagic_spell_attrib[i].description,
                    mymagic_spell_attrib[i].assoc_func,
                    mymagic_spell_attrib[i].change_alive,
                    mymagic_spell_attrib[i].change_owner,
                    mymagic_spell_attrib[i].turn_undead,
                    mymagic_spell_attrib[i].casting_prob,
                    mymagic_spell_attrib[i].casting_range,
                    mymagic_spell_attrib[i].balance
                );
            }
            break;

        case SPELL_MAGIC_BALANCE:
            for(i=0;i<MAX_MAGIC_BALANCE;i++){

                /* Skip blanks */
                if((int)strlen(mymagic_balance[i].name) < 1)
                    continue;

                /* Magic Balance Data String */
                sprintf(data,"%s,%d~%d~%s~%s~%d~%d",
                    data,
                    i,
                    mymagic_balance[i].id,
                    mymagic_balance[i].name,
                    mymagic_balance[i].description,
                    mymagic_balance[i].casting_prob,
                    mymagic_balance[i].balance
                );
            }
            break;

        case SPELL_WALL:
            for(i=0;i<MAX_WALLS;i++){

                /* Skip blanks */
                if((int)strlen(mywalls[i].name) < 1)
                    continue;

                /* Wall Data String */
                sprintf(data,"%s,%d~%d~%s~%s~%d~%s~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d",
                    data,
                    i,
                    mywalls[i].id,
                    mywalls[i].name,
                    mywalls[i].disp,
                    mywalls[i].color,
                    mywalls[i].description,
                    mywalls[i].attack,
                    mywalls[i].defense,
                    mywalls[i].casting_prob,
                    mywalls[i].casting_range,
                    mywalls[i].assoc_func,
                    mywalls[i].mount,
                    mywalls[i].autospawn,
                    mywalls[i].genspells,
                    mywalls[i].turnlimit,
                    mywalls[i].uses,
                    mywalls[i].balance
                );
            }
            break;

        case SPELL_BLOB:
            for(i=0;i<MAX_BLOBS;i++){

                /* Skip blanks */
                if((int)strlen(myblobs[i].name) < 1)
                    continue;

                /* Blob Data String */
                sprintf(data,"%s,%d~%d~%s~%s~%d~%s~%d~%d~%d~%d~%d~%d~%d~%d~%d~%d",
                    data,
                    i,
                    myblobs[i].id,
                    myblobs[i].name,
                    myblobs[i].disp,
                    myblobs[i].color,
                    myblobs[i].description,
                    myblobs[i].attack,
                    myblobs[i].defense,
                    myblobs[i].casting_prob,
                    myblobs[i].casting_range,
                    myblobs[i].assoc_func,
                    myblobs[i].autospawn,
                    myblobs[i].devourer,
                    myblobs[i].turnlimit,
                    myblobs[i].uses,
                    myblobs[i].balance
                );
            }
            break;

        /* Unknown Spell Type / Bad Request */
        default:
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Net - Invalid Spell Type Request : %d",spelltype));
            net_invalid(socket);
            return;
    }

    sprintf(packet,"%d|%d|%s|%s",CE_NET_SPELLDATA,(int)strlen(data),data,net_gencrc(data));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Spell Data Data : %s (Packed Data Len %d)",packet,(int)strlen(data)));

    net_send(socket, packet);
}

/*
#####################
#..net_historylog().#
#####################
Values :
    id~timedate~log
*/
void net_historylog (int socket)
{
    char packet[NET_MAX_PACKET_SIZE],data[NET_MAX_PACKET_SIZE];
    int i;

    memset(data,'\0',sizeof(data));
    memset(packet,'\0',sizeof(packet));

    for(i=0;i<MAX_HISTORY;i++){
        if(i==0)
            sprintf(data,"%d~%s~%s",i,myhistory[i].datetime,myhistory[i].message);
        else
            sprintf(data,"%s,%d~%s~%s",data,i,myhistory[i].datetime,myhistory[i].message);
    }

    sprintf(packet,"%d|%d|%s|%s",CE_NET_HISTORYLOG,(int)strlen(data),data,net_gencrc(data));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - History Log Data : %s (Packed Data Len %d)",packet,(int)strlen(packet)));

    net_send(socket, packet);
}

/*
##################
#..net_chatlog().#
##################
Values :
    id~timedate~player_id~msg
*/
void net_chatlog (int socket)
{
    char packet[NET_MAX_PACKET_SIZE],data[NET_MAX_PACKET_SIZE];
    int i;

    memset(data,'\0',sizeof(data));
    memset(packet,'\0',sizeof(packet));

    for(i=0;i<MAX_CHAT;i++){
        if(i==0)
            sprintf(data,"%d~%s~%d~%s",i,mychat[i].datetime,
                mychat[i].player_id,mychat[i].message);
        else
            sprintf(data,"%s,%d~%s~%d~%s",data,i,mychat[i].datetime,
                mychat[i].player_id,mychat[i].message);
    }

    sprintf(packet,"%d|%d|%s|%s",CE_NET_CHATLOG,(int)strlen(data),data,net_gencrc(data));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Chat Log Data : %s (Packed Data Len %d)",packet,(int)strlen(packet)));

    net_send(socket, packet);
}

/*
################
#..net_ready().#
################
*/
void net_ready (int socket)
{
    char packet[NET_MAX_PACKET_SIZE],data[NET_MAX_PACKET_SIZE];
    int pid = net_sockettopid(socket);

    /* Player doesn't exist yet */
    if(pid < 0){
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Couldnt find player id for socket %d",socket));
        net_invalid(socket);
        return;
    }

    memset(data,'\0',sizeof(data));
    memset(packet,'\0',sizeof(packet));

    if(myplayers[pid].ready){
        myplayers[pid].ready = false;
        sprintf(data,"You are marked Not Ready");
    } else {
        myplayers[pid].ready = true;
        sprintf(data,"You are marked Ready");
    }

    sprintf(packet,"%d|%d|%s|%s",CE_NET_READY,(int)strlen(data),data,net_gencrc(data));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - %s (Packed Data Len %d)",packet,(int)strlen(packet)));

    net_send(socket, packet);

    /* Unlock Game Loop */
    net_wait = false;
    forceupdate = true;
}

/*
####################
#..net_newplayer().#
####################
Incoming format :
    name~color
Return format :
    0 - result (-2=malformed / -1=wrong mode / 0=server full / 1=success)
    1 - player id
*/
void net_newplayer (int socket, char msg[256])
{
    char packet[NET_MAX_PACKET_SIZE],data[NET_MAX_PACKET_SIZE];
    char *d = msg;
    char *newdata = NULL;
    char name[255];
    char histtext[255];
    int pid = net_getnextfreeplayerslot();
    int color = 0;
    int i = 0;

    memset(data,'\0',sizeof(data));
    memset(packet,'\0',sizeof(packet));
    memset(histtext,'\0',sizeof(histtext));

    newdata = strtok(d,"~");
    while(newdata != NULL){
        switch(i){
            case 0:
                memset(name,'\0',sizeof(name));
                sprintf(name,"%s",newdata);
                break;
            case 1:
                color = atoi(newdata);
                if(color<0||color>MAX_PLAYER_COLORS)
                    color = 0;
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
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Bad field count : %d fields (expected 2)",i+1));
        goto MALFORMED;
    }

    if(myworld.mode == CE_WORLD_MODE_SETUP && (myworld.submode == 1 || myworld.submode == 3)){

        /* Server full? */
        if(pid > myworld.players){
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Net - Server is too full for socket : %d",socket));

            sprintf(data,"0~-1");

        /* Valid Name? */
        } else if((int)strlen(name) < 33 && valid_input(name)){

            memset(myplayers[pid].name,'\0',sizeof(myplayers[pid].name));
            myplayers[pid].id = pid;
            myplayers[pid].socket = socket;
            sprintf(myplayers[pid].name,"%s",name);
            myplayers[pid].color = (color*3)+1;
            myplayers[pid].ready = false;
            myworld.current_player = net_getnextfreeplayerslot();

            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Net - New Player [%d] on Socket : %d",pid,socket));

            sprintf(data,"1~%d",pid);

            sprintf(histtext,"%s has joined the server!", myplayers[pid].name);
            history_add(histtext);

            /* send chat log to client too */
            net_chatlog(socket);

            /* Unlock Game Loop */
            net_wait = false;
            forceupdate = true;

        } else {
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Net - Invalid New Name : %s (Packed Data Len %d)",name,(int)strlen(name)));

            MALFORMED:
            sprintf(data,"-2~-1");
        }

    /* Wrong Mode */
    } else {
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "Net - New Player during non-setup mode from socket : %d",socket));
        sprintf(data,"-1~-1");
    }

    sprintf(packet,"%d|%d|%s|%s",CE_NET_NEWPLAYER,(int)strlen(data),data,net_gencrc(data));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - New Player Result : %s (Packed Data Len %d)",packet,(int)strlen(packet)));

    net_send(socket, packet);
}

/*
##################
#..net_chatmsg().#
##################
*/
void net_chatmsg (int socket, char msg[256])
{
    int pid = net_sockettopid(socket);

    if(pid < 0){
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Chat MSG from Invalid Player : %s (Packed Data Len %d)",msg,(int)strlen(msg)));
        net_invalid(socket);
        return;
    }

    if((int)strlen(msg) > MAX_CHATMSG_LENGTH){
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Chat MSG too big : %s (Packed Data Len %d)",msg,(int)strlen(msg)));
        net_invalid(socket);
        return;
    }

    /* Yeah, we'll accept that */
    chat_add(pid,msg);

    /* Unlock Game Loop */
    net_wait = false;
    forceupdate = true;
}

/*
##################
#..net_beepmsg().#
##################
Values :
    msg~beep
*/
void net_beepmsg (int socket)
{
    char packet[NET_MAX_PACKET_SIZE],data[NET_MAX_PACKET_SIZE];

    memset(data,'\0',sizeof(data));
    memset(packet,'\0',sizeof(packet));

    sprintf(data,"%s~%d",infobar_text,beepmsg);

    sprintf(packet,"%d|%d|%s|%s",CE_NET_BEEPMSG,(int)strlen(data),data,net_gencrc(data));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Beep Msg Data : %s (Packed Data Len %d)",packet,(int)strlen(data)));

    net_send(socket, packet);
    /* Unlock Game Loop */
    net_wait = false;
    forceupdate = true;
}

/*
##############################
#..net_recv_spellselection().#
##############################
Values :
    spellid,illusion
*/
void net_recv_spellselection (int socket, char spelldata[256])
{
    char packet[NET_MAX_PACKET_SIZE],data[NET_MAX_PACKET_SIZE];
    char *d = spelldata;
    char *newdata = NULL;
    int i=0;
    int spellid;
    bool illusion = false;
    int pid = net_sockettopid(socket);

    /* Player doesn't exist yet */
    if(pid < 0){
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Couldnt find player id for socket %d",socket));
        net_invalid(socket);
        return;
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Processing selected spell: %s (From %d)",spelldata,pid));

    newdata = strtok(d,"~");
    while(newdata != NULL){
        switch(i){
            case 0:
                spellid = atoi(newdata);
                break;
            case 1:
                if(atoi(newdata)==1)
                    illusion = true;
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
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Bad field count : %d fields (expected 2)",i+1));

        /* Tell Client to try again */
        spellid = -99;
    }

    /* Invalid Spell Selection? */
    if(spellid < -1 || spellid > (myplayers[pid].total_spells+1) ||
        myspells[myplayers[pid].spells[spellid]].beencast){

        memset(data,'\0',sizeof(data));
        memset(packet,'\0',sizeof(packet));
        sprintf(data,"-1");
        sprintf(packet,"%d|%d|%s|%s",CE_NET_SELECTEDSPELL,(int)strlen(data),data,net_gencrc(data));

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Invalid Spell Selection : %s (Packed Data Len %d)",packet,(int)strlen(packet)));

        myplayers[pid].ready = false;

        net_send(socket, packet);
        return;
    }

    /* -1 Spellid is valid (no spell selected) */
    if(spellid>0){
        myplayers[pid].selected_spell = spellid;
        if(myspells[myplayers[pid].spells[spellid]].spell_type == SPELL_MONSTER)
            myspells[myplayers[pid].spells[spellid]].illusion = illusion;
    }
    myplayers[pid].ready = true;

    /* Unlock Network */
    net_wait = false;
    forceupdate = true;
}

/*
####################
#..net_recv_move().#
####################
*/
void net_recv_move (int socket, char spelldata[256])
{
    char packet[NET_MAX_PACKET_SIZE],data[NET_MAX_PACKET_SIZE];
    char *d = spelldata;
    char *newdata = NULL;
    int i=0;
    int cur_x,cur_y,to_x,to_y;
    int retflag = CE_NET_ACTION_FAIL_INVALID;
    int pid = net_sockettopid(socket);

    /* Player doesn't exist yet */
    if(pid < 0){
        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
            "Net - Couldnt find player id for socket %d",socket));
        net_invalid(socket);
        return;
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Processing Move : %s",spelldata));

    newdata = strtok(d,"~");
    while(newdata != NULL){
        switch(i){
            case 0:
                cur_x = atoi(newdata);
                break;
            case 1:
                cur_y = atoi(newdata);
                break;
            case 2:
                to_x = atoi(newdata);
                break;
            case 3:
                to_y = atoi(newdata);
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
        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
            "Net - Bad field count : %d fields (expected 4)",i+1));

    } else { /* XXX Make sure its the right players turn? */
        if(myworld.mode==CE_WORLD_MODE_MOVE){
            myworld.cursor[0] = cur_x;
            myworld.cursor[1] = cur_y;
            moveitem(to_x,to_y);
            retflag=CE_NET_ACTION_OK;

            /* Tell Client that its engaged to fight */
            if(myworld.selected_item[0] > 0 && checkadjacent() && myworld.submode != 1)
                retflag=CE_NET_ACTION_ENGAGED;
        }
    }

    /* Send back the result */
    memset(data,'\0',sizeof(data));
    memset(packet,'\0',sizeof(packet));
    sprintf(data,"%d",retflag);
    sprintf(packet,"%d|%d|%s|%s",CE_NET_ACTION,(int)strlen(data),data,net_gencrc(data));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Move Return : %s (Packed Data Len %d)",packet,(int)strlen(packet)));

    net_send(socket, packet);

    /* Tell connected players whats happened */
    net_arenalayout(CE_NET_SOCKET_ALL);
    net_gamestate(CE_NET_SOCKET_ALL);
    net_playerstate(CE_NET_SOCKET_ALL);
    net_spelllist(CE_NET_SOCKET_ALL);
    net_beepmsg(CE_NET_SOCKET_ALL);

    /* Unlock Network */
    net_wait = false;
    forceupdate = true;
}

/*
#####################
#..net_recv_mount().#
#####################
Values :
    mount(1)/dismount(2),yes/no
*/
void net_recv_mount (int socket, char spelldata[256])
{
    char *d = spelldata;
    char *newdata = NULL;
    int i=0;
    int dir,action;
    int pid = net_sockettopid(socket);

    /* Player doesn't exist yet */
    if(pid < 0){
        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
            "Net - Couldnt find player id for socket %d",socket));
        net_invalid(socket);
        return;
    }

    console_log(__FILE__,__func__,__LINE__,LOG_ALL, sprintf(log_message,
        "Net - Processing (Dis)Mount - %s",spelldata));

    newdata = strtok(d,"~");
    while(newdata != NULL){
        switch(i){
            case 0:
                dir = atoi(newdata);
                break;
            case 1:
                action = atoi(newdata);
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

    /* Mount! */
    if(dir==1 && action==1){
        mount_item(myworld.cursor[0],myworld.cursor[1]);
        myworld.selected_item[0] = 0;
        myworld.selected_item[1] = 0;
        myworld.submode = 0;

    /* Don't Mount! */
    } else if(dir==1 && action==0){
        myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
        myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];
        myworld.submode = 0;

    /* Dismount! */
    } else if(dir==2 && action==1){
    myworld.submode = 4;

    /* Don't Dismount! */
    } else if(dir==2 && action==0){
        myworld.submode = 5;

    /* lol wut? */
    } else {
        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
            "Net - Unknown Mount Command : dir %d action %d",dir,action));
    }

    net_spelllist(CE_NET_SOCKET_ALL);
    net_spelldata(CE_NET_SOCKET_ALL,SPELL_PLAYER);
    net_arenalayout(CE_NET_SOCKET_ALL);
    net_gamestate(CE_NET_SOCKET_ALL);
    net_playerstate(CE_NET_SOCKET_ALL);
    net_beepmsg(CE_NET_SOCKET_ALL);

    /* Unlock Network */
    net_wait = false;
    forceupdate = true;
}

/*
######################
#..net_recv_action().#
######################
*/
void net_recv_action (int socket, char spelldata[256])
{
    char packet[NET_MAX_PACKET_SIZE],data[NET_MAX_PACKET_SIZE];
    char *d = spelldata;
    char *newdata = NULL;
    int i=0;
    int action,cur_x,cur_y;
    int retflag = CE_NET_ACTION_FAIL_INVALID;
    int pid = net_sockettopid(socket);
    int top_layer,cursoritem;
    int move_range, attack_range, rangedistance;

    /* Player doesn't exist yet */
    if(pid < 0){
        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
            "Net - Couldnt find player id for socket %d",socket));
        net_invalid(socket);
        return;
    }

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Processing Action"));

    newdata = strtok(d,"~");
    while(newdata != NULL){
        switch(i){
            case 0:
                action = atoi(newdata);
                break;
            case 1:
                cur_x = atoi(newdata);
                break;
            case 2:
                cur_y = atoi(newdata);
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
        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
            "Net - Bad field count : %d fields (expected 3)",i+1));

    } else { /* XXX Make sure its the right players turn? */

        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Action value : %d",action));

        /* Do action! */
        switch(action){
            case CE_NET_ACTION_CAST:
                if(myworld.mode==CE_WORLD_MODE_CASTING){

                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                        "Net - Attempting to Cast Spell!"));

                    retflag = CE_NET_ACTION_OK;

                    myworld.cursor[0]=cur_x;
                    myworld.cursor[1]=cur_y;

                    top_layer = itemstack_top(myworld.cursor[0],myworld.cursor[1]);
                    cursoritem = myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer];

                    switch(myspells[myplayers[pid].spells[myplayers[pid].selected_spell]].spell_type) {

                        case SPELL_BLOB:
                            if((myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] == 0 ||
                                myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].dead) &&
                                (myblobs[myspells[myplayers[pid].spells[myplayers[pid].selected_spell]].id].casting_range == 0 ||
                                checkdistance() <= myblobs[myspells[myplayers[pid].spells[myplayers[pid].selected_spell]].id].casting_range)
                                && checklos()){

                                    cast_spell();

                                if(myspells[myplayers[pid].spells[myplayers[pid].selected_spell]].beencast){
                                    myworld.current_player++;
                                    skipdeadplayers();
                                    myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
                                    myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];
                                }

                            } else {
                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                    log_message,
                                    "trying to cast too far from player[%dx%d] => cursor[%dx%d] | range is %d or Blocked",
                                    myspells[myworld.current_player+9].current_pos[0],
                                    myspells[myworld.current_player+9].current_pos[1],
                                    myworld.cursor[0],myworld.cursor[1],
                                    myblobs[myspells[myplayers[pid].spells[myplayers[pid].selected_spell]].id].casting_range)
                                );
                                retflag = CE_NET_ACTION_FAIL_DIST;
                            }
                            break;

                        case SPELL_WALL:
                            if((myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] == 0 ||
                                myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].dead) &&
                                (mywalls[myspells[myplayers[pid].spells[myplayers[pid].selected_spell]].id].casting_range == 0 ||
                                checkdistance() <= mywalls[myspells[myplayers[pid].spells[myplayers[pid].selected_spell]].id].casting_range)
                                && checklos()){

                                cast_spell();

                                if(myspells[myplayers[pid].spells[myplayers[pid].selected_spell]].beencast){
                                    myworld.current_player++;
                                    skipdeadplayers();
                                    myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
                                    myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];
                                }

                            } else {
                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                    log_message,
                                    "trying to cast too far from player[%dx%d] => cursor[%dx%d] | range is %d or Blocked",
                                    myspells[myworld.current_player+9].current_pos[0],
                                    myspells[myworld.current_player+9].current_pos[1],
                                    myworld.cursor[0],myworld.cursor[1],
                                    mywalls[myspells[myplayers[pid].spells[myplayers[pid].selected_spell]].id].casting_range)
                                );
                                retflag = CE_NET_ACTION_FAIL_DIST;
                            }
                            break;

                        case SPELL_MAGIC_BALANCE:
                            cast_spell();

                            myworld.current_player++;
                            skipdeadplayers();
                            myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
                            myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];

                            break;

                        case SPELL_MAGIC_ATTRIB:
                            if(mymagic_spell_attrib[myspells[myplayers[pid].spells[myplayers[pid].selected_spell]].id].casting_range == 0 ||
                                checkdistance() <= mymagic_spell_attrib[myspells[myplayers[pid].spells[myplayers[pid].selected_spell]].id].casting_range){

                                if(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] > 0){

                                    cast_spell();

                                    /* Spell was 'cast' */
                                    if(myspells[myplayers[pid].spells[myplayers[pid].selected_spell]].beencast){
                                        myworld.current_player++;
                                        myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
                                        myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];
                                    }

                                } else {
                                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                        sprintf(log_message,
                                        "trying to cast Spell Atrrib Spell on empty pos at %dx%d layer %d",
                                        myworld.cursor[0],myworld.cursor[1],top_layer)
                                    );
                                    retflag = CE_NET_ACTION_FAIL_INVALID;
                                }
                            } else {
                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                    log_message,
                                    "trying to cast too far from player[%dx%d] => cursor[%dx%d]",
                                    myspells[myworld.current_player+9].current_pos[0],
                                    myspells[myworld.current_player+9].current_pos[1],
                                    myworld.cursor[0],myworld.cursor[1])
                                );
                                retflag = CE_NET_ACTION_FAIL_DIST;
                            }
                            break;

                        case SPELL_MAGIC_UPGRADE:
                            if(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] == myworld.current_player+9 ||
                                (top_layer > 0 &&
                                myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer-1] == myworld.current_player+9)
                            ){

                                cast_spell();
                                myworld.current_player++;
                                skipdeadplayers();
                                myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
                                myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];

                            } else {
                                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                        sprintf(log_message,
                                        "trying to cast upgrade on non-player owned player at %dx%d layer %d",
                                        myworld.cursor[0],myworld.cursor[1],top_layer)
                                    );
                                    retflag = CE_NET_ACTION_FAIL_INVALID;
                            }
                            break;

                        case SPELL_MAGIC_SPECIAL:
                            if(mymagic_special[myspells[myplayers[pid].spells[myplayers[pid].selected_spell]].id].casting_range == 0 ||
                                checkdistance() <= mymagic_special[myspells[myplayers[pid].spells[myplayers[pid].selected_spell]].id].casting_range){

                                cast_spell();
                                myworld.current_player++;
                                skipdeadplayers();
                                myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
                                myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];

                            } else {
                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                    log_message,
                                    "trying to cast too far from player[%dx%d] => cursor[%dx%d]",
                                    myspells[myworld.current_player+9].current_pos[0],
                                    myspells[myworld.current_player+9].current_pos[1],
                                    myworld.cursor[0],myworld.cursor[1])
                                );
                                retflag = CE_NET_ACTION_FAIL_DIST;
                            }
                            break;

                        case SPELL_TREE:
                            if(((myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] == 0 ||
                                myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].dead) &&
                                (mytrees[myspells[myplayers[pid].spells[myplayers[pid].selected_spell]].id].casting_range == 0 ||
                                checkdistance() <= mytrees[myspells[myplayers[pid].spells[myplayers[pid].selected_spell]].id].casting_range)
                                && checklos() && !checkadjacent_any(myworld.cursor[0] - 1,myworld.cursor[1] - 1)) ||
                                mytrees[myspells[myplayers[pid].spells[myplayers[pid].selected_spell]].id].assoc_func == 1){

                                cast_spell();

                                if(myspells[myplayers[pid].spells[myplayers[pid].selected_spell]].beencast){
                                    myworld.current_player++;
                                    skipdeadplayers();
                                    myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
                                    myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];
                                }

                            } else {
                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(
                                    log_message,
                                    "trying to cast too far from player[%dx%d] => cursor[%dx%d] | range is %d or Blocked",
                                    myspells[myworld.current_player+9].current_pos[0],
                                    myspells[myworld.current_player+9].current_pos[1],
                                    myworld.cursor[0],myworld.cursor[1],
                                    mytrees[myspells[myplayers[pid].spells[myplayers[pid].selected_spell]].id].casting_range)
                                );
                                retflag = CE_NET_ACTION_FAIL_DIST;
                            }
                            break;

                        case SPELL_MAGIC_RANGED:
                            if(mymagic_ranged[myspells[myplayers[pid].spells[myplayers[pid].selected_spell]].id].ranged_range == 0 ||
                                (checkdistance() <= mymagic_ranged[myspells[myplayers[pid].spells[myplayers[pid].selected_spell]].id].ranged_range &&
                                checklos())){

                                if(myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] > 0 &&
                                    !myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].dead){

                                    if(myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].player_id != myplayers[myworld.current_player-1].id){
                                        cast_spell();

                                        if(myspells[myplayers[pid].spells[myplayers[pid].selected_spell]].beencast){
                                            myworld.current_player++;
                                            skipdeadplayers();
                                            myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
                                            myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];
                                        }

                                    } else {
                                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                            sprintf(log_message,
                                            "Trying to attack friendly at %dx%d layer %d",
                                            myworld.cursor[0],myworld.cursor[1],top_layer)
                                        );
                                        retflag = CE_NET_ACTION_FAIL_INVALID;
                                    }
                                } else {
                                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                        sprintf(log_message,
                                        "Nothing to attack at %dx%d layer %d",
                                        myworld.cursor[0],myworld.cursor[1],top_layer)
                                    );
                                    retflag = CE_NET_ACTION_FAIL_INVALID;
                                }
                            } else {
                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                    sprintf(log_message,
                                    "trying to cast too far from player[%dx%d] => cursor[%dx%d]",
                                    myspells[pid+10].current_pos[0],
                                    myspells[pid+10].current_pos[1],
                                    myworld.cursor[0],myworld.cursor[1])
                                );
                                retflag = CE_NET_ACTION_FAIL_DIST;
                            }
                            break;

                        case SPELL_MONSTER:
                        default:

                            if((myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer] == 0 ||
                                myspells[myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer]].dead) &&
                                (myworld.cursor[0] < myspells[pid+10].current_pos[0]+2 &&
                                myworld.cursor[0] > myspells[pid+10].current_pos[0]-2) &&
                                (myworld.cursor[1] < myspells[pid+10].current_pos[1]+2 &&
                                myworld.cursor[1] > myspells[pid+10].current_pos[1]-2)) {

                                cast_spell();

                                myworld.current_player++;
                                skipdeadplayers();
                                myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
                                myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];

                            } else {
                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                    sprintf(log_message,
                                    "trying to spawn too far from player[%dx%d] or ontop of another item => cursor[%dx%d]",
                                    myspells[pid].current_pos[0],
                                    myspells[pid].current_pos[1],
                                    myworld.cursor[0],myworld.cursor[1])
                                );
                                retflag = CE_NET_ACTION_FAIL_DIST;
                            }
                            break;
                    }
                }
                break;

            case CE_NET_ACTION_SELECT:

                retflag = CE_NET_ACTION_OK;

                myworld.cursor[0]=cur_x;
                myworld.cursor[1]=cur_y;

                top_layer = itemstack_top(myworld.cursor[0],myworld.cursor[1]);
                cursoritem = myworld.layout[myworld.cursor[0]][myworld.cursor[1]][top_layer];

                if((cursoritem > 0 && !myspells[cursoritem].dead) ||
                    myworld.selected_item[0] > 0) {
                    /*
                    ################
                    #..Select.item.#
                    ################
                    */
                    if(myworld.selected_item[0] == 0) {
                        /*
                            This.. is proper ugly =F
                            If...
                                There is an item there
                                Its yours
                                its not dead
                                If its a tree, is it worth selecting?
                                Is not a Blob?
                                (mount+is mounted / has attacking ability)
                        */
                        if(cursoritem > 0 &&
                            myspells[cursoritem].player_id == myplayers[myworld.current_player-1].id &&
                            !myspells[cursoritem].beenmoved &&
                            myspells[cursoritem].spell_type != SPELL_BLOB &&
                            (myspells[cursoritem].spell_type != SPELL_TREE ||
                                (myspells[cursoritem].spell_type == SPELL_TREE &&
                                mytrees[myspells[cursoritem].id].attack > 0) ||
                                is_mounted(myworld.cursor[0],myworld.cursor[1])
                            )
                        ){
                            /* Is item mounted? */
                            if(myworld.submode == 0 && is_mounted(myworld.cursor[0],myworld.cursor[1])){

                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                    sprintf(log_message,
                                    "Ask mounted player if they want to dismount")
                                );

                                myworld.submode = 3;

                            }

                            myspells[cursoritem].beenmoved = true;
                            myworld.selected_item[0] = cursoritem;

                            console_log(__FILE__,__func__,__LINE__,LOG_DEBUG,
                                sprintf(
                                log_message,"Item thinks it is at %dx%dx%d",
                                myspells[myworld.selected_item[0]].current_pos[0],
                                myspells[myworld.selected_item[0]].current_pos[1],
                                myspells[myworld.selected_item[0]].current_pos[2])
                            );

                            switch(myspells[cursoritem].spell_type){
                                case SPELL_PLAYER:
                                    myworld.selected_item[1] =
                                        myplayers[myspells[myworld.selected_item[0]].id].move_range;
                                    break;

                                case SPELL_TREE:
                                case SPELL_WALL:
                                case SPELL_BLOB:
                                    myworld.selected_item[1] = 0;
                                    break;

                                case SPELL_MONSTER:
                                default:
                                    myworld.selected_item[1] =
                                        mymonsters[myspells[myworld.selected_item[0]].id].move_range;
                                    break;
                            }
                        if(can_fly(myspells[myworld.selected_item[0]].current_pos[0],
                                    myspells[myworld.selected_item[0]].current_pos[1])){

                                console_log(__FILE__,__func__,__LINE__,
                                    LOG_DEBUG, sprintf(log_message,
                                    "Item can fly %d place(s) this turn",
                                    myworld.selected_item[1])
                                );

                            } else {
                                console_log(__FILE__,__func__,__LINE__,
                                    LOG_DEBUG, sprintf(log_message,
                                    "Item can move %d place(s) this turn",
                                    myworld.selected_item[1])
                                );
                            }
                        } else {
                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                sprintf(log_message,
                                "Item is out of moves or doesnt belong to you (beenmoved : %d)",
                                myspells[cursoritem].beenmoved)
                            );
                            retflag = CE_NET_ACTION_FAIL_OOT;
                        }

                        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                            sprintf(log_message,
                            "Item (id[%d]type[%d]) on layer %d belongs to %d [you are (%d)]",
                            cursoritem, myspells[cursoritem].spell_type, top_layer,
                            myspells[cursoritem].player_id,
                            myplayers[myworld.current_player-1].id)
                        );

                    } else {
                        if(!checkadjacent() && (myworld.submode == 0 || myworld.submode == 5)){

                            if(myspells[myworld.selected_item[0]].spell_type == SPELL_PLAYER){

                                move_range =
                                    myplayers[myspells[myworld.selected_item[0]].id].move_range;

                                attack_range =
                                    myplayers[myspells[myworld.selected_item[0]].id].ranged_range;

                            } else if(myspells[myworld.selected_item[0]].spell_type == SPELL_TREE){

                                move_range = 0;
                                attack_range = 0;

                            } else {
                                move_range =
                                    mymonsters[myspells[myworld.selected_item[0]].id].move_range;

                                attack_range =
                                    mymonsters[myspells[myworld.selected_item[0]].id].ranged_range;
                            }

                            /*
                            #####################
                            #..Flying.Creatures.#
                            #####################
                            */
                            if(can_fly(myspells[myworld.selected_item[0]].current_pos[0],
                                    myspells[myworld.selected_item[0]].current_pos[1])){

                                rangedistance = checkdistance();

                                if(rangedistance > move_range){

                                    console_log(__FILE__,__func__,__LINE__,
                                        LOG_DEBUG, sprintf(log_message,
                                        "Trying to fly too far (cursor distance : %d / creature range %d)",
                                        rangedistance,
                                        mymonsters[myspells[myworld.selected_item[0]].id].move_range)
                                    );

                                    retflag = CE_NET_ACTION_FAIL_DIST;
                                } else if (rangedistance < 1) {

                                    console_log(__FILE__,__func__,__LINE__,
                                        LOG_DEBUG, sprintf(log_message,
                                        "Flying source and dest are the same. Ending turn.")
                                    );

                                    if(attack_range > 0){
                                        myworld.submode = 1;
                                    } else {
                                        myworld.selected_item[0] = 0;
                                        myworld.selected_item[1] = 0;
                                    }
                                } else {
                                    moveitem(
                                        myspells[myworld.selected_item[0]].current_pos[0],
                                        myspells[myworld.selected_item[0]].current_pos[1]
                                    );
                                }
                            /*
                            ######################
                            #..Walking.Creatures.#
                            ######################
                            */
                            } else {
                                if(attack_range > 0){
                                    myworld.submode = 1;
                                } else {
                                    myworld.selected_item[0] = 0;
                                    myworld.selected_item[1] = 0;
                                }
                                console_log(__FILE__,__func__,__LINE__,
                                    LOG_DEBUG, sprintf(log_message,
                                    "Source and dest are the same. Ending turn.")
                                );
                            }

                        /*
                        ##################
                        #..Ranged.Attack.#
                        ##################
                        */
                        } else if (myworld.submode == 1){

                            if(myspells[cursoritem].player_id != myplayers[myworld.current_player-1].id){
                                rangedistance = checkdistance();

                                if(myspells[myworld.selected_item[0]].spell_type == SPELL_PLAYER){
                                    attack_range =
                                        myplayers[myspells[myworld.selected_item[0]].id].ranged_range;
                                } else {
                                    attack_range =
                                        mymonsters[myspells[myworld.selected_item[0]].id].ranged_range;
                                }

                                if(rangedistance > attack_range){
                                    console_log(__FILE__,__func__,__LINE__,
                                        LOG_NOTICE, sprintf(log_message,
                                        "Trying to attack too far (cursor distance : %d / creature range %d)",
                                        rangedistance,mymonsters[myspells[myworld.selected_item[0]].id].ranged_range)
                                    );
                                    retflag = CE_NET_ACTION_FAIL_DIST;
                                } else if (rangedistance < 1) {
                                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                        sprintf(log_message,
                                        "Attack range source and dest are the same.")
                                    );
                                    retflag = CE_NET_ACTION_FAIL_INVALID;
                                } else {
                                    if(checklos()){
                                        if(cursoritem > 0 && !myspells[cursoritem].dead) {
                                            if(creature_attack(myspells[myworld.selected_item[0]].current_pos[0],
                                                myspells[myworld.selected_item[0]].current_pos[1])){

                                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                                    sprintf(log_message,"Successful attack"));
                                                retflag = CE_NET_ACTION_ATTACK_SUCCESS;
                                            } else {
                                                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                                    sprintf(log_message,"Unsuccessful attack"));
                                                retflag = CE_NET_ACTION_ATTACK_FAILED;
                                            }
                                        } else {
                                            console_log(__FILE__,__func__,__LINE__,
                                                LOG_NOTICE, sprintf(log_message,
                                                "Attacking nothing / wasted shot!")
                                            );
                                        }
                                        myworld.submode = 0;
                                        myworld.selected_item[0] = 0;
                                        myworld.selected_item[1] = 0;
                                    } else {
                                        console_log(__FILE__,__func__,__LINE__,
                                            LOG_NOTICE, sprintf(log_message,
                                            "LOS failed, something blocking shot!")
                                        );
                                        retflag = CE_NET_ACTION_FAIL_LOS;
                                    }
                                }
                            } else if (cursoritem == 0){

                                console_log(__FILE__,__func__,__LINE__,
                                    LOG_NOTICE, sprintf(log_message,
                                    "Noting to attack, wasting shot!")
                                );
                                myworld.submode = 0;
                                myworld.selected_item[0] = 0;
                                myworld.selected_item[1] = 0;
                            } else {

                                console_log(__FILE__,__func__,__LINE__,
                                    LOG_NOTICE, sprintf(log_message,
                                    "Trying to attack your own spell! %d vs %d",
                                    myspells[cursoritem].player_id,
                                    myplayers[myworld.current_player-1].id)
                                );
                                retflag = CE_NET_ACTION_FAIL_ATTACKSELF;
                            }

                        } else {
                            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                                sprintf(log_message,"Engaged to creature!"));
                            retflag = CE_NET_ACTION_ENGAGED;
                        }
                    }
                } else {

                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                        sprintf(log_message,
                        "Nothing selected and nothing to select on current position [%dx%d]",
                        myworld.cursor[0],myworld.cursor[1])
                    );
                    retflag = CE_NET_ACTION_FAIL_INVALID;
                }
                break;

            case CE_NET_ACTION_DESELECT:

                retflag = CE_NET_ACTION_OK;

                /* End spell's turn but keep player in control */
                myspells[myworld.selected_item[0]].beenmoved = true;

                /* Have a ranged attack turn? */
                if(myspells[myworld.selected_item[0]].spell_type == SPELL_PLAYER){
                    attack_range =
                        myplayers[myspells[myworld.selected_item[0]].id].ranged_range;
                } else if(myspells[myworld.selected_item[0]].spell_type == SPELL_TREE){
                    attack_range = 0;
                } else {
                    attack_range =
                        mymonsters[myspells[myworld.selected_item[0]].id].ranged_range;
                }

                /* Go to range attack mode */
                if(myworld.submode == 0 && attack_range > 0)
                    myworld.submode++;
                else
                    myworld.selected_item[0] = 0;
                break;
        }
    }

    /* Send back the result */
    memset(data,'\0',sizeof(data));
    memset(packet,'\0',sizeof(packet));
    sprintf(data,"%d",retflag);
    sprintf(packet,"%d|%d|%s|%s",CE_NET_ACTION,(int)strlen(data),data,net_gencrc(data));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Action Return : %s (Packed Data Len %d)",packet,(int)strlen(packet)));

    net_send(socket, packet);

    net_spelllist(CE_NET_SOCKET_ALL);
    net_spelldata(CE_NET_SOCKET_ALL,SPELL_PLAYER);
    net_arenalayout(CE_NET_SOCKET_ALL);
    net_gamestate(CE_NET_SOCKET_ALL);
    net_playerstate(CE_NET_SOCKET_ALL);
    net_beepmsg(CE_NET_SOCKET_ALL);

    /* Unlock Network */
    net_wait = false;
    forceupdate = true;
}

/*
#######################
#..net_recv_endturn().#
#######################
*/
void net_recv_endturn (int socket)
{
    char packet[NET_MAX_PACKET_SIZE],data[NET_MAX_PACKET_SIZE];
    int pid = net_sockettopid(socket);
    int s = 0;

    memset(data,'\0',sizeof(data));
    memset(packet,'\0',sizeof(packet));

    if(pid == (myworld.current_player-1)){
        switch(myworld.mode){
            case CE_WORLD_MODE_CASTING:
                s = myplayers[pid].spells[myplayers[pid].selected_spell];
                myspells[s].beencast = true;
                myplayers[pid].selected_spell = 0;
                sprintf(data,"Turn Ended");
                break;

            case CE_WORLD_MODE_MOVE:
                myworld.selected_item[0]=0;
                myworld.selected_item[1]=0;
                myworld.submode = 0;

                myworld.current_player++;

                myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
                myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];

                sprintf(data,"Turn Ended");
                if(myworld.current_player > myworld.players)
                    sprintf(data,"Round Ended");
                break;
        }
    } else {
        sprintf(data,"Not your turn! Your PID : %d (Current Player : %d)",
            pid,(myworld.current_player-1));
    }

    sprintf(packet,"%d|%d|%s|%s",CE_NET_ENDTURN,(int)strlen(data),data,net_gencrc(data));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - %s (Packed Data Len %d)",packet,(int)strlen(packet)));

    net_send(socket, packet);

    skipdeadplayers();

    /* Unlock Game Loop */
    net_wait = false;
    forceupdate = true;
}
