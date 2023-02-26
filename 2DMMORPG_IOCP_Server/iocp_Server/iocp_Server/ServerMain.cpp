#include "ServerMain.h"

ServerMain::ServerMain()
{
	use_DB = false;
	servertest = false;
	initServer();
}

ServerMain::~ServerMain()
{
	WSACleanup();
}

void ServerMain::initServer()
{
	for (int i = 0; i < MAX_OBJECT; ++i) {
		clients[i] = new SocketInfo;
		clients[i]->set_isLive(false);
	}
	loadMap();
	initNPC();
	initSocket();

	m_iocp_handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	m_new_client_id = 0;
	DBCount = 0;
	
	m_DBManager = new DatabaseManager;
	if (use_DB == true) {
		m_DBManager->sql_load_database();
		DBCount = m_DBManager->vec_database.size();
		cout <<"DB length: " <<DBCount << endl;
	}


	initThread();
}

void ServerMain::initNPC()
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
		clients[npc_id]->set_hp(100);
		clients[npc_id]->set_att(0);
	}
	for (int npc_id = NPC_ID_START + NUM_NPC - 100; npc_id < NPC_ID_START + NUM_NPC - 50; ++npc_id) {
		// 던전1
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
		clients[npc_id]->set_hp(100);
		clients[npc_id]->set_att(20);

	}
	for (int npc_id = NPC_ID_START + NUM_NPC - 50; npc_id < NPC_ID_START + NUM_NPC; ++npc_id) {
		// 던전2
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
		clients[npc_id]->set_hp(100);
		clients[npc_id]->set_att(20);
	}

	cout << "npc 생성완료\n";
}

void ServerMain::loadMap()
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
	cout << "load map \n";
}

void ServerMain::initSocket()
{
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	// 소켓 설정 bind
	if (::bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR) {
		cout << "bind fail \n";
		closesocket(listenSocket);
		WSACleanup();
		return;
	}
	m_accept_socket = listenSocket;
}

void ServerMain::initThread()
{
	thread accept_thread{ &ServerMain::do_accept_thread, this};
	thread queue_thread{ &ServerMain::do_timer_thread, this };
	thread main_workThread_1{ &ServerMain::do_mainWorker_thread, this};
	thread main_workThread_2{ &ServerMain::do_mainWorker_thread, this };
	thread main_workThread_3{ &ServerMain::do_mainWorker_thread, this };
	thread main_workThread_4{ &ServerMain::do_mainWorker_thread, this };

	accept_thread.join();
	queue_thread.join();
	main_workThread_1.join();
	main_workThread_2.join();
	main_workThread_3.join();
	main_workThread_4.join();
}

void ServerMain::do_accept_thread()
{
	listen(m_accept_socket, 5);
	SOCKADDR_IN clientADDR;
	int addr_len = sizeof(SOCKADDR_IN);
	memset(&clientADDR, 0, addr_len);
	SOCKET clientSocket;
	DWORD flags;

	while (true) {
		clientSocket = accept(m_accept_socket, (struct sockaddr *)&clientADDR, &addr_len);
		if (clientSocket == INVALID_SOCKET) {	// 소켓연결실패시
			printf("err  \n");
			break;
		}
		else {
			printf("success  \n");
		}

		int user_id = m_new_client_id++;
		clients[user_id]->set_id(user_id);
		clients[user_id]->set_socket(clientSocket);
		clients[user_id]->set_isDead(true);
		clients[user_id]->set_isLive(true);
		CreateIoCompletionPort(reinterpret_cast<HANDLE>(clientSocket), m_iocp_handle, user_id, 0);	// iocp에 소켓을 등록하는 단계

		m_PacketSend->send_id(user_id, clientSocket);

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
			addEvent(ev_heal);
			for (auto& cl : clients) {
				if (false == cl->get_isLive()) { continue; }
				if (true == cl->get_isDead()) { continue; }

				int other_player = cl->get_id();
				if (true == is_NPC(other_player)
					&& false == is_active(other_player)
					&& true == is_near_npc(user_id, other_player)) { // 플레이어 주변의 npc 깨우기
					clients[other_player]->set_isActive(true);
					EVENT ev{ other_player, 0, EV_MOVE, chrono::high_resolution_clock::now() + 1s };
					addEvent(ev);
				}

				if (true == is_near(user_id, other_player))
				{
					if (false == is_NPC(other_player)) { // npc가 아닌경우에
						m_PacketSend->send_put_player(other_player, cl->get_socket(), user_id, clients[user_id]->get_xPos(),
							clients[user_id]->get_yPos(), clients[user_id]->get_hp(), clients[user_id]->get_att(), clients[user_id]->get_exp(),
							clients[user_id]->get_level(), clients[user_id]->get_obj_type()); // 내 로그인 정보를 다른클라이언트에 전송
						// 뷰리스트에 없던 id이면 삽입
						cl->get_viewList_lock().lock();
						if (cl->get_id() != user_id) {
							if (cl->viewList_count(user_id) == 0) {
								cl->insert_viewList(user_id);
							}
						}
						cl->get_viewList_lock().unlock();
					}

					if (other_player != user_id) { // 다른 클라이언트 정보도 나에게 전송
						m_PacketSend->send_put_player(user_id, clients[user_id]->get_socket(), other_player, cl->get_xPos(),
							cl->get_yPos(), cl->get_hp(), cl->get_att(), cl->get_exp(), cl->get_level(), cl->get_obj_type());
						// 뷰리스트에 없던 id이면 삽입
						clients[user_id]->get_viewList_lock().lock();
						if (user_id != other_player) {
							if (clients[user_id]->viewList_count(other_player) == 0) {
								clients[user_id]->insert_viewList(other_player);
							}
						}
						clients[user_id]->get_viewList_lock().unlock();
					}
				}

			}
			clients[user_id]->set_isLive(true);
		}

		// memset
		memset(&clients[user_id]->get_over().over, 0, sizeof(clients[user_id]->get_over().over));
		flags = 0;
		int ret = WSARecv(clientSocket, clients[user_id]->get_over().wsabuf, 1, NULL,
			&flags, &(clients[user_id]->get_over().over), NULL);	// 플레그 0으로 수정하면 에러뜸

		if (0 != ret)		// send recv할때 리턴값이 있다 ret 값이 0이 아닐경우 그 에러가 WSA_IO_PENDING 이 아닐경우 에러
		{
			int err_no = WSAGetLastError();
			if (WSA_IO_PENDING != err_no)
				sock_errDisplay(" WSARecv Error: ", err_no);
		}

	}
}

