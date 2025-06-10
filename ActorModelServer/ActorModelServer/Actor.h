#pragma once
#include <optional>
#include <queue>
#include <mutex>
#include <utility>
#include "CoreType.h"
#include "PacketManager.h"
#include "NetServerSerializeBuffer.h"

namespace Deserializer
{
	template<typename T>
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
	std::optional<std::tuple<Args...>> Deserialize(NetBuffer& buffer)
	{
		const char* ptr = buffer.GetReadBufferPtr();
		size_t remaining = buffer.GetUseSize();

		return DeserializeImpl<Args...>(ptr, remaining);
	}
}

class Actor
{
public:
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

public:
	void Stop();

protected:
	void ProcessMessage();

protected:
	std::mutex queueMutex;

	std::queue<Message> consumerQueue;
	std::queue<Message> storeQueue;

	std::atomic_bool isStop{};

public:
	template<typename DerivedType, typename Func, typename... Args>
	void RegisterPacketHandler(const PacketId type, Func DerivedType::* func)
	{
		messageFactories[type] = [func](Actor* actor, NetBuffer* packet) -> std::function<void()>
			{
				DerivedType* derived = static_cast<DerivedType*>(actor);
				auto argsOpt = Deserializer::Deserialize<Args...>(*packet);
				if (not argsOpt.has_value())
				{
					return []() { /* Handle deserialization failure */ };
				}

				return [derived, func, args = std::move(std::move(argsOpt.value()))]()
					{
						std::apply([&](Args&&... unpackedArgs)
							{
								(derived->*func)(std::forward<Args>(unpackedArgs)...);
							}, std::move(args));
					};
			};
	}

private:
	using MessageFactory = std::function<std::function<void()>(Actor*, NetBuffer*)>;
	std::unordered_map<PacketId, MessageFactory> messageFactories;
};