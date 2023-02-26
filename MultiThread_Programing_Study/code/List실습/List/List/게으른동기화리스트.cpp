#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>

using namespace std;
using namespace chrono;

class NODE {
public:
	int key;
	NODE* volatile next;
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

constexpr int NUM_TEST = 4000000;
constexpr int KEY_RANGE = 1000;

LLIST my_set;

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
		}
	}
}

constexpr int MAX_THREAD = 8;

int main()
{
	for (int num = 1; num <= MAX_THREAD; num = num * 2) {
		vector <thread> threads;
		my_set.clear();
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < num; ++i)
			threads.emplace_back(benchmark, num);
		for (auto& th : threads) th.join();
		auto end_t = high_resolution_clock::now();
		auto du = end_t - start_t;

		cout << num << " Threads,  ";
		cout << "Exec time " <<
			duration_cast<milliseconds>(du).count() << "ms  ";
		my_set.display20();
	}
}