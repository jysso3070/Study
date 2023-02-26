// 2D_client.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "framework.h"
#include "2D_client.h"
#include "global.h"
#include "../../iocp_Server/iocp_Server/protocol.h"
//#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console")


#define MAX_LOADSTRING 100

// define
#define MAX_PLAYER 20

// 비동기io
#define	WM_SOCKET			WM_USER + 1

// 전역 함수
void Render_Board(HDC BackBuffer, int x, int y);
void PacketProccess(void * buf);

// 전역 변수:
constexpr int FOV = 11;	// 플레이어 시야 범위 21X21
constexpr int blockSize = 25;
int WorldMap_Arr[MAP_SCALE_Y][MAP_SCALE_X];	// 월드맵 배열 한칸에 40픽셀
int myID; //클라이언트  ID
char buffer[MAX_BUFFER];
bool loginOK = false;
bool gamestart = false;
bool chatting = false;
bool deadFlag = false;
bool loginFail = false;
chrono::steady_clock::time_point dead_time;
string filepath = "../../map1.txt";

PLAYER_INFO MyInfo{};
PLAYER_INFO PlayerArr[MAX_OBJECT];
unordered_map<int, NPC_INFO> map_NPCs;
list<CHAT> chatList;
list<BATTLE_MESS> battle_mess_list;
mutex chatlock;
WSABUF recv_buf; // WSARecv를 사용하기위해 WSABUF 사용

void LoadMap()
{
	ifstream in(filepath);
	char c;
	for (int i = 0; i < MAP_SCALE_Y; ++i) {
		for (int j = 0; j < MAP_SCALE_X; ++j) {
			in >> c;
			WorldMap_Arr[i][j] = atoi(&c);
		}
	}
}

void Render_LoginScene(HDC BackBuffer, char* ID)
{
	Rectangle(BackBuffer, 0, 0, WINDOWSize_X, WINDOWSize_Y);
	//Rectangle(BackBuffer, WINDOWSize_X/2, WINDOWSize_Y/2, 690, 55);
	TextOut(BackBuffer, WINDOWSize_X / 3, WINDOWSize_Y / 2, TEXT("input ID and press ENTER "), 25);
	TextOut(BackBuffer, WINDOWSize_X / 3, WINDOWSize_Y / 2 + 15, TEXT("ID : "), 5);
	TextOutA(BackBuffer, WINDOWSize_X / 3 + 25, WINDOWSize_Y / 2 + 15, ID, strlen(ID));
}

void Render_Stat(HDC BackBuffer, char* id)
{
	Rectangle(BackBuffer, 525, 0, 650, 100);
	TextOut(BackBuffer, 527, 5, TEXT("ID:"), 3);
	TextOutA(BackBuffer, 547, 5, id, strlen(id));

	TCHAR strLv[10];
	wsprintf(strLv, TEXT("Lv: %d"), MyInfo.Lv);
	TextOut(BackBuffer, 527, 25, strLv, lstrlen(strLv));
	TCHAR strHP[10];
	wsprintf(strHP, TEXT("HP: %d"), MyInfo.hp);
	TextOut(BackBuffer, 527, 45, strHP, lstrlen(strHP));
	TCHAR strEXP[10];
	wsprintf(strEXP, TEXT("EXP: %d"), MyInfo.exp);
	TextOut(BackBuffer, 527, 65, strEXP, lstrlen(strEXP));
}


