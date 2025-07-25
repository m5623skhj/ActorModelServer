#pragma once

class SimpleClient
{
private:
	SimpleClient() = default;
	~SimpleClient() = default;

public:
	static SimpleClient& GetInst();

public:
	bool Start();
	void Stop();

private:
	bool needStop{ false };
};