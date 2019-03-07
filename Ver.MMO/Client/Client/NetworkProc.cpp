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

	if (!g_bSendFlag)	// Send 가능여부 확인 / 이는 FD_WRITE 발생여부임 -> AsyncSelect에서는 connect 반응이 FD_WRITE로 옴
		return FALSE;

	// g_SendQ에 있는 데이터들을 최대 dfNETWORK_WSABUFF_SIZE 크기로 보낸다
	int iSendSize = g_SendQ.GetUseSize();
	iSendSize = min(CRingBuffer::en_BUFFER_DEFALUT, iSendSize);
	// g_SendQ 에 보낼 데이터가 있는지 확인
	if (iSendSize <= 0)
		return TRUE;

	g_bSendFlag = TRUE;

	while (1)
	{
		// g_SendQ 에 데이터가 있는지 확인 (루프용)
		iSendSize = g_SendQ.GetUseSize();
		iSendSize = min(CRingBuffer::en_BUFFER_DEFALUT, iSendSize);
		if (iSendSize <= 0)
			break;

		// Peek으로 데이터 뽑음. 전송이 제대로 마무리됐을 경우 삭제
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
				// 보낼 사이즈보다 더 크다면 오류
				// 생기면 안되는 상황이지만 가끔 이런 경우가 생길 수 있다
				//-----------------------------------------------------
				return false;
			}
			else
			{
				//-----------------------------------------------------
				// Send Complate
				// 패킷 전송이 완료됐다는 의미는 아님. 소켓 버퍼에 복사를 완료했다는 의미
				// 송신 큐에서 Peek으로 빼냈던 데이터 제거
				//-----------------------------------------------------
				g_SendQ.MoveReadPos(iResult);
			}
		}
	}
	return true;
}

bool SendPacket(st_NETWORK_PACKET_HEADER  *pHeader, CSerialBuffer *pPacket)
{
	// 접속상태 예외처리
	g_bSendFlag = true;
	g_SendQ.Enqueue((char*)pHeader, sizeof(st_NETWORK_PACKET_HEADER));
	g_SendQ.Enqueue((char*)pPacket->GetBufferPtr(), pPacket->GetUseSize());
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

void mpMoveStart(st_NETWORK_PACKET_HEADER *pHeader, CSerialBuffer *pPacket, BYTE iDir, WORD iX, WORD iY)
{
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

		// 처리할 패킷이 없음
		if (iResult == 1)
			break;
		// 패킷 처리 오류
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
	// RecvQ에 Header길이만큼 있는지 확인
	int g_iRecvQSize = g_RecvQ.GetUseSize();
	if (g_iRecvQSize < sizeof(st_NETWORK_PACKET_HEADER))
		return 1;

	// Packet 길이 확인 (Header크기 + Payload길이 + EndCode 길이)
	g_RecvQ.Peek((char*)&stPacketHeader, sizeof(st_NETWORK_PACKET_HEADER));
	if ((WORD)g_iRecvQSize < sizeof(st_NETWORK_PACKET_HEADER) + stPacketHeader.bySize + sizeof(byEndcode))
		return 1;

	// 받은 패킷으로부터 Header 제거
	g_RecvQ.MoveReadPos(sizeof(st_NETWORK_PACKET_HEADER));

	///////////////////////////////////////////////////////
	// Recv Packet - Process
	//
	///////////////////////////////////////////////////////
	if (stPacketHeader.byCode != dfNETWORK_PACKET_CODE)
		return -1;

	CSerialBuffer Packet;
	// Payload 부분 패킷 버퍼로 복사
	if (!g_RecvQ.Dequeue(Packet.GetBufferPtr(), stPacketHeader.bySize))
		return -1;

	// Endcode 제거 및 확인
	g_RecvQ.Dequeue((char*)&byEndcode, 1);
	if (byEndcode != dfNETWORK_PACKET_END)
		return -1;
	
	Packet.MoveWritePos(stPacketHeader.bySize);

	// 실질적인 패킷 처리 함수 호출
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
	// 1. 다른 클라이언트의 캐릭터 생성 패킷		Server -> Client
	//
	// 처음 서버에 접속시 이미 접속되어 있던 캐릭터들의 정보
	// 또는 게임중에 접속된 클라이언트들의 생성 용 정보.
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
	// 2. 캐릭터 삭제 패킷						Server -> Client
	//
	// 캐릭터의 접속해제 또는 캐릭터가 죽었을때 전송됨.
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
	// 캐릭터 이동시작 패킷						Server -> Client
	//
	// 다른 유저의 캐릭터 이동시 본 패킷을 받는다.
	// 패킷 수신시 해당 캐릭터를 찾아 이동처리를 해주도록 한다.
	// 
	// 패킷 수신 시 해당 키가 계속해서 눌린것으로 생각하고
	// 해당 방향으로 계속 이동을 하고 있어야만 한다.
	//
	//	4	-	ID
	//	1	-	Direction	( 방향 디파인 값 8방향 )
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
	// 캐릭터 이동중지 패킷						Server -> Client
	//
	// ID 에 해당하는 캐릭터가 이동을 멈춘것이므로 
	// 캐릭터를 찾아서 방향과, 좌표를 입력해주고 멈추도록 처리한다.
	//
	//	4	-	ID
	//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
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
	// 캐릭터 공격 패킷							Server -> Client
	//
	// 패킷 수신시 해당 캐릭터를 찾아서 공격1번 동작으로 액션을 취해준다.
	// 방향이 다를 경우에는 해당 방향으로 바꾼 후 해준다.
	//
	//	4	-	ID
	//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
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
	// 캐릭터 공격 패킷							Server -> Client
	//
	// 패킷 수신시 해당 캐릭터를 찾아서 공격2번 동작으로 액션을 취해준다.
	// 방향이 다를 경우에는 해당 방향으로 바꾼 후 해준다.
	//
	//	4	-	ID
	//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
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
	// 캐릭터 공격 패킷							Server -> Client
	//
	// 패킷 수신시 해당 캐릭터를 찾아서 공격2번 동작으로 액션을 취해준다.
	// 방향이 다를 경우에는 해당 방향으로 바꾼 후 해준다.
	//
	//	4	-	ID
	//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
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
	// 캐릭터 데미지 패킷							Server -> Client
	//
	// 공격에 맞은 캐릭터의 정보를 보냄.
	//
	//	4	-	AttackID	( 공격자 ID )
	//	4	-	DamageID	( 피해자 ID )
	//	1	-	DamageHP	( 피해자 HP )
	//
	//---------------------------------------------------------------
	*pPacket >> ID1;
	*pPacket >> ID2;
	*pPacket >> ID2_HP;

	// 타격 유효 알림
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


	// HP 조절
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