
#include <WS2tcpip.h> 
#pragma comment(lib, "Ws2_32.lib") 

#include "protocol.h"
#include "globals.h"

struct OVER_EX {		// send할때마다 overlapped 구조체를 만들어서 해야한다.
	WSAOVERLAPPED over;
	WSABUF wsabuf[1];
	char net_buf[MAX_BUFFER];
	bool is_recv;
};

struct SOCKETINFO {
	OVER_EX recv_over;
	SOCKET socket;
	int id;
	int DBkey;
	short x, y;
	mutex view_list_lock;
};


map <int, SOCKETINFO*> clients;	// 이런 공용자료를 읽고 쓸때는 뮤텍스를 걸어야한다.
HANDLE g_iocp;
list<int> list_clientID;
int new_user_id = 0;
vector<set<int>> view_list(20);	// 연결된 클라id마다 시야범위안에있는 플레이어들
vector<DATABASE> vec_database;

void sql_show_error();
void sql_HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);
void sql_load_database();
void sql_update_database(int keyid, int x, int y);

void error_display(const char *msg, int err_no)		// 네트워크 에러를 검출하여 보여주는 함수
{
	WCHAR *lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::cout << msg;
	std::wcout << L"에러 " << lpMsgBuf << std::endl;
	while (true);		// 이부분에서 일부러 멈추게 하기위해서
	LocalFree(lpMsgBuf);
}

