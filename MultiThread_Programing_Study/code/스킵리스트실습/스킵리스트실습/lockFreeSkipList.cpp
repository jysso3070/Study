#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <set>

using namespace std;
using namespace chrono;

constexpr int MAX_THREAD = 16;

thread_local int thread_id;

class LFSKNode;

constexpr int MAX_LEVEL = 9;
constexpr int NUM_TEST = 4000000;
constexpr int KEY_RANGE = 1000;

bool Marked(LFSKNode* curr)
{
	int add = reinterpret_cast<int> (curr);
	return ((add & 0x1) == 0x1);
}

LFSKNode* GetReference(LFSKNode* curr)
{
	int addr = reinterpret_cast<int> (curr);
	return reinterpret_cast<LFSKNode*>(addr & 0xFFFFFFFE);
}

LFSKNode* Get(LFSKNode* curr, bool* marked)
{
	int addr = reinterpret_cast<int> (curr);
	*marked = ((addr & 0x01) != 0);
	return reinterpret_cast<LFSKNode*>(addr & 0xFFFFFFFE);
}

LFSKNode* AtomicMarkableReference(LFSKNode* node, bool mark)
{
	int addr = reinterpret_cast<int>(node);
	if (mark)
		addr = addr | 0x1;
	else
		addr = addr & 0xFFFFFFFE;
	return reinterpret_cast<LFSKNode*>(addr);
}

LFSKNode* Set(LFSKNode* node, bool mark)
{
	int addr = reinterpret_cast<int>(node);
	if (mark)
		addr = addr | 0x1;
	else
		addr = addr & 0xFFFFFFFE;
	return reinterpret_cast<LFSKNode*>(addr);
}

class LFSKNode
{
public:
	int key;
	LFSKNode* next[MAX_LEVEL + 1];
	int topLevel;

	// ���ʳ�忡 ���� ������
	LFSKNode() {
		for (int i = 0; i <= MAX_LEVEL; i++) {
			next[i] = AtomicMarkableReference(NULL, false);
		}
		topLevel = MAX_LEVEL;
	}

	// �Ϲݳ�忡 ���� ������
	LFSKNode(int x, int height) {
		key = x;
		for (int i = 0; i <= MAX_LEVEL; i++) {
			next[i] = AtomicMarkableReference(NULL, false);
		}
		topLevel = height;
	}

	void InitNode() {
		key = 0;
		for (int i = 0; i <= MAX_LEVEL; i++) {
			next[i] = AtomicMarkableReference(NULL, false);
		}
		topLevel = MAX_LEVEL;
	}

	void InitNode(int x, int top) {
		key = x;
		for (int i = 0; i <= MAX_LEVEL; i++) {
			next[i] = AtomicMarkableReference(NULL, false);
		}
		topLevel = top;
	}

	bool CompareAndSet(int level, LFSKNode* old_node, LFSKNode* next_node, bool old_mark, bool next_mark) {
		int old_addr = reinterpret_cast<int>(old_node);
		if (old_mark) old_addr = old_addr | 0x1;
		else old_addr = old_addr & 0xFFFFFFFE;
		int next_addr = reinterpret_cast<int>(next_node);
		if (next_mark) next_addr = next_addr | 0x1;
		else next_addr = next_addr & 0xFFFFFFFE;
		return atomic_compare_exchange_strong(reinterpret_cast<atomic_int*>(&next[level]), &old_addr, next_addr);
	}
};

class LFSKLIST
{
public:

	LFSKNode* head;
	LFSKNode* tail;

	LFSKLIST() {
		head = new LFSKNode(0x80000000, MAX_LEVEL);
		tail = new LFSKNode(0x7FFFFFFF, MAX_LEVEL);
		for (int i = 0; i <= MAX_LEVEL; i++) {
			head->next[i] = AtomicMarkableReference(tail, false);
		}
	}

	~LFSKLIST() {
		clear();
		delete head;
		delete tail;
	}

