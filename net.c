/***************************************************************
*  net.c                                           #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  Network routines                                #.....#     *
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
#include "generate.h"
#include "input_common.h"
#include "chat.h"
#include "net.h"
#include "net_client.h"
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

extern int mypid;

bool net_enable = false;
bool net_dedicated = false;
bool net_connected = false;
bool net_wait = false;
int net_port = NET_DEFAULT_PORT;

extern int fdmax, listener;
extern int server_socket;

/*
========================================================== Internal Functions ==
*/

/*
######################
#..net_sockettopid().#
######################
*/
int net_sockettopid (int socket)
{
    int i;

    for(i=0;i<myworld.players;i++)
        if(myplayers[i].socket == socket)
            return myplayers[i].id;

    return -1;
}

/*
#################
#..net_gencrc().#
#################
*/
char* net_gencrc (char data[NET_MAX_PACKET_SIZE])
{

    /* CRC should include a Version Number
        so client is in sync with server
        for enum'd data types?  */

    return "CRCLOLZ";
}

/*
###################
#..net_checkcrc().#
###################
*/
static bool net_checkcrc (char data[NET_MAX_PACKET_SIZE])
{
    return true;
}

/*
######################
#..valid_ipaddress().#
######################
*/
bool valid_ipaddress (char address[255])
{
    char validchars[11] = "1234567890";
    int i,j;
    int sepcount=0;
    bool valid;

    if(strlen(address) < 7 || strlen(address) > 15){
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Address too short/long [%d]",(int)strlen(address)));
        return false;
    }

    for(i=0;i<(int)strlen(address);i++){
        valid = false;
        for(j=0;j<(int)strlen(validchars);j++){
            if(address[i] == '.'){
                valid = true;
                sepcount++;
                break;
            }

            if(address[i] == validchars[j]){
                valid = true;
                break;
            }
        }
        if(!valid){
            console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                "invalid char [%c]",address[i]));
            return false;
        }
    }

    if(sepcount !=3){
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "invalid amount of seperators [%d]",sepcount));
        return false;
    }
    return true;
}

/*
###############
#..net_send().#
###############
*/
void net_send (int socket, char data[NET_MAX_PACKET_SIZE])
{
    char senddata[NET_MAX_PACKET_SIZE+1];
    int ret,left;
    char *data_sent;

    /* Send All 'socket'? */
    if(socket == CE_NET_SOCKET_ALL){
        net_send_all(data);
        return;
    }

    if((int)strlen(data)>NET_MAX_PACKET_SIZE){
        console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
            "Net - Trying to send too much? : Len %d",(int)strlen(data)));
        return;
    }

    memset(senddata,'\0',sizeof(senddata));
    sprintf(senddata,"%s%s",data,NET_END_PACKET_SEP);

    data_sent = (char *)malloc((int)strlen(senddata)+1);
    if(data_sent == NULL){
        console_log(__FILE__,__func__,__LINE__,LOG_ERROR, sprintf(log_message,
            "Net - Failed to Alloc memory for net_send()"));
        shutdown_chaos();
        return;
    }

    left = (int)strlen(senddata);

    strcpy(data_sent,senddata);

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Sending to %d : %s (Len %d)",socket,senddata,(int)strlen(senddata)));

    while(left > 0){
        if((ret = send(socket, data_sent, left, 0)) == -1){
            console_log(__FILE__,__func__,__LINE__,LOG_WARNING, sprintf(log_message,
                "Net - Send Error : %d (Left to Send: %d)",ret,left));
            return;
        }


        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Sent %d of %d",ret,left));

        left -= ret;
        data_sent += ret;
    }
}

/*
###################
#..net_send_all().#
###################
*/
void net_send_all (char data[NET_MAX_PACKET_SIZE])
{
    int i;

    for(i=0;i<myworld.players;i++)
        if(myplayers[i].socket > 0)
            net_send(myplayers[i].socket,data);
}

/*
######################
#..net_get_request().#
######################
*/
ce_net_request net_get_request (char data[NET_MAX_PACKET_SIZE])
{
    ce_net_request request;
    char *field = NULL;
    int i = 0;

    /* Defaults */
    request.type = CE_NET_INVALID;
    request.len = 0;
    memset(request.data,'\0',sizeof(request.data));
    memset(request.crc,'\0',sizeof(request.crc));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - Split Packet Data : %s (Len %d)",data,(int)strlen(data)));

    field = strtok(data,NET_PACKET_SEP);
    while(field != NULL){
        switch(i){
            case 0:
                request.type = atoi(field);
                break;
            case 1:
                request.len = atoi(field);
                if(request.len > (strlen(data) - 4)){
                    request.type = CE_NET_INVALID;
                    goto BADPACKET;
                }
                break;
            case 2:
                if(strlen(field)!=request.len){
                    request.type = CE_NET_INVALID;
                    goto BADPACKET;
                }
                memset(request.data, '\0', sizeof(request.data));
                sprintf(request.data,"%s",field);
                break;
            case 3:
                strcpy(request.crc,field);
                break;
        }
        i++;
        field = strtok(NULL,NET_PACKET_SEP);
    }

    if(!net_checkcrc(request.crc))
        request.type = CE_NET_BADCRC;

    /* Dirty Data Escape */
    BADPACKET:

    return request;
}

