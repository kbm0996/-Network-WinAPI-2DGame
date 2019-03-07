#include "Contents.h"

DWORD g_dwLoopCnt;
std::map<DWORD, st_CHARACTER *>	g_CharacterMap;
std::list<st_CHARACTER *>		g_Sector[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];

bool CreateCharacter(st_SESSION * session)
{
	st_CHARACTER* pCharacter = new st_CHARACTER();

	pCharacter->pSession = session;
	pCharacter->dwSessionID = session->dwSessionID;
	pCharacter->byHP = (BYTE)100;
	pCharacter->shX = rand() % dfRANGE_MOVE_RIGHT;
	pCharacter->shY = rand() % dfRANGE_MOVE_BOTTOM;

	pCharacter->dwAction = dfACTION_STAND;
	pCharacter->dwActionTick = timeGetTime();
	pCharacter->byDirection = dfACTION_MOVE_LL;
	pCharacter->byMoveDirection = pCharacter->byDirection;
	pCharacter->shX = pCharacter->shX;
	pCharacter->shY = pCharacter->shY;

	pCharacter->CurSector.iX = pCharacter->OldSector.iX = -1;
	pCharacter->CurSector.iY = pCharacter->OldSector.iY = -1;

	pCharacter->pSession->dwLastRecvTick = timeGetTime();

	g_CharacterMap.emplace(session->dwSessionID, pCharacter);

	// 섹터에 삽입
	Sector_AddCharacter(pCharacter);


	// 해당 클라이언트에게 캐릭터 정보 전송
	mylib::CSerialBuffer Packet;
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
	mpCreateMyCharacter(&Packet, pCharacter->dwSessionID, pCharacter->byDirection, pCharacter->shX, pCharacter->shY, pCharacter->byHP);
	SendPacket_Unicast(session, &Packet);

	// 주변 정보 나한테 전송
	st_SECTOR_AROUND SectorAround;
	GetSectorAround(pCharacter->CurSector.iX, pCharacter->CurSector.iY, &SectorAround);
	for (int iCnt = 0; iCnt < SectorAround.iCount; iCnt++)
	{
		std::list<st_CHARACTER*> &SectorList = g_Sector[SectorAround.Around[iCnt].iY][SectorAround.Around[iCnt].iX];
		for (auto iter = SectorList.begin(); iter != SectorList.end(); ++iter)
		{
			if ((*iter)->dwSessionID == session->dwSessionID)
				continue;
			mpCreateOtherCharacter(&Packet, (*iter)->dwSessionID, (*iter)->byDirection, (*iter)->shX, (*iter)->shY, (*iter)->byHP);
			SendPacket_Unicast(session, &Packet);
		}
	}

	// 주변에 내 정보 전송
	mpCreateOtherCharacter(&Packet, pCharacter->dwSessionID, pCharacter->byDirection, pCharacter->shX, pCharacter->shY, pCharacter->byHP);
	SendPacket_Around(session, &Packet);


	return true;
}

st_CHARACTER * FindCharacter(DWORD dwSessionID)
{
	auto iter = g_CharacterMap.find(dwSessionID);
	if (g_CharacterMap.end() == iter)
		return nullptr;
	return iter->second;
}

bool DisconnectCharacter(st_SESSION * pSession)
{
	mylib::CSerialBuffer Packet;
	mpDeleteCharacter(&Packet, pSession->dwSessionID);
	SendPacket_Around(pSession, &Packet);

	// 세션정보에 있는 ID로 캐릭터정보를 찾음.
	st_CHARACTER *pCharacter = FindCharacter(pSession->dwSessionID);
	if (pCharacter == NULL)
		return false;

	// 섹터리스트와 캐릭터맵에서 캐릭터정보를 지운후 동적할당 해제
	Sector_RemoveCharacter(pCharacter);
	g_CharacterMap.erase(pCharacter->dwSessionID);
	delete pCharacter;
	return true;
}

