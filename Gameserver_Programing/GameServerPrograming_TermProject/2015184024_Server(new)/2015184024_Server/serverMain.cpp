
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include <WS2tcpip.h> 
#pragma comment(lib, "Ws2_32.lib") 
#pragma comment(lib, "lua53.lib")

#include "globals.h"
#include "protocol.h"
#include "SocketInfo.h"

bool use_DB = false;
bool servertest = false;

#define start_posX	397
#define start_posY	69

default_random_engine dre;
uniform_int_distribution<> uidall(0, 800);
uniform_int_distribution<> uidx_1(310, 470);
uniform_int_distribution<> uidy_1(1, 55);
uniform_int_distribution<> uidx_2(1, 280);
uniform_int_distribution<> uidy_2(290, 348);
uniform_int_distribution<> uid_hotspot(10, 30);
priority_queue <EVENT> event_queue;
mutex timer_lock;

//array<SocketInfo*, MAX_OBJECT> clients;
//SocketInfo clients[MAX_OBJECT];

//map <int, SocketInfo*> clients;	// �̷� �����ڷḦ �а� ������ ���ؽ��� �ɾ���Ѵ�.
SocketInfo* clients[MAX_OBJECT];
HANDLE g_iocp_handle;
int new_user_id = 0;
vector<DATABASE> vec_database; // �����ͺ��̽�
int DBCount = 0;
string filepath = "../../map1.txt"; // �����ϰ��
int WorldMap_Arr[MAP_SCALE_Y][MAP_SCALE_X];	// ����� �迭 ��ĭ�� 40�ȼ�

void LoadMap();
void do_worker_thread();
void do_timer_thread();
void add_eventToQueue(EVENT &event);

void processPacket(int id, void* buf);
void process_player_move(int id, void* buf);
void process_heal_event(int id);
void process_respawn_event(int id);
void process_npc_respawn_event(int id);
void process_npc_chase(int npc_id, int target_id);
void process_npc_move(int npc_id);

bool is_NPC(int npc_id);
bool is_active(int npc_id);
bool is_near(int my_id, int other_id);
bool is_near_npc(int my_id, int npc_id);
bool in_aggro_range(int npc_id, int player);

void send_packet(int id, void* buf);
void send_id_packet(int id);
void send_login_ok_packet(int id);
void send_put_player_packet(int client, int new_id);
void send_pos_packet(int client_id, int move_id);
void send_remove_player_packet(int client, int leaver);
void send_all_chat_packet(int chatter, char name[], char mess[]);
void send_chat_packet(int client, int chatter, char mess[]);
void send_stat_change_packet(int client);
void send_player_dead_packet(int client);
void send_login_fail_packet(int client);
void send_battle_mess_packet(int hitter_id, int damaged_id, short damage);

void create_NPC();
int lua_get_npc_xpos(lua_State *L);
int lua_get_npc_ypos(lua_State *L);
int lua_send_NPCchat_packet(lua_State *L);

void error_display(const char *msg, int err_no);
void sql_show_error();
void sql_HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);
void sql_load_database();
void sql_update_database(int keyid, short x, short y, short hp, short exp, short level);
void sql_insert_database(int key, char id[], char name[], short level, short x, short y, short hp, short exp, short att);

int main()
{
	LoadMap();
	for (int i = 0; i < MAX_OBJECT; ++i) {
		clients[i] = new SocketInfo;
		clients[i]->set_isLive(false);
	}
	std::wcout.imbue(std::locale("korean")); // ������ �ѱ۷� ǥ�����ֱ�����
	create_NPC(); // npc ����
	// sql db ������
	if (true == use_DB) {
		sql_load_database();
	}

	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	::bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(SOCKADDR_IN));

	listen(listenSocket, 5);
	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(SOCKADDR_IN);
	memset(&clientAddr, 0, addrLen);
	SOCKET clientSocket;
	DWORD flags;

	g_iocp_handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);	// �ڵ鰪�� ���Ϲ޾Ƽ� icop�� ����ϴ� �ܰ�
	thread worker_thread_1{ do_worker_thread };
	thread worker_thread_2{ do_worker_thread };
	thread worker_thread_3{ do_worker_thread };
	thread worker_thread_4{ do_worker_thread };
	thread eventQueue_timerThread{ do_timer_thread };
	while (true) {
		clientSocket = accept(listenSocket, (struct sockaddr *)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET) {	// ���Ͽ�����н�
			printf("err  \n");
			break;
		}
		else {
			printf("success  \n");
		}

		int user_id = new_user_id++;
		clients[user_id]->set_id(user_id);
		clients[user_id]->set_socket(clientSocket);
		clients[user_id]->set_isDead(true);
		clients[user_id]->set_isLive(true);
		CreateIoCompletionPort(reinterpret_cast<HANDLE>(clientSocket), g_iocp_handle, user_id, 0);	// iocp�� ������ ����ϴ� �ܰ�

		send_id_packet(user_id);

		// stress test
		if (true == servertest) {
			while (true) {
				short x = uid_hotspot(dre);
				short y = uid_hotspot(dre);
				if (WorldMap_Arr[y][x] == 0) {
					clients[user_id]->set_xPos(x);
					clients[user_id]->set_yPos(y);
					break;
				}
			}
			clients[user_id]->set_hp(100);
			clients[user_id]->set_att(60);
			clients[user_id]->set_exp(0);
			clients[user_id]->set_level(1);
			clients[user_id]->set_obj_type(TYPE_PLAYER);
			clients[user_id]->set_isDead(false);
			clients[user_id]->set_isLive(true);
			EVENT ev_heal{ user_id, 0, EV_HEAL, chrono::high_resolution_clock::now() + 5s };
			add_eventToQueue(ev_heal);
			for (auto& cl : clients) {
				if (false == cl->get_isLive()) { continue; }
				if (true == cl->get_isDead()) { continue; }

				int other_player = cl->get_id();
				if (true == is_NPC(other_player)
					&& false == is_active(other_player)
					&& true == is_near_npc(user_id, other_player)) { // �÷��̾� �ֺ��� npc �����
					clients[other_player]->set_isActive(true);
					EVENT ev{ other_player, 0, EV_MOVE, chrono::high_resolution_clock::now() + 1s };
					add_eventToQueue(ev);
				}

				if (true == is_near(user_id, other_player))
				{
					if (false == is_NPC(other_player)) { // npc�� �ƴѰ�쿡
						send_put_player_packet(other_player, user_id); // �� �α��� ������ �ٸ�Ŭ���̾�Ʈ�� ����
					}

					if (other_player != user_id) { // �ٸ� Ŭ���̾�Ʈ ������ ������ ����
						send_put_player_packet(user_id, other_player);
					}
				}
			}
			clients[user_id]->set_isLive(true);
		}

		// memset
		memset(&clients[user_id]->get_over().over, 0, sizeof(clients[user_id]->get_over().over));
		flags = 0;
		int ret = WSARecv(clientSocket, clients[user_id]->get_over().wsabuf, 1, NULL,
			&flags, &(clients[user_id]->get_over().over), NULL);	// �÷��� 0���� �����ϸ� ������

		if (0 != ret)		// send recv�Ҷ� ���ϰ��� �ִ� ret ���� 0�� �ƴҰ�� �� ������ WSA_IO_PENDING �� �ƴҰ�� ����
		{
			int err_no = WSAGetLastError();
			if (WSA_IO_PENDING != err_no)
				error_display(" WSARecv Error: ", err_no);
		}

	}
	worker_thread_1.join();
	worker_thread_2.join();
	worker_thread_3.join();
	worker_thread_4.join();
	eventQueue_timerThread.join();
	closesocket(listenSocket);
	WSACleanup();
}

