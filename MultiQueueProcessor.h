#pragma once
#include <map>
#include <list>
#include <thread>
#include <mutex>
#include <unistd.h>

template<typename Key, typename Value>
struct IConsumer
{
	virtual void Consume(Key id, const Value &value)
	{
		id;
		value;
	}
};

#define MaxCapacity 1000

template<typename Key, typename Value>
class MultiQueueProcessor
{
public:
	MultiQueueProcessor() :
		running{ true },
		th(std::bind(&MultiQueueProcessor::Process, this)) {}

	~MultiQueueProcessor()
	{
		StopProcessing();
		th.join();
	}

	void StopProcessing()
	{
		running = false;
	}

	void Subscribe(Key id, IConsumer<Key, Value> * consumer)
	{
		std::lock_guard<std::recursive_mutex> lock{ mtx };
		auto iter = consumers.find(id);
		if (iter == consumers.end())
		{
			consumers.insert(std::make_pair(id, consumer));
		}
	}

	void Unsubscribe(Key id)
	{
		std::lock_guard<std::recursive_mutex> lock{ mtx };
		auto iter = consumers.find(id);
		if (iter != consumers.end())
			consumers.erase(id);
		std::lock_guard<std::recursive_mutex> unlock{ mtx };
	}

	void Enqueue(Key id, Value value)
	{
		std::lock_guard<std::recursive_mutex> lock{ mtx };
		auto iter = queues.find(id);
		if (iter != queues.end())
		{
			if (iter->second.size() < MaxCapacity)
				iter->second.push_back(value);
		}
		else
		{
			queues.insert(std::make_pair(id, std::list<Value>()));
			iter = queues.find(id);
			if (iter != queues.end())
			{
				if (iter->second.size() < MaxCapacity)
					iter->second.push_back(value);
			}
		}
		std::pair <int,int> foo;
		foo = std::make_pair(id, value);
		struct IConsumer<int, int> *test = reinterpret_cast<IConsumer<int,int>*>(&foo);
		Subscribe(id, test);
		std::lock_guard<std::recursive_mutex> unlock{ mtx };
	}

	Value Dequeue(Key id)
	{
		std::lock_guard<std::recursive_mutex> lock{ mtx };
		auto iter = queues.find(id);
		if (iter != queues.end())
		{
			if (iter->second.size() > 0)
			{
				auto front = iter->second.front();
				iter->second.pop_front();
				Unsubscribe(id);
				return front;
			}
		}
		std::lock_guard<std::recursive_mutex> unlock{ mtx };
		return Value{};
	}

protected:
	void Process()
	{
		while (running)
		{
			sleep(10);
			std::lock_guard<std::recursive_mutex> lock{ mtx };
			for (auto iter = queues.begin(); iter != queues.end(); ++iter)
			{
				auto consumerIter = consumers.find(iter->first);
				if (consumerIter != consumers.end())
				{
					Value front = Dequeue(iter->first);
					if (front != Value{})
						consumerIter->second->Consume(iter->first, front);
				}
			}
			std::lock_guard<std::recursive_mutex> unlock{ mtx };
		}
	}

protected:
	std::map<Key, IConsumer<Key, Value> *> consumers;
	std::map<Key, std::list<Value>> queues;

	bool running;
	std::recursive_mutex mtx;
	std::thread th;
};


/*This main has to be in .cpp file, for simplicity i have added it here*/
int main()
{
	int key = 0;
	int value = 0;
	MultiQueueProcessor <int, int> *pMQP = nullptr;
	pMQP = new MultiQueueProcessor<int,int>();
	while(true)
	{
		sleep(10);
		pMQP->Enqueue(key, value);
		key++;
		value++;
		
		if (key >= MaxCapacity)
			break;
	}
	
	if (pMQP)
	{
		delete pMQP;
		pMQP = nullptr;
	}
}