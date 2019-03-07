#include "stdafx.h"
#include "NetworkProc.h"

using namespace mylib;


SOCKET g_Socket;
BOOL g_bConnect = FALSE;
BOOL g_bSendFlag = FALSE;
CRingBuffer g_RecvQ;
CRingBuffer g_SendQ;

int NetworkInit(WCHAR * szIP)
{
	int err;
	WSADATA	wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		err = WSAGetLastError();
		return err;
	}
	g_Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (g_Socket == INVALID_SOCKET)
	{
		err = WSAGetLastError();
		return err;
	}

	BOOL bEnable = TRUE;
	setsockopt(g_Socket, IPPROTO_TCP, TCP_NODELAY, (char *)&bEnable, sizeof(bEnable));

	SOCKADDR_IN serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(dfSERVER_PORT);
	InetPton(AF_INET, szIP, &serveraddr.sin_addr);

	if (WSAAsyncSelect(g_Socket, g_hWnd, UM_NETWORK, FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE) == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		return err;
	}

	if (connect(g_Socket, (SOCKADDR *)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
			return err;
	}

	return 0;
}

bool NetworkProc(WPARAM wParam, LPARAM lParam)
{
	// TODO: WSAAsyncSelect()�� �����ϴ� �޼����� LPARAM ��
	//  WSAGETSELECTERROR(LPARAM) : LPARAM�� ���� 16��Ʈ. ���� �ڵ� == HIWORD(LPARAM)
	//  WSAGETSELECTEVENT(LPARAM) : LPARAM�� ���� 16��Ʈ. WSAAsyncSelect() ������ ���ڿ��� ������ �̺�Ʈ �� �߻��� �̺�Ʈ == LOWORD(LPARAM)
	if (WSAGETSELECTERROR(lParam) != 0)
		return FALSE;

	switch (WSAGETSELECTEVENT(lParam))
	{
	case FD_CONNECT:
		g_bConnect = TRUE;
		return TRUE;
	case FD_READ:
	{
		// Ŭ���� ���, Read���� �ݺ��� �־ WOULDBLOCK �� ������ ��� ó���ص� ��
		if (!RecvEvent())
			return FALSE;
		return TRUE;
	}
	case FD_WRITE: //ó�� ����������, WOULDBLOCK�� �߰� ������ ����
		g_bSendFlag = TRUE;
		if (!SendEvent())
			return FALSE;
		return TRUE;
	case FD_CLOSE:
		//���� ����. ��� `������ ���������ϴ�` �޽��� ���� ��ġ
		return FALSE;
	}
	return TRUE;
}

void Network_Close()
{
	g_bConnect = FALSE;
	g_bSendFlag = FALSE;
	closesocket(g_Socket);
	WSACleanup();
}


bool SendEvent()
{
	char SendBuff[CRingBuffer::en_BUFFER_DEFALUT];

	if (!g_bSendFlag)	// Send ���ɿ��� Ȯ�� / �̴� FD_WRITE �߻������� -> AsyncSelect������ connect ������ FD_WRITE�� ��
		return FALSE;

	// g_SendQ�� �ִ� �����͵��� �ִ� dfNETWORK_WSABUFF_SIZE ũ��� ������
	int iSendSize = g_SendQ.GetUseSize();
	iSendSize = min(CRingBuffer::en_BUFFER_DEFALUT, iSendSize);
	// g_SendQ �� ���� �����Ͱ� �ִ��� Ȯ��
	if (iSendSize <= 0)
		return TRUE;

	g_bSendFlag = TRUE;

	while (1)
	{
		// g_SendQ �� �����Ͱ� �ִ��� Ȯ�� (������)
		iSendSize = g_SendQ.GetUseSize();
		iSendSize = min(CRingBuffer::en_BUFFER_DEFALUT, iSendSize);
		if (iSendSize <= 0)
			break;

		// Peek���� ������ ����. ������ ����� ���������� ��� ����
		g_SendQ.Peek(SendBuff, iSendSize);

		// Send
		int iResult = send(g_Socket, SendBuff, iSendSize, 0);
		if (iResult == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err != WSAEWOULDBLOCK)
			{
				g_bSendFlag = FALSE;
				break;
			}
		}
		else
		{
			if (iResult > iSendSize)
			{
				//-----------------------------------------------------
				// ���� ������� �� ũ�ٸ� ����
				// ����� �ȵǴ� ��Ȳ������ ���� �̷� ��찡 ���� �� �ִ�
				//-----------------------------------------------------
				return false;
			}
			else
			{
				//-----------------------------------------------------
				// Send Complate
				// ��Ŷ ������ �Ϸ�ƴٴ� �ǹ̴� �ƴ�. ���� ���ۿ� ���縦 �Ϸ��ߴٴ� �ǹ�
				// �۽� ť���� Peek���� ���´� ������ ����
				//-----------------------------------------------------
				g_SendQ.MoveReadPos(iResult);
			}
		}
	}
	return true;
}

