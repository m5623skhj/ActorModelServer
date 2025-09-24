#pragma once
#include "Actor.h"
#include <memory>

class ServerCore;

class NonNetworkActor : public Actor, public std::enable_shared_from_this<NonNetworkActor>
{
public:
	friend ServerCore;

public:
	NonNetworkActor() = delete;
	explicit NonNetworkActor(bool inIsTesterActor = false);
	~NonNetworkActor() override = default;

public:
	static void UnregisterNonNetworkActor(const std::shared_ptr<NonNetworkActor>& unregisterActor);

protected:
	void PreTimer() override {}
	void OnTimer() override {}
	void PostTimer() override {}

public:
	bool IsTesterActor() const { return isTesterActor; }

private:
	bool isTesterActor{};
};