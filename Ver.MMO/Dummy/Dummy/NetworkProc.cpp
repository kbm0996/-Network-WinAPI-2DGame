#include "NetworkProc.h"
#include "Contents.h"
#include <map>
#include <conio.h>
#include "CSystemLog.h"

WCHAR g_szIP[16];
DWORD g_MaxClient;

std::map<SOCKET, st_SESSION*>		g_SessionMap;

DWORD		g_SessionID;

// Log Data
UINT64	g_ConnectTry;
UINT64	g_ConnectSuccess;
UINT64	g_ConnectFail;
UINT64	g_SendBytes;
UINT64	g_RecvBytes;
UINT64	g_PacketCnt;

DWORD	g_EchoCnt;
DWORD	g_RTTSum;
DWORD	g_RTTAvr;
DWORD	g_RTTMax;

// Server Controll
BOOL g_bShutdown = false;

bool NetworkInit()
{
	WSADATA		wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return false;
	

	DummyConnect();
	return true;
}

void NetworkClose()
{
	st_SESSION* pSession;
	for (auto iter = g_SessionMap.begin(); iter != g_SessionMap.end();)
	{
		pSession = (*iter).second;
		++iter;
		closesocket(pSession->Sock);
		delete pSession;
	}
	g_SessionMap.clear();

	st_CHARACTER* pCharacter;
	for (auto iter = g_CharacterMap.begin(); iter != g_CharacterMap.end();)
	{
		pCharacter = (*iter).second;
		++iter;
		delete pCharacter;
	}

	WSACleanup();
}

void NetworkProc()
{
	int		iSockCnt = 0;
	//-----------------------------------------------------
	// FD_SET은 FD_SETSIZE만큼씩만 소켓 검사 가능
	//-----------------------------------------------------
	FD_SET ReadSet;
	FD_SET WriteSet;
	SOCKET	UserTable_SOCKET[FD_SETSIZE] = { 0, };	// FD_SET에 등록된 소켓 저장

	FD_ZERO(&ReadSet);
	FD_ZERO(&WriteSet);
	memset(UserTable_SOCKET, INVALID_SOCKET, sizeof(SOCKET) *FD_SETSIZE);

	//-----------------------------------------------------
	// 모든 클라이언트에 대해 Socket 검사
	//-----------------------------------------------------
	for (auto iter = g_SessionMap.begin(); iter != g_SessionMap.end();)
	{
		st_SESSION * pSession = iter->second;
		++iter;	// SelectSocket 함수 내부에서 ClientMap을 삭제하는 경우가 있어서 미리 증가
				//-----------------------------------------------------
				// 해당 클라이언트 ReadSet 등록
				// SendQ에 데이터가 있다면 WriteSet 등록
				//-----------------------------------------------------
		UserTable_SOCKET[iSockCnt] = pSession->Sock;

		// ReadSet, WriteSet 등록
		FD_SET(pSession->Sock, &ReadSet);
		if (pSession->SendQ.GetUseSize() > 0)
			FD_SET(pSession->Sock, &WriteSet);

		++iSockCnt;
		//-----------------------------------------------------
		// select 최대치 도달, 만들어진 테이블 정보로 select 호출 후 정리
		//-----------------------------------------------------
		if (FD_SETSIZE <= iSockCnt)
		{
			SelectSocket(UserTable_SOCKET, &ReadSet, &WriteSet, iSockCnt);

			FD_ZERO(&ReadSet);
			FD_ZERO(&WriteSet);

			memset(UserTable_SOCKET, INVALID_SOCKET, sizeof(SOCKET) * FD_SETSIZE);
			//---------------------------------------------------------
			//  단일 스레드에 select 방식의 경우 소켓이 천 개 이상으로 많아지고
			// 서버의 로직도 부하가 걸리는 경우 접속자 츠리가 엄청 느려진다.
			//  따라서 64개씩 select 처리를 할 때 매번 accept 처리를 해주도록 한다.
			// 접속자 처리가 조금은 더 원활하게 진행된다.
			//---------------------------------------------------------
			iSockCnt = 0;
		}
	}
	//-----------------------------------------------------
	// 전체 클라이언트 for 문 종료 후 iSockCnt 수치가 있다면
	// 추가적으로 마지막 Select 호출을 해준다
	//-----------------------------------------------------
	if (iSockCnt > 0)
	{
		SelectSocket(UserTable_SOCKET, &ReadSet, &WriteSet, iSockCnt);
	}
}

