#include "PreCompile.h"
#include "Actor.h"

void Actor::ProcessMessage()
{
	Message message;
	while (not queue.Empty())
	{
		if (queue.Dequeue(&message))
		{
			(message)();
		}
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