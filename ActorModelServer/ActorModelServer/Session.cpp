#include "PreCompile.h"
#include "Session.h"

Session::Session(SessionIdType inSessionIdType, const SOCKET& inSock)
	: sessionId(inSessionIdType)
	, sock(inSock)
{
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