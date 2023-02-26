#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
using namespace std;
using namespace chrono;
#define THREAD_SIZE 16

class Bakery {
    volatile bool* volatile flag;
    volatile int* volatile label;
    int size;
public:
    Bakery(int n)
    {
        flag = new bool[n];
        label = new int[n];
        size = n;
        for (int i = 0; i < n; ++i)
        {
            flag[i] = false;
            label[i] = 0;
        }
    }

    ~Bakery()
    {
        delete(flag);
        delete(label);
        flag = NULL;
        label = NULL;
    }

    void Lock(int t_id)
    {
        flag[t_id] = true;
        label[t_id] = MaxLabel() + 1;
        flag[t_id] = false;
        for (int i = 0; i < size; ++i)
        {
            atomic_thread_fence(memory_order_seq_cst);
            while (flag[i]);
            while ((label[i] != 0) && (label[i], i) < (label[t_id], t_id));
        }
    }

    void Unlock(int t_id)
    {
        label[t_id] = 0;
        //flag[t_id] = false;
    }

    int MaxLabel()
    {
        int max = label[0];
        for (int i = 0; i < size; ++i)
        {
            if (max < label[i])
                max = label[i];
        }
        return max;
    }
};

volatile int sum = 0;
Bakery bakery{ THREAD_SIZE };

void thread_func(int t_id)
{
    for (int i = 0; i < 5000'0000 / THREAD_SIZE; ++i)
    {
        bakery.Lock(t_id);
        sum += 2;
        //cout << sum << endl;
        bakery.Unlock(t_id);
    }
}

int main()
{
    thread* t[THREAD_SIZE];
    for (int i = 0; i < THREAD_SIZE; ++i)
    {
        t[i] = new thread{ thread_func, i };
    }
    auto start_t = chrono::high_resolution_clock::now();
    for (int i = 0; i < THREAD_SIZE; ++i)
    {
        t[i]->join();
    }
    auto end_t = chrono::high_resolution_clock::now();
    auto exec_time = end_t - start_t;
    cout << "Exec Time = " << duration_cast<milliseconds>(exec_time).count() << "ms " << endl;
    cout << "Sum = " << sum << endl;
}