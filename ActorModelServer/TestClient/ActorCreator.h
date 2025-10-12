#pragma once
#include "Actor.h"
#include "Player.h"
#include <memory>
#include <iostream>
#include "../ContentsServer/ActorType.h"
#include "Logger.h"
#include "LogExtension.h"

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
			std::string logString = "[ActorCreator] Unknown actor type: " + std::to_string(static_cast<unsigned int>(inActorType));
			LOG_ERROR(logString);
		}
		break;
		}

		return nullptr;
	}
}
