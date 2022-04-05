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

class CQUEUE
{
	NODE* head, * tail;
	mutex enq_lock, deq_lock;
	//nullmutex llock;

public:
	CQUEUE()
	{
		head = tail = new NODE(0);
	}
	~CQUEUE()
	{
		Init();
		delete head;
	}
	void Init()
	{
		while (head != tail)
		{
			NODE* p = head;
			head = head->next;
			delete p;
		}
	}
	void Enq(int x)
	{
		NODE* e = new NODE(x);

		enq_lock.lock();
		tail->next = e;
		tail = e;
		enq_lock.unlock();
	}

	int Deq()
	{
		int result;
		deq_lock.lock();
		if (head->next == nullptr) {
			deq_lock.unlock();
			return -1;
		}
		else {
			NODE* to_delete = head;
			result = head->next->value;
			head = head->next;
			deq_lock.unlock();
			delete to_delete;
			return result;
		}
	}

	void Print20()
	{
		NODE* p = head->next;
		for (int i = 0; i < 20; ++i)
		{
			if (p == nullptr)
				break;
			cout << p->value << ", ";
			p = p->next;
		}
	}
};

CQUEUE myqueue;


void BenchMark(int num_threads)
{
	for (int i = 0; i < NUM_TEST / num_threads; ++i) {
		if ((0 == rand() % 2) || i < 32 / num_threads) {
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