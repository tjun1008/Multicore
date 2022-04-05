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
	mutex nlock;
	NODE() {}
	NODE(int x) : value(x), next(nullptr) {}
	~NODE() {}
};


class FLIST
{
	NODE head, tail;

public:
	FLIST()
	{
		head.value = 0x80000000;
		tail.value = 0x7FFFFFFF;
		head.next = &tail;
	}
	~FLIST()
	{
		Init();
	}
	void Init()
	{
		while (head.next != &tail)
		{
			NODE *p = head.next;
			head.next = p->next;
			delete p;
		}
	}
	bool Add(int x)
	{
		//노드에 뮤텍스 추가
		NODE *pred =NULL, *curr = NULL;

		head.nlock.lock();
		pred = &head;
		curr = pred->next;
		curr->nlock.lock();
		while (curr->value < x) {
			pred->nlock.unlock();
			pred = curr;
			curr = curr->next;
			curr->nlock.lock();
		}
		if (curr->value == x)
		{
			curr->nlock.unlock();
			pred->nlock.unlock();
			return false;
		}
		else
		{
			NODE *new_node = new NODE(x);
			pred->next = new_node;
			new_node->next = curr;
			curr->nlock.unlock();
			pred->nlock.unlock();
			return true;
		}

	}

	bool Remove(int x)
	{
		NODE *pred = NULL, *curr = NULL;
		head.nlock.lock();
		pred = &head;
		curr = pred->next;
		curr->nlock.lock();
		while (curr->value < x) {
			pred->nlock.unlock();
			pred = curr;
			curr = curr->next;
			curr->nlock.lock();
		}
		if (curr->value != x)
		{
			curr->nlock.unlock();
			pred->nlock.unlock();
			return false;
		}
		else
		{

			pred->next = curr->next;
			curr->nlock.unlock();
			pred->nlock.unlock();
			delete curr;
			return true;
		}

	}

	bool Contains(int x) { 
		NODE *curr = NULL; 
		
		curr = head.next;
		curr->nlock.lock();
		
		while (curr->value < x)
		{ 
			
			curr->nlock.unlock();
			curr = curr->next; 
			curr->nlock.lock();
		} 
		
		if (curr->value != x)
		{ 
			
			curr->nlock.unlock();
			
			return false; 
		} 
		else 
		{ 
			
			curr->nlock.unlock();
			
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

FLIST clist;

void BenchMark(int num_threads)
{
	for (int i = 0; i < NUM_TEST / num_threads; ++i)
	{
		int x = rand() % RANGE;
		switch (rand() % 3)
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

/*
bool Contains(int x) {
		NODE* pred = NULL, * curr = NULL;
		head.nlock.lock();
		pred = &head;
		curr = pred->next;
		curr->nlock.lock();

		while (curr->value < x)
		{
			pred->nlock.unlock();
			pred = curr;
			curr = curr->next;
			curr->nlock.lock();
		}

		if (curr->value == x)
		{
			curr->nlock.unlock();
			pred->nlock.unlock();
			return true;
		}
		else
		{
			curr->nlock.unlock();
			pred->nlock.unlock();
			return false;
		}
	}
*/