void ServerMain::do_mainWorker_thread()
{
	while (true) {
		WSAOVERLAPPED* p_over;
		ULONG key;
		PULONG p_key = &key;
		DWORD recv_byte;

		GetQueuedCompletionStatus(m_iocp_handle, &recv_byte, (PULONG_PTR)p_key, &p_over, INFINITE);

		OVER_EX* over_ex = reinterpret_cast<OVER_EX*>(p_over);
		if (false == clients[key]->get_isLive()) {
			continue;
		}
		SOCKET client_s = clients[key]->get_socket();

		if (recv_byte == 0) {	//접속종료인지 확인
			if (true == use_DB) {
				/*for (auto &db : vec_database) {
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
				}*/
			}
			closesocket(client_s);
			clients[key]->set_isLive(false);
			for (auto &cl : clients)
			{
				if (false == cl->get_isLive()) { continue; }
				if (false == is_NPC(cl->get_id())) {
					m_PacketSend->send_remove_player(cl->get_id(), cl->get_socket(), key);
					cl->get_viewList_lock().lock();
					cl->delete_viewList(key);
					cl->get_viewList_lock().unlock();
				}
				clients[cl->get_id()]->delete_viewList(key);
			}

			continue;
		} // 클라이언트가 closesocket을 했을 경우

		if (EV_RECV == over_ex->event_type) {
			over_ex->buffer[recv_byte] = 0;

			// recv처리
			processPacket(key, over_ex->buffer);

			DWORD flags = 0;
			memset(&over_ex->over, 0x00, sizeof(WSAOVERLAPPED));
			WSARecv(client_s, over_ex->wsabuf, 1, 0, &flags, &over_ex->over, 0);
		}
		else if (EV_SEND == over_ex->event_type) {
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
			process_player_respawn_event(key);
			delete over_ex;
		}
		else if (EV_PLAYER_MOVE_NOTIFY == over_ex->event_type) {
			int player_id = *(int *)(over_ex->buffer);
			process_npc_meet_event(key, player_id);
			delete over_ex;
		}
		else {
			cout << "Unkown evnet type" << over_ex->event_type << "\n";
			while (true);
		}
	}
}

void ServerMain::do_timer_thread()
{
	while (true) {
		eventQueue_lock.lock();
		while (true == event_queue.empty()) {
			eventQueue_lock.unlock();
			this_thread::sleep_for(10ms);
			eventQueue_lock.lock();
		}
		const EVENT &ev = event_queue.top();
		if (ev.wakeup_time > chrono::high_resolution_clock::now()) {
			eventQueue_lock.unlock();
			this_thread::sleep_for(10ms);
			continue;
		}

		EVENT temp_ev = ev;
		event_queue.pop();
		eventQueue_lock.unlock();

		if (EV_MOVE == temp_ev.event_type) {
			OVER_EX *over_ex = new OVER_EX;
			over_ex->event_type = EV_MOVE;
			PostQueuedCompletionStatus(m_iocp_handle, 1, temp_ev.obj_id, &over_ex->over);
		}
		else if (EV_MOVE_COOLTIME == temp_ev.event_type) {
			OVER_EX *over_ex = new OVER_EX;
			over_ex->event_type = EV_MOVE_COOLTIME;
			PostQueuedCompletionStatus(m_iocp_handle, 1, temp_ev.obj_id, &over_ex->over);
		}
		else if (EV_HEAL == temp_ev.event_type) {
			OVER_EX *over_ex = new OVER_EX;
			over_ex->event_type = EV_HEAL;
			PostQueuedCompletionStatus(m_iocp_handle, 1, temp_ev.obj_id, &over_ex->over);
		}
		else if (EV_MOVE_TARGET == temp_ev.event_type) {
			OVER_EX *over_ex = new OVER_EX;
			over_ex->event_type = EV_MOVE_TARGET;
			PostQueuedCompletionStatus(m_iocp_handle, 1, temp_ev.obj_id, &over_ex->over);
		}
		else if (EV_NPC_RESPAWN == temp_ev.event_type) {
			OVER_EX *over_ex = new OVER_EX;
			over_ex->event_type = EV_NPC_RESPAWN;
			PostQueuedCompletionStatus(m_iocp_handle, 1, temp_ev.obj_id, &over_ex->over);
		}
		else if (EV_PLAYER_RESPAWN == temp_ev.event_type) {
			OVER_EX *over_ex = new OVER_EX;
			over_ex->event_type = EV_PLAYER_RESPAWN;
			PostQueuedCompletionStatus(m_iocp_handle, 1, temp_ev.obj_id, &over_ex->over);
		}

	}
}

