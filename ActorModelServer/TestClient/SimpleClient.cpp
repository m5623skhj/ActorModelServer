#include "PreCompile.h"
#include "SimpleClient.h"
#include "NetServerSerializeBuffer.h"

#pragma comment(lib, "ws2_32.lib")

SimpleClient& SimpleClient::GetInst()
{
	static SimpleClient instance;
	return instance;
}

bool SimpleClient::Start(const std::wstring& optionFilePath)
{
	if (not ReadOptionFile(optionFilePath))
	{
		std::cout << "ReadOptionFile failed" << '\n';
		return false;
	}

	if (not TryConnectToServer())
	{
		std::cout << "TryConnectToServer failed" << '\n';
		return false;
	}

	return true;
}

void SimpleClient::Stop()
{
	WSACleanup();
	needStop = true;
}

bool SimpleClient::TryConnectToServer()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cout << "WSAStartup failed with error: " << WSAGetLastError() << '\n';
		return false;
	}

	sessionSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sessionSocket == INVALID_SOCKET)
	{
		std::cout << "Socket creation failed with error: " << WSAGetLastError() << '\n';
		WSACleanup();
		return false;
	}

	if (not ConnectToServer())
	{
		std::cout << "Failed to connect to server" << '\n';
		closesocket(sessionSocket);
		WSACleanup();
		return false;
	}

	return true;
}

bool SimpleClient::ConnectToServer() const
{
	static constexpr int RECONNECT_TRY_COUNT = 5;

	sockaddr_in sessionGetterAddr;
	sessionGetterAddr.sin_family = AF_INET;
	sessionGetterAddr.sin_port = htons(targetPort);
	InetPton(AF_INET, targetIp, &sessionGetterAddr.sin_addr);

	bool connected{ false };
	for (int i = 0; i < RECONNECT_TRY_COUNT; ++i)
	{
		if (connect(sessionSocket, reinterpret_cast<sockaddr*>(&sessionGetterAddr), sizeof(sessionGetterAddr)) == SOCKET_ERROR)
		{
			Sleep(1000);
			{
				continue;
			}
		}

		connected = true;
		break;
	}

	return connected;
}

bool SimpleClient::ReadOptionFile(const std::wstring& optionFilePath)
{
	_wsetlocale(LC_ALL, L"Korean");

	WCHAR cBuffer[BUFFER_MAX];

	FILE* fp;
	_wfopen_s(&fp, optionFilePath.c_str(), L"rt, ccs=UNICODE");

	const int iJumpBOM = ftell(fp);
	fseek(fp, 0, SEEK_END);
	const int iFileSize = ftell(fp);
	fseek(fp, iJumpBOM, SEEK_SET);
	const int fileSize = static_cast<int>(fread_s(cBuffer, BUFFER_MAX, sizeof(WCHAR), iFileSize / 2, fp));
	const int iAmend = iFileSize - fileSize;
	fclose(fp);

	cBuffer[iFileSize - iAmend] = '\0';
	WCHAR* pBuff = cBuffer;

	if (not g_Paser.GetValue_String(pBuff, L"SERVER_INFO", L"IP", targetIp))
	{
		return false;
	}
	if (not g_Paser.GetValue_Short(pBuff, L"SERVER_INFO", L"PORT", reinterpret_cast<short*>(&targetPort)))
	{
		return false;
	}
	if (not g_Paser.GetValue_Byte(pBuff, L"SERIALIZEBUF", L"PACKET_CODE", &NetBuffer::m_byHeaderCode))
	{
		return false;
	}
	if (not g_Paser.GetValue_Byte(pBuff, L"SERIALIZEBUF", L"PACKET_KEY", &NetBuffer::m_byXORCode))
	{
		return false;
	}

	return true;
}