// TODO: SelectSocket
bool SelectSocket(SOCKET* pTableSocket, FD_SET *pReadSet, FD_SET *pWriteSet, int iSockCnt)
{
	////////////////////////////////////////////////////////////////////
	// select() 함수의 디폴트 한계 값인 FD_SETSIZE(기본 64소켓)을 넘지 않았다면 
	// TimeOut 시간(Select 대기 시간)을 0으로 만들 필요가 없다.
	//
	// 그러나, FD_SETSIZE(기본 64소켓)을 넘었다면 Select를 두 번 이상 해야한다.
	// 이 경우 TimeOut 시간을 0으로 해야하는데 
	// 그 이유는 첫번째 select에서 블럭이 걸려 두번째 select가 반응하지 않기 떄문이다.
	////////////////////////////////////////////////////////////////////
	//-----------------------------------------------------
	// select 함수의 대기시간 설정
	//-----------------------------------------------------
	timeval Time;
	Time.tv_sec = 0;
	Time.tv_usec = 0;

	//-----------------------------------------------------
	// 접속자 요청과 현재 접속중인 클라이언트들의 메시지 송신 검사
	//-----------------------------------------------------
	//  select() 함수에서 block 상태에 있다가 요청이 들어오면,
	// select() 함수는 몇대에서 요청이 들어왔는지 감지를 하고 그 개수를 return합니다.
	// select() 함수가 return 되는 순간 flag를 든 소켓만 WriteSet, ReadSet에 남아있게 됩니다.
	// 그리고 각각 소켓 셋으로 부터 flag 표시를 한 소켓들을 checking하는 것입니다.
	int iResult = select(0, pReadSet, pWriteSet, NULL, &Time);
	if (iResult == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		LOG(L"SYSTEM", LOG_ERROR, L"select() ErrorCode: %d", err);
		return false;
	}
	else if (iResult > 0)
	{
		for (int iCnt = 0; iCnt < iSockCnt; ++iCnt)
		{
			if (pTableSocket[iCnt] == INVALID_SOCKET)
				continue;

			if (FD_ISSET(pTableSocket[iCnt], pWriteSet))
			{
				if (!SendEvent(pTableSocket[iCnt]))
				{
					DisconnectSession(pTableSocket[iCnt]);
					continue;
				}
			}

			if (FD_ISSET(pTableSocket[iCnt], pReadSet))
			{
				if (!RecvEvent(pTableSocket[iCnt]))
				{
					DisconnectSession(pTableSocket[iCnt]);
					continue;
				}
			}
		}
	}

	return true;
}

bool RecvEvent(SOCKET Sock)
{
	st_SESSION *pSession = FindSession(Sock);
	if (pSession == nullptr)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"FindSession() # not found session");
		return false;
	}

	WSABUF wsabuf[2];
	DWORD dwTransferred = 0;
	DWORD dwFlag = 0;
	int iBufCnt = 1;
	wsabuf[0].buf = pSession->RecvQ.GetWriteBufferPtr();
	wsabuf[0].len = pSession->RecvQ.GetUnbrokenEnqueueSize();
	if (wsabuf[0].len < pSession->RecvQ.GetFreeSize())
	{
		wsabuf[1].buf = pSession->RecvQ.GetBufferPtr();
		wsabuf[1].len = pSession->RecvQ.GetFreeSize() - wsabuf[0].len;
		++iBufCnt;
	}

	int iResult = WSARecv(pSession->Sock, wsabuf, iBufCnt, &dwTransferred, &dwFlag, NULL, NULL);
	if (iResult == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != 10054)
		{
			LOG(L"SYSTEM", LOG_ERROR, L"recv() SessionID: %d, ErrorCode: %d", pSession->dwSessionID, err);
		}
		return false;
	}

	pSession->RecvQ.MoveWritePos(dwTransferred);
	pSession->dwLastRecvTick = timeGetTime();
	g_RecvBytes += iResult;

	//-----------------------------------------------------
	// 패킷 확인
	// * 패킷이 하나 이상 버퍼에 있을 수 있으므로 반복문으로 한 번에 전부 처리해야함
	//-----------------------------------------------------
	while (1)
	{
		if (!RecvComplete(pSession))
			break;
	}

	return true;
}

