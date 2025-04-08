#include "PreCompile.h"
#include "ServerCore.h"

static constexpr IOCompletionKeyType iocpCloseKey(0xffffffffffffffff, -1);
static CTLSMemoryPool<IOCompletionKeyType> ioCompletionKeyPool(dfNUM_OF_NETBUF_CHUNK, false);

ServerCore& ServerCore::GetInst()
{
	static ServerCore instance;
	return instance;
}

bool ServerCore::StartServer(const std::wstring& optionFilePath)
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

	// Need shutdown all connections

	for (BYTE i = 0; i < numOfWorkerThread; ++i)
	{
		PostQueuedCompletionStatus(iocpHandle, 0, (ULONG_PTR)(&iocpCloseKey), nullptr);
		ioThreads[i].join();
	}

	SetEvent(logicThreadEventStopHandle);
	for (BYTE i = 0; i < numOfLogicThread; ++i)
	{
		logicThreads[i].join();
		packetAssembleThreads[i].join();
		releaseThreads[i].join();
	}
	CloseHandle(iocpHandle);

	WSACleanup();
}

bool ServerCore::OptionParsing(const std::wstring optionFilePath)
{
	_wsetlocale(LC_ALL, L"Korean");

	CParser parser;
	WCHAR cBuffer[BUFFER_MAX];

	FILE* fp;
	_wfopen_s(&fp, optionFilePath.c_str(), L"rt, ccs=UNICODE");
	if (fp == NULL)
		return false;

	int iJumpBOM = ftell(fp);
	fseek(fp, 0, SEEK_END);
	int iFileSize = ftell(fp);
	fseek(fp, iJumpBOM, SEEK_SET);
	int FileSize = (int)fread_s(cBuffer, BUFFER_MAX, sizeof(WCHAR), BUFFER_MAX / 2, fp);
	int iAmend = iFileSize - FileSize;
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
	logicThreadEventStopHandle = CreateEvent(NULL, TRUE, FALSE, NULL);

	for (ThreadIdType i = 0; i < numOfWorkerThread; ++i)
	{
		ioThreads.emplace_back([this, i]() { this->RunIOThreads(); });
	}
	for (ThreadIdType i = 0; i < numOfLogicThread; ++i)
	{
		sessionMapMutex.emplace_back(std::make_unique<std::shared_mutex>());
		sessionMap.emplace_back();
		releaseThreadsEventHandles.emplace_back(CreateEvent(NULL, FALSE, FALSE, NULL));
		releaseThreads.emplace_back([this, i]() { this->RunReleaseThread(i); });
		packetAssembleThreadEvents.emplace_back(CreateEvent(NULL, FALSE, FALSE, NULL));
		packetAssembleThreads.emplace_back([this, i]() { this->RunPacketAssembleThread(i); });
		logicThreads.emplace_back([this, i]() { this->RunLogicThreads(i); });
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
			int error = GetLastError();
			if (error == WSAEINTR)
			{
				break;
			}
			else
			{
				std::cout << "accept failed with " << error << " in RunAcceptThread()" << std::endl;
				continue;
			}
		}

		const auto sessionId = ++sessionIdGenerator;
		auto newSession = std::make_shared<Session>(sessionId, clientSock, static_cast<ThreadIdType>(sessionId / numOfLogicThread));
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

void ServerCore::RunIOThreads()
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

		if (GetQueuedCompletionStatus(iocpHandle, &transferred, (PULONG_PTR)(&ioCompletionKey), &overlapped, INFINITE) == false)
		{
			std::cout << "GQCS failed with " << GetLastError() << std::endl;
			continue;
		}

		if (overlapped == NULL)
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

		if (transferred == 0)
		{
			ioCompletedSession->DecreaseIOCount();
			continue;
		}

		if (&ioCompletedSession->recvIOData.overlapped == overlapped)
		{
			OnRecvIOCompleted(*ioCompletedSession, transferred);
		}
		else if (&ioCompletedSession->sendIOData.overlapped == overlapped)
		{
			OnSendIOCompleted(*ioCompletedSession);
		}
	}
}

void ServerCore::RunLogicThreads(const ThreadIdType threadId)
{
	static constexpr int sleepTimeMs = 33;
	HANDLE eventHandle = { logicThreadEventStopHandle };
	while (not isStop)
	{
		const auto waitResult = WaitForSingleObject(eventHandle, sleepTimeMs);
		switch (waitResult)
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
			std::cout << "Invalid wait result in RunLogicThreads()" << std::endl;
			break;
		}
		}
	}
}

