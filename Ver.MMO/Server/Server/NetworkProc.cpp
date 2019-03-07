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
	// FD_SET�� FD_SETSIZE��ŭ���� ���� �˻� ����
	//-----------------------------------------------------
	FD_SET ReadSet;
	FD_SET WriteSet;
	SOCKET	UserTable_SOCKET[FD_SETSIZE] = { INVALID_SOCKET, };	// FD_SET�� ��ϵ� ���� ����

	FD_ZERO(&ReadSet);
	FD_ZERO(&WriteSet);
	memset(UserTable_SOCKET, 0, sizeof(SOCKET) *FD_SETSIZE);

	// Listen Socket Setting
	FD_SET(g_ListenSocket, &ReadSet);
	UserTable_SOCKET[iSockCnt] = g_ListenSocket;
	++iSockCnt;
	//-----------------------------------------------------
	// ListenSocket �� ��� Ŭ���̾�Ʈ�� ���� Socket �˻�
	//-----------------------------------------------------
	for (auto iter = g_SessionMap.begin(); iter != g_SessionMap.end();)
	{
		st_SESSION * pSession = iter->second;
		++iter;	// SelectSocket �Լ� ���ο��� ClientMap�� �����ϴ� ��찡 �־ �̸� ����
				//-----------------------------------------------------
				// �ش� Ŭ���̾�Ʈ ReadSet ���
				// SendQ�� �����Ͱ� �ִٸ� WriteSet ���
				//-----------------------------------------------------
		UserTable_SOCKET[iSockCnt] = pSession->Sock;

		// ReadSet, WriteSet ���
		FD_SET(pSession->Sock, &ReadSet);
		if (pSession->SendQ.GetUseSize() > 0)
			FD_SET(pSession->Sock, &WriteSet);

		++iSockCnt;
		//-----------------------------------------------------
		// select �ִ�ġ ����, ������� ���̺� ������ select ȣ�� �� ����
		//-----------------------------------------------------
		if (FD_SETSIZE <= iSockCnt)
		{
			SelectSocket(UserTable_SOCKET, &ReadSet, &WriteSet, iSockCnt);

			FD_ZERO(&ReadSet);
			FD_ZERO(&WriteSet);

			memset(UserTable_SOCKET, 0, sizeof(SOCKET) * FD_SETSIZE);
			//---------------------------------------------------------
			//  ���� �����忡 select ����� ��� ������ õ �� �̻����� ��������
			// ������ ������ ���ϰ� �ɸ��� ��� ������ ������ ��û ��������.
			//  ���� 64���� select ó���� �� �� �Ź� accept ó���� ���ֵ��� �Ѵ�.
			// ������ ó���� ������ �� ��Ȱ�ϰ� ����ȴ�.
			//---------------------------------------------------------
			FD_SET(g_ListenSocket, &ReadSet);
			UserTable_SOCKET[0] = g_ListenSocket;
			iSockCnt = 1;
		}
	}
	//-----------------------------------------------------
	// ��ü Ŭ���̾�Ʈ for �� ���� �� iSockCnt ��ġ�� �ִٸ�
	// �߰������� ������ Select ȣ���� ���ش�
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
	// select() �Լ��� ����Ʈ �Ѱ� ���� FD_SETSIZE(�⺻ 64����)�� ���� �ʾҴٸ� 
	// TimeOut �ð�(Select ��� �ð�)�� 0���� ���� �ʿ䰡 ����.
	//
	// �׷���, FD_SETSIZE(�⺻ 64����)�� �Ѿ��ٸ� Select�� �� �� �̻� �ؾ��Ѵ�.
	// �� ��� TimeOut �ð��� 0���� �ؾ��ϴµ� 
	// �� ������ ù��° select���� ���� �ɷ� �ι�° select�� �������� �ʱ� �����̴�.
	////////////////////////////////////////////////////////////////////
	//-----------------------------------------------------
	// select �Լ��� ���ð� ����
	//-----------------------------------------------------
	timeval Time;
	Time.tv_sec = 0;
	Time.tv_usec = 0;

	//-----------------------------------------------------
	// ������ ��û�� ���� �������� Ŭ���̾�Ʈ���� �޽��� �۽� �˻�
	//-----------------------------------------------------
	//  select() �Լ����� block ���¿� �ִٰ� ��û�� ������,
	// select() �Լ��� ��뿡�� ��û�� ���Դ��� ������ �ϰ� �� ������ return�մϴ�.
	// select() �Լ��� return �Ǵ� ���� flag�� �� ���ϸ� WriteSet, ReadSet�� �����ְ� �˴ϴ�.
	// �׸��� ���� ���� ������ ���� flag ǥ�ø� �� ���ϵ��� checking�ϴ� ���Դϴ�.
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
				// ListenSocket�� ������ ���� �뵵�̹Ƿ� ���� ó�� 
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

	// ���������� ���� �޽��� �ð�
	pSession->dwLastRecvTick = timeGetTime();

	// ���� RecvQ�� Put�� ����� ���ٸ� �ڵ������� ����ó���� �� ���������.
	// ť�� ��������. �� ���� ó���� ���ϰ��ִٴ� ��.
	if (pSession->RecvQ.GetFreeSize() == 0)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"# FULL RecvQ #	SessionID: %ld", pSession->dwSessionID);
		return true;
	}

	int iResult = recv(pSession->Sock, pSession->RecvQ.GetWriteBufferPtr(), pSession->RecvQ.GetUnbrokenEnqueueSize(), 0);
	if (iResult == SOCKET_ERROR)
	{
		// �޴ٰ� ���� �߻��� ����
		int err = WSAGetLastError();
		if (err != 10054)
		{
			LOG(L"SYSTEM", LOG_ERROR, L"# recv() #	SessionID: %d, ErrorCode: %d", pSession->dwSessionID, err);
		}
		return false;
	}
	else if (iResult == 0)
	{
		// Ŭ���̾�Ʈ ������ ����
		LOG(L"SYSTEM", LOG_DEBUG, L"# recv() #	bytes 0 SessionID: %d", pSession->dwSessionID);
		return false;
	}

	if (pSession->RecvQ.MoveWritePos(iResult) != iResult)
	{
		LOG(L"SYSTEM", LOG_ERROR, L"# recv() #	RecvQ Enqueue Failed, SessionID: %d", pSession->dwSessionID);
		return false;
	}

	//-----------------------------------------------------
	// ��Ŷ Ȯ��
	// * ��Ŷ�� �ϳ� �̻� ���ۿ� ���� �� �����Ƿ� �ݺ������� �� ���� ���� ó���ؾ���
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
			// ���� ������� �� ũ�ٸ� ����
			// ����� �ȵǴ� ��Ȳ������ ���� �̷� ��찡 ���� �� �ִ�
			//-----------------------------------------------------
			return false;
		}

		//-----------------------------------------------------
		// Send Complate
		// ��Ŷ ������ �Ϸ�ƴٴ� �ǹ̴� �ƴ�. ���� ���ۿ� ���縦 �Ϸ��ߴٴ� �ǹ�
		// �۽� ť���� Peek���� ���´� ������ ����
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

		// ������ ũ�� �ʾҴٸ� ������ Ŭ�� ����
		shX = idrX;
		shY = idrY;
	}

	// ���� ����
	pCharacter->dwAction = byDir;
	// 2����(LL,RR)�� 8����(LL,LU,UU,RU,RR,RD,DD,LD)�� ���� 
	pCharacter->byMoveDirection = byDir;

	// ���� ���� ����
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

	// ������ �ϸ鼭 ��ǥ�� �ణ ������ ��� ���� ���
	if (Sector_UpdateCharacter(pCharacter))
	{
		// ���Ͱ� ����� ��� Ŭ�󿡰� ���� ���� ����
		CharacterSectorUpdatePacket(pCharacter);
	}

	// �̵� �׼��� ����Ǵ� �������� ���巹Ŀ���� ���� ���� ����
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
	// ĳ���� �̵����� ��Ŷ						Client -> Server
	//
	// �̵��� Ű���� �Է��� ��� �����Ǿ��� ��, �� ��Ŷ�� ������ �����ش�.
	//
	//	1	-	Direction	( ���� ������ �� ��/�츸 ��� )
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

		// ������ ũ�� �ʾҴٸ� ������ Ŭ�� ����
		shX = idrX;
		shY = idrY;
	}

	// ���� ����
	pCharacter->dwAction = dfACTION_STAND;
	// 2����(LL,RR)�� 8����(LL,LU,UU,RU,RR,RD,DD,LD)�� ���� 
	pCharacter->byMoveDirection = byDir;

	// ���� ���� ����
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

	// ������ �ϸ鼭 ��ǥ�� �ణ ������ ��� ���� ���
	if (Sector_UpdateCharacter(pCharacter))
	{
		// ���Ͱ� ����� ��� Ŭ�󿡰� ���� ���� ����
		CharacterSectorUpdatePacket(pCharacter);
	}

	// �̵� �׼��� ����Ǵ� �������� ���巹Ŀ���� ���� ���� ����
	pCharacter->dwActionTick = timeGetTime();

	mpMoveStop(Packet, session->dwSessionID, byDir, pCharacter->shX, pCharacter->shY);
	SendPacket_Around(session, Packet);
	return true;
}

