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
// stamped pointer 

/*class SPTR 
{ 
public: 
	NODE* volatile ptr;
	volatile int stamp;
	
	SPTR() 
	{
		ptr = nullptr;
		stamp = 0;
	} 
	
	SPTR(NODE* p, int v) 
	{ 
		ptr = p;
		stamp = v; 
	} 
};
*/

struct SPTR {
	NODE* volatile ptr;
	int volatile stamp;
};


class SLFQUEUE
{
	SPTR head,  tail;


public:
	SLFQUEUE()
	{
		head .ptr= tail.ptr = new NODE(0);
	}
	~SLFQUEUE()
	{
		//Init();
		//delete head.ptr;
	}
	void Init()
	{
		NODE* ptr;

		while (head.ptr ->next != nullptr)
		{
			ptr = head.ptr->next;
			head.ptr->next = head.ptr->next->next;
			delete ptr;
		}

		tail = head;
	}

	bool CAS(NODE* volatile* addr, NODE* old_node, NODE* new_node) 
	{ 
		return atomic_compare_exchange_strong(reinterpret_cast<volatile atomic_int*>(addr), reinterpret_cast<int*>(&old_node), reinterpret_cast<int>(new_node)); 
	} 
	
	bool STAMPCAS(SPTR* addr, NODE* old_node, int old_stamp, NODE* new_node) 
	{ 
		SPTR old_ptr{ old_node, old_stamp };
		SPTR new_ptr{ new_node, old_stamp + 1 };
		return atomic_compare_exchange_strong(reinterpret_cast<atomic_llong*>(addr), reinterpret_cast<long long*>(&old_ptr), *(reinterpret_cast<long long*>(&new_ptr))); 
	}



	void Enq(int x)
	{
		NODE* newnode = new NODE(x);

		while (true) 
		{ 
			SPTR last = tail;
			NODE* next = last.ptr->next;
			
			if (last.ptr != tail.ptr) continue;
			
			if (nullptr == next) 
			{ 
				if (CAS(&(last.ptr->next), nullptr, newnode))
				{ 
					STAMPCAS(&tail, last.ptr, last.stamp, newnode);
					return;
				} 
			} 
			
			else STAMPCAS(&tail, last.ptr, last.stamp, next); 
		}

	}

	int Deq()
	{
		while (true) {
			SPTR first = head;
			NODE* next = first.ptr->next;
			SPTR last = tail;
			NODE* lastnext = last.ptr->next;

			if (first.ptr != head.ptr) 
				continue;
			
			if (last.ptr == first.ptr) 
			{ 
				if (lastnext == nullptr) 
				{ 
					cout << "EMPTY!!! ";
					this_thread::sleep_for(1ms);
					return -1; 
				} 
				
				else 
				{ 
					STAMPCAS(&tail, last.ptr, last.stamp, lastnext);
					continue; 
				} 
			} 
			
			if (nullptr == next) 
				continue;
			
			int result = next->value; 
			
			if (false == STAMPCAS(&head, first.ptr, first.stamp, next)) 
				continue; 
			//first.ptr->next = nullptr;
			delete first.ptr; 
			return result; 
		}	
	}

	int EMPTY_ERROR() 
	{ 
		cout << "ERROR\n";
		return -1; 
	}



	void Print20()
	{
		NODE* p = head.ptr->next;
		for (int i = 0; i < 20; ++i)
		{
			if (p == nullptr)
				break;
			cout << p->value << ", ";
			p = p->next;
		}
	}
};

SLFQUEUE myqueue;


void BenchMark(int num_threads)
{
	for (int i = 0; i < NUM_TEST / num_threads; ++i) {
		if ((0 == rand() % 2) || i < 10000 / num_threads) {
			myqueue.Enq(i);
		}
		else {
			myqueue.Deq();
		}
	}
}

int main()
{
	for (int i = 1; i <= 8; i = i * 2)
	{
		vector <thread> workers;
		myqueue.Init();
		auto start_t = high_resolution_clock::now();
		for (int j = 0; j < i; ++j)
			workers.emplace_back(BenchMark, i);
		for (auto& th : workers)
			th.join();

		auto end_t = high_resolution_clock::now();
		auto du_t = end_t - start_t;

		myqueue.Print20();
		cout << endl;
		cout << i << " threads";
		cout << "    exec_time    " << duration_cast<milliseconds>(du_t).count();
		cout << "ms\n";
	}
}