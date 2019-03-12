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
	// TODO: WSAAsyncSelect()가 생성하는 메세지의 LPARAM 값
	//  WSAGETSELECTERROR(LPARAM) : LPARAM의 상위 16비트. 에러 코드 == HIWORD(LPARAM)
	//  WSAGETSELECTEVENT(LPARAM) : LPARAM의 하위 16비트. WSAAsyncSelect() 마지막 인자에서 지정한 이벤트 중 발생한 이벤트 == LOWORD(LPARAM)
	if (WSAGETSELECTERROR(lParam) != 0)
		return FALSE;

	switch (WSAGETSELECTEVENT(lParam))
	{
	case FD_CONNECT:
		g_bConnect = TRUE;
		return TRUE;
	case FD_READ:
	{
		// 클라의 경우, Read에서 반복문 넣어서 WOULDBLOCK 뜰 때까지 모두 처리해도 됨
		if (!RecvEvent())
			return FALSE;
		return TRUE;
	}
	case FD_WRITE: //처음 연결했을때, WOULDBLOCK이 뜨고 해제된 순간
		g_bSendFlag = TRUE;
		if (!SendEvent())
			return FALSE;
		return TRUE;
	case FD_CLOSE:
		//연결 끊김. 대게 `연결이 끊어졌습니다` 메시지 띄우고 방치
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
	char Packet[CRingBuffer::en_BUFFER_DEFALUT];	// 임시 수신버퍼
	BYTE Endcode = 0;	// 패킷용 임시버퍼 모든 타입의 패킷을 처리할 수 있는 넉넉한 사이즈

	retval = recv(g_Socket, Packet, g_RecvQ.GetFreeSize(), 0);
	if (retval == 0)
		return false;	// recv 리턴값으로 종료 (0) / 에러 (SOCKET_ERROR) 처리

	if (retval == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
			return false;
	}
	if (g_RecvQ.GetFreeSize() <= 0)
		return false;

	// 데이터를 받으면 일단 g_RecvQ에 보관
	g_RecvQ.Enqueue(Packet, retval);


	// * 완료패킷 처리 부 - g_RecvQ 에 들어있는 모든 완성패킷을 처리 해야 함.
	while (1)
	{
		// 1.	g_RecvQ 에 최소한의 사이즈가 있는지 확인. 조건 - 헤더사이즈 초과
		if (g_RecvQ.GetUseSize() < sizeof(st_NETWORK_PACKET_HEADER))
			break;

		// 2.	g_RecvQ 에서 헤더를 Peek
		retval = g_RecvQ.Peek((char*)&PacketHeader, sizeof(st_NETWORK_PACKET_HEADER));

		// 3.	헤더의 Code 확인
		if (PacketHeader.byCode != dfNETWORK_PACKET_CODE)
			return false;

		// 4.	헤더의 Len 값과 g_RecvQ 의 데이터 사이즈 비교 		-완성패킷 사이즈 : 헤더 + Len + 엔드코드 ////
		if (g_RecvQ.GetUseSize() < sizeof(PacketHeader) + PacketHeader.bySize + sizeof(dfNETWORK_PACKET_END))
			return false;

		// 5.	데이터 Peek 했던 헤더를 g_RecvQ 에서 지우고 (이미 빼두었으므로 또 뺄 필요 없음)
		g_RecvQ.MoveReadPos(retval); // Dequeue를 안 쓴 이유는 memcpy가 너무 무겁기 때문

									 // 6.	g_RecvQ 에서 Len 만큼 임시 패킷버퍼로 뽑음. 
		if (g_RecvQ.Dequeue(Packet, PacketHeader.bySize) == 0)
			return false;

		// 7.	g_RecvQ 에서 엔드코드인 1 Byte 또 뽑음
		retval = g_RecvQ.Dequeue((char*)&Endcode, sizeof(dfNETWORK_PACKET_END));
		if (retval == 0)
			return false;

		// 8.	엔드코드 확인 ////
		if (Endcode != dfNETWORK_PACKET_END)
			return false;

		// 9.	헤더의 타입에 따른 분기를 위해 패킷 프로시저 호출
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

	// 타격 유효 알림
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

	// HP 조절
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
	char Buffer[CRingBuffer::en_BUFFER_DEFALUT];	// 보내기용 임시 버퍼

	if (!g_bSendFlag)	// Send 가능여부 확인 / 이는 FD_WRITE 발생여부임 -> AsyncSelect에서는 connect 반응이 FD_WRITE로 옴
		return false;

	// g_SendQ 에 보낼 데이터가 있는지 확인
	if (g_SendQ.GetUseSize() <= 0)
		return true;

	g_bSendFlag = TRUE;
	// g_SendQ 에 남은 데이터가 없을 때 까지 또는 Send 가 실패할 때 까지 계속 보냄.
	// select 방식의 경우는 매 루프마다 검사 후 send 를 시도하지만 AsyncSelect 방식은
	// 상태의 변화만 알려주기 때문에 보낼 수 없는 상태가 될 때까지 보낼 것은 다 보내야 함
	while (1)
	{
		// g_SendQ 에 데이터가 있는지 확인 (루프용)
		if (g_SendQ.GetUseSize() <= 0)
			break;

		// g_SendQ 에서 보내기용 임시버퍼로 Peek, 
		g_SendQ.Peek(Buffer, g_SendQ.GetUseSize());

		// send 시도 (보내기용 버퍼, 보낼 사이즈)
		int retval = send(g_Socket, Buffer, g_SendQ.GetUseSize(), 0);
		if (retval == SOCKET_ERROR)
		{
			// 에러발생시 WSAEWOULDBLOCK 이라면 더 이상 보내지 못함, 보내기 플래그 전환 후 중단
			if (WSAGetLastError() != WSAEWOULDBLOCK)
			{
				g_bSendFlag = FALSE;
				break;
			}
		}

		// 보내진 바이트가 나왔다면 g_SendQ 에서 제거.
		g_SendQ.MoveReadPos(retval);
	}
	return TRUE;
}

bool SendPacket(st_NETWORK_PACKET_HEADER  *pHeader, char *pPacket)
{
	// 접속상태 예외처리
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

//  서버로 데이터 Send 시에는 항상 g_SendQ 에 먼저 데이터를 넣고, 
// 현재 Send 가 가능한 상태인지 확인 후 Send 를 호출하여 정상적으로 보내진 사이즈를 g_SendQ 에서 지우는 형식으로 진행.
//
//  패킷 데이터 생성은 각각의 패킷마다 생성 함수를 별도로 만들어서 패킷 데이터 생성코드를 관리한다.
//
//  패킷생성시 헤더와 페이로드가 필요하므로 이에 대한 포인터를 인자로 받아 생성 함수 내부에서 데이터를 셋팅하며, 
// 이에 필요한 정보들을 추가 인자로 받아서 패킷 데이터로 조합한다.
// 기타 필요정보는 최대한 외부 코드에 종속되지 않도록 개별적인 정보를 인자로 넘기는 것이 좋다.

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

