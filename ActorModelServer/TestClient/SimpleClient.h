#pragma once
#include <thread>
#include "Queue.h"
#include "NetServerSerializeBuffer.h"
#include "Ringbuffer.h"

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

protected:
	int GetRecvBufferSize()
	{
		return static_cast<int>(recvBufferQueue.GetRestSize());
	}

	NetBuffer* GetRecvBuffer();

private:
	void DoRecv(char* recvBuffer);
	bool MakePacketsFromRingBuffer();
	static bool PacketDecode(NetBuffer& buffer);

private:
	CListBaseQueue<NetBuffer*> recvBufferQueue;
	CRingbuffer recvRingBuffer;
};