#include "PreCompile.h"
#include "ActorManager.h"

std::shared_ptr<Actor> ActorManager::FindActor(const ActorIdType actorId)
{
	std::shared_lock lock(actorMapMutex);
	const auto itor = actorMap.find(actorId);
	if (itor == actorMap.end())
	{
		return nullptr;
	}

	return itor->second;
}

void ActorManager::AddActor(const std::shared_ptr<Actor>& actor)
{
	if (actor == nullptr)
	{
		return;
	}

	std::unique_lock lock(actorMapMutex);
	actorMap[actor->GetActorId()] = actor;

	actor->OnActorCreated();
}

void ActorManager::RemoveActor(const ActorIdType actorId)
{
	std::shared_ptr<Actor> eraseActor = nullptr;
	std::unique_lock lock(actorMapMutex);
	{
		const auto itor = actorMap.find(actorId);
		if (itor == actorMap.end())
		{
			return;
		}

		eraseActor = itor->second;
		actorMap.erase(actorId);
	}

	if (eraseActor != nullptr)
	{
		eraseActor->OnActorDestroyed();
	}
}

void ActorManager::ClearAllActors()
{
	std::unique_lock lock(actorMapMutex);
	actorMap.clear();
}