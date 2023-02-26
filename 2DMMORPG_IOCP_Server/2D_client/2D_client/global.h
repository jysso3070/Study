#pragma once
#include "framework.h"
 

// define
#define WINDOWSize_X 650
#define WINDOWSize_Y 660
#define MAX_ID_LEN	10


struct PLAYER_INFO
{
	int id;
	short x, y;
	short hp;
	short att;
	short exp;
	short Lv = 1;
	//bool login;
	bool isNear;
};

struct NPC_INFO
{
	int id;
	char obj_type;
	short x, y;
	short hp;
	short att;
	char chat[100];
	bool isNear;
	bool render_chat;
	chrono::steady_clock::time_point chat_time;
};

struct CHAT
{
	int id;
	char name[10];
	char chat[100];
};

struct BATTLE_MESS
{
	int hitter_id;
	int damaged_id;
	short damage;
};

// 체스판 비트맵 이미지 출력
//memdc = CreateCompatibleDC(BackBuffer);
//SelectObject(memdc, hBitmap);
//StretchBlt(BackBuffer, 0, 0, 400, 400, memdc, 5, 5, 521, 522, SRCCOPY);	//테두리 잘라냄
//DeleteDC(memdc);



// 클라이언트 시점안에들어온 플레이어 화면에 보이게하기
//for (int i = 0; i < MAX_PLAYER; ++i) {	// 다른플레이어 위치
//	if (PlayerArr[i].login == true)
//	{
//		if (PlayerArr[i].x > (x - 220) && PlayerArr[i].x < (x + 220) && PlayerArr[i].y >(y - 220) && PlayerArr[i].y < (y + 220))
//		{
//			int px = PlayerArr[i].x - transX;
//			int py = PlayerArr[i].y - transY;
//
//			HBRUSH hBrush = CreateSolidBrush(RGB(200, 0, 0));
//			HBRUSH oldBrush = (HBRUSH)SelectObject(BackBuffer, hBrush);
//			Rectangle(BackBuffer, px - 10, py - 10, px + 10, py + 10);
//			SelectObject(BackBuffer, oldBrush);
//			DeleteObject(hBrush);
//		}
//	}
//}