bool SendPacket(st_NETWORK_PACKET_HEADER  *pHeader, CSerialBuffer *pPacket)
{
	// ���ӻ��� ����ó��
	g_bSendFlag = true;
	g_SendQ.Enqueue((char*)pHeader, sizeof(st_NETWORK_PACKET_HEADER));
	g_SendQ.Enqueue((char*)pPacket->GetBufferPtr(), pPacket->GetUseSize());
	SendEvent();
	return true;
}

//  ������ ������ Send �ÿ��� �׻� g_SendQ �� ���� �����͸� �ְ�, 
// ���� Send �� ������ �������� Ȯ�� �� Send �� ȣ���Ͽ� ���������� ������ ����� g_SendQ ���� ����� �������� ����.
//
//  ��Ŷ ������ ������ ������ ��Ŷ���� ���� �Լ��� ������ ���� ��Ŷ ������ �����ڵ带 �����Ѵ�.
//
//  ��Ŷ������ ����� ���̷ε尡 �ʿ��ϹǷ� �̿� ���� �����͸� ���ڷ� �޾� ���� �Լ� ���ο��� �����͸� �����ϸ�, 
// �̿� �ʿ��� �������� �߰� ���ڷ� �޾Ƽ� ��Ŷ �����ͷ� �����Ѵ�.
// ��Ÿ �ʿ������� �ִ��� �ܺ� �ڵ忡 ���ӵ��� �ʵ��� �������� ������ ���ڷ� �ѱ�� ���� ����.

void mpMoveStart(st_NETWORK_PACKET_HEADER *pHeader, CSerialBuffer *pPacket, BYTE iDir, WORD iX, WORD iY)
{
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

	*pPacket << iDir;
	*pPacket << iX;
	*pPacket << iY;
	*pPacket << (BYTE)dfNETWORK_PACKET_END;

	pHeader->byCode = dfNETWORK_PACKET_CODE;
	pHeader->bySize = 5;
	pHeader->byType = dfPACKET_CS_MOVE_START;
}

void mpMoveStop(st_NETWORK_PACKET_HEADER *pHeader, CSerialBuffer *pPacket, BYTE iDir, WORD iX, WORD iY)
{
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

	*pPacket << iDir;
	*pPacket << iX;
	*pPacket << iY;
	*pPacket << (BYTE)dfNETWORK_PACKET_END;

	pHeader->byCode = dfNETWORK_PACKET_CODE;
	pHeader->bySize = 5;
	pHeader->byType = dfPACKET_CS_MOVE_STOP;
}

void mpAttack1(st_NETWORK_PACKET_HEADER *pHeader, CSerialBuffer *pPacket, BYTE iDir, WORD iX, WORD iY)
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
	pPacket->Clear();

	*pPacket << iDir;
	*pPacket << iX;
	*pPacket << iY;
	*pPacket << (BYTE)dfNETWORK_PACKET_END;

	pHeader->byCode = dfNETWORK_PACKET_CODE;
	pHeader->bySize = 5;
	pHeader->byType = dfPACKET_CS_ATTACK1;
}

