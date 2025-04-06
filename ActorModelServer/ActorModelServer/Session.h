#pragma once
#include "Actor.h"
#include "Ringbuffer.h"
#include "NetServerSerializeBuffer.h"
#include "LockFreeQueue.h"
#include "Queue.h"

class ServerCore;

using ThreadIdType = unsigned char;
using SessionIdType = unsigned long long;

static int constexpr ONE_SEND_WSABUF_MAX = 20;

enum class IO_MODE : LONG
{
	IO_NONE_SENDING = 0,
	IO_SENDING = 1,
};

enum class IO_POST_ERROR : char
{
	SUCCESS = 0
	, IS_DELETED_SESSION
	, FAILED_RECV_POST
	, FAILED_SEND_POST
	, INVALID_OPERATION_TYPE
};

class Session : public Actor
{
public:
	friend ServerCore;

public:
	Session() = delete;
	explicit Session(const SessionIdType inSessionIdType, const SOCKET& inSock, const ThreadIdType inThreadId);
	~Session() override = default;

public:
	void Disconnect();

protected:
	void OnConnected();
	void OnDisconnected();

	void PreTimer();
	void OnTimer();
	void PostTimer();

private:
	bool DoRecv();
	bool DoSend();

public:
	SessionIdType GetSessionId() const { return sessionId; }
	ThreadIdType GetThreadId() const { return threadId; }

private:
	inline void IncreaseIOCount();
	inline void DecreaseIOCount();
	inline void ReleaseSession();

private:
	SOCKET sock{};
	SessionIdType sessionId{};
	std::atomic_int ioCount{};
	bool ioCancel{};
	ThreadIdType threadId{};

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
		NetBuffer* sendBufferStore[ONE_SEND_WSABUF_MAX];
	};
	SendIOData sendIOData;
};