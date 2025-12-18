#include "PreCompile.h"
#include "NPCBase.h"

NPCBase::NPCBase(const bool inIsTesterActor)
	: NonNetworkActor(inIsTesterActor)
{
}

void NPCBase::OnActorCreated()
{
}

void NPCBase::OnActorDestroyed()
{
}