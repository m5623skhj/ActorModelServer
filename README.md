# ActorModelServer

## 제작 기간 : 2025.04.25 ~ 진행중

1. 개요
2. 빌드 환경
3. 프로젝트 구성
4. 주요 클래스
5. 스레드 모델 및 액터 분배
6. 액터 생성과 메시지 송신
7. 패킷 등록
8. 타이머(Ticker / TimerEvent)
9. 트랜잭션(MediatorBase, 2PC)
10. 테스트 지원
11. 서버 옵션 파일

---

### 1. 개요

액터 모델 서버를 만들어보기 위한 프로젝트입니다.  
유저가 사용하는 모든 객체들은 액터를 상속 받아서 구현되어야 하며, 객체들간의 통신은 직접 하지 않고 `SendMessage()`를 통하여 메시지 방식으로만 통신해야 합니다.

각 액터들은 일정 기간 마다 `PreTimer()`, `OnTimer()`, `PostTimer()`가 호출되며, 액터를 구현한 각 객체들은 이들을 통하여 컨트롤 됩니다.  
액터들은 다른 액터와 `SendMessage()`를 사용하여 통신할 때, 다른 스레드에 액터가 존재한다고 하더라도, 동기화를 신경쓰지 않아도 됩니다.  
이는 각 액터가 자신을 담당하는 단일 로직 스레드 상에서만 메시지를 소비하기 때문이며, 송신 측은 큐에 메시지를 적재할 뿐 직접 수신 액터의 상태를 건드리지 않습니다.

---

### 2. 빌드 환경

* OS : Windows (IOCP / Winsock2 기반)
* C++ 표준 : C++20

---

### 3. 프로젝트 구성

솔루션(`ActorModelServer.sln`)은 다음 프로젝트들로 구성됩니다.

* **ActorModelServer** : 액터 모델 서버 코어 라이브러리 (정적 라이브러리).  
  `Actor`, `Session`, `NonNetworkActor`, `MediatorBase`, `ServerCore`, `Ticker`, `TimerEvent`, `PacketManager`, `TestSupport` 등 코어 구성 요소가 위치합니다.
* **ContentsServer** : 코어 라이브러리를 사용해 구현한 예제 서버.  
  `Player` 세션 구현, `TradeMediator` 트랜잭션 예제, `PacketHandlerRegister.cpp` / `PacketHandler.cpp` 패킷 등록 및 처리 예제가 들어있습니다.
* **TestClient** : 서버에 접속해 동작을 검증하기 위한 콘솔 클라이언트.
* **Logger** : JSON 직렬화 기반의 비동기 파일/콘솔 로거 라이브러리.
* **Common** : 서버/클라이언트가 공유하는 헤더 (`Protocol.h`, `ActorType.h`).

---

### 4. 주요 클래스

* **Actor**
  * 대부분의 클래스들이 상속 받아야 할 최상위 기본 클래스입니다. 액터로 선언되지 않는 클래스에 대해서는 유저가 직접 관리해야 합니다.
  * 액터 ID는 생성 시 자동 발급되며, `ServerCore::GetTargetThreadId(actorId)` 규칙으로 담당 로직 스레드가 결정됩니다.
  * 각 액터들은 `SendMessage()`를 통해서만 다른 액터들과 통신해야 합니다.
  * `PreTimer()`, `OnTimer()`, `PostTimer()` 를 구현할 수 있으며, 구현된 함수는 스레드에서 기술된 순서대로 호출해 줍니다.
  * `Stop()` 을 호출하면 더 이상 메시지를 받지 않으며, 큐에 적재된 메시지도 소비를 중단합니다.

* **Session**
  * 네트워크를 통한 IO 진행이 필요한 클래스의 경우 이 클래스를 상속 받아서 구현해야 합니다. `Actor` 의 파생 클래스입니다.
  * `OnConnected()` / `OnDisconnected()` 를 구현할 수 있습니다.
  * 패킷 송신은 `SendPacket(IPacket&)` 로 수행합니다. `Disconnect()` 호출 시 셧다운 후 IO Count 가 0 이 되는 시점에 Release 스레드가 정리합니다.

