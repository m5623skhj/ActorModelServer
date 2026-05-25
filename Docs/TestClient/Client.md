# Client

`Client`는 `TestClient` 프로젝트의 중심 클래스입니다.

## 왜 중요한가

- 서버와 실제로 패킷을 주고받는 검증 경로가 여기서 구현됩니다.
- 수신 패킷 분기, 반복 Ping/Pong, 클라이언트 측 액터 관리의 연결점이 됩니다.

## 관련 문서

- [[TestClient/TestClient]]
- [[Common/Protocol]]
- [[ContentsServer/Player]]
- [[Core/MessageFlow]]

## 역할

1. `SimpleClient` 위에 프로젝트 전용 로직 추가
2. 별도 로직 스레드로 수신 버퍼 소비
3. 패킷 ID에 따라 클라이언트 동작 분기
4. 월드 액터 생성/제거 알림 처리

## 중요한 함수

### `StartClient(const std::wstring& optionFilePath)`

- 내부 로직 스레드를 먼저 시작합니다.
- 이후 `SimpleClient::Start()` 경로를 통해 연결을 시도합니다.

### `StopClient()`

- `SimpleClient::Stop()`을 호출합니다.
- `logicThread`가 살아 있으면 join해서 마무리합니다.

### `SendPacket(IPacket& packet)`

- 패킷 ID와 바디를 `NetBuffer`에 기록한 뒤 `SimpleClient::SendPacket()`으로 넘깁니다.
- 테스트 클라이언트의 모든 패킷 송신 진입점입니다.

### `RunLogicThread()`

- 클라이언트 수신 버퍼를 33ms 간격으로 확인합니다.
- 받은 `NetBuffer`를 `ProcessLogic()`에 넘기고 사용 후 해제합니다.

### `ProcessLogic(NetBuffer& buffer)`

- 먼저 `PACKET_ID`를 읽습니다.
- 이후 다음 케이스로 분기합니다.
  - `PONG`
  - `APPEAR_ACTOR`
  - `DISAPPEAR_ACTOR`

이 함수는 서버측 `Actor::CreateMessageFromPacket()`과 달리, 클라이언트에서는 단순한 `switch` 기반 분기를 사용한다는 점이 차이입니다.

### `Pong(const NetBuffer&)`

- 현재 구현은 단순합니다.
- `PONG`를 받으면 다시 `Ping`을 보내 연결 상태를 계속 확인합니다.

### `AddActor(NetBuffer& packet)`

- 버퍼에서 `actorId`, `actorType`을 읽습니다.
- `ActorCreator::CreateActor()`로 클라이언트 액터를 만들고 `ActorManager`에 넣습니다.

### `RemoveActor(NetBuffer& packet)`

- 버퍼에서 `actorId`를 읽어 `ActorManager`에서 제거합니다.
- 다만 현재 공용 프로토콜에는 `DISAPPEAR_ACTOR`용 구체 패킷 구조체가 없어, 이 부분은 확장 중인 인터페이스로 봐야 합니다.

### `OnConnected()` / `OnDisconnected()`

- 현재 구현은 비어 있습니다.
- 필요하면 연결 상태 표시나 초기 동기화 트리거를 넣을 수 있는 자리입니다.

## 서버와의 대응 관계

- 서버 `Player::HandlePing()`이 `Pong`을 보내면
- 클라이언트 `Client::Pong()`이 다시 `Ping`을 보냅니다.

즉, 현재 기본 검증 시나리오는 두 클래스가 왕복 루프를 형성합니다.

## 함께 읽기

- 전체 테스트 흐름: [[TestClient/TestClient]]
- 서버 대응 구현: [[ContentsServer/Player]]
- 패킷 정의: [[Common/Protocol]]