void LoadMap()
{
	ifstream in(filepath);
	char c;
	for (int i = 0; i < MAP_SCALE_Y; ++i) {
		for (int j = 0; j < MAP_SCALE_X; ++j) {
			in >> c;
			WorldMap_Arr[i][j] = atoi(&c);
		}
		//cout << "\n";
	}
}

void do_worker_thread()
{
	while (true) {
		DWORD recv_byte;
		ULONG key;
		PULONG p_key = &key;
		WSAOVERLAPPED* p_over;

		GetQueuedCompletionStatus(g_iocp_handle, &recv_byte, (PULONG_PTR)p_key, &p_over, INFINITE);

		OVER_EX* over_ex = reinterpret_cast<OVER_EX*>(p_over);
		if (false == clients[key]->get_isLive()) {
			continue;
		}
		SOCKET client_s = clients[key]->get_socket();

		if (recv_byte == 0) {	//������������ Ȯ��
			if (true == use_DB) {
				for (auto &db : vec_database) {
					if (db.idKey == clients[key]->get_DBkey()) {
						short x = clients[key]->get_xPos();
						short y = clients[key]->get_yPos();
						short hp = clients[key]->get_hp();
						short exp = clients[key]->get_exp();
						short level = clients[key]->get_level();
						sql_update_database(db.idKey, x, y, hp, exp, level);
						db.x = x;
						db.y = y;
						db.hp = hp;
						db.exp = exp;
						db.level = level;
					}
				}
			}
			closesocket(client_s);
			clients[key]->set_isLive(false);
			for (auto &cl : clients)
			{
				if (false == cl->get_isLive()) { continue; }
				if (false == is_NPC(cl->get_id())) {
					send_remove_player_packet(cl->get_id(), key);
				}
				clients[cl->get_id()]->delete_viewList(key);
			}

			continue;
		} // Ŭ���̾�Ʈ�� closesocket�� ���� ���

		if (EV_RECV == over_ex->event_type) {
			over_ex->buffer[recv_byte] = 0;

			// recvó��
			processPacket(key, over_ex->buffer);

			DWORD flags = 0;
			memset(&over_ex->over, 0x00, sizeof(WSAOVERLAPPED));
			WSARecv(client_s, over_ex->wsabuf, 1, 0, &flags, &over_ex->over, 0);
		}
		else if (EV_SEND == over_ex->event_type) {
			//cout << "send \n";
			delete over_ex;
		}
		else if (EV_MOVE == over_ex->event_type) {
			process_npc_move(key);
			delete over_ex;
		}
		else if (EV_MOVE_COOLTIME == over_ex->event_type) {
			clients[key]->set_moveCooltime(false);
			delete over_ex;
		}
		else if (EV_HEAL == over_ex->event_type) {
			//cout << "heal event\n";
			process_heal_event(key);
			delete over_ex;
		}
		else if (EV_MOVE_TARGET == over_ex->event_type) {
			if (true == clients[key]->get_isDead()) { continue; }
			//cout << "target event \n";
			process_npc_chase(key, clients[key]->get_targetID());
		}
		else if (EV_NPC_RESPAWN == over_ex->event_type) {
			//cout << "npc respawn event \n";
			process_npc_respawn_event(key);
			delete over_ex;
		}
		else if (EV_PLAYER_RESPAWN == over_ex->event_type) {
			//cout << "respawn event\n";
			process_respawn_event(key);
			delete over_ex;
		}
		else if (EV_PLAYER_MOVE_NOTIFY == over_ex->event_type) {
			int player_id = *(int *)(over_ex->buffer);

			lua_State *L = clients[key]->get_L();
			clients[key]->get_vm_lock().lock();
			lua_getglobal(L, "event_player_move_notify");
			lua_pushnumber(L, player_id);
			lua_pushnumber(L, clients[player_id]->get_xPos());
			lua_pushnumber(L, clients[player_id]->get_yPos());
			lua_pcall(L, 3, 0, 0);
			clients[key]->get_vm_lock().unlock();
		}
		else {
			cout << "Unkown evnet type" << over_ex->event_type << "\n";
			while (true);
		}
	}
}

void add_eventToQueue(EVENT &event)
{
	timer_lock.lock();
	event_queue.push(event);
	timer_lock.unlock();
}

void do_timer_thread()
{
	while (true) {
		timer_lock.lock();
		while (true == event_queue.empty()) {
			timer_lock.unlock();
			this_thread::sleep_for(10ms);
			timer_lock.lock();
		}
		const EVENT &ev = event_queue.top();
		if (ev.wakeup_time > chrono::high_resolution_clock::now()) {
			timer_lock.unlock();
			this_thread::sleep_for(10ms);
			continue;
		}

		EVENT temp_ev = ev;
		event_queue.pop();
		timer_lock.unlock();

		if (EV_MOVE == temp_ev.event_type) {
			OVER_EX *over_ex = new OVER_EX;
			over_ex->event_type = EV_MOVE;
			PostQueuedCompletionStatus(g_iocp_handle, 1, temp_ev.obj_id, &over_ex->over);
		}
		else if (EV_MOVE_COOLTIME == temp_ev.event_type) {
			OVER_EX *over_ex = new OVER_EX;
			over_ex->event_type = EV_MOVE_COOLTIME;
			PostQueuedCompletionStatus(g_iocp_handle, 1, temp_ev.obj_id, &over_ex->over);
		}
		else if (EV_HEAL == temp_ev.event_type) {
			OVER_EX *over_ex = new OVER_EX;
			over_ex->event_type = EV_HEAL;
			PostQueuedCompletionStatus(g_iocp_handle, 1, temp_ev.obj_id, &over_ex->over);
		}
		else if (EV_MOVE_TARGET == temp_ev.event_type) {
			OVER_EX *over_ex = new OVER_EX;
			over_ex->event_type = EV_MOVE_TARGET;
			PostQueuedCompletionStatus(g_iocp_handle, 1, temp_ev.obj_id, &over_ex->over);
		}
		else if (EV_NPC_RESPAWN == temp_ev.event_type) {
			OVER_EX *over_ex = new OVER_EX;
			over_ex->event_type = EV_NPC_RESPAWN;
			PostQueuedCompletionStatus(g_iocp_handle, 1, temp_ev.obj_id, &over_ex->over);
		}
		else if (EV_PLAYER_RESPAWN == temp_ev.event_type) {
			OVER_EX *over_ex = new OVER_EX;
			over_ex->event_type = EV_PLAYER_RESPAWN;
			PostQueuedCompletionStatus(g_iocp_handle, 1, temp_ev.obj_id, &over_ex->over);
		}

	}
}

