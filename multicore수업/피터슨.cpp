#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <atomic>

using namespace std;
using namespace chrono;

volatile int sum;

mutex myl;

//volatile bool flag[2] = { false,false };
atomic<char> flag[2] = { 0,0 };
volatile int victim = 0;

void p_lock(int my_id)
{
	int other = 1 - my_id;
	//flag[my_id] = true;
	flag[my_id] = 1;
	victim = my_id;
	atomic_thread_fence(memory_order_seq_cst);
	//while ((flag[other] == true) && (my_id == victim));
	while ((flag[other].fetch_add(0) == 1) && (my_id == victim));
}

void p_unlock(int my_id)
{
	flag[my_id] = false;
}

void worker(int num_threads, int my_id)
{

	const int loop_count = 5000'0000 / num_threads;

	for (auto i = 0; i < loop_count; ++i) {

		p_lock(my_id);
		sum = sum + 2;
		p_unlock(my_id);
	}
}

int main()
{
	for (int i = 2; i <= 2; i *= 2)
	{
		sum = 0;

		vector<thread> workers;
		auto start_t = high_resolution_clock::now();

		for (int j = 0; j < i; ++j)
			workers.emplace_back(worker, i, j);

		for (auto& th : workers)
			th.join();

		auto end_t = high_resolution_clock::now();
		auto du_t = end_t - start_t;

		cout << "number of threads = " << i;
		cout << " exec_time = " << duration_cast<milliseconds>(du_t).count();
		cout << " sum = " << sum << endl;
	}

}