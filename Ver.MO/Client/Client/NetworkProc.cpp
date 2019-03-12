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

void NetworkClose()
{
	closesocket(g_Socket);
	WSACleanup();
}

bool RecvEvent()
{
	st_NETWORK_PACKET_HEADER PacketHeader;
	int retval;
	char Packet[CRingBuffer::en_BUFFER_DEFALUT];	// �ӽ� ���Ź���
	BYTE Endcode = 0;	// ��Ŷ�� �ӽù��� ��� Ÿ���� ��Ŷ�� ó���� �� �ִ� �˳��� ������

	retval = recv(g_Socket, Packet, g_RecvQ.GetFreeSize(), 0);
	if (retval == 0)
		return false;	// recv ���ϰ����� ���� (0) / ���� (SOCKET_ERROR) ó��

	if (retval == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
			return false;
	}
	if (g_RecvQ.GetFreeSize() <= 0)
		return false;

	// �����͸� ������ �ϴ� g_RecvQ�� ����
	g_RecvQ.Enqueue(Packet, retval);


	// * �Ϸ���Ŷ ó�� �� - g_RecvQ �� ����ִ� ��� �ϼ���Ŷ�� ó�� �ؾ� ��.
	while (1)
	{
		// 1.	g_RecvQ �� �ּ����� ����� �ִ��� Ȯ��. ���� - ��������� �ʰ�
		if (g_RecvQ.GetUseSize() < sizeof(st_NETWORK_PACKET_HEADER))
			break;

		// 2.	g_RecvQ ���� ����� Peek
		retval = g_RecvQ.Peek((char*)&PacketHeader, sizeof(st_NETWORK_PACKET_HEADER));

		// 3.	����� Code Ȯ��
		if (PacketHeader.byCode != dfNETWORK_PACKET_CODE)
			return false;

		// 4.	����� Len ���� g_RecvQ �� ������ ������ �� 		-�ϼ���Ŷ ������ : ��� + Len + �����ڵ� ////
		if (g_RecvQ.GetUseSize() < sizeof(PacketHeader) + PacketHeader.bySize + sizeof(dfNETWORK_PACKET_END))
			return false;

		// 5.	������ Peek �ߴ� ����� g_RecvQ ���� ����� (�̹� ���ξ����Ƿ� �� �� �ʿ� ����)
		g_RecvQ.MoveReadPos(retval); // Dequeue�� �� �� ������ memcpy�� �ʹ� ���̱� ����

									 // 6.	g_RecvQ ���� Len ��ŭ �ӽ� ��Ŷ���۷� ����. 
		if (g_RecvQ.Dequeue(Packet, PacketHeader.bySize) == 0)
			return false;

		// 7.	g_RecvQ ���� �����ڵ��� 1 Byte �� ����
		retval = g_RecvQ.Dequeue((char*)&Endcode, sizeof(dfNETWORK_PACKET_END));
		if (retval == 0)
			return false;

		// 8.	�����ڵ� Ȯ�� ////
		if (Endcode != dfNETWORK_PACKET_END)
			return false;

		// 9.	����� Ÿ�Կ� ���� �б⸦ ���� ��Ŷ ���ν��� ȣ��
		OnRecv(PacketHeader.byType, Packet);
	}

	return true;
}

bool OnRecv(BYTE byType, char* Packet)
{
	switch (byType)
	{
	case dfPACKET_SC_CREATE_MY_CHARACTER:
		OnRecv_CreateMyCharacter((st_PACKET_SC_CREATE_MY_CHARACTER *)Packet);
		break;
	case dfPACKET_SC_CREATE_OTHER_CHARACTER:
		OnRecv_CreateOtherCharacter((st_PACKET_SC_CREATE_OTHER_CHARACTER *)Packet);
		break;
	case dfPACKET_SC_DELETE_CHARACTER:
		OnRecv_DeleteCharacter((st_PACKET_SC_DELETE_CHARACTER *)Packet);
		break;
	case dfPACKET_SC_MOVE_START:
		OnRecv_MoveStart((st_PACKET_SC_MOVE_START *)Packet);
		break;
	case dfPACKET_SC_MOVE_STOP:
		OnRecv_MoveStop((st_PACKET_SC_MOVE_STOP *)Packet);
		break;
	case dfPACKET_SC_ATTACK1:
		OnRecv_Attack1((st_PACKET_SC_ATTACK1 *)Packet);
		break;
	case dfPACKET_SC_ATTACK2:
		OnRecv_Attack2((st_PACKET_SC_ATTACK2 *)Packet);
		break;
	case dfPACKET_SC_ATTACK3:
		OnRecv_Attack3((st_PACKET_SC_ATTACK3 *)Packet);
		break;
	case dfPACKET_SC_DAMAGE:
		OnRecv_Damage((st_PACKET_SC_DAMAGE *)Packet);
		break;
	}
	return true;
}

void OnRecv_CreateMyCharacter(st_PACKET_SC_CREATE_MY_CHARACTER * Packet)
{
	g_pMyPlayer = new CPlayer(Packet->X, Packet->Y, TRUE, 0, Packet->ID, Packet->HP);
	g_pMyPlayer->SetDirection(Packet->Direction);
	g_lst.push_back((CBaseObject*)g_pMyPlayer);
}