bool SendEvent(SOCKET Sock)
{
	st_SESSION * pSession = FindSession(Sock);
	if (pSession == NULL)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"FindSession() # not found session");
		return false;
	}

	int iSendSize = pSession->SendQ.GetUseSize();
	if (iSendSize < sizeof(st_NETWORK_PACKET_HEADER))
		return true;

	while (iSendSize != 0)
	{
		int iResult = send(pSession->Sock, pSession->SendQ.GetReadBufferPtr(), pSession->SendQ.GetUnbrokenDequeueSize(), 0);
		if (iResult == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
			{
				return true;
			}
			if (err != 10054)
			{
				LOG(L"SYSTEM", LOG_ERROR, L"send() SessionID: %d, ErrorCode: %d", pSession->dwSessionID, err);
			}
			return false;
		}
		else if (iResult == 0)
		{
			LOG(L"SYSTEM", LOG_DEBUG, L"send() bytes 0 # SessionID: %d", pSession->dwSessionID);
		}

		if (iResult > iSendSize)
		{
			//-----------------------------------------------------
			// 보낼 사이즈보다 더 크다면 오류
			// 생기면 안되는 상황이지만 가끔 이런 경우가 생길 수 있다
			//-----------------------------------------------------
			return false;
		}

		//-----------------------------------------------------
		// Send Complate
		// 패킷 전송이 완료됐다는 의미는 아님. 소켓 버퍼에 복사를 완료했다는 의미
		// 송신 큐에서 Peek으로 빼냈던 데이터 제거
		//-----------------------------------------------------
		pSession->SendQ.MoveReadPos(iResult);
		iSendSize -= iResult;

		g_SendBytes += iResult;
	}
	return true;
}

void SendPacket_Unicast(st_SESSION * pSession, mylib::CSerialBuffer * pPacket)
{
	if (pSession == nullptr)
		return;
	pSession->SendQ.Enqueue((char*)pPacket->GetBufferPtr(), pPacket->GetUseSize());
}

st_SESSION * FindSession(SOCKET sock)
{
	std::map<SOCKET, st_SESSION*>::iterator iter = g_SessionMap.find(sock);
	if (iter == g_SessionMap.end())
		return nullptr;
	return iter->second;
}

st_SESSION * CreateSession(SOCKET Socket)
{
	st_SESSION* stpSession = new st_SESSION();
	stpSession->Sock = Socket;
	stpSession->RecvQ.Clear();
	stpSession->SendQ.Clear();
	stpSession->dwLastRecvTick = timeGetTime();
	stpSession->dwSessionID = ++g_SessionID;
	g_SessionMap.insert(std::pair<SOCKET, st_SESSION*>(Socket, stpSession));
	return stpSession;
}

bool DisconnectSession(SOCKET sock)
{
	st_SESSION*	pSession = FindSession(sock);
	if (pSession == nullptr)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"FindSession() # not found session");
		return false;
	}

	st_CHARACTER* pCharacter = FindCharacter(pSession->dwSessionID);
	if (pCharacter == nullptr)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"DisconnectSession() SessionID: %d Character Not Found", pSession->dwSessionID);
		return false;
	}

	LOG(L"SYSTEM", LOG_SYSTM, L"DisconnectSession() SessionID: %d", pSession->dwSessionID);

	g_SessionMap.erase(sock);
	g_CharacterMap.erase(pSession->dwSessionID);

	closesocket(pSession->Sock);
	delete pSession;
	delete pCharacter;

	return true;
}

