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

constexpr int MAX_LEVEL = 10;

class SKNODE {
public:
	int value;
	SKNODE* volatile next[MAX_LEVEL + 1];
	int toplevel;
	volatile bool is_removed;
	volatile bool fully_linked;
	recursive_mutex n_lock;

	SKNODE() : value(0), toplevel(0), fully_linked(false), is_removed(false)
	{
		for (auto& n : next) n = nullptr;
	}
	SKNODE(int v, int top) : value(v), toplevel(top), fully_linked(false), is_removed(false)
	{
		for (auto& n : next) n = nullptr;
	}
};

class ZSKLIST {
	SKNODE head, tail;
public:
	ZSKLIST() {
		head.value = 0x80000000;
		tail.value = 0x7FFFFFFF;
		//head.next[1] = &tail.next[1];    X
		//head.next[1] = &tail;            O
		for (auto& n : head.next) n = &tail;
	}
	~ZSKLIST() {
		Init();
	}

	void Init()
	{
		while (head.next[0] != &tail) {
			SKNODE* p = head.next[0];
			head.next[0] = p->next[0];
			delete p;
		}
		for (auto& n : head.next) n = &tail;
	}

	int Find(int x, SKNODE* preds[], SKNODE* currs[])
	{
		int found_level = -1;
		int curr_level = MAX_LEVEL;
		preds[curr_level] = &head;
		while (true) {
			currs[curr_level] = preds[curr_level]->next[curr_level];
			while (currs[curr_level]->value < x) {
				preds[curr_level] = currs[curr_level];
				currs[curr_level] = currs[curr_level]->next[curr_level];
			}
			if ((-1 == found_level) && (currs[curr_level]->value == x))
				found_level = curr_level;
			if (0 == curr_level)
				return found_level;
			curr_level--;
			preds[curr_level] = preds[curr_level + 1];
		}
	}

	bool Add(int x)
	{
		SKNODE* preds[MAX_LEVEL + 1];
		SKNODE* currs[MAX_LEVEL + 1];

		while (true) {
			int found_level = Find(x, preds, currs);
			if (-1 != found_level) {
				if (true == currs[found_level]->is_removed)
					continue;
				while (false == currs[found_level]->fully_linked);
				return false;
			}

			// 노드가 존재하지 않는다 추가하자.
			// Locking을 하자.

			int new_level = 0;
			while (0 == rand() % 2) {
				new_level++;
				if (MAX_LEVEL == new_level)
					break;
			}


			bool valid = true;
			int max_lock_level = -1;
			for (int i = 0; i <= new_level; ++i) {
				preds[i]->n_lock.lock();
				max_lock_level = i;
				if ((false == preds[i]->is_removed)
					&& (false == currs[i]->is_removed)
					&& (preds[i]->next[i] == currs[i])) {
				}
				else {
					valid = false;
					break;
				}
			}
			if (false == valid) {
				for (int i = 0; i <= max_lock_level; ++i)
					preds[i]->n_lock.unlock();
				continue;
			}

			SKNODE* new_node = new SKNODE(x, new_level);

			for (int i = 0; i <= new_level; ++i)
				new_node->next[i] = currs[i];
			for (int i = 0; i <= new_level; ++i)
				preds[i]->next[i] = new_node;

			new_node->fully_linked = true;
			for (int i = 0; i <= max_lock_level; ++i)
				preds[i]->n_lock.unlock();

			return true;
		}
	}

	bool Remove(int x)
	{
		SKNODE* preds[MAX_LEVEL + 1];
		SKNODE* currs[MAX_LEVEL + 1];

		int found_level = Find(x, preds, currs);

		SKNODE* victim = nullptr;
		if (-1 != found_level)
			victim = currs[found_level];

		if ((-1 != found_level) && (false == victim->is_removed)
			&& (true == victim->fully_linked)
			&& (found_level == victim->toplevel)) {
			victim->n_lock.lock();
			if (true == victim->is_removed) {
				victim->n_lock.unlock();
				return false;
			}
			victim->is_removed = true;
		}
		else
			return false;
		while (true) {
			bool valid = true;
			int max_lock_level = 0;
			for (int i = 0; i <= victim->toplevel; ++i) {
				preds[i]->n_lock.lock();
				max_lock_level = i;
				valid = (false == preds[i]->is_removed) && (preds[i]->next[i] == victim);
				if (false == valid) { break; }
			}
			if (false == valid) {
				for (int i = 0; i <= max_lock_level; ++i) {
					preds[i]->n_lock.unlock();
				}
				Find(x, preds, currs);
				continue;
			}

			for (int i = 0; i <= victim->toplevel; ++i)
				preds[i]->next[i] = victim->next[i];
			for (int i = 0; i <= max_lock_level; ++i)
				preds[i]->n_lock.unlock();
			victim->n_lock.unlock();
			return true;
		}
	}

	bool Contains(int x)
	{
		SKNODE* preds[MAX_LEVEL + 1];
		SKNODE* currs[MAX_LEVEL + 1];

		int found_level = Find(x, preds, currs);
		return (found_level != -1) && (false == currs[found_level]->is_removed)
			&& (true == currs[found_level]->fully_linked);
	}

	void Print20()
	{
		SKNODE* p = head.next[0];
		for (int i = 0; i < 20; ++i)
		{
			if (p == &tail)
				break;
			cout << p->value << ", ";
			p = p->next[0];
		}
	}
};

ZSKLIST sklist;

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
