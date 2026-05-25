# Player

`Player`는 `ContentsServer`에서 사용하는 대표적인 `Session` 파생 클래스입니다.

## 왜 중요한가

- 코어 라이브러리를 실제 게임 세션 타입으로 어떻게 감싸는지 보여줍니다.
- 패킷 등록, 응답 전송, 중재자와의 연결 지점이 모두 이 클래스에 모입니다.

## 관련 문서

- [[ContentsServer/ContentsServer]]
- [[Core/Session]]
- [[Core/Actor]]
- [[Common/Protocol]]

## 역할

1. 소켓을 가진 플레이어 세션 표현
2. 패킷 핸들러 등록
3. Ping 요청 응답
4. 거래 준비용 메시지 수신 지점 제공
5. 플레이어 ID 저장

## 중요한 함수

### `Player(const SOCKET& inSock)`

- `Session` 생성자를 호출합니다.
- 바로 `RegisterAllPacketHandler()`를 호출해 패킷 처리표를 준비합니다.

### `RegisterAllPacketHandler()`

- 현재는 `PACKET_ID::PING`만 등록합니다.

```cpp
RegisterPacketHandler(PACKET_ID::PING, &Player::HandlePing);
```

즉, 예제 서버의 가장 기본적인 패킷 반응은 여기서 시작됩니다.

### `HandlePing()`

- `Pong` 패킷을 생성하고 `SendPacket(pong)`을 호출합니다.
- 현재 서버에서 실제로 완성된 최소 응답 기능입니다.

### `PrepareItemForTrade(int64_t)`

- 거래 준비용 메시지 수신 지점입니다.
- 현재 구현은 비어 있습니다.

### `PrepareMoneyForTrade(int64_t)`

- 마찬가지로 거래 준비용 확장 포인트입니다.
- 현재 구현은 비어 있습니다.

### `HandleTestMessage(int32_t, PlayerIdType)`

- 메시지 기반 상호작용 테스트를 위한 진입점입니다.
- 현재 코드베이스에서 구조를 보여주는 용도가 더 큽니다.

### `OnActorCreated()` / `OnActorDestroyed()`

- 세션 생성/제거 시점 훅입니다.
- 현재 구현은 비어 있습니다.

## 이 클래스가 보여주는 설계 포인트

- 패킷 수신은 `Player`가 직접 받는 것이 아니라 `Session`이 받은 뒤 메시지로 전달합니다.
- `Player`는 그 메시지가 실제 실행되는 위치입니다.
- 컨텐츠 로직이 코어 네트워크 루프와 분리되어 있다는 점이 중요합니다.

## 함께 읽기

- 서버 쪽 전체 맥락: [[ContentsServer/ContentsServer]]
- 패킷 흐름: [[Core/MessageFlow]]
- 세션 공통 동작: [[Core/Session]]
