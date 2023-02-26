#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <set>

using namespace std;
using namespace chrono;


class NODE {
public:
	int key;
	NODE* volatile next;

	NODE(){
		next = nullptr;
	}
	NODE(int x){
		key = x;
		next = nullptr;
	}
	~NODE()
	{
	}
};

class null_mutex
{
public:
	void lock()
	{
	}
	void unlock()
	{
	}
};

// ���䵿��ȭ
class CQUEUE {
	NODE *head, *tail;
	mutex deq_lock, enq_lock;
	//null_mutex m_lock;
public:
	CQUEUE()
	{
		head = tail = new NODE();
	}
	~CQUEUE()
	{
		clear();
		delete head;
	}

	void clear()
	{
		while (head != tail) {
			NODE* to_delete = head;
			head = head->next;
			delete to_delete;
		}
	}

	void Enq(int x)
	{
		NODE* new_node = new NODE(x);
		enq_lock.lock();
		tail->next = new_node;
		tail = new_node;
		enq_lock.unlock();
	}

	int Deq()
	{
		int result;
		deq_lock.lock();
		if (head->next == nullptr) {
			deq_lock.unlock();
			return -1;
		}
		else {
			NODE* to_delete = head;
			result = head->next->key;
			head = head->next;
			deq_lock.unlock();
			delete to_delete;
			return result;
		}
	}

