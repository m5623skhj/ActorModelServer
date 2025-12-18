#include "PreCompile.h"
#include "Actor.h"
#include "ActorManager.h"

Actor::Actor(const ActorId inActorId)
	: actorId(inActorId)
{
}

void Actor::OnActorCreated()
{
}

void Actor::OnActorDestroyed()
{
}

void Actor::RegisterActor(const std::shared_ptr<Actor>& registerActor)
{
	ActorManager::GetInst().AddActor(registerActor);
}

void Actor::UnregisterActor(const ActorId targetActorId)
{
	ActorManager::GetInst().RemoveActor(targetActorId);
}
