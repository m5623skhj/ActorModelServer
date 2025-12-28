#pragma once
#include <unordered_map>
#include "Actor.h"
#include <memory>
#include <shared_mutex>
#include "../ActorModelServer/CoreType.h"

class ActorManager
{
private:
	ActorManager() = default;
	~ActorManager() = default;
public:
	ActorManager(const ActorManager&) = delete;
	ActorManager& operator=(const ActorManager&) = delete;
	ActorManager(const ActorManager&&) = delete;
	ActorManager& operator=(const ActorManager&&) = delete;

public:
	static ActorManager& GetInst()
	{
		static ActorManager instance;
		return instance;
	}

public:
	[[nodiscard]]
	std::shared_ptr<Actor> FindActor(ActorIdType actorId);
	void AddActor(const std::shared_ptr<Actor>& actor);
	void RemoveActor(ActorIdType actorId);
	void ClearAllActors();

private:
	std::unordered_map<ActorIdType, std::shared_ptr<Actor>> actorMap;
	std::shared_mutex actorMapMutex;
};
