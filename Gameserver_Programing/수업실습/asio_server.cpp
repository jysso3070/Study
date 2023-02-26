#include <SDKDDKVer.h>
#include <iostream>
#include <thread>
#include <vector>
#include <set>
#include <mutex>
#include <chrono>
#include <queue>
#include <atomic>

#include <boost/asio.hpp>
#include "..\..\2019_iocp\2019_iocp\protocol.h"

using boost::asio::ip::tcp;

std::atomic_int g_user_ID;

const auto X_START_POS = 4;
const auto Y_START_POS = 4;

class session;

struct player {
	bool connected;
	std::chrono::system_clock::time_point start_time;
	class session *my_session;
	int pos_x;
	int pos_y;
};

player players[NPC_ID_START];


void Init_Server()
{
	_wsetlocale(LC_ALL, L"korean");

	for (int i = 0; i < NPC_ID_START; ++i) {
		players[i].connected = false;
	}
}


int GetNewClientID()
{
	if (g_user_ID >= NPC_ID_START) {
		std::cout << "MAX USER FULL\n";
		exit(-1);
	}
	return g_user_ID++;
}

class session
{
private:
	tcp::socket socket_;
	enum { max_length = 1024 };
	unsigned char data_[max_length];
	unsigned char packet_[max_length];
	int curr_packet_size_;
	int prev_data_size_;
	int my_id_;

	void Send_Packet(void *packet, unsigned id)
	{
		int packet_size = reinterpret_cast<unsigned char *>(packet)[0];
		unsigned char *buff = new unsigned char[packet_size];
		memcpy(buff, packet, packet_size);
		players[id].my_session->do_write(buff, packet_size);
	}

	void Process_Packet(unsigned char *packet, int id)
	{
		player *P = &players[id];
		int y = P->pos_y;
		int x = P->pos_x;
		switch (packet[1]) {
		case CS_UP: y--; if (y < 0) y = 0; break;
		case CS_DOWN: y++; if (y >= WORLD_WIDTH) y = WORLD_WIDTH - 1; break;
		case CS_LEFT: x--; if (x < 0) x = 0; break;
		case CS_RIGHT: x++; if (x >= WORLD_HEIGHT) x = WORLD_HEIGHT - 1; break;
		default: std::cout << "Invalid Packet From Client [" << id << "]\n"; system("pause"); exit(-1);
		}
		P->pos_x = x;
		P->pos_y = y;

		sc_packet_pos sp;

		sp.id = id;
		sp.size = sizeof(sc_packet_pos);
		sp.type = SC_POS;
		sp.x = P->pos_x;
		sp.y = P->pos_y;

		for (int i = 0; i < NPC_ID_START; ++i) {
			if (true != players[i].connected) continue;
			Send_Packet(&sp, i);
		}
	}

	void do_read()
	{
		socket_.async_read_some(boost::asio::buffer(data_, max_length),
			[this](boost::system::error_code ec, std::size_t length)
			{
				if (ec)
				{
					if (ec.value() == boost::asio::error::operation_aborted) return;
					if (false == players[my_id_].connected) return;
					players[my_id_].connected = false;
					std::cout << "Receive Error on Session[" << my_id_ << "] EC[" << ec << "]\n";
					socket_.shutdown(socket_.shutdown_both);
					socket_.close();
					sc_packet_remove_player p;
					p.id = my_id_;
					p.size = sizeof(p);
					p.type = SC_REMOVE_PLAYER;
					for (int i = 0; i < g_user_ID; ++i)
						if (true == players[i].connected)
							Send_Packet(&p, i);
					// socket_ = boost::asio::ip::tcp::socket(*io_service_);
					return;
				}

				int data_to_process = static_cast<int>(length);
				unsigned char * buf = data_;
				while (0 < data_to_process) {
					if (0 == curr_packet_size_) {
						curr_packet_size_ = buf[0];
						if (buf[0] > 200) {
							std::cout << "Invalid Packet Size [ << buf[0] << ] Terminating Server!\n";
							exit(-1);
						}
					}
					int need_to_build = curr_packet_size_ - prev_data_size_;
					if (need_to_build <= data_to_process) {
						// 패킷 조립
						memcpy(packet_ + prev_data_size_, buf, need_to_build);
						Process_Packet(packet_, my_id_);
						curr_packet_size_ = 0;
						prev_data_size_ = 0;
						data_to_process -= need_to_build;
						buf += need_to_build;
					}
					else {
						// 훗날을 기약
						memcpy(packet_ + prev_data_size_, buf, data_to_process);
						prev_data_size_ += data_to_process;
						data_to_process = 0;
						buf += data_to_process;
					}
				}
				do_read();
			});
	}

