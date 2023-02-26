#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <set>

using namespace std;
using namespace chrono;

constexpr int MAX_THREAD = 8;

thread_local int thread_id;

class NODE {
public:
	int key;
	NODE*  volatile next;	
	volatile bool is_removed;
	mutex n_lock;

	NODE()
	{
		is_removed = false;
		next = nullptr;
	}
	
	NODE(int x)
	{
		key = x;
		is_removed = false;
		next = nullptr;
	}
	~NODE()
	{
	}

	void Lock()
	{
		n_lock.lock();
	}

	void Unlock()
	{
		n_lock.unlock();
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
class CLIST {
	NODE head, tail;
	null_mutex m_lock;
public:
	CLIST()
	{
		head.key = 0x80000000;
		tail.key = 0x7FFFFFFF;
		head.next = &tail;
	}
	~CLIST()
	{

	}

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
		NODE* pred = &head;
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

		NODE* pred = &head;
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

		NODE* pred = &head;
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
	void display20()
	{
		NODE* ptr = head.next;
		for (int i = 0; i < 20; ++i) {
			if (&tail == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << endl;
	}
};

// 세밀한동기화
class FLIST {
	NODE head, tail;
public:
	FLIST()
	{
		head.key = 0x80000000;
		tail.key = 0x7FFFFFFF;
		head.next = &tail;
	}
	~FLIST()
	{

	}

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
		head.Lock();
		NODE* pred = &head;
		NODE* curr = pred->next;
		curr->Lock();
		while (curr->key < x) {
			pred->Unlock();
			pred = curr;
			curr = curr->next;
			curr->Lock();
		}

		if (curr->key == x) {
			pred->Unlock();
			curr->Unlock();
			return false;
		}
		else {
			NODE* new_node = new NODE(x);
			new_node->next = curr;
			pred->next = new_node;
			pred->Unlock();
			curr->Unlock();
			return true;
		}
	}

	bool Remove(int x)
	{
		head.Lock();
		NODE* pred = &head;
		NODE* curr = pred->next;
		curr->Lock();
		while (curr->key < x) {
			pred->Unlock();
			pred = curr;
			curr = curr->next;
			curr->Lock();
		}

		if (curr->key != x) {
			pred->Unlock();
			curr->Unlock();
			return false;
		}
		else {
			pred->next = curr->next;
			delete curr;
			pred->Unlock();
			// curr->Unlock();
			return true;
		}
	}
	
	bool Contains(int x)
	{
		head.Lock();
		NODE* pred = &head;
		NODE* curr = pred->next;
		curr->Lock();
		while (curr->key < x) {
			pred->Unlock();
			pred = curr;
			curr = curr->next;
			curr->Lock();
		}

		if (curr->key != x) {
			pred->Unlock();
			curr->Unlock();
			return false;
		}
		else {
			pred->Unlock();
			curr->Unlock();
			return true;
		}
	}
	void display20()
	{
		NODE* ptr = head.next;
		for (int i = 0; i < 20; ++i) {
			if (&tail == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << endl;
	}
};

// 낙천적 동기화
class OLIST {
	NODE head, tail;
public:
	OLIST()
	{
		head.key = 0x80000000;
		tail.key = 0x7FFFFFFF;
		head.next = &tail;
	}
	~OLIST()
	{

	}

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

	bool is_valid(NODE* pred, NODE* curr)
	{
		NODE* p = &head;
		while (p->key <= pred->key) {
			if (p == pred) {
				return pred->next == curr;
			}
			p = p->next;
		}
		return false;
	}

	bool Add(int x)
	{
		while (true) {
			NODE* pred = &head;
			NODE* curr = pred->next;
			while (curr->key < x) {
				pred = curr;
				curr = curr->next;
			}

			pred->Lock();
			curr->Lock();

			if (false == is_valid(pred, curr))
			{
				pred->Unlock();
				curr->Unlock();
				continue;
			}

			if (curr->key == x) {
				pred->Unlock();
				curr->Unlock();
				return false;
			}
			else {
				NODE* new_node = new NODE(x);
				new_node->next = curr;
				pred->next = new_node;
				pred->Unlock();
				curr->Unlock();
				return true;
			}
		}
	}

	bool Remove(int x)
	{
		while (true) {
			NODE* pred = &head;
			NODE* curr = pred->next;
			while (curr->key < x) {
				pred = curr;
				curr = curr->next;
			}

			pred->Lock();
			curr->Lock();

			if (false == is_valid(pred, curr))
			{
				pred->Unlock();
				curr->Unlock();
				continue;
			}

			if (curr->key != x) {
				pred->Unlock();
				curr->Unlock();
				return false;
			}
			else {
				pred->next = curr->next;
				pred->Unlock();
				curr->Unlock();
				// delete curr;
				return true;
			}
		}
	}

	bool Contains(int x)
	{
		while (true) {
			NODE* pred = &head;
			NODE* curr = pred->next;
			while (curr->key < x) {
				pred = curr;
				curr = curr->next;
			}

			pred->Lock();
			curr->Lock();

			if (false == is_valid(pred, curr))
			{
				pred->Unlock();
				curr->Unlock();
				continue;
			}

			if (curr->key == x) {
				pred->Unlock();
				curr->Unlock();
				return true;
			}
			else {
				pred->Unlock();
				curr->Unlock();
				return false;
			}
		}
	}

	void display20()
	{
		NODE* ptr = head.next;
		for (int i = 0; i < 20; ++i) {
			if (&tail == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << endl;
	}
};

// 게으른 동기화
class LLIST {
	NODE head, tail;
public:
	LLIST()
	{
		head.key = 0x80000000;
		tail.key = 0x7FFFFFFF;
		head.next = &tail;
	}
	~LLIST()
	{

	}

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

	bool is_valid(NODE* pred, NODE* curr)
	{
		return (false == pred->is_removed) &&
			(false == curr->is_removed) &&
			pred->next == curr;
	}

	bool Add(int x)
	{
		while (true) {
			NODE* pred = &head;
			NODE* curr = pred->next;
			while (curr->key < x) {
				pred = curr;
				curr = curr->next;
			}

			pred->Lock();
			curr->Lock();

			if (false == is_valid(pred, curr))
			{
				pred->Unlock();
				curr->Unlock();
				continue;
			}

			if (curr->key == x) {
				pred->Unlock();
				curr->Unlock();
				return false;
			}
			else {
				NODE* new_node = new NODE(x);
				new_node->next = curr;
				pred->next = new_node;
				pred->Unlock();
				curr->Unlock();
				return true;
			}
		}
	}

	bool Remove(int x)
	{
		while (true) {
			NODE* pred = &head;
			NODE* curr = pred->next;
			while (curr->key < x) {
				pred = curr;
				curr = curr->next;
			}

			pred->Lock();
			curr->Lock();

			if (false == is_valid(pred, curr))
			{
				pred->Unlock();
				curr->Unlock();
				continue;
			}

			if (curr->key != x) {
				pred->Unlock();
				curr->Unlock();
				return false;
			}
			else {
				curr->is_removed = true;
				atomic_thread_fence(std::memory_order_seq_cst);
				pred->next = curr->next;
				pred->Unlock();
				curr->Unlock();
				// delete curr;
				return true;
			}
		}
	}

	bool Contains(int x)
	{
		NODE* curr = &head;
		while (curr->key < x)
			curr = curr->next;
		return (false == curr->is_removed) && (curr->key == x);
	}

	void display20()
	{
		NODE* ptr = head.next;
		for (int i = 0; i < 20; ++i) {
			if (&tail == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << endl;
	}
};

// shared_ptr 노드
class SPNODE {
public:
	int key;
	shared_ptr<SPNODE>  next;
	bool is_removed;
	mutex n_lock;

	SPNODE()
	{
		is_removed = false;
	}

	SPNODE(int x)
	{
		key = x;
		is_removed = false;
	}
	~SPNODE()
	{
	}

	void Lock()
	{
		n_lock.lock();
	}

	void Unlock()
	{
		n_lock.unlock();
	}
};

// 게으른 동기화(shared_ptr)
class SPLLIST {
	shared_ptr<SPNODE> head, tail;
public:
	SPLLIST() // 생성자는
	{
		head = make_shared<SPNODE>(0x80000000);
		tail = make_shared<SPNODE>(0x7FFFFFFF);
		head->next = tail;
	}
	~SPLLIST()
	{
	}

	void clear()
	{
		head->next = tail;
	}

	bool is_valid(const shared_ptr<SPNODE> &pred, const shared_ptr<SPNODE> &curr)
	{
		return (false == pred->is_removed) &&
			(false == curr->is_removed) &&
			atomic_load(&pred->next) == curr;
	}

	bool Add(int x)
	{
		while (true) {
			shared_ptr<SPNODE> pred = head;
			shared_ptr<SPNODE> curr = atomic_load(&pred->next);
			while (curr->key < x) {
				pred = curr;
				curr = atomic_load(&curr->next);
			}

			pred->Lock();
			curr->Lock();

			if (false == is_valid(pred, curr))
			{
				pred->Unlock();
				curr->Unlock();
				continue;
			}

			if (curr->key == x) {
				pred->Unlock();
				curr->Unlock();
				return false;
			}
			else {
				shared_ptr<SPNODE> new_node = make_shared<SPNODE>(x);
				new_node->next = curr;
				atomic_store(&pred->next, new_node);
				pred->Unlock();
				curr->Unlock();
				return true;
			}
		}
	}

	bool Remove(int x)
	{
		while (true) {
			shared_ptr<SPNODE> pred = head;
			shared_ptr<SPNODE> curr = atomic_load(&pred->next);
			while (curr->key < x) {
				pred = curr;
				curr = atomic_load(&curr->next);
			}

			pred->Lock();
			curr->Lock();

			if (false == is_valid(pred, curr))
			{
				pred->Unlock();
				curr->Unlock();
				continue;
			}

			if (curr->key == x) {
				curr->is_removed = true;
				atomic_thread_fence(memory_order_seq_cst);
				atomic_store(&pred->next, atomic_load(&curr->next));
				pred->Unlock();
				curr->Unlock();
				return false;
			}
			else {
				pred->Unlock();
				curr->Unlock();
				return true;
			}
		}
	}

	bool Contains(int x)
	{
		shared_ptr<SPNODE> curr = head;
		while (curr->key < x)
			curr = atomic_load(&curr->next);
		return (false == curr->is_removed) && (curr->key == x);
	}

	void display20()
	{
		shared_ptr<SPNODE> ptr = head->next;
		for (int i = 0; i < 20; ++i) {
			if (tail == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << endl;
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
	LFNODE* get_next(bool* is_removed)
	{
		int value = next;
		*is_removed = (0 != (value & 1));
		return reinterpret_cast<LFNODE*>(value &0xFFFFFFFE);
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
	bool removed() {
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
	~LFLIST() { clear(); }
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

	void FIND(int key, LFNODE** pred, LFNODE** curr)
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
			LFNODE* pred, * curr;
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

class SET_LIST {
	set<int> m_set;
public:
	SET_LIST()
	{
	}
	~SET_LIST()
	{
	}

	void clear()
	{
		m_set.clear();
	}

	bool Add(int x)
	{
		if (0 != m_set.count(x)) return false;
		m_set.insert(x);
		return true;
	}

	bool Remove(int x)
	{
		if (0 == m_set.count(x)) return false;
		m_set.erase(x);
		return true;
	}

	bool Contains(int x)
	{
		return 0 != m_set.count(x);
	}
	void display20()
	{
		int count = 20;
		for (auto v : m_set) {
			cout << v << ", ";
			if (count-- <= 0)
				break;
		}
		cout << endl;
	}
};

// stl set 성긴 동기화
class SET_CLIST {
	set <int> m_set;
	mutex m_l;
public:
	SET_CLIST()
	{
	}
	~SET_CLIST()
	{
	}

	void clear()
	{
		m_set.clear();
	}

	bool Add(int x)
	{
		lock_guard<mutex>L(m_l);
		if (0 != m_set.count(x)) return false;
		m_set.insert(x);
		return true;
	}

	bool Remove(int x)
	{
		lock_guard<mutex>L(m_l);
		if (0 == m_set.count(x)) return false;
		m_set.erase(x);
		return true;
	}

	bool Contains(int x)
	{
		lock_guard<mutex>L(m_l);
		return 0 != m_set.count(x);
	}

	void display20()
	{
		int counter = 20;
		for (auto v : m_set) {
			cout << v << ", ";
			if (counter-- <= 0) break;
		}
		cout << endl;
	}
};

constexpr int ADD = 1;
constexpr int REMOVE = 2;
constexpr int CONTAINS = 3;
constexpr int CLEAR = 4;
constexpr int DISPLAY_20 = 5;

struct Invocation {
	int method_type;
	int parameter1;
};

struct Response {
	bool resp1;
};

struct SeqObject_SET {
	set <int> m_set;
public:
	Response apply(Invocation inv)
	{
		switch (inv.method_type) {
		case ADD:
			if (0 != m_set.count(inv.parameter1)) return Response{ false };
			m_set.insert(inv.parameter1);
			return Response{ true };
		case REMOVE:
			if (0 == m_set.count(inv.parameter1)) return Response{ false };
			m_set.erase(inv.parameter1);
			return Response{ true };
		case CONTAINS:
			return Response{ 0 != m_set.count(inv.parameter1) };
		case CLEAR:
			m_set.clear();
			return Response();
		case DISPLAY_20:
			int counter = 20;
			for (auto v : m_set) {
				cout << v << ", ";
				if (counter-- <= 0) break;
			}
			cout << endl;
			return Response();
			break;
		}
	}
};

constexpr int NUM_TEST = 400;
constexpr int KEY_RANGE = 1000;

class SEQ_SET_LIST {
	SeqObject_SET m_set;
public:
	SEQ_SET_LIST()
	{
	}
	~SEQ_SET_LIST()
	{
	}

	bool Add(int x)
	{
		return m_set.apply(Invocation{ ADD, x }).resp1;
	}

	bool Remove(int x)
	{
		return m_set.apply(Invocation{ REMOVE, x }).resp1;
	}

	bool Contains(int x)
	{
		return m_set.apply(Invocation{ CONTAINS, x }).resp1;
	}

	void clear()
	{
		m_set.apply(Invocation{ CLEAR, 0 });
	}

	void display20()
	{
		m_set.apply(Invocation{ DISPLAY_20, 0 });
		return;
	}
};

class UNODE;

class CONSENSUS {
	atomic_int next;
public:
	CONSENSUS() {
		next = -1;
	}
	UNODE* decide(UNODE* value)
	{
		int v = reinterpret_cast<int> (value);
		int ov = -1;
		atomic_compare_exchange_strong(&next, &ov, v);
		v = next;
		return reinterpret_cast<UNODE*>(v);
	}
};

class UNODE {
public:
	Invocation inv;
	UNODE* next;
	int	seq;
	CONSENSUS decide_next;
public:
	UNODE(Invocation pinv)
	{
		inv = pinv;
		next = nullptr;
		seq = 0;
	}
	UNODE()
	{
		next = nullptr;
		seq = 0;
	}
};

class LFUniversal_SET {
	UNODE tail;
	UNODE* head[MAX_THREAD];
	UNODE* get_new()
	{
		UNODE* new_node = head[0];
		for (auto p : head) {
			if (new_node->seq < p->seq) new_node = p;
		}
		return new_node;
	}

public:
	LFUniversal_SET()
	{
		for (auto& p : head) p = &tail;
	}
	Response apply(Invocation inv)
	{
		UNODE* prefer = new UNODE(inv);
		while (0 == prefer->seq) {
			UNODE* before = get_new();
			UNODE* after = before->decide_next.decide(prefer);
			before->next = after;
			after->seq = before->seq + 1;
			head[thread_id] = after;
		}
		SeqObject_SET s_set;
		UNODE* p = tail.next;
		while (p != prefer && p->inv.method_type != DISPLAY_20) {
			s_set.apply(p->inv);
			p = p->next;
		}
		return s_set.apply(inv);
	}
};

class LF_SET_LIST {
	LFUniversal_SET m_set;
public:
	LF_SET_LIST()
	{
	}
	~LF_SET_LIST()
	{
	}

	bool Add(int x)
	{
		return m_set.apply(Invocation{ ADD, x }).resp1;
	}

	bool Remove(int x)
	{
		return m_set.apply(Invocation{ REMOVE, x }).resp1;
	}

	bool Contains(int x)
	{
		return m_set.apply(Invocation{ CONTAINS, x }).resp1;
	}

	void clear()
	{
		m_set.apply(Invocation{ CLEAR, 0 });
	}

	void display20()
	{
		m_set.apply(Invocation{ DISPLAY_20, 0 });
		return;
	}
};

LF_SET_LIST my_set;

void benchmark(int num_threads, int t_id)
{
	
	thread_id = t_id;
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
		}
	}
}

int main()
{
	for (int num = 1; num <= MAX_THREAD; num = num * 2) {
		vector <thread> threads;
		my_set.clear();
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < num; ++i)
			threads.emplace_back(benchmark, num, i);
		for (auto& th : threads) th.join();
		auto end_t = high_resolution_clock::now();
		auto du = end_t - start_t;

		cout << num << " Threads,  ";
		cout << "Exec time " <<
			duration_cast<milliseconds>(du).count() << "ms  ";
		my_set.display20();
	}
}