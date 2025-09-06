#pragma once
#include "Actor.h"

class ServerCore;

class NonNetworkActor : public Actor
{
public:
	friend ServerCore;

public:
	NonNetworkActor() = delete;
	explicit NonNetworkActor(bool inIsTesterActor);
	~NonNetworkActor() override;

protected:
	void PreTimer() override {}
	void OnTimer() override {}
	void PostTimer() override {}

private:
	bool isTesterActor{};
};