	void do_write(unsigned char *packet, std::size_t length)
	{
		socket_.async_write_some(boost::asio::buffer(packet, length),
			[this, packet, length](boost::system::error_code ec, std::size_t bytes_transferred)
			{
				if (!ec)
				{
					if (length != bytes_transferred) {
						std::cout << "Incomplete Send occured on session[" << my_id_ << "]. This session should be closed.\n";
					}
					delete packet;
				}
			});
	}


public:
	session(tcp::socket &new_socket) : socket_(std::move(new_socket))
	{
		curr_packet_size_ = 0;
		prev_data_size_ = 0;
	}

	void start()
	{
		my_id_ = GetNewClientID();
		if (99 == (my_id_ % 100)) std::cout << "Client[" << my_id_ + 1 << "] Connected\n";
		players[my_id_].my_session = this;
		players[my_id_].connected = true;
		players[my_id_].start_time = std::chrono::system_clock::now();

		do_read();

		sc_packet_login_ok lo_p;
		lo_p.id = my_id_;
		lo_p.size = sizeof(lo_p);
		lo_p.type = SC_LOGIN_OK;
		Send_Packet(&lo_p, my_id_);

		sc_packet_put_player p;
		p.id = my_id_;
		p.size = sizeof(sc_packet_put_player);
		p.type = SC_PUT_PLAYER;
		p.x = players[my_id_].pos_x;
		p.y = players[my_id_].pos_y;

		// 나의 접속을 모든 플레이어에게 알림
		player *P = &players[my_id_];
		for (int i = 0; i < NPC_ID_START; ++i) {
			if (i == my_id_) continue;
			if (true == players[i].connected) {
				Send_Packet(&p, i);
			}
		}
		// 나에게 접속해 있는 다른 플레이어 정보를 전송
		// 나에게 주위에 있는 NPC의 정보를 전송
		for (int i = 0; i < NPC_ID_START; ++i) {
			if (true == players[i].connected)
				if (i != my_id_) {
					p.id = i;
					p.x = players[i].pos_x;
					p.y = players[i].pos_y;
					Send_Packet(&p, my_id_);
				}
		}
	}
};

class server
{
public:
	server(boost::asio::io_context& io_context, short port)
		: acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
	{
		do_accept();
	}

private:
	void do_accept()
	{
		acceptor_.async_accept(
			[this](boost::system::error_code ec, tcp::socket socket)
			{
				if (!ec)
				{
					int new_id = GetNewClientID();
					class session *new_session = new session(socket);
					players[new_id].my_session = new_session;
					players[new_id].my_session->start();
				}

				do_accept();
			});
	}

	tcp::acceptor acceptor_;
};

void worker_thread(boost::asio::io_context *service)
{
	service->run();
}

int main()
{
	boost::asio::io_context io_service;
	std::vector <std::thread> worker_threads;

	Init_Server();

	server s(io_service, SERVER_PORT);

	for (auto i = 0; i < 4; i++)
		worker_threads.emplace_back(worker_thread, &io_service );
	for (auto &th : worker_threads) { th.join();}
}