void Update()
{
	//  게임 업데이트 처리를 매번 해주면 부하가 많이 일어남.
	// 가장 좋은 것은 클라이언트의 처리 주기와 동일하게 맞추는 것이지만 많은 부하가 발생
	// 적절하게 비슷한 속도로 돌아갈 수 있도록 맞춰줌.
	static DWORD dwFrameCnt = 0;
	static DWORD dwUpdateTick = timeGetTime();
	static DWORD dwSystemTick = timeGetTime();
	DWORD dwCurrentTick = timeGetTime();
	int iAccumFrameTick = dwCurrentTick - dwUpdateTick;
	int iFrame = iAccumFrameTick / 40;

	//  클라이언트는 50프레임으로 1초 = 1000ms /25 한프레임은 40ms임.
	// 현재틱 - 과거틱의 차가 40ms보다 작으면 업데이트 처리를 하지않음.
	if (iAccumFrameTick < 40)
		return;

	dwUpdateTick = dwCurrentTick;
	++dwFrameCnt;

	if (dwCurrentTick - dwSystemTick >= 1000)
	{
		if (dwFrameCnt != dfFRAME_MAX)
		{
			wprintf(L"Frame : %d, Loop : %d\n", dwFrameCnt, g_dwLoopCnt);
		}

		g_dwLoopCnt = 0;

		// 프레임드롭
		if (dwFrameCnt < dfFRAME_MAX)
		{
			// 로직을 바로 더 돌려서 따라잡아야 함
			Update();
		}
		// 프레임오버
		else if (dwFrameCnt > dfFRAME_MAX)
		{
			return;
		}

		//PrintSectorInfo();	/// 섹터 상황 출력
		dwSystemTick = dwCurrentTick;
		dwFrameCnt = 0;
	}
	/// ... 적절한 게임 업데이트 처리 타이밍 계산 ..


	/////////////////////////////////////////////////////////////////////////////

	// 게임 업데이트 처리를 매번 해준다면 쓸데없는 부하가 일어남
	// 가장 좋은 것은 클라이언트의 처리 주기와 똑같이 맞추는 것이지만 많은 부하가 예상될 수 있으므로
	// 적절하게 클라이언트와 최대한 비슷한 속도로 달아갈 수 있게끔 해주면 됨
	for (auto iter = g_CharacterMap.begin(); iter != g_CharacterMap.end();)
	{
		st_CHARACTER * pCharacter = iter->second;
		++iter;

		// TODO: 사망 처리
		//  콘텐츠가 다양해질 경우, 공격받고 사망하는 것 이외에 함정에 빠졌다던가 등의 다양한 원인으로 사망할 수 있음
		// 따라서, '사망의 조건'을 설정해 한꺼번에 처리하는 것이 좋다.
		//if (pCharacter->byHP <= 0)
		//{
		//	DisconnectSession(pCharacter->pSession->Sock);
		//}
		//else
		{
			// TODO: 잠수 처리
			//일정 시간동안 수신이 없으면 종료 처리
			//if (dwCurrentTick - pCharacter->pSession->dwLastRecvTick > dfNETWORK_PACKET_RECV_TIMEOUT)
			//{
			//	DisconnectSession(pCharacter->pSession->Sock);
			//	continue;
			//}

			switch (pCharacter->dwAction)
			{
			case dfACTION_MOVE_LL:
				if (CharacterMoveCheck(pCharacter->shX - dfSPEED_PLAYER_X,
					pCharacter->shY))
				{
					pCharacter->shX -= dfSPEED_PLAYER_X;
				}
				break;
			case dfACTION_MOVE_LU:
				if (CharacterMoveCheck(pCharacter->shX - dfSPEED_PLAYER_X,
					pCharacter->shY - dfSPEED_PLAYER_Y))
				{
					pCharacter->shX -= dfSPEED_PLAYER_X;
					pCharacter->shY -= dfSPEED_PLAYER_Y;
				}
				break;
			case dfACTION_MOVE_UU:
				if (CharacterMoveCheck(pCharacter->shX,
					pCharacter->shY - dfSPEED_PLAYER_Y))
				{
					pCharacter->shY -= dfSPEED_PLAYER_Y;
				}
				break;
			case dfACTION_MOVE_RU:
				if (CharacterMoveCheck(pCharacter->shX + dfSPEED_PLAYER_X,
					pCharacter->shY - dfSPEED_PLAYER_Y))
				{
					pCharacter->shX += dfSPEED_PLAYER_X;
					pCharacter->shY -= dfSPEED_PLAYER_Y;
				}
				break;
			case dfACTION_MOVE_RR:
				if (CharacterMoveCheck(pCharacter->shX + dfSPEED_PLAYER_X,
					pCharacter->shY))
				{
					pCharacter->shX += dfSPEED_PLAYER_X;
				}
				break;
			case dfACTION_MOVE_RD:
				if (CharacterMoveCheck(pCharacter->shX + dfSPEED_PLAYER_X,
					pCharacter->shY + dfSPEED_PLAYER_Y))
				{
					pCharacter->shX += dfSPEED_PLAYER_X;
					pCharacter->shY += dfSPEED_PLAYER_Y;
				}
				break;
			case dfACTION_MOVE_DD:
				if (CharacterMoveCheck(pCharacter->shX,
					pCharacter->shY + dfSPEED_PLAYER_Y))
				{
					pCharacter->shY += dfSPEED_PLAYER_Y;
				}
				break;
			case dfACTION_MOVE_LD:
				if (CharacterMoveCheck(pCharacter->shX - dfSPEED_PLAYER_X,
					pCharacter->shY + dfSPEED_PLAYER_Y))
				{
					pCharacter->shX -= dfSPEED_PLAYER_X;
					pCharacter->shY += dfSPEED_PLAYER_Y;
				}
				break;
			}

			// 이동인 경우 섹터 업데이트를 함
			if (pCharacter->dwAction >= dfACTION_MOVE_LL && pCharacter->dwAction <= dfACTION_MOVE_LD)
			{
				// 섹터가 변경된 경우 클라에게 관련 정보를 쏜다
				if (Sector_UpdateCharacter(pCharacter))
				{
					CharacterSectorUpdatePacket(pCharacter);
				}
			}
		}
	}
}

