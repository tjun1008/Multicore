#include <iostream>
#include <thread>
#include <atomic>

using namespace std;

int error_count = 0;
volatile int* bound;
volatile bool done = false;

void worker0()
{
	for (int i = 0; i < 2500'0000; ++i)
		*bound = -(1 + *bound);
	done = true;
}

void worker1()
{
	while (false == done)
	{
		int v = *bound;
		if ((v != 0) && (v != -1))
			error_count++;
	}
}

int main()
{
	//bound = new int(0);

	int arr[32];
	long long temp = reinterpret_cast<long long>(arr + 16);
	temp = temp & 0xFFFFFFFFFFFFFFC0;
	temp = temp - 2;
	bound = reinterpret_cast<int*>(temp);
	*bound = 0;
	thread t1{ worker0 };
	thread t2{ worker1 };
	t1.join();
	t2.join();
	

	cout << "Memory Error : " << error_count << endl;
	//delete bound;
}