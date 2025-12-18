#pragma once

using ActorId = unsigned long long;

class Actor
{
public:
	explicit Actor(ActorId inActorId);
	virtual ~Actor() = default;

	template<typename Derived, typename... Args>
	static std::shared_ptr<Derived> Create(Args&&... args)
	{
		static_assert(std::is_base_of_v<Actor, Derived>, "Derived must derive from Actor");

		auto actor = std::make_shared<Derived>(std::forward<Args>(args)...);
		Actor::RegisterActor(actor);

		return actor;
	}
	virtual void OnActorCreated();
	virtual void OnActorDestroyed();

	static void RegisterActor(const std::shared_ptr<Actor>& registerActor);
	static void UnregisterActor(ActorId targetActorId);

public:
	[[nodiscard]]
	ActorId GetActorId() const { return actorId; }

private:
	ActorId actorId;
};