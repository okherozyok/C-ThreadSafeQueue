#pragma once

#include <queue>
#include <mutex>
#include <memory>
#include <condition_variable>
//#include <iostream>

template<typename T>
class ThreadSafeQueue
{
public:
	ThreadSafeQueue(int maxSize);
	ThreadSafeQueue(ThreadSafeQueue const&) = delete;
	ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;
	~ThreadSafeQueue();

	bool push(std::shared_ptr<const T>);
	std::shared_ptr<const T> pop();

	bool empty() const;
	size_t size() const;
	void clear();
private:
	mutable std::mutex mutex_; // 互斥量必须是可变的
	int maxSize;
	std::queue<std::shared_ptr<const T>> queue_;
	std::condition_variable conditionVar;
	void unsafeClear();
};

template<typename T>
ThreadSafeQueue<T>::ThreadSafeQueue(int maxSize) :maxSize(maxSize)
{
}

template<typename T>
ThreadSafeQueue<T>::~ThreadSafeQueue()
{
	std::lock_guard<std::mutex> lock(mutex_);
	unsafeClear();
	conditionVar.notify_all();
}

template<typename T>
bool ThreadSafeQueue<T>::push(std::shared_ptr<const T> value)
{
	std::lock_guard<std::mutex> lock(mutex_);
	bool rtn = false;
	if (queue_.size() >= maxSize)
	{
		rtn = false;
	}

	//std::cout << "push前" << value.use_count() << std::endl;
	queue_.push(std::move(value));
	rtn = true;
	conditionVar.notify_one();

	return rtn;
}

template<typename T>
std::shared_ptr<const T> ThreadSafeQueue<T>::pop()
{
	std::unique_lock<std::mutex> lock(mutex_);
	conditionVar.wait(lock, [this] {return !queue_.empty(); });

	std::shared_ptr<const T> popValue = std::move(queue_.front());
	//std::cout << "front后 " << popValue.use_count() << std::endl;
	queue_.pop();
	return popValue;
}

template<typename T>
bool ThreadSafeQueue<T> ::empty() const
{
	std::lock_guard<std::mutex> lk(mutex_);
	return queue_.empty();
}

template<typename T>
size_t ThreadSafeQueue<T> ::size() const {
	std::lock_guard<std::mutex> lock(mutex_);
	return queue_.size();
}

template<typename T>
void ThreadSafeQueue<T>::clear()
{
	std::lock_guard<std::mutex> lock(mutex_);
	unsafeClear();
}

template<typename T>
inline void ThreadSafeQueue<T>::unsafeClear()
{
	while (!queue_.empty())
	{
		queue_.pop();
	}
}
