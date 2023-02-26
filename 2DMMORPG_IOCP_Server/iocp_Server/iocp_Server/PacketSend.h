#pragma once
#include "globals.h"

class PacketSend
{
public:
	PacketSend();
	~PacketSend();

public:
	void send_packet(int client_id, SOCKET socket, void* buf);
	void send_id(int client_id, SOCKET socket);
	void send_remove_player(int client_id, SOCKET socket, int leaver);
	void send_login_fail(int client_id, SOCKET socket);
	void send_login_success(int client_id, SOCKET socket);
	void send_put_player(int client_id, SOCKET socket, int new_id, short x, short y, short hp, 
		short att, short exp, short level, char obj_type);
	void send_battle_mess(int hitter_id, SOCKET hit_socket, int damaged_id, SOCKET dam_socket, short damage);
	void send_stat_change(int client_id, SOCKET socket, short hp, short exp, short level);
	void send_chat(int chatter, int receiver_id, SOCKET receiver_socket, char name[], char mess[]);
	void send_pos(int client_id, SOCKET socket, int move_id, short move_x, short move_y);
	void send_player_dead(int client_id, SOCKET socket);
	void send_npc_chat(int chatter, int receiver_id, SOCKET receiver_socket, char mess[]);

	void errDisplay(const char *msg, int err_code);
};

