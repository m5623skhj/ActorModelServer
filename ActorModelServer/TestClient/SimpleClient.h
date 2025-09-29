#pragma once
#include <thread>
#include "NetServerSerializeBuffer.h"
#include "Ringbuffer.h"
#include <mutex>
#include <queue>

class SimpleClient
{
protected:
	SimpleClient() = default;
	virtual ~SimpleClient() = default;

protected:
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

public:
	[[nodiscard]]
	bool IsNeedStop() const { return needStop; }

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
		std::scoped_lock lock(recvBufferDequeMutex);
		return static_cast<int>(recvBufferQueue.size());
	}

	NetBuffer* GetRecvBuffer();
	void SendPacket(NetBuffer* packetBuffer);

private:
	void OnConnected();

	void DoRecv(char* recvBuffer);
	bool MakePacketsFromRingBuffer();
	static bool PacketDecode(NetBuffer& buffer);

	void DoSend();

private:
	std::mutex recvBufferDequeMutex;
	std::mutex sendBufferDequeMutex;
	std::queue<NetBuffer*> recvBufferQueue;
	std::queue<NetBuffer*> sendBufferQueue;
	CRingbuffer recvRingBuffer;
	HANDLE sendThreadEventHandle;
};