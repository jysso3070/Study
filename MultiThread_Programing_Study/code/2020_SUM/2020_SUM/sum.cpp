#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>
using namespace std;
using namespace chrono;

const int MAX_THREAD = 16;

volatile int sum[MAX_THREAD * 64];
mutex sumLock;

void thread_func(int t_id, int num_thread)
{
	volatile int local_sum = 0;
	for (int i = 0; i < 5000'0000 / num_thread; ++i)
	{
		sum[t_id * 64] += 2;
	}
}

int main()
{
	for (int count = 1; count <= 16; count *= 2) {
		vector <thread> threads;
		for (auto& s : sum) { s = 0; }
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < count; ++i)
			threads.emplace_back(thread_func, i, count);

		for (auto& t : threads)
			t.join();
		int t_sum = 0;
		for (auto& s : sum) t_sum += s;
		auto end_t = high_resolution_clock::now();
		auto exec_time = end_t - start_t;
		cout << "Number of Threads = " << count << ", ";
		cout << "Exec Time = " << duration_cast<milliseconds>(exec_time).count() << "ms, ";
		cout << "Sum = " << t_sum << endl;
	}
	
}