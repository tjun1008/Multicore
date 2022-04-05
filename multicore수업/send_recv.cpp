#include <iostream>
#include <thread>
#include <mutex>

using namespace std;

volatile int g_data = 0;
volatile bool g_flag = false;
mutex ll;

void receive()
{
	//ll.lock();
	while (false == g_flag)
	{
		//ll.unlock();
		//ll.lock();
	}
	//ll.unlock();
		cout << "Value : " << g_data << endl;
}

void sender()
{
	
	g_data = 999;
	//ll.lock();
	
	g_flag = true; 
	//ll.unlock();
	cout << "Sender finished.\n";
}

int main()
{
	thread t1{ receive };
	thread t2{ sender };
	t1.join();
	t2.join();
}