bool is_near(int me, int other)	// 두 아이디 간 거리가 시야범위 내 인지 판별
{
	if (abs(clients[me]->x - clients[other]->x) < 8 && abs(clients[me]->y - clients[other]->y) < 8) {
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
	send_over->is_recv = false;
	memcpy(send_over->net_buf, packet, packet_size);
	send_over->wsabuf[0].buf = send_over->net_buf;
	send_over->wsabuf[0].len = packet_size;

	WSASend(clients[id]->socket, send_over->wsabuf, 1, 0, 0, &send_over->over, 0);
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
	packet.x = clients[new_id]->x;
	packet.y = clients[new_id]->y;

	send_packet(client, &packet);

	lock_guard<mutex> lg{ clients[client]->view_list_lock };
	if (client != new_id) {
		view_list[client].insert(new_id);
	}

}

void send_pos_packet(int client, int mover)
{
	sc_packet_pos packet;
	packet.id = mover;
	packet.size = sizeof(packet);
	packet.type = SC_POS;
	packet.x = clients[mover]->x;
	packet.y = clients[mover]->y;

	send_packet(client, &packet);
}

void send_remove_player_packet(int client, int leaver)
{
	sc_packet_remove_player packet;
	packet.id = leaver;
	packet.size = sizeof(packet);
	packet.type = SC_REMOVE_PLAYER;
	send_packet(client, &packet);

	lock_guard<mutex> lg{ clients[client]->view_list_lock };
	view_list[client].erase(leaver);
}

void ProcessPacket(int id, void* buf)
{
	char* packet = reinterpret_cast<char*>(buf);
	short x = clients[id]->x;
	short y = clients[id]->y;
	switch (packet[1]) {
	case CS_REQUEST_LOGIN:
	{
		cs_packet_request_login *login_packet = reinterpret_cast<cs_packet_request_login *>(buf);
		cout <<"[" <<login_packet->loginid<<"] request login" << "\n";
		// 여기에 데이터베이스에서 아이디 체크하는 코드 넣기
		for (auto &db : vec_database) {
			if (strcmp(db.idName, login_packet->loginid) == 0) {
				cout << "[" << login_packet->loginid << "] login success" << "\n";
				send_login_ok_packet(id);
				clients[id]->x = db.x;
				clients[id]->y = db.y;
				clients[id]->DBkey = db.idKey;

				for (auto& cl : clients) {
					int other_player = cl.first;
					if (is_near(id, other_player) == true)
					{
						send_put_player_packet(other_player, id); // 내 로그인 정보를 다른클라이언트에 전송

						if (other_player != id) { // 다른 클라이언트 정보도 나에게 전송
							send_put_player_packet(id, other_player);
						}
					}
				}
				return;
			}
		}
		cout << "[" << login_packet->loginid << "] is invalid ID" << "\n";
		closesocket(clients[id]->socket);
		clients.erase(id);
		return;
		
	}
	case KEY_LEFT:
		if (x >= 1) {
			x -= 1;
		}
		break;
	case KEY_RIGHT:
		if (x <= MAP_SCALE_X - 1) {
			x += 1;
		}
		break;
	case KEY_UP:
		if (y >= 1) {
			y -= 1;
		}
		break;
	case KEY_DOWN:
		if (y <= MAP_SCALE_X - 1) {
			y += 1;
		}
		break;
	}
	clients[id]->x = x;
	clients[id]->y = y;

	//lock_guard<mutex> lg{ view_list_lock };
	send_pos_packet(id, id); // 자기 자신에게 전송

	set<int> temp = view_list[id];

	for (auto &cl : temp) {	// 시야 리스트 내에 있는 플레이어들에게만 이동한 정보 전송
		if (is_near(id, cl) == true) {
			send_pos_packet(cl, id);
		}
		else {
			send_remove_player_packet(cl, id);
			send_remove_player_packet(id, cl);
		}
	}
	for (auto& cl : clients) {
		if (cl.first != id)
		{
			if (view_list[id].count(cl.first) == 0) // 시야 리스트에 없는 플레이어들 중 시야범위안으로 들어온 플레이어에게 전송
			{
				if (is_near(cl.first, id) == true)
				{
					send_put_player_packet(cl.first, id);
					send_put_player_packet(id, cl.first);
				}
			}
		}

	}
}


void do_worker()
{
	while (true) {
		DWORD num_byte;
		ULONG key;
		PULONG p_key = &key;
		WSAOVERLAPPED* p_over;

		GetQueuedCompletionStatus(g_iocp, &num_byte, (PULONG_PTR)p_key, &p_over, INFINITE); //  최초에는 해당 쓰레드를 쓰레드 풀 안에 등록하는 기능
																				//	두번째 부터는 쓰레드 풀로부터 쓰레드를 가져와서 iocp를 통해 데이터를 받는다?

		OVER_EX* over_ex = reinterpret_cast<OVER_EX*>(p_over);	// over_ex구조체로 받아온 데이터를 넣는다
		SOCKET client_s = clients[key]->socket;

		if (num_byte == 0) {	//접속종료인지 확인
			for (auto &db : vec_database) {
				if (db.idKey == clients[key]->DBkey) {
					short x = clients[key]->x;
					short y = clients[key]->y;
					sql_update_database(db.idKey, x, y);
					db.x = x;
					db.y = y;
				}
			}
			closesocket(client_s);
			clients.erase(key);
			for (auto &cl : clients)
			{
				send_remove_player_packet(cl.first, key);
				view_list[cl.first].erase(key);
			}
			list_clientID.emplace_back(key);

			continue;
		} // 클라이언트가 closesocket을 했을 경우


		if (true == over_ex->is_recv) {

			over_ex->net_buf[num_byte] = 0;
			cout << "From client[" << client_s << "] : ";
			cout << over_ex->net_buf << " (" << num_byte << ") bytes)\n";
			// recv처리
			ProcessPacket(key, over_ex->net_buf);


			DWORD flags = 0;
			memset(&over_ex->over, 0x00, sizeof(WSAOVERLAPPED));
			//send를 기다리지않고 리시브상태로 돌아가기 때문에 recv용 overlapped 구조체를 새로 만들어야한다
			WSARecv(client_s, over_ex->wsabuf, 1, 0, &flags, &over_ex->over, 0);
		}
		else {
			if (num_byte == 0) {
				closesocket(client_s);
				clients.erase(client_s);
				delete p_over;
				continue;
			} // 클라이언트가 closesocket을 했을 경우
			cout << "TRACE - SEND message [ " << client_s << "] : ";
			cout << over_ex->net_buf[1] << " (" << num_byte << " bytes)\n";

			delete over_ex;		// p_over를 delete해야 send했다는 의미이다
		}
	}
}

int main()
{
	for (int i = 1; i < 21; ++i) {
		list_clientID.emplace_back(i);
	}

	std::wcout.imbue(std::locale("korean")); // 오류문 한글로 표시해주기위함
	// sql db 엑세스
	sql_load_database();
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

	g_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);	// 핸들값을 리턴받아서 icop를 등록하는 단계
	thread worker_thread{ do_worker };
	//thread worker_thread2{ do_worker };
	//thread worker_thread3{ do_worker };
	while (true)
	{
		clientSocket = accept(listenSocket, (struct sockaddr *)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET)	// 소켓연결실패시
		{
			printf("err  \n");
			break;
		}
		else
		{
			printf("success  \n");
		}

		int user_id = list_clientID.front();	// 클라이언트아이디 키값 할당
		list_clientID.pop_front();				// 할당한 키값 리스트에서 삭제 (중복방지)
		clients[user_id] = new SOCKETINFO;
		//memset(&clients[user_id], 0, sizeof(struct SOCKETINFO));
		clients[user_id]->socket = clientSocket;
		clients[user_id]->recv_over.wsabuf[0].len = MAX_BUFFER;
		clients[user_id]->recv_over.wsabuf[0].buf = clients[user_id]->recv_over.net_buf;
		clients[user_id]->recv_over.is_recv = true;
		flags = 0;
		CreateIoCompletionPort(reinterpret_cast<HANDLE>(clientSocket), g_iocp, user_id, 0);	// iocp에 소켓을 등록하는 단계
		
		send_id_packet(user_id);

		//send_login_ok_packet(user_id);
		//clients[user_id].x = 0;
		//clients[user_id].y = 0;

		//for (auto& cl : clients) {
		//	int other_player = cl.first;
		//	if (is_near(user_id, other_player) == true)
		//	{
		//		send_put_player_packet(other_player, user_id); // 내 로그인 정보를 다른클라이언트에 전송

		//		if (other_player != user_id) { // 다른 클라이언트 정보도 나에게 전송
		//			send_put_player_packet(user_id, other_player);
		//		}
		//	}
		//}

		// memset
		memset(&clients[user_id]->recv_over.over, 0, sizeof(clients[user_id]->recv_over.over));

		int ret = WSARecv(clientSocket, clients[user_id]->recv_over.wsabuf, 1, NULL,
			&flags, &(clients[user_id]->recv_over.over), NULL);	// 플레그 0으로 수정하면 에러뜸

		if (0 != ret)		// send recv할때 리턴값이 있다 ret 값이 0이 아닐경우 그 에러가 WSA_IO_PENDING 이 아닐경우 에러
		{
			int err_no = WSAGetLastError();
			if (WSA_IO_PENDING != err_no)
				error_display(" WSARecv Error: ", err_no);
		}

	}
	worker_thread.join();

	closesocket(listenSocket);
	WSACleanup();
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
	SQLHENV henv;		// 데이터베이스에 연결할때 사옹하는 핸들
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0; // sql명령어를 전달하는 핸들
	SQLRETURN retcode;  // sql명령어를 날릴때 성공유무를 리턴해줌
	SQLINTEGER nKey, nLevel, nX, nY;	// 인티저
	SQLWCHAR szID[11];	// 문자열
	SQLLEN cbID = 0, cbKey = 0, cbLevel = 0, cbX = 0, cbY = 0;

	setlocale(LC_ALL, "korean"); // 오류코드 한글로 변환
	//std::wcout.imbue(std::locale("korean"));

	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0); // ODBC로 연결

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0); // 5초간 연결 5초넘어가면 타임아웃

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2015184024_gameserver", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
				//retcode = SQLConnect(hdbc, (SQLWCHAR*)L"jys_gameserver", SQL_NTS, (SQLWCHAR*)NULL, SQL_NTS, NULL, SQL_NTS);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt); // SQL명령어 전달할 한들

					retcode = SQLExecDirect(hstmt, (SQLWCHAR *)L"SELECT c_id, c_key, c_level, c_px, c_py FROM player_table ORDER BY 2, 1, 3", SQL_NTS); // 모든 정보 다 가져오기
					//retcode = SQLExecDirect(hstmt, (SQLWCHAR *)L"EXEC select_highlevel 90", SQL_NTS); // 90레벨 이상만 가져오기

					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

						// Bind columns 1, 2, and 3  
						retcode = SQLBindCol(hstmt, 1, SQL_UNICODE_CHAR, szID, 11, &cbID); // 이름 유니코드경우 SQL_UNICODE_CHAR 사용
						retcode = SQLBindCol(hstmt, 2, SQL_C_LONG, &nKey, 4, &cbKey);	// 아이디
						retcode = SQLBindCol(hstmt, 3, SQL_C_LONG, &nLevel, 4, &cbLevel);	// 경험치
						retcode = SQLBindCol(hstmt, 4, SQL_C_LONG, &nX, 4, &cbX);
						retcode = SQLBindCol(hstmt, 5, SQL_C_LONG, &nY, 4, &cbY);

						// Fetch and print each row of data. On an error, display a message and exit.  
						for (int i = 0; ; i++) {
							retcode = SQLFetch(hstmt);  // hstmt 에서 데이터를 꺼내오는거
							if (retcode == SQL_ERROR)
								sql_show_error();
							if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
								wprintf(L"%d: %d %lS %d %d %d \n", i + 1, nKey, szID, nLevel, nX, nY);
								
								// wchar char로 변환
								char *temp;
								int strSize = WideCharToMultiByte(CP_ACP, 0, szID, -1, NULL, 0, NULL, NULL);
								temp = new char[11];
								WideCharToMultiByte(CP_ACP, 0, szID, -1, temp, strSize, 0, 0);
								if (isdigit(temp[strlen(temp) - 1]) == 0) {
									//cout << "문자열공백제거\n";
									temp[strlen(temp)-1] = 0; // 무슨이유에선진 모르겟지만 아이디마지막문자가 영문일 경우 맨뒤에 공백하나가 추가됨
								}
								DATABASE data;
								data.idKey = nKey;
								memcpy(data.idName, temp, 10);
								data.level = nLevel;
								data.x = (short)nX;
								data.y = (short)nY;
								cout<< "[" << data.idName << "]  ";
								cout << data.idKey << " " << data.level << " " << data.x << " " << data.y << "\n";
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
						SQLCancel(hstmt); // 핸들캔슬
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
}

