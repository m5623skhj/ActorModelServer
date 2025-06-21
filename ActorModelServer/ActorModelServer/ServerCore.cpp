#include "PreCompile.h"
#include "ServerCore.h"
#include <ranges>

static constexpr IOCompletionKeyType iocpCloseKey(0xffffffffffffffff, -1);
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
		std::cout << "OptionParsing() failed" << std::endl;
		return false;
	}

	if (not InitNetwork())
	{
		std::cout << "InitNetwork() failed" << std::endl;
		return false;
	}

	if (not SetSessionFactory(std::move(factoryFunc)))
	{
		std::cout << "SessionFactoryFunc cannot be nullptr" << std::endl;
		return false;
	}

	iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, numOfUsingWorkerThread);
	if (iocpHandle == INVALID_HANDLE_VALUE)
	{
		std::cout << "CreateIoCompletionPort() failed with " << GetLastError() << std::endl;
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
		PostQueuedCompletionStatus(iocpHandle, 0, (ULONG_PTR)(&iocpCloseKey), nullptr);
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
	if (!parser.GetValue_Byte(pBuff, L"SERVER", L"USE_IOCPWORKER", &numOfUsingWorkerThread))
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
		std::cout << "WSAStartup() failed with " << GetLastError() << std::endl;
		return false;
	}

	listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
	{
		std::cout << "socket() failed with " << GetLastError() << std::endl;
		return false;
	}

	SOCKADDR_IN serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(port);
	if (bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		std::cout << "bind() failed with " << GetLastError() << std::endl;
		return false;
	}

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		std::cout << "listen() failed with " << GetLastError() << std::endl;
		return false;
	}

	return true;
}

bool ServerCore::InitThreads()
{
	acceptThread = std::thread([this]() { RunAcceptThread(); });
	logicThreadEventStopHandle = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	packetAssembleStopEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	for (ThreadIdType i = 0; i < numOfWorkerThread; ++i)
	{
		ioThreads.emplace_back([this, i]() { this->RunIOThread(); });
	}
	for (ThreadIdType i = 0; i < numOfLogicThread; ++i)
	{
		sessionMapMutex.emplace_back(std::make_unique<std::shared_mutex>());
		nonNetworkActorMapMutex.emplace_back(std::make_unique<std::shared_mutex>());
		sessionMap.emplace_back();
		nonNetworkActorMap.emplace_back();
		releaseThreadsEventHandles.emplace_back(CreateEvent(nullptr, FALSE, FALSE, nullptr));
		releaseThreads.emplace_back([this, i]() { this->RunReleaseThread(i); });
		packetAssembleThreadEvents.emplace_back(CreateEvent(nullptr, FALSE, FALSE, nullptr));
		logicThreads.emplace_back([this, i]() { this->RunLogicThread(i); });
	}

	return true;
}

void ServerCore::RunAcceptThread()
{
	SOCKET clientSock{};
	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(clientAddr);

	while (isStop)
	{
		clientSock = accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSock == INVALID_SOCKET)
		{
			if (const int error = GetLastError(); error == WSAEINTR)
			{
				break;
			}
			else
			{
				std::cout << "accept failed with " << error << " in RunAcceptThread()" << std::endl;
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
		if (CreateIoCompletionPort((HANDLE)clientSock, iocpHandle, (ULONG_PTR)(ioCompletionKey), 0) != INVALID_HANDLE_VALUE)
		{
			newSession->DoRecv();
		}
		newSession->DecreaseIOCount();
	}
}

void ServerCore::RunIOThread()
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

		if (GetQueuedCompletionStatus(iocpHandle, &transferred, (PULONG_PTR)(&ioCompletionKey), &overlapped, INFINITE) == false)
		{
			std::cout << "GQCS failed with " << GetLastError() << std::endl;
			continue;
		}

		if (overlapped == nullptr)
		{
			std::cout << "GQCS success, but overlapped is NULL" << std::endl;
			break;
		}

		if (ioCompletionKey == nullptr)
		{
			std::cout << "GQCS success, but ioCompletionKey is nullptr" << std::endl;
			continue;
		}

		if (*ioCompletionKey == iocpCloseKey)
		{
			PostQueuedCompletionStatus(iocpHandle, 0, (ULONG_PTR)(&iocpCloseKey), nullptr);
			break;
		}

		ioCompletedSession = FindSession(ioCompletionKey->sessionId, ioCompletionKey->threadId);
		if (ioCompletedSession == nullptr)
		{
			std::cout << "GQCS success, but session is nullptr" << std::endl;
			break;
		}

		OnIOCompleted(*ioCompletedSession, overlapped, transferred);
	}
}

