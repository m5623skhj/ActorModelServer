#include "PreCompile.h"
#include "ServerCore.h"
#include <ranges>
#include "Ticker.h"

static constexpr IOCompletionKeyType IOCP_CLOSE_KEY(0xffffffffffffffff, -1);
static CTLSMemoryPool<IOCompletionKeyType> ioCompletionKeyPool(dfNUM_OF_NETBUF_CHUNK, false);

ServerCore& ServerCore::GetInst()
{
	static ServerCore instance;
	return instance;
}

bool ServerCore::StartServer(const std::wstring& optionFilePath, SessionFactoryFunc&& factoryFunc)
{
	if (not OptionParsing(optionFilePath))
	{
		std::cout << "OptionParsing() failed" << '\n';
		return false;
	}

	if (not InitNetwork())
	{
		std::cout << "InitNetwork() failed" << '\n';
		return false;
	}

	if (not InitThreads())
	{
		std::cout << "InitThreads() failed" << '\n';
		return false;
	}

	if (not SetSessionFactory(std::move(factoryFunc)))
	{
		std::cout << "SessionFactoryFunc cannot be nullptr" << '\n';
		return false;
	}

	iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, numOfUsingWorkerThread);
	if (iocpHandle == INVALID_HANDLE_VALUE)
	{
		std::cout << "CreateIoCompletionPort() failed with " << GetLastError() << '\n';
		return false;
	}

	return true;
}

void ServerCore::StopServer()
{
	closesocket(listenSocket);
	acceptThread.join();

	for (BYTE i = 0; i < numOfWorkerThread; ++i)
	{
		PostQueuedCompletionStatus(iocpHandle, 0, reinterpret_cast<ULONG_PTR>(&IOCP_CLOSE_KEY), nullptr);
		ioThreads[i].join();
	}

	SetEvent(logicThreadEventStopHandle);
	for (BYTE i = 0; i < numOfLogicThread; ++i)
	{
		logicThreads[i].join();
		releaseThreads[i].join();
	}
	CloseHandle(iocpHandle);

	WSACleanup();
}

ThreadIdType ServerCore::GetTargetThreadId(const ActorIdType actorId) const
{
	return actorId % numOfLogicThread;
}

bool ServerCore::OptionParsing(const std::wstring& optionFilePath)
{
	_wsetlocale(LC_ALL, L"Korean");

	CParser parser;
	WCHAR cBuffer[BUFFER_MAX];

	FILE* fp;
	_wfopen_s(&fp, optionFilePath.c_str(), L"rt, ccs=UNICODE");
	if (fp == nullptr)
	{
		return false;
	}

	const int iJumpBom = ftell(fp);
	fseek(fp, 0, SEEK_END);
	const int iFileSize = ftell(fp);
	fseek(fp, iJumpBom, SEEK_SET);
	const int fileSize = static_cast<int>(fread_s(cBuffer, BUFFER_MAX, sizeof(WCHAR), BUFFER_MAX / 2, fp));
	const int iAmend = iFileSize - fileSize;
	fclose(fp);

	cBuffer[iFileSize - iAmend] = '\0';
	WCHAR* pBuff = cBuffer;

	if (!parser.GetValue_Short(pBuff, L"SERVER", L"BIND_PORT", &port))
		return false;
	if (!parser.GetValue_Byte(pBuff, L"SERVER", L"WORKER_THREAD", &numOfWorkerThread))
		return false;
	if (!parser.GetValue_Byte(pBuff, L"SERVER", L"USING_WORKER_THREAD", &numOfUsingWorkerThread))
		return false;
	if (!parser.GetValue_Byte(pBuff, L"SERVER", L"LOGIC_THREAD", &numOfLogicThread))
		return false;

	return true;
}