void sql_update_database(int keyid, int x, int y)
{
	SQLHENV henv;		// 데이터베이스에 연결할때 사옹하는 핸들
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0; // sql명령어를 전달하는 핸들
	SQLRETURN retcode;  // sql명령어를 날릴때 성공유무를 리턴해줌
	SQLWCHAR query[1024];
	wsprintf(query, L"UPDATE player_table SET c_px = %d, c_py = %d WHERE c_key = %d", x, y, keyid);


	setlocale(LC_ALL, "korean"); // 오류코드 한글로 변환
	//std::wcout.imbue(std::locale("korean"));

	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0); // ODBC로 연결

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0); // 5초간 연결 5초넘어가면 타임아웃

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2015184024_gameserver", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
				//retcode = SQLConnect(hdbc, (SQLWCHAR*)L"jys_gameserver", SQL_NTS, (SQLWCHAR*)NULL, SQL_NTS, NULL, SQL_NTS);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt); // SQL명령어 전달할 한들

					retcode = SQLExecDirect(hstmt, (SQLWCHAR *)query, SQL_NTS); // 쿼리문
					//retcode = SQLExecDirect(hstmt, (SQLWCHAR *)L"EXEC select_highlevel 90", SQL_NTS); // 90레벨 이상만 가져오기

					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						cout << "DataBase update success";
					}
					else {
						sql_HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
					}

					// Process data  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						SQLCancel(hstmt); // 핸들캔슬
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