	void display20()
	{
		NODE* ptr = head->next;
		for (int i = 0; i < 20; ++i) {
			if (nullptr == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << endl;
	}
};

// ������ ������ ť
class LFQUEUE {
	NODE* volatile head;
	NODE* volatile tail;
	//null_mutex m_lock;
public:
	LFQUEUE()
	{
		head = tail = new NODE();
	}
	~LFQUEUE()
	{
		clear();
		delete head;
	}

	void clear()
	{
		while (head != tail) {
			NODE* to_delete = head;
			head = head->next;
			delete to_delete;
		}
	}

	bool CAS(NODE* volatile* next, NODE* old_p, NODE* new_p) {
		return atomic_compare_exchange_strong(reinterpret_cast<volatile atomic_int*>(next),
			reinterpret_cast<int*>(&old_p), reinterpret_cast<int>(new_p));
	}

	void Enq(int x)
	{
		NODE* new_node = new NODE(x);
		while (true) {
			NODE* last = tail;
			NODE* next = last->next;
			if (last != tail) continue;
			if (nullptr == next) {
				if (CAS(&last->next, nullptr, new_node)) {
					CAS(&tail, last, new_node);
					return;
				}
			}
			else {
				CAS(&tail, last, next);
			}
		}

	}

	int Deq()
	{
		while (true) {
			NODE* first = head;
			NODE* last = tail;
			NODE* next = first->next;
			if (head != first) continue;
			if (nullptr == next) return -1; //empty
			if (last == first) {
				CAS(&tail, last, next);
				continue;
			}
			int value = next->key;
			if (false == CAS(&head, first, next)) continue;
			delete first;
			return value;
		}
	}

	void display20()
	{
		NODE* ptr = head->next;
		for (int i = 0; i < 20; ++i) {
			if (nullptr == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << endl;
	}
};

class STPTR {
public:
	long long ptr;
	STPTR() { ptr = 0; }
	STPTR(NODE* p, int stamp) {
		ptr = stamp;
		ptr = ptr << 32;
		ptr = ptr + reinterpret_cast<int>(p);
	}
	void set_ptr(NODE* p, int stamp) {
		ptr = reinterpret_cast<int>(p) + (stamp << 32);
	}
	NODE* get_addr() {
		return reinterpret_cast<NODE*>(ptr);
	}
	NODE* get_addr(int *stamp) {
		long long p = ptr;
		*stamp = p >> 32;
		return reinterpret_cast<NODE*>(p);
	}
	int get_stamp()
	{
		long long p = ptr;
		return p >> 32;
	}
	~STPTR(){}
};

class FREELIST
{
public:
	NODE* head;
	NODE* tail;

	FREELIST() {
		head = tail = new NODE(0);
	}
	~FREELIST() {
		while (nullptr != head) {
			NODE* p = head;
			head = head->next;
			delete p;
		}
	}

	NODE* get_node(int x) {
		if (head == tail) return new NODE(x);
		NODE* p = head;
		head = head->next;
		p->key = x;
		p->next = nullptr;
		return p;
	}
	void free_node(NODE* p) {
		p->next = nullptr;
		tail->next = p;
		tail = p;
	}
};

thread_local FREELIST freelist;

class STLFQUEUE {
	STPTR head;
	STPTR tail;
public:
	STLFQUEUE()
	{
		NODE* p = new NODE();
		head = tail = STPTR(p, 0 );
	}
	~STLFQUEUE()
	{
		clear();
		delete head.get_addr();
	}

	void clear()
	{
		while (head.get_addr() != tail.get_addr()) {
			NODE* to_delete = head.get_addr();
			head.set_ptr(head.get_addr()->next, 0);
			delete to_delete;
		}
	}

	bool CAS(NODE* volatile * next, NODE* old_p, NODE* new_p) {
		return atomic_compare_exchange_strong(
			reinterpret_cast<volatile atomic_int*>(next),
			reinterpret_cast<int*>(&old_p),
			reinterpret_cast<int>(new_p));
	}

	bool CAS(STPTR* next, NODE* old_p, NODE* new_p, int old_st, int new_st) {
		STPTR old_v{ old_p, old_st };
		STPTR new_v{ new_p, new_st };
		long long ov = old_v.ptr;
		long long nv = new_v.ptr;
		return atomic_compare_exchange_strong(
			reinterpret_cast<volatile atomic_llong*>(next),
			&ov, nv);
	}

	void Enq(int key)
	{
		////NODE* e = new NODE(key);
		NODE* e = freelist.get_node(key);
		while (true) {
			int laststamp;
			NODE* last = tail.get_addr(&laststamp);
			NODE* next = last->next;
			if (laststamp != tail.get_stamp()) continue;
			if (nullptr != next) {
				CAS(&tail, last, next, laststamp, laststamp + 1);
				continue;
			}
			if (false == CAS(&(last->next), next, e))
				continue;
			CAS(&tail, last, e, laststamp, laststamp + 1);
			return;
		}
	}

	int Deq()
	{
		while (true) {
			int firststamp;
			int laststamp;
			NODE* first = head.get_addr(&firststamp);
			NODE* last = tail.get_addr(&laststamp);
			NODE* next = first->next;

			if (firststamp != head.get_stamp()) continue;
			if (nullptr == next) return -1;
			if (first == last) {
				CAS(&tail, last, next, laststamp, laststamp + 1);
				continue;
			}
			int value;
			if (nullptr != next) value = next->key; 
			if (false == CAS(&head, first, next, firststamp, firststamp + 1)) continue;
			freelist.free_node(first);
			return value;
		}
	}

	void display20()
	{
		NODE* ptr = head.get_addr()->next;
		for (int i = 0; i < 20; ++i) {
			if (nullptr == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << endl;
	}
};

constexpr int NUM_TEST = 1000'0000;
constexpr int MAX_THREAD = 8;

STLFQUEUE my_queue;

void benchmark(int num_threads)
{

	for (int i = 0; i < NUM_TEST / num_threads; ++i) {
		if (rand() % 2 == 0 || (i < 2 / num_threads)) {
			my_queue.Enq(i);
		}
		else {
			my_queue.Deq();
		}
	}
}

int main()
{
	for (int num = 1; num <= MAX_THREAD; num = num * 2) {
		vector <thread> threads;
		my_queue.clear();
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < num; ++i)
			threads.emplace_back(benchmark, num);
		for (auto& th : threads) th.join();
		auto end_t = high_resolution_clock::now();
		auto du = end_t - start_t;

		cout << num << " Threads,  ";
		cout << "Exec time " <<
			duration_cast<milliseconds>(du).count() << "ms  ";
		my_queue.display20();
	}
}