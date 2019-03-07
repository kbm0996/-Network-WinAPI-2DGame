#include "NetworkProc.h"
#include "Contents.h"
#include <map>
#include <conio.h>
#include "CSystemLog.h"

SOCKET								g_ListenSocket = INVALID_SOCKET;
std::map<SOCKET, st_SESSION*>		g_SessionMap;

DWORD		g_SessionID;

// Server Controll
BOOL g_bShutdown = false;

bool netStartUp()
{
	int	err;
	LOG_SET(LOG_CONSOLE | LOG_FILE, LOG_DEBUG);

	WSADATA		wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		err = WSAGetLastError();
		LOG(L"SYSTEM", LOG_ERROR, L"WSAStartup() ErrorCode: %d", err);
		return false;
	}

	g_ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (g_ListenSocket == INVALID_SOCKET)
	{
		err = WSAGetLastError();
		LOG(L"SYSTEM", LOG_ERROR, L"socket() ErrorCode: %d", err);
		return false;
	}

	BOOL bOptval = TRUE;
	setsockopt(g_ListenSocket, IPPROTO_TCP, TCP_NODELAY, (char *)&bOptval, sizeof(bOptval));

	SOCKADDR_IN serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(dfNETWORK_PORT);
	InetPton(AF_INET, L"0.0.0.0", &serveraddr.sin_addr);
	if (bind(g_ListenSocket, (SOCKADDR *)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		LOG(L"SYSTEM", LOG_ERROR, L"bind() ErrorCode: %d", err);
		return false;
	}

	if (listen(g_ListenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		LOG(L"SYSTEM", LOG_ERROR, L"listen() ErrorCode: %d", err);
		return false;
	}

	LOG(L"SYSTEM", LOG_SYSTM, L"Server Start");
	return true;
}

void netCleanUp()
{
	closesocket(g_ListenSocket);
	WSACleanup();
}

void NetworkProcess()
{
	int		iSockCnt = 0;
	//-----------------------------------------------------
	// FD_SET은 FD_SETSIZE만큼씩만 소켓 검사 가능
	//-----------------------------------------------------
	FD_SET ReadSet;
	FD_SET WriteSet;
	SOCKET	UserTable_SOCKET[FD_SETSIZE] = { INVALID_SOCKET, };	// FD_SET에 등록된 소켓 저장

	FD_ZERO(&ReadSet);
	FD_ZERO(&WriteSet);
	memset(UserTable_SOCKET, 0, sizeof(SOCKET) *FD_SETSIZE);

	// Listen Socket Setting
	FD_SET(g_ListenSocket, &ReadSet);
	UserTable_SOCKET[iSockCnt] = g_ListenSocket;
	++iSockCnt;
	//-----------------------------------------------------
	// ListenSocket 및 모든 클라이언트에 대해 Socket 검사
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

			memset(UserTable_SOCKET, 0, sizeof(SOCKET) * FD_SETSIZE);
			//---------------------------------------------------------
			//  단일 스레드에 select 방식의 경우 소켓이 천 개 이상으로 많아지고
			// 서버의 로직도 부하가 걸리는 경우 접속자 츠리가 엄청 느려진다.
			//  따라서 64개씩 select 처리를 할 때 매번 accept 처리를 해주도록 한다.
			// 접속자 처리가 조금은 더 원활하게 진행된다.
			//---------------------------------------------------------
			FD_SET(g_ListenSocket, &ReadSet);
			UserTable_SOCKET[0] = g_ListenSocket;
			iSockCnt = 1;
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
		LOG(L"SYSTEM", LOG_ERROR, L"# select() #	ErrorCode: %d", err);
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
				if (!ProcSend(pTableSocket[iCnt]))
				{
					DisconnectSession(pTableSocket[iCnt]);
					continue;
				}
			}

			if (FD_ISSET(pTableSocket[iCnt], pReadSet))
			{
				//-----------------------------------------------------
				// ListenSocket은 접속자 수락 용도이므로 별도 처리 
				//-----------------------------------------------------
				if (pTableSocket[iCnt] == g_ListenSocket)
				{
					ProcAccept();
				}
				else
				{
					if (!ProcRecv(pTableSocket[iCnt]))
					{
						DisconnectSession(pTableSocket[iCnt]);
						continue;
					}
				}

			}
		}
	}

	return true;
}

