#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <set>

using namespace std;
using namespace chrono;

const int NUM_TEST = 1000'0000;


class NODE {
public:
	int value;
	NODE* next;
	NODE() :next(nullptr) {}
	NODE(int x) : value(x), next(nullptr) {}
	~NODE() {}
};

class nullmutex
{
public:
	void lock() {}
	void unlock() {}
};

class CSTACK
{
	NODE* top;
	mutex top_lock;
	

public:
	CSTACK()
	{
		top = nullptr;
	}
	~CSTACK()
	{
		Init();
		
	}
	void Init()
	{
		while (top != nullptr)
		{
			NODE* ptr = top;
			top = top->next;
			delete ptr;
		}
	}
	void Push(int x)
	{
		NODE* e = new NODE{ x }; 
		top_lock.lock();
		e->next = top;
		top = e;
		top_lock.unlock();


	}

	int Pop()
	{
		top_lock.lock();
		
		if (nullptr == top) {
			top_lock.unlock();
			//락을 위에 하면 락 가지고 return 할 수 있어서 언락 먼저
			return 0; 
		} 
		
		int temp = top->value; 
		NODE *ptr = top;
		top = top->next;
		top_lock.unlock();
		delete ptr;
		return temp;

		
	}

	void Print20()
	{
		NODE* p = top;
		for (int i = 0; i < 20; ++i)
		{
			if (p == nullptr)
				break;
			cout << p->value << ", ";
			p = p->next;
		}
	}
};

CSTACK mystack;


void BenchMark(int num_threads)
{
	for (int i = 0; i < NUM_TEST / num_threads; ++i) {
		if ((0 == rand() % 2) || i < 1000 / num_threads) {
			mystack.Push(i);
		}
		else {
			mystack.Pop();
		}
	}
}

int main()
{
	for (int i = 1; i <= 8; i = i * 2)
	{
		vector <thread> workers;
		mystack.Init();
		auto start_t = high_resolution_clock::now();
		for (int j = 0; j < i; ++j)
			workers.emplace_back(BenchMark, i);
		for (auto& th : workers)
			th.join();

		auto end_t = high_resolution_clock::now();
		auto du_t = end_t - start_t;

		mystack.Print20();
		cout << endl;
		cout << i << " threads";
		cout << "    exec_time    " << duration_cast<milliseconds>(du_t).count();
		cout << "ms\n";
	}
}