void ServerMain::addEvent(EVENT & event)
{
	eventQueue_lock.lock();
	event_queue.push(event);
	eventQueue_lock.unlock();
}

void ServerMain::processPacket(int id, void * buff)
{
	char* packet = reinterpret_cast<char*>(buff);
	switch (packet[1]) {
	case CS_REQUEST_LOGIN:
	{
		cs_packet_request_login *login_packet = reinterpret_cast<cs_packet_request_login *>(buff);
		cout << "[" << login_packet->loginid << "] request login" << "\n";

		// 여기에 데이터베이스에서 아이디 체크하는 코드 넣기
		if (true == use_DB) {
			for (auto &db : m_DBManager->vec_database) {
				if (strcmp(db.idName, login_packet->loginid) == 0) {
					for (int i = 0; i < NPC_ID_START; ++i) {
						if (false == clients[i]->get_isLive()) { continue; }
						if (clients[i]->get_DBkey() == db.idKey) {
							cout << "logint fail \n";
							m_PacketSend->send_login_fail(id, clients[id]->get_socket());
							closesocket(clients[id]->get_socket());
							clients[id]->set_isLive(false);
							return;
						}
					}
					cout << "[" << login_packet->loginid << "] login success" << "\n";
					m_PacketSend->send_login_success(id, clients[id]->get_socket());
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
					addEvent(ev_heal);
					for (auto& cl : clients) {
						if (false == cl->get_isLive()) { continue; }
						if (true == cl->get_isDead()) { continue; }

						int other_player = cl->get_id();
						if (true == is_NPC(other_player)
							&& false == is_active(other_player)
							&& true == is_near_npc(id, other_player)) { // 플레이어 주변의 npc 깨우기
							clients[other_player]->set_isActive(true);
							EVENT ev{ other_player, 0, EV_MOVE, chrono::high_resolution_clock::now() + 1s };
							addEvent(ev);
						}

						if (true == is_near(id, other_player))
						{
							if (false == is_NPC(other_player)) { // npc가 아닌경우에
								m_PacketSend->send_put_player(other_player, cl->get_socket(), id, clients[id]->get_xPos(),
									clients[id]->get_yPos(), clients[id]->get_hp(), clients[id]->get_att(), clients[id]->get_exp(),
									clients[id]->get_level(), clients[id]->get_obj_type()); // 내 로그인 정보를 다른클라이언트에 전송
								// 뷰리스트에 없던 id이면 삽입
								cl->get_viewList_lock().lock();
								if (cl->get_id() != id) {
									if (cl->viewList_count(id) == 0) {
										cl->insert_viewList(id);
									}
								}
								cl ->get_viewList_lock().unlock();
							}

							if (other_player != id) { // 다른 클라이언트 정보도 나에게 전송
								m_PacketSend->send_put_player(id, clients[id]->get_socket(), other_player, cl->get_xPos(),
									cl->get_yPos(), cl->get_hp(), cl->get_att(), cl->get_exp(), cl->get_level(), cl->get_obj_type());
								// 뷰리스트에 없던 id이면 삽입
								clients[id]->get_viewList_lock().lock();
								if (id != other_player) {
									if (clients[id]->viewList_count(other_player) == 0) {
										clients[id]->insert_viewList(other_player);
									}
								}
								clients[id]->get_viewList_lock().unlock();
							}
						}
					}
					return;
				}
			}
		}
		else if (false == use_DB) {
			cout << "[" << login_packet->loginid << "] login success" << "\n";
			m_PacketSend->send_login_success(id, clients[id]->get_socket());
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
			addEvent(ev_heal);
			for (auto& cl : clients) {
				if (false == cl->get_isLive()) { continue; }
				if (true == cl->get_isDead()) { continue; }

				int other_player = cl->get_id();
				if (true == is_NPC(other_player)
					&& false == is_active(other_player)
					&& true == is_near_npc(id, other_player)) { // 플레이어 주변의 npc 깨우기
					clients[other_player]->set_isActive(true);
					EVENT ev{ other_player, 0, EV_MOVE, chrono::high_resolution_clock::now() + 1s };
					addEvent(ev);
				}

				if (true == is_near(id, other_player))
				{
					if (false == is_NPC(other_player)) { // npc가 아닌경우에
						m_PacketSend->send_put_player(other_player, cl->get_socket(), id, clients[id]->get_xPos(),
							clients[id]->get_yPos(), clients[id]->get_hp(), clients[id]->get_att(), clients[id]->get_exp(),
							clients[id]->get_level(), clients[id]->get_obj_type()); // 내 로그인 정보를 다른클라이언트에 전송
						// 뷰리스트에 없던 id이면 삽입
						cl->get_viewList_lock().lock();
						if (cl->get_id() != id) {
							if (cl->viewList_count(id) == 0) {
								cl->insert_viewList(id);
							}
						}
						cl->get_viewList_lock().unlock();
					}

					if (other_player != id) { // 다른 클라이언트 정보도 나에게 전송
						m_PacketSend->send_put_player(id, clients[id]->get_socket(), other_player, cl->get_xPos(),
							cl->get_yPos(), cl->get_hp(), cl->get_att(), cl->get_exp(), cl->get_level(), cl->get_obj_type());
						// 뷰리스트에 없던 id이면 삽입
						clients[id]->get_viewList_lock().lock();
						if (id != other_player) {
							if (clients[id]->viewList_count(other_player) == 0) {
								clients[id]->insert_viewList(other_player);
							}
						}
						clients[id]->get_viewList_lock().unlock();
					}
				}
			}
			return;
		}

		if (true == use_DB) {
			cout << "[" << login_packet->loginid << "] is invalid ID" << "\n"; //
			cout << "create new ID to DB \n";
			DBCount++;
			m_DBManager->sql_insert_database(DBCount, login_packet->loginid, login_packet->loginid, 1, start_posX, start_posY, 100, 0, 60);
			DATABASE *db = new DATABASE;
			db->idKey = DBCount;
			strcpy_s(db->idName, login_packet->loginid);
			db->level = 1;
			db->x = start_posX;
			db->y = start_posY;
			db->hp = 100;
			db->exp = 0;
			m_DBManager->vec_database.emplace_back(*db);
			m_PacketSend->send_login_success(id, clients[id]->get_socket());
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
			addEvent(ev_heal);
			for (auto& cl : clients) {
				if (false == cl->get_isLive()) { continue; }
				if (true == cl->get_isDead()) { continue; }

				int other_player = cl->get_id();
				if (true == is_NPC(other_player)
					&& false == is_active(other_player)
					&& true == is_near_npc(id, other_player)) { // 플레이어 주변의 npc 깨우기
					clients[other_player]->set_isActive(true);
					EVENT ev{ other_player, 0, EV_MOVE, chrono::high_resolution_clock::now() + 1s };
					addEvent(ev);
				}

				if (true == is_near(id, other_player))
				{
					if (false == is_NPC(other_player)) { // npc가 아닌경우에
						m_PacketSend->send_put_player(other_player, cl->get_socket(), id, clients[id]->get_xPos(),
							clients[id]->get_yPos(), clients[id]->get_hp(), clients[id]->get_att(), clients[id]->get_exp(),
							clients[id]->get_level(), clients[id]->get_obj_type()); // 내 로그인 정보를 다른클라이언트에 전송
						// 뷰리스트에 없던 id이면 삽입
						cl->get_viewList_lock().lock();
						if (cl->get_id() != id) {
							if (cl->viewList_count(id) == 0) {
								cl->insert_viewList(id);
							}
						}
						cl->get_viewList_lock().unlock();
					}

					if (other_player != id) { // 다른 클라이언트 정보도 나에게 전송
						m_PacketSend->send_put_player(id, clients[id]->get_socket(), other_player, cl->get_xPos(),
							cl->get_yPos(), cl->get_hp(), cl->get_att(), cl->get_exp(), cl->get_level(), cl->get_obj_type());
						// 뷰리스트에 없던 id이면 삽입
						clients[id]->get_viewList_lock().lock();
						if (id != other_player) {
							if (clients[id]->viewList_count(other_player) == 0) {
								clients[id]->insert_viewList(other_player);
							}
						}
						clients[id]->get_viewList_lock().unlock();
					}
				}
			}
			return;
		}

	}
	case CS_ATTACK:
	{
		//cout << "ATTACk" << endl;
		cs_packet_attack * attack_packet = reinterpret_cast<cs_packet_attack *>(buff);
		int id = attack_packet->id;
		short x = clients[id]->get_xPos();
		short y = clients[id]->get_yPos();
		short att = clients[id]->get_att();
		set<int> temp = clients[id]->get_viewList();
		for (auto vl : temp) {
			if (false == is_NPC(vl)) { continue; }
			short vx = clients[vl]->get_xPos();
			short vy = clients[vl]->get_yPos();
			if (vx == x && vy == (y - 1)) { // 위
				clients[vl]->set_hp(clients[vl]->get_hp() - att);
				m_PacketSend->send_battle_mess(id, clients[id]->get_socket(), vl, -1, att);
			}
			else if (vx == (x - 1) && vy == y) { // 왼쪽
				clients[vl]->set_hp(clients[vl]->get_hp() - att);
				m_PacketSend->send_battle_mess(id, clients[id]->get_socket(), vl, -1, att);
			}
			else if (vx == (x + 1) && vy == y) { // 오른쪽
				clients[vl]->set_hp(clients[vl]->get_hp() - att);
				m_PacketSend->send_battle_mess(id, clients[id]->get_socket(), vl, -1, att);
			}
			else if (vx == x && vy == (y + 1)) { // 아래
				clients[vl]->set_hp(clients[vl]->get_hp() - att);
				m_PacketSend->send_battle_mess(id, clients[id]->get_socket(), vl, -1, att);
			}

			if (0 >= clients[vl]->get_hp()) { // hp 0 이하면 삭제
				if (TYPE_PEACE_MONSTER == clients[vl]->get_obj_type()) {
					clients[id]->set_exp(clients[id]->get_exp() + 100);
				}
				if (TYPE_WAR_MONSTER == clients[vl]->get_obj_type()) {
					clients[id]->set_exp(clients[id]->get_exp() + 200);
				}
				// 스탯변화
				if (clients[id]->get_exp() >= 100 * (pow(2, clients[id]->get_level() - 1))) {
					clients[id]->set_level(clients[id]->get_level() + 1);
				}
				m_PacketSend->send_stat_change(id, clients[id]->get_socket(), clients[id]->get_hp(), clients[id]->get_exp(), 
					clients[id]->get_level());
				for (int i = 0; i < NPC_ID_START; ++i) {
					if (false == clients[i]->get_isLive()) { continue; }
					if (true == is_near(vl, i)) {
						m_PacketSend->send_remove_player(i, clients[i]->get_socket(), vl);
						clients[i]->get_viewList_lock().lock();
						clients[i]->delete_viewList(vl);
						clients[i]->get_viewList_lock().unlock();
					}
				}
				//send_remove_player_packet(id, vl);
				clients[vl]->set_isDead(true);
				//clients[vl].is_live = false;
				EVENT ev_npc_respawn{ vl, 0, EV_NPC_RESPAWN, chrono::high_resolution_clock::now() + 30s };
				addEvent(ev_npc_respawn);

			}
		}
		break;
	}
	case CS_CHAT:
	{
		cs_packet_chat * chat_packet = reinterpret_cast<cs_packet_chat*>(buff);
		for (int i = 0; i < NPC_ID_START; ++i) {
			if (true == clients[i]->get_isLive()) {
				m_PacketSend->send_chat(id, i, clients[i]->get_socket(), chat_packet->name, chat_packet->chat);
			}
		}
		
		break;
	}
	case CS_LEFT:
		process_player_move(id, buff);
		break;
	case CS_RIGHT:
		process_player_move(id, buff);
		break;
	case CS_UP:
		process_player_move(id, buff);
		break;
	case CS_DOWN:
		process_player_move(id, buff);
		break;
	}
}

