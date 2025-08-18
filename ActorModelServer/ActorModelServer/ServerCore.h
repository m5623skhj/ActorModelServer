#pragma once
#include <thread>
#include <vector>
#include <Windows.h>
#include <string>
#include <shared_mutex>
#include "Session.h"

#include "NonNetworkActor.h"

#pragma comment(lib, "ws2_32.lib")

struct IOCompletionKeyType
{
	bool operator==(const IOCompletionKeyType& other) const
	{
		return sessionId == other.sessionId && threadId == other.threadId;
	}

	SessionIdType sessionId;
	ThreadIdType threadId;
};
using ReleaseSessionKey = IOCompletionKeyType;
using SessionFactoryFunc = std::function<std::shared_ptr<Session>(SessionIdType, SOCKET, ThreadIdType)>;

class ServerCore
{
private:
	ServerCore() = default;
	~ServerCore() = default;

public:
	static ServerCore& GetInst();

public:
	bool StartServer(const std::wstring& optionFilePath, SessionFactoryFunc&& factoryFunc);
	void StopServer();
	bool IsStop() const { return isStop; }

public:
	[[nodiscard]]
	ThreadIdType GetTargetThreadId(ActorIdType actorId) const;

private:
	bool OptionParsing(const std::wstring& optionFilePath);
	bool InitNetwork();
	bool InitThreads();

private:
	void RunAcceptThread();
	void RunIoThread();
	void RunLogicThread(ThreadIdType threadId);
	void RunReleaseThread(ThreadIdType threadId);

private:
	static bool OnIoCompleted(Session& ioCompletedSession, const LPOVERLAPPED& overlapped, DWORD transferred);
	static bool OnRecvIoCompleted(Session& session, DWORD transferred);
	static bool OnSendIoCompleted(Session& session);

	static bool RecvStreamToBuffer(Session& session, OUT NetBuffer& buffer, OUT int& restSize);
	static inline bool PacketDecode(OUT NetBuffer& buffer);

private:
	bool SetSessionFactory(SessionFactoryFunc&& factoryFunc);
	std::shared_ptr<Session> CreateSession(SessionIdType sessionId, SOCKET sock, ThreadIdType threadId) const;

private:
	void PreWakeLogicThread(ThreadIdType threadId);
	void OnWakeLogicThread(ThreadIdType threadId);
	void PostWakeLogicThread(ThreadIdType threadId);

public:
	void ReleaseSession(SessionIdType sessionId, ThreadIdType threadId);

public:
	bool RegisterNonNetworkActor(NonNetworkActor* actor, ThreadIdType threadId);
	bool UnregisterNonNetworkActor(const NonNetworkActor* actor, ThreadIdType threadId);

private:
	void InsertSession(std::shared_ptr<Session>& session);
	void EraseAllSession(ThreadIdType threadId);
	void EraseSession(SessionIdType sessionId, ThreadIdType threadId);
	[[nodiscard]]
	std::shared_ptr<Session> FindSession(SessionIdType sessionId, ThreadIdType threadId);

private:
	bool isStop{};

private:
	SessionFactoryFunc sessionFactory{};

private:
	short port{};
	BYTE numOfWorkerThread{};
	BYTE numOfUsingWorkerThread{};
	BYTE numOfLogicThread{};

	std::thread acceptThread;
	std::vector<std::thread> ioThreads;
	std::vector<std::thread> logicThreads;
	std::vector<HANDLE> packetAssembleThreadEvents;
	HANDLE packetAssembleStopEvent;
	std::vector<std::thread> releaseThreads;
	std::vector<HANDLE> releaseThreadsEventHandles;
	std::vector<CLockFreeQueue<ReleaseSessionKey>> releaseThreadsQueue;
	HANDLE logicThreadEventStopHandle{};

	HANDLE iocpHandle{ INVALID_HANDLE_VALUE };

	SOCKET listenSocket{ INVALID_SOCKET };

private:
	std::atomic_int numOfUser{};
	std::vector<std::unique_ptr<std::shared_mutex>> sessionMapMutex;
	std::vector<std::unordered_map<SessionIdType, std::shared_ptr<Session>>> sessionMap;

	std::vector<std::unique_ptr<std::shared_mutex>> nonNetworkActorMapMutex;
	std::vector<std::unordered_map<ActorIdType, NonNetworkActor*>> nonNetworkActorMap;
};

