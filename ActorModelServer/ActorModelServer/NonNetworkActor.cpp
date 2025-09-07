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

NonNetworkActor::~NonNetworkActor()
{
	if (not isTesterActor)
	{
		ServerCore::GetInst().UnregisterNonNetworkActor(this, threadId);
	}
}