bool CharacterMoveCheck(SHORT shX, SHORT shY)
{
	if (shX > dfRANGE_MOVE_LEFT && shX < dfRANGE_MOVE_RIGHT &&
		shY > dfRANGE_MOVE_TOP && shY < dfRANGE_MOVE_BOTTOM)
		return true;
	return false;
}

int DeadReckoningPos(DWORD dwAction, DWORD dwActionTick, int iOldPosX, int iOldPosY, int * pPosX, int * pPosY)
{
	// 시간차를 구해서 몇 프레임이 지났는지 계산
	DWORD dwIntervalTick = timeGetTime() - dwActionTick;

	int iActionFrame = dwIntervalTick / 20;
	int iRemoveFrame = 0;

	int iValue;

	int iRPosX = iOldPosX;
	int iRPosY = iOldPosY;

	// 1. 계산된 프레임으로 x축, y축 좌표 이동값 구하기
	int iDX = iActionFrame * dfRECKONING_SPEED_PLAYER_X;
	int iDY = iActionFrame * dfRECKONING_SPEED_PLAYER_Y;

	switch (dwAction)
	{
	case dfACTION_MOVE_LL:
		iRPosX = iOldPosX - iDX;
		iRPosY = iOldPosY;
		break;
	case dfACTION_MOVE_LU:
		iRPosX = iOldPosX - iDX;
		iRPosY = iOldPosY - iDY;
		break;
	case dfACTION_MOVE_UU:
		iRPosX = iOldPosX;
		iRPosY = iOldPosY - iDY;
		break;
	case dfACTION_MOVE_RU:
		iRPosX = iOldPosX + iDX;
		iRPosY = iOldPosY - iDY;
		break;
	case dfACTION_MOVE_RR:
		iRPosX = iOldPosX + iDX;
		iRPosY = iOldPosY;
		break;
	case dfACTION_MOVE_RD:
		iRPosX = iOldPosX + iDX;
		iRPosY = iOldPosY + iDY;
		break;
	case dfACTION_MOVE_DD:
		iRPosX = iOldPosX;
		iRPosY = iOldPosY + iDY;
		break;
	case dfACTION_MOVE_LD:
		iRPosX = iOldPosX - iDX;
		iRPosY = iOldPosY + iDY;
		break;
	}
	// 여기까지 - iRPosX, iRPosY에 계산된 좌표 완료

	// 이후로는 - 계산된 좌표가 화면의 이동 영역을 벗어난 경우,
	//           그 액션을 잘라내기 위해 벗어난 이후의 프레임을 계산하는 과정
	if (iRPosX <= dfRANGE_MOVE_LEFT)
	{
		iValue = abs(dfRANGE_MOVE_LEFT - abs(iRPosX)) / dfRECKONING_SPEED_PLAYER_X;
		iRemoveFrame = max(iValue, iRemoveFrame);
	}
	if (iRPosX >= dfRANGE_MOVE_RIGHT)
	{
		iValue = abs(dfRANGE_MOVE_RIGHT - iRPosX) / dfRECKONING_SPEED_PLAYER_X;
		iRemoveFrame = max(iValue, iRemoveFrame);
	}
	if (iRPosY <= dfRANGE_MOVE_TOP)
	{
		iValue = abs(dfRANGE_MOVE_TOP - abs(iRPosY)) / dfRECKONING_SPEED_PLAYER_Y;
		iRemoveFrame = max(iValue, iRemoveFrame);
	}
	if (iRPosY >= dfRANGE_MOVE_BOTTOM)
	{
		iValue = abs(dfRANGE_MOVE_BOTTOM - iRPosY) / dfRECKONING_SPEED_PLAYER_Y;
		iRemoveFrame = max(iValue, iRemoveFrame);
	}

	// 삭제되어야 할 프레임이 나타났다면 좌표 재계산
	if (iRemoveFrame > 0)
	{
		iActionFrame -= iRemoveFrame;
		// 보정된 좌표로 다시 계산
		iDX = iActionFrame * dfRECKONING_SPEED_PLAYER_X;
		iDY = iActionFrame * dfRECKONING_SPEED_PLAYER_Y;

		switch (dwAction)
		{
		case dfACTION_MOVE_LL:
			iRPosX = iOldPosX - iDX;
			iRPosY = iOldPosY;
			break;
		case dfACTION_MOVE_LU:
			iRPosX = iOldPosX - iDX;
			iRPosY = iOldPosY - iDY;
			break;
		case dfACTION_MOVE_UU:
			iRPosX = iOldPosX;
			iRPosY = iOldPosY - iDY;
			break;
		case dfACTION_MOVE_RU:
			iRPosX = iOldPosX + iDX;
			iRPosY = iOldPosY - iDY;
			break;
		case dfACTION_MOVE_RR:
			iRPosX = iOldPosX + iDX;
			iRPosY = iOldPosY;
			break;
		case dfACTION_MOVE_RD:
			iRPosX = iOldPosX + iDX;
			iRPosY = iOldPosY + iDY;
			break;
		case dfACTION_MOVE_DD:
			iRPosX = iOldPosX;
			iRPosY = iOldPosY + iDY;
			break;
		case dfACTION_MOVE_LD:
			iRPosX = iOldPosX - iDX;
			iRPosY = iOldPosY + iDY;
			break;
		}
	}

	iRPosX = min(iRPosX, dfRANGE_MOVE_RIGHT);
	iRPosX = max(iRPosX, dfRANGE_MOVE_LEFT);
	iRPosY = min(iRPosY, dfRANGE_MOVE_BOTTOM);
	iRPosY = max(iRPosY, dfRANGE_MOVE_TOP);

	*pPosX = iRPosX;
	*pPosY = iRPosY;

	return iActionFrame;
}

