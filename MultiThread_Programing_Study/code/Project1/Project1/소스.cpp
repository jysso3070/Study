#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
using namespace std;
using namespace chrono;

volatile int sum;
mutex sum_lock;

volatile bool flag[2] = { false, false };
volatile int victim = 0;

void p_lock(int my_id)
{
    int other = 1 - my_id;
    flag[my_id] = true;
    victim = my_id;
    //_asm mfence;
    atomic_thread_fence(memory_order_seq_cst);
    while ((true == flag[other]) && (victim == my_id));
}

void p_unlock(int my_id)
{
    flag[my_id] = false;
}


void worker(int t_id)
{
    for (int i = 0; i < 2500'0000; ++i) {
        p_lock(t_id);
        sum += 2;
        p_unlock(t_id);
    }
}

int main()
{
    auto start_t = chrono::high_resolution_clock::now();
    thread t1{ worker, 0 };
    thread t2{ worker, 1 };

    t1.join();
    t2.join();

    //for (int i = 0; i < 5000'0000; ++i) sum += 2;



    auto end_t = chrono::high_resolution_clock::now();
    auto exec_t = end_t - start_t;

    cout << "Exec time = " << duration_cast<milliseconds>(exec_t).count() << endl;
    cout << "Sum = " << sum << endl;

}