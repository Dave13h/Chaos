/***************************************************************
*  net.h                                           #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  Network headers                                 #.....#     *
*                                                  #######     *
*                                                              *
***************************************************************/

enum CE_NET_REQUEST {
    CE_NET_BADCRC = -2,
    CE_NET_INVALID,
    CE_NET_PING,

    /* Info */
    CE_NET_GAMESTATE,
    CE_NET_ARENALAYOUT,
    CE_NET_PLAYERSTATE,
    CE_NET_SPELLLIST,
    CE_NET_SPELLDATA,
    CE_NET_HISTORYLOG,
    CE_NET_CHATLOG,
    CE_NET_BEEPMSG,

    /* Commands */
    CE_NET_READY,
    CE_NET_NEWPLAYER,
    CE_NET_CHATMSG,
    CE_NET_SELECTEDSPELL,
    CE_NET_MOVE,
    CE_NET_ACTION,
    CE_NET_MOUNT,
    CE_NET_ENDTURN
};

enum CE_NET_ACTION {
    CE_NET_ACTION_FAIL_LOS = -4,
    CE_NET_ACTION_FAIL_DIST,
    CE_NET_ACTION_FAIL_ATTACKSELF,
    CE_NET_ACTION_FAIL_OWNER,
    CE_NET_ACTION_FAIL_OOT,
    CE_NET_ACTION_FAIL_INVALID,
    CE_NET_ACTION_OK,
    CE_NET_ACTION_ATTACK_SUCCESS,
    CE_NET_ACTION_ATTACK_FAILED,
    CE_NET_ACTION_ENGAGED,
    CE_NET_ACTION_CAST,
    CE_NET_ACTION_SELECT,
    CE_NET_ACTION_DESELECT
};

enum CE_NET_SPECIAL {
    CE_NET_SOCKET_ALL = -1
};

typedef struct {
    short int type;
    short int len;
    char data[NET_MAX_PACKET_SIZE];
    char crc[256];
} ce_net_request;

typedef struct {
    int len;
    char data[MAX_SPELLS][NET_MAX_SERIAL_CHUNK];
} ce_net_serial_data;

//=========================================== Internals
int net_sockettopid(int socket);
char* net_gencrc(char data[256]);
bool valid_ipaddress (char address[255]);
void net_send(int socket, char data[NET_MAX_PACKET_SIZE]);
void net_send_all(char data[NET_MAX_PACKET_SIZE]);
ce_net_request net_get_request(char data[NET_MAX_PACKET_SIZE]);
ce_net_serial_data net_unserialise(char datastr[], char sep[2]);

//============================================= Returns
void net_invalid(int socket);

//========================================== Game Loops
void gameloop_net_server(void);
void gameloop_net_client(void);
