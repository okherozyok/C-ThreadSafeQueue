/*
�ඩ�ģ�Ϊÿһ��������Ϣ��ʹ�ö˴���һ�����У���ֹʹ�ö�������Ϣ��������ʱӰ������ʹ�öˡ�
*/

#pragma once

#include <map>
#include <set>
#include <mutex>
#include <shared_mutex>
#include <string>
#include "ThreadSafeQueue.h"
//#include <iostream>

using localMessageType_uint = uint8_t;
using localQueueHandle_uint = uint16_t;

enum class LocalMQ_ErrorCode
{
	RIGHT = 0,
	NO_MESSAGE_TYPE = 100,
	SOME_QUEUE_FULL = 200,
};

struct ErrorDescription
{
	LocalMQ_ErrorCode errCode;
	std::string description;
	std::vector<localQueueHandle_uint> localQueueHandles;
	ErrorDescription(LocalMQ_ErrorCode code, const std::string& desc)
		: errCode(code), description(desc) {}
};

template<typename T>
class LocalMQ
{
public:
	LocalMQ();
	LocalMQ(LocalMQ const&) = delete;
	LocalMQ& operator=(const LocalMQ&) = delete;
	~LocalMQ();

	localQueueHandle_uint registerListen(localMessageType_uint messageType, int size = 10);
	std::unique_ptr<ErrorDescription> publish(localMessageType_uint messageType, std::shared_ptr<const T>);
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

	// TODO ����id���޼��
	localQueueHandle_uint toBeReturned = queueId;
	shared_ptr<ThreadSafeQueue<T>> safeQueue = make_shared<ThreadSafeQueue<T>>(size);
	queueMap[toBeReturned] = move(safeQueue);
	messageTypeMap[messageType].push_back(toBeReturned);

	queueId++;

	return toBeReturned;
}

template<typename T>
std::unique_ptr<ErrorDescription> LocalMQ<T>::publish(localMessageType_uint messageType, std::shared_ptr<const T> sharedMessage)
{
	using namespace std;
	shared_lock<shared_mutex> rlock(rw_mutex);

	auto errDes = make_unique<ErrorDescription>(LocalMQ_ErrorCode::RIGHT, "");
	if (messageTypeMap.find(messageType) == messageTypeMap.end())
	{
		errDes->errCode = LocalMQ_ErrorCode::NO_MESSAGE_TYPE;
		errDes->description = "Can't find messageType=" + to_string(messageType);
		return errDes;
	}

	vector<localQueueHandle_uint> queues = messageTypeMap[messageType];
	for (localQueueHandle_uint queueHandle : queues)
	{
		shared_ptr<ThreadSafeQueue<T>> safeQueue = queueMap[queueHandle];
		if (!safeQueue->push(sharedMessage))
		{
			errDes->errCode = LocalMQ_ErrorCode::SOME_QUEUE_FULL;
			errDes->localQueueHandles.emplace_back(queueHandle);
		}
	}

	return errDes;
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