/*
######################
#..net_unserialise().#
######################
    WARNING : NEVER call this from within a strtok() loop.
*/
ce_net_serial_data net_unserialise (char datastr[NET_MAX_PACKET_SIZE], char sep[2])
{
    char *data = datastr;
    char *newdata = NULL;
    int i=0;
    ce_net_serial_data returndata;

    console_log(__FILE__,__func__,__LINE__,LOG_DEBUG, sprintf(log_message,
        "Net - Serialised Data : %s",datastr));

    newdata = strtok(data,sep);
    while(newdata != NULL){
        console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
            "Net - Seperated Data : %s",newdata));

        /* Data too long to handle? bin it all. */
        if((int)strlen(newdata) > NET_MAX_SERIAL_CHUNK){
            returndata.len = 0;
            return returndata;
        }

        memset(returndata.data[i],'\0',sizeof(returndata.data[i]));
        sprintf(returndata.data[i],"%s",newdata);
        i++;
        newdata = strtok(NULL,sep);
    }

    returndata.len = i;
    return returndata;
}

/*
============================================================ Return Functions ==
*/

/*
##################
#..net_invalid().#
##################
*/
void net_invalid (int socket)
{
    char packet[NET_MAX_PACKET_SIZE],data[NET_MAX_PACKET_SIZE];

    memset(data,'\0',sizeof(data));
    memset(packet,'\0',sizeof(packet));

    sprintf(data,"Invalid Request");

    sprintf(packet,"%d|%d|%s|%s",CE_NET_INVALID,(int)strlen(data),data,net_gencrc(data));

    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
        "Net - %s (Packed Data Len %d)",packet,(int)strlen(data)));

    net_send(socket, packet);
}

/*
================================================================== Game Loops ==
*/

