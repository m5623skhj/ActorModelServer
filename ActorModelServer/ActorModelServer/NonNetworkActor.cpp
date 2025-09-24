#include "PreCompile.h"
#include "NonNetworkActor.h"
#include "ServerCore.h"

NonNetworkActor::NonNetworkActor(const bool inIsTesterActor)
	: Actor()
	, isTesterActor(inIsTesterActor)
{
	threadId = ServerCore::GetInst().GetTargetThreadId(GetActorId());

	if (not isTesterActor)
	{
		ServerCore::GetInst().RegisterNonNetworkActor(shared_from_this(), threadId);
	}
}

void NonNetworkActor::UnregisterNonNetworkActor(const std::shared_ptr<NonNetworkActor>& unregisterActor)
{
	if (not unregisterActor->IsTesterActor())
	{
		ServerCore::GetInst().UnregisterNonNetworkActor(unregisterActor, unregisterActor->GetThreadId());
	}
}
