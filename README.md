 # ActorModelServer

## 제작 기간 : 2025.04.25 ~ 진행중

1. 개요
2. 주요 클래스
3. 패킷 등록

---

1. 개요

액터 모델 서버를 만들어보기 위한 프로젝트입니다.    
유저가 사용하는 모든 객체들은 액터를 상속 받아서 구현되어야 하며, 객체들간의 통신은 직접 하지 않고 SendMessage()를 통하여 메시지 방식으로만 통신해야 합니다.  

각 액터들은 일정 기간 마다 PreTimer(), OnTimer(), PostTimer()가 호출되며, 액터를 구현한 각 객체들은 이들을 통하여 컨트롤 됩니다.  
액터들은 다른 액터와 SendMessage()를 사용하여 통신할 때, 다른 스레드에 액터가 존재한다고 하더라도, 동기화를 신경쓰지 않아도 됩니다.  


---

2. 주요 클래스

* Actor 
	* 대부분의 클래스들이 상속 받아야 할 최상위 기본 클래스입니다. 액터로 선언되지 않는 클래스에 대해서는 유저가 직접 관리해야 합니다.
	* CreateActor()로 생성해야 정상적으로 액터가 등록되며, 다른 방법으로 액터를 생성할 시 정상 등록되지 않을 수 있습니다.
	* 각 액터들은 SendMessage()를 통해서만 다른 액터들과 통신해야 합니다.
	* PreTimer(), OnTimer(), PostTimer()를 구현할 수 있으며, 구현된 함수는 스레드에서 기술된 순서대로 호출해줍니다.

* Session
	* 네트워크를 통한 IO 진행이 필요한 클래스의 경우 이 클래스를 상속 받아서 구현해야 합니다.
	* OnConnected()와 OnDisconnected()를 구현해야 합니다.

* NonNetworkActor
	* 네트워크를 통한 IO가 필요 없을 경우 이 클래스를 상속 받아서 구현합니다.

* MediatorBase
	* 서로 다른 액터의 동작에 대해 트랜잭션이 필요할 경우, 사용하는 중재자 역할의 기본 클래스입니다.
	* 사용자는 OnTransaction~()와 Send~()를 구현하여 해당 클래스를 사용할 수 있습니다.
	* 해당 클래스도 액터로 취급되며, 필요시 Pre, On, PostTimer()를 구현할 수 있습니다.  

* ServerCore
	* ActorModelServer 라이브러리의 본체입니다.
	* StartServer()를 호출하면 아래의 스레드가 생성됩니다.
		* AcceptThread : 클라이언트의 접속을 받는 스레드입니다.
		* IOThread : 클라이언트로 부터의 IO 완료 처리를 담당 처리하는 스레드입니다.
		* LogicThread : IOThread로 부터 Recv 반응이 왔을 때 패킷을 핸들링하는 스레드입니다.
		* ReleaseThread : 통신이 중단된 세션들을 정리하는 스레드입니다.
	* StopServer()를 호출하면 생성된 스레드가 모두 정리될 때 까지 블락 상태로 대기하게 됩니다.

---

3. 패킷 등록

ContentsServer에서 예시를 확인할 수 있습니다.  

* 동봉된 Protocol.h 파일에서 IPacket을 상속 받아 패킷을 구현합니다.
	* 각 패킷은 매크로 함수인 GET_PACKET_ID(packetId)와 GET_PACKET_SIZE()를 프로토콜 파일에 기재해야 합니다.
* PacketHandlerRegister.cpp에서 구현한 것과 같이 Session을 상속 받은 클래스에서 RegisterPacketHandler(packetId, Handler)를 기재합니다.
* PacketHandler.cpp에서와 같이 핸들러를 구현합니다.

해당하는 패킷이 도착하였을 경우, 처리를 거쳐서 메시지 큐에 쌓이게 되고, 각 세션 구현체의 OnTimer()가 호출될 때, 도착한 패킷에 대한 핸들러가 호출됩니다.

---