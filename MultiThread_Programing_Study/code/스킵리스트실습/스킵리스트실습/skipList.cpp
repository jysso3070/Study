#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <set>

using namespace std;
using namespace chrono;

constexpr int MAX_THREAD = 16;

thread_local int thread_id;

constexpr int NUM_TEST = 400'0000;
constexpr int KEY_RANGE = 1000;

constexpr int MAX_LEVEL = 10;

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

class SKNODE {
public:
	int key;
	SKNODE* volatile next[MAX_LEVEL+1];
	int toplevel;

	SKNODE()
	{
		for (auto& p : next) p = nullptr;
		toplevel = MAX_LEVEL;
	}
	~SKNODE()
	{
	}
	SKNODE(int x, int top) {
		key = x;
		for (auto& p : next) p = nullptr;
		toplevel = top;
	}

};

// 성긴동기화
class SKLIST {
	SKNODE head, tail;
	mutex m_lock;
public:
	SKLIST()
	{
		head.key = 0x80000000;
		tail.key = 0x7FFFFFFF;
		for (auto& n : head.next) n = &tail;
		head.toplevel = tail.toplevel = MAX_LEVEL;
	}
	~SKLIST()
	{
		//clear();
	}

	void clear()
	{
		SKNODE* ptr = head.next[0];
		while (ptr != &tail) {
			SKNODE* to_delete = ptr;
			ptr = ptr->next[0];
			delete to_delete;
		}
		for (auto& n : head.next) n = &tail;
	}

	void Find(int x, SKNODE* preds[], SKNODE* currs[]) {
		preds[MAX_LEVEL] = &head;
		for (int cl = MAX_LEVEL; cl >= 0; --cl) {
			if(cl != MAX_LEVEL)
				preds[cl] = preds[cl + 1];
			currs[cl] = preds[cl]->next[cl];
			while (x > currs[cl]->key) {
				preds[cl] = currs[cl];
				currs[cl] = currs[cl]->next[cl];
			}
		}
	}
	bool Add(int x)
	{
		SKNODE* preds[MAX_LEVEL + 1];
		SKNODE* currs[MAX_LEVEL + 1];
		m_lock.lock();
		Find(x, preds, currs);

		if (currs[0]->key == x) {
			m_lock.unlock();
			return false;
		}
		else {
			int toplevel = 0;
			while (0 == (rand() % 2)) {
				toplevel++;
				if (MAX_LEVEL == toplevel) break;
			}
			SKNODE* new_node = new SKNODE(x, toplevel);
			for (int i = 0; i <= toplevel; ++i) {
				preds[i]->next[i] = new_node;
				new_node->next[i] = currs[i];
			}
			m_lock.unlock();
			return true;
		}
	}

	bool Remove(int x)
	{
		SKNODE* preds[MAX_LEVEL + 1];
		SKNODE* currs[MAX_LEVEL + 1];
		m_lock.lock();
		Find(x, preds, currs);

		if (currs[0]->key != x) {
			m_lock.unlock();
			return false;
		}
		else {
			int toplevel = currs[0]->toplevel;
			for (int i = 0; i <= toplevel; ++i) {
				preds[i]->next[i] = currs[i]->next[i];
			}
			delete currs[0];
			m_lock.unlock();
			return true;
		}
	}

