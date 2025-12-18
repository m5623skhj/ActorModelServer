#pragma once
#include <optional>
#include <queue>
#include <mutex>
#include <utility>
#include "PacketManager.h"
#include "NetServerSerializeBuffer.h"
#include "BuildConfig.h"

namespace Deserializer
{
	template<typename T>
	[[nodiscard]]
	std::optional<T> ReadPrimitive(const char*& data, size_t& remaining)
	{
		if (remaining < sizeof(T))
		{
			return std::nullopt;
		}

		T value;
		std::memcpy(&value, data, sizeof(T));
		data += sizeof(T);
		remaining -= sizeof(T);

		return value;
	}

	[[nodiscard]]
	inline std::optional<std::string> ReadString(const char*& data, size_t& remaining)
	{
		const auto lengthOpt = ReadPrimitive<uint16_t>(data, remaining);
		if (not lengthOpt.has_value())
		{
			return std::nullopt;
		}

		const uint16_t length = lengthOpt.value();
		if (remaining < length)
		{
			return std::nullopt;
		}

		std::string str((data), length);
		data += length;
		remaining -= length;

		return str;
	}

	template<typename T>
	[[nodiscard]]
	std::optional<T> DeserializeOne(const char*& data, size_t& remaining)
	{
		if constexpr (std::is_same_v<T, std::string>)
		{
			return ReadString(data, remaining);
		}
		else
		{
			return ReadPrimitive<T>(data, remaining);
		}
	}

	template<typename... Args>
	[[nodiscard]]
	std::optional<std::tuple<Args...>> DeserializeImpl(const char*& data, size_t& remaining)
	{
		if constexpr (sizeof...(Args) == 0)
		{
			return std::tuple<>();
		}
		else if constexpr (sizeof...(Args) == 1)
		{
			auto resultOpt = DeserializeOne<Args...>(data, remaining);
			if (not resultOpt.has_value())
			{
				return std::nullopt;
			}

			return std::make_tuple(resultOpt.value());
		}
		else
		{
			using FirstType = std::tuple_element_t<0, std::tuple<Args...>>;
			auto firstOpt = DeserializeOne<FirstType>(data, remaining);
			if (not firstOpt.has_value())
			{
				return std::nullopt;
			}

			auto restOpt = std::apply([&]<typename... RestTypes>([[maybe_unused]]auto unUse, RestTypes... restArgs)
				{
					return DeserializeImpl<RestTypes...>(data, remaining);
				}, std::tuple<Args...>{});

			if (not restOpt.has_value())
			{
				return std::nullopt;
			}

			return std::tuple_cat(std::make_tuple(firstOpt.value()), restOpt.value());
		}
	}

	template<typename... Args>
	[[nodiscard]]
	std::optional<std::tuple<Args...>> Deserialize(NetBuffer& buffer)
	{
		const char* ptr = buffer.GetReadBufferPtr();
		size_t remaining = buffer.GetUseSize();

		return DeserializeImpl<Args...>(ptr, remaining);
	}
}

class ActorIdGenerator
{
private:
	ActorIdGenerator() = default;
	~ActorIdGenerator() = default;

public:
	[[nodiscard]]
	static ActorIdType GenerateActorId()
	{
		return ++actorIdGenerator;
	}

private:
	static inline std::atomic<ActorIdType> actorIdGenerator = 0;
};

class Actor
{
public:
	Actor() = default;
	virtual ~Actor() = default;

	virtual void OnActorCreated();
	virtual void OnActorDestroyed();

public:
	template<typename Func, typename... Args>
	[[nodiscard]]
	bool SendMessage(Func&& func, Args&&... args)
	{
		if (isStop.load())
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

	template<typename Func, typename DerivedType, typename... Args>
	[[nodiscard]]
	bool SendMessage(Func&& func, std::shared_ptr<DerivedType> sendTarget, Args&&... args)
	{
		static_assert(std::is_base_of_v<Actor, DerivedType>, "SendMessage() : DerivedType must inherit from Actor");

		if (isStop.load())
		{
			return false;
		}

		auto boundFunction = std::bind(std::forward<Func>(func), std::forward<std::shared_ptr<DerivedType>>(sendTarget), std::forward<Args>(args)...);
		{
			std::scoped_lock lock(queueMutex);
			storeQueue.push([boundFunction]() { boundFunction(); });
		}
		return true;
	}

	[[nodiscard]]
	bool SendMessage(Message&& message)
	{
		if (isStop.load())
		{
			return false;
		}

		{
			std::scoped_lock lock(queueMutex);
			storeQueue.emplace(std::move(message));
		}
		return true;
	}

	[[nodiscard]]
	bool SendMessage(const Message& message)
	{
		if (isStop.load())
		{
			return false;
		}

		{
			std::scoped_lock lock(queueMutex);
			storeQueue.emplace(message);
		}
		return true;
	}

public:
	void Stop();

#if ENABLE_TEST_SUPPORT
public:
	void PreTimerForTest();
	void OnTimerForTest();
	void PostTimerForTest();
#endif

public:
	virtual void PreTimer() {}
	virtual void OnTimer() {}
	virtual void PostTimer() {}

protected:
	void ProcessMessage();

protected:
	std::mutex queueMutex;

	std::queue<Message> consumerQueue;
	std::queue<Message> storeQueue;

	std::atomic_bool isStop{};

public:
	[[nodiscard]]
	std::optional<Message> CreateMessageFromPacket(NetBuffer& buffer)
	{
		if (const int useSize = buffer.GetUseSize(); useSize < static_cast<int>(sizeof(PACKET_ID)))
		{
			return std::nullopt;
		}

		PACKET_ID packetId;
		buffer >> packetId;

		return FindFunctionObject(packetId, buffer);
	}

private:
	[[nodiscard]]
	std::optional<Message> FindFunctionObject(const PACKET_ID packetId, NetBuffer& buffer)
	{
		const auto itor = messageFactories.find(packetId);
		if (itor == messageFactories.end())
		{
			return std::nullopt;
		}

		return itor->second(this, &buffer);
	}

public:
	using MessageFactory = std::function<std::function<void()>(Actor*, NetBuffer*)>;

	template<typename DerivedType, typename... Args>
	void RegisterPacketHandler(const PACKET_ID packetId, void (DerivedType::* func)(Args...))
	{
		if (messageFactories.contains(packetId))
		{
			throw std::runtime_error("Packet ID already registered.");
		}

		messageFactories[packetId] = [func](Actor* actor, NetBuffer* packet) -> std::function<void()>
			{
				DerivedType* derived = static_cast<DerivedType*>(actor);
				auto argsOpt = Deserializer::Deserialize<Args...>(*packet);
				if (not argsOpt.has_value())
				{
					return []() { /* Handle deserialization failure */ };
				}
				return [derived, func, args = std::move(argsOpt.value())]()
					{
						std::apply([&](Args&&... unpackedArgs)
							{
								(derived->*func)(std::forward<Args>(unpackedArgs)...);
							}, std::move(args));
					};
			};
	}

public:
	[[nodiscard]]
	ActorIdType GetActorId() const { return actorId; }

protected:
	[[nodiscard]]
	ThreadIdType GetThreadId() const { return threadId; }

protected:
	std::unordered_map<PACKET_ID, MessageFactory> messageFactories;
	ThreadIdType threadId{};

protected:
	ActorIdType actorId{};
};