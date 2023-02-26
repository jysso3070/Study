#pragma once

// define
#define MAX_OBJECT	50000
#define MAP_SCALE_X	800
#define MAP_SCALE_Y	800
#define MAX_CHAT	100

// obj type
#define TYPE_PLAYER			1
#define TYPE_PEACE_MONSTER	2
#define TYPE_WAR_MONSTER	3

#define NPC_ID_START	20100
#define NUM_NPC			10000

// server
#define MAX_BUFFER 1024
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 3500


// client to server
#define CS_LEFT			4
#define CS_RIGHT		6
#define CS_UP			8
#define CS_DOWN			2
#define CS_REQUEST_LOGIN	11
#define CS_ATTACK			12
#define CS_CHAT				13

// server to client
#define SC_LOGIN_OK			1
#define SC_PUT_PLAYER		2
#define SC_REMOVE_PLAYER	3
#define SC_POS				4
#define SC_SEND_ID			5
#define SC_CHAT				6
#define SC_STAT_CHANGE		7
#define SC_PLAYER_DEAD		8
#define SC_LOGIN_FAIL		9
#define SC_BATTLE_MESS		10


#pragma pack(push ,1)

struct sc_packet_pos
{
	char size;
	char type;
	int id;
	short x, y;
};

struct sc_packet_send_id // 소켓연결 성공시 클라이언트 고유 id키값 부여
{
	char size;
	char type;
	int id;
};

struct sc_packet_login_ok
{
	char size;
	char type;
	int id;
	short x, y;
};

struct sc_packet_put_player	// 플레이어 랜더링 정보
{
	char size;
	char type;
	int id;
	char obj_type;
	short x, y;
	short hp;
	short att;
	short exp;
	short level;
};

struct sc_packet_remove_player {
	char size;
	char type;
	int id;
};

struct sc_packet_chat {
	char size;
	char type;
	int	 id;
	char name[10];
	char chat[MAX_CHAT];
};

struct sc_packet_stat_change {
	char size;
	char type;
	short hp;
	short exp;
	short level;
};

struct sc_packet_player_dead {
	char size;
	char type;
};

struct sc_packet_login_fail {
	char size;
	char type;
};

struct sc_packet_battle_mess {
	char size;
	char type;
	int hitterID;
	int damagedID;
	short damage;
};

// client to server
struct cs_packet_up
{
	char size;
	char type;
};
struct cs_packet_down
{
	char size;
	char type;
};
struct cs_packet_left
{
	char size;
	char type;
};
struct cs_packet_right
{
	char size;
	char type;
};

struct cs_packet_request_login
{
	char size;
	char type;
	char loginid[10];
	int id;
};

struct cs_packet_attack
{
	char size;
	char type;
	int id;
};

struct cs_packet_chat {
	char size;
	char type;
	int	 id;
	char name[10];
	char chat[MAX_CHAT];
};

#pragma pack (pop) 