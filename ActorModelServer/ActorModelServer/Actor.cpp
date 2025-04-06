#include "PreCompile.h"
#include "Actor.h"

void Actor::ProcessMessage()
{
	Message message;
	while (not queue[usingQueueIndex].empty())
	{
		std::scoped_lock lock(queueMutex[usingQueueIndex]);

		auto& messageFunction = queue[usingQueueIndex].front();
		if (messageFunction == nullptr)
		{
			queue[usingQueueIndex].pop();
			continue;
		}

		messageFunction();
		queue[usingQueueIndex].pop();
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