void OnRecv_CreateOtherCharacter(st_PACKET_SC_CREATE_OTHER_CHARACTER * Packet)
{
	for (int iCnt = 0; iCnt < dfMAX_USER; iCnt++)
	{
		if (g_pPlayer[iCnt] == NULL)
		{
			g_pPlayer[iCnt] = new CPlayer(Packet->X, Packet->Y, FALSE, 0, Packet->ID, Packet->HP);
			g_pPlayer[iCnt]->SetDirection(Packet->Direction);
			g_lst.push_back((CBaseObject*)g_pPlayer[iCnt]);
			break;
		}
	}
}

void OnRecv_DeleteCharacter(st_PACKET_SC_DELETE_CHARACTER * Packet)
{
	for (ObjectList<CBaseObject*>::iterator Object_iter = g_lst.begin(); Object_iter != g_lst.end(); ++Object_iter)
	{
		ObjectList<CBaseObject*>::Node* pNode = Object_iter.getNode();

		if (pNode != nullptr)
		{
			if ((pNode->_Data)->GetObjectID() == Packet->ID)
			{
				if (pNode->_Data != NULL)
				{
					delete pNode->_Data;
					g_lst.erase(Object_iter);
					break;
				}
			}
		}
	}
}

void OnRecv_MoveStart(st_PACKET_SC_MOVE_START * Packet)
{
	for (int iCnt = 0; iCnt < dfMAX_USER; iCnt++)
		if (g_pPlayer[iCnt]->GetObjectID() == Packet->ID)
		{
			g_pPlayer[iCnt]->SetPosition(Packet->X, Packet->Y);
			g_pPlayer[iCnt]->ActionInput(Packet->Direction);
			break;
		}
}

void OnRecv_MoveStop(st_PACKET_SC_MOVE_STOP * Packet)
{
	for (int iCnt = 0; iCnt < dfMAX_USER; iCnt++)
		if (g_pPlayer[iCnt]->GetObjectID() == Packet->ID)
		{
			g_pPlayer[iCnt]->SetDirection(Packet->Direction);
			g_pPlayer[iCnt]->SetPosition(Packet->X, Packet->Y);
			g_pPlayer[iCnt]->ActionInput(dfACTION_STAND);
			break;
		}
}

void OnRecv_Attack1(st_PACKET_SC_ATTACK1 * Packet)
{
	for (int iCnt = 0; iCnt < dfMAX_USER; iCnt++)
		if (g_pPlayer[iCnt]->GetObjectID() == Packet->ID)
		{
			g_pPlayer[iCnt]->SetDirection(Packet->Direction);
			g_pPlayer[iCnt]->SetPosition(Packet->X, Packet->Y);
			g_pPlayer[iCnt]->ActionInput(dfACTION_ATTACK1);
			break;
		}
}

void OnRecv_Attack2(st_PACKET_SC_ATTACK2 * Packet)
{
	for (int iCnt = 0; iCnt < dfMAX_USER; iCnt++)
		if (g_pPlayer[iCnt]->GetObjectID() == Packet->ID)
		{
			g_pPlayer[iCnt]->SetDirection(Packet->Direction);
			g_pPlayer[iCnt]->SetPosition(Packet->X, Packet->Y);
			g_pPlayer[iCnt]->ActionInput(dfACTION_ATTACK2);
			break;
		}
}

void OnRecv_Attack3(st_PACKET_SC_ATTACK3 * Packet)
{
	for (int iCnt = 0; iCnt < dfMAX_USER; iCnt++)
		if (g_pPlayer[iCnt]->GetObjectID() == Packet->ID)
		{
			g_pPlayer[iCnt]->SetDirection(Packet->Direction);
			g_pPlayer[iCnt]->SetPosition(Packet->X, Packet->Y);
			g_pPlayer[iCnt]->ActionInput(dfACTION_ATTACK3);
			break;
		}
}

void OnRecv_Damage(st_PACKET_SC_DAMAGE * Packet)
{
	ObjectList<CBaseObject*>::iterator Object_iter;

	// Ÿ�� ��ȿ �˸�
	if (g_pMyPlayer->GetObjectID() == Packet->ID1)
	{
		g_pMyPlayer->SetEffect();
	}
	else
	{
		for (int iCnt = 0; iCnt < dfMAX_USER; iCnt++)
		{
			if (g_pPlayer[iCnt]->GetObjectID() == Packet->ID1)
			{
				g_pPlayer[iCnt]->SetEffect();
				break;
			}
		}
	}

	// HP ����
	if (g_pMyPlayer->GetObjectID() == Packet->ID2)
	{
		g_pMyPlayer->SetHP(Packet->ID2_HP);
	}
	else
	{
		for (int iCnt = 0; iCnt < dfMAX_USER; iCnt++)
		{
			if (g_pPlayer[iCnt]->GetObjectID() == Packet->ID2)
			{
				g_pPlayer[iCnt]->SetHP(Packet->ID2_HP);
				break;
			}
		}
	}
}