void Render_ChatBox(HDC BackBuffer, char* chat)
{
	Rectangle(BackBuffer, 525, 95, 650, 500);

	if (true == chatting) {
		TextOut(BackBuffer, 530, 475, TEXT("CHAT : "), 7);
		TextOutA(BackBuffer, 570, 475, chat, strlen(chat));
	}
	short idx = 0;
	for (auto chat : chatList) {
		char temp[MAX_CHAT];
		strcpy_s(temp, chat.chat);
		if (chat.id == MyInfo.id) { //내 채팅
			TextOut(BackBuffer, 530, 450 - (idx * 13), TEXT("ME:"), 3);
			TextOutA(BackBuffer, 560, 450 - (idx * 13), temp, strlen(temp));
		}
		else {
			char name[MAX_CHAT];
			strcpy_s(name, chat.name);
			char str[2] = ":";
			strcat_s(name, str);
			strcat_s(name, temp);
			TextOutA(BackBuffer, 530, 450 - (idx * 13), name, strlen(name));
			//TextOutA(BackBuffer, 570, 450 - (idx * 13), temp, strlen(temp));
		}
		++idx;
	}
}

void Render_Dead_Message(HDC BackBuffer)
{
	Rectangle(BackBuffer, 0, 0, WINDOWSize_X, WINDOWSize_Y);
	HFONT myFont = CreateFont(30, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, TEXT("궁서"));
	HFONT oldFont = (HFONT)SelectObject(BackBuffer, myFont);
	TextOut(BackBuffer, WINDOWSize_X / 4, WINDOWSize_Y / 2, TEXT("YOU DIE"), 7);
	TextOut(BackBuffer, WINDOWSize_X / 4, WINDOWSize_Y / 2 + 50, TEXT("WAIT FOR 5 SECONDS"), 19);

	SelectObject(BackBuffer, oldFont);
	DeleteObject(myFont);
}

void Render_Battle_Message(HDC BackBuffer)
{
	//Rectangle(BackBuffer, 0 ,580, WINDOWSize_X, WINDOWSize_Y);
	//TextOut(BackBuffer, 10 , 550, TEXT("battle mess"), 11);
	short idx = 0;
	auto templist = battle_mess_list;
	for (auto mess : templist) {
		char str[100];
		if (mess.hitter_id == MyInfo.id) {
			sprintf_s(str, "%d번 몬스터에게 %d 의 데미지를 입혔다. ", mess.damaged_id, mess.damage);
			TextOutA(BackBuffer, 10, 565 - (idx * 20), str, strlen(str));
		}
		else if (mess.damaged_id == MyInfo.id) {
			sprintf_s(str, "%d번 전쟁몬스터에게 %d의 데미지를 입었다! ", mess.hitter_id, mess.damage);
			TextOutA(BackBuffer, 10, 565 - (idx * 20), str, strlen(str));
		}
		++idx;
	}
}