bool ProcAccept()
{
	SOCKADDR_IN SessionAddr;
	SOCKET SessionSocket;
	int addrlen = sizeof(SessionAddr);
	SessionSocket = accept(g_ListenSocket, (SOCKADDR *)&SessionAddr, &addrlen);
	if (SessionSocket == INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		LOG(L"SYSTEM", LOG_ERROR, L"# accept() #	ErrorCode: %d", err);
		return false;
	}

	st_SESSION* stpSession = CreateSession(SessionSocket);

	WCHAR szSessionIP[16] = { 0 };
	DWORD dwAddrBufSize = sizeof(szSessionIP);
	WSAAddressToString((SOCKADDR*)&SessionAddr, sizeof(SOCKADDR), NULL, szSessionIP, &dwAddrBufSize);

	g_SessionMap.insert(std::pair<SOCKET, st_SESSION*>(stpSession->Sock, stpSession));
	CreateCharacter(stpSession);

	LOG(L"SYSTEM", LOG_SYSTM, L"Connect Session IP:%s / SessionID:%d", szSessionIP, g_SessionID);
	return true;
}

bool ProcRecv(SOCKET Sock)
{
	st_SESSION *pSession = FindSession(Sock);
	if (pSession == nullptr)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"# FindSession() #	not found session");
		return false;
	}

	// 마지막으로 받은 메시지 시간
	pSession->dwLastRecvTick = timeGetTime();

	// 만약 RecvQ에 Put할 사이즈가 없다면 자동적으로 종료처리로 들어가 끊어버린다.
	// 큐가 가득찬것. 이 뜻은 처리를 못하고있다는 것.
	if (pSession->RecvQ.GetFreeSize() == 0)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"# FULL RecvQ #	SessionID: %ld", pSession->dwSessionID);
		return true;
	}

	int iResult = recv(pSession->Sock, pSession->RecvQ.GetWriteBufferPtr(), pSession->RecvQ.GetUnbrokenEnqueueSize(), 0);
	if (iResult == SOCKET_ERROR)
	{
		// 받다가 에러 발생시 종료
		int err = WSAGetLastError();
		if (err != 10054)
		{
			LOG(L"SYSTEM", LOG_ERROR, L"# recv() #	SessionID: %d, ErrorCode: %d", pSession->dwSessionID, err);
		}
		return false;
	}
	else if (iResult == 0)
	{
		// 클라이언트 측에서 종료
		LOG(L"SYSTEM", LOG_DEBUG, L"# recv() #	bytes 0 SessionID: %d", pSession->dwSessionID);
		return false;
	}

	if (pSession->RecvQ.MoveWritePos(iResult) != iResult)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"# recv() #	RecvQ Enqueue Failed, SessionID: %d", pSession->dwSessionID);
		return false;
	}

	//-----------------------------------------------------
	// 패킷 확인
	// * 패킷이 하나 이상 버퍼에 있을 수 있으므로 반복문으로 한 번에 전부 처리해야함
	//-----------------------------------------------------
	while (1)
	{
		if (!CompleteRecvPacket(pSession))
			break;
	}

	return true;
}

bool ProcSend(SOCKET Sock)
{
	st_SESSION * pSession = FindSession(Sock);
	if (pSession == NULL)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"# FindSession() #	not found session");
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
				LOG(L"SYSTEM", LOG_ERROR, L"# send() #	SessionID: %d, ErrorCode: %d", pSession->dwSessionID, err);
			}
			return false;
		}
		else if (iResult == 0)
		{
			LOG(L"SYSTEM", LOG_DEBUG, L"# send() #	bytes 0, SessionID: %d", pSession->dwSessionID);
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
	}
	return true;
}

void SendPacket_Unicast(st_SESSION * pSession, mylib::CSerialBuffer * pPacket)
{
	if (pSession == nullptr)
		return;
	pSession->SendQ.Enqueue((char*)pPacket->GetBufferPtr(), pPacket->GetUseSize());
}

void SendPacket_SectorOne(int iSectorX, int iSectorY, mylib::CSerialBuffer * pPacket, st_SESSION * pExceptSession)
{
	if (iSectorX <= -1 || iSectorX > dfSECTOR_MAX_X || iSectorY <= -1 || iSectorY > dfSECTOR_MAX_Y)
		return;

	for (auto iter = g_Sector[iSectorY][iSectorX].begin(); iter != g_Sector[iSectorY][iSectorX].end(); ++iter)
	{
		if ((*iter)->pSession != pExceptSession)
			SendPacket_Unicast((*iter)->pSession, pPacket);
	}
}

