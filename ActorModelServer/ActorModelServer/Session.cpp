#include "PreCompile.h"
#include "Session.h"

Session::Session(const SessionIdType inSessionIdType, const SOCKET& inSock, const ThreadIdType inThreadId)
	: sessionId(inSessionIdType)
	, sock(inSock)
	, ioCount(1)
	, threadId(inThreadId)
{
	ZeroMemory(&recvIOData.overlapped, sizeof(OVERLAPPED));
	ZeroMemory(&sendIOData.overlapped, sizeof(OVERLAPPED));
}

void Session::OnConnected()
{
	isUsingSession = true;
}

void Session::OnDisconnected()
{
	isUsingSession = false;
}

void Session::OnRecv()
{

}

void Session::Disconnect()
{
	if (isUsingSession == false)
	{
		return;
	}

	isUsingSession = false;
	shutdown(sock, SD_BOTH);
	closesocket(sock);
}

bool Session::DoRecv()
{
	int brokenSize = recvIOData.ringBuffer.GetNotBrokenPutSize();
	int restSize = recvIOData.ringBuffer.GetFreeSize() - brokenSize;
	int bufferCount = 1;

	WSABUF buffer[2];
	buffer[0].buf = recvIOData.ringBuffer.GetWriteBufferPtr();
	buffer[0].len = brokenSize;
	if (restSize > 0)
	{
		buffer[1].buf = recvIOData.ringBuffer.GetBufferPtr();
		buffer[1].len = restSize;
		++bufferCount;
	}

	IncreaseIOCount();
	DWORD flag = 0;
	if (WSARecv(sock, buffer, bufferCount, nullptr, &flag, &recvIOData.overlapped, nullptr) == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			std::cout << "WSARecv() failed with " << WSAGetLastError() << std::endl;
			DecreaseIOCount();
			return false;
		}
	}

	return true;
}

bool Session::DoSend()
{
	IO_MODE expected = IO_MODE::IO_NONE_SENDING;
	while (true)
	{
		if (sendIOData.ioMode.compare_exchange_strong(expected, IO_MODE::IO_SENDING) == false)
		{
			break;
		}

		int restSize = sendIOData.sendQueue.GetRestSize();
		if (restSize == 0)
		{
			if (sendIOData.ioMode.compare_exchange_strong(expected, IO_MODE::IO_NONE_SENDING) == false)
			{
				continue;
			}
			break;
		}

		if (restSize < 0)
		{
			DecreaseIOCount();
			return false;
		}

		WSABUF buffer[ONE_SEND_WSABUF_MAX];
		if (ONE_SEND_WSABUF_MAX < restSize)
		{
			restSize = ONE_SEND_WSABUF_MAX;
		}

		for (int i = 0; i < restSize; ++i)
		{
			sendIOData.sendQueue.Dequeue(&sendIOData.sendBufferStore[i]);
			buffer[i].buf = sendIOData.sendBufferStore[i]->GetBufferPtr();
			buffer[i].len = sendIOData.sendBufferStore[i]->GetAllUseSize();
		}
		sendIOData.bufferCount += restSize;

		IncreaseIOCount();
		if (WSASend(sock, buffer, restSize, nullptr, 0, &sendIOData.overlapped, nullptr) == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				std::cout << "WSASend() failed with " << WSAGetLastError() << std::endl;
				DecreaseIOCount();
				return false;
			}
		}
		break;
	}

	return true;
}

void Session::IncreaseIOCount()
{
	++ioCount;
}

void Session::DecreaseIOCount()
{
	if (--ioCount == 0)
	{
		ReleaseSession();
	}
}

void Session::ReleaseSession()
{
	shutdown(sock, SD_BOTH);
	isUsingSession = false;

	OnDisconnected();
}