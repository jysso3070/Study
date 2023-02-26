#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>
#include <atomic>
using namespace std;
using namespace chrono;

const int MAX_THREAD = 16;

volatile int sum = 0;
mutex sumLock;

volatile int X = 0;

bool CAS(volatile int* addr, int expected, int new_value) {
	return atomic_compare_exchange_strong(reinterpret_cast<volatile atomic_int*>(addr), 
		&expected, new_value);
}

void cas_lock() {
	/*if (true == CAS(&X, 0, 1)) return;
	while (true) {
		if (true == CAS(&X, 0, 1)) return;
	}*/
	while (false == CAS(&X, 0, 1));
}

void cas_unlock() {
	X = 0;
}


void thread_func(int t_id, int num_thread)
{
	volatile int local_sum = 0;
	for (int i = 0; i < 5000'0000 / num_thread; ++i)
	{
		//sumLock.lock();
		cas_lock();
		sum += 2;
		//sumLock.unlock();
		cas_unlock();
	}
}

int main()
{
	for (int count = 1; count <= 16; count *= 2) {
		vector <thread> threads;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < count; ++i)
			threads.emplace_back(thread_func, i, count);

		for (auto& t : threads)
			t.join();

		auto end_t = high_resolution_clock::now();
		auto exec_time = end_t - start_t;
		cout << "Number of Threads = " << count << ", ";
		cout << "Exec Time = " << duration_cast<milliseconds>(exec_time).count() << "ms, ";
		cout << "Sum = " << sum << endl;
		sum = 0;
	}

}