* **NonNetworkActor**
  * 네트워크를 통한 IO가 필요 없을 경우 이 클래스를 상속 받아서 구현합니다.
  * 등록은 `ActorCreator::Create<T>(...)` 호출 시 자동으로 수행됩니다 (내부적으로 `RegisterNonNetworkActor` 가 불립니다).

* **MediatorBase**
  * 서로 다른 액터의 동작에 대해 트랜잭션이 필요할 경우, 사용하는 중재자 역할의 기본 클래스입니다.
  * 2-Phase Commit(2PC) 형태로 동작합니다. 자세한 내용은 9. 트랜잭션 항목을 참고하세요.
  * `NonNetworkActor` 를 상속하므로, 액터로 취급되며 `Pre/On/PostTimer()` 를 구현할 수 있습니다. 기본 `OnTimer()` 는 트랜잭션 타임아웃을 감시합니다.

* **ServerCore**
  * ActorModelServer 라이브러리의 본체이며 싱글톤(`ServerCore::GetInst()`)입니다.
  * `StartServer(optionFilePath, sessionFactoryFunc)` 호출 시 옵션을 파싱하고 다음 스레드들이 생성됩니다.
    * **Accept 스레드 (1개)** : 클라이언트의 접속을 받아 세션을 생성합니다.
    * **IO 스레드 (옵션의 `WORKER_THREAD` 개)** : IOCP 완료 통지를 받아 수신/송신 완료 처리를 담당합니다.
    * **Logic 스레드 (옵션의 `LOGIC_THREAD` 개)** : 약 33ms 주기로 깨어나, 자신이 담당하는 액터들에 대해 `PreTimer → OnTimer → PostTimer` 를 호출합니다. 각 액터는 `OnTimer` 안에서 메시지 큐에 쌓인 메시지를 차례로 처리합니다.
    * **Release 스레드 (`LOGIC_THREAD` 개)** : 통신이 중단된 세션들을 정리합니다. Logic 스레드 인덱스와 1:1 매칭됩니다.
    * **Ticker 스레드 (1개)** : 전역 틱을 갱신하며, 등록된 `TimerEvent` 들을 시간에 맞춰 발화시킵니다 (현재 100ms 간격으로 시작).
  * `StopServer()` 를 호출하면 listen 소켓을 닫고, IOCP 종료 키를 broadcast 한 뒤, 모든 스레드가 정리될 때까지 블록 상태로 대기합니다.
  * 액터 조회용 `FindActor<T>(actorId, isNetworkActor)`, `FindSession`, `FindNonNetworkActor` 등을 제공합니다.

* **Ticker / TimerEvent**
  * `Ticker` 는 전역 싱글톤 틱 발생기입니다. `Start(intervalMs)` 로 동작을 시작합니다.
  * 사용자가 `TimerEvent` 를 상속해 `Fire()` 를 구현한 객체를 `TimerEventCreator::Create<T>(intervalMs, ...)` 로 생성하고, `Ticker::GetInstance().RegisterTimerEvent(...)` 로 등록하면 주기적으로 호출됩니다.

* **PacketManager**
  * 패킷 ID 에서 메시지 함수 객체를 조회하기 위한 싱글톤입니다. (현재는 보조적인 위치이며, 패킷 핸들러 등록은 `Actor::RegisterPacketHandler` 를 통해 세션 단위로 수행됩니다.)

* **Logger**
  * 별도의 정적 라이브러리로, `LOG_DEBUG(string)` / `LOG_ERROR(string)` 매크로를 통해 비동기 파일 로깅을 수행합니다.
  * 로그 객체는 JSON 으로 직렬화되어 `Log Folder/` 하위에 기록됩니다 (Release 빌드에서 `LOG_DEBUG` 는 비활성화됩니다).

---

### 5. 스레드 모델 및 액터 분배

* 액터의 담당 로직 스레드는 단순 모듈러 매핑으로 결정됩니다.

  ```cpp
  ThreadIdType ServerCore::GetTargetThreadId(ActorIdType actorId) const
  {
      return actorId % numOfLogicThread;
  }
  ```

