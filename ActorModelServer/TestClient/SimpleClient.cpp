#include "PreCompile.h"
#include "SimpleClient.h"

#pragma comment(lib, "ws2_32.lib")

bool SimpleClient::Start(const std::wstring& optionFilePath)
{
	if (not ReadOptionFile(optionFilePath))
	{
		std::cout << "ReadOptionFile failed" << '\n';
		return false;
	}

	sendThreadEventHandle = CreateSemaphore(nullptr, 0, LONG_MAX, nullptr);
	CreateAllThreads();

	if (not TryConnectToServer())
	{
		std::cout << "TryConnectToServer failed" << '\n';
		return false;
	}

	return true;
}

void SimpleClient::Stop()
{
	WaitStopAllThreads();

	WSACleanup();
}

bool SimpleClient::TryConnectToServer()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cout << "WSAStartup failed with error: " << WSAGetLastError() << '\n';
		return false;
	}

	sessionSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sessionSocket == INVALID_SOCKET)
	{
		std::cout << "Socket creation failed with error: " << WSAGetLastError() << '\n';
		WSACleanup();
		return false;
	}

	if (not ConnectToServer())
	{
		std::cout << "Failed to connect to server" << '\n';
		closesocket(sessionSocket);
		WSACleanup();
		return false;
	}
	OnConnected();

	return true;
}

