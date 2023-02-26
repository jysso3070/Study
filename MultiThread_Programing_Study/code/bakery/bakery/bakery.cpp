#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <vector>
#include <algorithm>
#include <atomic>
using namespace std;
using namespace chrono;
#define THREADNUM 1

mutex sumLock;


volatile bool flag[THREADNUM];
volatile int ticket[THREADNUM];
volatile int sum;

int getMax()
{
    int max = ticket[0];
    for (int i = 0; i < THREADNUM; ++i) {
        if (max < ticket[i]) {
            max = ticket[i];
        }
    }
    return max;
}

void lock(int t_id)
{
    flag[t_id] = true;
    ticket[t_id] = 1 + *max_element(ticket, ticket + THREADNUM); //getMax();
    atomic_thread_fence(memory_order_seq_cst);
    flag[t_id] = false;
    for (int i = 0; i < THREADNUM; ++i) {
        //_asm mfence;
        atomic_thread_fence(memory_order_seq_cst);
        while (flag[i]);
        atomic_thread_fence(memory_order_seq_cst);
        while (ticket[i] != 0 && ( (ticket[i], i) < (ticket[t_id], t_id) ) );

    }
}

void unlock(int t_id)
{
    ticket[t_id] = 0;
}

void thread_func(int t_id)
{
    for (int i = 0; i < 5000'0000 / THREADNUM; ++i) {
        lock(t_id);
        sum += 2;
        unlock(t_id);
        //sumLock.lock();
        //sum += 2;
        //sumLock.unlock();
    }
}

int main()
{
    for (int i = 0; i < THREADNUM; ++i) {
        flag[i] = false;
        ticket[i] = 0;
    }
    vector<thread> threads;
    auto start_t = chrono::high_resolution_clock::now();
    for (int i = 0; i < THREADNUM; ++i) {
        threads.emplace_back(thread_func, i);
    }
    for (auto& t : threads) {
        t.join();
    }
    auto end_t = chrono::high_resolution_clock::now();
    auto exec_time = end_t - start_t;
    cout << "Exec Time = " << duration_cast<milliseconds>(exec_time).count() << "ms " << endl;
    cout << "Sum = " << sum << endl;
}