bool ServerCore::InitNetwork()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa))
	{
		std::cout << "WSAStartup() failed with " << GetLastError() << '\n';
		return false;
	}

	listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
	{
		std::cout << "socket() failed with " << GetLastError() << '\n';
		return false;
	}

	SOCKADDR_IN serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(port);
	if (bind(listenSocket, reinterpret_cast<SOCKADDR*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
	{
		std::cout << "bind() failed with " << GetLastError() << '\n';
		return false;
	}

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		std::cout << "listen() failed with " << GetLastError() << '\n';
		return false;
	}

	return true;
}

bool ServerCore::InitThreads()
{
	acceptThread = std::thread([this]() { RunAcceptThread(); });
	logicThreadEventStopHandle = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	packetAssembleStopEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	Ticker::GetInstance().Start(100);

	for (ThreadIdType i = 0; i < numOfWorkerThread; ++i)
	{
		ioThreads.emplace_back([this]() { this->RunIoThread(); });
	}
	for (ThreadIdType i = 0; i < numOfLogicThread; ++i)
	{
		sessionMapMutex.emplace_back(std::make_unique<std::shared_mutex>());
		nonNetworkActorMapMutex.emplace_back(std::make_unique<std::shared_mutex>());
		sessionMap.emplace_back();
		nonNetworkActorMap.emplace_back();
		releaseThreadsEventHandles.emplace_back(CreateEvent(nullptr, FALSE, FALSE, nullptr));
		releaseThreads.emplace_back([this, i]() { this->RunReleaseThread(i); });
		releaseThreadsQueue.emplace_back();
		releaseThreadsQueueMutex.emplace_back(std::make_unique<std::mutex>());
		packetAssembleThreadEvents.emplace_back(CreateEvent(nullptr, FALSE, FALSE, nullptr));
		logicThreads.emplace_back([this, i]() { this->RunLogicThread(i); });
	}

	return true;
}

void ServerCore::RunAcceptThread()
{
	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(clientAddr);

	while (not isStop)
	{
		const SOCKET clientSock = accept(listenSocket, reinterpret_cast<SOCKADDR*>(&clientAddr), &addrLen);
		if (clientSock == INVALID_SOCKET)
		{
			if (const DWORD error = GetLastError(); error == WSAEINTR)
			{
				break;
			}
			else
			{
				std::cout << "accept failed with " << error << " in RunAcceptThread()" << '\n';
				continue;
			}
		}

		const auto sessionId = ActorIdGenerator::GenerateActorId();
		auto newSession = CreateSession(sessionId, clientSock, GetTargetThreadId(sessionId));
		if (newSession == nullptr)
		{
			continue;
		}
		newSession->IncreaseIOCount();
		InsertSession(newSession);

		auto ioCompletionKey = ioCompletionKeyPool.Alloc();
		ioCompletionKey->sessionId = sessionId;
		ioCompletionKey->threadId = newSession->GetThreadId();
		if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(clientSock), iocpHandle, reinterpret_cast<ULONG_PTR>(ioCompletionKey), 0) != INVALID_HANDLE_VALUE)
		{
			newSession->DoRecv();
		}
		newSession->OnActorCreated();
		newSession->DecreaseIOCount();
	}

	std::cout << "Accept thread stopped" << '\n';
}

void ServerCore::RunIoThread()
{
	IOCompletionKeyType* ioCompletionKey{};
	LPOVERLAPPED overlapped{};
	DWORD transferred{};
	std::shared_ptr<Session> ioCompletedSession{ nullptr };

	while (not isStop)
	{
		if (ioCompletionKey != nullptr)
		{
			ioCompletionKeyPool.Free(ioCompletionKey);
		}

		ioCompletionKey = {};
		transferred = {};
		overlapped = {};
		ioCompletedSession.reset();

		const auto iocpRetval = GetQueuedCompletionStatus(iocpHandle, &transferred, reinterpret_cast<PULONG_PTR>(&ioCompletionKey), &overlapped, INFINITE);
		if (overlapped == nullptr)
		{
			std::cout << "Overlapped is NULL" << '\n';
			break;
		}

		if (ioCompletionKey == nullptr)
		{
			std::cout << "IoCompletionKey is nullptr" << '\n';
			continue;
		}

		if (*ioCompletionKey == IOCP_CLOSE_KEY)
		{
			PostQueuedCompletionStatus(iocpHandle, 0, reinterpret_cast<ULONG_PTR>(&IOCP_CLOSE_KEY), nullptr);
			break;
		}

		ioCompletedSession = FindSession(ioCompletionKey->sessionId, ioCompletionKey->threadId);
		if (ioCompletedSession == nullptr)
		{
			std::cout << "GetQueuedCompletionStatus success, but session is nullptr" << '\n';
			break;
		}

		if (iocpRetval == false)
		{
			if (const auto error = GetLastError(); error != ERROR_NETNAME_DELETED)
			{
				std::cout << "GetQueuedCompletionStatus failed with " << GetLastError() << '\n';
			}
			ioCompletedSession->DecreaseIOCount();

			continue;
		}

		OnIoCompleted(*ioCompletedSession, overlapped, transferred);
	}

	std::cout << "IO thread stopped" << '\n';
}

