#pragma once
#include <optional>
#include <queue>
#include <mutex>
#include <functional>
#include <utility>

template <typename T>
class MPSCMessageQueue
{
public:
	inline void Push(T&& input)
	{
		std::lock_guard lock(queueMutex);
		queue.push(std::move(input));
	}

	inline std::optional<T> Pop()
	{
		std::lock_guard lock(queueMutex);
		if (queue.empty() == true)
		{
			return std::nullopt;
		}

		T returnValue = std::move(queue.front());
		queue.pop();

		return returnValue;
	}

	inline bool Empty()
	{
		return queue.empty();
	}

private:
	std::queue<T> queue;
	std::mutex queueMutex;
};

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
		queue.Push([boundFunction]() { boundFunction(); });
		return true;
	}
	void ProcessMessage();

public:
	void Stop();

private:
	MPSCMessageQueue<Message> queue;
	std::atomic_bool isStop{};
};