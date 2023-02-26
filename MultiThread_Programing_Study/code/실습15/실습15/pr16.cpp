#include <iostream>
#include <thread>
#include <memory>
#include <atomic>
using namespace std;

constexpr int SIZE = 2500'0000;
volatile bool done = false;
volatile int *bound;
int err;

void thread_1() {
    for (int i = 0; i <= SIZE; ++i) {
        *bound = -(1 + *bound);
    }
    done = true;
}

void thread_2() {
    while (!done) {
        int v = *bound;
        if (v != 0 && v != -1) {
            cout << hex << v << ", ";
            err++;
        }
    }

}

int main() {
    int *big_array = new int[64];
    int addr = reinterpret_cast<int>(&big_array[31]);
    addr = addr / 64;
    addr = addr * 64;
    // addr = addr - (addr & 64);
    addr = addr - 2;
    bound = reinterpret_cast<int*>(addr);
    *bound = 0;



    //bound = new int{ 0 };
    thread t1{ thread_1 };
    thread t2{ thread_2 };
    t1.join(); t2.join();

    cout << "err: " << err << endl;
    delete bound;
}