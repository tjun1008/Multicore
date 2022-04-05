#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <set>

using namespace std;
using namespace chrono;

const int NUM_TEST = 400'0000;
const int RANGE = 1000;


class NODE {
public:
	int value;
	NODE* next;
	NODE() {  }
	NODE(int x) : value(x), next(nullptr) {}
	~NODE() {}
};

class nullmutex
{
public:
	void lock() {}
	void unlock() {}
};

class CLIST
{
	NODE head, tail;
	mutex llock;
	//nullmutex llock;
public:
	CLIST()
	{
		head.value = 0x80000000;
		tail.value = 0x7FFFFFFF;
		head.next = &tail;
	}
	~CLIST()
	{
		Init();
	}
	void Init()
	{
		while (head.next != &tail)
		{
			NODE* p = head.next;
			head.next = p->next;
			delete p;
		}
	}
	bool Add(int x)
	{
		NODE *pred, *curr;
		pred = &head;

		llock.lock();
		curr = pred->next;
		while(curr->value < x) {
			pred = curr;
			curr = curr->next;
		}
		if (curr->value == x)
		{
			llock.unlock();
			return false;
		}
		else
		{
			NODE* new_node = new NODE(x);
			pred->next = new_node;
			new_node->next = curr;
			llock.unlock();
			return true;
		}
		
	}

	bool Remove(int x)
	{
		NODE *pred, *curr;
		pred = &head;

		llock.lock();
		curr = pred->next;
		while (curr->value < x) {
			pred = curr;
			curr = curr->next;
		}
		if (curr->value != x)
		{
			llock.unlock();
			return false;
		}
		else
		{
			
			pred->next = curr->next;
			delete curr;
			llock.unlock();
			return true;
		}

	}

	bool Contains(int x)
	{
		NODE *curr;

		
		llock.lock();
		
		curr = head.next;
		while (curr->value < x) {
			
			curr = curr->next;
		}
		if (curr->value != x)
		{
			llock.unlock();
			return false;
		}
		else
		{

			llock.unlock();
			return true;
		}

	}
	void Print20()
	{
		NODE* p = head.next;
		for (int i = 0; i < 20; ++i)
		{
			if (p == &tail)
				break;
			cout << p->value << ", ";
			p = p->next;
		}
	}
};

CLIST clist;
set <int> set_list;
mutex stllock;

void BenchMark(int num_threads)
{
	for (int i = 0; i < NUM_TEST/ num_threads; ++i)
	{
		int x = rand() % RANGE;
		switch (rand()%3)
		{
		case 0:
			
			clist.Add(x);
			
			break;
		case 1:
			clist.Remove(x);
			
			break;
		case 2:
			clist.Contains(x);
			
			break;
		default:
			break;
		}
	}
}

void BenchMark2(int num_threads)
{
	for (int i = 0; i < NUM_TEST / num_threads; ++i)
	{
		int x = rand() % RANGE;
		switch (rand() % 3)
		{
		case 0:
			stllock.lock();
			set_list.insert(x);
			stllock.unlock();
			break;
		case 1:
			stllock.lock();
			set_list.erase(x);
			stllock.unlock();
			break;
		case 2:
			stllock.lock();
			set_list.count(x);
			stllock.unlock();
			break;
		default:
			break;
		}
	}
}

int main()
{
	for (int i = 1; i <= 8; i = i * 2)
	{
		vector <thread> workers;
		clist.Init();
		auto start_t = high_resolution_clock::now();
		for (int j = 0; j < i; ++j)
			workers.emplace_back(BenchMark, i);
		for (auto& th : workers)
			th.join();

		auto end_t = high_resolution_clock::now();
		auto du_t = end_t - start_t;

		clist.Print20();
		cout << endl;
		cout << i << " threads";
		cout << "    exec_time    " << duration_cast<milliseconds>(du_t).count();
		cout << "ms\n";
	}
}