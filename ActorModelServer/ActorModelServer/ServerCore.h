#pragma once
#include <thread>
#include <vector>
#include <Windows.h>
#include <string>
#include "Session.h"
#include <shared_mutex>

#pragma comment(lib, "ws2_32.lib")

using ThreadIdType = unsigned char;

constexpr unsigned int logicThreadStopSleepTime = 10000;

class ServerCore
{
private:
	ServerCore() = default;
	~ServerCore() = default;

public:
	static ServerCore& GetInst();

public:
	bool StartServer(const std::wstring& optionFilePath);
	void StopServer();

private:
	bool OptionParsing(const std::wstring optionFilePath);
	bool InitNetwork();
	bool InitThreads();

private:
	void RunAcceptThread();
	void RunIOThreads();
	void RunLogicThreads(const ThreadIdType threadId);

private:
	bool OnRecvIOCompleted(Session& session, const DWORD transferred);
	bool OnSendIOCompleted(Session& session);

	bool RecvStreamToBuffer(Session& session, OUT NetBuffer& buffer, OUT int restSize);
	bool PacketDecode(OUT NetBuffer& buffer);

private:
	void InsertSession(std::shared_ptr<Session>& session);
	void EraseSession(const SessionIdType sessionId, const ThreadIdType threadId);
	std::shared_ptr<Session> FindSession(const SessionIdType sessionId, ThreadIdType threadId);

private:
	bool isStop{};

private:
	SessionIdType sessionIdGenerator{};

private:
	short port{};
	BYTE numOfWorkerThread{};
	BYTE numOfUsingWorkerThread{};
	BYTE numOfLogicThread{};

	std::thread acceptThread;
	std::vector<std::thread> ioThreads;
	std::vector<std::thread> logicThreads;
	std::vector<HANDLE> logicThreadEventHandles;
	HANDLE logicThreadEventStopHandle{};

	HANDLE iocpHandle{ INVALID_HANDLE_VALUE };

	SOCKET listenSocket{ INVALID_SOCKET };

private:
	std::atomic_int numOfUser{};
	std::vector<std::unique_ptr<std::shared_mutex>> sessionMapMutex;
	std::vector<std::unordered_map<SessionIdType, std::shared_ptr<Session>>> sessionMap;
};

