#include <iostream>
#include <thread>
#include <atomic>

using namespace std;
const int SIZE = 5000'0000;

volatile int x, y;
//atomic <int> x, y; //실행속도가 정말 느려짐
int trace_x[SIZE], trace_y[SIZE];

void worker0()
{
	for (int i = 0; i < SIZE; ++i)
	{
		x = i;
		atomic_thread_fence(memory_order_seq_cst);
		trace_y[i] = y;
	}
}

void worker1()
{
	for (int i = 0; i < SIZE; ++i)
	{
		y = i;
		atomic_thread_fence(memory_order_seq_cst);
		trace_x[i] = x;
	}
}

int main()
{
	int count = 0;
	thread t1{ worker0 };
	thread t2{ worker1 };
	t1.join();
	t2.join();

	for (int i = 0; i < (SIZE - 1); ++i)
	{
		if (trace_x[i] != trace_x[i + 1]) continue;
		int x_value = trace_x[i];
		if (trace_y[x_value] != trace_y[x_value + 1]) continue;
		if (i != trace_y[x_value]) continue;
		//cout << "x = " << x_value << " , y =" << trace_y[x_value] << endl;
		count++;
	}
	cout << "Memory Consistency Violation : " << count << endl;
}