* 따라서 한 액터에 대한 모든 `Pre/On/PostTimer` 및 메시지 소비는 항상 동일한 로직 스레드에서 실행됩니다. 액터 내부 상태에 대한 락이 필요 없는 이유가 여기에 있습니다.
* 송신 측에서 `SendMessage()` 를 호출하면, 수신 측 액터의 큐(`storeQueue`)에 mutex 보호 하에 메시지가 적재됩니다. 수신 측은 `ProcessMessage()` 안에서 `storeQueue` 와 `consumerQueue` 를 swap 하여 락 보유 시간을 최소화한 상태로 메시지를 소비합니다.

---

### 6. 액터 생성과 메시지 송신

* 액터 생성은 항상 `ActorCreator::Create<T>(args...)` 를 통해야 합니다. 다른 방식으로 액터를 생성할 시 정상 등록되지 않을 수 있습니다.

  ```cpp
  // NonNetworkActor 파생: ActorCreator::Create 가 자동으로 등록까지 수행합니다.
  auto npc = ActorCreator::Create<MyNpc>(/* ctor args... */);

  // Session 파생: StartServer 에 넘긴 factory 함수가 호출되어 자동 생성됩니다.
  ServerCore::GetInst().StartServer(L"ServerOption.txt",
      [](SOCKET sock) { return std::make_shared<Player>(sock); });
  ```

* `Actor::SendMessage` 는 세 가지 오버로드를 제공합니다. 모든 오버로드는 **메시지가 처리될 액터의 `storeQueue`** 에 적재되며, 그 액터의 로직 스레드에서만 실행됩니다.

  ```cpp
  // 1) (Func, Args...) — 호출한 액터(this) 의 큐에 적재.
  //    자기 자신에게 일을 미루거나, 자유 함수/람다를 deferred 실행할 때 사용합니다.
  me->SendMessage([]() { /* deferred work */ });

  // 2) (Func, sendTarget, Args...) — sendTarget 의 큐에 적재.
  //    멤버 함수 포인터를 sendTarget(this 슬롯) 과 args 로 바인딩한 뒤
  //    sendTarget 의 큐에 적재하므로, sendTarget 의 로직 스레드에서 실행되어 액터 격리가 보장됩니다.
  mediator->SendMessage(&Player::HandleTestMessage, target, value, requestPlayerId);

  // 3) (Message) — 미리 만들어둔 std::function<void()> 를 호출한 액터(this) 의 큐에 적재.
  me->SendMessage(std::move(message));
  ```

  반환값이 `bool` 인 점에 주의하세요. 적재 대상 액터가 `Stop()` 된 상태라면 `false` 가 반환되며 메시지는 적재되지 않습니다.

---

### 7. 패킷 등록

`ContentsServer` 에서 예시를 확인할 수 있습니다.

* `Common/Protocol.h` 에서 `IPacket` 을 상속 받아 패킷을 구현합니다.
  * 각 패킷은 매크로 함수인 `GET_PACKET_ID(packetId)` 와 `GET_PACKET_SIZE()` 를 프로토콜 파일에 기재해야 합니다.
* `Session` 을 상속한 클래스에서 `RegisterPacketHandler(packetId, &Derived::HandlerFunc)` 로 핸들러를 등록합니다.
* 핸들러 함수의 인자는 자동으로 역직렬화됩니다. 인자 타입은 primitive 또는 `std::string` 을 지원합니다.

예시 (`ContentsServer/Player`):

```cpp
// Player.cpp
Player::Player(const SOCKET& inSock) : Session(inSock)
{
    RegisterAllPacketHandler();
}

void Player::RegisterAllPacketHandler()
{
    RegisterPacketHandler(PACKET_ID::PING, &Player::HandlePing);
    // RegisterPacketHandler(PACKET_ID::SOME_PACKET, &Player::HandleSome); // 임의 인자 가능
}

// PacketHandler.cpp
void Player::HandlePing()
{
    Pong pong;
    SendPacket(pong);
}
```

