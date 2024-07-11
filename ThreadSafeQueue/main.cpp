// ThreadSafeDeque.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <memory> // for std::shared_ptr
#include <thread>
#include <vector>
#include <cassert>
#include "ThreadSafeQueue.h"
#include "CommonQueueTest.h"

using namespace std;

class Message
{
public:
	Message(int data) {
		this->data = data;
	}
	~Message() {
		cout << "释放资源 " << data << endl;
	}
	int data;
};

const int num_threads = 10; // 定义要创建的线程数量
ThreadSafeQueue<Message> safeQ(10);

void batchTest()
{
	std::vector<std::thread> pushThreads;
	std::vector<std::thread> popFrontThreads;

	// 创建多个线程
	for (int i = 0; i < num_threads; i++) {
		pushThreads.emplace_back([i]()
			{
				shared_ptr<Message> sp = make_shared<Message>(i);
				bool pushed = false;

				this_thread::sleep_for(chrono::milliseconds(10));
				pushed = safeQ.push(sp);

				if (!pushed)
				{
					cout << "pushed fail" << endl;
				}
			}); // 使用emplace_back直接在vector中构造thread对象
	}
	for (int i = 0; i < 1; i++) {
		popFrontThreads.emplace_back([i]()
			{
				for (int j = 0; j < num_threads; j++)
				{
					if (i % 2 == 0)
					{
						this_thread::sleep_for(chrono::milliseconds(30));
					}
					else {
						this_thread::sleep_for(chrono::milliseconds(100));
					}
					std::shared_ptr<const Message> poppedSp = safeQ.pop();
					cout << poppedSp->data << endl;
				}
			}); // 使用emplace_back直接在vector中构造thread对象
	}

	// 等待所有线程完成
	for (auto& thread : pushThreads) {
		thread.join(); // 阻塞当前线程，直到thread线程执行完毕
	}
	for (auto& thread : popFrontThreads) {
		thread.join(); // 阻塞当前线程，直到thread线程执行完毕
	}
}

void sharePtrDeconstructionTest()
{
	ThreadSafeQueue<Message> safeQ(10);
	{
		shared_ptr<Message> csp = make_shared<Message>(10);
		cout << "初始构造csp " << csp.use_count() << endl;
		assert(1 == csp.use_count());
		safeQ.push(csp);
		//td.push(move(csp));
		cout << "析构前的csp " << csp.use_count() << endl;
		assert(2 == csp.use_count());
	}

	{
		shared_ptr<const Message> poped = safeQ.pop();
		cout << "最终poped " << poped.use_count() << endl;
		assert(1 == poped.use_count());
		cout << "读取消息值 " << poped.get()->data << endl;
	}
	cout << "退出所有栈" << endl;

	cout << "队列大小 " << safeQ.size();
	assert(0 == safeQ.size());
}

void constructDeconstructTest1()
{
	ThreadSafeQueue<Message>* safeQueue = new ThreadSafeQueue<Message>(10);
	{
		shared_ptr<Message> csp = make_shared<Message>(10);
		safeQueue->push(csp);
	}

	thread t1([safeQueue]()
		{
			safeQueue->close();
			this_thread::sleep_for(chrono::seconds(1));
		});

	t1.join();

	delete safeQueue;
	cout << "结束" << endl;
}
void constructDeconstructTest2()
{
	ThreadSafeQueue<Message>* safeQueue = new ThreadSafeQueue<Message>(10);
	{
		shared_ptr<Message> csp = make_shared<Message>(10);
		safeQueue->push(csp);
	}

	delete safeQueue;
	this_thread::sleep_for(chrono::seconds(1));
	cout << "结束" << endl;
}

void commonQueueHeaderTest()
{
	shared_ptr<int> sp = make_shared<int>(1000);
	SAFE_Q_INT.push(sp);
	shared_ptr<const int> got = SAFE_Q_INT.pop();
	cout << *got.get() << endl;
	assert(1000 == *got.get());
}

void queueFullTest()
{
	using namespace std;
	{
		ThreadSafeQueue<int> safeQ(0);
		shared_ptr<int> value = make_shared<int>(56);
		assert(false == safeQ.push(value));
	}
	{
		ThreadSafeQueue<int> safeQ(1);
		shared_ptr<int> value = make_shared<int>(56);
		assert(true == safeQ.push(value));
	}
	{
		ThreadSafeQueue<int> safeQ(1);
		shared_ptr<int> value1 = make_shared<int>(56);
		safeQ.push(value1);
		shared_ptr<int> value2 = make_shared<int>(56);
		assert(false == safeQ.push(value2));
	}
}

int main()
{
	//batchTest();
	//constructDeconstructTest1();
	//constructDeconstructTest2();
	//sharePtrDeconstructionTest();
	//commonQueueHeaderTest();
	queueFullTest();

	return 0;
}


// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
