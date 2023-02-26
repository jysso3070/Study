#include <tbb\parallel_for.h>
#include <iostream>

using namespace std;
using namespace tbb;

atomic_int sum = 0;

int main() {
	size_t n = 5000'0000;

	parallel_for(size_t(0), n, [&](int i) {
		sum += 2;
	});
	cout << "Sum = " << sum << endl;
}