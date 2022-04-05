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


class SPNODE {
public:
	int value;
	shared_ptr<SPNODE> next;
	mutex nlock;
	bool removed;

	SPNODE() : removed(false), next(nullptr) {}
	SPNODE(int x) : value(x), removed(false), next(nullptr) {}
	~SPNODE() {}
};


class SPZLIST
{
	shared_ptr<SPNODE> head, tail;

public:
	SPZLIST()
	{
		head = make_shared<SPNODE>(0x80000000);
		tail = make_shared<SPNODE>(0x7FFFFFFF);
		head->next = tail; // �Ϲ� �����Ͷ� �Ȱ���
	}
	~SPZLIST()
	{
		Init();
	}
	void Init()
	{
		head->next = tail;
		
	}
	bool Add(int x)
	{
		shared_ptr<SPNODE> pred , curr;


		pred = head;
		curr = pred->next;

		while (curr->value < x) {

			pred = curr;
			curr = curr->next;

		}
		pred->nlock.lock();
		curr->nlock.lock();

		if (validate(pred, curr))
		{
			if (curr->value == x)
			{
				curr->nlock.unlock();
				pred->nlock.unlock();
				return false;
			}

			else
			{
				shared_ptr<SPNODE>  new_node = make_shared<SPNODE>(x);
				new_node->next = curr;
				pred->next = new_node;
				curr->nlock.unlock();
				pred->nlock.unlock();
				return true;
			}


		}

		pred->nlock.unlock();
		curr->nlock.unlock();

	}


	bool Remove(int x)
	{
		shared_ptr<SPNODE> pred ,  curr ;
		pred = head;
		curr = pred->next;
		while (curr->value < x)
		{
			pred = curr;
			curr = curr->next;
		}
		pred->nlock.lock();
		curr->nlock.lock();

		if (validate(pred, curr))
		{
			if (curr->value == x)
			{
				curr->removed = true;
				//atomic_thread_fence(memory_order_seq_cst);
				pred->next = curr->next;
				pred->nlock.unlock();
				curr->nlock.unlock();
				return true;
			}
			else {
				pred->nlock.unlock();
				curr->nlock.unlock();
				return false;
			}
		}
		curr->nlock.unlock();
		pred->nlock.unlock();

	
	}

	bool Contains(int x) {  // pred �����
		shared_ptr<SPNODE> pred ,  curr;
		curr = head;
		while (curr->value < x)
		{
			curr = curr->next;
		}
		return curr->value == x && !curr->removed;

	}


	void Print20()
	{
		shared_ptr<SPNODE> p = head->next;
		for (int i = 0; i < 20; ++i)
		{
			if (p == tail)
				break;
			cout << p->value << ", ";
			p = p->next;
		}
	}

	bool validate(const shared_ptr<SPNODE> &pred, const shared_ptr<SPNODE> &curr) //�ݵ�� ��ų��!
	{
		return !pred->removed && !curr->removed && pred->next == curr;
	}


};

SPZLIST clist;

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
