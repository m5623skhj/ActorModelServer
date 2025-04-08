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
			std::scoped_lock lock(queueMutex);
			storeQueue.push([boundFunction]() { boundFunction(); });
		}
		return true;
	}
	void ProcessMessage();

public:
	void Stop();

protected:
	std::mutex queueMutex;

	std::queue<Message> consumerQueue;
	std::queue<Message> storeQueue;

	std::atomic_bool isStop{};
};