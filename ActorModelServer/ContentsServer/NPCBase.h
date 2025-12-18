#pragma once
#include "NonNetworkActor.h"

class NPCBase : public NonNetworkActor
{
public:
	NPCBase() = delete;
	explicit NPCBase(bool inIsTesterActor = false);
	~NPCBase() override = default;

public:
	void OnActorCreated() override;
	void OnActorDestroyed() override;
};