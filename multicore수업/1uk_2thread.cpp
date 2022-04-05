#include <iostream>
#include <thread>

using namespace std;

int sum;

void worker()
{
	cout << "My thread id : " << this_thread::get_id() << endl; // 네임스페이스


	for (auto i = 0; i < 2500'0000; ++i)
		sum += 2;
}

int main()
{
	int sum = 0;
	thread t1{ worker };
	thread t2{ worker };

	t1.join();
	t2.join();
	cout << "sum = " << sum << endl;
}