	void clear()
	{
		LFSKNode* curr = head->next[0];
		while (curr != tail) {
			LFSKNode* temp = curr;
			curr = GetReference(curr->next[0]);
			delete temp;
		}
		for (int i = 0; i <= MAX_LEVEL; i++) {
			head->next[i] = AtomicMarkableReference(tail, false);
		}
	}

	bool Find(int x, LFSKNode* preds[], LFSKNode* succs[])
	{
		int bottomLevel = 0;
		bool marked = false;
		bool snip;
		LFSKNode* pred = NULL;
		LFSKNode* curr = NULL;
		LFSKNode* succ = NULL;
	retry:
		while (true) {
			pred = head;
			for (int level = MAX_LEVEL; level >= bottomLevel; level--) {
				curr = GetReference(pred->next[level]);
				while (true) {
					succ = curr->next[level];
					while (Marked(succ)) { //ǥ�õǾ��ٸ� ����
						snip = pred->CompareAndSet(level, curr, succ, false, false);
						if (!snip) goto retry;
						//	if (level == bottomLevel) freelist.free(curr);
						curr = GetReference(pred->next[level]);
						succ = curr->next[level];
					}

					// ǥ�� ���� ���� ���
					// Ű���� ���� ����� Ű������ �۴ٸ� pred����
					if (curr->key < x) {
						pred = curr;
						curr = GetReference(succ);
						// Ű���� �׷��� ���� ���
						// currŰ�� ���Ű���� ���ų� ū���̹Ƿ� pred�� Ű���� 
						// ��� ����� �ٷ� �� ��尡 �ȴ�.		
					}
					else {
						break;
					}
				}
				preds[level] = pred;
				succs[level] = curr;
			}
			return (curr->key == x);
		}
	}

	bool Add(int x)
	{
		int topLevel = 0;
		LFSKNode* newNode = nullptr;
		while ((rand() % 2) == 1)
		{
			topLevel++;
			if (topLevel == MAX_LEVEL) break;
		}

		int bottomLevel = 0;
		LFSKNode* preds[MAX_LEVEL + 1];
		LFSKNode* succs[MAX_LEVEL + 1];
		while (true) {
			bool found = Find(x, preds, succs);
			// ��� Ű�� ���� ǥ�õ��� ���� ��带 ã���� Ű�� �̹� ���տ� �����Ƿ� false ��ȯ
			if (found) {
				return false;
			}
			else {
				if (nullptr == newNode)
					newNode = new LFSKNode;
				newNode->InitNode(x, topLevel);

				for (int level = bottomLevel; level <= topLevel; level++) {
					LFSKNode* succ = succs[level];
					// ���� ������� next�� ǥ�õ��� ���� ����, find()�� ��ȯ�� ��带 ����
					newNode->next[level] = Set(succ, false);
				}

				//find���� ��ȯ�� pred�� succ�� ���� �������� ���� ����
				LFSKNode* pred = preds[bottomLevel];
				LFSKNode* succ = succs[bottomLevel];

				newNode->next[bottomLevel] = Set(succ, false);

				//pred->next�� ���� succ�� ����Ű�� �ִ��� �ʾҴ��� Ȯ���ϰ� newNode�� ��������
				if (!pred->CompareAndSet(bottomLevel, succ, newNode, false, false))
					// �����ϰ��� next���� ����Ǿ����Ƿ� �ٽ� ȣ���� ����
					continue;

				for (int level = bottomLevel + 1; level <= topLevel; level++) {
					while (true) {
						pred = GetReference(preds[level]);
						succ = GetReference(succs[level]);
						// ������ ���� ���� ������ ���ʴ�� ����
						// ������ �����Ұ�� �����ܰ�� �Ѿ��
						if (pred->CompareAndSet(level, succ, newNode, false, false))
							break;
						//Findȣ���� ���� ����� preds, succs�� ���� ��´�.
						Find(x, preds, succs);
					}
				}

				//��� ������ ����Ǿ����� true��ȯ
				return true;
			}
		}
	}

