#pragma once
#include "NonNetworkActor.h"

namespace ActorCreator
{
	template<typename Derived, typename... Args>
	static std::shared_ptr<Derived> Create(Args&&... args)
	{
		static_assert(std::is_base_of_v<Actor, Derived>, "Derived must derive from Actor");

		auto actor = std::make_shared<Derived>(std::forward<Args>(args)...);
		if constexpr (std::is_base_of_v<NonNetworkActor, Derived>)
		{
			NonNetworkActor::RegisterNonNetworkActor(actor);
		}
		else
		{
			actor->OnActorCreated();
		}	

		return actor;
	}
}