void ServerMain::process_player_move(int id, void * buff)
{
	if (true == clients[id]->get_moveCooltime()) { return; }
	char* packet = reinterpret_cast<char*>(buff);
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
				addEvent(ev);
			}
			if (true == is_near(id, other_id)) {
				new_view_list.insert(other_id);
				if (true == is_NPC(other_id)) {
					OVER_EX *over_ex = new OVER_EX;
					over_ex->event_type = EV_PLAYER_MOVE_NOTIFY;
					*(int *)(over_ex->buffer) = id;
					PostQueuedCompletionStatus(m_iocp_handle, 1, other_id, &over_ex->over);
				}
			}
		}
	}

	// 자기 자신에게 전송
	m_PacketSend->send_pos(id, clients[id]->get_socket(), id, clients[id]->get_xPos(), clients[id]->get_yPos());

	for (auto cl : new_view_list) {
		if (0 == old_view_list.count(cl)) { // new뷰리스트중에 새로 추가된것
			m_PacketSend->send_put_player(id, clients[id]->get_socket(), cl, clients[cl]->get_xPos(), clients[cl]->get_yPos(),
				clients[cl]->get_hp(), clients[cl]->get_att(), clients[cl]->get_exp(), clients[cl]->get_level(), clients[cl]->get_obj_type());
			clients[id]->get_viewList_lock().lock();
			clients[id]->insert_viewList(cl);
			clients[id]->get_viewList_lock().unlock();

			if (false == is_NPC(cl)) {
				m_PacketSend->send_put_player(cl, clients[cl]->get_socket(), id, clients[id]->get_xPos(), clients[id]->get_yPos(),
					clients[id]->get_hp(), clients[id]->get_att(), clients[id]->get_exp(), clients[id]->get_level(), clients[id]->get_obj_type());
				clients[cl]->get_viewList_lock().lock();
				clients[cl]->insert_viewList(id);
				clients[cl]->get_viewList_lock().unlock();
			}
		}
	}

	for (auto cl : old_view_list) {
		if (0 != new_view_list.count(cl)) { // new 뷰리스트에 있으면서 
			if (false == is_NPC(cl)) { // NPC가 아닌플레이어
				//send_pos_packet(cl, id); // 내가 움직인 위치 전송
				m_PacketSend->send_pos(cl, clients[cl]->get_socket(), id, 
					clients[id]->get_xPos(), clients[id]->get_yPos());
			}
		}
		else { // new view리스트에 없으면 remove 패킷전송
			m_PacketSend->send_remove_player(id, clients[id]->get_socket(), cl);
			clients[id]->get_viewList_lock().lock();
			clients[id]->delete_viewList(cl);
			clients[id]->get_viewList_lock().unlock();

			if (false == is_NPC(cl)) {
				m_PacketSend->send_remove_player(cl, clients[cl]->get_socket(), id);
				clients[cl]->get_viewList_lock().lock();
				clients[cl]->delete_viewList(id);
				clients[cl]->get_viewList_lock().unlock();
			}
		}
	}

	clients[id]->set_moveCooltime(true);
	EVENT ev_move_cooltime{ id, 0, EV_MOVE_COOLTIME, chrono::high_resolution_clock::now() + 500ms };
	addEvent(ev_move_cooltime);
}

