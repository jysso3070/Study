#include <iostream>
#include <map>
#include <thread>
#include <set>
#include <mutex>
#include <chrono>
#include <queue>


extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

using namespace std;
using namespace chrono;
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "lua53.lib")

#include "protocol.h"

#define MAX_BUFFER        1024
constexpr auto VIEW_RANGE = 3;

enum EVENT_TYPE { EV_RECV, EV_SEND, EV_MOVE, EV_PLAYER_MOVE_NOTIFY, EV_MOVE_TARGET, EV_ATTACK, EV_HEAL };

struct OVER_EX {
	WSAOVERLAPPED over;
	WSABUF	wsabuf[1];
	char	net_buf[MAX_BUFFER];
	EVENT_TYPE	event_type;
};

struct SOCKETINFO
{
	OVER_EX	recv_over;
	SOCKET	socket;
	int		id;

	bool is_active;
	short	x, y;
	set <int> near_id;
	mutex near_lock;
	lua_State *L;
	mutex vm_lock;
};

struct EVENT {
	int obj_id;
	high_resolution_clock::time_point wakeup_time;
	int event_type;
	int target_obj;

	constexpr bool operator < (const EVENT& left) const
	{
		return wakeup_time > left.wakeup_time;
	}
};

priority_queue <EVENT> timer_queue;
mutex  timer_lock;

map <int, SOCKETINFO *> clients;
HANDLE	g_iocp;

int new_user_id = 0;

void lua_error(lua_State *L, const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
	lua_close(L);
	exit(EXIT_FAILURE);
	while (true);
}

void error_display(const char *msg, int err_no)
{
	WCHAR *lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	cout << msg;
	wcout << L"에러 " << lpMsgBuf << endl;
	while (true);
	LocalFree(lpMsgBuf);
}

void add_timer(EVENT &ev)
{
	timer_lock.lock();
	timer_queue.push(ev);
	timer_lock.unlock();
}

bool Is_NPC(int id)
{
	return id >= NPC_ID_START;
}

bool Is_Active(int npc_id)
{
	return clients[npc_id]->is_active;
}

bool is_near(int a, int b)
{
	if (VIEW_RANGE < abs(clients[a]->x - clients[b]->x)) return false;
	if (VIEW_RANGE < abs(clients[a]->y - clients[b]->y)) return false;
	return true;
}

bool is_near_NPC(int a, int b)
{
	if (VIEW_RANGE + VIEW_RANGE < abs(clients[a]->x - clients[b]->x)) return false;
	if (VIEW_RANGE + VIEW_RANGE < abs(clients[a]->y - clients[b]->y)) return false;
	return true;
}

void send_packet(int id, void *buff)
{
	char *packet = reinterpret_cast<char *>(buff);
	int packet_size = packet[0];
	OVER_EX *send_over = new OVER_EX;
	memset(send_over, 0x00, sizeof(OVER_EX));
	send_over->event_type = EV_SEND;
	memcpy(send_over->net_buf, packet, packet_size);
	send_over->wsabuf[0].buf = send_over->net_buf;
	send_over->wsabuf[0].len = packet_size;
	int ret = WSASend(clients[id]->socket, send_over->wsabuf, 1, 0, 0, &send_over->over, 0);
	if (0 != ret) {
		int err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no)
			error_display("WSARecv Error :", err_no);
	}
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
	packet.x = clients[new_id]->x;
	packet.y = clients[new_id]->y;
	send_packet(client, &packet);

	if (client == new_id) return;
	lock_guard<mutex>lg{ clients[client]->near_lock };
	clients[client]->near_id.insert(new_id);
}

void send_pos_packet(int client, int mover)
{
	sc_packet_pos packet;
	packet.id = mover;
	packet.size = sizeof(packet);
	packet.type = SC_POS;
	packet.x = clients[mover]->x;
	packet.y = clients[mover]->y;

	clients[client]->near_lock.lock();
	if (0 != clients[client]->near_id.count(mover)) {
		clients[client]->near_lock.unlock();
		send_packet(client, &packet);
	}
	else {
		clients[client]->near_lock.unlock();
		send_put_player_packet(client, mover);
	}
}