	bool Contains(int x)
	{
		SKNODE* preds[MAX_LEVEL + 1];
		SKNODE* currs[MAX_LEVEL + 1];
		m_lock.lock();
		Find(x, preds, currs);

		if (currs[0]->key != x) {
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
		SKNODE* ptr = head.next[0];
		for (int i = 0; i < 20; ++i) {
			if (&tail == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next[0];
		}
		cout << endl;
	}
};

class LSKNODE {
public:
	int key;
	LSKNODE* volatile next[MAX_LEVEL + 1];
	int toplevel;
	volatile bool is_removed;
	volatile bool is_fully_linked;
	recursive_mutex n_lock;

	LSKNODE() {
		for (auto& p : next) p = nullptr;
		toplevel = MAX_LEVEL;
		is_removed = false;
		is_fully_linked = false;
	}
	LSKNODE(int x, int top) {
		key = x;
		for (auto& p : next) p = nullptr;
		toplevel = top;
		is_removed = false;
		is_fully_linked = false;
	}
	~LSKNODE() {}
};

class LSKLIST {
	LSKNODE head, tail;
public:
	LSKLIST() {
		head.key = 0x80000000;
		tail.key = 0x7FFFFFFF;
		for (auto& n : head.next) n = &tail;
		head.toplevel = tail.toplevel = MAX_LEVEL;
		head.is_fully_linked = tail.is_fully_linked = true;
	}
	~LSKLIST() {
		clear();
	}

	void clear()
	{
		LSKNODE* ptr = head.next[0];
		while (ptr != &tail) {
			LSKNODE* to_delete = ptr;
			ptr = ptr->next[0];
			delete to_delete;
		}
		for (auto& n : head.next) n = &tail;
	}

	int Find(int x, LSKNODE* preds[], LSKNODE* currs[]) 
	{
		int l_found = -1;
		preds[MAX_LEVEL] = &head;
		for (int cl = MAX_LEVEL; cl >= 0; --cl) {
			if (cl != MAX_LEVEL)
				preds[cl] = preds[cl + 1];
			currs[cl] = preds[cl]->next[cl];
			while (x > currs[cl]->key) {
				preds[cl] = currs[cl];
				currs[cl] = currs[cl]->next[cl];
			}
			if ((x == currs[cl]->key) && (-1 == l_found))
				l_found = cl;
		}
		return l_found;
	}

	bool Add(int x)
	{
		LSKNODE* preds[MAX_LEVEL + 1];
		LSKNODE* currs[MAX_LEVEL + 1];

		int toplevel = 0;
		while (0 == rand() % 2) {
			toplevel++;
			if (MAX_LEVEL == toplevel) break;
		}

		while (true) {
			int l_found = Find(x, preds, currs);
			if (-1 != l_found) {
				if (true == currs[l_found]->is_removed)
					continue;
				while (false == currs[l_found]->is_fully_linked);
				return false;
			}
			bool valid = true;
			int lock_index = 0;
			for (int i = 0; i <= toplevel; ++i) {
				preds[i]->n_lock.lock();
				lock_index = i;
				valid = (false == preds[i]->is_removed &&
					false == currs[i]->is_removed &&
					preds[i]->next[i] == currs[i]);
				if (false == valid) break;
			}
			if (false == valid) {
				for (int i = 0; i <= lock_index; ++i)
					preds[i]->n_lock.unlock();
				continue;
			}

			LSKNODE* new_node = new LSKNODE(x, toplevel);

			for (int i = 0; i <= toplevel; ++i)
				new_node->next[i] = currs[i];
			for (int i = 0; i <= toplevel; ++i)
				preds[i]->next[i] = new_node;

			for (int i = 0; i <= lock_index; ++i)
				preds[i]->n_lock.unlock();
			new_node->is_fully_linked = true;
			return true;
		}
	}

	bool Remove(int x)
	{
		LSKNODE* preds[MAX_LEVEL + 1];
		LSKNODE* currs[MAX_LEVEL + 1];

		int l_found = Find(x, preds, currs);

		LSKNODE* victim = nullptr;
		if (-1 != l_found)
			victim = currs[l_found];

		if ((-1 != l_found) && (false == victim->is_removed)
			&& (true == victim->is_fully_linked)
			&& (l_found == victim->toplevel)) {
			victim->n_lock.lock();
			if (true == victim->is_removed) {
				victim->n_lock.unlock();
				return false;
			}
			victim->is_removed = true;
		}
		else
			return false;
		while (true) {
			bool valid = true;
			int lock_index = 0;
			for (int i = 0; i <= victim->toplevel; ++i) {
				preds[i]->n_lock.lock();
				lock_index = i;
				valid = (false == preds[i]->is_removed) && (preds[i]->next[i] == victim);
				if (false == valid) { break; }
			}
			if (false == valid) {
				for (int i = 0; i <= lock_index; ++i) {
					preds[i]->n_lock.unlock();
				}
				Find(x, preds, currs);
				continue;
			}

			for (int i = 0; i <= victim->toplevel; ++i)
				preds[i]->next[i] = victim->next[i];
			for (int i = 0; i <= lock_index; ++i)
				preds[i]->n_lock.unlock();
			victim->n_lock.unlock();
			return true;
		}
	}

	bool Contains(int x)
	{
		LSKNODE* preds[MAX_LEVEL + 1];
		LSKNODE* currs[MAX_LEVEL + 1];

		int l_found = Find(x, preds, currs);
		return (l_found != -1) && (false == currs[l_found]->is_removed)
			&& (true == currs[l_found]->is_fully_linked);
	}

	void display20()
	{
		LSKNODE* ptr = head.next[0];
		for (int i = 0; i < 20; ++i) {
			if (&tail == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next[0];
		}
		cout << endl;
	}
};


LSKLIST my_set;

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