	bool Remove(int x)
	{
		int bottomLevel = 0;
		LFSKNode* preds[MAX_LEVEL + 1];
		LFSKNode* succs[MAX_LEVEL + 1];
		LFSKNode* succ;

		while (true) {
			bool found = Find(x, preds, succs);
			if (!found) {
				//�������� �����Ϸ��� ��尡 ���ų�, ¦�� �´� Ű�� ���� ��尡 ǥ�� �Ǿ� �ִٸ� false��ȯ
				return false;
			}
			else {
				LFSKNode* nodeToRemove = succs[bottomLevel];
				//�������� ������ ��� ����� next�� mark�� �а� AttemptMark�� �̿��Ͽ� ���ῡ ǥ��
				for (int level = nodeToRemove->topLevel; level >= bottomLevel + 1; level--) {
					succ = nodeToRemove->next[level];
					// ���� ������ ǥ�õǾ������� �޼���� ���������� �̵�
					// �׷��� ���� ��� �ٸ� �����尡 ������ �޴ٴ� ���̹Ƿ� ���� ���� ������ �ٽ� �а�
					// ���ῡ �ٽ� ǥ���Ϸ��� �õ��Ѵ�.
					while (!Marked(succ)) {
						nodeToRemove->CompareAndSet(level, succ, succ, false, true);
						succ = nodeToRemove->next[level];
					}
				}
				//�̺κп� �Դٴ� ���� �������� ������ ��� ���� ǥ���ߴٴ� �ǹ�

				bool marked = false;
				succ = nodeToRemove->next[bottomLevel];
				while (true) {
					//�������� next������ ǥ���ϰ� ���������� Remove()�Ϸ�
					bool iMarkedIt = nodeToRemove->CompareAndSet(bottomLevel, succ, succ, false, true);
					succ = succs[bottomLevel]->next[bottomLevel];

					if (iMarkedIt) {
						Find(x, preds, succs);
						return true;
					}
					else if (Marked(succ)) return false;
				}
			}
		}
	}

	bool Contains(int x)
	{
		int bottomLevel = 0;
		bool marked = false;
		LFSKNode* pred = head;
		LFSKNode* curr = NULL;
		LFSKNode* succ = NULL;

		for (int level = MAX_LEVEL; level >= bottomLevel; level--) {
			curr = GetReference(pred->next[level]);
			while (true) {
				succ = curr->next[level];
				while (Marked(succ)) {
					curr = GetReference(curr->next[level]);
					succ = curr->next[level];
				}
				if (curr->key < x) {
					pred = curr;
					curr = GetReference(succ);
				}
				else {
					break;
				}
			}
		}
		return (curr->key == x);
	}
	void display20()
	{
		LFSKNode* curr = head;
		printf("First 20 entries are : ");
		for (int i = 0; i < 20; ++i) {
			curr = curr->next[0];
			if (NULL == curr) break;
			printf("%d(%d), ", curr->key, curr->topLevel);
		}
		printf("\n");
	}
};


LFSKLIST my_set;

void benchmark(int num_threads, int t_id)
{
	thread_id = t_id;
	for (int i = 0; i < NUM_TEST / num_threads; ++i) {
		switch (rand() % 3) {
		case 0:
			my_set.Add(rand() % KEY_RANGE);
			break;
		case 1:
			my_set.Remove(rand() % KEY_RANGE);
			break;
		case 2:
			my_set.Contains(rand() % KEY_RANGE);
			break;
		}
	}
}

int main()
{
	for (int num = 1; num <= MAX_THREAD; num = num * 2) {
		vector <thread> threads;
		my_set.clear();
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < num; ++i)
			threads.emplace_back(benchmark, num, i);
		for (auto& th : threads) th.join();
		auto end_t = high_resolution_clock::now();
		auto du = end_t - start_t;

		cout << num << " Threads,  ";
		cout << "Exec time " <<
			duration_cast<milliseconds>(du).count() << "ms  ";
		my_set.display20();
	}
}