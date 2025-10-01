#include "PreCompile.h"
#include "Actor.h"

void Actor::OnActorCreated()
{
}

#if ENABLE_TEST_SUPPORT
void Actor::PreTimerForTest()
{
	PreTimer();
}

void Actor::OnTimerForTest()
{
	OnTimer();
}

void Actor::PostTimerForTest()
{
	PostTimer();
}
#endif

void Actor::ProcessMessage()
{
	if (isStop.load())
	{
		return;
	}

	{
		std::scoped_lock lock(queueMutex);
		std::swap(consumerQueue, storeQueue);
	}

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
	isStop.store(true);
}