bool SimpleClient::ConnectToServer() const
{
	static constexpr int RECONNECT_TRY_COUNT = 5;

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(targetPort);
	InetPton(AF_INET, targetIp, &serverAddr.sin_addr);

	bool connected{ false };
	for (int i = 0; i < RECONNECT_TRY_COUNT; ++i)
	{
		if (connect(sessionSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
		{
			Sleep(1000);
			{
				continue;
			}
		}

		connected = true;
		break;
	}

	return connected;
}

void SimpleClient::CreateAllThreads()
{
	recvThread = std::jthread(&SimpleClient::RunRecvThread, this);
	sendThread = std::jthread(&SimpleClient::RunSendThread, this);
}

void SimpleClient::WaitStopAllThreads()
{
	needStop = true;
	ReleaseSemaphore(sendThreadEventHandle, 1, nullptr);

	if (recvThread.joinable())
	{
		recvThread.join();
	}
	if (sendThread.joinable())
	{
		sendThread.join();
	}

	CloseHandle(sendThreadEventHandle);
}

void SimpleClient::RunRecvThread()
{
	char recvBuffer[4096];
	while (not needStop)
	{
		DoRecv(recvBuffer);
	}
}

void SimpleClient::RunSendThread()
{
	while (not needStop)
	{
		WaitForSingleObject(sendThreadEventHandle, INFINITE);
		DoSend();
	}
}

bool SimpleClient::ReadOptionFile(const std::wstring& optionFilePath)
{
	_wsetlocale(LC_ALL, L"Korean");

	WCHAR cBuffer[BUFFER_MAX];

	FILE* fp;
	_wfopen_s(&fp, optionFilePath.c_str(), L"rt, ccs=UNICODE");

	const int iJumpBOM = ftell(fp);
	fseek(fp, 0, SEEK_END);
	const int iFileSize = ftell(fp);
	fseek(fp, iJumpBOM, SEEK_SET);
	const int fileSize = static_cast<int>(fread_s(cBuffer, BUFFER_MAX, sizeof(WCHAR), iFileSize / 2, fp));
	const int iAmend = iFileSize - fileSize;
	fclose(fp);

	cBuffer[iFileSize - iAmend] = '\0';
	WCHAR* pBuff = cBuffer;

	if (not g_Paser.GetValue_String(pBuff, L"SERVER_INFO", L"IP", targetIp))
	{
		return false;
	}
	if (not g_Paser.GetValue_Short(pBuff, L"SERVER_INFO", L"PORT", reinterpret_cast<short*>(&targetPort)))
	{
		return false;
	}
	if (not g_Paser.GetValue_Byte(pBuff, L"SERIALIZEBUF", L"PACKET_CODE", &NetBuffer::m_byHeaderCode))
	{
		return false;
	}
	if (not g_Paser.GetValue_Byte(pBuff, L"SERIALIZEBUF", L"PACKET_KEY", &NetBuffer::m_byXORCode))
	{
		return false;
	}

	return true;
}

NetBuffer* SimpleClient::GetRecvBuffer()
{
	std::scoped_lock lock(recvBufferDequeMutex);
	NetBuffer* buffer = recvBufferQueue.front();
	recvBufferQueue.pop();

	return buffer;
}

void SimpleClient::SendPacket(NetBuffer* packetBuffer)
{
	if (packetBuffer == nullptr)
	{
		return;
	}

	if (not packetBuffer->m_bIsEncoded)
	{
		packetBuffer->m_iWriteLast = packetBuffer->m_iWrite;
		packetBuffer->m_iWrite = 0;
		packetBuffer->m_iRead = 0;
		packetBuffer->Encode();
	}

	{
		std::scoped_lock lock(sendBufferDequeMutex);
		sendBufferQueue.push(packetBuffer);
	}
	ReleaseSemaphore(sendThreadEventHandle, 1, nullptr);
}

void SimpleClient::OnConnected()
{
	
}

void SimpleClient::DoRecv(char* recvBuffer)
{
	if (const int recvSize = recv(sessionSocket, recvBuffer, sizeof(recvBuffer), 0); recvSize > 0)
	{
		if (recvRingBuffer.GetFreeSize() < recvSize)
		{
			std::cout << "recvRingBuffer is full" << '\n';
			needStop = true;
			return;
		}

		if (const int restSize = recvRingBuffer.Enqueue(recvBuffer, recvSize); restSize < recvSize)
		{
			std::cout << "recvRingBuffer is full" << '\n';
		}

		if (not MakePacketsFromRingBuffer())
		{
			std::cout << "MakePacketsFromRingBuffer failed" << '\n';
			needStop = true;
		}
	}
	else if (recvSize == 0)
	{
		std::cout << "Connection closed by server" << '\n';
		needStop = true;
	}
	else
	{
		std::cout << "recv failed with error: " << WSAGetLastError() << '\n';
		needStop = true;
	}
}

bool SimpleClient::MakePacketsFromRingBuffer()
{
	int restSize = recvRingBuffer.GetUseSize();

	while (restSize > df_HEADER_SIZE)
	{
		NetBuffer& buffer = *NetBuffer::Alloc();
		recvRingBuffer.Peek(buffer.m_pSerializeBuffer, df_HEADER_SIZE);
		buffer.m_iRead = 0;

		WORD payloadLength;
		buffer >> payloadLength;
		if (restSize < payloadLength + df_HEADER_SIZE)
		{
			NetBuffer::Free(&buffer);
			if (payloadLength > dfDEFAULTSIZE)
			{
				return false;
			}

			break;
		}

		recvRingBuffer.RemoveData(df_HEADER_SIZE);
		const int dequeuedSize = recvRingBuffer.Dequeue(&buffer.m_pSerializeBuffer[buffer.m_iWrite], payloadLength);
		buffer.m_iWrite += dequeuedSize;
		if (PacketDecode(buffer) == false)
		{
			NetBuffer::Free(&buffer);
			return false;
		}

		restSize -= dequeuedSize + df_HEADER_SIZE;
		{
			std::scoped_lock lock(recvBufferDequeMutex);
			recvBufferQueue.push(&buffer);
		}
	}

	return true;
}

bool SimpleClient::PacketDecode(NetBuffer& buffer)
{
	if (buffer.Decode() == false)
	{
		return false;
	}
	
	return true;
}

void SimpleClient::DoSend()
{
	int restSize;
	{
		std::scoped_lock lock(sendBufferDequeMutex);
		restSize = static_cast<int>(sendBufferQueue.size());

	}

	if (restSize <= 0)
	{
		return;
	}

	while (restSize > 0)
	{
		NetBuffer* buffer;
		{
			std::scoped_lock lock(sendBufferDequeMutex);
			buffer = sendBufferQueue.front();
			sendBufferQueue.pop();
		}

		if (buffer == nullptr)
		{
			break;
		}

		buffer->m_iRead = 0;
		if (const int sendSize = send(sessionSocket, buffer->m_pSerializeBuffer, buffer->GetAllUseSize(), 0); sendSize == SOCKET_ERROR)
		{
			std::cout << "send failed with error: " << WSAGetLastError() << '\n';
			needStop = true;
			NetBuffer::Free(buffer);
			break;
		}
		NetBuffer::Free(buffer);
		--restSize;
	}
}
