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



class BACKOFF 
{ 
int limit;
const int minDelay = 1000;
const int maxDelay = 10000;

public:
	BACKOFF() { limit = minDelay; }
	void do_backoff() 
	{ 
		int delay = rand() % limit;

		if (0 == delay) 
			return; 
		
		limit = limit + limit;
		
		if (limit > maxDelay)
			limit = maxDelay;
		
		_asm mov ecx, delay;
	myloop: 
		_asm loop myloop; 
	} 
};

class LFBOSTACK
{
	NODE* volatile top;

	bool CAS_TOP(NODE* old_node, NODE* new_node)
	{
		return atomic_compare_exchange_strong(reinterpret_cast<volatile atomic_int*>(&top),
			reinterpret_cast<int*>(&old_node),
			reinterpret_cast<int>(new_node));
	}

public:
	LFBOSTACK()
	{
		top = nullptr;
	}
	~LFBOSTACK()
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
		BACKOFF bo; 
		
		NODE* e = new NODE(x);
		
		while (true) {
			NODE* p = top;
			e->next = p;

			if (true == CAS_TOP( p, e))
			{
				return;
			}
			bo.do_backoff();
		}
	}

	int Pop()
	{
		BACKOFF bo;
		
		while (true) {
			NODE* p = top;
			if (nullptr == p)
			{
				return 0;
			}

			NODE* next = p->next;

			int value = p->value; //cas하고 읽는거 의미없다. 미리 읽어둬야한다.

			if (p != top) continue;

			if (true == CAS_TOP( p, next))
			{
				//delete p; 
				return value;
			}

			bo.do_backoff();
		}


	}

	int EMPTY_ERROR() {
		cout << "스택이 비어있다!\n";
		return 0;
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

LFBOSTACK mystack;


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

		//workers.clear();

		auto end_t = high_resolution_clock::now();
		auto du_t = end_t - start_t;

		mystack.Print20();
		cout << endl;
		cout << i << " threads";
		cout << "    exec_time    " << duration_cast<milliseconds>(du_t).count();
		cout << "ms\n";
	}
}