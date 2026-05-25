# Diagrams

이 문서는 구조 이해에 도움이 되는 다이어그램만 모아 둔 인덱스입니다.

## 서버 스레드 모델

```mermaid
flowchart LR
    Accept["Accept Thread"] --> SessionCreate["Session 생성"]
    SessionCreate --> LogicMap["actorId % logicThread"]
    IO["IO Threads"] --> IOCP["IOCP 완료 처리"]
    IOCP --> Assemble["패킷 조립"]
    Assemble --> Queue["Actor storeQueue 적재"]
    Logic["Logic Threads"] --> Queue
    Logic --> ActorRun["PreTimer / OnTimer / PostTimer / 메시지 처리"]
    Release["Release Threads"] --> Cleanup["세션 정리"]
    Ticker["Ticker Thread"] --> Timer["TimerEvent Fire"]
```

## 액터 메시지 모델

```mermaid
flowchart TD
    Sender["Source Actor"] --> SendMsg["SendMessage(...)"]
    SendMsg --> Store["Target.storeQueue"]
    Store --> Swap["ProcessMessage에서 큐 스왑"]
    Swap --> Consume["consumerQueue 실행"]
    Consume --> Handler["멤버 함수 또는 람다 실행"]
```

## Ping/Pong 시퀀스

```mermaid
sequenceDiagram
    participant C as TestClient
    participant S as Session(Player)
    participant L as Logic Thread

    C->>S: PING
    S->>L: HandlePing 메시지 적재
    L->>S: HandlePing 실행
    S->>C: PONG
```

## 중재자 흐름

```mermaid
sequenceDiagram
    participant M as TradeMediator
    participant P1 as Participant A
    participant P2 as Participant B

    M->>M: StartTransaction
    M->>P1: Prepare 요청
    M->>P2: Prepare 요청
    alt 모두 준비 완료
        M->>P1: Commit 요청
        M->>P2: Commit 요청
    else 실패 또는 타임아웃
        M->>P1: Rollback 요청
        M->>P2: Rollback 요청
    end
```

## 문서 연결

- 코어 설명: [[Core/ActorModelCore]]
- 패킷 흐름: [[Core/MessageFlow]]
- 중재자 설명: [[Core/MediatorAndTimer]]
- 예제 서버: [[ContentsServer/ContentsServer]]
