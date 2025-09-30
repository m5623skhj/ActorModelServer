#include "PreCompile.h"
#include "Client.h"
#include "../ContentsServer/Protocol.h"

int main()
{
	Sleep(1000);
	if (not Client::GetInst().StartClient(L"ClientOption.txt"))
	{
		std::cout << "Failed to start SimpleClient" << '\n';
		return -1;
	}

	const std::string print = "StopClient : ESC\n";
	Ping ping;
	Client::GetInst().SendPacket(ping);

	while (true)
	{
		system("cls");
		if (GetAsyncKeyState(VK_ESCAPE) & VK_RETURN)
		{
			Client::GetInst().StopClient();
			break;
		}

		std::cout << print;
		Sleep(1000);
	}

	std::cout << "Client stopped" << '\n';
	return 0;
}
