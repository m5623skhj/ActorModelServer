#include <iostream>
#include "ServerCore.h"

int main()
{
	if (not ServerCore::GetInst().StartServer(L""))
	{
		std::cout << "StartServer() failed" << std::endl;
		return 0;
	}

	while (ServerCore::GetInst().IsStop() == false)
	{
		if (GetAsyncKeyState('Q') & 0x8000)
		{
			ServerCore::GetInst().StopServer();
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	std::cout << "Server stopped" << std::endl;
	return 0;
}