void ServerCore::RunLogicThread(const ThreadIdType threadId)
{
	static constexpr int SLEEP_TIME_MS = 33;
	const HANDLE eventHandle = { logicThreadEventStopHandle };
	while (not isStop)
	{
		switch (WaitForSingleObject(eventHandle, SLEEP_TIME_MS))
		{
		case WAIT_TIMEOUT:
		{
			PreWakeLogicThread(threadId);
			OnWakeLogicThread(threadId);
			PostWakeLogicThread(threadId);

			break;
		}
		case WAIT_OBJECT_0:
		{
			break;
		}
		default:
		{
			std::cout << "Invalid wait result in RunLogicThread()" << '\n';
			break;
		}
		}
	}

	std::cout << "Logic thread " << threadId << " stopped" << '\n';
}

void ServerCore::RunReleaseThread(const ThreadIdType threadId)
{
	const HANDLE eventHandles[2] = { releaseThreadsEventHandles[threadId], logicThreadEventStopHandle };
	while (not isStop)
	{
		switch (WaitForMultipleObjects(2, eventHandles, FALSE, INFINITE))
		{
		case WAIT_OBJECT_0:
		{
			std::scoped_lock lock(*releaseThreadsQueueMutex[threadId]);
			while (!releaseThreadsQueue[threadId].empty())
			{
				const auto [targetSessionId, targetThreadId] = releaseThreadsQueue[threadId].front();
				if (auto session = FindSession(targetSessionId, targetThreadId); session != nullptr)
				{
					EraseSession(targetSessionId, targetThreadId);
					session->OnDisconnected();
				}
			}
		}
		break;
		case WAIT_OBJECT_0 + 1:
		{
			Sleep(RELEASE_THREAD_STOP_SLEEP_TIME);
			EraseAllSession(threadId);
			break;
		}
		default:
		{
			std::cout << "Invalid wait result in RunReleaseThread()" << '\n';
			break;
		}
		}
	}
}

bool ServerCore::OnIoCompleted(Session& ioCompletedSession, const LPOVERLAPPED& overlapped, const DWORD transferred)
{
	if (transferred == 0)
	{
		ioCompletedSession.DecreaseIOCount();
		return true;
	}

	if (&ioCompletedSession.recvIOData.overlapped == overlapped)
	{
		return OnRecvIoCompleted(ioCompletedSession, transferred);
	}
	else if (&ioCompletedSession.sendIOData.overlapped == overlapped)
	{
		return OnSendIoCompleted(ioCompletedSession);
	}

	return false;
}

