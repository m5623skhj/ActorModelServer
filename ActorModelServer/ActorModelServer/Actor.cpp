#include "PreCompile.h"
#include "Actor.h"

void Actor::ProcessMessage()
{
	if (isStop)
	{
		// do what?
	}

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
}

void Actor::Stop()
{
	isStop = true;
}