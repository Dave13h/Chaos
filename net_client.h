/***************************************************************
*  net_client.h                                    #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  Network Client headers                          #.....#     *
*                                                  #######     *
*                                                              *
***************************************************************/

void init_net_client(void);
void net_check_client(void);
bool net_connecttoserver(char address[15]);

//============================================ Requests
void net_joingame(char name[256]);
void net_send_chatmsg(char msg[MAX_CHATMSG_LENGTH]);
void net_send_ready(void);
void net_send_selectedspell(int spid,bool illusion);
void net_send_move(int cur_x, int cur_y, int to_x, int to_y);
void net_send_mount(int dir, int action);
void net_send_action(int action,int cur_x, int cur_y);
void net_send_endturn(void);
void net_client_color(void);

//============================================= Returns
void net_return_ping(char ping[256]);
void net_recv_gamestate(char data[NET_MAX_PACKET_SIZE]);
void net_recv_arenalayout(char data[NET_MAX_PACKET_SIZE]);
void net_recv_playerstate(char data[NET_MAX_PACKET_SIZE]);
void net_recv_newplayer(char data[NET_MAX_PACKET_SIZE]);
void net_recv_spelllist(char data[NET_MAX_PACKET_SIZE]);

void net_recv_spelldata(char data[NET_MAX_PACKET_SIZE]);
void net_recv_data_player(char data[MAX_SPELLS][NET_MAX_SERIAL_CHUNK],int total_entries);
void net_recv_data_monster(char data[MAX_SPELLS][NET_MAX_SERIAL_CHUNK],int total_entries);
void net_recv_data_magic_ranged(char data[MAX_SPELLS][NET_MAX_SERIAL_CHUNK],int total_entries);
void net_recv_data_tree(char data[MAX_SPELLS][NET_MAX_SERIAL_CHUNK],int total_entries);
void net_recv_data_magic_special(char data[MAX_SPELLS][NET_MAX_SERIAL_CHUNK],int total_entries);
void net_recv_data_magic_upgrade(char data[MAX_SPELLS][NET_MAX_SERIAL_CHUNK],int total_entries);
void net_recv_data_magic_attrib(char data[MAX_SPELLS][NET_MAX_SERIAL_CHUNK],int total_entries);
void net_recv_data_magic_balance(char data[MAX_SPELLS][NET_MAX_SERIAL_CHUNK],int total_entries);
void net_recv_data_wall(char data[MAX_SPELLS][NET_MAX_SERIAL_CHUNK],int total_entries);
void net_recv_data_blob(char data[MAX_SPELLS][NET_MAX_SERIAL_CHUNK],int total_entries);

void net_recv_historylog(char data[NET_MAX_PACKET_SIZE]);
void net_recv_chatlog(char data[NET_MAX_PACKET_SIZE]);
void net_recv_selectedspell(char data[NET_MAX_PACKET_SIZE]);
void net_recv_actionreturn(char data[NET_MAX_PACKET_SIZE]);
void net_recv_beepmsg(char data[NET_MAX_PACKET_SIZE]);
