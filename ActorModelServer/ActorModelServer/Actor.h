#pragma once
#include <optional>
#include <queue>
#include <mutex>
#include <functional>
#include <utility>
#include "LockFreeQueue.h"

class Actor
{
public:
	using Message = std::function<void()>;

	Actor() = default;
	virtual ~Actor() = default;

public:
	template<typename Func, typename... Args>
	bool SendMessage(Func&& func, Args&&... args)
	{
		if (isStop)
		{
			return false;
		}

		auto boundFunction = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
		queue.Enqueue([boundFunction]() { boundFunction(); });
		return true;
	}
	void ProcessMessage();

public:
	void Stop();

private:
	CLockFreeQueue<Message> queue;
	std::atomic_bool isStop{};
};