void ServerMain::process_npc_move(int npc_id)
{
	/*if (0 == clients.count(npc_id)) {
		cout << "npc id: " << npc_id << "does not exist \n";
		while (true);
	}*/
	if (false == is_NPC(npc_id)) {  // npc가 아닐경우
		cout << "ID" << npc_id << "is not NPC \n";
		while (true);
	}
	if (true == clients[npc_id]->get_isDead()) { return; }

	bool player_exist = false;
	for (int i = 0; i < NPC_ID_START; ++i) {
		if (false == clients[i]->get_isLive()) { // 없는 id
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
			m_PacketSend->send_remove_player(vl, clients[vl]->get_socket(), npc_id);
			clients[vl]->get_viewList_lock().lock();
			clients[vl]->delete_viewList(npc_id);
			clients[vl]->get_viewList_lock().unlock();
		}
	}
	for (auto &cl : clients) {
		if (false == cl->get_isLive()) { continue; }
		if (true == cl->get_isDead()) { continue; }
		if (true == is_NPC(cl->get_id())) { continue; } // npc에게는 npc의 이동을 보내지않음
		if (false == is_near(cl->get_id(), npc->get_id())) { continue; } // 플레이어가 시야안에 없다면 보내지않는다.
		// 시야안으로 새로 들어온 플레이어에게 put
		if (cl->viewList_count(npc->get_id()) == 0) {
			m_PacketSend->send_put_player(cl->get_id(), cl->get_socket(), npc->get_id(), npc->get_xPos(), npc->get_yPos(),
				npc->get_hp(), npc->get_att(), npc->get_exp(), npc->get_level(), npc->get_obj_type());
			cl->get_viewList_lock().lock();
			cl->insert_viewList(npc->get_id());
			cl->get_viewList_lock().unlock();
		}
		else {
			m_PacketSend->send_pos(cl->get_id(), cl->get_socket(), npc->get_id(), npc->get_xPos(), npc->get_yPos());
		}
		
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
			EVENT ev_move_target{ npc_id, 0, EV_MOVE_TARGET, chrono::high_resolution_clock::now() + 1s }; // 이동후 다시 1초뒤 이동하게 타이머add
			addEvent(ev_move_target);
			return;
		}
	}


	EVENT new_ev{ npc_id, 0, EV_MOVE, chrono::high_resolution_clock::now() + 1s }; // 이동후 다시 1초뒤 이동하게 타이머add
	addEvent(new_ev);

	if (TYPE_WAR_MONSTER == clients[npc_id]->get_obj_type()) { // war_monster 공격
		short x = clients[npc_id]->get_xPos();
		short y = clients[npc_id]->get_yPos();
		for (int i = 0; i < NPC_ID_START; ++i) {
			if (true == clients[i]->get_isLive()) {
				short px = clients[i]->get_xPos();
				short py = clients[i]->get_yPos();
				short att = clients[npc_id]->get_att();
				if (x == px && y - 1 == py) { // 위
					clients[i]->set_hp(clients[i]->get_hp() - att);
					m_PacketSend->send_stat_change(i, clients[i]->get_socket(), clients[i]->get_hp(), clients[i]->get_exp(),
						clients[i]->get_level());
				}
				if (x - 1 == px && y == py) { // 왼
					clients[i]->set_hp(clients[i]->get_hp() - att);
					m_PacketSend->send_stat_change(i, clients[i]->get_socket(), clients[i]->get_hp(), clients[i]->get_exp(),
						clients[i]->get_level());
				}
				if (x + 1 == px && y == py) { // 오른
					clients[i]->set_hp(clients[i]->get_hp() - att);
					m_PacketSend->send_stat_change(i, clients[i]->get_socket(), clients[i]->get_hp(), clients[i]->get_exp(),
						clients[i]->get_level());
				}
				if (x == px && y + 1 == py) { // 아래
					clients[i]->set_hp(clients[i]->get_hp() - att);
					m_PacketSend->send_stat_change(i, clients[i]->get_socket(), clients[i]->get_hp(), clients[i]->get_exp(),
						clients[i]->get_level());
				}
				if (x == px && y == py) { // 겹친상태
					clients[i]->set_hp(clients[i]->get_hp() - att);
					m_PacketSend->send_stat_change(i, clients[i]->get_socket(), clients[i]->get_hp(), clients[i]->get_exp(),
						clients[i]->get_level());
				}
				// 플레이어 사망시 부활 이벤트 추가
				if (0 >= clients[i]->get_hp() && clients[i]->get_isDead() == false) {
					clients[i]->set_isDead(true);
					m_PacketSend->send_player_dead(i, clients[i]->get_socket());
					EVENT ev_respawn{ i, 0, EV_PLAYER_RESPAWN, chrono::high_resolution_clock::now() + 5s };
					addEvent(ev_respawn);
					for (auto vl : clients[i]->get_viewList()) {
						if (false == is_NPC(vl)) {
							m_PacketSend->send_remove_player(vl, clients[vl]->get_socket(), i);
							clients[vl]->get_viewList_lock().lock();
							clients[vl]->delete_viewList(i);
							clients[vl]->get_viewList_lock().unlock();
						}
					}
				}
			}
		}
	}
}