bool ServerCore::OnRecvIoCompleted(Session& session, const DWORD transferred)
{
	session.recvIOData.ringBuffer.MoveWritePos(static_cast<int>(transferred));
	int restSize = session.recvIOData.ringBuffer.GetUseSize();

	while (restSize > df_HEADER_SIZE)
	{
		NetBuffer& buffer = *NetBuffer::Alloc();
		bool sendSuccess = false;

		do
		{
			if (not RecvStreamToBuffer(session, buffer, restSize))
			{
				std::cout << "RecvStreamToBuffer failed. Session id : " << session.GetSessionId() << '\n';
				break;
			}

			auto messageOpt = session.CreateMessageFromPacket(buffer);
			if (not messageOpt.has_value())
			{
				std::cout << "CreateMessageFromPacket failed. Session id : " << session.GetSessionId() << '\n';
				break;
			}

			if (not session.SendMessage(std::move(messageOpt.value())))
			{
				std::cout << "SendMessage failed. Session id : " << session.GetSessionId() << '\n';
				break;
			}

			sendSuccess = true;
		} while (false);

		NetBuffer::Free(&buffer);
		if (sendSuccess == true)
		{
			continue;
		}

		session.DecreaseIOCount();
		return false;
	}

	return session.DoRecv();
}

bool ServerCore::OnSendIoCompleted(Session& session)
{
	for (int i = 0; i < session.sendIOData.bufferCount; ++i)
	{
		NetBuffer::Free(session.sendIOData.sendBufferStore[i]);
	}

	session.sendIOData.bufferCount = 0;
	session.sendIOData.ioMode = IO_MODE::IO_NONE_SENDING;
	return session.DoSend();
}

bool ServerCore::RecvStreamToBuffer(Session& session, OUT NetBuffer& buffer, OUT int& restSize)
{
	session.recvIOData.ringBuffer.Peek((char*)buffer.m_pSerializeBuffer, df_HEADER_SIZE);
	buffer.m_iRead = 0;

	WORD payloadLength;
	buffer >> payloadLength;
	if (restSize < payloadLength + df_HEADER_SIZE)
	{
		if (payloadLength > dfDEFAULTSIZE)
		{
			return false;
		}
	}

	session.recvIOData.ringBuffer.RemoveData(df_HEADER_SIZE);
	const int dequeuedSize = session.recvIOData.ringBuffer.Dequeue(&buffer.m_pSerializeBuffer[buffer.m_iWrite], payloadLength);
	buffer.m_iWrite += dequeuedSize;
	if (PacketDecode(buffer) == false)
	{
		return false;
	}
	restSize -= dequeuedSize + df_HEADER_SIZE;

	return true;
}

bool ServerCore::PacketDecode(OUT NetBuffer& buffer)
{
	return buffer.Decode();
}

bool ServerCore::SetSessionFactory(SessionFactoryFunc&& inSessionFactoryFunc)
{
	if (inSessionFactoryFunc == nullptr)
	{
		return false;
	}

	sessionFactory = std::move(inSessionFactoryFunc);
	return true;
}

std::shared_ptr<Session> ServerCore::CreateSession(const SessionIdType sessionId, const SOCKET sock, const ThreadIdType threadId) const
{
	if (sessionFactory == nullptr)
	{
		throw std::logic_error("SessionFactoryFunc is not set. Call SetSessionFactory() first.");
	}
	return sessionFactory(sessionId, sock, threadId);
}

void ServerCore::PreWakeLogicThread(const ThreadIdType threadId)
{
	{
		std::shared_lock lock(*sessionMapMutex[threadId]);
		for (auto& session : sessionMap[threadId] | std::views::values)
		{
			if (session != nullptr)
			{
				session->PreTimer();
			}
		}
	}

	{
		std::shared_lock lock(*nonNetworkActorMapMutex[threadId]);
		for (const auto& actor : nonNetworkActorMap[threadId] | std::views::values)
		{
			if (actor != nullptr)
			{
				actor->PreTimer();
			}
		}
	}
}

void ServerCore::OnWakeLogicThread(const ThreadIdType threadId)
{
	{
		std::shared_lock lock(*sessionMapMutex[threadId]);
		for (auto& session : sessionMap[threadId] | std::views::values)
		{
			if (session != nullptr)
			{
				session->OnTimer();
			}
		}
	}

	{
		std::shared_lock lock(*nonNetworkActorMapMutex[threadId]);
		for (const auto& nonNetworkActor : nonNetworkActorMap[threadId] | std::views::values)
		{
			if (nonNetworkActor != nullptr)
			{
				nonNetworkActor->OnTimer();
			}
		}
	}
}