void processPacket(int id, void* buf)
{
	char* packet = reinterpret_cast<char*>(buf);
	switch (packet[1]) {
	case CS_REQUEST_LOGIN:
	{
		cs_packet_request_login *login_packet = reinterpret_cast<cs_packet_request_login *>(buf);
		cout << "[" << login_packet->loginid << "] request login" << "\n";

		// ���⿡ �����ͺ��̽����� ���̵� üũ�ϴ� �ڵ� �ֱ�
		if (true == use_DB) {
			for (auto &db : vec_database) {
				if (strcmp(db.idName, login_packet->loginid) == 0) {
					for (int i = 0; i < NPC_ID_START; ++i) {
						if (false == clients[i]->get_isLive()) { continue; }
						if (clients[i]->get_DBkey() == db.idKey) {
							cout << "logint fail \n";
							send_login_fail_packet(id);
							closesocket(clients[id]->get_socket());
							clients[id]->set_isLive(false);
							return;
						}
					}
					cout << "[" << login_packet->loginid << "] login success" << "\n";
					send_login_ok_packet(id);
					clients[id]->set_xPos(db.x);
					clients[id]->set_yPos(db.y);
					clients[id]->set_DBkey(db.idKey);
					clients[id]->set_hp(db.hp);
					clients[id]->set_att(60);
					clients[id]->set_exp(db.exp);
					clients[id]->set_level(db.level);
					clients[id]->set_obj_type(TYPE_PLAYER);
					clients[id]->set_isDead(false);
					clients[id]->set_isLive(true);
					EVENT ev_heal{ id, 0, EV_HEAL, chrono::high_resolution_clock::now() + 5s };
					add_eventToQueue(ev_heal);
					for (auto& cl : clients) {
						if (false == cl->get_isLive()) { continue; }
						if (true == cl->get_isDead()) { continue; }

						int other_player = cl->get_id();
						if (true == is_NPC(other_player)
							&& false == is_active(other_player)
							&& true == is_near_npc(id, other_player)) { // �÷��̾� �ֺ��� npc �����
							clients[other_player]->set_isActive(true);
							EVENT ev{ other_player, 0, EV_MOVE, chrono::high_resolution_clock::now() + 1s };
							add_eventToQueue(ev);
						}

						if (true == is_near(id, other_player))
						{
							if (false == is_NPC(other_player)) { // npc�� �ƴѰ�쿡
								send_put_player_packet(other_player, id); // �� �α��� ������ �ٸ�Ŭ���̾�Ʈ�� ����
							}

							if (other_player != id) { // �ٸ� Ŭ���̾�Ʈ ������ ������ ����
								send_put_player_packet(id, other_player);
							}
						}
					}
					return;
				}
			}
		}
		else if (false == use_DB) {
			cout << "[" << login_packet->loginid << "] login success" << "\n";
			send_login_ok_packet(id);
			clients[id]->set_xPos(start_posX);//start_posX;
			clients[id]->set_yPos(start_posY);//start_posY;
			clients[id]->set_hp(100);
			clients[id]->set_att(60);
			clients[id]->set_exp(0);
			clients[id]->set_level(1);
			clients[id]->set_obj_type(TYPE_PLAYER);
			clients[id]->set_isDead(false);
			clients[id]->set_isLive(true);
			EVENT ev_heal{ id, 0, EV_HEAL, chrono::high_resolution_clock::now() + 5s };
			add_eventToQueue(ev_heal);
			for (auto& cl : clients) {
				if (false == cl->get_isLive()) { continue; }
				if (true == cl->get_isDead()) { continue; }

				int other_player = cl->get_id();
				if (true == is_NPC(other_player)
					&& false == is_active(other_player)
					&& true == is_near_npc(id, other_player)) { // �÷��̾� �ֺ��� npc �����
					clients[other_player]->set_isActive(true);
					EVENT ev{ other_player, 0, EV_MOVE, chrono::high_resolution_clock::now() + 1s };
					add_eventToQueue(ev);
				}

				if (true == is_near(id, other_player))
				{
					if (false == is_NPC(other_player)) { // npc�� �ƴѰ�쿡
						send_put_player_packet(other_player, id); // �� �α��� ������ �ٸ�Ŭ���̾�Ʈ�� ����
					}

					if (other_player != id) { // �ٸ� Ŭ���̾�Ʈ ������ ������ ����
						send_put_player_packet(id, other_player);
					}
				}
			}
			return;
		}

		if (true == use_DB) {
			cout << "[" << login_packet->loginid << "] is invalid ID" << "\n"; //
			cout << "create new ID to DB \n";
			DBCount++;
			sql_insert_database(DBCount, login_packet->loginid, login_packet->loginid, 1, start_posX, start_posY, 100, 0, 60);
			DATABASE *db = new DATABASE;
			db->idKey = DBCount;
			strcpy_s(db->idName, login_packet->loginid);
			db->level = 1;
			db->x = start_posX;
			db->y = start_posY;
			db->hp = 100;
			db->exp = 0;
			vec_database.emplace_back(*db);
			send_login_ok_packet(id);
			clients[id]->set_DBkey(db->idKey);
			clients[id]->set_xPos(db->x);
			clients[id]->set_yPos(db->y);
			clients[id]->set_hp(db->hp);
			clients[id]->set_att(60);
			clients[id]->set_exp(db->exp);
			clients[id]->set_level(db->level);
			clients[id]->set_obj_type(TYPE_PLAYER);
			clients[id]->set_isDead(false);
			clients[id]->set_isLive(true);
			EVENT ev_heal{ id, 0, EV_HEAL, chrono::high_resolution_clock::now() + 5s };
			add_eventToQueue(ev_heal);
			for (auto& cl : clients) {
				if (false == cl->get_isLive()) { continue; }
				if (true == cl->get_isDead()) { continue; }

				int other_player = cl->get_id();
				if (true == is_NPC(other_player)
					&& false == is_active(other_player)
					&& true == is_near_npc(id, other_player)) { // �÷��̾� �ֺ��� npc �����
					clients[other_player]->set_isActive(true);
					EVENT ev{ other_player, 0, EV_MOVE, chrono::high_resolution_clock::now() + 1s };
					add_eventToQueue(ev);
				}

				if (true == is_near(id, other_player))
				{
					if (false == is_NPC(other_player)) { // npc�� �ƴѰ�쿡
						send_put_player_packet(other_player, id); // �� �α��� ������ �ٸ�Ŭ���̾�Ʈ�� ����
					}

					if (other_player != id) { // �ٸ� Ŭ���̾�Ʈ ������ ������ ����
						send_put_player_packet(id, other_player);
					}
				}
			}
			return;
		}

	}
	case CS_ATTACK:
	{
		//cout << "ATTACk" << endl;
		cs_packet_attack * attack_packet = reinterpret_cast<cs_packet_attack *>(buf);
		int id = attack_packet->id;
		short x = clients[id]->get_xPos();
		short y = clients[id]->get_yPos();
		short att = clients[id]->get_att();
		set<int> temp = clients[id]->get_viewList();
		for (auto vl : temp) {
			if (false == is_NPC(vl)) { continue; }
			short vx = clients[vl]->get_xPos();
			short vy = clients[vl]->get_yPos();
			if (vx == x && vy == (y - 1)) { // ��
				clients[vl]->set_hp(clients[vl]->get_hp() - att);
				send_battle_mess_packet(id, vl, att);
			}
			else if (vx == (x - 1) && vy == y) { // ����
				clients[vl]->set_hp(clients[vl]->get_hp() - att);
				send_battle_mess_packet(id, vl, att);
			}
			else if (vx == (x + 1) && vy == y) { // ������
				clients[vl]->set_hp(clients[vl]->get_hp() - att);
				send_battle_mess_packet(id, vl, att);
			}
			else if (vx == x && vy == (y + 1)) { // �Ʒ�
				clients[vl]->set_hp(clients[vl]->get_hp() - att);
				send_battle_mess_packet(id, vl, att);
			}

			if (0 >= clients[vl]->get_hp()) { // hp 0 ���ϸ� ����
				if (TYPE_PEACE_MONSTER == clients[vl]->get_obj_type()) {
					clients[id]->set_exp(clients[id]->get_exp() + 100);
				}
				if (TYPE_WAR_MONSTER == clients[vl]->get_obj_type()) {
					clients[id]->set_exp(clients[id]->get_exp() + 200);
				}
				send_stat_change_packet(id);
				for (int i = 0; i < NPC_ID_START; ++i) {
					if (false == clients[i]->get_isLive()) { continue; }
					if (true == is_near(vl, i)) {
						send_remove_player_packet(i, vl);
					}
				}
				//send_remove_player_packet(id, vl);
				clients[vl]->set_isDead(true);
				//clients[vl].is_live = false;
				EVENT ev_npc_respawn{ vl, 0, EV_NPC_RESPAWN, chrono::high_resolution_clock::now() + 30s };
				add_eventToQueue(ev_npc_respawn);

			}
		}
		break;
	}
	case CS_CHAT:
	{
		cs_packet_chat * chat_packet = reinterpret_cast<cs_packet_chat*>(buf);
		send_all_chat_packet(chat_packet->id, chat_packet->name, chat_packet->chat);
		break;
	}
	case CS_LEFT:
		process_player_move(id, buf);
		break;
	case CS_RIGHT:
		process_player_move(id, buf);
		break;
	case CS_UP:
		process_player_move(id, buf);
		break;
	case CS_DOWN:
		process_player_move(id, buf);
		break;
	}
}

