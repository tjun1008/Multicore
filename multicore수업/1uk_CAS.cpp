#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>

using namespace std;
using namespace chrono;

volatile int sum;
volatile int LOCK = 0;

mutex myl;

bool CAS(volatile int* addr, int expected, int new_val)
{
	return atomic_compare_exchange_strong(reinterpret_cast<volatile atomic_int*>(addr), &expected, new_val);
}


void CAS_LOCK()
{
	while (!CAS(&LOCK, 0, 1));
}

void CAS_UNLOCK()
{
	while (!CAS(&LOCK, 1, 0));
}

void worker(int num_threads)
{
	const int loop_count = 500'0000 / num_threads;

	for (auto i = 0; i < loop_count; ++i) {
		CAS_LOCK();
		sum += 2;
		CAS_UNLOCK();
	}
}

int main()
{
	for (int i = 1; i <= 8; i *= 2)
	{
		sum = 0;
		vector<thread> workers;
		auto start_t = high_resolution_clock::now();

		for (int j = 0; j < i; ++j)
			workers.emplace_back(worker, i);

		for (auto& th : workers)
			th.join();

		auto end_t = high_resolution_clock::now();
		auto du_t = end_t - start_t;

		cout << "number of threads = " << i;
		cout << " exec_time = " << duration_cast<milliseconds>(du_t).count();
		cout << " sum = " << sum << endl;
	}

}