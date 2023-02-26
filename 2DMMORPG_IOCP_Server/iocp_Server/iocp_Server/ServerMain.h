#pragma once
#include "globals.h"
#include "SocketInfo.h"
#include "PacketSend.h"
#include "DatabaseManager.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

class ServerMain
{
public:
	ServerMain();
	~ServerMain();
public:
	
	void initServer();
	void initNPC();
	void loadMap();
	void initSocket();
	void initThread();

	void do_accept_thread();
	void do_mainWorker_thread();
	void do_timer_thread();

	void addEvent(EVENT &event);

	void processPacket(int id, void* buff);
	void process_player_move(int id, void* buff);
	void process_npc_move(int npc_id);
	void process_npc_chase(int npc_id, int target_id);
	void process_heal_event(int id);
	void process_npc_respawn_event(int id);
	void process_player_respawn_event(int id);
	void process_npc_meet_event(int npc_id, int player_id);

	bool is_NPC(int id);
	bool is_active(int id);
	bool is_near_npc(int my_id, int other_id);
	bool is_near(int my_id, int other_id);
	bool in_aggro_range(int npc_id, int player);

	void sock_errDisplay(const char *msg, int err_code);

private:
	PacketSend *m_PacketSend = NULL;
	DatabaseManager *m_DBManager = NULL;

	bool use_DB;
	bool servertest;

	HANDLE m_iocp_handle;
	int m_new_client_id;
	int DBCount;
	SOCKET m_accept_socket;
	mutex eventQueue_lock;

	string filepath = "../../map1.txt"; // ¸ÊÆÄÀÏ°æ·Î
	int WorldMap_Arr[MAP_SCALE_Y][MAP_SCALE_X];	// ¿ùµå¸Ê ¹è¿­ ÇÑÄ­¿¡ 40ÇÈ¼¿

	SocketInfo* clients[MAX_OBJECT];
	priority_queue <EVENT> event_queue;

	//vector<DATABASE> vec_database;
};