void process_player_move(int id, void* buf)
{
	if (true == clients[id]->get_moveCooltime()) { return; }
	char* packet = reinterpret_cast<char*>(buf);
	short x = clients[id]->get_xPos();
	short y = clients[id]->get_yPos();
	if (true == clients[id]->get_isDead()) { return; }
	switch (packet[1]) {
	case CS_LEFT:
		if (x >= 1 && WorldMap_Arr[y][x - 1] == 0) {
			x -= 1;
		}
		else {
			return;
		}
		break;
	case CS_RIGHT:
		if (x <= MAP_SCALE_X - 1 && WorldMap_Arr[y][x + 1] == 0) {
			x += 1;
		}
		else {
			return;
		}
		break;
	case CS_UP:
		if (y >= 1 && WorldMap_Arr[y - 1][x] == 0) {
			y -= 1;
		}
		else {
			return;
		}
		break;
	case CS_DOWN:
		if (y <= MAP_SCALE_X - 1 && WorldMap_Arr[y + 1][x] == 0) {
			y += 1;
		}
		else {
			return;
		}
		break;
	}

	clients[id]->get_viewList_lock().lock();
	auto old_view_list = clients[id]->get_viewList();
	clients[id]->get_viewList_lock().unlock();

	clients[id]->set_xPos(x);
	clients[id]->set_yPos(y);

	set<int> new_view_list;
	for (auto &cl : clients) {
		if (false == cl->get_isLive()) { continue; }
		if (true == cl->get_isDead()) { continue; }

		int other_id = cl->get_id();
		if (id == other_id) continue;
		if (true == is_near_npc(id, other_id)) {
			if (true == is_NPC(other_id) && false == is_active(other_id)) {
				clients[other_id]->set_isActive(true);
				EVENT ev{ other_id, 0, EV_MOVE, chrono::high_resolution_clock::now() + 1s };
				add_eventToQueue(ev);
			}
			if (true == is_near(id, other_id)) {
				new_view_list.insert(other_id);
				if (true == is_NPC(other_id)) {
					OVER_EX *over_ex = new OVER_EX;
					over_ex->event_type = EV_PLAYER_MOVE_NOTIFY;
					*(int *)(over_ex->buffer) = id;
					PostQueuedCompletionStatus(g_iocp_handle, 1, other_id, &over_ex->over);
				}
			}
		}
	}

	send_pos_packet(id, id); // �ڱ� �ڽſ��� ����

	for (auto cl : new_view_list) {
		if (0 == old_view_list.count(cl)) { // new�丮��Ʈ�߿� ���� �߰��Ȱ�
			send_put_player_packet(id, cl);
			if (false == is_NPC(cl)) {
				send_put_player_packet(cl, id);
			}
		}
	}

	for (auto cl : old_view_list) {
		if (0 != new_view_list.count(cl)) { // new �丮��Ʈ�� �����鼭 
			if (false == is_NPC(cl)) { // NPC�� �ƴ��÷��̾�
				send_pos_packet(cl, id); // ���� ������ ��ġ ����
			}
		}
		else { // new view����Ʈ�� ������
			send_remove_player_packet(id, cl); // remove ��Ŷ����
			if (false == is_NPC(cl)) {
				send_remove_player_packet(cl, id);
			}
		}
	}

	clients[id]->set_moveCooltime(true);
	EVENT ev_move_cooltime{ id, 0, EV_MOVE_COOLTIME, chrono::high_resolution_clock::now() + 500ms };
	add_eventToQueue(ev_move_cooltime);

	//cout << "move" << endl;
}

void process_heal_event(int id)
{
	if (100 > clients[id]->get_hp() && clients[id]->get_isDead() == false) {
		clients[id]->set_hp(clients[id]->get_hp() + 10);
		send_stat_change_packet(id);
	}
	EVENT ev_heal{ id, 0, EV_HEAL, chrono::high_resolution_clock::now() + 5s };
	add_eventToQueue(ev_heal);
}

void process_respawn_event(int id)
{
	clients[id]->set_xPos(start_posX);
	clients[id]->set_yPos(start_posY);
	clients[id]->set_hp(100);
	clients[id]->set_att(60);
	clients[id]->set_exp(clients[id]->get_exp() / 2);
	clients[id]->set_level(1);
	clients[id]->set_isDead(false);
	send_put_player_packet(id, id);
	send_stat_change_packet(id);
	for (auto& cl : clients) {
		if (false == cl->get_isLive()) { continue; }

		int other_player = cl->get_id();

		if (true == is_near(id, other_player))
		{
			if (false == is_NPC(other_player)) { // npc�� �ƴѰ�쿡
				send_put_player_packet(other_player, id); // �� �α��� ������ �ٸ�Ŭ���̾�Ʈ�� ����
			}

			if (other_player != id) { // �ٸ� Ŭ���̾�Ʈ ������ ������ ����
				send_put_player_packet(id, other_player);
			}
		}
	}
}

void process_npc_respawn_event(int id)
{
	clients[id]->set_hp(100);
	clients[id]->set_isDead(false);
	EVENT ev_move{ id, 0, EV_MOVE, chrono::high_resolution_clock::now() + 1s };
	add_eventToQueue(ev_move);
}

