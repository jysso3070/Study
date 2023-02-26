#pragma once

#include <iostream> 
#include <map> 
#include <thread>
#include <set>
#include <vector>
#include <algorithm>
#include <iterator>
#include <list>
#include <mutex>
#include <fstream>
#include <chrono>
#include <queue>
#include <random>
#include <concurrent_unordered_map.h>

#include <WS2tcpip.h> 
#pragma comment(lib, "Ws2_32.lib") 
#pragma comment(lib, "lua53.lib")
#include "protocol.h"

using namespace std;

//sql
#define UNICODE  
#include <sqlext.h>  

enum EVENT_TYPE {
	EV_RECV, EV_SEND, EV_MOVE, EV_PLAYER_MOVE_NOTIFY, EV_MOVE_TARGET,
	EV_ATTACK, EV_HEAL, EV_PLAYER_RESPAWN, EV_NPC_RESPAWN, EV_MOVE_COOLTIME
};

struct DATABASE
{
	int idKey;
	char idName[10];
	short x, y;
	short hp;
	short exp;
	short level;
};

struct OVER_EX {
	WSAOVERLAPPED over;
	WSABUF wsabuf[1];
	char buffer[MAX_BUFFER];
	EVENT_TYPE event_type;
};

struct EVENT
{
	int obj_id;
	int target_obj;
	int event_type;
	chrono::high_resolution_clock::time_point wakeup_time;
	constexpr bool operator < (const EVENT& left) const {
		return wakeup_time > left.wakeup_time;
	}
};

#define start_posX	397
#define start_posY	69

static default_random_engine dre;
static uniform_int_distribution<> uidall(0, 799);
static uniform_int_distribution<> uidx_1(310, 470);
static uniform_int_distribution<> uidy_1(1, 55);
static uniform_int_distribution<> uidx_2(1, 280);
static uniform_int_distribution<> uidy_2(290, 348);
static uniform_int_distribution<> uid_hotspot(10, 30);