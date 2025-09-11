#pragma once
#include "Actor.h"
#include "Player.h"
#include "../ActorModelServer/CoreType.h"
#include <memory>
#include <iostream>

namespace ActorCreator
{
	inline std::shared_ptr<Actor> CreateActor(const ActorIdType inActorId, const ACTOR_TYPE inActorType)
	{
		switch (inActorType)
		{
		case PLAYER:
		{
			return std::make_shared<Player>(inActorId);
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
