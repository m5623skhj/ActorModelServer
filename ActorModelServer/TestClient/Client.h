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
	bool StartClient(const std::wstring& optionFilePath);
	void StopClient();

private:
	void RunLogicThread();
	void ProcessLogic(NetBuffer& buffer);

private:
	std::jthread logicThread;

private:
	void Pong(const NetBuffer& packet);
	static void AddActor(NetBuffer& packet);
};