void SendPacket_Around(st_SESSION * pSession, mylib::CSerialBuffer * pPacket, bool bSendMe)
{
	st_CHARACTER* pCharacter = FindCharacter(pSession->dwSessionID);
	if (pCharacter == nullptr)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"# SendPacket_Around() #	Character Not Found");
		return;
	}

	st_SECTOR_AROUND SectorAround;
	GetSectorAround(pCharacter->CurSector.iX, pCharacter->CurSector.iY, &SectorAround);

	if (bSendMe == true)
	{
		for (int iCnt = 0; iCnt < SectorAround.iCount; ++iCnt)
		{
			SendPacket_SectorOne(SectorAround.Around[iCnt].iX, SectorAround.Around[iCnt].iY,
				pPacket, NULL);
		}
	}
	else
	{
		for (int iCnt = 0; iCnt < SectorAround.iCount; ++iCnt)
		{
			SendPacket_SectorOne(SectorAround.Around[iCnt].iX, SectorAround.Around[iCnt].iY,
				pPacket, pSession);
		}
	}
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
	stpSession->dwSessionID = ++g_SessionID;
	return stpSession;
}

bool DisconnectSession(SOCKET sock)
{
	st_SESSION*	pSession = FindSession(sock);
	if (pSession == nullptr)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"# FindSession() #	not found session");
		return false;
	}

	LOG(L"SYSTEM", LOG_SYSTM, L"DisconnectSession() SessionID: %d", pSession->dwSessionID);

	DisconnectCharacter(pSession);
	g_SessionMap.erase(sock);
	closesocket(pSession->Sock);
	delete pSession;

	return true;
}

bool PacketProc(st_SESSION * session, WORD wType, mylib::CSerialBuffer * Packet)
{
	switch (wType)
	{
	case dfPACKET_CS_MOVE_START:
		PacketProc_Move_Start(session, Packet);
		break;
	case dfPACKET_CS_MOVE_STOP:
		PacketProc_Move_Stop(session, Packet);
		break;
	case dfPACKET_CS_ATTACK1:
		PacketProc_Attack1(session, Packet);
		break;
	case dfPACKET_CS_ATTACK2:
		PacketProc_Attack2(session, Packet);
		break;
	case dfPACKET_CS_ATTACK3:
		PacketProc_Attack3(session, Packet);
		break;
	case dfPACKET_CS_ECHO:
		PacketProc_Echo(session, Packet);
		break;
	default:
		LOG(L"SYSTEM", LOG_ERROR, L"Unknown Packet SessionID: %d, PacketType: %d", session->dwSessionID, wType);
		DisconnectSession(session->Sock);
		break;
	}
	return true;
}

bool PacketProc_Move_Start(st_SESSION * session, mylib::CSerialBuffer * Packet)
{
	BYTE byDir;
	SHORT shX, shY;

	*Packet >> byDir;
	*Packet >> shX;
	*Packet >> shY;

	// Character Search
	st_CHARACTER * pCharacter = FindCharacter(session->dwSessionID);
	if (pCharacter == nullptr)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"# PacketProc_Move_Start() #	Character Not Found");
		return false;
	}

	LOG(L"DEBUG", LOG_DEBUG, L"# MOVESTART #	SessionID:%d / Direction %d / X:%d / Y:%d / Sector[%d, %d] (%d, %d)", session->dwSessionID, byDir, shX, shY, pCharacter->CurSector.iX, pCharacter->CurSector.iY, shX / dfSECTOR_PIXEL_WIDTH, shY / dfSECTOR_PIXEL_HEIGHT);

	// Dead Reckoning
	if (abs(pCharacter->shX - shX) > dfERROR_RANGE || abs(pCharacter->shY - shY) > dfERROR_RANGE)
	{
		int idrX, idrY;
		int iDeadFrame = DeadReckoningPos(pCharacter->dwAction,
			pCharacter->dwActionTick,
			pCharacter->shX,
			pCharacter->shY,
			&idrX, &idrY);
		if (abs(idrX - shX) > dfERROR_RANGE || abs(idrY - shY) > dfERROR_RANGE)
		{
			LOG(L"DEBUG", LOG_DEBUG, L"# MOVESTART - SYNC #	SessionID:%d / Direction %d / X:%d / Y:%d / Sector[%d, %d] (%d, %d)", session->dwSessionID, byDir, shX, shY, pCharacter->CurSector.iX, pCharacter->CurSector.iY, shX / dfSECTOR_PIXEL_WIDTH, shY / dfSECTOR_PIXEL_HEIGHT);
			mpSync(Packet, pCharacter->dwSessionID, idrX, idrY);
			SendPacket_Around(pCharacter->pSession, Packet, true);
		}

		// 오차가 크지 않았다면 서버가 클라에 맞춤
		shX = idrX;
		shY = idrY;
	}

	// 동작 변경
	pCharacter->dwAction = byDir;
	// 2방향(LL,RR)과 8방향(LL,LU,UU,RU,RR,RD,DD,LD)이 있음 
	pCharacter->byMoveDirection = byDir;

	// 보는 방향 변경
	switch (byDir)
	{
	case dfPACKET_MOVE_DIR_RR:
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RD:
		pCharacter->byDirection = dfPACKET_MOVE_DIR_RR;
		break;
	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LU:
	case dfPACKET_MOVE_DIR_LD:
		pCharacter->byDirection = dfPACKET_MOVE_DIR_LL;
		break;
	}
	pCharacter->shX = shX;
	pCharacter->shY = shY;

	// 정지를 하면서 좌표가 약간 조절된 경우 섹터 고려
	if (Sector_UpdateCharacter(pCharacter))
	{
		// 섹터가 변경된 경우 클라에게 관련 정보 전송
		CharacterSectorUpdatePacket(pCharacter);
	}

	// 이동 액션이 변경되는 시점에서 데드레커닝을 위한 정보 저장
	pCharacter->dwActionTick = timeGetTime();

	mpMoveStart(Packet, session->dwSessionID, byDir, pCharacter->shX, pCharacter->shY);
	SendPacket_Around(session, Packet);
	return true;
}