void mpAttack2(st_NETWORK_PACKET_HEADER *pHeader, CSerialBuffer *pPacket, BYTE iDir, WORD iX, WORD iY)
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
	pPacket->Clear();

	*pPacket << iDir;
	*pPacket << iX;
	*pPacket << iY;
	*pPacket << (BYTE)dfNETWORK_PACKET_END;

	pHeader->byCode = dfNETWORK_PACKET_CODE;
	pHeader->bySize = 5;
	pHeader->byType = dfPACKET_CS_ATTACK2;
}

void mpAttack3(st_NETWORK_PACKET_HEADER *pHeader, CSerialBuffer *pPacket, BYTE iDir, WORD iX, WORD iY)
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
	pPacket->Clear();

	*pPacket << iDir;
	*pPacket << iX;
	*pPacket << iY;
	*pPacket << (BYTE)dfNETWORK_PACKET_END;

	pHeader->byCode = dfNETWORK_PACKET_CODE;
	pHeader->bySize = 5;
	pHeader->byType = dfPACKET_CS_ATTACK3;
}

bool RecvEvent()
{
	WSABUF wsabuf[2];
	DWORD dwTransferred = 0;
	DWORD dwFlag = 0;
	int iBufCnt = 1;

	wsabuf[0].buf = g_RecvQ.GetWriteBufferPtr();
	wsabuf[0].len = g_RecvQ.GetUnbrokenEnqueueSize();
	if (wsabuf[0].len < g_RecvQ.GetFreeSize())
	{
		wsabuf[1].buf = g_RecvQ.GetBufferPtr();
		wsabuf[1].len = g_RecvQ.GetFreeSize() - wsabuf[0].len;
		++iBufCnt;
	}

	int iResult = WSARecv(g_Socket, wsabuf, iBufCnt, &dwTransferred, &dwFlag, NULL, NULL);
	if (iResult == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			return false;
		}
	}

	g_RecvQ.MoveWritePos(dwTransferred);

	while (1)
	{
		iResult = RecvComplete();

		// ó���� ��Ŷ�� ����
		if (iResult == 1)
			break;
		// ��Ŷ ó�� ����
		if (iResult == -1)
		{
			//_LOG(dfLOG_LEVEL_ERROR, L" # RECV PACKET PROCESS ERROR #	ID : %d", pSession->dwSessionID);
			break;
		}
	}

	return true;
}

int RecvComplete()
{
	st_NETWORK_PACKET_HEADER stPacketHeader;
	BYTE byEndcode;

	///////////////////////////////////////////////////////
	// Recv Packet - Check
	//
	///////////////////////////////////////////////////////
	// RecvQ�� Header���̸�ŭ �ִ��� Ȯ��
	int g_iRecvQSize = g_RecvQ.GetUseSize();
	if (g_iRecvQSize < sizeof(st_NETWORK_PACKET_HEADER))
		return 1;

	// Packet ���� Ȯ�� (Headerũ�� + Payload���� + EndCode ����)
	g_RecvQ.Peek((char*)&stPacketHeader, sizeof(st_NETWORK_PACKET_HEADER));
	if ((WORD)g_iRecvQSize < sizeof(st_NETWORK_PACKET_HEADER) + stPacketHeader.bySize + sizeof(byEndcode))
		return 1;

	// ���� ��Ŷ���κ��� Header ����
	g_RecvQ.MoveReadPos(sizeof(st_NETWORK_PACKET_HEADER));

	///////////////////////////////////////////////////////
	// Recv Packet - Process
	//
	///////////////////////////////////////////////////////
	if (stPacketHeader.byCode != dfNETWORK_PACKET_CODE)
		return -1;

	CSerialBuffer Packet;
	// Payload �κ� ��Ŷ ���۷� ����
	if (!g_RecvQ.Dequeue(Packet.GetBufferPtr(), stPacketHeader.bySize))
		return -1;

	// Endcode ���� �� Ȯ��
	g_RecvQ.Dequeue((char*)&byEndcode, 1);
	if (byEndcode != dfNETWORK_PACKET_END)
		return -1;
	
	Packet.MoveWritePos(stPacketHeader.bySize);

	// �������� ��Ŷ ó�� �Լ� ȣ��
	if (!OnRecv(stPacketHeader.byType, &Packet))
		return -1;

	return 0;
}

