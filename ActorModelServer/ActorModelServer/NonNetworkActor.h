#pragma once
#include "Actor.h"

class NonNetworkActor : public Actor
{
public:
	NonNetworkActor();
	~NonNetworkActor() override;

public:
	void PreTimer() override {}
	void OnTimer() override {}
	void PostTimer() override {}
};