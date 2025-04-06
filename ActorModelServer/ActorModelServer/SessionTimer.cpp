#include "PreCompile.h"
#include "Session.h"

void Session::PreTimer()
{

}

void Session::OnTimer()
{
	ProcessMessage();
}

void Session::PostTimer()
{

}