bool OnRecv(BYTE byType, CSerialBuffer* pPacket)
{
	switch (byType)
	{
	case dfPACKET_SC_CREATE_MY_CHARACTER:
		netPacketProc_CreateMyCharacter(pPacket);
		break;
	case dfPACKET_SC_CREATE_OTHER_CHARACTER:
		netPacketProc_CreateOtherCharacter(pPacket);
		break;
	case dfPACKET_SC_DELETE_CHARACTER:
		netPacketProc_DeleteCharacter(pPacket);
		break;
	case dfPACKET_SC_MOVE_START:
		netPacketProc_MoveStart(pPacket);
		break;
	case dfPACKET_SC_MOVE_STOP:
		netPacketProc_MoveStop(pPacket);
		break;
	case dfPACKET_SC_ATTACK1:
		netPacketProc_Attack1(pPacket);
		break;
	case dfPACKET_SC_ATTACK2:
		netPacketProc_Attack2(pPacket);
		break;
	case dfPACKET_SC_ATTACK3:
		netPacketProc_Attack3(pPacket);
		break;
	case dfPACKET_SC_DAMAGE:
		netPacketProc_Damage(pPacket);
		break;
	}
	return true;
}

void netPacketProc_CreateMyCharacter(CSerialBuffer * pPacket)
{
	DWORD ID;
	BYTE Dir;
	WORD X, Y;
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
	*pPacket >> X;
	*pPacket >> Y;
	*pPacket >> HP;

	g_pMyPlayer = new CPlayer(X, Y, TRUE, 0, ID, HP, Dir);
	g_lst.push_back((CBaseObject*)g_pMyPlayer);
}

void netPacketProc_CreateOtherCharacter(CSerialBuffer * pPacket)
{
	CPlayer* newPlayer;
	DWORD ID;
	BYTE Dir;
	WORD X, Y;
	BYTE HP;
	//---------------------------------------------------------------
	// 1. �ٸ� Ŭ���̾�Ʈ�� ĳ���� ���� ��Ŷ		Server -> Client
	//
	// ó�� ������ ���ӽ� �̹� ���ӵǾ� �ִ� ĳ���͵��� ����
	// �Ǵ� �����߿� ���ӵ� Ŭ���̾�Ʈ���� ���� �� ����.
	//
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
	*pPacket >> X;
	*pPacket >> Y;
	*pPacket >> HP;


	newPlayer = new CPlayer(X, Y, FALSE, 0, ID, HP, Dir);
	g_lst.push_back((CBaseObject*)newPlayer);
}

void netPacketProc_DeleteCharacter(CSerialBuffer * pPacket)
{
	DWORD ID;
	//---------------------------------------------------------------
	// 2. ĳ���� ���� ��Ŷ						Server -> Client
	//
	// ĳ������ �������� �Ǵ� ĳ���Ͱ� �׾����� ���۵�.
	//
	//	4	-	ID
	//
	//---------------------------------------------------------------
	*pPacket >> ID;

	for (CLinkedlist<CBaseObject*>::iterator Object_iter = g_lst.begin(); Object_iter != g_lst.end(); ++Object_iter)
	{
		CLinkedlist<CBaseObject*>::Node* pNode = Object_iter.getNode();

		if (pNode != nullptr)
		{
			if ((pNode->_Data)->GetObjectID() == ID)
			{
				if (pNode->_Data != nullptr)
				{
					delete pNode->_Data;
					g_lst.erase(Object_iter);
					break;
				}
			}
		}
	}
}

