#include "PreCompile.h"
#include "../Common/Protocol.h"
#include "Client.h"
#include "ActorManager.h"
#include "ActorCreator.h"
#include "Logger.h"
#include "LogExtension.h"

void Client::Pong(const NetBuffer& packet)
{
	UNREFERENCED_PARAMETER(packet);

	Ping ping;
	SendPacket(ping);
}

void Client::AddActor(NetBuffer& packet)
{
	ActorIdType actorId;
	ACTOR_TYPE actorType;
	packet >> actorId;
	packet >> actorType;

	const auto newActor = ActorCreator::CreateActor(actorId, actorType);
	if (newActor == nullptr)
	{
		const std::string logString = "[Client] Failed to create actor. Actor ID: " + std::to_string(actorId) + ", Actor Type: " + std::to_string(static_cast<unsigned int>(actorType));
		LOG_ERROR(logString);
		return;
	}

	ActorManager::GetInst().AddActor(newActor);
}