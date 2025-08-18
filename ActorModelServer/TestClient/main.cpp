#include "PreCompile.h"
#include "SimpleClient.h"

int main()
{
	if (not SimpleClient::GetInst().Start(L"TestClient/Option.txt"))
	{
		std::cout << "Failed to start SimpleClient" << '\n';
		return -1;
	}

	while (true)
	{
		
	}

	return 0;
}