void GetSectorAround(int iSectorX, int iSectorY, st_SECTOR_AROUND * pSectorAround)
{
	int iCntX, iCntY;

	iSectorX--;
	iSectorY--;

	pSectorAround->iCount = 0;

	for (iCntY = 0; iCntY < 3; iCntY++)
	{
		if (iSectorY + iCntY < 0 || iSectorY + iCntY >= dfSECTOR_MAX_Y)
			continue;

		for (iCntX = 0; iCntX < 3; iCntX++)
		{
			if (iSectorX + iCntX < 0 || iSectorX + iCntX >= dfSECTOR_MAX_X)
				continue;

			pSectorAround->Around[pSectorAround->iCount].iX = iSectorX + iCntX;
			pSectorAround->Around[pSectorAround->iCount].iY = iSectorY + iCntY;
			pSectorAround->iCount++;
		}
	}
}

void GetUpdateSectorAround(st_CHARACTER * pCharacter, st_SECTOR_AROUND * pRemoveSector, st_SECTOR_AROUND * pAddSector)
{
	int iCntOld, iCntCur;
	bool bFind;
	st_SECTOR_AROUND OldSectorAround, CurSectorAround;

	OldSectorAround.iCount = 0;
	CurSectorAround.iCount = 0;

	pRemoveSector->iCount = 0;
	pAddSector->iCount = 0;

	GetSectorAround(pCharacter->OldSector.iX, pCharacter->OldSector.iY, &OldSectorAround);
	GetSectorAround(pCharacter->CurSector.iX, pCharacter->CurSector.iY, &CurSectorAround);

	// OldSector 중 AddSector에 없는 정보를 찾아 RemoveSector에 넣음
	for (iCntOld = 0; iCntOld < OldSectorAround.iCount; iCntOld++)
	{
		bFind = false;
		for (iCntCur = 0; iCntCur < CurSectorAround.iCount; iCntCur++)
		{
			if (OldSectorAround.Around[iCntOld].iX == CurSectorAround.Around[iCntCur].iX &&
				OldSectorAround.Around[iCntOld].iY == CurSectorAround.Around[iCntCur].iY)
			{
				bFind = true;
				break;
			}
		}
		if (bFind == false)
		{
			pRemoveSector->Around[pRemoveSector->iCount] = OldSectorAround.Around[iCntOld];
			pRemoveSector->iCount++;
		}
	}
	// CurSector 중 OldSector에 없는 정보를 찾아서 AddSector에 넣음
	for (iCntCur = 0; iCntCur < CurSectorAround.iCount; iCntCur++)
	{
		bFind = false;
		for (iCntOld = 0; iCntOld < OldSectorAround.iCount; iCntOld++)
		{
			if (OldSectorAround.Around[iCntOld].iX == CurSectorAround.Around[iCntCur].iX &&
				OldSectorAround.Around[iCntOld].iY == CurSectorAround.Around[iCntCur].iY)
			{
				bFind = true;
				break;
			}
		}

		if (bFind == false)
		{
			pAddSector->Around[pAddSector->iCount] = CurSectorAround.Around[iCntCur];
			pAddSector->iCount++;
		}
	}
}

