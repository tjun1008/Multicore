#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>

using namespace std;
using namespace chrono;

const int MAX_THREAD = 16;

volatile int sum[MAX_THREAD * 16];

mutex myl;

void worker(int num_threads,const int thread_id)
{

	const int loop_count = 5000'0000 / num_threads;

	for (auto i = 0; i < loop_count; ++i) {

		sum[thread_id * 16] += 2;

	}
}

int main()
{
	for (int i = 1; i <= MAX_THREAD; i *= 2)
	{
		int total_sum = 0;

		for (auto& v : sum)
			v = 0;

		vector<thread> workers;
		auto start_t = high_resolution_clock::now();

		for (int j = 0; j < i; ++j)
			workers.emplace_back(worker, i,j);

		for (auto& th : workers)
			th.join();

		for (auto v : sum)
			total_sum += v;

		auto end_t = high_resolution_clock::now();
		auto du_t = end_t - start_t;

		cout << "number of threads = " << i;
		cout << " exec_time = " << duration_cast<milliseconds>(du_t).count();
		cout << " sum = " << total_sum << endl;
	}

}