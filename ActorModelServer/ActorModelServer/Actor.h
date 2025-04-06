#pragma once
#include <optional>
#include <queue>
#include <mutex>
#include <functional>
#include <utility>
#include <array>
#include <queue>

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
		{
			std::scoped_lock lock(queueMutex[storeQueueIndex]);
			queue[storeQueueIndex].push([boundFunction]() { boundFunction(); });
		}
		return true;
	}
	void ProcessMessage();

public:
	void Stop();

protected:
	unsigned char storeQueueIndex{ 0 };
	unsigned char usingQueueIndex{ 1 };
	std::array<std::queue<Message>, 2> queue;
	std::array<std::mutex, 2> queueMutex;
	std::atomic_bool isStop{};
};