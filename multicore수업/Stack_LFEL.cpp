#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <set>

using namespace std;
using namespace chrono;

const int NUM_TEST = 1000'0000;
const int MAX_THREADS = 8;


class NODE {
public:
	int value;
	NODE* next;
	NODE() :next(nullptr) {}
	NODE(int x) : value(x), next(nullptr) {}
	~NODE() {}
};


constexpr int EMPTY = 0;
constexpr int WAITING = 1;
constexpr int BUSY = 2;

class LockFreeExchanger {
	atomic_int slot;
	int get_state() {
		int t = slot;
		return (t >> 30) & 0x3;
	}
	int get_value() {
		int t = slot;
		return t & 0x3FFFFFFF;
	}
	bool CAS(int old_st, int new_st, int old_v, int new_v) {
		int ov = old_st << 30 + (old_v & 0x3FFFFFFF);
		int nv = new_st << 30 + (new_v & 0x3FFFFFFF);
		bool ret = atomic_compare_exchange_strong(&slot, &ov, nv);
		old_v = ov & 0x3FFFFFFF;
		return ret;
	}
public:
	LockFreeExchanger() {
		slot = 0;
	}
	~LockFreeExchanger() {}
	int exchange(int value, bool* time_out, bool* is_busy) {
		for (int i = 0; i < 100; ++i) {
			switch (get_state()) {
			case EMPTY:
			{
				if (true == CAS(EMPTY, WAITING, 0, value)) {
					int counter = 0;
					while (BUSY != get_state()) {
						counter++;
						if (counter > 1000) {
							if (true == CAS(WAITING, EMPTY, value, 0)) {
								*time_out = true;
								return 0;
							} break;;
						}
					}
					int ret = get_value();
					slot = 0;
					return ret;
				}
				else {
					continue;
				}
			}
			break;
			case WAITING:
			{
				int ret = get_value();
				if (true == CAS(WAITING, BUSY, ret, value))
					return -1;
				else continue;
			}
			break;
			case BUSY:
			{
				*is_busy = true;
				continue;
				break;
			}
			}
		}
		*is_busy = true;
		*time_out = true;
		return 0;
	}
};

constexpr int CAPACITY = 8; //쿼드코어니까 8개이상 필요없다 

class EliminationArray {
	int _num_threads;
	int range;
	LockFreeExchanger exchanger[MAX_THREADS];
public:
	EliminationArray() : range(1) {}
	EliminationArray(int num_threads) : _num_threads(num_threads) { range = 1; }
	~EliminationArray() {}
	int Visit(int value, bool* time_out) {
		int slot = rand() % range;
		bool busy;
		int ret = exchanger[slot].exchange(value, time_out, &busy);
		if ((true == *time_out) && (range > 1)) range--;
		if ((true == busy) && (range <= _num_threads / 2)) range++;
		// MAX RANGE is # of thread / 2
		return ret;
	}
};

class BackOff
{
	int limit;
	const int minDelay = 1000;
	const int maxDelay = 10000;

public:
	BackOff() { limit = minDelay; }
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

class LFELSTACK
{
	NODE* volatile top;
	EliminationArray el_array;

	bool CAS_TOP(NODE* old_node, NODE* new_node)
	{
		return atomic_compare_exchange_strong(reinterpret_cast<volatile atomic_int*>(&top),
			reinterpret_cast<int*>(&old_node),
			reinterpret_cast<int>(new_node));
	}

public:
	LFELSTACK()
	{
		top = nullptr;
	}
	~LFELSTACK()
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
		BackOff bo;
		NODE* e = new NODE{ x };

		while (true) {
			NODE* p = top;
			e->next = p;

			if (true == CAS_TOP(p, e)) { return; }

			bool time_out = false;
			int ret = el_array.Visit(x, &time_out);

			if (false == time_out) {
				if (0 == ret) return; //아니면 처음부터 다시 
			}
		}
	}

	int Pop()
	{
		BackOff bo;

		while (true) {
			NODE* p = top;

			if (nullptr == p) { return 0; }

			NODE* next = p->next;
			int value = p->value;

			if (p != top)
				continue;

			if (true == CAS_TOP(p, next))
			{
				//delete p; 
				return value;
			}

			bool time_out;
			int ret = el_array.Visit(0, &time_out);

			if (false == time_out)
			{
				if (0 != ret)
					return ret; //처음부터 다시 
			}
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

LFELSTACK mystack;


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