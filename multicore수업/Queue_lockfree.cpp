#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <set>

using namespace std;
using namespace chrono;

const int NUM_TEST = 1000'0000;
const int RANGE = 1000;


class NODE {
public:
	int value;
	NODE* next;
	NODE() { next == nullptr; }
	NODE(int x) : value(x), next(nullptr) {}
	~NODE() {}
};

class nullmutex
{
public:
	void lock() {}
	void unlock() {}
};

class LFQUEUE
{
	NODE* volatile head;
	NODE* volatile tail;
	

public:
	LFQUEUE()
	{
		head = tail = new NODE(0);
	}
	~LFQUEUE()
	{
		//Init();
	}
	void Init()
	{
		NODE* ptr;
		while (head->next != nullptr)
		{
			ptr = head->next;
			head->next = head->next->next;
			delete ptr;
		}

		tail = head;
	}

	bool CAS(NODE* volatile* addr, NODE* old_node, NODE* new_node)
	{ 
		return atomic_compare_exchange_strong(reinterpret_cast<volatile atomic_int*>(addr), reinterpret_cast<int*>(&old_node), reinterpret_cast<int>(new_node)); 
	}

	void Enq(int x)
	{
		NODE* e = new NODE(x); 
		
		while (true) 
		{ 
			NODE* last = tail;
			NODE* next = last->next;
			
			if (last != tail) 
				continue; 
			
			if (nullptr == next) {
				if (CAS(&last->next, nullptr, e)) {
					CAS(&tail, last, e);
					return;
				}
			}
			else {
				CAS(&tail, last, next);
			}
		}
	}

	int Deq()
	{
		while (true) {
			NODE* first = head;
			NODE* next = first->next;
			NODE* last = tail;
			NODE* lastnext = last->next;
			
			if (first != head) 
				continue; 
			
			if (last == first) 
			{ 
				if (lastnext == nullptr) 
				{ 
					cout << "EMPTY!!!\n";
					this_thread::sleep_for(1ms);
					return -1; 
				} 
				
				else 
				{ 
					CAS(&tail, last, lastnext); 
					continue; 
				} 
			} 
			
			if (nullptr == next) continue;
			
			int result = next->value;
			
			if (false == CAS(&head, first, next)) 
				continue;
			
			//first->next = nullptr;
			delete first; 
			
			return result;
		}

	}

	void Print20()
	{
		NODE* p = head->next;
		for (int i = 0; i < 20; ++i)
		{
			if (p == tail)
				break;
			cout << p->value << ", ";
			p = p->next;
		}

	}
};

LFQUEUE my_queue;

void BenchMark(int num_threads)
{
	for (int i = 0; i < NUM_TEST / num_threads; ++i)
	{
		if ((rand() % 2 == 0) || i < 10000 / num_threads)
		{
			my_queue.Enq(i);
		}

		else {
			my_queue.Deq();
		}
	}
}


int main()
{
	for (int i = 1; i <= 8; i = i * 2)
	{
		vector <thread> workers;
		my_queue.Init();
		auto start_t = high_resolution_clock::now();
		for (int j = 0; j < i; ++j)
			workers.emplace_back(BenchMark, i);
		for (auto& th : workers)
			th.join();

		auto end_t = high_resolution_clock::now();
		auto du_t = end_t - start_t;

		my_queue.Print20();
		cout << endl;
		cout << i << " threads";
		cout << "    exec_time    " << duration_cast<milliseconds>(du_t).count();
		cout << "ms\n\n";
	}
}