void process_npc_chase(int npc_id, int target_id)
{
	if (false == clients[target_id]->get_isLive()) { return; }

	set<int> old_view_list;
	for (int i = 0; i < NPC_ID_START; ++i) {
		if (false == clients[i]->get_isLive()) { continue; }
		if (true == is_near(npc_id, i)) {
			old_view_list.insert(i);
		}
	}

	short t_x = clients[target_id]->get_xPos();
	short t_y = clients[target_id]->get_yPos();
	short n_x = clients[npc_id]->get_xPos();
	short n_y = clients[npc_id]->get_yPos();

	short dis_x = abs(n_x - t_x);
	short dis_y = abs(n_y - t_y);

	if (dis_x > dis_y) {
		if (n_x > t_x) {
			clients[npc_id]->add_xPos(-1);
		}
		else if (n_x < t_x) {
			clients[npc_id]->add_xPos(1);
		}
	}
	else if (dis_x == dis_y) {
		if (n_x > t_x) {
			clients[npc_id]->add_xPos(-1);
		}
		else if (n_x < t_x) {
			clients[npc_id]->add_xPos(1);
		}
	}
	else if (dis_x < dis_y) {
		if (n_y > t_y) {
			clients[npc_id]->add_yPos(-1);
		}
		else if (n_y < t_y) {
			clients[npc_id]->add_yPos(1);
		}
	}

	if (t_x == clients[npc_id]->get_xPos() && t_y == clients[npc_id]->get_yPos()) {
		clients[npc_id]->set_xPos(n_x);
		clients[npc_id]->set_yPos(n_y);
	}

	for (auto vl : old_view_list) {
		if (true == is_NPC(vl)) { continue; }
		if (false == is_near(npc_id, vl)) {
			send_remove_player_packet(vl, npc_id);
		}
	}
	for (auto &cl : clients) {
		if (false == cl->get_isLive()) { continue; }
		if (true == cl->get_isDead()) { continue; }
		if (true == is_NPC(cl->get_id())) { continue; } // npc���Դ� npc�� �̵��� ����������
		if (false == is_near(cl->get_id(), npc_id)) { continue; } // �÷��̾ �þ߾ȿ� ���ٸ� �������ʴ´�.
		send_pos_packet(cl->get_id(), npc_id);
	}

	short x = clients[npc_id]->get_xPos();
	short y = clients[npc_id]->get_yPos();
	for (int i = 0; i < NPC_ID_START; ++i) {
		if (true == clients[i]->get_isLive()) {
			short px = clients[i]->get_xPos();
			short py = clients[i]->get_yPos();
			short att = clients[npc_id]->get_att();
			if (x == px && y - 1 == py) { // ��
				clients[i]->set_hp(clients[i]->get_hp() - att);
				send_stat_change_packet(i);
				send_battle_mess_packet(npc_id, i, att);
			}
			if (x - 1 == px && y == py) { // ��
				clients[i]->set_hp(clients[i]->get_hp() - att);
				send_stat_change_packet(i);
				send_battle_mess_packet(npc_id, i, att);
			}
			if (x + 1 == px && y == py) { // ����
				clients[i]->set_hp(clients[i]->get_hp() - att);
				send_stat_change_packet(i);
				send_battle_mess_packet(npc_id, i, att);
			}
			if (x == px && y + 1 == py) { // �Ʒ�
				clients[i]->set_hp(clients[i]->get_hp() - att);
				send_stat_change_packet(i);
				send_battle_mess_packet(npc_id, i, att);
			}
			if (x == px && y == py) { // ��ģ����
				clients[i]->set_hp(clients[i]->get_hp() - att);
				send_stat_change_packet(i);
				send_battle_mess_packet(npc_id, i, att);
			}
			// �÷��̾� ����� ��Ȱ �̺�Ʈ �߰�
			if (0 >= clients[i]->get_hp() && clients[i]->get_isDead() == false) {
				clients[i]->set_isDead(true);
				send_player_dead_packet(i);
				EVENT ev_respawn{ i, 0, EV_PLAYER_RESPAWN, chrono::high_resolution_clock::now() + 5s };
				add_eventToQueue(ev_respawn);
				for (auto vl : clients[i]->get_viewList()) {
					if (false == is_NPC(vl)) {
						send_remove_player_packet(vl, i);
					}
				}
			}
		}
	}

	if (true == clients[target_id]->get_isDead()) {
		EVENT new_ev{ npc_id, 0, EV_MOVE, chrono::high_resolution_clock::now() + 1s };
		add_eventToQueue(new_ev);
		return;
	}

	EVENT ev_move_target{ npc_id, 0, EV_MOVE_TARGET, chrono::high_resolution_clock::now() + 1s }; // �̵��� �ٽ� 1�ʵ� �̵��ϰ� Ÿ�̸�add
	add_eventToQueue(ev_move_target);

}

void process_npc_move(int npc_id)
{
	/*if (0 == clients.count(npc_id)) {
		cout << "npc id: " << npc_id << "does not exist \n";
		while (true);
	}*/
	if (false == is_NPC(npc_id)) {  // npc�� �ƴҰ��
		cout << "ID" << npc_id << "is not NPC \n";
		while (true);
	}
	if (true == clients[npc_id]->get_isDead()) { return; }

	bool player_exist = false;
	for (int i = 0; i < NPC_ID_START; ++i) {
		if (false == clients[i]->get_isLive()) { // ���� id
			continue;
		}
		if (true == is_near_npc(i, npc_id)) {
			player_exist = true;
			break;
		}
	}
	if (false == player_exist) {
		if (true == is_active(npc_id)) {
			clients[npc_id]->set_isActive(false);
			return;
		}
	}


	SocketInfo *npc = clients[npc_id];
	int x = npc->get_xPos();
	int y = npc->get_yPos();

	set<int> old_view_list;
	for (int i = 0; i < NPC_ID_START; ++i) {
		if (false == clients[i]->get_isLive()) { continue; }
		if (true == is_near(npc->get_id(), i)) {
			old_view_list.insert(i);
		}
	}

	switch (rand() % 4) {
	case 0:
		if (y > 0 && WorldMap_Arr[y - 1][x] == 0) {
			--y;
		}
		break;
	case 1:
		if (y < (MAP_SCALE_Y - 1) && WorldMap_Arr[y + 1][x] == 0) {
			++y;
		}
		break;
	case 2:
		if (x > 0 && WorldMap_Arr[y][x - 1] == 0) {
			--x;
		}
		break;
	case 3:
		if (x < (MAP_SCALE_X - 1) && WorldMap_Arr[y][x + 1] == 0) {
			++x;
		}
		break;
	}
	npc->set_xPos(x);
	npc->set_yPos(y);

	/*set<int> new_view_list;
	for (auto &obj : clients) {
		if (true == is_near(npc->id, obj.second->id)) {
			new_view_list.insert(obj.second->id);
		}
	}*/

	for (auto vl : old_view_list) {
		if (true == is_NPC(vl)) { continue; }
		if (false == is_near(npc_id, vl)) {
			send_remove_player_packet(vl, npc_id);
		}
	}
	for (auto &cl : clients) {
		if (false == cl->get_isLive()) { continue; }
		if (true == cl->get_isDead()) { continue; }
		if (true == is_NPC(cl->get_id())) { continue; } // npc���Դ� npc�� �̵��� ����������
		if (false == is_near(cl->get_id(), npc->get_id())) { continue; } // �÷��̾ �þ߾ȿ� ���ٸ� �������ʴ´�.
		send_pos_packet(cl->get_id(), npc->get_id());
	}

	if (TYPE_WAR_MONSTER == npc->get_obj_type()) {
		set<int> aggro_list;
		for (int i = 0; i < NPC_ID_START; ++i) {
			if (false == clients[i]->get_isLive()) { continue; }
			if (true == clients[i]->get_isDead()) { continue; }
			if (true == in_aggro_range(npc->get_id(), i)) {
				aggro_list.insert(i);
			}
		}
		if (0 != aggro_list.size()) {
			//cout << "aggro on \n";
			double min_dis = 100;
			int target_id = -1;
			for (auto p : aggro_list) {
				double distance = sqrt(pow(clients[npc_id]->get_xPos() - clients[p]->get_xPos(), 2) + pow(clients[npc_id]->get_yPos() - clients[p]->get_yPos(), 2));
				if (distance < min_dis) {
					min_dis = distance;
					target_id = p;
				}
			}
			clients[npc_id]->set_targetID(target_id);
			EVENT ev_move_target{ npc_id, 0, EV_MOVE_TARGET, chrono::high_resolution_clock::now() + 1s }; // �̵��� �ٽ� 1�ʵ� �̵��ϰ� Ÿ�̸�add
			add_eventToQueue(ev_move_target);
			return;
		}
	}


	EVENT new_ev{ npc_id, 0, EV_MOVE, chrono::high_resolution_clock::now() + 1s }; // �̵��� �ٽ� 1�ʵ� �̵��ϰ� Ÿ�̸�add
	add_eventToQueue(new_ev);

	if (TYPE_WAR_MONSTER == clients[npc_id]->get_obj_type()) { // war_monster ����
		short x = clients[npc_id]->get_xPos();
		short y = clients[npc_id]->get_yPos();
		for (int i = 0; i < NPC_ID_START; ++i) {
			if (true == clients[i]->get_isLive()) {
				short px = clients[i]->get_xPos();
				short py = clients[i]->get_yPos();
				short att = clients[npc_id]->get_att();
				if (x == px && y - 1 == py) { // ��
					clients[i]->set_hp(clients[i]->get_hp() - att);
					send_stat_change_packet(i);
				}
				if (x - 1 == px && y == py) { // ��
					clients[i]->set_hp(clients[i]->get_hp() - att);
					send_stat_change_packet(i);
				}
				if (x + 1 == px && y == py) { // ����
					clients[i]->set_hp(clients[i]->get_hp() - att);
					send_stat_change_packet(i);
				}
				if (x == px && y + 1 == py) { // �Ʒ�
					clients[i]->set_hp(clients[i]->get_hp() - att);
					send_stat_change_packet(i);
				}
				if (x == px && y == py) { // ��ģ����
					clients[i]->set_hp(clients[i]->get_hp() - att);
					send_stat_change_packet(i);
				}
				// �÷��̾� ����� ��Ȱ �̺�Ʈ �߰�
				if (0 >= clients[i]->get_hp() && clients[i]->get_isDead() == false) {
					clients[i]->set_isDead(true);
					send_player_dead_packet(i);
					EVENT ev_respawn{ i, 0, EV_PLAYER_RESPAWN, chrono::high_resolution_clock::now() + 5s };
					add_eventToQueue(ev_respawn);
					for (auto vl : clients[i]->get_viewList()) {
						if (false == is_NPC(vl)) {
							send_remove_player_packet(vl, i);
						}
					}
				}
			}
		}
	}

}


