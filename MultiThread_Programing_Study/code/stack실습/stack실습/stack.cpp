#include <iostream>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>

using namespace std;
using namespace chrono;

int num_thread;
constexpr int MAX_THREAD = 8;
atomic_int el_success;

class NODE {
public:
	int key;
	NODE* volatile next;

	NODE() :next{ nullptr } {}
	NODE(int key_value) : key{ key_value }, next{ nullptr }{}
	~NODE() {}
};

// 성긴동기화 스택
class CSTACK {
	NODE* top;
	mutex s_lock;

public:
	CSTACK() {
		top = nullptr;
	}
	~CSTACK() {
		clear();
	}

	void clear()
	{
		while (nullptr != top) {
			NODE* to_delete = top;
			top = top->next;
			delete to_delete;
		}
	}

	void Push(int key)
	{
		NODE* e = new NODE(key);
		s_lock.lock();
		e->next = top;
		top = e;
		s_lock.unlock();
	}

	int Pop()
	{
		s_lock.lock();
		if (nullptr == top) {
			s_lock.unlock();
			return -1;
		}
		NODE* p = top;
		int value = p->key;
		top = top->next;
		s_lock.unlock();
		delete p;
		return value;
	}

	void display20()
	{
		NODE* ptr = top;
		for (int i = 0; i < 20; ++i) {
			if (nullptr == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << endl;
	}
};

class LFSTACK {
	NODE* volatile top;

public:
	LFSTACK() : top(nullptr) {}
	~LFSTACK() {
		clear();
	}

	void clear()
	{
		while (nullptr != top) {
			NODE* to_delete = top;
			top = top->next;
			delete to_delete;
		}
	}

	bool CAS(NODE* volatile* ptr, NODE* o_node, NODE* n_node)
	{
		return atomic_compare_exchange_strong(
			reinterpret_cast<volatile atomic_int*>(ptr),
			reinterpret_cast<int*>(&o_node),
			reinterpret_cast<int>(n_node));
	}

	void Push(int key)
	{
		NODE* e = new NODE(key);
		while (true) {
			NODE* first = top;
			e->next = first;
			if (true == CAS(&top, first, e))
				return;
		}
	}

	int Pop()
	{
		while (true) {
			NODE* first = top;
			if (nullptr == first) {
				return -1; // EMPTY
			}
			NODE* next = first->next;
			if (top != first) continue;
			int value = first->key;
			if (true == CAS(&top, first, first->next)) {
				//delete first;
				return value;
				// ABA문제 때문에 delete는 하지 않는다
			}
		}
	}

	void display20()
	{
		NODE* ptr = top;
		for (int i = 0; i < 20; ++i) {
			if (nullptr == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << endl;
	}
};

// 백오프
class BACKOFF {
	int minDelay, maxDelay;
	int limit;
public:
	void init(int min, int max) {
		minDelay = min;
		maxDelay = max;
		limit = min;
	}

	void backoff() {
		int delay = 0;
		if (limit != 0) delay = rand() % limit;
		//if (0 == limit) return;
		limit *= 2;
		if (limit > maxDelay) limit = maxDelay;

		//this_thread::sleep_for(chrono::microseconds(delay));


		int current, start;
		// 클럭 레지스터를 읽는 명령어 사용
		_asm RDTSC;
		_asm mov start, eax;
		do {
			_asm RDTSC;
			_asm mov current, eax;
		} while (current - start < delay);

		//   _asm mov eax, delay;
		//myloop:
		//   _asm dec eax;
		//   _asm jnz myloop;
	}
};

// 락프리백오프스택
class LFBOSTACK {
	BACKOFF bo;
	NODE* volatile top;
public:
	LFBOSTACK() {
		top = nullptr;
		bo.init(1, 1000);
	}
	~LFBOSTACK() {
		clear();
	}

	void clear()
	{
		while (nullptr != top) {
			NODE* to_delete = top;
			top = top->next;
			delete to_delete;
		}
	}

	bool CAS(NODE* volatile* ptr, NODE* o_node, NODE* n_node)
	{
		return atomic_compare_exchange_strong(
			reinterpret_cast<volatile atomic_int*>(ptr),
			reinterpret_cast<int*>(&o_node),
			reinterpret_cast<int>(n_node));
	}

	void Push(int key)
	{
		NODE* e = new NODE(key);

		while (true) {
			NODE* first = top;
			e->next = first;
			if (true == CAS(&top, first, e))
				return;
			bo.backoff();
		}
	}

	int Pop()
	{
		while (true) {
			NODE* first = top;
			if (nullptr == top) {
				return -2; // EMPTY
			}
			NODE* next = first->next;
			int value = first->key;
			if (top != first) continue;
			if (true == CAS(&top, first, next)) {
				//delete first;
				return value;
			}
			// ABA문제 때문에 delete는 하지 않는다

			bo.backoff();
		}
	}

	void display20()
	{
		NODE* ptr = top;
		for (int i = 0; i < 20; ++i) {
			if (nullptr == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << endl;
	}
};

constexpr int EMPTY_ST = 0;
constexpr int WAITING_ST = 1;
constexpr int BUSY_ST = 2;
// 교환자
class EXCHANGER {
	atomic_int slot;
	int get_state() {
		int t = slot;
		return (t >> 30) & 0x3;
	}
	int get_value() {
		int t = slot;
		return t & 0x3FFFFFFF;
	}
	bool CAS(int old_st, int new_st, int old_v, int new_v) {
		int ov = old_st << 30 + (old_v & 0x3FFFFFFF);
		int nv = new_st << 30 + (new_v & 0x3FFFFFFF);
		bool ret = atomic_compare_exchange_strong(&slot, &ov, nv);
		old_v = ov & 0x3FFFFFFF;
		return ret;
	}
public:
	EXCHANGER() {
		slot = 0;
	}
	~EXCHANGER(){}
	int exchange(int value, bool* time_out, bool* is_busy) {
		for (int i = 0; i < 100; ++i) {
			switch (get_state()) {
			case EMPTY_ST:
			{
				if (true == CAS(EMPTY_ST, WAITING_ST, 0, value)) {
					int counter = 0;
					while (BUSY_ST != get_state()) {
						counter++;
						if (counter > 1000) {
							if (true == CAS(WAITING_ST, EMPTY_ST, value, 0)) {
								*time_out = true;
								return 0;
							} break;;
						}
					}
					int ret = get_value();
					slot = 0;
					return ret;
				}
				else {
					continue;
				}
			}
				break;
			case WAITING_ST:
			{
				int ret = get_value();
				if (true == CAS(WAITING_ST, BUSY_ST, ret, value))
					return -1;
				else continue;
			}
				break;
			case BUSY_ST:
			{
				*is_busy = true;
				continue;
				break;
			}
			}
		}
		*is_busy = true;
		*time_out = true;
		return 0;
	}
};

class ELIMINATION_ARRAY {
	int range;
	EXCHANGER ex[1 + MAX_THREAD / 2];
public:
	ELIMINATION_ARRAY() : range(1){}
	~ELIMINATION_ARRAY(){}
	int visit(int value, bool* time_out) {
		int index = rand() % range;
		bool is_busy = false;
		int ret = ex[index].exchange(value, time_out, &is_busy);
		if (true == *time_out && range > 1) range--;
		if (true == is_busy && range < num_thread / 2) range++;
		return ret;
	}
};

class LFELSTACK {
	NODE* volatile top;
	ELIMINATION_ARRAY el;

public:
	LFELSTACK() {
		top = nullptr;
	}
	~LFELSTACK() {
		clear();
	}

	void clear()
	{
		while (nullptr != top) {
			NODE* to_delete = top;
			top = top->next;
			delete to_delete;
		}
	}

	bool CAS(NODE* volatile* ptr, NODE* o_node, NODE* n_node)
	{
		return atomic_compare_exchange_strong(
			reinterpret_cast<volatile atomic_int*>(ptr),
			reinterpret_cast<int*>(&o_node),
			reinterpret_cast<int>(n_node));
	}

	void Push(int key)
	{
		NODE* e = new NODE(key);

		while (true) {
			NODE* first = top;
			e->next = first;
			if (true == CAS(&top, first, e))
				return;
			bool time_out = false;
			int ret = el.visit(key, &time_out);
			if ((false == time_out) && (-1 == ret))
				return;
		}
	}

	int Pop()
	{
		while (true) {
			NODE* first = top;
			if (nullptr == first) {
				return -1;
			}
			NODE* next = first->next;
			int value = first->key;
			if (top != first) continue;
			if (true == CAS(&top, first, next)) {
				//delete first;
				return value;
				++el_success;
				// ABA문제 때문에 delete는 하지 않는다
			}
		}
	}

	void display20()
	{
		NODE* ptr = top;
		for (int i = 0; i < 20; ++i) {
			if (nullptr == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << endl;
	}
};

constexpr int NUM_TEST = 10000000;


CSTACK my_stack;

void benchmark(int num_threads)
{

	for (int i = 0; i < NUM_TEST / num_threads; ++i) {
		if (rand() % 2 == 0 || (i < 2 / num_threads)) {
			my_stack.Push(i);
		}
		else {
			my_stack.Pop();
		}
	}
}

int main()
{
	for (num_thread = 1; num_thread <= MAX_THREAD; num_thread = num_thread * 2) {
		vector <thread> threads;
		my_stack.clear();
		el_success = 0;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < num_thread; ++i)
			threads.emplace_back(benchmark, num_thread);
		for (auto& th : threads) th.join();
		auto end_t = high_resolution_clock::now();
		auto du = end_t - start_t;

		cout << num_thread << " Threads,  ";
		cout << "Exec time " <<
			duration_cast<milliseconds>(du).count() << "ms  ";
		cout << "el success = " << el_success << endl;
		my_stack.display20();
	}
}