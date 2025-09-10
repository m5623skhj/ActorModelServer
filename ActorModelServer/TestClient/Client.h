#pragma once
#include "SimpleClient.h"

class Client : public SimpleClient
{
public:
	Client() = default;
	~Client() override = default;

public:
	static Client& GetInst();

public:
	bool Start(const std::wstring& optionFilePath);
	void Stop();

private:
	void RunLogicThread();
	void ProcessLogic(NetBuffer& buffer);

private:
	std::jthread logicThread;

private:
	void Pong(NetBuffer& packet);
};