bool is_NPC(int npc_id)
{
	return npc_id >= NPC_ID_START;
}

bool is_active(int npc_id)
{
	return clients[npc_id]->get_isActive();
}

bool is_near(int my_id, int other_id)	// �� ���̵� �� �Ÿ��� �þ߹��� �� ���� �Ǻ�
{
	if (abs(clients[my_id]->get_xPos() - clients[other_id]->get_xPos()) < 8 && abs(clients[my_id]->get_yPos() - clients[other_id]->get_yPos()) < 8) {
		return true;
	}
	else
		return false;
}

bool is_near_npc(int my_id, int npc_id)
{
	if (abs(clients[my_id]->get_xPos() - clients[npc_id]->get_xPos()) < 16 && abs(clients[my_id]->get_yPos() - clients[npc_id]->get_yPos()) < 16) {
		return true;
	}
	else
		return false;
}

bool in_aggro_range(int npc_id, int player)
{
	if (abs(clients[npc_id]->get_xPos() - clients[player]->get_xPos()) < 4 && abs(clients[npc_id]->get_yPos() - clients[player]->get_yPos()) < 4) {
		return true;
	}
	else
		return false;
}

void send_packet(int id, void* buf)
{
	char* packet = reinterpret_cast<char*>(buf);
	int packet_size = packet[0];
	OVER_EX *send_over = new OVER_EX;
	memset(send_over, 0x00, sizeof(OVER_EX));
	send_over->event_type = EV_SEND;
	memcpy(send_over->buffer, packet, packet_size);
	send_over->wsabuf[0].buf = send_over->buffer;
	send_over->wsabuf[0].len = packet_size;

	int returnCode = WSASend(clients[id]->get_socket(), send_over->wsabuf, 1, 0, 0, &send_over->over, 0);
	if (0 != returnCode) {
		int err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no)
			error_display("WSASend Error :", err_no);
	}
}

void send_id_packet(int id)
{
	sc_packet_send_id packet;
	packet.id = id;
	packet.size = sizeof(packet);
	packet.type = SC_SEND_ID;
	send_packet(id, &packet);
}

void send_login_ok_packet(int id)
{
	sc_packet_login_ok packet;
	packet.id = id;
	packet.size = sizeof(packet);
	packet.type = SC_LOGIN_OK;
	send_packet(id, &packet);
}

void send_put_player_packet(int client, int new_id)
{
	sc_packet_put_player packet;
	packet.id = new_id;
	packet.size = sizeof(packet);
	packet.type = SC_PUT_PLAYER;
	packet.x = clients[new_id]->get_xPos();
	packet.y = clients[new_id]->get_yPos();
	packet.hp = clients[new_id]->get_hp();
	packet.att = clients[new_id]->get_att();
	packet.exp = clients[new_id]->get_exp();
	packet.obj_type = clients[new_id]->get_obj_type();
	packet.level = clients[new_id]->get_level();

	send_packet(client, &packet);

	if (client == new_id) {
		return;
	}
	lock_guard<mutex> lg{ clients[client]->get_viewList_lock() };
	if (client != new_id) {
		if (0 == clients[client]->viewList_count(new_id)) {
			clients[client]->insert_viewList(new_id);
		}
	}

}

void send_pos_packet(int client_id, int move_id)
{
	sc_packet_pos packet;
	packet.id = move_id;
	packet.size = sizeof(packet);
	packet.type = SC_POS;
	packet.x = clients[move_id]->get_xPos();
	packet.y = clients[move_id]->get_yPos();

	clients[client_id]->get_viewList_lock().lock();
	if (0 != clients[client_id]->viewList_count(move_id)) {
		clients[client_id]->get_viewList_lock().unlock();
		send_packet(client_id, &packet);
	}
	else {
		clients[client_id]->get_viewList_lock().unlock();
		if (client_id == move_id) { 
			send_packet(client_id, &packet); 
			return;
		}
		send_put_player_packet(client_id, move_id);
	}


}

void send_remove_player_packet(int client, int leaver)
{
	sc_packet_remove_player packet;
	packet.id = leaver;
	packet.size = sizeof(packet);
	packet.type = SC_REMOVE_PLAYER;
	send_packet(client, &packet);

	lock_guard<mutex> lg{ clients[client]->get_viewList_lock() };
	clients[client]->delete_viewList(leaver);;
}

void send_all_chat_packet(int chatter,char name[], char mess[])
{
	sc_packet_chat packet;
	packet.id = chatter;
	packet.size = sizeof(packet);
	packet.type = SC_CHAT;
	strcpy_s(packet.name, name);
	strcpy_s(packet.chat, mess);
	for (int i = 0; i < NPC_ID_START; ++i) {
		if (true == clients[i]->get_isLive()) {
			send_packet(i, &packet);
		}
	}
}

void send_chat_packet(int client, int chatter, char mess[])
{
	sc_packet_chat packet;
	packet.id = chatter;
	packet.size = sizeof(packet);
	packet.type = SC_CHAT;
	strcpy_s(packet.chat, mess);
	send_packet(client, &packet);
}

void send_stat_change_packet(int client)
{
	sc_packet_stat_change packet;
	packet.type = SC_STAT_CHANGE;
	packet.hp = clients[client]->get_hp();
	packet.size = sizeof(packet);
	if (clients[client]->get_exp() >= 100 * (pow(2, clients[client]->get_level() - 1))) {
		clients[client]->set_level(clients[client]->get_level() + 1);
	}
	packet.exp = clients[client]->get_exp();
	packet.level = clients[client]->get_level();
	send_packet(client, &packet);
}

void send_player_dead_packet(int client)
{
	sc_packet_player_dead packet;
	packet.type = SC_PLAYER_DEAD;
	packet.size = sizeof(packet);
	send_packet(client, &packet);
}

void send_login_fail_packet(int client)
{
	sc_packet_login_fail packet;
	packet.type = SC_LOGIN_FAIL;
	packet.size = sizeof(packet);
	send_packet(client, &packet);
}

void send_battle_mess_packet(int hitter_id, int damaged_id, short damage )
{
	sc_packet_battle_mess packet;
	packet.type = SC_BATTLE_MESS;
	packet.size = sizeof(packet);
	packet.hitterID = hitter_id;
	packet.damagedID = damaged_id;
	packet.damage = damage;

	if (clients[hitter_id]->get_id() < NPC_ID_START) { // �����ڰ� �÷��̾��ϰ��
		send_packet(hitter_id, &packet);
	}
	else {
		send_packet(damaged_id, &packet);
	}
}

