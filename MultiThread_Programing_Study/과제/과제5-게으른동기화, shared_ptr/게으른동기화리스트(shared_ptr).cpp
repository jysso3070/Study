#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
#include <vector>
#include <random>

using namespace std;
using namespace chrono;


class SPNODE {
public:
	int key;
	shared_ptr<SPNODE> next;
	mutex n_lock;
	bool marked;

	SPNODE() {
		marked = false;
	}
	SPNODE(int x) {
		key = x;
		marked = false;
	}
	~SPNODE() {}
	void Lock() {
		n_lock.lock();
	}
	void Unlock() {
		n_lock.unlock();
	}
};

class null_mutex {
public:
	void Lock() {};
	void Unlock() {};
};

class SPLLIST {
	shared_ptr<SPNODE> head, tail;
	//mutex m_lock;
public:
	SPLLIST()
	{
		head = make_shared<SPNODE>(0x8000'0000);
		tail = make_shared<SPNODE>(0x7FFF'FFFF);
		head->next = tail;
	}
	~SPLLIST() {}
	void clear()
	{
		head->next = tail;
	}
	// 오버헤드를 줄이기 위해 const와 래퍼런스로 넘김
	bool validate(const shared_ptr<SPNODE> &pred, const shared_ptr<SPNODE> &curr) {
		return !pred->marked && !curr->marked && pred->next == curr;
	}
	bool Add(int x)
	{
		while (true) {
			shared_ptr<SPNODE> pred = head;
			shared_ptr<SPNODE> curr = pred->next;
			while (curr->key < x) {
				pred = curr;
				curr = curr->next;
			}
			pred->Lock();
			curr->Lock();

			if (false == validate(pred, curr)) { // validate 가 false일때 락 걸어둔 노드 언락
				curr->Unlock();
				pred->Unlock();
				continue;
			}
			if (curr->key == x) {
				curr->Unlock();
				pred->Unlock();
				return false;
			}
			else {
				shared_ptr<SPNODE> new_node = make_shared<SPNODE>(x);
				new_node->next = curr;
				pred->next = new_node;
				curr->Unlock();
				pred->Unlock();
				return true;
			}
		}
	}
	bool Remove(int x)
	{
		while (true) {
			shared_ptr<SPNODE> pred = head;
			shared_ptr<SPNODE> curr = pred->next;
			while (curr->key < x) {
				pred = curr;
				curr = curr->next;
			}
			pred->Lock();
			curr->Lock();

			if (false == validate(pred, curr)) { // validate 가 false일때 락 걸어둔 노드 언락
				curr->Unlock();
				pred->Unlock();
				continue;
			}
			if (curr->key != x) {
				curr->Unlock();
				pred->Unlock();
				return false;
			}
			else {
				pred->next = curr->next;
				curr->Unlock();
				pred->Unlock();
				return true;
			}
		}
	}
	bool Contains(int x)
	{
		shared_ptr<SPNODE> curr = head;
		while (curr->key < x) {
			curr = curr->next;
		}
		return curr->key == x && !curr->marked;
	}

	void display20() {
		shared_ptr<SPNODE> ptr = head->next;
		for (int i = 0; i < 20; ++i) {
			if (tail == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << endl;
	}
	
};


const auto NUM_TEST = 400'0000;
const auto KEY_RANGE = 1000;
SPLLIST my_set;

void benchmark(int num_threads)
{
	for (int i = 0; i < NUM_TEST / num_threads; ++i) {
		switch (rand() % 3) {
		case 0:
			my_set.Add(rand() % KEY_RANGE);
			break;
		case 1:
			my_set.Remove(rand() % KEY_RANGE);
			break;
		case 2:
			my_set.Contains(rand() % KEY_RANGE);
			break;
		default: cout << "Error\n";
			exit(-1);
		}
	}
}

int main() {
	// 스레드 개수 바꿔가면서 벤치마크 실행
	my_set.clear();

	for (int count = 1; count <= 8; count *= 2) {
		vector <thread> threads;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < count; ++i)
			threads.emplace_back(benchmark, count);

		for (auto& t : threads)
			t.join();

		auto end_t = high_resolution_clock::now();
		auto exec_time = end_t - start_t;
		cout << "Number of Threads = " << count << ", ";
		cout << "Exec Time = " << duration_cast<milliseconds>(exec_time).count() << "ms, \n";
		my_set.display20();
		my_set.clear();
	}
}