void ServerMain::process_npc_chase(int npc_id, int target_id)
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
			m_PacketSend->send_remove_player(vl, clients[vl]->get_socket(), npc_id);
			clients[vl]->get_viewList_lock().lock();
			clients[vl]->delete_viewList(npc_id);
			clients[vl]->get_viewList_lock().unlock();
		}
	}
	for (auto &cl : clients) {
		if (false == cl->get_isLive()) { continue; }
		if (true == cl->get_isDead()) { continue; }
		if (true == is_NPC(cl->get_id())) { continue; } // npc에게는 npc의 이동을 보내지않음
		if (false == is_near(cl->get_id(), npc_id)) { continue; } // 플레이어가 시야안에 없다면 보내지않는다.
		m_PacketSend->send_pos(cl->get_id(), cl->get_socket(), clients[npc_id]->get_id(), clients[npc_id]->get_xPos(), clients[npc_id]->get_yPos());
	}

	short x = clients[npc_id]->get_xPos();
	short y = clients[npc_id]->get_yPos();
	for (int i = 0; i < NPC_ID_START; ++i) {
		if (true == clients[i]->get_isLive()) {
			short px = clients[i]->get_xPos();
			short py = clients[i]->get_yPos();
			short att = clients[npc_id]->get_att();
			if (x == px && y - 1 == py) { // 위
				clients[i]->set_hp(clients[i]->get_hp() - att);
				m_PacketSend->send_stat_change(i, clients[i]->get_socket(), clients[i]->get_hp(), clients[i]->get_exp(),
					clients[i]->get_level());
				m_PacketSend->send_battle_mess(npc_id, -1, i, clients[i]->get_socket(), att);

			}
			if (x - 1 == px && y == py) { // 왼
				clients[i]->set_hp(clients[i]->get_hp() - att);
				m_PacketSend->send_stat_change(i, clients[i]->get_socket(), clients[i]->get_hp(), clients[i]->get_exp(),
					clients[i]->get_level());
				m_PacketSend->send_battle_mess(npc_id, -1, i, clients[i]->get_socket(), att);
			}
			if (x + 1 == px && y == py) { // 오른
				clients[i]->set_hp(clients[i]->get_hp() - att);
				m_PacketSend->send_stat_change(i, clients[i]->get_socket(), clients[i]->get_hp(), clients[i]->get_exp(),
					clients[i]->get_level());
				m_PacketSend->send_battle_mess(npc_id, -1, i, clients[i]->get_socket(), att);
			}
			if (x == px && y + 1 == py) { // 아래
				clients[i]->set_hp(clients[i]->get_hp() - att);
				m_PacketSend->send_stat_change(i, clients[i]->get_socket(), clients[i]->get_hp(), clients[i]->get_exp(),
					clients[i]->get_level());
				m_PacketSend->send_battle_mess(npc_id, -1, i, clients[i]->get_socket(), att);
			}
			if (x == px && y == py) { // 겹친상태
				clients[i]->set_hp(clients[i]->get_hp() - att);
				m_PacketSend->send_stat_change(i, clients[i]->get_socket(), clients[i]->get_hp(), clients[i]->get_exp(),
					clients[i]->get_level());
				m_PacketSend->send_battle_mess(npc_id, -1, i, clients[i]->get_socket(), att);
			}
			// 플레이어 사망시 부활 이벤트 추가
			if (0 >= clients[i]->get_hp() && clients[i]->get_isDead() == false) {
				clients[i]->set_isDead(true);
				m_PacketSend->send_player_dead(i, clients[i]->get_socket());
				EVENT ev_respawn{ i, 0, EV_PLAYER_RESPAWN, chrono::high_resolution_clock::now() + 5s };
				addEvent(ev_respawn);
				for (auto vl : clients[i]->get_viewList()) {
					if (false == is_NPC(vl)) {
						m_PacketSend->send_remove_player(vl, clients[vl]->get_socket(), i);
						clients[vl]->get_viewList_lock().lock();
						clients[vl]->delete_viewList(i);
						clients[vl]->get_viewList_lock().unlock();
					}
				}
			}
		}
	}

	if (true == clients[target_id]->get_isDead()) {
		EVENT new_ev{ npc_id, 0, EV_MOVE, chrono::high_resolution_clock::now() + 1s };
		addEvent(new_ev);
		return;
	}

	EVENT ev_move_target{ npc_id, 0, EV_MOVE_TARGET, chrono::high_resolution_clock::now() + 1s }; // 이동후 다시 1초뒤 이동하게 타이머add
	addEvent(ev_move_target);
}