bool PacketProc_Move_Stop(st_SESSION * session, mylib::CSerialBuffer * Packet)
{
	BYTE byDir;
	SHORT shX, shY;
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
	*Packet >> byDir;
	*Packet >> shX;
	*Packet >> shY;

	// Character Search
	st_CHARACTER * pCharacter = FindCharacter(session->dwSessionID);
	if (pCharacter == nullptr)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"# MOVESTOP #	SessionID:%d / Character Not Found", session->dwSessionID);
		return false;
	}

	LOG(L"DEBUG", LOG_DEBUG, L"# MOVESTOP #	SessionID:%d / Direction %d / X:%d / Y:%d / Sector[%d, %d] (%d, %d)", session->dwSessionID, byDir, shX, shY, pCharacter->CurSector.iX, pCharacter->CurSector.iY, shX / dfSECTOR_PIXEL_WIDTH, shY / dfSECTOR_PIXEL_HEIGHT);

	// Dead Reckoning
	if (abs(pCharacter->shX - shX) > dfERROR_RANGE || abs(pCharacter->shY - shY) > dfERROR_RANGE)
	{
		int idrX, idrY;
		int iDeadFrame = DeadReckoningPos(pCharacter->dwAction,
			pCharacter->dwActionTick,
			pCharacter->shX,
			pCharacter->shY,
			&idrX, &idrY);
		if (abs(idrX - shX) > dfERROR_RANGE || abs(idrY - shY) > dfERROR_RANGE)
		{
			LOG(L"DEBUG", LOG_DEBUG, L"# MOVESTOP - SYNC #	SessionID:%d / Direction %d / X:%d / Y:%d / Sector[%d, %d] (%d, %d)", session->dwSessionID, byDir, shX, shY, pCharacter->CurSector.iX, pCharacter->CurSector.iY, shX / dfSECTOR_PIXEL_WIDTH, shY / dfSECTOR_PIXEL_HEIGHT);

			mpSync(Packet, pCharacter->dwSessionID, idrX, idrY);
			SendPacket_Around(pCharacter->pSession, Packet, true);
		}

		// 오차가 크지 않았다면 서버가 클라에 맞춤
		shX = idrX;
		shY = idrY;
	}

	// 동작 변경
	pCharacter->dwAction = dfACTION_STAND;
	// 2방향(LL,RR)과 8방향(LL,LU,UU,RU,RR,RD,DD,LD)이 있음 
	pCharacter->byMoveDirection = byDir;

	// 보는 방향 변경
	switch (byDir)
	{
	case dfPACKET_MOVE_DIR_RR:
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RD:
		pCharacter->byDirection = dfPACKET_MOVE_DIR_RR;
		break;
	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LU:
	case dfPACKET_MOVE_DIR_LD:
		pCharacter->byDirection = dfPACKET_MOVE_DIR_LL;
		break;
	}
	pCharacter->shX = shX;
	pCharacter->shY = shY;

	// 정지를 하면서 좌표가 약간 조절된 경우 섹터 고려
	if (Sector_UpdateCharacter(pCharacter))
	{
		// 섹터가 변경된 경우 클라에게 관련 정보 전송
		CharacterSectorUpdatePacket(pCharacter);
	}

	// 이동 액션이 변경되는 시점에서 데드레커닝을 위한 정보 저장
	pCharacter->dwActionTick = timeGetTime();

	mpMoveStop(Packet, session->dwSessionID, byDir, pCharacter->shX, pCharacter->shY);
	SendPacket_Around(session, Packet);
	return true;
}

