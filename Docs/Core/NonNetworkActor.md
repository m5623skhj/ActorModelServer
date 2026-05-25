# NonNetworkActor

`NonNetworkActor`는 네트워크 소켓이 필요 없는 액터를 위한 기반 클래스입니다.

## 왜 중요한가

- 서버 내부 서비스 객체도 액터 모델로 통일할 수 있게 해 줍니다.
- 테스트 전용 액터와 일반 런타임 액터를 구분하는 진입점이 됩니다.
- `MediatorBase`의 직접 부모이기도 합니다.

## 관련 문서

- [[Core/Actor]]
- [[Core/ServerCore]]
- [[Core/MediatorAndTimer]]

## 핵심 역할

- `Actor`의 메시지 모델을 그대로 사용
- 네트워크 소켓 없이 로직 스레드에 등록
- 필요 시 테스트 전용 액터는 서버 등록 없이 로컬 테스트에서만 사용

## 중요한 함수

### `RegisterNonNetworkActor(const std::shared_ptr<NonNetworkActor>&)`

- `isTesterActor == false`일 때만 `ServerCore`에 등록합니다.
- 실제 저장은 `ServerCore::RegisterNonNetworkActor()`가 수행합니다.

### `UnregisterNonNetworkActor(const std::shared_ptr<NonNetworkActor>&)`

- 런타임 등록된 액터를 `ServerCore` 저장소에서 제거합니다.

### `IsTesterActor() const`

- 테스트 전용 액터 여부를 반환합니다.
- 테스트용 액터는 서버 저장소에 들어가지 않기 때문에, 실제 서버 쓰레드 없이도 검증 경로를 만들 수 있습니다.

## 기본 타이머 구현

현재 기본 구현은 아래처럼 비어 있습니다.

- `PreTimer()`
- `OnTimer()`
- `PostTimer()`

즉, 파생 클래스가 필요한 훅만 재정의하는 구조입니다.

## 대표 파생 클래스

- 중재자: [[Core/MediatorAndTimer]]

## 읽을 때 핵심

`NonNetworkActor`는 "세션이 아닌데도 액터처럼 동작하고 싶다"는 요구를 해결하는 클래스입니다.
