#pragma once
#include <thread>
#include <vector>
#include <Windows.h>
#include <string>

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
	void RunIOThreads(const ThreadIdType threadId);
	void RunLogicThreads(const ThreadIdType threadId);

private:
	bool isStop{};

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
};