bool SendEvent()
{
	char Buffer[CRingBuffer::en_BUFFER_DEFALUT];	// ������� �ӽ� ����

	if (!g_bSendFlag)	// Send ���ɿ��� Ȯ�� / �̴� FD_WRITE �߻������� -> AsyncSelect������ connect ������ FD_WRITE�� ��
		return false;

	// g_SendQ �� ���� �����Ͱ� �ִ��� Ȯ��
	if (g_SendQ.GetUseSize() <= 0)
		return true;

	g_bSendFlag = TRUE;
	// g_SendQ �� ���� �����Ͱ� ���� �� ���� �Ǵ� Send �� ������ �� ���� ��� ����.
	// select ����� ���� �� �������� �˻� �� send �� �õ������� AsyncSelect �����
	// ������ ��ȭ�� �˷��ֱ� ������ ���� �� ���� ���°� �� ������ ���� ���� �� ������ ��
	while (1)
	{
		// g_SendQ �� �����Ͱ� �ִ��� Ȯ�� (������)
		if (g_SendQ.GetUseSize() <= 0)
			break;

		// g_SendQ ���� ������� �ӽù��۷� Peek, 
		g_SendQ.Peek(Buffer, g_SendQ.GetUseSize());

		// send �õ� (������� ����, ���� ������)
		int retval = send(g_Socket, Buffer, g_SendQ.GetUseSize(), 0);
		if (retval == SOCKET_ERROR)
		{
			// �����߻��� WSAEWOULDBLOCK �̶�� �� �̻� ������ ����, ������ �÷��� ��ȯ �� �ߴ�
			if (WSAGetLastError() != WSAEWOULDBLOCK)
			{
				g_bSendFlag = FALSE;
				break;
			}
		}

		// ������ ����Ʈ�� ���Դٸ� g_SendQ ���� ����.
		g_SendQ.MoveReadPos(retval);
	}
	return TRUE;
}

bool SendPacket(st_NETWORK_PACKET_HEADER  *pHeader, char *pPacket)
{
	// ���ӻ��� ����ó��
	g_bSendFlag = true;

	CSerialBuffer clPacket;
	clPacket.Clear();

	//clPacket << pHeader;
	//clPacket << pPacket; 
	if (clPacket.Enqueue((char*)pHeader, sizeof(st_NETWORK_PACKET_HEADER)) != sizeof(st_NETWORK_PACKET_HEADER))
		return false;

	if (clPacket.Enqueue((char*)pPacket, pHeader->bySize) != pHeader->bySize)
		return false;

	clPacket << dfNETWORK_PACKET_END;

	if (g_SendQ.Enqueue(clPacket.GetBufferPtr(), clPacket.GetUseSize()) != clPacket.GetUseSize())
		return false;

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

void mpMoveStart(st_NETWORK_PACKET_HEADER *pHeader, st_PACKET_CS_MOVE_START *pPacket, BYTE iDir, WORD iX, WORD iY)
{
	pHeader->byCode = dfNETWORK_PACKET_CODE;
	pHeader->bySize = 5;
	pHeader->byType = dfPACKET_CS_MOVE_START;

	pPacket->Direction = iDir;
	pPacket->X = iX;
	pPacket->Y = iY;
}

void mpMoveStop(st_NETWORK_PACKET_HEADER *pHeader, st_PACKET_CS_MOVE_STOP *pPacket, BYTE iDir, WORD iX, WORD iY)
{
	pHeader->byCode = dfNETWORK_PACKET_CODE;
	pHeader->bySize = 5;
	pHeader->byType = dfPACKET_CS_MOVE_STOP;

	pPacket->Direction = iDir;
	pPacket->X = iX;
	pPacket->Y = iY;
}

void mpAttack1(st_NETWORK_PACKET_HEADER *pHeader, st_PACKET_CS_ATTACK1 *pPacket, BYTE iDir, WORD iX, WORD iY)
{
	pHeader->byCode = dfNETWORK_PACKET_CODE;
	pHeader->bySize = 5;
	pHeader->byType = dfPACKET_CS_ATTACK1;

	pPacket->Direction = iDir;
	pPacket->X = iX;
	pPacket->Y = iY;
}

void mpAttack2(st_NETWORK_PACKET_HEADER *pHeader, st_PACKET_CS_ATTACK2 *pPacket, BYTE iDir, WORD iX, WORD iY)
{
	pHeader->byCode = dfNETWORK_PACKET_CODE;
	pHeader->bySize = 5;
	pHeader->byType = dfPACKET_CS_ATTACK2;

	pPacket->Direction = iDir;
	pPacket->X = iX;
	pPacket->Y = iY;
}

void mpAttack3(st_NETWORK_PACKET_HEADER *pHeader, st_PACKET_CS_ATTACK3 *pPacket, BYTE iDir, WORD iX, WORD iY)
{
	pHeader->byCode = dfNETWORK_PACKET_CODE;
	pHeader->bySize = 5;
	pHeader->byType = dfPACKET_CS_ATTACK3;

	pPacket->Direction = iDir;
	pPacket->X = iX;
	pPacket->Y = iY;
}

