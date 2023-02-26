#include "PacketSend.h"

PacketSend::PacketSend()
{
}

PacketSend::~PacketSend()
{
}

void PacketSend::send_packet(int client_id, SOCKET socket, void * buf)
{
	char* packet = reinterpret_cast<char*>(buf);
	int packet_size = packet[0];
	OVER_EX *send_over = new OVER_EX;
	ZeroMemory(send_over, sizeof(OVER_EX));
	send_over->event_type = EV_SEND;
	memcpy(send_over->buffer, packet, packet_size);
	send_over->wsabuf[0].buf = send_over->buffer;
	send_over->wsabuf[0].len = packet_size;

	int ret_code = WSASend(socket, send_over->wsabuf, 1, 0, 0, &send_over->over, 0);
	if (0 != ret_code) {
		int err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no)
			errDisplay("WSASend Error :", err_no);
	}
}

void PacketSend::send_id(int client_id, SOCKET socket)
{
	sc_packet_send_id packet;
	packet.id = client_id;
	packet.size = sizeof(packet);
	packet.type = SC_SEND_ID;
	send_packet(client_id, socket, &packet);
}

void PacketSend::send_remove_player(int client_id, SOCKET socket, int leaver)
{
	sc_packet_remove_player packet;
	packet.id = leaver;
	packet.size = sizeof(packet);
	packet.type = SC_REMOVE_PLAYER;
	send_packet(client_id, socket, &packet);
}

void PacketSend::send_login_fail(int client_id, SOCKET socket)
{
	sc_packet_login_fail packet;
	packet.type = SC_LOGIN_FAIL;
	packet.size = sizeof(packet);
	send_packet(client_id, socket, &packet);
}

void PacketSend::send_login_success(int client_id, SOCKET socket)
{
	sc_packet_login_ok packet;
	packet.id = client_id;
	packet.size = sizeof(packet);
	packet.type = SC_LOGIN_OK;
	send_packet(client_id, socket, &packet);
}

void PacketSend::send_put_player(int client_id, SOCKET socket, int new_id, short x, short y, short hp, short att, short exp, short level, char obj_type)
{
	sc_packet_put_player packet;
	packet.id = new_id;
	packet.size = sizeof(packet);
	packet.type = SC_PUT_PLAYER;
	packet.x = x;
	packet.y = y;
	packet.hp = hp;
	packet.att = att;
	packet.exp = exp;
	packet.level = level;
	packet.obj_type = obj_type;

	send_packet(client_id, socket, &packet);
}

void PacketSend::send_battle_mess(int hitter_id, SOCKET hit_socket, int damaged_id, SOCKET dam_socket, short damage)
{
	sc_packet_battle_mess packet;
	packet.type = SC_BATTLE_MESS;
	packet.size = sizeof(packet);
	packet.hitterID = hitter_id;
	packet.damagedID = damaged_id;
	packet.damage = damage;

	if (hitter_id < NPC_ID_START) { // 공격자가 플레이어일경우
		send_packet(hitter_id, hit_socket, &packet);
	}
	else {
		send_packet(damaged_id, dam_socket, &packet);
	}
}

void PacketSend::send_stat_change(int client_id, SOCKET socket, short hp, short exp, short level)
{
	sc_packet_stat_change packet;
	packet.type = SC_STAT_CHANGE;
	packet.size = sizeof(packet);
	packet.hp = hp;
	packet.exp = exp;
	packet.level = level;
	send_packet(client_id, socket, &packet);
}

void PacketSend::send_chat(int chatter, int receiver_id, SOCKET receiver_socket, char name[], char mess[])
{
	sc_packet_chat packet;
	packet.id = chatter;
	packet.size = sizeof(packet);
	packet.type = SC_CHAT;
	strcpy_s(packet.name, name);
	strcpy_s(packet.chat, mess);

	send_packet(receiver_id, receiver_socket, &packet);
}

void PacketSend::send_pos(int client_id, SOCKET socket, int move_id, short move_x, short move_y)
{
	sc_packet_pos packet;
	packet.id = move_id;
	packet.size = sizeof(packet);
	packet.type = SC_POS;
	packet.x = move_x;
	packet.y = move_y;

	send_packet(client_id, socket, &packet);
}

void PacketSend::send_player_dead(int client_id, SOCKET socket)
{
	sc_packet_player_dead packet;
	packet.type = SC_PLAYER_DEAD;
	packet.size = sizeof(packet);
	send_packet(client_id, socket, &packet);
}

void PacketSend::send_npc_chat(int chatter, int receiver_id, SOCKET receiver_socket, char mess[])
{
	sc_packet_chat packet;
	packet.id = chatter;
	packet.size = sizeof(packet);
	packet.type = SC_CHAT;
	strcpy_s(packet.chat, mess);
	send_packet(receiver_id, receiver_socket, &packet);
}

void PacketSend::errDisplay(const char * msg, int err_code)
{
	WCHAR *lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::cout << msg;
	std::wcout << L"에러 " << lpMsgBuf << std::endl;
	while (true);		// 이부분에서 일부러 멈추게 하기위해서
	LocalFree(lpMsgBuf);
}
