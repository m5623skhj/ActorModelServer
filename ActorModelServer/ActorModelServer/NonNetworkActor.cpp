#include "PreCompile.h"
#include "NonNetworkActor.h"
#include "ServerCore.h"

NonNetworkActor::NonNetworkActor(const bool inIsTesterActor)
	: isTesterActor(inIsTesterActor)
{
}

void NonNetworkActor::RegisterNonNetworkActor(const std::shared_ptr<NonNetworkActor>& registerActor)
{
	if (not registerActor->IsTesterActor())
	{
		ServerCore::GetInst().RegisterNonNetworkActor(registerActor, registerActor->GetThreadId());
	}
}

void NonNetworkActor::UnregisterNonNetworkActor(const std::shared_ptr<NonNetworkActor>& unregisterActor)
{
	if (not unregisterActor->IsTesterActor())
	{
		ServerCore::GetInst().UnregisterNonNetworkActor(unregisterActor, unregisterActor->GetThreadId());
	}
}