void Render_Board(HDC BackBuffer, int x, int y)
{
	//Rectangle(BackBuffer, )


	x *= blockSize;	// 게임내 블럭사이즈에 맞춰서 좌표 변환
	y *= blockSize;
	int diffOfLength = (FOV - 1) * blockSize;	// 서버와 클라이언트에서 좌표차이
	for (int i = 0; i < MAP_SCALE_Y; ++i) {
		for (int j = 0; j < MAP_SCALE_X; ++j) {
			if (x / blockSize - FOV < j && j < x / blockSize + FOV && y / blockSize - FOV < i && i < y / blockSize + FOV)
			{
				if (WorldMap_Arr[i][j] == 0) // 노말지형 연두색
				{
					HBRUSH hBrush = CreateSolidBrush(RGB(116, 223, 89));
					HBRUSH oldBrush = (HBRUSH)SelectObject(BackBuffer, hBrush);

					Rectangle(BackBuffer, j * blockSize + diffOfLength - x, i * blockSize + diffOfLength - y,
						(j + 1) * blockSize + diffOfLength - x, (i + 1) * blockSize + diffOfLength - y);

					SelectObject(BackBuffer, oldBrush);
					DeleteObject(hBrush);
				}
				if (WorldMap_Arr[i][j] == 1) // 나무 갈색
				{
					HBRUSH hBrush = CreateSolidBrush(RGB(150, 30, 30));
					HBRUSH oldBrush = (HBRUSH)SelectObject(BackBuffer, hBrush);

					Rectangle(BackBuffer, j * blockSize + diffOfLength - x - 3, i * blockSize + diffOfLength - y - 3,
						(j + 1) * blockSize + diffOfLength - x + 3, (i + 1) * blockSize + diffOfLength - y + 3);

					SelectObject(BackBuffer, oldBrush);
					DeleteObject(hBrush);
				}
				if (WorldMap_Arr[i][j] == 2) // 바다지형 파란색
				{
					HBRUSH hBrush = CreateSolidBrush(RGB(30, 30, 100));
					HBRUSH oldBrush = (HBRUSH)SelectObject(BackBuffer, hBrush);

					Rectangle(BackBuffer, j * blockSize + diffOfLength - x, i * blockSize + diffOfLength - y,
						(j + 1) * blockSize + diffOfLength - x, (i + 1) * blockSize + diffOfLength - y);

					SelectObject(BackBuffer, oldBrush);
					DeleteObject(hBrush);
				}
				if (WorldMap_Arr[i][j] == 3) // 벽 회색
				{
					HBRUSH hBrush = CreateSolidBrush(RGB(45, 45, 45));
					HBRUSH oldBrush = (HBRUSH)SelectObject(BackBuffer, hBrush);

					Rectangle(BackBuffer, j * blockSize + diffOfLength - x, i * blockSize + diffOfLength - y,
						(j + 1) * blockSize + diffOfLength - x, (i + 1) * blockSize + diffOfLength - y);

					SelectObject(BackBuffer, oldBrush);
					DeleteObject(hBrush);
				}
				if (i == 0 && j % 10 == 0)	// 블럭숫자 표시 가로
				{
					TCHAR str[10];
					wsprintf(str, TEXT("%d"), j + 1);
					TextOut(BackBuffer, j * blockSize + diffOfLength - x, i * blockSize + diffOfLength - y, str, lstrlen(str));
				}
				if (j == 0 && i % 10 == 0)	// 블럭숫자 표시 세로
				{
					TCHAR str[10];
					wsprintf(str, TEXT("%d"), i + 1);
					TextOut(BackBuffer, j * blockSize + diffOfLength - x, i * blockSize + diffOfLength - y, str, lstrlen(str));
				}
			}
		}
	}
	for (auto npc : map_NPCs) {	// 몬스터 npc
		if (npc.second.isNear == true && npc.second.obj_type == TYPE_PEACE_MONSTER) {
			// 평화 몬스터 npc 위치 하늘색
			float npcx = (npc.second.x * blockSize) - x + diffOfLength;
			float npcy = (npc.second.y * blockSize) - y + diffOfLength;
			HBRUSH hBrush = CreateSolidBrush(RGB(30, 250, 250));
			HBRUSH oldBrush = (HBRUSH)SelectObject(BackBuffer, hBrush);
			Rectangle(BackBuffer, npcx - 3, npcy - 3, npcx + blockSize + 3, npcy + blockSize + 3);
			SelectObject(BackBuffer, oldBrush);
			DeleteObject(hBrush);

			if (npc.second.chat_time + 1s > chrono::high_resolution_clock::now()) {
				char mess[10];
				strcpy_s(mess, map_NPCs[npc.first].chat);
				//TCHAR str[10];
				//wsprintf(str, TEXT("%s"), mess);
				TextOutA(BackBuffer, npcx - 5, npcy - 10, mess, strlen(mess));
			}
		}
		else if (npc.second.isNear == true && npc.second.obj_type == TYPE_WAR_MONSTER) {
			// 전쟁 몬스터 npc 위치 빨간색
			float npcx = (npc.second.x * blockSize) - x + diffOfLength;
			float npcy = (npc.second.y * blockSize) - y + diffOfLength;
			HBRUSH hBrush = CreateSolidBrush(RGB(250, 50, 50));
			HBRUSH oldBrush = (HBRUSH)SelectObject(BackBuffer, hBrush);
			Rectangle(BackBuffer, npcx - 3, npcy - 3, npcx + blockSize + 3, npcy + blockSize + 3);
			SelectObject(BackBuffer, oldBrush);
			DeleteObject(hBrush);
		}
	}

	for (int i = 0; i < MAX_OBJECT; ++i) {	// 다른플레이어 위치 주황색
		if (PlayerArr[i].isNear == true)
		{
			float px = (PlayerArr[i].x * blockSize) - x + diffOfLength;	// 위치 변환
			float py = (PlayerArr[i].y * blockSize) - y + diffOfLength;

			HBRUSH hBrush = CreateSolidBrush(RGB(250, 150, 30));
			HBRUSH oldBrush = (HBRUSH)SelectObject(BackBuffer, hBrush);
			Rectangle(BackBuffer, px - 3, py - 3, px + blockSize + 3, py + blockSize + 3);
			SelectObject(BackBuffer, oldBrush);
			DeleteObject(hBrush);
		}
	}




	// 화면 정중앙에 내 캐릭터 위치
	HBRUSH hBrush = CreateSolidBrush(RGB(250, 250, 30)); // 노랑
	HBRUSH oldBrush = (HBRUSH)SelectObject(BackBuffer, hBrush);
	Rectangle(BackBuffer, blockSize *(FOV - 1) - 3, blockSize *(FOV - 1) - 3, blockSize *FOV + 3, blockSize *FOV + 3);
	SelectObject(BackBuffer, oldBrush);
	DeleteObject(hBrush);
}

