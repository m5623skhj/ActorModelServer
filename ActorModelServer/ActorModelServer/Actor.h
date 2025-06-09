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
	T ReadPrimitive(const char*& data, size_t& remaining)
	{
		if (remaining < sizeof(T))
		{
			throw std::runtime_error("Deserialize: not enough data");
		}

		T value;
		std::memcpy(&value, data, sizeof(T));
		data += sizeof(T);
		remaining -= sizeof(T);
		return value;
	}

	inline std::string ReadString(const char*& data, size_t& remaining)
	{
		uint16_t length = ReadPrimitive<uint16_t>(data, remaining);
		if (remaining < length)
		{
			throw std::runtime_error("Deserialize: string too long");
		}

		std::string str(reinterpret_cast<const char*>(data), length);
		data += length;
		remaining -= length;
		return str;
	}

	template<typename T>
	T DeserializeOne(const char*& data, size_t& remaining)
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
	std::tuple<Args...> DeserializeImpl(const char*& data, size_t& remaining)
	{
		if constexpr (sizeof...(Args) == 0)
		{
			return std::tuple<>();
		}
		else if constexpr (sizeof...(Args) == 1)
		{
			return std::make_tuple(DeserializeOne<Args...>(data, remaining));
		}
		else
		{
			using FirstType = std::tuple_element_t<0, std::tuple<Args...>>;
			FirstType first = DeserializeOne<FirstType>(data, remaining);

			using RestTypes = decltype(std::apply([](auto, auto... rest)
				{
					return std::tuple<decltype(rest)...>{};
				}, std::tuple<Args...>{}));

			auto rest = std::apply([&](auto, auto... rest_args)
				{
					return DeserializeImpl<decltype(rest_args)...>(data, remaining);
				}, std::tuple<Args...>{});

			return std::tuple_cat(std::make_tuple(first), rest);
		}
	}

	template<typename... Args>
	std::tuple<Args...> Deserialize(NetBuffer& buffer)
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
				std::tuple<Args...> args = Deserializer::Deserialize<Args...>(*packet);
				return [=]() {
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