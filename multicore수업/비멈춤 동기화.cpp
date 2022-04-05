#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <atomic>
#include <set>

using namespace std;
using namespace chrono;

const int NUM_TEST = 400'0000;
const int RANGE = 100;


//32비트로 해야함

class LFNODE;

class MPTR {
	int value;

public:
	void set(LFNODE* node)
	{
		value = reinterpret_cast<int>(node);
		if (true == value)
			value = value | 0x01;
		else value = value & 0xFFFFFFFE;
	}

	LFNODE* getptr()
	{
		return reinterpret_cast<LFNODE*>(value & 0xFFFFFFFE);
	}

	LFNODE* getptr(bool* removed)
	{
		int temp = value;
		if (0 == (temp & 0x1))
			*removed = false;
		else *removed = true;
		return reinterpret_cast<LFNODE*>(temp & 0xFFFFFFFE);
	}

	bool CAS(LFNODE* old_node, LFNODE* new_node, bool old_removed, bool new_removed)
	{
		int old_value, new_value;
		old_value = reinterpret_cast<int>(old_node);
		if (true == old_removed) //old_value++;
			old_value = old_value | 0x01;
		else old_value = old_value & 0xFFFFFFFFE;

		new_value = reinterpret_cast<int>(new_node);
		if (true == old_removed) new_value = new_value | 0x01;
			//new_value++;
		else new_value = new_value & 0xFFFFFFFFE;

		return atomic_compare_exchange_strong(reinterpret_cast<atomic_int*>(&value), &old_value, new_value);
	}

	bool TryMarking(LFNODE* old_node, bool new_removed)
	{
		int old_value, new_value;
		old_value = reinterpret_cast<int>(old_node);
		old_value = old_value & 0xFFFFFFFFE;
		new_value = old_value;
		if (true == new_removed)
			new_value = new_value | 0x01;
		//new_value++;

		return atomic_compare_exchange_strong(reinterpret_cast<atomic_int*>(&value), &old_value, new_value);
	}


};

class LFNODE {
public:
	int value;
	MPTR next;
	//LFNODE* next;
	//LFNODE *next; 합쳐야함
	//bool marked;
	//mutex nlock;

	LFNODE()
	{
		next.set(nullptr);
	}

	LFNODE(int x)
	{
		next.set(nullptr);
		value = x;
	}

	~LFNODE() {}

};



class LFLIST
{
	LFNODE head, tail;
	LFNODE* freelist;
	LFNODE freetail;
	mutex fl;

public:
	LFLIST()
	{
		head.value = 0x80000000;
		tail.value = 0x7FFFFFFF;
		head.next.set(&tail);
		freetail.value = 0x7FFFFFFF;
		freelist = &freetail;
	}
	~LFLIST()
	{
		Init();
	}
	void Init()
	{
		
		while (head.next.getptr() != &tail)
		{
			LFNODE* ptr = head.next.getptr();
			head.next.set(ptr->next.getptr());
			delete ptr;
			
		}

	}

	void recycle_freelist()
	{
		LFNODE* p = freelist;
		while (p != &freetail)
		{
			LFNODE* n = p->next.getptr();
			delete p;
			p = n;
		}

		freelist = &freetail;
	}

	void find(int x, LFNODE* (&pred), LFNODE* (&curr))
	{

	retry:
		pred = &head;
		curr = pred->next.getptr();

		while (true)
		{
			//curr 쓰레기 제거

			bool removed;
			LFNODE* succ = curr->next.getptr(&removed);

			while (true == removed)
			{
				if (false == pred->next.CAS(curr, succ, false, false))
					goto retry; 

				curr = succ;
				succ = curr->next.getptr(&removed);
			}

			if (curr->value >= x)
				return;
			pred = curr;
			curr = curr->next.getptr();
		}
	}


	bool Add(int x)
	{
		LFNODE* pred, * curr;
		LFNODE* newnode = new LFNODE(x);

		while (true)
		{
			find(x, pred, curr);

			if (x == curr->value)
			{
				delete newnode;
				return false;
			}

			else
			{
				newnode->next.set(curr);

				if (pred->next.CAS(curr, newnode, false, false))
					return true;

			}

		}


	}


	bool Remove(int x)
	{
		LFNODE* pred, * curr;
		//pred = &head;
		//curr = pred->next.getptr();

		bool snip;

		while (true)
		{
			find(x, pred, curr);

			if (x != curr->value)
			{
				return false;
			}

			else
			{
				LFNODE* succ = curr->next.getptr();
				snip = curr->next.TryMarking(succ, true);

				
				if (!snip)
					continue;
				pred->next.CAS(curr, succ, false, false);
				return true;
			}
		}



	}

	bool Contains(int x) {  // pred 만들기
		LFNODE* pred, * curr;
		bool marked = false;
		curr = &head;

		while (curr->value < x)
		{
			curr = curr->next.getptr();
			LFNODE* succ = curr->next.getptr(&marked);
		}

		return curr->value == x && !marked;



	}


	void Print20()
	{
		LFNODE* p = head.next.getptr();

		for (int i = 0; i < 20; ++i)
		{
			if (p == &tail)
				break;
			cout << p->value << ", ";
			p = p->next.getptr();
		}
	}

};

LFLIST clist;

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
