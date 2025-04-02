#pragma once
#include "Actor.h"
#include "Ringbuffer.h"
#include "NetServerSerializeBuffer.h"
#include "LockFreeQueue.h"
#include "Queue.h"

class ServerCore;

using SessionIdType = unsigned long long;

enum class IO_MODE : LONG
{
	IO_NONE_SENDING = 0,
	IO_SENDING = 1,
};

class Session : public Actor
{
public:
	friend ServerCore;

public:
	Session() = delete;
	explicit Session(SessionIdType inSessionIdType, const SOCKET& inSock);
	~Session() override = default;

public:
	void OnConnected();
	void OnDisconnected();
	void OnRecv();

private:
	void DoRecv();
	void DoSend();

public:
	SessionIdType GetSessionId() { return sessionId; }

private:
	void IncreaseIOCount();
	void DecreaseIOCount();
	void ReleaseSession();

private:
	SOCKET sock{};
	SessionIdType sessionId{};
	std::atomic_int ioCount{};
	bool ioCancel{};

	std::atomic_bool isUsingSession{};

private:
	struct RecvIOData
	{
		OVERLAPPED overlapped;
		CRingbuffer ringBuffer;
		CListBaseQueue<NetBuffer*> recvStoredQueue;
	};
	RecvIOData recvIOData;
	
	struct SendIOData
	{
		int bufferCount{};
		std::atomic<IO_MODE> ioMode{ IO_MODE::IO_NONE_SENDING };
		OVERLAPPED overlapped;
		CLockFreeQueue<NetBuffer*> sendQueue;
	};
	SendIOData sendIOData;
};