#include "Actor.h"

void Actor::ProcessMessage()
{
	while (not queue.Empty())
	{
		auto messageOpt = queue.Pop();
		if (not messageOpt.has_value())
		{
			continue;
		}

		(*messageOpt)();
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