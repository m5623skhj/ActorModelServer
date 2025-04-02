#include "PreCompile.h"
#include "Session.h"

Session::Session(SessionIdType inSessionIdType, const SOCKET& inSock)
	: sessionId(inSessionIdType)
	, sock(inSock)
	, ioCount(1)
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

void Session::DoRecv()
{

}

void Session::DoSend()
{

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