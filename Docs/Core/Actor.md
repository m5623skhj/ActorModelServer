# Actor

`Actor`는 이 솔루션의 가장 기본이 되는 실행 단위입니다.

## 왜 중요한가

- 모든 서버 측 로직 객체의 공통 조상입니다.
- 메시지 큐, 액터 ID, 목표 로직 스레드 개념이 이 클래스에서 시작됩니다.
- 패킷 핸들러 등록과 메시지 실행도 여기서 연결됩니다.

## 관련 문서

- [[Core/ServerCore]]
- [[Core/Session]]
- [[Core/MessageFlow]]
- [[ContentsServer/Player]]

## 핵심 개념

### 액터 ID와 스레드 귀속

- 생성자에서 액터 ID를 발급받습니다.
- 같은 시점에 `ServerCore::GetInst().GetTargetThreadId(actorId)`를 호출해 자기 로직 스레드를 결정합니다.
- 이 값은 액터 실행 위치를 고정하는 기준입니다.

### 이중 큐 구조

- `storeQueue`: 외부가 메시지를 넣는 큐
- `consumerQueue`: 현재 프레임에서 소비할 큐

`ProcessMessage()`는 잠깐 락을 잡고 두 큐를 교환한 뒤, `consumerQueue`를 끝까지 실행합니다.

## 중요한 함수

### `SendMessage(Func&&, Args&&...)`

- 현재 액터 자신에게 작업을 예약합니다.
- 호출 즉시 실행하는 것이 아니라, 다음 로직 루프에서 실행될 메시지를 큐에 넣습니다.
- `isStop == true`면 `false`를 반환합니다.

### `SendMessage(Func&&, std::shared_ptr<DerivedType> sendTarget, Args&&...)`

- 다른 액터에게 멤버 함수를 요청하는 오버로드입니다.
- 핵심은 "호출자는 다른 스레드에 있어도, 실행은 항상 `sendTarget`의 로직 스레드에서 일어난다"는 점입니다.

### `SendMessage(Message&&)` / `SendMessage(const Message&)`

- 이미 만들어 둔 `std::function<void()>`를 큐에 적재할 때 사용합니다.

### `CreateMessageFromPacket(NetBuffer&)`

- 버퍼에서 `PACKET_ID`를 먼저 읽습니다.
- 등록된 메시지 팩토리를 찾아 실행용 메시지 객체를 만듭니다.
- 핸들러가 없거나 버퍼 크기가 부족하면 `std::nullopt`를 반환합니다.

### `RegisterPacketHandler(PACKET_ID, void (DerivedType::*)(Args...))`

- 패킷 ID에 대응하는 핸들러를 등록합니다.
- 내부적으로는 "버퍼를 인자 목록으로 역직렬화한 뒤 멤버 함수를 호출하는 람다"를 `messageFactories`에 저장합니다.
- 같은 패킷 ID를 두 번 등록하면 예외를 던집니다.

### `ProcessMessage()`

- `storeQueue`와 `consumerQueue`를 교환합니다.
- `consumerQueue`를 하나씩 꺼내서 실행합니다.
- 널 메시지는 건너뜁니다.

### `Stop()`

- 액터를 정지 상태로 전환합니다.
- 이후 `SendMessage()`는 실패하게 됩니다.

## 패킷 핸들러 등록이 중요한 이유

이 설계 덕분에 IO 스레드는 패킷 내용을 "직접 해석해서 즉시 실행"하지 않고,  
액터가 자기 문맥에서 실행할 메시지로 바꿔 넣기만 합니다.

## 읽을 때 같이 볼 함수

- 패킷이 메시지로 바뀌는 과정: [[Core/MessageFlow]]
- 세션에서 `CreateMessageFromPacket()`을 실제로 쓰는 곳: [[Core/Session]]
- 예제 등록 코드: [[ContentsServer/Player]]