해당하는 패킷이 도착하였을 경우, IO 스레드에서 디코드 후 핸들러 함수 객체(메시지)가 생성되고, 해당 세션의 메시지 큐에 적재됩니다. 이후 로직 스레드의 `OnTimer()` 가 호출될 때, 도착한 패킷에 대한 핸들러가 호출됩니다.

---

### 8. 타이머(Ticker / TimerEvent)

전역 주기 작업이 필요할 때 사용합니다. `Ticker` 는 `ServerCore::StartServer()` 내부에서 100ms 간격으로 시작됩니다.

```cpp
class MyTimer : public TimerEvent
{
public:
    // TimerEventCreator::Create 가 (id, intervalMs, args...) 순으로 전달합니다.
    MyTimer(TimerEventId id, TimerEventInterval intervalMs)
        : TimerEvent(id, intervalMs) {}

    void Fire() override { /* do work */ }
};

auto timer = TimerEventCreator::Create<MyTimer>(/*intervalMs=*/1000);
Ticker::GetInstance().RegisterTimerEvent(timer);
// 해제
Ticker::GetInstance().UnregisterTimerEvent(timer->GetTimerEventId());
```

* `Fire()` 는 `Ticker` 스레드에서 호출되므로, 액터 상태를 직접 만지지 말고 필요하면 대상 액터의 `SendMessage()` 로 위임하세요.

---

### 9. 트랜잭션(MediatorBase, 2PC)

`MediatorBase` 는 여러 액터에 걸친 작업을 2-Phase Commit 형태로 조정합니다.

* 흐름 : `StartTransaction(participants, timeout)` → `SendPrepareRequest(...)` (참여자에게 prepare 요청) → 참여자들이 `OnParticipantReady(transactionId, participantId, success)` 로 응답 → 모두 prepared 면 `CommitTransaction` (`SendCommitRequest`) → `OnTransactionCompleted`. 실패 또는 타임아웃 시 `RollbackTransaction` (`SendRollbackRequest`) → `OnTransactionFailed`.
* 타임아웃은 `MediatorBase::OnTimer()` 에서 감시되므로 Mediator 도 정상적으로 등록된 액터여야 합니다.
* 사용자는 다음 가상 함수를 구현하여 자신만의 트랜잭션 로직을 구현합니다.
  * `OnTransactionStarted`, `OnTransactionCompleted`, `OnTransactionFailed`
  * `SendPrepareRequest`, `SendCommitRequest`, `SendRollbackRequest`
* 예제는 `ContentsServer/MediatorImpl.h` 의 `TradeMediator` 를 참고하세요.

---

### 10. 테스트 지원

`BuildConfig.h` 의 `ENABLE_TEST_SUPPORT` 매크로가 1이면, 로직 스레드 없이도 액터를 구동시켜 단위 테스트할 수 있는 인터페이스가 활성화됩니다.

```cpp
using namespace CoreTestSupport;
TestInterface::RegisterSession(player.get());
TestInterface::SendPacketForTest(*player, pingPacket); // 핸들러 즉시 실행
TestInterface::Tick(); // PreTimer/OnTimer/PostTimer 1회 호출
TestInterface::TearDown();
```

* `Actor::PreTimerForTest()` / `OnTimerForTest()` / `PostTimerForTest()` 도 함께 제공됩니다.

---

### 11. 서버 옵션 파일

`ContentsServer/ServerOption.txt` 가 UNICODE(UTF-16) 텍스트 파일로 제공되며, 다음 키를 사용합니다.

```
:SERVER
{
    BIND_PORT          = 10001
    WORKER_THREAD      = 4   ; 생성할 IO 스레드 수
    USING_WORKER_THREAD= 4   ; IOCP concurrent thread 수 힌트
    LOGIC_THREAD       = 4   ; 로직 스레드 수 (릴리즈 스레드도 동일 수로 1:1 생성됨)
}

:SERIALIZEBUF
{
    PACKET_CODE = 119  ; NetBuffer 헤더 코드
    PACKET_KEY  = 50   ; NetBuffer XOR 키
}
```

* 파일 인코딩이 UTF-16 이어야 함에 유의해 주세요 (`_wfopen_s(... L"rt, ccs=UNICODE")` 로 읽습니다).

---