bool PacketProc_Attack1(st_SESSION * session, mylib::CSerialBuffer * Packet)
{
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
	BYTE byDir;
	SHORT shX, shY;

	*Packet >> byDir;
	*Packet >> shX;
	*Packet >> shY;

	st_CHARACTER* pCharacter = FindCharacter(session->dwSessionID);
	if (pCharacter == nullptr)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"# ATTACK1 #	SessionID:%d / Character Not Found", session->dwSessionID);
		return false;
	}

	LOG(L"DEBUG", LOG_DEBUG, L"# ATTACK1 #	SessionID:%d / Direction %d / X:%d / Y:%d / Sector[%d, %d] (%d, %d)", session->dwSessionID, byDir, shX, shY, pCharacter->CurSector.iX, pCharacter->CurSector.iY, shX / dfSECTOR_PIXEL_WIDTH, shY / dfSECTOR_PIXEL_HEIGHT);

	if (byDir == dfDIR_LEFT)
	{
		for (auto iter = g_CharacterMap.begin(); iter != g_CharacterMap.end(); ++iter)
		{
			if ((*iter).second->dwSessionID == session->dwSessionID)
				continue;

			if (abs((*iter).second->shY - shY) <= 30 && (shX - (*iter).second->shX) <= 85 && (shX - (*iter).second->shX) > 0)
			{
				(*iter).second->byHP -= 2;

				mpDamage(Packet, pCharacter->dwSessionID, (*iter).second->dwSessionID, (*iter).second->byHP);
				SendPacket_Around(session, Packet, true);
				break;
			}
		}
	}
	else if (byDir == dfDIR_RIGHT)
	{
		for (auto iter = g_CharacterMap.begin(); iter != g_CharacterMap.end(); ++iter)
		{
			if ((*iter).second->dwSessionID == session->dwSessionID)
				continue;

			if (abs((*iter).second->shY - shY) <= 30 && ((*iter).second->shX) - shX <= 85 && (*iter).second->shX - shX > 0)
			{
				(*iter).second->byHP -= 2;

				mpDamage(Packet, pCharacter->dwSessionID, (*iter).second->dwSessionID, (*iter).second->byHP);
				SendPacket_Around(session, Packet, true);
				break;
			}
		}
	}

	pCharacter->byDirection = byDir;
	pCharacter->dwAction = dfACTION_STAND;
	pCharacter->dwActionTick = timeGetTime();

	mpAttack1(Packet, pCharacter->dwSessionID, byDir, pCharacter->shX, pCharacter->shY);
	SendPacket_Around(session, Packet);

	return true;
}

