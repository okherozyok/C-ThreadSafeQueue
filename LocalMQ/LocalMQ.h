#pragma once

#include <map>
#include <set>
#include <mutex>
#include "ThreadSafeQueue.h"

class LocalMQ_MessageKey
{
public:
	LocalMQ_MessageKey(uint8_t messageId, uint8_t queueId) : messageId(messageId), queueId(queueId) {}

	// 重载小于运算符，用于map的排序
	bool operator<(const LocalMQ_MessageKey& other) const {
		// 首先比较first，如果相等则比较second
		if (messageId != other.messageId) {
			return messageId < other.messageId;
		}
		return queueId < other.queueId;
	}
private:
	uint8_t messageId;
	uint16_t queueId;
};

template<typename T>
class LocalMQ
{
public:
	LocalMQ();
	LocalMQ(LocalMQ const&) = delete;
	LocalMQ& operator=(const LocalMQ&) = delete;
	~LocalMQ();

	uint16_t subscribeMessage(uint8_t messageId, int size = 10);

private:
	std::mutex mutex_;
	// TODO map size
	std::map<LocalMQ_MessageKey, std::unique_ptr<ThreadSafeQueue<T>>> messageMap;
	uint8_t queueId = 0;
};

template<typename T>
LocalMQ<T>::LocalMQ()
{
}
template<typename T>
LocalMQ<T>::~LocalMQ()
{
}

template<typename T>
uint16_t LocalMQ<T>::subscribeMessage(uint8_t messageId, int size)
{
	using namespace std;
	lock_guard<mutex> lock(mutex_);
	uint16_t toBeReturned = queueId;

	LocalMQ_MessageKey msgKey(messageId, queueId);
	unique_ptr<ThreadSafeQueue<T>> safeQueue = make_unique<ThreadSafeQueue<T>>(size);
	messageMap[msgKey] = move(safeQueue);

	queueId++;

	return toBeReturned;
}