bool OnRecv(st_SESSION * session, WORD wType, mylib::CSerialBuffer * Packet)
{
	switch (wType)
	{
	case dfPACKET_SC_CREATE_MY_CHARACTER:
		OnRecv_CreateMyCharacter(session, Packet);
		break;
	case dfPACKET_SC_CREATE_OTHER_CHARACTER:
		OnRecv_CreateOtherCharacter(session, Packet);
		break;
	case dfPACKET_SC_DELETE_CHARACTER:
		OnRecv_DeleteCharacter(session, Packet);
		break;
	case dfPACKET_SC_MOVE_START:
		OnRecv_Move_Start(session, Packet);
		break;
	case dfPACKET_SC_MOVE_STOP:
		OnRecv_Move_Stop(session, Packet);
		break;
	case dfPACKET_SC_ATTACK1:
		OnRecv_Attack1(session, Packet);
		break;
	case dfPACKET_SC_ATTACK2:
		OnRecv_Attack2(session, Packet);
		break;
	case dfPACKET_SC_ATTACK3:
		OnRecv_Attack3(session, Packet);
		break;
	case dfPACKET_SC_DAMAGE:
		OnRecv_Damage(session, Packet);
		break;
	case dfPACKET_SC_SYNC:
		OnRecv_Sync(session, Packet);
		break;
	case dfPACKET_SC_ECHO:
		OnRecv_Echo(session, Packet);
		break;
	default:
		LOG(L"SYSTEM", LOG_ERROR, L"Unknown Packet SessionID: %d, PacketType: %d", session->dwSessionID, wType);
		DisconnectSession(session->Sock);
		break;
	}
	
	++g_PacketCnt;

	return true;
}

bool OnRecv_CreateMyCharacter(st_SESSION * session, mylib::CSerialBuffer * pPacket)
{
	DWORD ID;
	BYTE Dir;
	SHORT shX, shY;
	BYTE HP;
	//---------------------------------------------------------------
	// 0 - 클라이언트 자신의 캐릭터 할당		Server -> Client
	//
	// 서버에 접속시 최초로 받게되는 패킷으로 자신이 할당받은 ID 와
	// 자신의 최초 위치, HP 를 받게 된다. (처음에 한번 받게 됨)
	// 
	// 이 패킷을 받으면 자신의 ID,X,Y,HP 를 저장하고 캐릭터를 생성시켜야 한다.
	//
	//	4	-	ID
	//	1	-	Direction
	//	2	-	X
	//	2	-	Y
	//	1	-	HP
	//
	//---------------------------------------------------------------
	*pPacket >> ID;
	*pPacket >> Dir;
	*pPacket >> shX;
	*pPacket >> shY;
	*pPacket >> HP;

	session->dwSessionID = ID;

	st_CHARACTER* pCharacter = new st_CHARACTER();

	pCharacter->pSession = session;
	pCharacter->dwSessionID = session->dwSessionID;
	pCharacter->byDirection = Dir;
	pCharacter->byMoveDirection = Dir;
	pCharacter->shX = shX;
	pCharacter->shY = shY;
	pCharacter->shX = shX;
	pCharacter->shY = shY;
	pCharacter->byHP = HP;

	pCharacter->dwAction = dfACTION_STAND;
	pCharacter->dwActionTick = timeGetTime();


	g_CharacterMap.insert(std::pair<DWORD, st_CHARACTER*>(session->dwSessionID, pCharacter));
	return true;
}

bool OnRecv_CreateOtherCharacter(st_SESSION * session, mylib::CSerialBuffer * pPacket)
{
	return false;
}

bool OnRecv_DeleteCharacter(st_SESSION * session, mylib::CSerialBuffer * pPacket)
{
	return false;
}

bool OnRecv_Move_Start(st_SESSION * session, mylib::CSerialBuffer * Packet)
{
	return false;
}

bool OnRecv_Move_Stop(st_SESSION * session, mylib::CSerialBuffer * Packet)
{
	return false;
}

