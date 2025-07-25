#include "PreCompile.h"
#include "SimpleClient.h"

SimpleClient& SimpleClient::GetInst()
{
	static SimpleClient instance;
	return instance;
}

bool SimpleClient::Start()
{
	return true;
}

void SimpleClient::Stop()
{
	needStop = true;
}