void netPacketProc_MoveStart(CSerialBuffer * pPacket)
{
	DWORD ID;
	BYTE Dir;
	WORD X, Y;
	//---------------------------------------------------------------
	// ĳ���� �̵����� ��Ŷ						Server -> Client
	//
	// �ٸ� ������ ĳ���� �̵��� �� ��Ŷ�� �޴´�.
	// ��Ŷ ���Ž� �ش� ĳ���͸� ã�� �̵�ó���� ���ֵ��� �Ѵ�.
	// 
	// ��Ŷ ���� �� �ش� Ű�� ����ؼ� ���������� �����ϰ�
	// �ش� �������� ��� �̵��� �ϰ� �־�߸� �Ѵ�.
	//
	//	4	-	ID
	//	1	-	Direction	( ���� ������ �� 8���� )
	//	2	-	X
	//	2	-	Y
	//
	//---------------------------------------------------------------
	*pPacket >> ID;
	*pPacket >> Dir;
	*pPacket >> X;
	*pPacket >> Y;


	for (CLinkedlist<CBaseObject*>::iterator Object_iter = g_lst.begin(); Object_iter != g_lst.end(); ++Object_iter)
	{
		if ((*Object_iter) != nullptr)
		{
			if ((*Object_iter)->GetObjectID() == ID)
			{
				if ((*Object_iter) != nullptr)
				{
					(*Object_iter)->SetPosition(X, Y);
					(*Object_iter)->ActionInput(Dir);
					break;
				}
			}
		}
	}
}

void netPacketProc_MoveStop(CSerialBuffer * pPacket)
{
	DWORD ID;
	BYTE Dir;
	WORD X, Y;
	//---------------------------------------------------------------
	// ĳ���� �̵����� ��Ŷ						Server -> Client
	//
	// ID �� �ش��ϴ� ĳ���Ͱ� �̵��� ������̹Ƿ� 
	// ĳ���͸� ã�Ƽ� �����, ��ǥ�� �Է����ְ� ���ߵ��� ó���Ѵ�.
	//
	//	4	-	ID
	//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
	//	2	-	X
	//	2	-	Y
	//
	//---------------------------------------------------------------
	*pPacket >> ID;
	*pPacket >> Dir;
	*pPacket >> X;
	*pPacket >> Y;

	for (CLinkedlist<CBaseObject*>::iterator Object_iter = g_lst.begin(); Object_iter != g_lst.end(); ++Object_iter)
	{
		if ((*Object_iter) != nullptr)
		{
			if ((*Object_iter)->GetObjectID() == ID)
			{
				if ((*Object_iter) != nullptr)
				{
					(*Object_iter)->SetDirection(Dir);
					(*Object_iter)->SetPosition(X, Y);
					(*Object_iter)->ActionInput(dfACTION_STAND);
					break;
				}
			}
		}
	}
}

void netPacketProc_Attack1(CSerialBuffer * pPacket)
{
	DWORD ID;
	BYTE Dir;
	WORD X, Y;
	//---------------------------------------------------------------
	// ĳ���� ���� ��Ŷ							Server -> Client
	//
	// ��Ŷ ���Ž� �ش� ĳ���͸� ã�Ƽ� ����1�� �������� �׼��� �����ش�.
	// ������ �ٸ� ��쿡�� �ش� �������� �ٲ� �� ���ش�.
	//
	//	4	-	ID
	//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
	//	2	-	X
	//	2	-	Y
	//
	//---------------------------------------------------------------
	*pPacket >> ID;
	*pPacket >> Dir;
	*pPacket >> X;
	*pPacket >> Y;

	for (CLinkedlist<CBaseObject*>::iterator Object_iter = g_lst.begin(); Object_iter != g_lst.end(); ++Object_iter)
	{
		if ((*Object_iter) != nullptr)
		{
			if ((*Object_iter)->GetObjectID() == ID)
			{
				if ((*Object_iter) != nullptr)
				{
					(*Object_iter)->SetDirection(Dir);
					(*Object_iter)->SetPosition(X, Y);
					(*Object_iter)->ActionInput(dfACTION_ATTACK1);
					break;
				}
			}
		}
	}
}

