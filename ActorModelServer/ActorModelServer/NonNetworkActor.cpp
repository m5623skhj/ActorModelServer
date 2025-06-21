#include "PreCompile.h"
#include "NonNetworkActor.h"
#include "ServerCore.h"

NonNetworkActor::NonNetworkActor()
	: Actor()
{
	threadId = ServerCore::GetInst().GetTargetThreadId(GetActorId());
	ServerCore::GetInst().RegisterNonNetworkActor(this, threadId);
}

NonNetworkActor::~NonNetworkActor()
{
	ServerCore::GetInst().UnregisterNonNetworkActor(this, threadId);
}