void ServerCore::RunPacketAssembleThread(const ThreadIdType threadId)
{
	HANDLE eventHandles[2] = { packetAssembleThreadEvents[threadId], logicThreadEventStopHandle };
	while (not isStop)
	{
		const auto waitResult = WaitForMultipleObjects(2, eventHandles, FALSE, INFINITE);
		switch (waitResult)
		{
		case WAIT_OBJECT_0:
		{
		}
		break;
		case WAIT_OBJECT_0 + 1:
		{
			// need wait?
			break;
		}
		break;
		default:
		{
			std::cout << "Invalid wait result in RunPacketAssembleThread()" << std::endl;
			break;
		}
		}
	}
}

void ServerCore::RunReleaseThread(const ThreadIdType threadId)
{
	HANDLE eventHandles[2] = { releaseThreadsEventHandles[threadId], logicThreadEventStopHandle };
	while (not isStop)
	{
		const auto waitResult = WaitForMultipleObjects(2, eventHandles, FALSE, INFINITE);
		switch (waitResult)
		{
		case WAIT_OBJECT_0:
		{
			while (releaseThreadsQueue[threadId].GetRestSize() > 0)
			{
				ReleaseSessionKey releaseSessionKey;
				if (releaseThreadsQueue[threadId].Dequeue(&releaseSessionKey))
				{
					auto session = FindSession(releaseSessionKey.sessionId, threadId);
					if (session != nullptr)
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

bool ServerCore::OnRecvIOCompleted(Session& session, const DWORD transferred)
{
	session.recvIOData.ringBuffer.MoveWritePos(transferred);
	int restSize = session.recvIOData.ringBuffer.GetUseSize();

	while (restSize > df_HEADER_SIZE)
	{
		NetBuffer& buffer = *NetBuffer::Alloc();
		if (RecvStreamToBuffer(session, buffer, restSize))
		{
			session.recvIOData.recvStoredQueue.Enqueue(&buffer);
		}
		else
		{
			session.DecreaseIOCount();
			NetBuffer::Free(&buffer);
			return false;
		}
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

bool ServerCore::RecvStreamToBuffer(Session& session, OUT NetBuffer& buffer, OUT int restSize)
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
	int dequeuedSize = session.recvIOData.ringBuffer.Dequeue(&buffer.m_pSerializeBuffer[buffer.m_iWrite], payloadLength);
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
	if (buffer.Decode() == false)
	{
		return false;
	}

	return true;
}

void ServerCore::PreWakeLogicThread(const ThreadIdType threadId)
{
	std::shared_lock lock(*sessionMapMutex[threadId]);
	for (auto& sessionMapPair : sessionMap[threadId])
	{
		if (sessionMapPair.second != nullptr)
		{
			{
				std::scoped_lock lock(sessionMapPair.second->queueMutex[sessionMapPair.second->storeQueueIndex]);
	
				sessionMapPair.second->usingQueueIndex ^= 1;
				sessionMapPair.second->storeQueueIndex ^= 1;
			}
	
			sessionMapPair.second->PreTimer();
		}
	}
}

void ServerCore::OnWakeLogicThread(const ThreadIdType threadId)
{
	std::shared_lock lock(*sessionMapMutex[threadId]);
	for (auto& sessionMapPair : sessionMap[threadId])
	{
		if (sessionMapPair.second != nullptr)
		{
			sessionMapPair.second->OnTimer();
		}
	}
}

void ServerCore::PostWakeLogicThread(const ThreadIdType threadId)
{
	std::shared_lock lock(*sessionMapMutex[threadId]);
	for (auto& sessionMapPair : sessionMap[threadId])
	{
		if (sessionMapPair.second != nullptr)
		{
			sessionMapPair.second->PostTimer();
		}
	}
}

void ServerCore::ReleaseSession(const SessionIdType sessionId, const ThreadIdType threadId)
{
	releaseThreadsQueue[threadId].Enqueue({ sessionId, threadId });
	SetEvent(releaseThreadsEventHandles[threadId]);
}

void ServerCore::InsertSession(std::shared_ptr<Session>& session)
{
	std::unique_lock lock(*sessionMapMutex[session->GetThreadId()]);

	sessionMap[session->GetThreadId()].insert({ session->GetSessionId(), session });
}

void ServerCore::EraseAllSession(const ThreadIdType threadId)
{
	std::unique_lock lock(*sessionMapMutex[threadId]);

	sessionMap[threadId].clear();
}

void ServerCore::EraseSession(const SessionIdType sessionId, const ThreadIdType threadId)
{
	std::unique_lock lock(*sessionMapMutex[threadId]);

	sessionMap[threadId].erase(sessionId);
}

std::shared_ptr<Session> ServerCore::FindSession(const SessionIdType sessionId, ThreadIdType threadId)
{
	std::shared_lock lock(*sessionMapMutex[threadId]);

	auto iter = sessionMap[threadId].find(sessionId);
	if (iter != sessionMap[threadId].end())
	{
		return iter->second;
	}

	return nullptr;
}