void ServerCore::RunLogicThread(const ThreadIdType threadId)
{
	static constexpr int SLEEP_TIME_MS = 33;
	const HANDLE eventHandle = { logicThreadEventStopHandle };
	while (not isStop)
	{
		switch (const auto waitResult = WaitForSingleObject(eventHandle, SLEEP_TIME_MS))
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
			std::cout << "Invalid wait result in RunLogicThread()" << std::endl;
			break;
		}
		}
	}
}

void ServerCore::RunReleaseThread(const ThreadIdType threadId)
{
	const HANDLE eventHandles[2] = { releaseThreadsEventHandles[threadId], logicThreadEventStopHandle };
	while (not isStop)
	{
		switch (const auto waitResult = WaitForMultipleObjects(2, eventHandles, FALSE, INFINITE))
		{
		case WAIT_OBJECT_0:
		{
			while (releaseThreadsQueue[threadId].GetRestSize() > 0)
			{
				ReleaseSessionKey releaseSessionKey;
				if (releaseThreadsQueue[threadId].Dequeue(&releaseSessionKey))
				{
					if (auto session = FindSession(releaseSessionKey.sessionId, threadId); session != nullptr)
					{
						EraseSession(releaseSessionKey.sessionId, threadId);
						session->OnDisconnected();
					}
				}
			}
		}
		break;
		case WAIT_OBJECT_0 + 1:
		{
			Sleep(releaseThreadStopSleepTime);
			EraseAllSession(threadId);
			break;
		}
		break;
		default:
		{
			std::cout << "Invalid wait result in RunReleaseThread()" << std::endl;
			break;
		}
		}
	}
}

bool ServerCore::OnIOCompleted(Session& ioCompletedSession, const LPOVERLAPPED& overlapped, const DWORD transferred)
{
	if (transferred == 0)
	{
		ioCompletedSession.DecreaseIOCount();
		return true;
	}

	if (&ioCompletedSession.recvIOData.overlapped == overlapped)
	{
		return OnRecvIOCompleted(ioCompletedSession, transferred);
	}
	else if (&ioCompletedSession.sendIOData.overlapped == overlapped)
	{
		return OnSendIOCompleted(ioCompletedSession);
	}

	return false;
}

bool ServerCore::OnRecvIOCompleted(Session& session, const DWORD transferred)
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

bool ServerCore::OnSendIOCompleted(Session& session)
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

bool ServerCore::SetSessionFactory(SessionFactoryFunc&& factoryFunc)
{
	if (factoryFunc == nullptr)
	{
		return false;
	}

	sessionFactory = std::move(factoryFunc);
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
		for (const auto& actor : nonNetworkActorMap[threadId] | std::views::values)
		{
			if (actor != nullptr)
			{
				actor->OnTimer();
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
	releaseThreadsQueue[threadId].Enqueue({ .sessionId= sessionId, .threadId= threadId});
	SetEvent(releaseThreadsEventHandles[threadId]);
}

bool ServerCore::RegisterNonNetworkActor(NonNetworkActor* actor, const ThreadIdType threadId)
{
	if (actor == nullptr)
	{
		return false;
	}

	std::unique_lock lock(*nonNetworkActorMapMutex[threadId]);
	nonNetworkActorMap[threadId].insert({ actor->GetActorId(), actor });

	return true;
}

bool ServerCore::UnregisterNonNetworkActor(const NonNetworkActor* actor, const ThreadIdType threadId)
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

void ServerCore::InsertSession(std::shared_ptr<Session>& session)
{
	std::unique_lock lock(*sessionMapMutex[session->GetThreadId()]);

	sessionMap[session->GetThreadId()].insert({ session->GetSessionId(), session });
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
}

void ServerCore::EraseSession(const SessionIdType sessionId, const ThreadIdType threadId)
{
	std::unique_lock lock(*sessionMapMutex[threadId]);

	sessionMap[threadId].erase(sessionId);
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