bool PacketProc_Attack2(st_SESSION * session, mylib::CSerialBuffer * Packet)
{

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
	BYTE byDir;
	SHORT shX, shY;

	*Packet >> byDir;
	*Packet >> shX;
	*Packet >> shY;


	st_CHARACTER* pCharacter = FindCharacter(session->dwSessionID);
	if (pCharacter == nullptr)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"# ATTACK2 #	SessionID:%d / Character Not Found", session->dwSessionID);
		return false;
	}

	LOG(L"DEBUG", LOG_DEBUG, L"# ATTACK2 #	SessionID:%d / Direction %d / X:%d / Y:%d / Sector[%d, %d] (%d, %d)", session->dwSessionID, byDir, shX, shY, pCharacter->CurSector.iX, pCharacter->CurSector.iY, shX / dfSECTOR_PIXEL_WIDTH, shY / dfSECTOR_PIXEL_HEIGHT);

	if (byDir == dfDIR_LEFT)
	{
		for (auto iter = g_CharacterMap.begin(); iter != g_CharacterMap.end(); ++iter)
		{
			if ((*iter).second->dwSessionID == session->dwSessionID)
				continue;

			if (abs((*iter).second->shY - shY) <= 30 && (shX - (*iter).second->shX) <= 85 && (shX - (*iter).second->shX) > 0)
			{
				(*iter).second->byHP -= 2;

				mpDamage(Packet, pCharacter->dwSessionID, (*iter).second->dwSessionID, (*iter).second->byHP);
				SendPacket_Around(session, Packet, true);
				break;
			}
		}
	}
	else if (byDir == dfDIR_RIGHT)
	{
		for (auto iter = g_CharacterMap.begin(); iter != g_CharacterMap.end(); ++iter)
		{
			if ((*iter).second->dwSessionID == session->dwSessionID)
				continue;

			if (abs((*iter).second->shY - shY) <= 30 && ((*iter).second->shX) - shX <= 85 && (*iter).second->shX - shX > 0)
			{
				(*iter).second->byHP -= 2;

				mpDamage(Packet, pCharacter->dwSessionID, (*iter).second->dwSessionID, (*iter).second->byHP);
				SendPacket_Around(session, Packet, true);
				break;
			}
		}
	}

	pCharacter->byDirection = byDir;
	pCharacter->dwAction = dfACTION_STAND;
	pCharacter->dwActionTick = timeGetTime();

	mpAttack2(Packet, pCharacter->dwSessionID, byDir, pCharacter->shX, pCharacter->shY);
	SendPacket_Around(session, Packet);

	return true;
}

bool PacketProc_Attack3(st_SESSION * session, mylib::CSerialBuffer * Packet)
{
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
	BYTE byDir;
	SHORT shX, shY;

	*Packet >> byDir;
	*Packet >> shX;
	*Packet >> shY;

	st_CHARACTER* pCharacter = FindCharacter(session->dwSessionID);
	if (pCharacter == nullptr)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"# ATTACK3 #	SessionID:%d / Character Not Found", session->dwSessionID);
		return false;
	}

	LOG(L"DEBUG", LOG_DEBUG, L"# ATTACK3 #	SessionID:%d / Direction %d / X:%d / Y:%d / Sector[%d, %d] (%d, %d)", session->dwSessionID, byDir, shX, shY, pCharacter->CurSector.iX, pCharacter->CurSector.iY, shX / dfSECTOR_PIXEL_WIDTH, shY / dfSECTOR_PIXEL_HEIGHT);

	if (byDir == dfDIR_LEFT)
	{
		for (auto iter = g_CharacterMap.begin(); iter != g_CharacterMap.end(); ++iter)
		{
			if ((*iter).second->dwSessionID == session->dwSessionID)
				continue;

			if (abs((*iter).second->shY - shY) <= 30 && (shX - (*iter).second->shX) <= 85 && (shX - (*iter).second->shX) > 0)
			{
				(*iter).second->byHP -= 4;

				mpDamage(Packet, pCharacter->dwSessionID, (*iter).second->dwSessionID, (*iter).second->byHP);
				SendPacket_Around(session, Packet, true);
				break;
			}
		}
	}
	else if (byDir == dfDIR_RIGHT)
	{
		for (auto iter = g_CharacterMap.begin(); iter != g_CharacterMap.end(); ++iter)
		{
			if ((*iter).second->dwSessionID == session->dwSessionID)
				continue;

			if (abs((*iter).second->shY - shY) <= 30 && ((*iter).second->shX) - shX <= 85 && (*iter).second->shX - shX > 0)
			{
				(*iter).second->byHP -= 4;

				mpDamage(Packet, pCharacter->dwSessionID, (*iter).second->dwSessionID, (*iter).second->byHP);
				SendPacket_Around(session, Packet, true);
				break;
			}
		}
	}

	pCharacter->byDirection = byDir;
	pCharacter->dwAction = dfACTION_STAND;
	pCharacter->dwActionTick = timeGetTime();

	mpAttack3(Packet, pCharacter->dwSessionID, byDir, pCharacter->shX, pCharacter->shY);
	SendPacket_Around(session, Packet);

	return true;
}

bool PacketProc_Echo(st_SESSION * session, mylib::CSerialBuffer * Packet)
{
	//---------------------------------------------------------------
	// Echo 응답 패킷						Client -> Server
	//
	//	4	-	Time
	//
	//---------------------------------------------------------------
	DWORD Time;
	*Packet >> Time;

	mpEcho(Packet, Time);
	SendPacket_Unicast(session, Packet);
	return true;
}