void create_NPC()
{
	for (int npc_id = NPC_ID_START; npc_id < NPC_ID_START + NUM_NPC - 100; ++npc_id) {
		clients[npc_id]->set_id(npc_id);
		clients[npc_id]->set_isActive(false);
		clients[npc_id]->set_obj_type(TYPE_PEACE_MONSTER);
		while (true) {
			short x = uidall(dre);
			short y = uidall(dre);
			if (WorldMap_Arr[y][x] == 0) {
				clients[npc_id]->set_xPos(x);
				clients[npc_id]->set_yPos(y);
				break;
			}
		}
		clients[npc_id]->set_socket(-1);
		clients[npc_id]->set_isDead(false);
		clients[npc_id]->set_isLive(true);
		clients[npc_id]->set_luaL_newState();
		lua_State *L = clients[npc_id]->get_L();
		luaL_openlibs(L);
		luaL_loadfile(L, "peace_monster.lua");
		lua_pcall(L, 0, 0, 0);
		lua_getglobal(L, "set_npc_id");
		lua_pushnumber(L, npc_id);
		lua_pcall(L, 1, 0, 0);
		lua_getglobal(L, "hp");
		lua_getglobal(L, "att");
		clients[npc_id]->set_hp((int)lua_tonumber(L, -2));
		clients[npc_id]->set_att((int)lua_tonumber(L, -1));
		lua_pop(L, 3);
		/*while (true) {
			lua_getglobal(L, "set_npc_pos");
			lua_pcall(L, 0, 0, 0);
			lua_getglobal(L, "init_x");
			lua_getglobal(L, "init_y");
			short x = (int)lua_tonumber(L, -2);
			short y = (int)lua_tonumber(L, -1);
			lua_pop(L, 2);
			if (WorldMap_Arr[y][x] == 0) {
				clients[npc_id].x = x;
				clients[npc_id].y = y;
				break;
			}
		}*/

		lua_register(L, "API_get_x_position", lua_get_npc_xpos);
		lua_register(L, "API_get_y_position", lua_get_npc_ypos);
		lua_register(L, "API_send_chat_packet", lua_send_NPCchat_packet);
	}
	for (int npc_id = NPC_ID_START + NUM_NPC - 100; npc_id < NPC_ID_START + NUM_NPC - 50; ++npc_id) {
		// ����1
		clients[npc_id]->set_id(npc_id);
		clients[npc_id]->set_isActive(false);
		clients[npc_id]->set_obj_type(TYPE_WAR_MONSTER);
		while (true) {
			short x = uidx_1(dre);
			short y = uidy_1(dre);
			if (WorldMap_Arr[y][x] == 0) {
				clients[npc_id]->set_xPos(x);
				clients[npc_id]->set_yPos(y);
				break;
			}
		}
		//clients[npc_id].x = 1;
		//clients[npc_id].y = 1;
		clients[npc_id]->set_socket(-1);
		clients[npc_id]->set_isDead(false);
		clients[npc_id]->set_isLive(true);
		clients[npc_id]->set_luaL_newState();
		lua_State *L = clients[npc_id]->get_L();
		luaL_openlibs(L);
		luaL_loadfile(L, "war_monster.lua");
		lua_pcall(L, 0, 0, 0);
		lua_getglobal(L, "set_npc_id");
		lua_pushnumber(L, npc_id);
		lua_pcall(L, 1, 0, 0);
		lua_getglobal(L, "hp");
		lua_getglobal(L, "att");
		clients[npc_id]->set_hp((int)lua_tonumber(L, -2));
		clients[npc_id]->set_att((int)lua_tonumber(L, -1));
		lua_pop(L, 3);
	}
	for (int npc_id = NPC_ID_START + NUM_NPC - 50; npc_id < NPC_ID_START + NUM_NPC; ++npc_id) {
		// ����2
		clients[npc_id]->set_id(npc_id);
		clients[npc_id]->set_isActive(false);
		clients[npc_id]->set_obj_type(TYPE_WAR_MONSTER);
		while (true) {
			short x = uidx_2(dre);
			short y = uidy_2(dre);
			if (WorldMap_Arr[y][x] == 0) {
				clients[npc_id]->set_xPos(x);
				clients[npc_id]->set_yPos(y);
				break;
			}
		}
		//clients[npc_id].x = 1;
		//clients[npc_id].y = 1;
		clients[npc_id]->set_socket(-1);
		clients[npc_id]->set_isDead(false);
		clients[npc_id]->set_isLive(true);
		clients[npc_id]->set_luaL_newState();
		lua_State *L = clients[npc_id]->get_L();
		luaL_openlibs(L);
		luaL_loadfile(L, "war_monster.lua");
		lua_pcall(L, 0, 0, 0);
		lua_getglobal(L, "set_npc_id");
		lua_pushnumber(L, npc_id);
		lua_pcall(L, 1, 0, 0);
		lua_getglobal(L, "hp");
		lua_getglobal(L, "att");
		clients[npc_id]->set_hp((int)lua_tonumber(L, -2));
		clients[npc_id]->set_att((int)lua_tonumber(L, -1));
		lua_pop(L, 3);
	}

	cout << "npc �����Ϸ�\n";
}

int lua_get_npc_xpos(lua_State *L)
{
	int npc_id = (int)lua_tonumber(L, -1);
	lua_pop(L, 2);
	int x = clients[npc_id]->get_xPos();
	lua_pushnumber(L, x);
	return 1;
}

int lua_get_npc_ypos(lua_State *L)
{
	int npc_id = (int)lua_tonumber(L, -1);
	lua_pop(L, 2);
	int y = clients[npc_id]->get_yPos();
	lua_pushnumber(L, y);
	return 1;
}

int lua_send_NPCchat_packet(lua_State *L)
{
	int player_id = (int)lua_tonumber(L, -3);
	int chatter_id = (int)lua_tonumber(L, -2);
	char *mess = (char*)lua_tostring(L, -1);
	lua_pop(L, 4);
	//cout << "CHAT : from= " << chatter_id << ",  to= " 
	//	<< player_id << ", [" << mess << "]\n";
	send_chat_packet(player_id, chatter_id, mess);
	return 0;
}


void error_display(const char *msg, int err_no)		// ��Ʈ��ũ ������ �����Ͽ� �����ִ� �Լ�
{
	WCHAR *lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::cout << msg;
	std::wcout << L"���� " << lpMsgBuf << std::endl;
	while (true);		// �̺κп��� �Ϻη� ���߰� �ϱ����ؼ�
	LocalFree(lpMsgBuf);
}

void sql_show_error() {
	printf("error\n");
}

void sql_HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode) {
	SQLSMALLINT iRec = 0;
	SQLINTEGER  iError;
	WCHAR       wszMessage[1000];
	WCHAR       wszState[SQL_SQLSTATE_SIZE + 1];
	if (RetCode == SQL_INVALID_HANDLE) {
		fwprintf(stderr, L"Invalid handle!\n");
		return;
	} while (SQLGetDiagRec(hType, hHandle, ++iRec, wszState, &iError, wszMessage,
		(SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT *)NULL) == SQL_SUCCESS) {
		// Hide data truncated.. 
		if (wcsncmp(wszState, L"01004", 5)) {
			fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
		}
	}
}

