#pragma once
#include "Actor.h"
#include "Player.h"
#include <memory>
#include <iostream>
#include "../ContentsServer/ActorType.h"

namespace ActorCreator
{
	inline std::shared_ptr<Actor> CreateActor(const ActorIdType inActorId, const ACTOR_TYPE inActorType)
	{
		switch (inActorType)
		{
		case PLAYER:
		{
			return Actor::Create<Player>(inActorType);
		}
		default:
		{
			std::cout << "Unknown actor type: " << static_cast<unsigned int>(inActorType) << '\n';
		}
		break;
		}

		return nullptr;
	}
}