int CompleteRecvPacket(st_SESSION * pSession)
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
		LOG(L"SYSTEM", LOG_ERROR, L" # RECV PACKET PROCESS ERROR #	PacketCode Error ID : %d", pSession->dwSessionID);
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
		LOG(L"SYSTEM", LOG_ERROR, L" # RECV PACKET PROCESS ERROR #	Packet Size Error ID : %d", pSession->dwSessionID);
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
		LOG(L"SYSTEM", LOG_ERROR, L" # RECV PACKET PROCESS ERROR #	Packet End Code Error ID : %d", pSession->dwSessionID);
		DisconnectSession(pSession->Sock);
		return false;
	}
	if (byEndcode != dfNETWORK_PACKET_END)
	{
		LOG(L"SYSTEM", LOG_ERROR, L" # RECV PACKET PROCESS ERROR #	Packet End Code Error ID : %d", pSession->dwSessionID);
		DisconnectSession(pSession->Sock);
		return false;
	}
	pPacket.MoveWritePos(stPacketHeader.bySize);

	// 실질적인 패킷 처리 함수 호출
	if (!PacketProc(pSession, stPacketHeader.byType, &pPacket))
	{
		return false;
	}
	return true;
}

void mpCreateMyCharacter(mylib::CSerialBuffer * pPacket, DWORD SessionID, BYTE Dir, WORD X, WORD Y, BYTE HP)
{
	st_NETWORK_PACKET_HEADER stPacketHeader;
	stPacketHeader.byCode = dfNETWORK_PACKET_CODE;
	stPacketHeader.bySize = 10;
	stPacketHeader.byType = dfPACKET_SC_CREATE_MY_CHARACTER;

	pPacket->Clear();
	pPacket->Enqueue((char*)&stPacketHeader, sizeof(st_NETWORK_PACKET_HEADER));
	*pPacket << SessionID;
	*pPacket << Dir;
	*pPacket << X;
	*pPacket << Y;
	*pPacket << HP;
	*pPacket << (BYTE)dfNETWORK_PACKET_END;
}

void mpCreateOtherCharacter(mylib::CSerialBuffer * pPacket, DWORD SessionID, BYTE Dir, WORD X, WORD Y, BYTE HP)
{
	st_NETWORK_PACKET_HEADER stPacketHeader;
	stPacketHeader.byCode = dfNETWORK_PACKET_CODE;
	stPacketHeader.bySize = 10;
	stPacketHeader.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;

	pPacket->Clear();
	pPacket->Enqueue((char*)&stPacketHeader, sizeof(st_NETWORK_PACKET_HEADER));
	*pPacket << SessionID;
	*pPacket << Dir;
	*pPacket << X;
	*pPacket << Y;
	*pPacket << HP;
	*pPacket << (BYTE)dfNETWORK_PACKET_END;
}

void mpDeleteCharacter(mylib::CSerialBuffer * pPacket, DWORD SessionID)
{
	st_NETWORK_PACKET_HEADER stPacketHeader;
	stPacketHeader.byCode = dfNETWORK_PACKET_CODE;
	stPacketHeader.bySize = 4;
	stPacketHeader.byType = dfPACKET_SC_DELETE_CHARACTER;

	pPacket->Clear();
	pPacket->Enqueue((char*)&stPacketHeader, sizeof(st_NETWORK_PACKET_HEADER));
	*pPacket << SessionID;
	*pPacket << (BYTE)dfNETWORK_PACKET_END;
}

void mpMoveStart(mylib::CSerialBuffer * pPacket, DWORD SessionID, BYTE Dir, WORD X, WORD Y)
{
	st_NETWORK_PACKET_HEADER stPacketHeader;
	stPacketHeader.byCode = dfNETWORK_PACKET_CODE;
	stPacketHeader.bySize = 9;
	stPacketHeader.byType = dfPACKET_SC_MOVE_START;

	pPacket->Clear();
	pPacket->Enqueue((char*)&stPacketHeader, sizeof(st_NETWORK_PACKET_HEADER));
	*pPacket << SessionID;
	*pPacket << Dir;
	*pPacket << X;
	*pPacket << Y;
	*pPacket << (BYTE)dfNETWORK_PACKET_END;
}

