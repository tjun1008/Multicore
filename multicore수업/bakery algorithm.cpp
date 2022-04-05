#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>

using namespace std;
using namespace chrono;

volatile int sum;

mutex myl;

volatile bool* flag;
volatile int* label;

void b_init(int n)
{
	flag = new bool[n];
	label = new int[n];

	for (int i = 0; i < n; ++i)
	{
		flag[i] = false;
		label[i] = 0;
	}
}

int Max(int n) // max(Number[1], ..., Number[NUM_THREADS]);
{
	int max = label[0];
	for (int i = 1; i < n; ++i)
	{
		if (label[i] > max)
			max = label[i];
	}
		

	return max;
}

void b_lock(int myid, int n)
{
	flag[myid] = true;

	label[myid] = Max(n) + 1;

	flag[myid] = false;

	for (int i = 0; i < n; ++i)
	{
		while (flag[i] == true); //티켓 받았는지 검사
		while ((label[i] !=0) && (label[myid] > label[i]));
	}
}

void b_unlock(int my_id)
{

	label[my_id] = 0;
}

void worker(int num_threads, int my_id)
{

	const int loop_count = 500'0000 / num_threads;

	for (auto i = 0; i < loop_count; ++i) {

		//b_lock(my_id, num_threads);
		myl.lock();
		sum += 2;
		myl.unlock();
		//b_unlock(my_id);

	}
}

int main()
{
	for (int n = 1; n <= 8; n *= 2) // 1 2 4 8개일때 실행시간 비교
	{
		sum = 0;

		vector<thread> workers;

		//빵집알고리즘을 위한 flag label 배열 할당
		//b_init(n);

		auto start_t = high_resolution_clock::now();

		for (int j = 0; j < n; ++j)
			workers.emplace_back(worker, n, j);

		for (auto& th : workers)
			th.join();

		auto end_t = high_resolution_clock::now();
		auto du_t = end_t - start_t;

		cout << "number of threads = " << n;
		cout << " exec_time = " << duration_cast<milliseconds>(du_t).count();
		cout << " sum = " << sum << endl;
	}

}