bool OnRecv_Attack1(st_SESSION * session, mylib::CSerialBuffer * Packet)
{
	return false;
}

bool OnRecv_Attack2(st_SESSION * session, mylib::CSerialBuffer * Packet)
{
	return false;
}

bool OnRecv_Attack3(st_SESSION * session, mylib::CSerialBuffer * Packet)
{
	return false;
}

bool OnRecv_Damage(st_SESSION * session, mylib::CSerialBuffer * pPacket)
{
	return false;
}

bool OnRecv_Sync(st_SESSION * session, mylib::CSerialBuffer * pPacket)
{
	DWORD ID;
	WORD X, Y;
	//---------------------------------------------------------------
	// 동기화를 위한 패킷					Server -> Client
	//
	// 서버로부터 동기화 패킷을 받으면 해당 캐릭터를 찾아서
	// 캐릭터 좌표를 보정해준다.
	//
	//	4	-	ID
	//	2	-	X
	//	2	-	Y
	//
	//
	//  MMO의 경우 사용자가 많아졌을때 클라 뿐만 아니라 서버쪽도 느려질 수 있다. 
	// 그때를 대비해서 Sync 패킷을 보냄
	//
	// 1. 많이 틀어졌을 경우 보낼 수 있고-
	// 2. 수시로 보낼 수도 있다.
	//
	//  단, 2번의 경우 TCP가 아니라 UDP가 권장된다.
	// 왜냐하면 TCP를 사용하여 수시로 전송할 경우 Ack를 보내서 부하가 발생
	// UDP는 그런거 없이 무조건 온다.
	//
	//  참고로 마우스로 이동하는 게임일 경우에는 서버와 클라가 로직이 같으므로
	// 누군가 의도적으로 변경하지 않는 한 좌표가 틀어질 일이 없어서 이런 처리를 하지않아도 된다.
	//  키보드 게임의 경우 서버에게 이동에 관한 정보를 주기 전에 먼저 움직일 수 있으므로
	// 좌표가 틀어질 수 있다.
	//---------------------------------------------------------------
	*pPacket >> ID;
	*pPacket >> X;
	*pPacket >> Y;

	st_CHARACTER* pCharacter = FindCharacter(ID);
	if (pCharacter == nullptr)
		return false;

	pCharacter->shX = X;
	pCharacter->shY = Y;
	pCharacter->shX = X;
	pCharacter->shY = Y;

	LOG(L"SYSTEM", LOG_ERROR, L" # SYNC # ID : %d (%d, %d)", session->dwSessionID, X, Y);

	return true;
}

bool OnRecv_Echo(st_SESSION * session, mylib::CSerialBuffer * Packet)
{
	DWORD Time;
	//---------------------------------------------------------------
	// Echo 응답 패킷						Client -> Server
	//
	//	4	-	Time
	//
	//---------------------------------------------------------------
	*Packet >> Time;

	++g_EchoCnt;

	DWORD RTT = timeGetTime() - Time;
	g_RTTSum += RTT;
	if (g_RTTMax < RTT)
		g_RTTMax = RTT;

	return true;
}