void send_remove_player_packet(int client, int leaver)
{
	sc_packet_remove_player packet;
	packet.id = leaver;
	packet.size = sizeof(packet);
	packet.type = SC_REMOVE_PLAYER;
	send_packet(client, &packet);

	lock_guard<mutex>lg{ clients[client]->near_lock };
	clients[client]->near_id.erase(leaver);
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


bool is_near_id(int player, int other)
{
	lock_guard <mutex> gl{ clients[player]->near_lock };
	return (0 != clients[player]->near_id.count(other));
}

void ProcessPacket(int id, void *buff)
{
	char *packet = reinterpret_cast<char *>(buff);
	short x = clients[id]->x;
	short y = clients[id]->y;
	clients[id]->near_lock.lock();
	auto old_vl = clients[id]->near_id;
	clients[id]->near_lock.unlock();
	switch (packet[1]) {
	case CS_UP: if (y > 0) y--;
		break;
	case CS_DOWN: if (y < WORLD_HEIGHT - 1) y++;
		break;
	case CS_LEFT: if (x > 0) x--;
		break;
	case CS_RIGHT: if (x < WORLD_WIDTH - 1) x++;
		break;
	default: cout << "Invalid Packet Type Error\n";
		while (true);
	}
	clients[id]->x = x;
	clients[id]->y = y;

	set <int> new_vl;
	for (auto &cl : clients) {
		int other = cl.second->id;
		if (id == other) continue;
		if (true == is_near_NPC(id, other)) {
			if (true == Is_NPC(other) && (false == Is_Active(other))) {
				clients[other]->is_active = true;
				EVENT ev{ other, high_resolution_clock::now() + 1s, EV_MOVE, 0 };
				add_timer(ev);
			}
			if (true == is_near(id, other)) {
				new_vl.insert(other);
				if (true == Is_NPC(other)) {
					OVER_EX *over_ex = new OVER_EX;
					over_ex->event_type = EV_PLAYER_MOVE_NOTIFY;
					*(int *)(over_ex->net_buf) = id;   // netbuf가 가르키는포인터를 int포인터를 가르키게한다
					PostQueuedCompletionStatus(g_iocp, 1, other, &over_ex->over);
				}
			}
		}
	}


	send_pos_packet(id, id);
	for (auto cl : old_vl) {
		if (0 != new_vl.count(cl)) {
			if (false == Is_NPC(cl))
				send_pos_packet(cl, id);
		}
		else
		{
			send_remove_player_packet(id, cl);
			if (false == Is_NPC(cl))
				send_remove_player_packet(cl, id);
		}
	}
	for (auto cl : new_vl) {
		if (0 == old_vl.count(cl)) {
			send_put_player_packet(id, cl);
			if (false == Is_NPC(cl))
				send_put_player_packet(cl, id);
		}
	}
}

void do_random_move(int npc_id)
{
	if (0 == clients.count(npc_id)) {
		cout << "NPC: " << npc_id << " does not EXIST!\n";
		while (true);
	}

	if (false == Is_NPC(npc_id)) {
		cout << "ID :" << npc_id << " is not NPC!!\n";
		while (true);
	}

	bool player_exists = false;
	for (int i = 0; i < NPC_ID_START; i++) {
		if (0 == clients.count(i)) continue;
		if (true == is_near_NPC(i, npc_id)) {
			player_exists = true;
			break;
		}
	}
	if (false == player_exists) {
		clients[npc_id]->is_active = false;
		return;
	}

	SOCKETINFO *npc = clients[npc_id];
	int x = npc->x;
	int y = npc->y;
	set <int> old_view_list;
	for (auto &obj : clients) {
		if (true == is_near(npc->id, obj.second->id))
			old_view_list.insert(obj.second->id);
	}
	switch (rand() % 4) {
	case 0: if (y > 0) y--; break;
	case 1: if (y < (WORLD_HEIGHT - 1)) y++; break;
	case 2: if (x > 0) x--; break;
	case 3: if (x < (WORLD_WIDTH - 1)) x++; break;
	}
	npc->x = x;
	npc->y = y;
	set <int> new_view_list;
	for (auto &obj : clients) {
		if (true == is_near(npc->id, obj.second->id))
			new_view_list.insert(obj.second->id);
	}
	for (auto &pc : clients) {
		if (true == Is_NPC(pc.second->id)) continue;
		if (false == is_near(pc.second->id, npc->id)) continue;
		send_pos_packet(pc.second->id, npc->id);
	}
	EVENT new_ev{ npc_id, high_resolution_clock::now() + 1s, EV_MOVE, 0 };
	add_timer(new_ev);
}

void do_worker()
{
	while (true) {
		DWORD num_byte;
		ULONGLONG key64;
		PULONG_PTR p_key = &key64;
		WSAOVERLAPPED *p_over;

		GetQueuedCompletionStatus(g_iocp, &num_byte, p_key, &p_over, INFINITE);
		unsigned int key = static_cast<unsigned>(key64);
		SOCKET client_s = clients[key]->socket;
		if (num_byte == 0) {
			closesocket(client_s);
			clients.erase(key);
			for (auto &cl : clients) {
				if (false == Is_NPC(cl.first))
					send_remove_player_packet(cl.first, key);
			}
			continue;
		}  // 클라이언트가 closesocket을 했을 경우		
		OVER_EX *over_ex = reinterpret_cast<OVER_EX *> (p_over);

		if (EV_RECV == over_ex->event_type) {
			ProcessPacket(key, over_ex->net_buf);

			DWORD flags = 0;
			memset(&over_ex->over, 0x00, sizeof(WSAOVERLAPPED));
			WSARecv(client_s, over_ex->wsabuf, 1, 0, &flags, &over_ex->over, 0);
		}
		else if (EV_SEND == over_ex->event_type) {
			delete over_ex;
		}
		else if (EV_MOVE == over_ex->event_type) {
			do_random_move(key);
			delete over_ex;
		}
		else if (EV_PLAYER_MOVE_NOTIFY == over_ex->event_type) {
			int player_id = *(int *)(over_ex->net_buf);

			clients[key]->vm_lock.lock();
			lua_State *L = clients[key]->L;
			lua_getglobal(L, "event_player_move_notify");
			lua_pushnumber(L, player_id);
			lua_pushnumber(L, clients[player_id]->x);
			lua_pushnumber(L, clients[player_id]->y);
			lua_pcall(L, 3, 0, 0);
			clients[key]->vm_lock.unlock();
			//lua_pop(L, 4);

		}
		else {
			cout << "Unknown Event Type :" << over_ex->event_type << endl;
			while (true);
		}
	}
}

int lua_get_x_position(lua_State *L)
{
	int npc_id = (int)lua_tonumber(L, -1);
	lua_pop(L, 2);
	int x = clients[npc_id]->x;
	lua_pushnumber(L, x);
	cout << "DD\n";
	return 1;
}

int lua_get_y_position(lua_State *L)
{
	int npc_id = (int)lua_tonumber(L, -1);
	lua_pop(L, 2);
	int y = clients[npc_id]->y;
	lua_pushnumber(L, y);
	return 1;
}

int lua_send_chat_packet(lua_State *L)
{
	int player_id = (int)lua_tonumber(L, -3);
	int chatter_id = (int)lua_tonumber(L, -2);
	char *mess = (char*)lua_tostring(L, -1);
	lua_pop(L, 4);
	send_chat_packet(player_id, chatter_id, mess);
	return 0;
}

void Create_NPC()
{
	cout << "initialize NPC \n";
	for (int npc_id = NPC_ID_START; npc_id < NPC_ID_START + NUM_NPC; ++npc_id) {
		clients[npc_id] = new SOCKETINFO;
		clients[npc_id]->id = npc_id;
		clients[npc_id]->x = rand() % WORLD_WIDTH;
		clients[npc_id]->y = rand() % WORLD_WIDTH;
		clients[npc_id]->socket = -1;
		clients[npc_id]->is_active = false;
		lua_State *L = clients[npc_id]->L = luaL_newstate();
		luaL_openlibs(L);
		luaL_loadfile(L, "monster.lua");
		lua_pcall(L, 0, 0, 0);
		/*if (0 != lua_pcall(L, 0, 0, 0)) {
			lua_error(L, "error running function : %s\n", lua_tostring(L, -1));
		}*/
		lua_getglobal(L, "set_npc_id");
		lua_pushnumber(L, npc_id);
		lua_pcall(L, 1, 0, 0);
		lua_register(L, "API_get_x_position", lua_get_x_position);
		lua_register(L, "API_get_y_position", lua_get_y_position);
		lua_register(L, "API_send_chat_packet", lua_send_chat_packet);
	}
	cout << "NPC initialization finished \n";
}

void do_ai()
{
	int count = 0;
	while (true) {
		auto ai_start = high_resolution_clock::now();
		cout << "AI Loop %d" << count++ << endl;
		for (auto &npc : clients) {
			if (false == Is_NPC(npc.second->id)) continue;

			bool player_exists = false;
			for (int i = 0; i < NPC_ID_START; i++) {
				if (0 == clients.count(i)) continue;
				if (true == is_near_NPC(i, npc.second->id)) {
					player_exists = true;
					break;
				}
			}

			if (false == player_exists) continue;

			int x = npc.second->x;
			int y = npc.second->y;
			set <int> old_view_list;
			for (auto &obj : clients) {
				if (true == is_near(npc.second->id, obj.second->id))
					old_view_list.insert(obj.second->id);
			}
			switch (rand() % 4) {
			case 0: if (y > 0) y--; break;
			case 1: if (y < (WORLD_HEIGHT - 1)) y++; break;
			case 2: if (x > 0) x--; break;
			case 3: if (x < (WORLD_WIDTH - 1)) x++; break;
			}
			npc.second->x = x;
			npc.second->y = y;
			set <int> new_view_list;
			for (auto &obj : clients) {
				if (true == is_near(npc.second->id, obj.second->id))
					new_view_list.insert(obj.second->id);
			}
			for (auto &pc : clients) {
				if (true == Is_NPC(pc.second->id)) continue;
				if (false == is_near(pc.second->id, npc.second->id)) continue;
				send_pos_packet(pc.second->id, npc.second->id);
			}
		}
		auto ai_end = high_resolution_clock::now();
		this_thread::sleep_for(1s - (ai_end - ai_start));
	}
}

void do_timer()
{
	while (true) {
		timer_lock.lock();
		while (true == timer_queue.empty()) {
			timer_lock.unlock();
			this_thread::sleep_for(10ms);
			timer_lock.lock();
		}
		const EVENT &ev = timer_queue.top();
		if (ev.wakeup_time > high_resolution_clock::now()) {
			timer_lock.unlock();
			this_thread::sleep_for(10ms);
			continue;
		}
		EVENT p_ev = ev;
		timer_queue.pop();
		timer_lock.unlock();

		OVER_EX *over_ex = new OVER_EX;
		over_ex->event_type = EV_MOVE;
		PostQueuedCompletionStatus(g_iocp, 1, p_ev.obj_id, &over_ex->over);
	}
}

int main()
{
	wcout.imbue(std::locale("korean"));

	Create_NPC();

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

	g_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	thread worker_thread{ do_worker };
	thread worker_thread2{ do_worker };
	thread worker_thread3{ do_worker };
	//	thread ai_thread{ do_ai };
	thread timer_thread{ do_timer };
	while (true) {
		clientSocket = accept(listenSocket, (struct sockaddr *)&clientAddr, &addrLen);
		int user_id = new_user_id++;
		//clients[user_id] = new SOCKETINFO;
		SOCKETINFO *new_player = new SOCKETINFO;
		//memset(clients[user_id], 0, sizeof(SOCKETINFO));
		new_player->id = user_id;
		new_player->socket = clientSocket;
		new_player->recv_over.wsabuf[0].len = MAX_BUFFER;
		new_player->recv_over.wsabuf[0].buf = new_player->recv_over.net_buf;
		new_player->recv_over.event_type = EV_RECV;
		new_player->x = rand() % WORLD_WIDTH;
		new_player->y = rand() % WORLD_HEIGHT;
		clients[user_id] = new_player;

		CreateIoCompletionPort(reinterpret_cast<HANDLE>(clientSocket), g_iocp, user_id, 0);

		send_login_ok_packet(user_id);
		
		for (auto &cl : clients) {
			int other_player = cl.first;
			if ((true == Is_NPC(other_player))
				&& (false == Is_Active(other_player))
				&& (true == is_near_NPC(other_player, user_id))) {
				clients[other_player]->is_active = true;
				EVENT ev{ other_player, high_resolution_clock::now() + 1s, EV_MOVE, 0 };
				add_timer(ev);
			}
			if (true == is_near(other_player, user_id)) {
				if (false == Is_NPC(other_player))
					send_put_player_packet(other_player, user_id);
				if (other_player != user_id) {
					send_put_player_packet(user_id, other_player);
				}
			}
		}
		memset(&clients[user_id]->recv_over.over, 0, sizeof(clients[user_id]->recv_over.over));
		flags = 0;
		int ret = WSARecv(clientSocket, clients[user_id]->recv_over.wsabuf, 1, NULL,
			&flags, &(clients[user_id]->recv_over.over), NULL);
		if (0 != ret) {
			int err_no = WSAGetLastError();
			if (WSA_IO_PENDING != err_no)
				error_display("WSARecv Error :", err_no);
		}
	}
	worker_thread.join();
	//	ai_thread.join();
	timer_thread.join();
	closesocket(listenSocket);
	WSACleanup();
}

