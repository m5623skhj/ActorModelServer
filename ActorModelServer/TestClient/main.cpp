#include "PreCompile.h"
#include "Client.h"

int main()
{
	Sleep(1000);
	if (not Client::GetInst().StartClient(L"ClientOption.txt"))
	{
		std::cout << "Failed to start SimpleClient" << '\n';
		return -1;
	}

	std::string print = "StopClient : ESC\n";
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