# Common Protocol

`Common` 프로젝트는 서버와 클라이언트가 공유하는 타입을 보관합니다.

## 포함 대상

- `Protocol.h`
- `ActorType.h`

## PACKET_ID

현재 정의된 패킷은 다음과 같습니다.

| ID | 의미 |
| --- | --- |
| `INVALID_PACKET` | 유효하지 않은 패킷 |
| `PING` | 클라이언트 상태 확인 요청 |
| `PONG` | `PING` 응답 |
| `APPEAR_ACTOR` | 액터 생성/출현 알림 |
| `DISAPPEAR_ACTOR` | 액터 제거/소멸 알림 |

## IPacket

모든 패킷은 `IPacket`을 상속합니다.

필수 인터페이스:

- `GetPacketId()`
- `GetPacketSize()`

## 매크로

### `GET_PACKET_ID(packetId)`

- 패킷 ID 반환 함수를 간단히 정의합니다.

### `GET_PACKET_SIZE()`

- 현재 객체 크기 기준으로 바디 크기를 계산합니다.
- 구현상 `sizeof(*this) - 8`을 사용합니다.

즉, 솔루션 전반은 "패킷 ID + 바디" 형태를 기본 전제로 둡니다.

## 예시 패킷

### Ping

- 바디 없음
- 연결 이후 서버 응답 확인용

### Pong

- 바디 없음
- `Ping` 응답

### AppearActor

- `actorId`
- `actorType`

이 패킷은 클라이언트가 월드에 등장한 액터를 추적하는 기능으로 확장될 수 있습니다.

### DisappearActor

- `PACKET_ID` enum 값은 존재합니다.
- 하지만 현재 `Common/Protocol.h`에는 이에 대응하는 구체 패킷 구조체가 정의되어 있지 않습니다.
- 반면 `TestClient` 쪽 분기 코드는 `actorId`를 읽는 형태로 준비되어 있습니다.

즉, 현재 저장소 기준으로 `DISAPPEAR_ACTOR`는 인터페이스는 열려 있지만 공용 패킷 정의는 아직 완성되지 않은 상태입니다.

## 패킷 직렬화 관점에서의 특징

- 송신 시 `Session::BuildPacketBuffer()`가 패킷 ID와 바디를 버퍼에 기록합니다.
- 이후 `NetBuffer::Encode()`가 적용됩니다.
- 수신 후에는 다시 `PACKET_ID`를 먼저 읽고 핸들러를 찾습니다.

## 프로토콜 문서를 보는 이유

새 기능을 추가할 때는 보통 아래 순서를 따르게 됩니다.

1. `PACKET_ID` 추가
2. 패킷 구조체 추가
3. 서버 액터 핸들러 등록
4. 클라이언트 분기 추가

즉, `Common`은 서버와 클라이언트 계약의 출발점입니다.