/*
##########################
#..gameloop_net_server().#
##########################
*/
void gameloop_net_server(void)
{
    switch(myworld.mode){
        /* Setup Mode */
        case CE_WORLD_MODE_SETUP:
            switch(myworld.submode){
                /* Setup Players */
                case 1:
                case 3:
                    if(allplayersready()){
                        generate_players();
                        empty_buffer();
                        myworld.submode=2;
                        net_gamestate(CE_NET_SOCKET_ALL);
                        net_playerstate(CE_NET_SOCKET_ALL);
                        net_wait = false;
                        setplayersready(false);
                    } else {
                        /* Keep Connected Players Informed */
                        if(!net_wait){
                            net_gamestate(CE_NET_SOCKET_ALL);
                            net_playerstate(CE_NET_SOCKET_ALL);
                            net_wait = true;
                        }
                    }
                    break;
                default:
                    break;
            }
            break;
        /* Spell Selection */
        case CE_WORLD_MODE_SELECTSPELLS:
            if(allplayersready()){
                myworld.mode = CE_WORLD_MODE_CASTING;
                myworld.submode = 0;
                myworld.current_player = 1;
                skipdeadplayers();
                myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
                myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];
                net_gamestate(CE_NET_SOCKET_ALL);
                net_spelldata(CE_NET_SOCKET_ALL,SPELL_PLAYER);
                net_wait = false;

                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,sprintf(log_message,
                    "Going to Casting Round"));
            }
            if(!net_wait)
                net_playerstate(CE_NET_SOCKET_ALL);

            net_wait = true;
            break;

        /* Casting Round */
        case CE_WORLD_MODE_CASTING:
            /* Skip players with no spells to cast */
            if(myplayers[myworld.current_player-1].selected_spell == 0){
                myworld.current_player++;
                skipdeadplayers();

                if(myworld.current_player > myworld.players) {
                    myworld.mode = CE_WORLD_MODE_MOVE;
                    myworld.submode = 0;
                    myworld.current_player = 1;
                    skipdeadplayers();
                    console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,
                        sprintf(log_message,"Going to Movement Mode"));
                }

                myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
                myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];

                /* Keep Connected Players Informed */
                if(!net_wait){
                    net_arenalayout(CE_NET_SOCKET_ALL);
                    net_gamestate(CE_NET_SOCKET_ALL);
                    net_spelllist(CE_NET_SOCKET_ALL);
                    net_playerstate(CE_NET_SOCKET_ALL);
                    net_spelldata(CE_NET_SOCKET_ALL,SPELL_PLAYER);
                    net_wait = true;
                }
            }
            break;

        /* Movement Round */
        case CE_WORLD_MODE_MOVE:
            if(myworld.current_player > myworld.players) {
                myworld.mode = CE_WORLD_MODE_POSTROUND;
                myworld.submode = 0;
                myworld.current_player = 1;
                myworld.cursor[0] = myspells[myworld.current_player+9].current_pos[0];
                myworld.cursor[1] = myspells[myworld.current_player+9].current_pos[1];

                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE,sprintf(log_message,
                    "Going to Post Round"));

                net_spelllist(CE_NET_SOCKET_ALL);
                net_gamestate(CE_NET_SOCKET_ALL);
                net_playerstate(CE_NET_SOCKET_ALL);
                net_wait = true;
            } else {
                /* Keep Connected Players Informed */
                if(!net_wait){
                    net_arenalayout(CE_NET_SOCKET_ALL);
                    net_gamestate(CE_NET_SOCKET_ALL);
                    net_playerstate(CE_NET_SOCKET_ALL);
                    net_wait = true;
                }
            }
            break;

        /* Post/Grow Round */
        case CE_WORLD_MODE_POSTROUND:
            if(!myworld.beenreset){
                reset_world();
                myworld.beenreset = true;
                net_beepmsg(CE_NET_SOCKET_ALL);

                net_arenalayout(CE_NET_SOCKET_ALL);
                net_spelllist(CE_NET_SOCKET_ALL);
                net_gamestate(CE_NET_SOCKET_ALL);
                net_playerstate(CE_NET_SOCKET_ALL);
                net_spelldata(CE_NET_SOCKET_ALL,SPELL_PLAYER);
            }

            if(!beepmsg){
                myworld.beenreset = false;
                myworld.mode = CE_WORLD_MODE_SELECTSPELLS;

                net_spelllist(CE_NET_SOCKET_ALL);
                net_gamestate(CE_NET_SOCKET_ALL);
                net_playerstate(CE_NET_SOCKET_ALL);
                net_spelldata(CE_NET_SOCKET_ALL,SPELL_PLAYER);
                net_wait = true;

                console_log(__FILE__,__func__,__LINE__,LOG_NOTICE, sprintf(log_message,
                    "Going to Spell Selection Round"));
            }
            break;

        /* Game Over */
        case CE_WORLD_MODE_ENDGAME:
            if(!net_wait){
                net_spelllist(CE_NET_SOCKET_ALL);
                net_gamestate(CE_NET_SOCKET_ALL);
                net_playerstate(CE_NET_SOCKET_ALL);
                net_wait = true;
            }
            break;

        default:
            break;
    }
}

/*
##########################
#..gameloop_net_client().#
##########################
*/
void gameloop_net_client(void)
{

    /* Force dead players to view arena mode */
    if(net_connected && isspelldead(myplayers[mypid].id+10) &&
        myworld.mode != CE_WORLD_MODE_ENDGAME){

        if(myworld.mode == CE_WORLD_MODE_SELECTSPELLS)
            sprintf(infobar_text,"Players are selecting their next spells");

        myworld.mode = CE_WORLD_MODE_SELECTSPELLS;
        if(myworld.submode != 2) /* View Spell Details */
            myworld.submode = 5;
    }

    switch(myworld.mode){
        /* Setup Mode */
        case CE_WORLD_MODE_SETUP:
            switch(myworld.submode){
                /* Server chosing arena size */
                case 2:
                    net_wait = true;
                    break;
            }
            break;

        /* Spell Selection */
        case CE_WORLD_MODE_SELECTSPELLS:
            if(myplayers[mypid].ready && !isspelldead(myplayers[mypid].id+10))
                net_wait=true;
            break;

        /* Casting Round */
        case CE_WORLD_MODE_CASTING:
            if((myworld.current_player-1) != mypid && !beepmsg)
                sprintf(infobar_text,"%s's turn",myplayers[myworld.current_player-1].name);
            break;

        /* Movement Round */
        case CE_WORLD_MODE_MOVE:
            if((myworld.current_player-1) != mypid)
                sprintf(infobar_text,"%s's turn",myplayers[myworld.current_player-1].name);
            break;

        /* Post/Grow Round */
        case CE_WORLD_MODE_POSTROUND:
            break;

        /* Game Over */
        case CE_WORLD_MODE_ENDGAME:
            forceupdate = true;
            net_wait = false;
            break;

        default:
            break;
    }
}
