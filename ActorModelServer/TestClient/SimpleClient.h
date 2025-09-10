#pragma once
#include <thread>
#include "Queue.h"
#include "NetServerSerializeBuffer.h"
#include "Ringbuffer.h"

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
		return static_cast<int>(recvBufferQueue.GetRestSize());
	}

	NetBuffer* GetRecvBuffer();
	void SendPacket(NetBuffer* packetBuffer);

private:
	void DoRecv(char* recvBuffer);
	bool MakePacketsFromRingBuffer();
	static bool PacketDecode(NetBuffer& buffer);

	void DoSend();

private:
	CListBaseQueue<NetBuffer*> recvBufferQueue;
	CListBaseQueue<NetBuffer*> sendBufferQueue;
	CRingbuffer recvRingBuffer;
	HANDLE sendThreadEventHandle;
};