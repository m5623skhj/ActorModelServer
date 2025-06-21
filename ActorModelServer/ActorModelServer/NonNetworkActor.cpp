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
		ServerCore::GetInst().RegisterNonNetworkActor(this, threadId);
	}
}

NonNetworkActor::~NonNetworkActor()
{
	if (not isTesterActor)
	{
		ServerCore::GetInst().UnregisterNonNetworkActor(this, threadId);
	}
}

#if ENABLE_TEST_SUPPORT

void NonNetworkActor::PreTimerForTest()
{
	PreTimer();
}

void NonNetworkActor::OnTimerForTest()
{
	OnTimer();
}

void NonNetworkActor::PostTimerForTest()
{
	PostTimer();
}

#endif