bool Sector_UpdateCharacter(st_CHARACTER * pCharacter)
{
	int iBeforeSectorX = pCharacter->CurSector.iX;
	int iBeforeSectorY = pCharacter->CurSector.iY;

	int iNewSectorX = pCharacter->shX / dfSECTOR_PIXEL_WIDTH;
	int iNewSectorY = pCharacter->shY / dfSECTOR_PIXEL_HEIGHT;

	if (iBeforeSectorX == iNewSectorX && iBeforeSectorY == iNewSectorY)
	{
		return false;
	}

	Sector_RemoveCharacter(pCharacter);
	Sector_AddCharacter(pCharacter);

	pCharacter->OldSector.iX = iBeforeSectorX;
	pCharacter->OldSector.iY = iBeforeSectorY;

	return true;
}

void Sector_RemoveCharacter(st_CHARACTER * pCharacter)
{
	// 캐릭터에 섹터 정보가 없다면 에러
	if (pCharacter->CurSector.iX == -1 || pCharacter->CurSector.iY == -1)
	{
		//OUTPUT_DEBUG(L"# REMOVE CHARACTER - INVALID CURRENT SECTOR # SessionID:%d / X:%d / Y:%d / Sector[%d, %d]\n",
		//			pCharacter->dwSessionID, pCharacter->shX, pCharacter->shY, pCharacter->CurSector.iX, pCharacter->CurSector.iY);
		return;
	}

	std::list<st_CHARACTER*> &SectorList = g_Sector[pCharacter->CurSector.iY][pCharacter->CurSector.iX];
	std::list<st_CHARACTER*>::iterator iter = SectorList.begin();

	// 섹터 리스트에서 해당 캐릭터 정보 제거
	while (iter != SectorList.end())
	{
		if (pCharacter == *iter)
		{
			SectorList.erase(iter);
			break;
		}
		++iter;
	}

	pCharacter->OldSector.iX = pCharacter->CurSector.iX;
	pCharacter->OldSector.iY = pCharacter->CurSector.iY;
	pCharacter->CurSector.iX = -1;
	pCharacter->CurSector.iY = -1;
}

