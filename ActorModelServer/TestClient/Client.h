#pragma once
#include "SimpleClient.h"

class IPacket;

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
	void SendPacket(IPacket& packet);

private:
	void RunLogicThread();
	void ProcessLogic(NetBuffer& buffer);

private:
	std::jthread logicThread;

private:
	void Pong(const NetBuffer& packet);
	static void AddActor(NetBuffer& packet);
};
