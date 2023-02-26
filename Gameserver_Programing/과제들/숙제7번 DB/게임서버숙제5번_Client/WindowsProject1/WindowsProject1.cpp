// WindowsProject1.cpp : 응용 프로그램에 대한 진입점을 정의합니다.

#include "stdafx.h"
#include "global.h"
#include "..\..\게임서버숙제5번_iocpServer\게임서버숙제5번_iocpServer\protocol.h"
#include "WindowsProject1.h"


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

PLAYER_INFO MyInfo{};
PLAYER_INFO PlayerArr[MAX_PLAYER];
WSABUF recv_buf; // WSARecv를 사용하기위해 WSABUF 사용

void Render_LoginScene(HDC BackBuffer, char* ID)
{
	Rectangle(BackBuffer, 0, 0, WINDOWSize_X, WINDOWSize_Y);
	//Rectangle(BackBuffer, WINDOWSize_X/2, WINDOWSize_Y/2, 690, 55);
	TextOut(BackBuffer, WINDOWSize_X/3, WINDOWSize_Y/2, TEXT("input ID and press ENTER "), 25);
	TextOut(BackBuffer, WINDOWSize_X / 3, WINDOWSize_Y / 2 + 15, TEXT("ID : "), 5);
	TextOutA(BackBuffer, WINDOWSize_X / 3 + 25, WINDOWSize_Y / 2 + 15, ID, strlen(ID));
}


void Render_Board(HDC BackBuffer, int x, int y)
{
	//Rectangle(BackBuffer, )


	x *= blockSize;	// 게임내 블럭사이즈에 맞춰서 좌표 변환
	y *= blockSize;
	int diffOfLength = (FOV - 1) * blockSize;	// 서버와 클라이언트에서 좌표차이
	for (int i = 0; i < MAP_SCALE_Y; ++i) {
		for (int j = 0; j < MAP_SCALE_X; ++j) {
			if (x/blockSize - FOV < j && j < x/blockSize + FOV && y/blockSize - FOV < i && i < y/blockSize + FOV) 
			{
				if (WorldMap_Arr[i][j] == 0) // 노말지형
				{	
					Rectangle(BackBuffer, j * blockSize + diffOfLength - x, i * blockSize + diffOfLength - y,
						(j + 1) * blockSize + diffOfLength - x, (i + 1) * blockSize + diffOfLength - y);
				}
				if (WorldMap_Arr[i][j] == 1) // 가로세로 격자
				{	
					HBRUSH hBrush = CreateSolidBrush(RGB(103, 26, 10));
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
	for (int i = 0; i < MAX_PLAYER; ++i) {	// 다른플레이어 위치
		if (PlayerArr[i].isNear == true)
		{
			float px = (PlayerArr[i].x * blockSize) - x + diffOfLength;	// 위치 변환
			float py = (PlayerArr[i].y * blockSize) - y + diffOfLength;

			HBRUSH hBrush = CreateSolidBrush(RGB(200, 0, 0));
			HBRUSH oldBrush = (HBRUSH)SelectObject(BackBuffer, hBrush);
			Rectangle(BackBuffer, px, py, px + blockSize, py + blockSize);
			SelectObject(BackBuffer, oldBrush);
			DeleteObject(hBrush);
		}
	}




	// 화면 정중앙에 내 캐릭터 위치
	HBRUSH hBrush = CreateSolidBrush(RGB(10, 150, 10));
	HBRUSH oldBrush = (HBRUSH)SelectObject(BackBuffer, hBrush);
	Rectangle(BackBuffer, blockSize *(FOV-1), blockSize *(FOV - 1), blockSize *FOV, blockSize *FOV);
	SelectObject(BackBuffer, oldBrush);
	DeleteObject(hBrush);
}

void ReadBuffer(SOCKET sock)
{
	int in_packet_size = 0;
	int saved_packet_size = 0;

	DWORD iobyte, ioflag = 0;
	WSARecv(sock, &recv_buf, 1, &iobyte, &ioflag, NULL, NULL);

	char * temp = reinterpret_cast<char*>(buffer);

	while (iobyte != 0)
	{
		if (in_packet_size == 0)
		{
			in_packet_size = temp[0];
		}
		if (iobyte + saved_packet_size >= in_packet_size)
		{
			memcpy(buffer + saved_packet_size, temp, in_packet_size - saved_packet_size);
			PacketProccess(buffer);
			temp += in_packet_size - saved_packet_size;
			iobyte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else
		{
			memcpy(buffer + saved_packet_size, temp, iobyte);
			saved_packet_size += iobyte;
			iobyte = 0;
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
			loginOK = true;
		}
		else {
			PlayerArr[id].id = id;
			//PlayerArr[id].login = true;
			PlayerArr[id].isNear = true;
			PlayerArr[id].x = put_player_packet->x;
			PlayerArr[id].y = put_player_packet->y;
		}
		break;
	}
	case SC_REMOVE_PLAYER:
	{
		sc_packet_remove_player *remove_player_packet = reinterpret_cast<sc_packet_remove_player *>(buf);
		int id = remove_player_packet->id;
		//PlayerArr[id].login =false;
		PlayerArr[id].isNear = false;
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
		else {
			PlayerArr[id].x = pos_packet->x;
			PlayerArr[id].y = pos_packet->y;
		}

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

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
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
		for (int i = 0; i < MAP_SCALE_X; ++i)	// 가로 세로 격자
		{
			for (int j = 0; j < MAP_SCALE_Y; j += 10)
			{
				WorldMap_Arr[i][j] = 1;
				WorldMap_Arr[j][i] = 1;
			}
		}

		WorldMap_Arr[6][7] = 1;

		// Socket init
		WSAStartup(MAKEWORD(2, 0), &WSAData);	//  네트워크 기능을 사용하기 위함, 인터넷 표준을 사용하기 위해
		serverSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, 0);
		memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(SERVER_PORT);
		inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);// ipv4에서 ipv6로 변환
		connect(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

		//
		WSAAsyncSelect(serverSocket, hWnd, WM_SOCKET, FD_READ || FD_CLOSE);
		//
		recv_buf.len = MAX_BUFFER;
		recv_buf.buf = buffer;
		

		break;
	case WM_CHAR:
		if (inputID == true)
			break;
		if (inputID == false)
		{
			IDlen = strlen(cID);
			cID[IDlen] = (TCHAR)wParam;
			cID[IDlen + 1] = 0;
			InvalidateRgn(hWnd, NULL, FALSE);
		}
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
			Render_Board(memdc, MyInfo.x, MyInfo.y);
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
			cs_packet_key key_packet;
			key_packet.cKey = KEY_RIGHT;
			//char * ptr = reinterpret_cast<char *>(&key_packet);
			//memcpy(&sendbuffer, ptr, sizeof(cs_packet_key));
			send(serverSocket, (char*)&key_packet, sizeof(cs_packet_key), 0);
		}
		if (wParam == VK_LEFT)
		{
			cs_packet_key key_packet;
			key_packet.cKey = KEY_LEFT;
			send(serverSocket, (char*)&key_packet, sizeof(cs_packet_key), 0);
		}
		if (wParam == VK_UP)
		{
			cs_packet_key key_packet;
			key_packet.cKey = KEY_UP;
			send(serverSocket, (char*)&key_packet, sizeof(cs_packet_key), 0);
		}
		if (wParam == VK_DOWN)
		{
			cs_packet_key key_packet;
			key_packet.cKey = KEY_DOWN;
			send(serverSocket, (char*)&key_packet, sizeof(cs_packet_key), 0);
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