void sql_load_database()
{
	SQLHENV henv;		// �����ͺ��̽��� �����Ҷ� ����ϴ� �ڵ�
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0; // sql��ɾ �����ϴ� �ڵ�
	SQLRETURN retcode;  // sql��ɾ ������ ���������� ��������
	SQLINTEGER nKey, nLevel, nX, nY, nHp, nExp;	// ��Ƽ��
	SQLWCHAR szID[11];	// ���ڿ�
	SQLLEN cbID = 0, cbKey = 0, cbLevel = 0, cbX = 0, cbY = 0, cbHp = 0, cbExp = 0;

	setlocale(LC_ALL, "korean"); // �����ڵ� �ѱ۷� ��ȯ
	//std::wcout.imbue(std::locale("korean"));

	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0); // ODBC�� ����

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0); // 5�ʰ� ���� 5�ʳѾ�� Ÿ�Ӿƿ�

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2015184024_gameserver", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
				//retcode = SQLConnect(hdbc, (SQLWCHAR*)L"jys_gameserver", SQL_NTS, (SQLWCHAR*)NULL, SQL_NTS, NULL, SQL_NTS);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt); // SQL��ɾ� ������ �ѵ�

					retcode = SQLExecDirect(hstmt, (SQLWCHAR *)L"SELECT c_id, c_key, c_level, c_px, c_py, c_hp, c_exp FROM player_table ORDER BY 2, 1, 3", SQL_NTS); // ��� ���� �� ��������
					//retcode = SQLExecDirect(hstmt, (SQLWCHAR *)L"EXEC select_highlevel 90", SQL_NTS); // 90���� �̻� ��������

					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

						// Bind columns 1, 2, and 3  
						retcode = SQLBindCol(hstmt, 1, SQL_UNICODE_CHAR, szID, 11, &cbID); // �̸� �����ڵ��� SQL_UNICODE_CHAR ���
						retcode = SQLBindCol(hstmt, 2, SQL_C_LONG, &nKey, 4, &cbKey);	// ���̵�
						retcode = SQLBindCol(hstmt, 3, SQL_C_LONG, &nLevel, 4, &cbLevel);	// ����ġ
						retcode = SQLBindCol(hstmt, 4, SQL_C_LONG, &nX, 4, &cbX);
						retcode = SQLBindCol(hstmt, 5, SQL_C_LONG, &nY, 4, &cbY);
						retcode = SQLBindCol(hstmt, 6, SQL_C_LONG, &nHp, 4, &cbHp);
						retcode = SQLBindCol(hstmt, 7, SQL_C_LONG, &nExp, 4, &cbExp);

						// Fetch and print each row of data. On an error, display a message and exit.  
						for (int i = 0; ; i++) {
							retcode = SQLFetch(hstmt);  // hstmt ���� �����͸� �������°�
							if (retcode == SQL_ERROR)
								sql_show_error();
							if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
								wprintf(L"%d: %d %lS %d %d %d %d %d \n", i + 1, nKey, szID, nLevel, nX, nY, nHp, nExp);

								// wchar char�� ��ȯ
								char *temp;
								int strSize = WideCharToMultiByte(CP_ACP, 0, szID, -1, NULL, 0, NULL, NULL);
								temp = new char[11];
								WideCharToMultiByte(CP_ACP, 0, szID, -1, temp, strSize, 0, 0);
								if (isdigit(temp[strlen(temp) - 1]) == 0) {
									//cout << "���ڿ���������\n";
									temp[strlen(temp) - 1] = 0; // �������������� �𸣰����� ���̵��������ڰ� ������ ��� �ǵڿ� �����ϳ��� �߰���
								}
								DATABASE data;
								data.idKey = nKey;
								memcpy(data.idName, temp, 10);
								data.level = nLevel;
								data.x = (short)nX;
								data.y = (short)nY;
								data.hp = (short)nHp;
								data.exp = (short)nExp;
								cout << "[" << data.idName << "]  ";
								cout << data.idKey << " " << data.level << " " << data.x << " " << data.y << " "
									<<data.hp<< " " << data.exp <<"\n";
								vec_database.emplace_back(data);
							}
							else
								break;
						}
					}
					else {
						sql_HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
					}

					// Process data  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						SQLCancel(hstmt); // �ڵ�ĵ��
						SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					}

					SQLDisconnect(hdbc);
				}

				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
	cout << "database access complete. \n";
	DBCount = vec_database.size();
	cout << DBCount << endl;
}

void sql_update_database(int keyid, short x, short y, short hp, short exp, short level)
{
	SQLHENV henv;		// �����ͺ��̽��� �����Ҷ� ����ϴ� �ڵ�
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0; // sql��ɾ �����ϴ� �ڵ�
	SQLRETURN retcode;  // sql��ɾ ������ ���������� ��������
	SQLWCHAR query[1024];
	wsprintf(query, L"UPDATE player_table SET c_px = %d, c_py = %d, c_hp = %d, c_exp = %d, c_level = %d WHERE c_key = %d", x, y, hp, exp, level, keyid);


	setlocale(LC_ALL, "korean"); // �����ڵ� �ѱ۷� ��ȯ
	//std::wcout.imbue(std::locale("korean"));

	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0); // ODBC�� ����

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0); // 5�ʰ� ���� 5�ʳѾ�� Ÿ�Ӿƿ�

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2015184024_gameserver", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
				//retcode = SQLConnect(hdbc, (SQLWCHAR*)L"jys_gameserver", SQL_NTS, (SQLWCHAR*)NULL, SQL_NTS, NULL, SQL_NTS);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt); // SQL��ɾ� ������ �ѵ�

					retcode = SQLExecDirect(hstmt, (SQLWCHAR *)query, SQL_NTS); // ������
					//retcode = SQLExecDirect(hstmt, (SQLWCHAR *)L"EXEC select_highlevel 90", SQL_NTS); // 90���� �̻� ��������

					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						cout << "DataBase update success";
					}
					else {
						sql_HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
					}

					// Process data  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						SQLCancel(hstmt); // �ڵ�ĵ��
						SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					}

					SQLDisconnect(hdbc);
				}

				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
}

void sql_insert_database(int key, char id[], char name[], short level, short x, short y, short hp, short exp, short att)
{
	SQLHENV henv;		// �����ͺ��̽��� �����Ҷ� ����ϴ� �ڵ�
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0; // sql��ɾ �����ϴ� �ڵ�
	SQLRETURN retcode;  // sql��ɾ ������ ���������� ��������
	SQLWCHAR query[1024];
	SQLWCHAR tempid[10];
	SQLWCHAR tempname[10];

	MultiByteToWideChar(CP_ACP, 0, id, strlen(id), tempid, lstrlen(tempid));
	//MultiByteToWideChar(CP_ACP, 0, name, strlen(name), tempname, lstrlen(tempname));

	wsprintf(query, L"INSERT INTO player_table (c_key, c_id, c_name, c_level, c_px, c_py, c_hp, c_exp, c_att) VALUES (%d, '%s', '%s', %d, %d, %d, %d, %d, %d)"
		, key, tempid, tempid, level, x, y, hp, exp, att);


	setlocale(LC_ALL, "korean"); // �����ڵ� �ѱ۷� ��ȯ
	//std::wcout.imbue(std::locale("korean"));

	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0); // ODBC�� ����

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0); // 5�ʰ� ���� 5�ʳѾ�� Ÿ�Ӿƿ�

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2015184024_gameserver", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
				//retcode = SQLConnect(hdbc, (SQLWCHAR*)L"jys_gameserver", SQL_NTS, (SQLWCHAR*)NULL, SQL_NTS, NULL, SQL_NTS);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt); // SQL��ɾ� ������ �ѵ�

					retcode = SQLExecDirect(hstmt, (SQLWCHAR *)query, SQL_NTS); // ������
					//retcode = SQLExecDirect(hstmt, (SQLWCHAR *)L"EXEC select_highlevel 90", SQL_NTS); // 90���� �̻� ��������

					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						cout << "insert success";
					}
					else {
						sql_HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
					}

					// Process data  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						SQLCancel(hstmt); // �ڵ�ĵ��
						SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					}

					SQLDisconnect(hdbc);
				}

				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
}
