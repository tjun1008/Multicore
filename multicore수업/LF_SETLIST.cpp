#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <set>

using namespace std;
using namespace chrono;

constexpr int MAX_THREAD = 8;

thread_local int thread_id;




constexpr int ADD = 1;
constexpr int REMOVE = 2;
constexpr int CONTAINS = 3;
constexpr int CLEAR = 4;
constexpr int PRINT20 = 5;

struct Invocation {
	int method_type;
	int input_value;
};

typedef int Response;


struct SeqObject {
	set <int> m_set;
public:
	Response apply(Invocation inv)
	{
		switch (inv.method_type) {
		case ADD:
			if (0 != m_set.count(inv.input_value)) return Response{ false };
			m_set.insert(inv.input_value);
			return Response{ true };
		case REMOVE:
			if (0 == m_set.count(inv.input_value)) return Response{ false };
			m_set.erase(inv.input_value);
			return Response{ true };
		case CONTAINS:
			return Response{ 0 != m_set.count(inv.input_value) };
		case CLEAR:
			m_set.clear();
			return Response();
		case PRINT20:
			int counter = 20;
			for (auto v : m_set) {
				cout << v << ", ";
				if (counter-- <= 0) break;
			}
			cout << endl;
			return Response();
			break;
		}
	}
};

constexpr int NUM_TEST = 4000;
constexpr int RANGE = 100;


class UNODE;

class CONSENSUS {
	volatile atomic_int d_value;
public:
	CONSENSUS() {
		d_value = -1;
	}

	UNODE* decide(UNODE* value)
	{
		int v = reinterpret_cast<int> (value);
		int init = -1;
		atomic_compare_exchange_strong(&d_value, &init, v);
		v = d_value;
		return reinterpret_cast<UNODE*>(v);
	}
};

class UNODE {
public:
	Invocation invoc;
	UNODE* next;
	volatile int	seq;
	CONSENSUS decideNext;
public:
	UNODE()
	{
		next = nullptr;
		seq = 0;
	}
	~UNODE() {}
	UNODE(const Invocation& pinv)
	{
		invoc = pinv;
		next = nullptr;
		seq = 0;
	}

};

class LFUniversal {
	UNODE tail;
	UNODE* head[MAX_THREAD];
	UNODE* get_new()
	{
		UNODE* new_node = head[0];
		for (auto p : head) {
			if (new_node->seq < p->seq) new_node = p;
		}
		return new_node;
	}

public:
	LFUniversal()
	{
		tail.seq = 1;
		for (int i = 0; i < MAX_THREAD; ++i) head[i] = &tail;

	}
	Response apply(Invocation invoc)
	{
		UNODE* prefer = new UNODE(invoc);

		while (prefer->seq == 0) {
			UNODE* before = get_new();
			UNODE* after = before->decideNext.decide(prefer);
			before->next = after;
			after->seq = before->seq + 1;
			head[thread_id] = after;
		}
		SeqObject myObject;
		UNODE* current = tail.next;
		while (current != prefer && current->invoc.method_type != PRINT20) {
			myObject.apply(current->invoc);
			current = current->next;
		}
		return myObject.apply(invoc);
	}
};

class LF_SETLIST {
	LFUniversal m_set;
public:
	LF_SETLIST()
	{
	}
	~LF_SETLIST()
	{
	}

	bool Add(int x)
	{
		return m_set.apply(Invocation{ ADD, x });
	}

	bool Remove(int x)
	{
		return m_set.apply(Invocation{ REMOVE, x });
	}

	bool Contains(int x)
	{
		return m_set.apply(Invocation{ CONTAINS, x });
	}

	void clear()
	{
		m_set.apply(Invocation{ CLEAR, 0 });
	}

	void Print20()
	{
		m_set.apply(Invocation{ PRINT20, 0 });
		return;
	}
};

LF_SETLIST clist;


void BenchMark(int num_threads, int t_id)
{
	thread_id = t_id;

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
		clist.clear();
		auto start_t = high_resolution_clock::now();
		for (int j = 0; j < i; ++j)
			workers.emplace_back(BenchMark, i, j);
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