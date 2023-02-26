#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
#include <vector>
#include <random>

using namespace std;
using namespace chrono;


class NODE {
public:
	int key;
	NODE* next;

	NODE() { next = nullptr; }
	NODE(int x) {
		next = nullptr;
		key = x;
	}
	~NODE() {}
};

class CLIST {
	NODE head, tail;
	mutex m_lock;
public:
	CLIST()
	{
		head.key = 0x8000'0000;
		tail.key = 0x7FFF'FFFF;
		head.next = &tail;
	}
	~CLIST() {}
	void clear()
	{
		NODE* ptr = head.next;
		while (ptr != &tail) {
			NODE* to_delete = ptr;
			ptr = ptr->next;
			delete to_delete;
		}
		head.next = &tail;
	}
	bool Add(int x)
	{
		NODE* pred = &head; // head주소는 항상 고정이기때문에 이 밑에 lock
		m_lock.lock();
		NODE* curr = pred->next;
		while (curr->key < x) {
			pred = curr;
			curr = curr->next;
		}

		if (curr->key == x) {
			m_lock.unlock();
			return false;
		}
		else {
			NODE* new_node = new NODE(x);
			new_node->next = curr;
			pred->next = new_node;
			m_lock.unlock();
			return true;
		}

	}
	bool Remove(int x)
	{
		NODE* pred = &head; // head주소는 항상 고정이기때문에 이 밑에 lock
		m_lock.lock();
		NODE* curr = pred->next;
		while (curr->key < x) {
			pred = curr;
			curr = curr->next;
		}

		if (curr->key != x) {
			m_lock.unlock();
			return false;
		}
		else {
			pred->next = curr->next;
			delete curr;
			m_lock.unlock();
			return true;
		}
	}
	bool Contains(int x)
	{
		NODE* pred = &head; // head주소는 항상 고정이기때문에 이 밑에 lock
		m_lock.lock();
		NODE* curr = pred->next;
		while (curr->key < x) {
			pred = curr;
			curr = curr->next;
		}

		if (curr->key != x) {
			m_lock.unlock();
			return false;
		}
		else {
			m_lock.unlock();
			return true;
		}
	}
	void display20() {
		NODE* ptr = head.next;
		for (int i = 0; i < 20 ; ++i) {
			if (&tail == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << endl;
	}
};


const auto NUM_TEST = 400'0000;
const auto KEY_RANGE = 1000;
CLIST my_set;

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