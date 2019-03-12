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
	// FD_SET�� FD_SETSIZE��ŭ���� ���� �˻� ����
	//-----------------------------------------------------
	FD_SET ReadSet;
	FD_SET WriteSet;
	SOCKET	UserTable_SOCKET[FD_SETSIZE] = { 0, };	// FD_SET�� ��ϵ� ���� ����

	FD_ZERO(&ReadSet);
	FD_ZERO(&WriteSet);
	memset(UserTable_SOCKET, INVALID_SOCKET, sizeof(SOCKET) *FD_SETSIZE);

	//-----------------------------------------------------
	// ��� Ŭ���̾�Ʈ�� ���� Socket �˻�
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

			memset(UserTable_SOCKET, INVALID_SOCKET, sizeof(SOCKET) * FD_SETSIZE);
			//---------------------------------------------------------
			//  ���� �����忡 select ����� ��� ������ õ �� �̻����� ��������
			// ������ ������ ���ϰ� �ɸ��� ��� ������ ������ ��û ��������.
			//  ���� 64���� select ó���� �� �� �Ź� accept ó���� ���ֵ��� �Ѵ�.
			// ������ ó���� ������ �� ��Ȱ�ϰ� ����ȴ�.
			//---------------------------------------------------------
			iSockCnt = 0;
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
	// ��Ŷ Ȯ��
	// * ��Ŷ�� �ϳ� �̻� ���ۿ� ���� �� �����Ƿ� �ݺ������� �� ���� ���� ó���ؾ���
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
	// 0 - Ŭ���̾�Ʈ �ڽ��� ĳ���� �Ҵ�		Server -> Client
	//
	// ������ ���ӽ� ���ʷ� �ްԵǴ� ��Ŷ���� �ڽ��� �Ҵ���� ID ��
	// �ڽ��� ���� ��ġ, HP �� �ް� �ȴ�. (ó���� �ѹ� �ް� ��)
	// 
	// �� ��Ŷ�� ������ �ڽ��� ID,X,Y,HP �� �����ϰ� ĳ���͸� �������Ѿ� �Ѵ�.
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
	// ����ȭ�� ���� ��Ŷ					Server -> Client
	//
	// �����κ��� ����ȭ ��Ŷ�� ������ �ش� ĳ���͸� ã�Ƽ�
	// ĳ���� ��ǥ�� �������ش�.
	//
	//	4	-	ID
	//	2	-	X
	//	2	-	Y
	//
	//
	//  MMO�� ��� ����ڰ� ���������� Ŭ�� �Ӹ� �ƴ϶� �����ʵ� ������ �� �ִ�. 
	// �׶��� ����ؼ� Sync ��Ŷ�� ����
	//
	// 1. ���� Ʋ������ ��� ���� �� �ְ�-
	// 2. ���÷� ���� ���� �ִ�.
	//
	//  ��, 2���� ��� TCP�� �ƴ϶� UDP�� ����ȴ�.
	// �ֳ��ϸ� TCP�� ����Ͽ� ���÷� ������ ��� Ack�� ������ ���ϰ� �߻�
	// UDP�� �׷��� ���� ������ �´�.
	//
	//  ����� ���콺�� �̵��ϴ� ������ ��쿡�� ������ Ŭ�� ������ �����Ƿ�
	// ������ �ǵ������� �������� �ʴ� �� ��ǥ�� Ʋ���� ���� ��� �̷� ó���� �����ʾƵ� �ȴ�.
	//  Ű���� ������ ��� �������� �̵��� ���� ������ �ֱ� ���� ���� ������ �� �����Ƿ�
	// ��ǥ�� Ʋ���� �� �ִ�.
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
	// Echo ���� ��Ŷ						Client -> Server
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
		LOG(L"SYSTEM", LOG_ERROR, L" # RECV PACKET PROCESS ERROR # PacketCode Error ID : %d", pSession->dwSessionID);
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
		LOG(L"SYSTEM", LOG_ERROR, L" # RECV PACKET PROCESS ERROR # Packet Size Error ID : %d", pSession->dwSessionID);
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

	// �������� ��Ŷ ó�� �Լ� ȣ��
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
	// 10. ĳ���� �̵����� ��Ŷ						Client -> Server
	//
	// �ڽ��� ĳ���� �̵����۽� �� ��Ŷ�� ������.
	// �̵� �߿��� �� ��Ŷ�� ������ ������, Ű �Է��� ����Ǿ��� ��쿡��
	// ������� �Ѵ�.
	//
	// (���� �̵��� ���� �̵� / ���� �̵��� ���� ���� �̵�... ���)
	//
	//	1	-	Direction	( ���� ������ �� 8���� ��� )
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
	// ĳ���� �̵����� ��Ŷ						Client -> Server
	//
	// �̵��� Ű���� �Է��� ��� �����Ǿ��� ��, �� ��Ŷ�� ������ �����ش�.
	//
	//	1	-	Direction	( ���� ������ �� ��/�츸 ��� )
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