void netPacketProc_Attack2(CSerialBuffer * pPacket)
{
	DWORD ID;
	BYTE Dir;
	WORD X, Y;
	//---------------------------------------------------------------
	// ĳ���� ���� ��Ŷ							Server -> Client
	//
	// ��Ŷ ���Ž� �ش� ĳ���͸� ã�Ƽ� ����2�� �������� �׼��� �����ش�.
	// ������ �ٸ� ��쿡�� �ش� �������� �ٲ� �� ���ش�.
	//
	//	4	-	ID
	//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
	//	2	-	X
	//	2	-	Y
	//
	//---------------------------------------------------------------
	*pPacket >> ID;
	*pPacket >> Dir;
	*pPacket >> X;
	*pPacket >> Y;

	for (CLinkedlist<CBaseObject*>::iterator Object_iter = g_lst.begin(); Object_iter != g_lst.end(); ++Object_iter)
	{
		if ((*Object_iter) != nullptr)
		{
			if ((*Object_iter)->GetObjectID() == ID)
			{
				if ((*Object_iter) != nullptr)
				{
					(*Object_iter)->SetDirection(Dir);
					(*Object_iter)->SetPosition(X, Y);
					(*Object_iter)->ActionInput(dfACTION_ATTACK2);
					break;
				}
			}
		}
	}
}

void netPacketProc_Attack3(CSerialBuffer * pPacket)
{
	DWORD ID;
	BYTE Dir;
	WORD X, Y;
	//---------------------------------------------------------------
	// ĳ���� ���� ��Ŷ							Server -> Client
	//
	// ��Ŷ ���Ž� �ش� ĳ���͸� ã�Ƽ� ����2�� �������� �׼��� �����ش�.
	// ������ �ٸ� ��쿡�� �ش� �������� �ٲ� �� ���ش�.
	//
	//	4	-	ID
	//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
	//	2	-	X
	//	2	-	Y
	//
	//---------------------------------------------------------------
	*pPacket >> ID;
	*pPacket >> Dir;
	*pPacket >> X;
	*pPacket >> Y;

	for (CLinkedlist<CBaseObject*>::iterator Object_iter = g_lst.begin(); Object_iter != g_lst.end(); ++Object_iter)
	{
		if ((*Object_iter) != nullptr)
		{
			if ((*Object_iter)->GetObjectID() == ID)
			{
				if ((*Object_iter) != nullptr)
				{
					(*Object_iter)->SetDirection(Dir);
					(*Object_iter)->SetPosition(X, Y);
					(*Object_iter)->ActionInput(dfACTION_ATTACK3);
					break;
				}
			}
		}
	}
}

void netPacketProc_Damage(CSerialBuffer * pPacket)
{
	DWORD ID1, ID2;
	BYTE ID2_HP;
	//---------------------------------------------------------------
	// ĳ���� ������ ��Ŷ							Server -> Client
	//
	// ���ݿ� ���� ĳ������ ������ ����.
	//
	//	4	-	AttackID	( ������ ID )
	//	4	-	DamageID	( ������ ID )
	//	1	-	DamageHP	( ������ HP )
	//
	//---------------------------------------------------------------
	*pPacket >> ID1;
	*pPacket >> ID2;
	*pPacket >> ID2_HP;

	// Ÿ�� ��ȿ �˸�
	for (CLinkedlist<CBaseObject*>::iterator Object_iter = g_lst.begin(); Object_iter != g_lst.end(); ++Object_iter)
	{
		if ((*Object_iter) != nullptr)
		{
			if ((*Object_iter)->GetObjectID() == ID1)
			{
				if ((*Object_iter) != nullptr)
				{
					(*Object_iter)->SetEffect();
					break;
				}
			}
		}
	}


	// HP ����
	for (CLinkedlist<CBaseObject*>::iterator Object_iter = g_lst.begin(); Object_iter != g_lst.end(); ++Object_iter)
	{
		if ((*Object_iter) != nullptr)
		{
			if ((*Object_iter)->GetObjectID() == ID2)
			{
				if ((*Object_iter) != nullptr)
				{
					(*Object_iter)->SetHP(ID2_HP);
					break;
				}
			}
		}
	}
}

void netPacketProc_Sync(CSerialBuffer * pPacket)
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

	for (CLinkedlist<CBaseObject*>::iterator Object_iter = g_lst.begin(); Object_iter != g_lst.end(); ++Object_iter)
	{
		if ((*Object_iter) != nullptr)
		{
			if ((*Object_iter)->GetObjectID() == ID)
			{
				if ((*Object_iter) != nullptr)
				{
					(*Object_iter)->SetPosition(X, Y);
					break;
				}
			}
		}
	}
}