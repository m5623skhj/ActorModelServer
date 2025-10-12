#pragma once
#include <functional>

using Message = std::function<void()>;
using ActorIdType = unsigned long long;
using ThreadIdType = unsigned char;
using SessionIdType = ActorIdType;

constexpr unsigned int RELEASE_THREAD_STOP_SLEEP_TIME = 10000;