void ServerMain::process_heal_event(int id)
{
	if (100 > clients[id]->get_hp() && clients[id]->get_isDead() == false) {
		clients[id]->set_hp(clients[id]->get_hp() + 10);
		m_PacketSend->send_stat_change(id, clients[id]->get_socket(), clients[id]->get_hp(), clients[id]->get_exp(),
			clients[id]->get_level());
	}
	EVENT ev_heal{ id, 0, EV_HEAL, chrono::high_resolution_clock::now() + 5s };
	addEvent(ev_heal);
}

void ServerMain::process_npc_respawn_event(int id)
{
	clients[id]->set_hp(100);
	clients[id]->set_isDead(false);
	EVENT ev_move{ id, 0, EV_MOVE, chrono::high_resolution_clock::now() + 1s };
	addEvent(ev_move);
}

void ServerMain::process_player_respawn_event(int id)
{
	clients[id]->set_xPos(start_posX);
	clients[id]->set_yPos(start_posY);
	clients[id]->set_hp(100);
	clients[id]->set_att(60);
	clients[id]->set_exp(clients[id]->get_exp() / 2);
	clients[id]->set_level(1);
	clients[id]->set_isDead(false);
	m_PacketSend->send_put_player(id, clients[id]->get_socket(), id, clients[id]->get_xPos(), clients[id]->get_yPos(),
		clients[id]->get_hp(), clients[id]->get_att(), clients[id]->get_exp(), clients[id]->get_level(), clients[id]->get_obj_type());
	m_PacketSend->send_stat_change(id, clients[id]->get_socket(), clients[id]->get_hp(), clients[id]->get_exp(), clients[id]->get_level());
	for (auto& cl : clients) {
		if (false == cl->get_isLive()) { continue; }

		int other_player = cl->get_id();

		if (true == is_near(id, other_player))
		{
			if (false == is_NPC(other_player)) { // npc가 아닌경우에
				m_PacketSend->send_put_player(other_player, cl->get_socket(), id, clients[id]->get_xPos(),
					clients[id]->get_yPos(), clients[id]->get_hp(), clients[id]->get_att(), clients[id]->get_exp(),
					clients[id]->get_level(), clients[id]->get_obj_type()); // 내 로그인 정보를 다른클라이언트에 전송
				// 뷰리스트에 없던 id이면 삽입
				cl->get_viewList_lock().lock();
				if (cl->get_id() != id) {
					if (cl->viewList_count(id) == 0) {
						cl->insert_viewList(id);
					}
				}
				cl->get_viewList_lock().unlock();
			}

			if (other_player != id) { // 다른 클라이언트 정보도 나에게 전송
				m_PacketSend->send_put_player(id, clients[id]->get_socket(), other_player, cl->get_xPos(),
					cl->get_yPos(), cl->get_hp(), cl->get_att(), cl->get_exp(), cl->get_level(), cl->get_obj_type());
				// 뷰리스트에 없던 id이면 삽입
				clients[id]->get_viewList_lock().lock();
				if (id != other_player) {
					if (clients[id]->viewList_count(other_player) == 0) {
						clients[id]->insert_viewList(other_player);
					}
				}
				clients[id]->get_viewList_lock().unlock();
			}
		}
	}
}

