#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <set>

using namespace std;
using namespace chrono;

constexpr int MAX_THREAD = 8;


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

// 성긴동기화
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

//// 세밀한동기화
//class FLIST {
//	NODE head, tail;
//public:
//	FLIST()
//	{
//		head.key = 0x80000000;
//		tail.key = 0x7FFFFFFF;
//		head.next = &tail;
//	}
//	~FLIST()
//	{
//
//	}
//
//	void clear()
//	{
//		NODE* ptr = head.next;
//		while (ptr != &tail) {
//			NODE* to_delete = ptr;
//			ptr = ptr->next;
//			delete to_delete;
//		}
//		head.next = &tail;
//	}
//
//	bool Add(int x)
//	{
//		head.Lock();
//		NODE* pred = &head;
//		NODE* curr = pred->next;
//		curr->Lock();
//		while (curr->key < x) {
//			pred->Unlock();
//			pred = curr;
//			curr = curr->next;
//			curr->Lock();
//		}
//
//		if (curr->key == x) {
//			pred->Unlock();
//			curr->Unlock();
//			return false;
//		}
//		else {
//			NODE* new_node = new NODE(x);
//			new_node->next = curr;
//			pred->next = new_node;
//			pred->Unlock();
//			curr->Unlock();
//			return true;
//		}
//	}
//
//	bool Remove(int x)
//	{
//		head.Lock();
//		NODE* pred = &head;
//		NODE* curr = pred->next;
//		curr->Lock();
//		while (curr->key < x) {
//			pred->Unlock();
//			pred = curr;
//			curr = curr->next;
//			curr->Lock();
//		}
//
//		if (curr->key != x) {
//			pred->Unlock();
//			curr->Unlock();
//			return false;
//		}
//		else {
//			pred->next = curr->next;
//			delete curr;
//			pred->Unlock();
//			// curr->Unlock();
//			return true;
//		}
//	}
//
//	bool Contains(int x)
//	{
//		head.Lock();
//		NODE* pred = &head;
//		NODE* curr = pred->next;
//		curr->Lock();
//		while (curr->key < x) {
//			pred->Unlock();
//			pred = curr;
//			curr = curr->next;
//			curr->Lock();
//		}
//
//		if (curr->key != x) {
//			pred->Unlock();
//			curr->Unlock();
//			return false;
//		}
//		else {
//			pred->Unlock();
//			curr->Unlock();
//			return true;
//		}
//	}
//	void display20()
//	{
//		NODE* ptr = head.next;
//		for (int i = 0; i < 20; ++i) {
//			if (&tail == ptr) break;
//			cout << ptr->key << ", ";
//			ptr = ptr->next;
//		}
//		cout << endl;
//	}
//};
//
//// 낙천적 동기화
//class OLIST {
//	NODE head, tail;
//public:
//	OLIST()
//	{
//		head.key = 0x80000000;
//		tail.key = 0x7FFFFFFF;
//		head.next = &tail;
//	}
//	~OLIST()
//	{
//
//	}
//
//	void clear()
//	{
//		NODE* ptr = head.next;
//		while (ptr != &tail) {
//			NODE* to_delete = ptr;
//			ptr = ptr->next;
//			delete to_delete;
//		}
//		head.next = &tail;
//	}
//
//	bool is_valid(NODE* pred, NODE* curr)
//	{
//		NODE* p = &head;
//		while (p->key <= pred->key) {
//			if (p == pred) {
//				return pred->next == curr;
//			}
//			p = p->next;
//		}
//		return false;
//	}
//
//	bool Add(int x)
//	{
//		while (true) {
//			NODE* pred = &head;
//			NODE* curr = pred->next;
//			while (curr->key < x) {
//				pred = curr;
//				curr = curr->next;
//			}
//
//			pred->Lock();
//			curr->Lock();
//
//			if (false == is_valid(pred, curr))
//			{
//				pred->Unlock();
//				curr->Unlock();
//				continue;
//			}
//
//			if (curr->key == x) {
//				pred->Unlock();
//				curr->Unlock();
//				return false;
//			}
//			else {
//				NODE* new_node = new NODE(x);
//				new_node->next = curr;
//				pred->next = new_node;
//				pred->Unlock();
//				curr->Unlock();
//				return true;
//			}
//		}
//	}
//
//	bool Remove(int x)
//	{
//		while (true) {
//			NODE* pred = &head;
//			NODE* curr = pred->next;
//			while (curr->key < x) {
//				pred = curr;
//				curr = curr->next;
//			}
//
//			pred->Lock();
//			curr->Lock();
//
//			if (false == is_valid(pred, curr))
//			{
//				pred->Unlock();
//				curr->Unlock();
//				continue;
//			}
//
//			if (curr->key != x) {
//				pred->Unlock();
//				curr->Unlock();
//				return false;
//			}
//			else {
//				pred->next = curr->next;
//				pred->Unlock();
//				curr->Unlock();
//				// delete curr;
//				return true;
//			}
//		}
//	}
//
//	bool Contains(int x)
//	{
//		while (true) {
//			NODE* pred = &head;
//			NODE* curr = pred->next;
//			while (curr->key < x) {
//				pred = curr;
//				curr = curr->next;
//			}
//
//			pred->Lock();
//			curr->Lock();
//
//			if (false == is_valid(pred, curr))
//			{
//				pred->Unlock();
//				curr->Unlock();
//				continue;
//			}
//
//			if (curr->key == x) {
//				pred->Unlock();
//				curr->Unlock();
//				return true;
//			}
//			else {
//				pred->Unlock();
//				curr->Unlock();
//				return false;
//			}
//		}
//	}
//
//	void display20()
//	{
//		NODE* ptr = head.next;
//		for (int i = 0; i < 20; ++i) {
//			if (&tail == ptr) break;
//			cout << ptr->key << ", ";
//			ptr = ptr->next;
//		}
//		cout << endl;
//	}
//};
//
//// 게으른 동기화
//class LLIST {
//	NODE head, tail;
//public:
//	LLIST()
//	{
//		head.key = 0x80000000;
//		tail.key = 0x7FFFFFFF;
//		head.next = &tail;
//	}
//	~LLIST()
//	{
//
//	}
//
//	void clear()
//	{
//		NODE* ptr = head.next;
//		while (ptr != &tail) {
//			NODE* to_delete = ptr;
//			ptr = ptr->next;
//			delete to_delete;
//		}
//		head.next = &tail;
//	}
//
//	bool is_valid(NODE* pred, NODE* curr)
//	{
//		return (false == pred->is_removed) &&
//			(false == curr->is_removed) &&
//			pred->next == curr;
//	}
//
//	bool Add(int x)
//	{
//		while (true) {
//			NODE* pred = &head;
//			NODE* curr = pred->next;
//			while (curr->key < x) {
//				pred = curr;
//				curr = curr->next;
//			}
//
//			pred->Lock();
//			curr->Lock();
//
//			if (false == is_valid(pred, curr))
//			{
//				pred->Unlock();
//				curr->Unlock();
//				continue;
//			}
//
//			if (curr->key == x) {
//				pred->Unlock();
//				curr->Unlock();
//				return false;
//			}
//			else {
//				NODE* new_node = new NODE(x);
//				new_node->next = curr;
//				pred->next = new_node;
//				pred->Unlock();
//				curr->Unlock();
//				return true;
//			}
//		}
//	}
//
//	bool Remove(int x)
//	{
//		while (true) {
//			NODE* pred = &head;
//			NODE* curr = pred->next;
//			while (curr->key < x) {
//				pred = curr;
//				curr = curr->next;
//			}
//
//			pred->Lock();
//			curr->Lock();
//
//			if (false == is_valid(pred, curr))
//			{
//				pred->Unlock();
//				curr->Unlock();
//				continue;
//			}
//
//			if (curr->key != x) {
//				pred->Unlock();
//				curr->Unlock();
//				return false;
//			}
//			else {
//				curr->is_removed = true;
//				atomic_thread_fence(std::memory_order_seq_cst);
//				pred->next = curr->next;
//				pred->Unlock();
//				curr->Unlock();
//				// delete curr;
//				return true;
//			}
//		}
//	}
//
//	bool Contains(int x)
//	{
//		NODE* curr = &head;
//		while (curr->key < x)
//			curr = curr->next;
//		return (false == curr->is_removed) && (curr->key == x);
//	}
//
//	void display20()
//	{
//		NODE* ptr = head.next;
//		for (int i = 0; i < 20; ++i) {
//			if (&tail == ptr) break;
//			cout << ptr->key << ", ";
//			ptr = ptr->next;
//		}
//		cout << endl;
//	}
//};
//
//// shared_ptr 노드
//class SPNODE {
//public:
//	int key;
//	shared_ptr<SPNODE>  next;
//	bool is_removed;
//	mutex n_lock;
//
//	SPNODE()
//	{
//		is_removed = false;
//	}
//
//	SPNODE(int x)
//	{
//		key = x;
//		is_removed = false;
//	}
//	~SPNODE()
//	{
//	}
//
//	void Lock()
//	{
//		n_lock.lock();
//	}
//
//	void Unlock()
//	{
//		n_lock.unlock();
//	}
//};
//
//// 게으른 동기화(shared_ptr)
//class SPLLIST {
//	shared_ptr<SPNODE> head, tail;
//public:
//	SPLLIST() // 생성자는
//	{
//		head = make_shared<SPNODE>(0x80000000);
//		tail = make_shared<SPNODE>(0x7FFFFFFF);
//		head->next = tail;
//	}
//	~SPLLIST()
//	{
//	}
//
//	void clear()
//	{
//		head->next = tail;
//	}
//
//	bool is_valid(const shared_ptr<SPNODE>& pred, const shared_ptr<SPNODE>& curr)
//	{
//		return (false == pred->is_removed) &&
//			(false == curr->is_removed) &&
//			atomic_load(&pred->next) == curr;
//	}
//
//	bool Add(int x)
//	{
//		while (true) {
//			shared_ptr<SPNODE> pred = head;
//			shared_ptr<SPNODE> curr = atomic_load(&pred->next);
//			while (curr->key < x) {
//				pred = curr;
//				curr = atomic_load(&curr->next);
//			}
//
//			pred->Lock();
//			curr->Lock();
//
//			if (false == is_valid(pred, curr))
//			{
//				pred->Unlock();
//				curr->Unlock();
//				continue;
//			}
//
//			if (curr->key == x) {
//				pred->Unlock();
//				curr->Unlock();
//				return false;
//			}
//			else {
//				shared_ptr<SPNODE> new_node = make_shared<SPNODE>(x);
//				new_node->next = curr;
//				atomic_store(&pred->next, new_node);
//				pred->Unlock();
//				curr->Unlock();
//				return true;
//			}
//		}
//	}
//
//	bool Remove(int x)
//	{
//		while (true) {
//			shared_ptr<SPNODE> pred = head;
//			shared_ptr<SPNODE> curr = atomic_load(&pred->next);
//			while (curr->key < x) {
//				pred = curr;
//				curr = atomic_load(&curr->next);
//			}
//
//			pred->Lock();
//			curr->Lock();
//
//			if (false == is_valid(pred, curr))
//			{
//				pred->Unlock();
//				curr->Unlock();
//				continue;
//			}
//
//			if (curr->key == x) {
//				curr->is_removed = true;
//				atomic_thread_fence(memory_order_seq_cst);
//				atomic_store(&pred->next, atomic_load(&curr->next));
//				pred->Unlock();
//				curr->Unlock();
//				return false;
//			}
//			else {
//				pred->Unlock();
//				curr->Unlock();
//				return true;
//			}
//		}
//	}
//
//	bool Contains(int x)
//	{
//		shared_ptr<SPNODE> curr = head;
//		while (curr->key < x)
//			curr = atomic_load(&curr->next);
//		return (false == curr->is_removed) && (curr->key == x);
//	}
//
//	void display20()
//	{
//		shared_ptr<SPNODE> ptr = head->next;
//		for (int i = 0; i < 20; ++i) {
//			if (tail == ptr) break;
//			cout << ptr->key << ", ";
//			ptr = ptr->next;
//		}
//		cout << endl;
//	}
//};
//
//class LFNODE {
//private:
//	atomic_int next;
//public:
//	int key;
//
//	LFNODE() { next = 0; }
//	LFNODE(int x) {
//		key = x;
//		next = 0;
//	}
//	~LFNODE() {}
//
//	void set_next(LFNODE* addr, bool is_removed)
//	{
//		int value = reinterpret_cast<int>(addr);
//		if (true == is_removed) value = value | 1;
//		next = value;
//	}
//	LFNODE* get_next()
//	{
//		return reinterpret_cast<LFNODE*>(next & 0xFFFFFFFE);
//	}
//	LFNODE* get_next(bool* is_removed)
//	{
//		int value = next;
//		*is_removed = (0 != (value & 1));
//		return reinterpret_cast<LFNODE*>(value & 0xFFFFFFFE);
//	}
//
//	bool CAS_NEXT(LFNODE* old_addr, LFNODE* new_addr, bool old_mark, bool new_mark)
//	{
//		int old_value = reinterpret_cast<int>(old_addr);
//		if (true == old_mark) old_value = old_value | 1;
//
//		int new_value = reinterpret_cast<int>(new_addr);
//		if (true == new_mark) new_value = new_value | 1;
//
//		return atomic_compare_exchange_strong(reinterpret_cast<atomic_int*>(&next), &old_value, new_value);
//	}
//
//	bool AttempMark()
//	{
//		int oldvalue = next;
//		if (0 != (oldvalue & 1)) return false;
//		int newvalue = oldvalue | 1;
//		return atomic_compare_exchange_strong(reinterpret_cast<atomic_int*>(&next), &oldvalue, newvalue);
//	}
//	bool removed() {
//	}
//};
//
//class LFLIST {
//	LFNODE head, tail;
//
//public:
//	LFLIST()
//	{
//		head.key = 0x8000'0000;
//		tail.key = 0x7FFF'FFFF;
//		head.set_next(&tail, false);
//	}
//	~LFLIST() { clear(); }
//	void clear()
//	{
//		LFNODE* ptr = head.get_next();
//		while (ptr != &tail) {
//			LFNODE* to_delete = ptr;
//			ptr = ptr->get_next();
//			delete to_delete;
//		}
//		head.set_next(&tail, false);
//	}
//
//	void FIND(int key, LFNODE** pred, LFNODE** curr)
//	{
//	retry:
//		LFNODE* pr = &head;
//		LFNODE* cu = pr->get_next();
//
//		while (true) {
//			// cu가 마킹되어있으면 제거하고 cu를 다시 세팅
//			bool is_removed;
//			LFNODE* su = cu->get_next(&is_removed);
//			while (true == is_removed) {
//				if (false == pr->CAS_NEXT(cu, su, false, false)) { // cas 실패했을때
//					goto retry;
//				}
//				cu = su;
//				su = cu->get_next(&is_removed);
//			}
//			if (cu->key >= key) {
//				*pred = pr;
//				*curr = cu;
//				return;
//			}
//			pr = cu;
//			cu = su;
//		}
//	}
//	bool Add(int x)
//	{
//		while (true) {
//			LFNODE* pred, * curr;
//			FIND(x, &pred, &curr);
//
//			if (curr->key == x) {
//				return false;
//			}
//			else {
//				LFNODE* new_node = new LFNODE(x);
//				new_node->set_next(curr, false);
//				if (true == pred->CAS_NEXT(curr, new_node, false, false)) {
//					return true;
//				}
//			}
//		}
//	}
//	bool Remove(int x)
//	{
//		while (true) {
//			LFNODE* prev, * curr;
//			FIND(x, &prev, &curr);
//
//			if (curr->key != x) {
//				return false;
//			}
//			else {
//				LFNODE* su = curr->get_next();
//				bool snip = curr->CAS_NEXT(su, su, false, true);
//				if (false == snip)
//				{
//					continue;
//				}
//				prev->CAS_NEXT(curr, su, false, false);
//				return true;
//			}
//		}
//
//	}
//	bool Contains(int x)
//	{
//		while (true) {
//			bool is_removed = false;
//			LFNODE* curr = &head;
//			while (curr->key < x) {
//				curr = curr->get_next(&is_removed);
//			}
//			return curr->key == x && !is_removed;
//		}
//	}
//	void display20() {
//		LFNODE* ptr = head.get_next();
//		for (int i = 0; i < 20; ++i) {
//			if (&tail == ptr) break;
//			cout << ptr->key << ", ";
//			ptr = ptr->get_next();
//		}
//		cout << endl;
//	}
//};
//
//class SET_LIST {
//	set<int> m_set;
//public:
//	SET_LIST()
//	{
//	}
//	~SET_LIST()
//	{
//	}
//
//	void clear()
//	{
//		m_set.clear();
//	}
//
//	bool Add(int x)
//	{
//		if (0 != m_set.count(x)) return false;
//		m_set.insert(x);
//		return true;
//	}
//
//	bool Remove(int x)
//	{
//		if (0 == m_set.count(x)) return false;
//		m_set.erase(x);
//		return true;
//	}
//
//	bool Contains(int x)
//	{
//		return 0 != m_set.count(x);
//	}
//	void display20()
//	{
//		int count = 20;
//		for (auto v : m_set) {
//			cout << v << ", ";
//			if (count-- <= 0)
//				break;
//		}
//		cout << endl;
//	}
//};
//
//// stl set 성긴 동기화
//class SET_CLIST {
//	set <int> m_set;
//	mutex m_l;
//public:
//	SET_CLIST()
//	{
//	}
//	~SET_CLIST()
//	{
//	}
//
//	void clear()
//	{
//		m_set.clear();
//	}
//
//	bool Add(int x)
//	{
//		lock_guard<mutex>L(m_l);
//		if (0 != m_set.count(x)) return false;
//		m_set.insert(x);
//		return true;
//	}
//
//	bool Remove(int x)
//	{
//		lock_guard<mutex>L(m_l);
//		if (0 == m_set.count(x)) return false;
//		m_set.erase(x);
//		return true;
//	}
//
//	bool Contains(int x)
//	{
//		lock_guard<mutex>L(m_l);
//		return 0 != m_set.count(x);
//	}
//
//	void display20()
//	{
//		int counter = 20;
//		for (auto v : m_set) {
//			cout << v << ", ";
//			if (counter-- <= 0) break;
//		}
//		cout << endl;
//	}
//};


constexpr int NUM_TEST = 1000'0000;


CQUEUE my_queue;

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