#include "PreCompile.h"
#include "Client.h"

int main()
{
	if (not Client::GetInst().Start(L"TestClient/Option.txt"))
	{
		std::cout << "Failed to start SimpleClient" << '\n';
		return -1;
	}

	while (true)
	{
		system("cls");
		if (GetAsyncKeyState(VK_ESCAPE) & VK_RETURN)
		{
			Client::GetInst().Stop();
			break;
		}

		std::cout << "Stop : ESC" << '\n';
		Sleep(1000);
	}

	std::cout << "Client stopped" << '\n';
	return 0;
}