#include <iostream>
#include <thread>
#include <memory>
#include <atomic>
using namespace std;
const auto SIZE = 5000'0000;
//volatile int x, y;
atomic<int> x, y;
int trace_x[SIZE], trace_y[SIZE];

void thread_0() {
    for (int i = 0; i < SIZE; ++i) {
        x = i;
        //atomic_thread_fence(memory_order_seq_cst);
        trace_y[i] = y;
    }
}

void thread_1() {
    for (int i = 0; i < SIZE; ++i) {
        y = i;
        //atomic_thread_fence(memory_order_seq_cst);
        trace_x[i] = x;
    }
}

int main() {
    thread t0{ thread_0 };
    thread t1{ thread_1 };
    t0.join(); t1.join();

    int err_count = 0;
    for (int i = 0; i < SIZE; ++i) {
        if (trace_x[i] == trace_x[i + 1])
            if (trace_y[trace_x[i]] == trace_y[trace_x[i] + 1])
                if (i == trace_y[trace_x[i]]) {
                    cout << "X: " << trace_x[i];
                    cout << ", Y: " << i << endl;
                    err_count++;
                }
    }
    cout << "total err: " << err_count << endl;
}