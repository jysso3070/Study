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
	mutex n_lock;

	NODE() { next = nullptr; }
	NODE(int x) {
		next = nullptr;
		key = x;
	}
	~NODE() {}
	void Lock() {
		n_lock.lock();
	}
	void Unlock() {
		n_lock.unlock();
	}
};

class LFNODE {
private:
	atomic_int next;
public:
	int key;

	LFNODE() { next = 0; }
	LFNODE(int x) {
		key = x;
		next = 0;
	}
	~LFNODE() {}

	void set_next(LFNODE* addr, bool is_removed)
	{
		int value = reinterpret_cast<int>(addr);
		if (true == is_removed) value = value | 1;
		next = value;
	}
	LFNODE* get_next()
	{
		return reinterpret_cast<LFNODE*>(next & 0xFFFFFFFE);
	}
	LFNODE* get_next(bool *is_removed)
	{
		int value = next;
		*is_removed = (0 != (value & 1));
		return reinterpret_cast<LFNODE*>(next & 0xFFFFFFFE);
	}

	bool CAS_NEXT(LFNODE* old_addr, LFNODE* new_addr, bool old_mark, bool new_mark)
	{
		int old_value = reinterpret_cast<int>(old_addr);
		if (true == old_mark) old_value = old_value | 1;

		int new_value = reinterpret_cast<int>(new_addr);
		if (true == new_mark) new_value = new_value | 1;

		return atomic_compare_exchange_strong(reinterpret_cast<atomic_int*>(&next), &old_value, new_value);
	}

	bool AttempMark()
	{
		int oldvalue = next;
		if (0 != (oldvalue & 1)) return false;
		int newvalue = oldvalue | 1;
		return atomic_compare_exchange_strong(reinterpret_cast<atomic_int*>(&next), &oldvalue, newvalue);
	}

};

class LFLIST {
	LFNODE head, tail;

public:
	LFLIST()
	{
		head.key = 0x8000'0000;
		tail.key = 0x7FFF'FFFF;
		head.set_next(&tail, false);
	}
	~LFLIST() {}
	void clear()
	{
		LFNODE* ptr = head.get_next();
		while (ptr != &tail) {
			LFNODE* to_delete = ptr;
			ptr = ptr->get_next();
			delete to_delete;
		}
		head.set_next(&tail, false);
	}

	void FIND(int key, LFNODE **pred, LFNODE **curr) 
	{	
	retry:
		LFNODE* pr = &head;
		LFNODE* cu = pr->get_next();

		while (true) {
			// cu가 마킹되어있으면 제거하고 cu를 다시 세팅
			bool is_removed;
			LFNODE* su = cu->get_next(&is_removed);
			while (true == is_removed) {
				if (false == pr->CAS_NEXT(cu, su, false, false)) { // cas 실패했을때
					goto retry;
				}
				cu = su;
				su = cu->get_next(&is_removed);
			}
			if (cu->key >= key) {
				*pred = pr;
				*curr = cu;
				return;
			}
			pr = cu;
			cu = su;
		}
	}
	bool Add(int x)
	{
		while (true) {
			LFNODE *pred, *curr;
			FIND(x, &pred, &curr);

			if (curr->key == x) {
				return false;
			}
			else {
				LFNODE* new_node = new LFNODE(x);
				new_node->set_next(curr, false);
				if (true == pred->CAS_NEXT(curr, new_node, false, false)) {
					return true;
				}
			}
		}
	}
	bool Remove(int x)
	{
		while (true) {
			LFNODE* prev, * curr;
			FIND(x, &prev, &curr);

			if (curr->key != x) {
				return false;
			}
			else {
				LFNODE* su = curr->get_next();
				bool snip = curr->CAS_NEXT(su, su, false, true);
				if (false == snip)
				{
					continue;
				}
				prev->CAS_NEXT(curr, su, false, false);
				return true;
			}
		}

	}
	bool Contains(int x)
	{
		while (true) {
			bool is_removed = false;
			LFNODE* curr = &head;
			while (curr->key < x) {
				curr = curr->get_next(&is_removed);
			}
			return curr->key == x && !is_removed;
		}
	}
	void display20() {
		LFNODE* ptr = head.get_next();
		for (int i = 0; i < 20; ++i) {
			if (&tail == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->get_next();
		}
		cout << endl;
	}
};


const auto NUM_TEST = 400'0000;
const auto KEY_RANGE = 1000;
LFLIST my_set;

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