void DummyConnect()
{
	int	err;
	SOCKADDR_IN serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(dfNETWORK_PORT);
	InetPton(AF_INET, g_szIP, &serveraddr.sin_addr);

	for (int i = 0; i < g_MaxClient; ++i)
	{
		++g_ConnectTry;
		g_ConnectSuccess = g_ConnectTry - g_ConnectFail;

		SOCKET Sock = socket(AF_INET, SOCK_STREAM, 0);
		if (Sock == INVALID_SOCKET)
		{
			err = WSAGetLastError();
			++g_ConnectFail;
			wprintf(L"ConnectFAIL:%lld\n", g_ConnectFail);
			wprintf(L"##### SOCKET ERROR [socket()] : Invalid Socket \n\n");
			continue;
		}

		BOOL bEnable = TRUE;
		setsockopt(Sock, IPPROTO_TCP, TCP_NODELAY, (char *)&bEnable, sizeof(bEnable));

		if (connect(Sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR)
		{
			err = WSAGetLastError();
			if (err != WSAEWOULDBLOCK)
			{
				++g_ConnectFail;
				wprintf(L"ConnectFAIL:%lld\n", g_ConnectFail);
				wprintf(L"##### SOCKET ERROR [connect()] : Connect Fail \n\n");
				continue;
			}
		}

		st_SESSION* stpSession = CreateSession(Sock);
	}
	wprintf(L"Connect		Try : %lld / Success : %lld / Fail : %lld \n\n", g_ConnectTry, g_ConnectSuccess, g_ConnectFail);
	return;
}

int RecvComplete(st_SESSION * pSession)
{
	//-----------------------------------------------------
	// 받은 내용 검사
	// 패킷헤더 크기 이상으로 뭔가 받았다면 스킵
	//-----------------------------------------------------
	int iRecvQSize = pSession->RecvQ.GetUseSize();
	if (iRecvQSize < sizeof(st_NETWORK_PACKET_HEADER)) // 처리할 패킷없음
	{
		return false;
	}

	//-----------------------------------------------------
	// I. PacketCode 검사
	//-----------------------------------------------------
	// Peek으로 검사를 하는 이유는 헤더를 얻은 후 사이즈 비교 후 하나의 완성된 패킷만큼의 데이터가 있는지 확인해
	// 패킷을 마저 얻을지 그냥 중단할지 결정
	// Get으로 얻는 경우는 검사 후 사이즈가 안맞다면 헤더를 다시 큐의 제자리에 넣어야 하는데 FIFO 이므로 어려움
	st_NETWORK_PACKET_HEADER stPacketHeader;
	pSession->RecvQ.Peek((char*)&stPacketHeader, sizeof(st_NETWORK_PACKET_HEADER));
	if (stPacketHeader.byCode != dfNETWORK_PACKET_CODE)
	{
		LOG(L"SYSTEM", LOG_ERROR, L" # RECV PACKET PROCESS ERROR # PacketCode Error ID : %d", pSession->dwSessionID);
		DisconnectSession(pSession->Sock);
		return false;
	}
	//-----------------------------------------------------
	// II. 큐에 저장된 데이터가 얻고자하는 패킷의 크기만큼 있는가
	//-----------------------------------------------------
	BYTE byEndcode;
	if ((WORD)iRecvQSize < stPacketHeader.bySize + sizeof(st_NETWORK_PACKET_HEADER) + sizeof(byEndcode))
	{
		return false;	// 더 이상 처리할 패킷이 없음
	}
	pSession->RecvQ.MoveReadPos(sizeof(st_NETWORK_PACKET_HEADER));


	mylib::CSerialBuffer pPacket;
	// 데이터가 터무니없이 크면 끊기
	if (stPacketHeader.bySize > pPacket.GetBufferSize())
	{
		LOG(L"SYSTEM", LOG_ERROR, L" # RECV PACKET PROCESS ERROR # Packet Size Error ID : %d", pSession->dwSessionID);
		DisconnectSession(pSession->Sock);
		return false;
	}

	// Payload 부분 패킷 버퍼로 복사
	if (stPacketHeader.bySize != pSession->RecvQ.Dequeue(pPacket.GetBufferPtr(), stPacketHeader.bySize))
	{
		return false;
	}
	if (!pSession->RecvQ.Dequeue((char*)&byEndcode, sizeof(byEndcode)))
	{
		LOG(L"SYSTEM", LOG_ERROR, L" # RECV PACKET PROCESS ERROR # Packet End Code Error ID : %d", pSession->dwSessionID);
		DisconnectSession(pSession->Sock);
		return false;
	}
	if (byEndcode != dfNETWORK_PACKET_END)
	{
		LOG(L"SYSTEM", LOG_ERROR, L" # RECV PACKET PROCESS ERROR # Packet End Code Error ID : %d", pSession->dwSessionID);
		DisconnectSession(pSession->Sock);
		return false;
	}
	pPacket.MoveWritePos(stPacketHeader.bySize);

	// 실질적인 패킷 처리 함수 호출
	if (!OnRecv(pSession, stPacketHeader.byType, &pPacket))
	{
		return false;
	}
	return true;
}
void mpMoveStart(mylib::CSerialBuffer * pPacket, BYTE Dir, WORD X, WORD Y)
{
	st_NETWORK_PACKET_HEADER stPacketHeader;
	stPacketHeader.byCode = dfNETWORK_PACKET_CODE;
	stPacketHeader.bySize = 5;
	stPacketHeader.byType = dfPACKET_CS_MOVE_START;

	//---------------------------------------------------------------
	// 10. 캐릭터 이동시작 패킷						Client -> Server
	//
	// 자신의 캐릭터 이동시작시 이 패킷을 보낸다.
	// 이동 중에는 본 패킷을 보내지 않으며, 키 입력이 변경되었을 경우에만
	// 보내줘야 한다.
	//
	// (왼쪽 이동중 위로 이동 / 왼쪽 이동중 왼쪽 위로 이동... 등등)
	//
	//	1	-	Direction	( 방향 디파인 값 8방향 사용 )
	//	2	-	X
	//	2	-	Y
	//
	//---------------------------------------------------------------

	pPacket->Clear();
	pPacket->Enqueue((char*)&stPacketHeader, sizeof(st_NETWORK_PACKET_HEADER));
	*pPacket << Dir;
	*pPacket << X;
	*pPacket << Y;
	*pPacket << (BYTE)dfNETWORK_PACKET_END;
}

void mpMoveStop(mylib::CSerialBuffer * pPacket, BYTE Dir, WORD X, WORD Y)
{
	st_NETWORK_PACKET_HEADER stPacketHeader;
	stPacketHeader.byCode = dfNETWORK_PACKET_CODE;
	stPacketHeader.bySize = 5;
	stPacketHeader.byType = dfPACKET_CS_MOVE_STOP;;

	//---------------------------------------------------------------
	// 캐릭터 이동중지 패킷						Client -> Server
	//
	// 이동중 키보드 입력이 없어서 정지되었을 때, 이 패킷을 서버에 보내준다.
	//
	//	1	-	Direction	( 방향 디파인 값 좌/우만 사용 )
	//	2	-	X
	//	2	-	Y
	//
	//---------------------------------------------------------------

	pPacket->Clear();
	pPacket->Enqueue((char*)&stPacketHeader, sizeof(st_NETWORK_PACKET_HEADER));
	*pPacket << Dir;
	*pPacket << X;
	*pPacket << Y;
	*pPacket << (BYTE)dfNETWORK_PACKET_END;
}

void mpAttack1(mylib::CSerialBuffer * pPacket, BYTE Dir, WORD X, WORD Y)
{
	st_NETWORK_PACKET_HEADER stPacketHeader;
	stPacketHeader.byCode = dfNETWORK_PACKET_CODE;
	stPacketHeader.bySize = 5;
	stPacketHeader.byType = dfPACKET_CS_ATTACK1;

	//---------------------------------------------------------------
	// 캐릭터 공격 패킷							Client -> Server
	//
	// 공격 키 입력시 본 패킷을 서버에게 보낸다.
	// 충돌 및 데미지에 대한 결과는 서버에서 알려 줄 것이다.
	//
	// 공격 동작 시작시 한번만 서버에게 보내줘야 한다.
	//
	//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
	//	2	-	X
	//	2	-	Y	
	//
	//---------------------------------------------------------------

	pPacket->Clear();
	pPacket->Enqueue((char*)&stPacketHeader, sizeof(st_NETWORK_PACKET_HEADER));
	*pPacket << Dir;
	*pPacket << X;
	*pPacket << Y;
	*pPacket << (BYTE)dfNETWORK_PACKET_END;
}

void mpAttack2(mylib::CSerialBuffer * pPacket, BYTE Dir, WORD X, WORD Y)
{
	st_NETWORK_PACKET_HEADER stPacketHeader;
	stPacketHeader.byCode = dfNETWORK_PACKET_CODE;
	stPacketHeader.bySize = 5;
	stPacketHeader.byType = dfPACKET_CS_ATTACK2;

	pPacket->Clear();
	pPacket->Enqueue((char*)&stPacketHeader, sizeof(st_NETWORK_PACKET_HEADER));
	*pPacket << Dir;
	*pPacket << X;
	*pPacket << Y;
	*pPacket << (BYTE)dfNETWORK_PACKET_END;
}

void mpAttack3(mylib::CSerialBuffer * pPacket, BYTE Dir, WORD X, WORD Y)
{
	st_NETWORK_PACKET_HEADER stPacketHeader;
	stPacketHeader.byCode = dfNETWORK_PACKET_CODE;
	stPacketHeader.bySize = 5;
	stPacketHeader.byType = dfPACKET_CS_ATTACK3;

	pPacket->Clear();
	pPacket->Enqueue((char*)&stPacketHeader, sizeof(st_NETWORK_PACKET_HEADER));
	*pPacket << Dir;
	*pPacket << X;
	*pPacket << Y;
	*pPacket << (BYTE)dfNETWORK_PACKET_END;
}

void mpEcho(mylib::CSerialBuffer * pPacket, DWORD Time)
{
	st_NETWORK_PACKET_HEADER stPacketHeader;
	stPacketHeader.byCode = dfNETWORK_PACKET_CODE;
	stPacketHeader.bySize = 4;
	stPacketHeader.byType = dfPACKET_CS_ECHO;

	pPacket->Clear();
	pPacket->Enqueue((char*)&stPacketHeader, sizeof(st_NETWORK_PACKET_HEADER));
	*pPacket << Time;
	*pPacket << (BYTE)dfNETWORK_PACKET_END;
}


/////////////////////////////////////////
// Server Control
/////////////////////////////////////////
void ServerControl()
{
	static bool bControlMode = false;

	//------------------------------------------
	// L : Control Lock / U : Unlock / Q : Quit
	//------------------------------------------
	//  _kbhit() 함수 자체가 느리기 때문에 사용자 혹은 더미가 많아지면 느려져서 실제 테스트시 주석처리 권장
	// 그런데도 GetAsyncKeyState를 안 쓴 이유는 창이 활성화되지 않아도 키를 인식함 Windowapi의 경우 
	// 제어가 가능하나 콘솔에선 어려움

	if (_kbhit())
	{
		WCHAR ControlKey = _getwch();

		if (L'u' == ControlKey || L'U' == ControlKey)
		{
			bControlMode = true;

			wprintf(L"[ Control Mode ] \n");
			wprintf(L"Press  L	- Key Lock \n");
			wprintf(L"Press  0~4	- Set Log Level \n");
			wprintf(L"Press  Q	- Quit \n");
		}

		if (bControlMode == true)
		{
			if (L'l' == ControlKey || L'L' == ControlKey)
			{
				wprintf(L"Controll Lock. Press U - Control Unlock \n");
				bControlMode = false;
			}

			if (L'q' == ControlKey || L'Q' == ControlKey)
			{
				g_bShutdown = true;
			}

			//------------------------------------------
			// Set Log Level
			//------------------------------------------
			if (L'0' == ControlKey)
			{
				LOG_SET(LOG_CONSOLE | LOG_FILE, LOG_DEBUG);
				wprintf(L"Set Log - DEBUG, WARNING, ERROR, SYSTEM \n");

			}
			if (L'1' == ControlKey)
			{
				LOG_SET(LOG_CONSOLE | LOG_FILE, LOG_WARNG);
				wprintf(L"Set Log - WARNING, ERROR, SYSTEM \n");
			}
			if (L'2' == ControlKey)
			{
				LOG_SET(LOG_CONSOLE | LOG_FILE, LOG_ERROR);
				wprintf(L"Set Log - ERROR, SYSTEM \n");
			}
			if (L'3' == ControlKey)
			{
				LOG_SET(LOG_CONSOLE | LOG_FILE, LOG_SYSTM);
				wprintf(L"Set Log - SYSTEM \n");
			}
		}
	}
}
