#include "PreCompile.h"
#include "Actor.h"

void Actor::ProcessMessage()
{
	{
		std::scoped_lock lock(queueMutex);
		std::swap(consumerQueue, storeQueue);
	}

	Message message;
	while (not consumerQueue.empty())
	{
		auto& messageFunction = consumerQueue.front();
		if (messageFunction == nullptr)
		{
			consumerQueue.pop();
			continue;
		}

		messageFunction();
		consumerQueue.pop();
	}

	if (isStop)
	{
		// do what?
	}
}

void Actor::Stop()
{
	isStop = true;
}