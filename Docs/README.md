# ActorModelServer Docs

이 폴더는 `ActorModelServer` 솔루션을 위한 Obsidian 문서 Vault입니다.

## 문서 시작점

- [[00_Overview]]: 전체 구조와 권장 탐색 순서
- [[GettingStarted]]: 빌드 대상과 실행 흐름
- [[Core/ActorModelCore]]: 코어 라이브러리의 핵심 개념
- [[Core/ServerCore]]: 서버 생명주기와 스레드 모델
- [[Core/Actor]]: 액터 메시지 모델의 중심
- [[Core/Session]]: 네트워크 세션 액터
- [[Core/NonNetworkActor]]: 비네트워크 액터 기반
- [[Core/MessageFlow]]: 메시지와 패킷이 처리되는 흐름
- [[Core/MediatorAndTimer]]: 타이머와 중재자 트랜잭션
- [[ContentsServer/ContentsServer]]: 예제 서버 구현
- [[ContentsServer/Player]]: 예제 세션 구현체
- [[TestClient/TestClient]]: 테스트 클라이언트
- [[TestClient/Client]]: 테스트 클라이언트 핵심 클래스
- [[Common/Protocol]]: 공유 프로토콜
- [[Diagrams/README]]: 다이어그램 모음

## Obsidian 사용 메모

- 문서 간 연결은 `[[문서명]]` 문법을 사용합니다.
- 구조도는 대부분 `mermaid` 블록으로 작성되어 Obsidian에서 바로 확인할 수 있습니다.
- 저장소 구조를 먼저 보고 싶다면 [[00_Overview]]부터 읽는 편이 좋습니다.
