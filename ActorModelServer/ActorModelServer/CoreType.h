#pragma once
#include <functional>

using Message = std::function<void()>;
using ActorIdType = unsigned long long;
using ThreadIdType = unsigned char;
using SessionIdType = ActorIdType;

constexpr unsigned int releaseThreadStopSleepTime = 10000;
