#pragma once

class SimpleClient
{
private:
	SimpleClient() = default;
	~SimpleClient() = default;

public:
	static SimpleClient& GetInst();

public:
	bool Start(const std::wstring& optionFilePath);
	void Stop();

private:
	bool TryConnectToServer();
	bool ConnectToServer() const;

private:
	bool ReadOptionFile(const std::wstring& optionFilePath);

private:
	bool needStop{ false };

	SOCKET sessionSocket;
	WCHAR targetIp[16];
	WORD targetPort;
};