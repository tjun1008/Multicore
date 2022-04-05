#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>

using namespace std;
using namespace chrono;

//int sum;
//volatile int sum;
atomic <int> sum;
mutex myl;

void worker(int num_threads)
{
	const int loop_count = 5000'0000 / num_threads;

	for (auto i = 0; i < loop_count; ++i) {
		//myl.lock();
		sum += 2;
		//myl.unlock();
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
			workers.emplace_back(worker,i);

		for (auto& th : workers)
			th.join();

		auto end_t = high_resolution_clock::now();
		auto du_t = end_t - start_t;

		cout << "number of threads = " << i;
		cout << " exec_time = " << duration_cast<milliseconds>(du_t).count();
		cout << " sum = " << sum << endl;
	}

}