void ServerMain::process_npc_meet_event(int npc_id, int player_id)
{
	if (clients[npc_id]->get_xPos() == clients[player_id]->get_xPos() &&
		clients[npc_id]->get_yPos() == clients[player_id]->get_yPos()) {
		char chat[6] = "hello";
		m_PacketSend->send_npc_chat(npc_id, player_id, clients[player_id]->get_socket(), chat);
	}
}

bool ServerMain::is_NPC(int id)
{
	return id >= NPC_ID_START;
}

bool ServerMain::is_active(int id)
{
	return clients[id]->get_isActive();
}

bool ServerMain::is_near_npc(int my_id, int npc_id)
{
	if (abs(clients[my_id]->get_xPos() - clients[npc_id]->get_xPos()) < 16 && abs(clients[my_id]->get_yPos() - clients[npc_id]->get_yPos()) < 16) {
		return true;
	}
	else
		return false;
}

bool ServerMain::is_near(int my_id, int other_id)
{
	if (abs(clients[my_id]->get_xPos() - clients[other_id]->get_xPos()) < 8 && abs(clients[my_id]->get_yPos() - clients[other_id]->get_yPos()) < 8) {
		return true;
	}
	else
		return false;
}

bool ServerMain::in_aggro_range(int npc_id, int player)
{
	if (abs(clients[npc_id]->get_xPos() - clients[player]->get_xPos()) < 4 && abs(clients[npc_id]->get_yPos() - clients[player]->get_yPos()) < 4) {
		return true;
	}
	else
		return false;
}

void ServerMain::sock_errDisplay(const char *msg, int err_code)
{
	WCHAR *lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::cout << msg;
	std::wcout << L"에러 " << lpMsgBuf << std::endl;
	while (true);
	LocalFree(lpMsgBuf);
}