bool PacketProc_Attack1(st_SESSION * session, mylib::CSerialBuffer * Packet)
{
	//---------------------------------------------------------------
	// ĳ���� ���� ��Ŷ							Client -> Server
	//
	// ���� Ű �Է½� �� ��Ŷ�� �������� ������.
	// �浹 �� �������� ���� ����� �������� �˷� �� ���̴�.
	//
	// ���� ���� ���۽� �ѹ��� �������� ������� �Ѵ�.
	//
	//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
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
	// ĳ���� ���� ��Ŷ							Client -> Server
	//
	// ���� Ű �Է½� �� ��Ŷ�� �������� ������.
	// �浹 �� �������� ���� ����� �������� �˷� �� ���̴�.
	//
	// ���� ���� ���۽� �ѹ��� �������� ������� �Ѵ�.
	//
	//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
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
	// ĳ���� ���� ��Ŷ							Client -> Server
	//
	// ���� Ű �Է½� �� ��Ŷ�� �������� ������.
	// �浹 �� �������� ���� ����� �������� �˷� �� ���̴�.
	//
	// ���� ���� ���۽� �ѹ��� �������� ������� �Ѵ�.
	//
	//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
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
	// Echo ���� ��Ŷ						Client -> Server
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
	// ���� ���� �˻�
	// ��Ŷ��� ũ�� �̻����� ���� �޾Ҵٸ� ��ŵ
	//-----------------------------------------------------
	int iRecvQSize = pSession->RecvQ.GetUseSize();
	if (iRecvQSize < sizeof(st_NETWORK_PACKET_HEADER)) // ó���� ��Ŷ����
	{
		return false;
	}

	//-----------------------------------------------------
	// I. PacketCode �˻�
	//-----------------------------------------------------
	// Peek���� �˻縦 �ϴ� ������ ����� ���� �� ������ �� �� �ϳ��� �ϼ��� ��Ŷ��ŭ�� �����Ͱ� �ִ��� Ȯ����
	// ��Ŷ�� ���� ������ �׳� �ߴ����� ����
	// Get���� ��� ���� �˻� �� ����� �ȸ´ٸ� ����� �ٽ� ť�� ���ڸ��� �־�� �ϴµ� FIFO �̹Ƿ� �����
	st_NETWORK_PACKET_HEADER stPacketHeader;
	pSession->RecvQ.Peek((char*)&stPacketHeader, sizeof(st_NETWORK_PACKET_HEADER));
	if (stPacketHeader.byCode != dfNETWORK_PACKET_CODE)
	{
		LOG(L"SYSTEM", LOG_ERROR, L" # RECV PACKET PROCESS ERROR #	PacketCode Error ID : %d", pSession->dwSessionID);
		DisconnectSession(pSession->Sock);
		return false;
	}
	//-----------------------------------------------------
	// II. ť�� ����� �����Ͱ� ������ϴ� ��Ŷ�� ũ�⸸ŭ �ִ°�
	//-----------------------------------------------------
	BYTE byEndcode;
	if ((WORD)iRecvQSize < stPacketHeader.bySize + sizeof(st_NETWORK_PACKET_HEADER) + sizeof(byEndcode))
	{
		return false;	// �� �̻� ó���� ��Ŷ�� ����
	}
	pSession->RecvQ.MoveReadPos(sizeof(st_NETWORK_PACKET_HEADER));


	mylib::CSerialBuffer pPacket;
	// �����Ͱ� �͹��Ͼ��� ũ�� ����
	if (stPacketHeader.bySize > pPacket.GetBufferSize())
	{
		LOG(L"SYSTEM", LOG_ERROR, L" # RECV PACKET PROCESS ERROR #	Packet Size Error ID : %d", pSession->dwSessionID);
		DisconnectSession(pSession->Sock);
		return false;
	}

	// Payload �κ� ��Ŷ ���۷� ����
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

	// �������� ��Ŷ ó�� �Լ� ȣ��
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
	//  _kbhit() �Լ� ��ü�� ������ ������ ����� Ȥ�� ���̰� �������� �������� ���� �׽�Ʈ�� �ּ�ó�� ����
	// �׷����� GetAsyncKeyState�� �� �� ������ â�� Ȱ��ȭ���� �ʾƵ� Ű�� �ν��� Windowapi�� ��� 
	// ��� �����ϳ� �ֿܼ��� �����

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
