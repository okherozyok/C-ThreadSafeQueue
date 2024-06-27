/*
多订阅，为每一个监听消息的使用端创建一个队列，防止使用端消费消息发生阻塞时影响其它使用端。
*/

#pragma once

#include <map>
#include <set>
#include <mutex>
#include <shared_mutex>
#include "ThreadSafeQueue.h"
//#include <iostream>

using localMessageType_uint = uint8_t;
using localQueueHandle_uint = uint16_t;

template<typename T>
class LocalMQ
{
public:
	LocalMQ();
	LocalMQ(LocalMQ const&) = delete;
	LocalMQ& operator=(const LocalMQ&) = delete;
	~LocalMQ();

	localQueueHandle_uint registerListen(localMessageType_uint messageType, int size = 10);
	void publish(localMessageType_uint messageType, std::shared_ptr<const T>);
	std::shared_ptr<const T> subscribe(localQueueHandle_uint queueHandle);

private:
	std::shared_mutex  rw_mutex;
	localQueueHandle_uint queueId = 0;
	std::map<localQueueHandle_uint, std::shared_ptr<ThreadSafeQueue<T>>> queueMap;
	std::map<localMessageType_uint, std::vector<localQueueHandle_uint>> messageTypeMap;
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
localQueueHandle_uint LocalMQ<T>::registerListen(localMessageType_uint messageType, int size)
{
	using namespace std;
	unique_lock<shared_mutex> wlock(rw_mutex);

	localQueueHandle_uint toBeReturned = queueId;
	shared_ptr<ThreadSafeQueue<T>> safeQueue = make_shared<ThreadSafeQueue<T>>(size);
	queueMap[toBeReturned] = move(safeQueue);
	messageTypeMap[messageType].push_back(toBeReturned);

	queueId++;

	return toBeReturned;
}

template<typename T>
void LocalMQ<T>::publish(localMessageType_uint messageType, std::shared_ptr<const T> sharedMessage)
{
	using namespace std;
	shared_lock<shared_mutex> rlock(rw_mutex);

	if (messageTypeMap.find(messageType) == messageTypeMap.end())
	{
		return;
	}

	vector<localQueueHandle_uint> queues = messageTypeMap[messageType];
	for (localQueueHandle_uint queueHandle : queues)
	{
		shared_ptr<ThreadSafeQueue<T>> safeQueue = queueMap[queueHandle];
		safeQueue->push(sharedMessage);
	}
}

template<typename T>
std::shared_ptr<const T> LocalMQ<T>::subscribe(localQueueHandle_uint queueHandle)
{
	using namespace std;
	shared_lock<shared_mutex> rlock(rw_mutex);

	if (queueMap.find(queueHandle) == queueMap.end())
	{
		return nullptr;
	}
	shared_ptr<ThreadSafeQueue<T>> safeQueue = queueMap[queueHandle];
	return  safeQueue->pop();
}


