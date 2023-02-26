#pragma once

// define

#define MAP_SCALE_X	800
#define MAP_SCALE_Y	800

// server
#define MAX_BUFFER 1024
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 3500


// client to server
#define KEY_LEFT	4
#define KEY_RIGHT	6
#define KEY_UP		8
#define KEY_DOWN	2
#define KEY_NULL	0
#define CS_REQUEST_LOGIN	11

// server to client
#define SC_LOGIN_OK			1
#define SC_PUT_PLAYER		2
#define SC_REMOVE_PLAYER	3
#define SC_POS				4
#define SC_SEND_ID			5



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
	short x, y;
};

struct sc_packet_remove_player {
	char size;
	char type;
	int id;
};

struct cs_packet_key
{
	char size;
	char cKey;
};

struct cs_packet_request_login
{
	char size;
	char type;
	char loginid[10];
	int id;
};

#pragma pack (pop) 