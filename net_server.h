/***************************************************************
*  net_server.h                                    #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  Network Server headers                          #.....#     *
*                                                  #######     *
*                                                              *
***************************************************************/

void init_net_server(void);
void net_check_server(void);
int net_getnextfreeplayerslot(void);
void net_player_disconnected(int socket);

//============================================ Requests
void net_send_ping(int socket);
void net_recv_ping(int socket, char ping[256]);
void net_gamestate(int socket);
void net_arenalayout(int socket);
void net_playerstate(int socket);
void net_spelllist(int socket);
void net_spelldata(int socket, int spelltype);
void net_historylog(int socket);
void net_chatlog(int socket);
void net_ready(int socket);
void net_newplayer(int socket, char name[256]);
void net_chatmsg(int socket, char msg[256]);
void net_beepmsg(int socket);
void net_recv_spellselection(int socket, char data[256]);
void net_recv_move(int socket, char data[256]);
void net_recv_mount(int socket, char spelldata[256]);
void net_recv_action(int socket, char data[256]);
void net_recv_endturn(int socket);