void mpMoveStop(mylib::CSerialBuffer * pPacket, DWORD SessionID, BYTE Dir, WORD X, WORD Y)
{
	st_NETWORK_PACKET_HEADER stPacketHeader;
	stPacketHeader.byCode = dfNETWORK_PACKET_CODE;
	stPacketHeader.bySize = 9;
	stPacketHeader.byType = dfPACKET_SC_MOVE_STOP;;

	pPacket->Clear();
	pPacket->Enqueue((char*)&stPacketHeader, sizeof(st_NETWORK_PACKET_HEADER));
	*pPacket << SessionID;
	*pPacket << Dir;
	*pPacket << X;
	*pPacket << Y;
	*pPacket << (BYTE)dfNETWORK_PACKET_END;
}

void mpAttack1(mylib::CSerialBuffer * pPacket, DWORD SessionID, BYTE Dir, WORD X, WORD Y)
{
	st_NETWORK_PACKET_HEADER stPacketHeader;
	stPacketHeader.byCode = dfNETWORK_PACKET_CODE;
	stPacketHeader.bySize = 9;
	stPacketHeader.byType = dfPACKET_SC_ATTACK1;

	pPacket->Clear();
	pPacket->Enqueue((char*)&stPacketHeader, sizeof(st_NETWORK_PACKET_HEADER));
	*pPacket << SessionID;
	*pPacket << Dir;
	*pPacket << X;
	*pPacket << Y;
	*pPacket << (BYTE)dfNETWORK_PACKET_END;
}

void mpAttack2(mylib::CSerialBuffer * pPacket, DWORD SessionID, BYTE Dir, WORD X, WORD Y)
{
	st_NETWORK_PACKET_HEADER stPacketHeader;
	stPacketHeader.byCode = dfNETWORK_PACKET_CODE;
	stPacketHeader.bySize = 9;
	stPacketHeader.byType = dfPACKET_SC_ATTACK2;

	pPacket->Clear();
	pPacket->Enqueue((char*)&stPacketHeader, sizeof(st_NETWORK_PACKET_HEADER));
	*pPacket << SessionID;
	*pPacket << Dir;
	*pPacket << X;
	*pPacket << Y;
	*pPacket << (BYTE)dfNETWORK_PACKET_END;
}

void mpAttack3(mylib::CSerialBuffer * pPacket, DWORD SessionID, BYTE Dir, WORD X, WORD Y)
{
	st_NETWORK_PACKET_HEADER stPacketHeader;
	stPacketHeader.byCode = dfNETWORK_PACKET_CODE;
	stPacketHeader.bySize = 9;
	stPacketHeader.byType = dfPACKET_SC_ATTACK3;

	pPacket->Clear();
	pPacket->Enqueue((char*)&stPacketHeader, sizeof(st_NETWORK_PACKET_HEADER));
	*pPacket << SessionID;
	*pPacket << Dir;
	*pPacket << X;
	*pPacket << Y;
	*pPacket << (BYTE)dfNETWORK_PACKET_END;
}

void mpDamage(mylib::CSerialBuffer * pPacket, DWORD AttackID, DWORD DamageID, BYTE DamageHP)
{
	st_NETWORK_PACKET_HEADER stPacketHeader;
	stPacketHeader.byCode = dfNETWORK_PACKET_CODE;
	stPacketHeader.bySize = 9;
	stPacketHeader.byType = dfPACKET_SC_DAMAGE;

	pPacket->Clear();
	pPacket->Enqueue((char*)&stPacketHeader, sizeof(st_NETWORK_PACKET_HEADER));
	*pPacket << AttackID;
	*pPacket << DamageID;
	*pPacket << DamageHP;
	*pPacket << (BYTE)dfNETWORK_PACKET_END;
}

void mpSync(mylib::CSerialBuffer * pPacket, DWORD SessionID, WORD X, WORD Y)
{
	st_NETWORK_PACKET_HEADER stPacketHeader;
	stPacketHeader.byCode = dfNETWORK_PACKET_CODE;
	stPacketHeader.bySize = 8;
	stPacketHeader.byType = dfPACKET_SC_SYNC;

	pPacket->Clear();
	pPacket->Enqueue((char*)&stPacketHeader, sizeof(st_NETWORK_PACKET_HEADER));
	*pPacket << SessionID;
	*pPacket << X;
	*pPacket << Y;
	*pPacket << (BYTE)dfNETWORK_PACKET_END;
}

void mpEcho(mylib::CSerialBuffer * pPacket, DWORD Time)
{
	st_NETWORK_PACKET_HEADER stPacketHeader;
	stPacketHeader.byCode = dfNETWORK_PACKET_CODE;
	stPacketHeader.bySize = 4;
	stPacketHeader.byType = dfPACKET_SC_ECHO;

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
