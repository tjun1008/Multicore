#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <set>

//다시 해보기

using namespace std;
using namespace chrono;

const int NUM_TEST = 400'0000;
const int RANGE = 1000;

constexpr int MAX_LEVEL = 10;

class SLNODE {
public:
	int value;
	SLNODE* next[MAX_LEVEL];
	int toplevel;

	SLNODE() 
	{ 
		value = 0;
		toplevel = MAX_LEVEL;
		for (auto& p : next) 
			p = nullptr; 
	}

	SLNODE(int _value, int _toplevel) 
	{ 
		value = _value;
		toplevel = _toplevel;
		
		for (auto& p : next) 
			p = nullptr; 
	}


	SLNODE(int _value) 
	{
		value = _value;
		toplevel = MAX_LEVEL;
		
		for (auto& p : next) 
			p = nullptr; 
	}

};


class SKLIST
{
	SLNODE head, tail; 
	mutex skl_lock;


public:
	SKLIST() {
		head.value = 0x80000000;
		tail.value = 0x7FFFFFFF;
		head.toplevel = tail.toplevel = MAX_LEVEL;
		//모든 노드를 감싸고 있어야한다. 

		for (auto& p : head.next)
			p = &tail;
	
	} 
	
	~SKLIST() 
	{ 
		Init(); 
	} 
	
	void Init() 
	{ 
		SLNODE *ptr; 
		
		while (head.next[0] != &tail) 
		{ 
			ptr = head.next[0];
			head.next[0] = head.next[0]->next[0];
			delete ptr; 
		} 
		
		for (auto &p : head.next) 
			p = &tail; 
	
	} 
	
	void Find(int x, SLNODE* preds[MAX_LEVEL], SLNODE* currs[MAX_LEVEL])
	{ 
		//curr_level
		
		int cl = MAX_LEVEL - 1; //맨 윗레벨
		
		while (true) 
		{ 
			if (MAX_LEVEL - 1 == cl)
				preds[cl] = &head; 
			else preds[cl] = preds[cl + 1]; 
			
			currs[cl] = preds[cl]->next[cl]; 
			
			while (currs[cl]->value < x) 
			{ 
				preds[cl] = currs[cl];
				currs[cl] = currs[cl]->next[cl];
			} 
			
			if (0 == cl)
				return;
			cl--; 
		} 
	} 
	
	bool Add(int x) 
	{ 
		SLNODE *preds[MAX_LEVEL], *currs[MAX_LEVEL];
		skl_lock.lock();
		
		Find(x, preds, currs);
		if (x == currs[0]->value)
		{ 
			skl_lock.unlock();
			return false; 
		} 
		
		else 
		{ 
			int new_level = 0;
			
			while (rand() % 2 == 1) 
			{ 
				new_level++;
				if (MAX_LEVEL == new_level) break;
			} 
			
			SLNODE *node = new SLNODE(x, new_level);
			
			for (int i = 0; i <= new_level; ++i)
			{ 
				node->next[i] = currs[i];
				preds[i]->next[i] = node;
			} 
			
			skl_lock.unlock();
			return true; 
		} 
	} 
	
	bool Remove(int x) 
	{ 
		SLNODE *preds[MAX_LEVEL], *currs[MAX_LEVEL];
		
		skl_lock.lock();
		Find(x, preds, currs); 
		
		if (x == currs[0]->value) 
		{ 
			//삭제 
			
			for (int i = 0; i <= currs[0]->toplevel; ++i) 
			{ 
				preds[i]->next[i] = currs[i]->next[i];
			} 
			
			delete currs[0];
			skl_lock.unlock();
			return true; 
		} 
		
		else 
		{ 
			skl_lock.unlock(); 
			return false; 
		} 
	
	} 
	
	bool Contains(int x) 
	{ 
		SLNODE *preds[MAX_LEVEL], *currs[MAX_LEVEL];
		skl_lock.lock();
		
		Find(x, preds, currs);
		
		if (x == currs[0]->value) 
		{ 
			skl_lock.unlock();
			return true; 
		} 
		
		else 
		{ 
			skl_lock.unlock();
			return false; 
		} 
	}

	void Print20()
	{
		SLNODE* p = head.next[0];
		for (int i = 0; i < 20; ++i)
		{
			if (p == &tail)
				break;
			cout << p->value << ", ";
			p = p->next[0];
		}
	}
};

SKLIST sklist;

void BenchMark(int num_threads)
{
	for (int i = 0; i < NUM_TEST / num_threads; ++i)
	{
		int x = rand() % RANGE;
		switch (rand() % 3)
		{
		case 0:

			sklist.Add(x);

			break;
		case 1:
			sklist.Remove(x);

			break;
		case 2:
			sklist.Contains(x);

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
		sklist.Init();
		auto start_t = high_resolution_clock::now();
		for (int j = 0; j < i; ++j)
			workers.emplace_back(BenchMark, i);
		for (auto& th : workers)
			th.join();

		auto end_t = high_resolution_clock::now();
		auto du_t = end_t - start_t;

		sklist.Print20();
		cout << endl;
		cout << i << " threads";
		cout << "    exec_time    " << duration_cast<milliseconds>(du_t).count();
		cout << "ms\n";
	}
}
