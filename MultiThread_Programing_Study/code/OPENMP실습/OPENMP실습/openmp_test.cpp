#include <omp.h>
#include<iostream>
#include <chrono>
#include <thread>
#include <mutex>

using namespace std;
using namespace chrono;

constexpr int LOOP = 5000'0000;

int omp_sum() {
	int sum = 0;
#pragma omp parallel shared (sum)
	{
		int nthreads = omp_get_num_threads();
		for (int i = 0; i < LOOP / nthreads; ++i) {
#pragma omp critical
			sum += 2;
		}
	}
	return sum;
}

int mthreadsum() 
{
	int sum = 0;
	thread worker[8];
	mutex sl;

	for (auto& th : worker)
		th = thread{ [&sum, &sl]() {
		for (int i = 0; i < LOOP / 8; ++i) {
			sl.lock();
			sum += 2;
			sl.unlock();
		}
	} };
	for (auto& th : worker)
		th.join();
	return sum;
}

int main() {

	int sum = 0;

	auto start_t = high_resolution_clock::now();
	//sum = omp_sum();
	sum = mthreadsum();
	auto end_t = high_resolution_clock::now();
	auto exec_time = end_t - start_t;

	cout << "sum = " << sum;
	cout << "  Exec Time = " << duration_cast<milliseconds>(exec_time).count() << endl;
}