void ReadBuffer(SOCKET sock)
{
	int cur_packet_size = 0;
	int saved_packet_size = 0;

	DWORD recv_byte, ioflag = 0;
	WSARecv(sock, &recv_buf, 1, &recv_byte, &ioflag, NULL, NULL);

	char * temp = reinterpret_cast<char*>(buffer);

	while (recv_byte != 0)
	{
		if (cur_packet_size == 0) {
			cur_packet_size = temp[0];
		}
		if (recv_byte + saved_packet_size >= cur_packet_size) {
			memcpy(buffer + saved_packet_size, temp, cur_packet_size - saved_packet_size);
			PacketProccess(buffer);
			temp += cur_packet_size - saved_packet_size;
			recv_byte -= cur_packet_size - saved_packet_size;
			saved_packet_size = 0;
			cur_packet_size = 0;
		}
		else {
			memcpy(buffer + saved_packet_size, temp, recv_byte);
			saved_packet_size += recv_byte;
			recv_byte = 0;
		}
	}
}

void PacketProccess(void * buf)
{
	char* temp = reinterpret_cast<char*>(buf);

	switch (temp[1]) {
	case SC_SEND_ID:
	{
		sc_packet_send_id *id_packet = reinterpret_cast<sc_packet_send_id *>(buf);
		myID = id_packet->id;
		break;
	}
	case SC_LOGIN_OK:
	{
		sc_packet_login_ok *login_packet = reinterpret_cast<sc_packet_login_ok *>(buf);
		//loginOK = true;
		gamestart = true;
		break;
	}
	case SC_PUT_PLAYER:
	{
		sc_packet_put_player *put_player_packet = reinterpret_cast<sc_packet_put_player *>(buf);
		int id = put_player_packet->id;
		if (id == myID) {
			MyInfo.id = put_player_packet->id;
			MyInfo.x = put_player_packet->x;
			MyInfo.y = put_player_packet->y;
			MyInfo.hp = put_player_packet->hp;
			MyInfo.att = put_player_packet->att;
			MyInfo.exp = put_player_packet->exp;
			MyInfo.Lv = put_player_packet->level;
			loginOK = true;

			deadFlag = false;
			cout << "pu my\n";
		}
		else if (id < NPC_ID_START) { // 다른플레이어
			PlayerArr[id].id = id;
			//PlayerArr[id].login = true;
			PlayerArr[id].isNear = true;
			PlayerArr[id].x = put_player_packet->x;
			PlayerArr[id].y = put_player_packet->y;
			PlayerArr[id].hp = put_player_packet->hp;
			PlayerArr[id].att = put_player_packet->att;
		}
		else { // NPC
			NPC_INFO *new_npc = new NPC_INFO;
			new_npc->id = put_player_packet->id;
			new_npc->x = put_player_packet->x;
			new_npc->y = put_player_packet->y;
			new_npc->hp = put_player_packet->hp;
			new_npc->att = put_player_packet->att;
			new_npc->obj_type = put_player_packet->obj_type;
			new_npc->isNear = true;
			new_npc->render_chat = false;
			map_NPCs[id] = *new_npc;
		}
		//cout << "put" << endl;
		break;
	}
	case SC_REMOVE_PLAYER:
	{
		sc_packet_remove_player *remove_player_packet = reinterpret_cast<sc_packet_remove_player *>(buf);
		int id = remove_player_packet->id;
		//PlayerArr[id].login =false;
		if (id < NPC_ID_START) { // 다른 플레이어
			PlayerArr[id].isNear = false;
		}
		else { // NPC
			map_NPCs.erase(id);
		}

		break;
	}
	case SC_POS:
	{
		sc_packet_pos *pos_packet = reinterpret_cast<sc_packet_pos *>(buf);
		int id = pos_packet->id;
		if (id == myID) {
			MyInfo.x = pos_packet->x;
			MyInfo.y = pos_packet->y;
		}
		else if (id < NPC_ID_START) { // 다른플레이어
			PlayerArr[id].x = pos_packet->x;
			PlayerArr[id].y = pos_packet->y;
		}
		else { // npc
			short x = pos_packet->x;
			short y = pos_packet->y;
			map_NPCs[id].x = x;
			map_NPCs[id].y = y;
		}

		break;
	}
	case SC_CHAT:
	{
		sc_packet_chat *chat_packet = reinterpret_cast<sc_packet_chat *>(buf);
		int chatter_id = chat_packet->id;
		if (chatter_id >= NPC_ID_START) { //npc 채팅
			strcpy_s(map_NPCs[chatter_id].chat, chat_packet->chat);
			map_NPCs[chatter_id].render_chat = true;
			map_NPCs[chatter_id].chat_time = chrono::high_resolution_clock::now();
		}
		else {
			char temp[MAX_CHAT];
			strcpy_s(temp, chat_packet->chat);
			CHAT *new_chat = new CHAT;
			new_chat->id = chat_packet->id;
			strcpy_s(new_chat->name, chat_packet->name);
			strcpy_s(new_chat->chat, chat_packet->chat);
			chatList.emplace_front(*new_chat);
		}
		break;
	}
	case SC_STAT_CHANGE:
	{
		sc_packet_stat_change *stat_packet = reinterpret_cast<sc_packet_stat_change*>(buf);
		MyInfo.hp = stat_packet->hp;
		MyInfo.exp = stat_packet->exp;
		MyInfo.Lv = stat_packet->level;
		cout << "stat change\n";
		break;
	}
	case SC_BATTLE_MESS:
	{
		sc_packet_battle_mess *battle_mess_packet = reinterpret_cast<sc_packet_battle_mess*>(buf);
		int hitter_id = battle_mess_packet->hitterID;
		int damaged_id = battle_mess_packet->damagedID;
		short damage = battle_mess_packet->damage;
		BATTLE_MESS *new_battle_mess = new BATTLE_MESS;
		new_battle_mess->hitter_id = hitter_id;
		new_battle_mess->damaged_id = damaged_id;
		new_battle_mess->damage = damage;
		battle_mess_list.emplace_front(*new_battle_mess);
		if (5 < battle_mess_list.size()) {
			//chatlock.lock();
			battle_mess_list.pop_back();
			//chatlock.unlock();
		}
		break;
	}
	case SC_PLAYER_DEAD:
	{
		deadFlag = true;
		dead_time = chrono::high_resolution_clock::now();
		battle_mess_list.clear();
		break;
	}
	case SC_LOGIN_FAIL:
	{
		loginFail = true;
		break;
	}

	}
}



HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: 여기에 코드를 입력합니다.

	// 전역 문자열을 초기화합니다.
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_WINDOWSPROJECT1, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 응용 프로그램 초기화를 수행합니다:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINDOWSPROJECT1));

	MSG msg;

	// 기본 메시지 루프입니다:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}


//
//  함수: MyRegisterClass()
//
//  용도: 창 클래스를 등록합니다.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINDOWSPROJECT1));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WINDOWSPROJECT1);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   함수: InitInstance(HINSTANCE, int)
//
//   용도: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//
//   주석:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

	HWND hWnd = CreateWindowW(szWindowClass, TEXT("Simple MMORPG"), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, WINDOWSize_X, WINDOWSize_Y, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  용도: 주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 응용 프로그램 메뉴를 처리합니다.
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HBITMAP hBitmap;

	//	dubble buffer
	HDC hdc, memdc;
	HBITMAP BackBit, oldBackBit;
	PAINTSTRUCT ps;
	static RECT rectView;

	// server client
	static WSADATA WSAData;
	static SOCKET serverSocket;
	static sockaddr_in serverAddr;

	SetTimer(hWnd, 1, 50, NULL);

	// ID 입력
	static bool inputID = false;
	static bool rqLogin = false;
	short IDlen;
	static char cID[MAX_ID_LEN];

	// 채팅
	short chat_len;
	static char chat[MAX_CHAT];

	switch (message)
	{
	case WM_CREATE:
		// dubble buffer
		GetClientRect(hWnd, &rectView);

		for (int i = 0; i < MAP_SCALE_X; ++i) // 초기화
		{
			for (int j = 0; j < MAP_SCALE_Y; ++j)
			{
				WorldMap_Arr[i][j] = 0;
			}
		}
		LoadMap();


		// Socket init
		WSAStartup(MAKEWORD(2, 0), &WSAData);	//  네트워크 기능을 사용하기 위함, 인터넷 표준을 사용하기 위해
		serverSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, 0);
		memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(SERVER_PORT);
		inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);// ipv4에서 ipv6로 변환
		connect(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

		// WSAAsyncSelect
		WSAAsyncSelect(serverSocket, hWnd, WM_SOCKET, FD_READ || FD_CLOSE);

		// WSABUF 주소에 데이터받아올 버퍼주소 할당
		recv_buf.len = MAX_BUFFER;
		recv_buf.buf = buffer;


		break;
	case WM_CHAR:
		//if (inputID == true)
			//break;
		if (inputID == false) {
			IDlen = strlen(cID);
			cID[IDlen] = (TCHAR)wParam;
			cID[IDlen + 1] = 0;
			InvalidateRgn(hWnd, NULL, FALSE);
		}
		if (true == chatting) {
			chat_len = strlen(chat);
			chat[chat_len] = (TCHAR)wParam;
			chat[chat_len + 1] = 0;
			InvalidateRgn(hWnd, NULL, FALSE);
		}
		// 키입력
		if (true == gamestart && false == chatting) {
			if (wParam == 'a') {
				cs_packet_attack packet;
				packet.type = CS_ATTACK;
				packet.id = MyInfo.id;
				packet.size = sizeof(packet);
				send(serverSocket, (char*)&packet, sizeof(packet), 0);
			}
		}
		InvalidateRgn(hWnd, NULL, FALSE);
		break;
	case WM_TIMER:
		switch (wParam)
		{
		case 1:
			break;
		}
		//InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// 메뉴 선택을 구문 분석합니다:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		if (rqLogin == true)
		{
			cs_packet_request_login packet;
			packet.size = sizeof(packet);
			packet.type = CS_REQUEST_LOGIN;
			packet.id = myID;
			strcpy_s(packet.loginid, cID);
			send(serverSocket, (char*)&packet, sizeof(packet), 0);
			rqLogin = false;
		}

		// dubble buffer
		hdc = BeginPaint(hWnd, &ps);
		memdc = CreateCompatibleDC(hdc);
		BackBit = CreateCompatibleBitmap(hdc, rectView.right, rectView.bottom);
		oldBackBit = (HBITMAP)SelectObject(memdc, BackBit);
		if (inputID == false) {
			Render_LoginScene(memdc, cID);
		}

		// 플레이어 시야, 다른플레이어 랜더링
		if (loginOK == true) {
			Render_Battle_Message(memdc);
			Render_Board(memdc, MyInfo.x, MyInfo.y);
			Render_ChatBox(memdc, chat);
			Render_Stat(memdc, cID);
			if (true == deadFlag) {
				Render_Dead_Message(memdc);
			}
		}
		else if (true == loginFail) {
			auto ret = MessageBox(hWnd, TEXT("your ID is already login"), TEXT("message"), MB_OK);
			SendMessage(hWnd, WM_CLOSE, 0, 0);
		}

		// 백버퍼 불러오기
		BitBlt(hdc, 0, 0, rectView.right, rectView.bottom, memdc, 0, 0, SRCCOPY);

		DeleteObject(SelectObject(memdc, oldBackBit));
		DeleteDC(memdc);

		EndPaint(hWnd, &ps);
	}
	break;
	case WM_KEYUP:
		break;
	case WM_KEYFIRST:
		if (wParam == VK_RETURN) {
			inputID = true;
			rqLogin = true;
		}
		if (wParam == VK_RIGHT)
		{
			cs_packet_right key_packet;
			key_packet.type = CS_RIGHT;
			key_packet.size = sizeof(key_packet);
			//char * ptr = reinterpret_cast<char *>(&key_packet);
			//memcpy(&sendbuffer, ptr, sizeof(cs_packet_key));
			send(serverSocket, (char*)&key_packet, sizeof(key_packet), 0);
		}
		if (wParam == VK_LEFT)
		{
			cs_packet_left key_packet;
			key_packet.type = CS_LEFT;
			key_packet.size = sizeof(key_packet);
			send(serverSocket, (char*)&key_packet, sizeof(key_packet), 0);
		}
		if (wParam == VK_UP)
		{
			cs_packet_up key_packet;
			key_packet.type = CS_UP;
			key_packet.size = sizeof(key_packet);
			send(serverSocket, (char*)&key_packet, sizeof(key_packet), 0);
		}
		if (wParam == VK_DOWN)
		{
			cs_packet_down key_packet;
			key_packet.type = CS_DOWN;
			key_packet.size = sizeof(key_packet);
			send(serverSocket, (char*)&key_packet, sizeof(key_packet), 0);
		}
		if (wParam == VK_SPACE)
		{
			if (false == chatting) {
				chatting = true;
			}
			else if (true == chatting) {
				cs_packet_chat packet;
				packet.id = MyInfo.id;
				packet.type = CS_CHAT;
				strcpy_s(packet.name, cID);
				strcpy_s(packet.chat, chat);
				send(serverSocket, (char*)&packet, sizeof(packet), 0);

				chatting = false;
				memset(chat, 0, sizeof(chat));
			}
		}

		InvalidateRgn(hWnd, NULL, FALSE);
		break;

	case WM_SOCKET:
		if (WSAGETSELECTERROR(lParam)) {
			closesocket((SOCKET)wParam);
			break;
		}
		switch (WSAGETSELECTEVENT(lParam))
		{
		case FD_READ:
			ReadBuffer((SOCKET)wParam);

			break;
		}
		InvalidateRgn(hWnd, NULL, FALSE);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
