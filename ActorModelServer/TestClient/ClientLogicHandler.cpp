#include "PreCompile.h"
#include "../ContentsServer/Protocol.h"
#include "Client.h"
#include "ActorManager.h"
#include "ActorCreator.h"

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
		std::cout << "Failed to create actor. Actor ID: " << actorId << ", Actor Type: " << static_cast<unsigned int>(actorType) << '\n';
		return;
	}

	ActorManager::GetInst().AddActor(newActor);
}