void Sector_AddCharacter(st_CHARACTER * pCharacter)
{
	// 신규 추가는 기존에 들어간 적이 없는 캐릭터만 가능
	if (pCharacter->CurSector.iX != -1 || pCharacter->CurSector.iY != -1)
	{
		//OUTPUT_DEBUG(L"# ADD CHARACTER - INVALID CURRENT SECTOR # SessionID:%d / X:%d / Y:%d / Sector[%d, %d]\n", 
		//			pCharacter->dwSessionID, pCharacter->shX, pCharacter->shY, pCharacter->CurSector.iX, pCharacter->CurSector.iY);
		return;
	}

	int iSectorX = pCharacter->shX / dfSECTOR_PIXEL_WIDTH;
	int iSectorY = pCharacter->shY / dfSECTOR_PIXEL_HEIGHT;

	if (iSectorX >= dfSECTOR_MAX_X || iSectorY >= dfSECTOR_MAX_Y || iSectorX < 0 || iSectorY < 0)
	{
		return;
	}

	pCharacter->OldSector.iX = pCharacter->CurSector.iX = iSectorX;
	pCharacter->OldSector.iY = pCharacter->CurSector.iY = iSectorY;
	g_Sector[iSectorY][iSectorX].push_back(pCharacter);
}

void CharacterSectorUpdatePacket(st_CHARACTER * pCharacter)
{
	// Sector_UpdateCharacter와 한 쌍
	// es)
	// if(Sector_UpdateCharacter(pCharacter))
	//		CharacterSectorUpdatePacket(pCharacter);

	////////////////////////////////////////////////////////////////////////////////////
	// 1. RemoveSector에 캐릭터 삭제 패킷 전송: 
	//   OldSector에서 사라진 섹터의 캐릭터들 - 대상 캐릭터 삭제 메시지
	//
	// 2. 이동한 캐릭터에게 RemoveSector의 캐릭터 삭제 패킷 전송 :
	//   대상 캐릭터 -  OldSector에서 시라진 섹터의 캐릭터 삭제 메시지
	//
	// 3. AddSector에 캐릭터 생성 패킷 전송 + AddSector에 생성된 캐릭터 이동 패킷 전송: 
	//   CurSector에서 추가된 섹터의 캐릭터들 - 대상 캐릭터 생성 메시지 & 이동 메시지
	//
	// 4. 이동한 캐릭터에게 AddSector에 있는 캐릭터 생성 패킷 전송 : 
	//   대상 캐릭터 - CurSector에서 추가된 섹터의 캐릭터들 생성 메시지
	//
	////////////////////////////////////////////////////////////////////////////////////

	st_SECTOR_AROUND RemoveSector, AddSector;
	st_CHARACTER *pExistCharacter;

	std::list<st_CHARACTER *> *pSectorList;
	std::list<st_CHARACTER *>::iterator ListIter;

	mylib::CSerialBuffer Packet;
	int iCnt;

	GetUpdateSectorAround(pCharacter, &RemoveSector, &AddSector);

	// 1. RemoveSector에 캐릭터 삭제 패킷 전송
	mpDeleteCharacter(&Packet, pCharacter->dwSessionID);
	for (iCnt = 0; iCnt < RemoveSector.iCount; iCnt++)
	{
		SendPacket_SectorOne(RemoveSector.Around[iCnt].iX,
			RemoveSector.Around[iCnt].iY,
			&Packet, pCharacter->pSession);
	}

	// 2. 이동하고있는 캐릭터에게 RemoveSector의 캐릭터 삭제 패킷 전송
	for (iCnt = 0; iCnt < RemoveSector.iCount; iCnt++)
	{
		pSectorList = &g_Sector[RemoveSector.Around[iCnt].iY][RemoveSector.Around[iCnt].iX];

		for (ListIter = pSectorList->begin(); ListIter != pSectorList->end(); List++iter)
		{
			mpDeleteCharacter(&Packet, (*ListIter)->dwSessionID);

			SendPacket_Unicast(pCharacter->pSession, &Packet);
		}
	}

	// 3. AddSector에 캐릭터 생성 패킷 전송
	mpCreateOtherCharacter(&Packet, pCharacter->dwSessionID, pCharacter->byDirection, pCharacter->shX, pCharacter->shY, pCharacter->byHP);
	for (iCnt = 0; iCnt < AddSector.iCount; ++iCnt)
	{
		SendPacket_SectorOne(AddSector.Around[iCnt].iX, AddSector.Around[iCnt].iY, &Packet, NULL);
	}
	// 3-1. AddSector에 생성된 캐릭터 이동 패킷 전송
	mpMoveStart(&Packet, pCharacter->dwSessionID, pCharacter->byMoveDirection, pCharacter->shX, pCharacter->shY);
	for (iCnt = 0; iCnt < AddSector.iCount; ++iCnt)
	{
		SendPacket_SectorOne(AddSector.Around[iCnt].iX, AddSector.Around[iCnt].iY, &Packet, NULL);
	}

	// 4. 이동한 캐릭터에게 AddSector에 있는 캐릭터 생성 패킷 전송
	for (iCnt = 0; iCnt < AddSector.iCount; ++iCnt)
	{
		pSectorList = &g_Sector[AddSector.Around[iCnt].iY][AddSector.Around[iCnt].iX];
		for (ListIter = pSectorList->begin(); ListIter != pSectorList->end(); List++iter)
		{
			pExistCharacter = *ListIter;
			// 이동한 캐릭터의 세션은 전송 대상이 아님
			if (pExistCharacter != pCharacter)
			{
				mpCreateOtherCharacter(&Packet, pExistCharacter->dwSessionID, pExistCharacter->byDirection, pExistCharacter->shX, pExistCharacter->shY, pExistCharacter->byHP);
				SendPacket_Unicast(pCharacter->pSession, &Packet);

				// AddSector의 캐릭터가 걷고 있었다면 이동 패킷 전송
				switch (pExistCharacter->dwAction)
				{
				case dfACTION_MOVE_DD:
				case dfACTION_MOVE_LD:
				case dfACTION_MOVE_LL:
				case dfACTION_MOVE_LU:
				case dfACTION_MOVE_UU:
				case dfACTION_MOVE_RU:
				case dfACTION_MOVE_RR:
				case dfACTION_MOVE_RD:
					mpMoveStart(&Packet, pExistCharacter->dwSessionID, pExistCharacter->byMoveDirection, pExistCharacter->shX, pExistCharacter->shY);
					SendPacket_Unicast(pCharacter->pSession, &Packet);
					break;
				case dfACTION_ATTACK1:
					mpAttack1(&Packet, pExistCharacter->dwSessionID, pExistCharacter->byDirection, pExistCharacter->shX, pExistCharacter->shY);
					SendPacket_Unicast(pCharacter->pSession, &Packet);
					break;
				case dfACTION_ATTACK2:
					mpAttack2(&Packet, pExistCharacter->dwSessionID, pExistCharacter->byDirection, pExistCharacter->shX, pExistCharacter->shY);
					SendPacket_Unicast(pCharacter->pSession, &Packet);
					break;
				case dfACTION_ATTACK3:
					mpAttack3(&Packet, pExistCharacter->dwSessionID, pExistCharacter->byDirection, pExistCharacter->shX, pExistCharacter->shY);
					SendPacket_Unicast(pCharacter->pSession, &Packet);
					break;
				}
			}
		}
	}
}

// TODO: Monitor Sector
void PrintSectorInfo()
{
	int iCntX, iCntY;
	for (iCntY = 0; iCntY < dfSECTOR_MAX_Y; ++iCntY)
	{
		for (iCntX = 0; iCntX < dfSECTOR_MAX_X; ++iCntX)
		{
			std::list<st_CHARACTER*> &SectorList = g_Sector[iCntY][iCntX];

			if (SectorList.empty())
				continue;

			wprintf(L"Sector[%d,%d][%zd] - ", iCntX, iCntY, SectorList.size());

			for (auto iter = SectorList.begin(); iter != SectorList.end(); ++iter)
			{
				wprintf(L"%d ", (*iter)->dwSessionID);
			}
			wprintf(L"\n");
		}
	}
}