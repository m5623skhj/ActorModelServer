#include "PreCompile.h"
#include "ServerCore.h"

const ULONG_PTR iocpCloseKey = 0xffffffffffffffff;

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
		PostQueuedCompletionStatus(iocpHandle, 0, iocpCloseKey, nullptr);
		ioThreads[i].join();
	}

	SetEvent(logicThreadEventStopHandle);
	for (BYTE i = 0; i < numOfLogicThread; ++i)
	{
		logicThreads[i].join();
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
		ioThreads.emplace_back([this, i]() { this->RunIOThreads(i); });
	}
	for (ThreadIdType i = 0; i < numOfLogicThread; ++i)
	{
		logicThreads.emplace_back([this, i]() { this->RunLogicThreads(i); });
		logicThreadEventHandles.emplace_back(CreateEvent(NULL, FALSE, FALSE, NULL));
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

		auto newSession = new Session(++sessionIdGenerator, clientSock);
		if (newSession == nullptr)
		{
			continue;
		}
		newSession->IncreaseIOCount();

		if (CreateIoCompletionPort((HANDLE)clientSock, iocpHandle, (ULONG_PTR)&newSession, 0) != INVALID_HANDLE_VALUE)
		{
			newSession->DoRecv();
		}
		newSession->DecreaseIOCount();
	}
}

void ServerCore::RunIOThreads(const ThreadIdType threadId)
{
	Session* ioCompletedSession{};
	LPOVERLAPPED overlapped{};
	DWORD transferred{};

	while (not isStop)
	{
		ioCompletedSession = nullptr;
		transferred = {};
		overlapped = {};

		if (GetQueuedCompletionStatus(iocpHandle, &transferred, (PULONG_PTR)&ioCompletedSession, &overlapped, INFINITE) == false)
		{
			std::cout << "GQCS failed with " << GetLastError() << std::endl;
			continue;
		}

		if (overlapped == NULL)
		{
			std::cout << "GQCS success, but overlapped is NULL" << std::endl;
			break;
		}

		if (ioCompletedSession == nullptr)
		{
			PostQueuedCompletionStatus(iocpHandle, 0, iocpCloseKey, nullptr);
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
	HANDLE eventHandles[2] = { logicThreadEventHandles[threadId], logicThreadEventStopHandle };
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
			Sleep(logicThreadStopSleepTime);
			break;
		}
		break;
		default:
		{
			std::cout << "Invalid wait result in RunLogicThreads()" << std::endl;
			break;
		}
		}
	}
}

void ServerCore::OnRecvIOCompleted(Session& session, const DWORD transferred)
{
	session.recvIOData.ringBuffer.MoveWritePos(transferred);
	int restSize = session.recvIOData.ringBuffer.GetUseSize();

	while (restSize > df_HEADER_SIZE)
	{
		NetBuffer& buffer = *NetBuffer::Alloc();
		if (RecvStreamToBuffer(session, buffer, restSize))
		{
			// SetEvent();
		}
		else
		{
			session.DecreaseIOCount();
			NetBuffer::Free(&buffer);
			break;
		}
	}
}

void ServerCore::OnSendIOCompleted(Session& session)
{

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