void ServerCore::PostWakeLogicThread(const ThreadIdType threadId)
{
	{
		std::shared_lock lock(*sessionMapMutex[threadId]);
		for (auto& session : sessionMap[threadId] | std::views::values)
		{
			if (session != nullptr)
			{
				session->PostTimer();
			}
		}
	}

	{
		std::shared_lock lock(*nonNetworkActorMapMutex[threadId]);
		for (const auto& actor : nonNetworkActorMap[threadId] | std::views::values)
		{
			if (actor != nullptr)
			{
				actor->PostTimer();
			}
		}
	}
}

void ServerCore::ReleaseSession(const SessionIdType sessionId, const ThreadIdType threadId)
{
	std::scoped_lock lock(*releaseThreadsQueueMutex[threadId]);
	releaseThreadsQueue[threadId].push({ .sessionId = sessionId, .threadId = threadId });
	SetEvent(releaseThreadsEventHandles[threadId]);
}

bool ServerCore::RegisterNonNetworkActor(const std::shared_ptr<NonNetworkActor>& actor, const ThreadIdType threadId)
{
	if (actor == nullptr)
	{
		return false;
	}

	std::unique_lock lock(*nonNetworkActorMapMutex[threadId]);
	nonNetworkActorMap[threadId].insert({ actor->GetActorId(), actor });

	return true;
}

bool ServerCore::UnregisterNonNetworkActor(const std::shared_ptr<NonNetworkActor>& actor, const ThreadIdType threadId)
{
	if (actor == nullptr)
	{
		return false;
	}

	std::unique_lock lock(*nonNetworkActorMapMutex[threadId]);
	if (nonNetworkActorMap[threadId].erase(actor->GetActorId()) == 0)
	{
		return false;
	}

	return true;
}

std::shared_ptr<Actor> ServerCore::FindActor(const ActorIdType actorId, const bool isNetworkActor)
{
	auto const threadId = GetTargetThreadId(actorId);
	if (isNetworkActor)
	{
		return FindSession(actorId, threadId);
	}

	return FindNonNetworkActor(actorId);
}

std::shared_ptr<NonNetworkActor> ServerCore::FindNonNetworkActor(const ActorIdType actorId)
{
	{
		auto const threadId = GetTargetThreadId(actorId);
		std::shared_lock lock(*nonNetworkActorMapMutex[threadId]);
		if (const auto itor = nonNetworkActorMap[threadId].find(actorId); itor != nonNetworkActorMap[threadId].end())
		{
			return itor->second;
		}
	}

	return nullptr;
}

void ServerCore::InsertSession(std::shared_ptr<Session>& session)
{
	std::unique_lock lock(*sessionMapMutex[session->GetThreadId()]);

	sessionMap[session->GetThreadId()].insert({ session->GetSessionId(), session });
	numOfUser.fetch_add(1, std::memory_order_relaxed);
}

void ServerCore::EraseAllSession(const ThreadIdType threadId)
{
	std::unique_lock lock(*sessionMapMutex[threadId]);

	for (auto& session : sessionMap[threadId] | std::views::values)
	{
		if (session != nullptr)
		{
			session->OnDisconnected();
		}
	}

	sessionMap[threadId].clear();
	numOfUser.store(0, std::memory_order_relaxed);
}

void ServerCore::EraseSession(const SessionIdType sessionId, const ThreadIdType threadId)
{
	std::unique_lock lock(*sessionMapMutex[threadId]);

	sessionMap[threadId].erase(sessionId);
	numOfUser.fetch_sub(1, std::memory_order_relaxed);
}

std::shared_ptr<Session> ServerCore::FindSession(const SessionIdType sessionId, const ThreadIdType threadId)
{
	std::shared_lock lock(*sessionMapMutex[threadId]);

	if (const auto itor = sessionMap[threadId].find(sessionId); itor != sessionMap[threadId].end())
	{
		return itor->second;
	}

	return nullptr;
}
