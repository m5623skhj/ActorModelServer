#pragma once
#include <thread>

class SimpleClient
{
private:
	SimpleClient() = default;
	~SimpleClient() = default;

public:
	static SimpleClient& GetInst();

public:
	[[nodiscard]]
	bool Start(const std::wstring& optionFilePath);
	void Stop();

private:
	[[nodiscard]]
	bool TryConnectToServer();
	[[nodiscard]]
	bool ConnectToServer() const;

	void CreateAllThreads();
	void WaitStopAllThreads();

	void RunRecvThread();
	void RunSendThread();

private:
	[[nodiscard]]
	bool ReadOptionFile(const std::wstring& optionFilePath);

private:
	bool needStop{ false };

	SOCKET sessionSocket;
	WCHAR targetIp[16];
	WORD targetPort;

	std::jthread recvThread;
	std::jthread sendThread;
};