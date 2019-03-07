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

	// ���Ϳ� ����
	Sector_AddCharacter(pCharacter);


	// �ش� Ŭ���̾�Ʈ���� ĳ���� ���� ����
	mylib::CSerialBuffer Packet;
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
	mpCreateMyCharacter(&Packet, pCharacter->dwSessionID, pCharacter->byDirection, pCharacter->shX, pCharacter->shY, pCharacter->byHP);
	SendPacket_Unicast(session, &Packet);

	// �ֺ� ���� ������ ����
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

	// �ֺ��� �� ���� ����
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

	// ���������� �ִ� ID�� ĳ���������� ã��.
	st_CHARACTER *pCharacter = FindCharacter(pSession->dwSessionID);
	if (pCharacter == NULL)
		return false;

	// ���͸���Ʈ�� ĳ���͸ʿ��� ĳ���������� ������ �����Ҵ� ����
	Sector_RemoveCharacter(pCharacter);
	g_CharacterMap.erase(pCharacter->dwSessionID);
	delete pCharacter;
	return true;
}

void Update()
{
	//  ���� ������Ʈ ó���� �Ź� ���ָ� ���ϰ� ���� �Ͼ.
	// ���� ���� ���� Ŭ���̾�Ʈ�� ó�� �ֱ�� �����ϰ� ���ߴ� �������� ���� ���ϰ� �߻�
	// �����ϰ� ����� �ӵ��� ���ư� �� �ֵ��� ������.
	static DWORD dwFrameCnt = 0;
	static DWORD dwUpdateTick = timeGetTime();
	static DWORD dwSystemTick = timeGetTime();
	DWORD dwCurrentTick = timeGetTime();
	int iAccumFrameTick = dwCurrentTick - dwUpdateTick;
	int iFrame = iAccumFrameTick / 40;

	//  Ŭ���̾�Ʈ�� 50���������� 1�� = 1000ms /25 ���������� 40ms��.
	// ����ƽ - ����ƽ�� ���� 40ms���� ������ ������Ʈ ó���� ��������.
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

		// �����ӵ��
		if (dwFrameCnt < dfFRAME_MAX)
		{
			// ������ �ٷ� �� ������ ������ƾ� ��
			Update();
		}
		// �����ӿ���
		else if (dwFrameCnt > dfFRAME_MAX)
		{
			return;
		}

		//PrintSectorInfo();	/// ���� ��Ȳ ���
		dwSystemTick = dwCurrentTick;
		dwFrameCnt = 0;
	}
	/// ... ������ ���� ������Ʈ ó�� Ÿ�̹� ��� ..


	/////////////////////////////////////////////////////////////////////////////

	// ���� ������Ʈ ó���� �Ź� ���شٸ� �������� ���ϰ� �Ͼ
	// ���� ���� ���� Ŭ���̾�Ʈ�� ó�� �ֱ�� �Ȱ��� ���ߴ� �������� ���� ���ϰ� ����� �� �����Ƿ�
	// �����ϰ� Ŭ���̾�Ʈ�� �ִ��� ����� �ӵ��� �޾ư� �� �ְԲ� ���ָ� ��
	for (auto iter = g_CharacterMap.begin(); iter != g_CharacterMap.end();)
	{
		st_CHARACTER * pCharacter = iter->second;
		++iter;

		// TODO: ��� ó��
		//  �������� �پ����� ���, ���ݹް� ����ϴ� �� �̿ܿ� ������ �����ٴ��� ���� �پ��� �������� ����� �� ����
		// ����, '����� ����'�� ������ �Ѳ����� ó���ϴ� ���� ����.
		//if (pCharacter->byHP <= 0)
		//{
		//	DisconnectSession(pCharacter->pSession->Sock);
		//}
		//else
		{
			// TODO: ��� ó��
			//���� �ð����� ������ ������ ���� ó��
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

			// �̵��� ��� ���� ������Ʈ�� ��
			if (pCharacter->dwAction >= dfACTION_MOVE_LL && pCharacter->dwAction <= dfACTION_MOVE_LD)
			{
				// ���Ͱ� ����� ��� Ŭ�󿡰� ���� ������ ���
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
	// �ð����� ���ؼ� �� �������� �������� ���
	DWORD dwIntervalTick = timeGetTime() - dwActionTick;

	int iActionFrame = dwIntervalTick / 20;
	int iRemoveFrame = 0;

	int iValue;

	int iRPosX = iOldPosX;
	int iRPosY = iOldPosY;

	// 1. ���� ���������� x��, y�� ��ǥ �̵��� ���ϱ�
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
	// ������� - iRPosX, iRPosY�� ���� ��ǥ �Ϸ�

	// ���ķδ� - ���� ��ǥ�� ȭ���� �̵� ������ ��� ���,
	//           �� �׼��� �߶󳻱� ���� ��� ������ �������� ����ϴ� ����
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

	// �����Ǿ�� �� �������� ��Ÿ���ٸ� ��ǥ ����
	if (iRemoveFrame > 0)
	{
		iActionFrame -= iRemoveFrame;
		// ������ ��ǥ�� �ٽ� ���
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

	// OldSector �� AddSector�� ���� ������ ã�� RemoveSector�� ����
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
	// CurSector �� OldSector�� ���� ������ ã�Ƽ� AddSector�� ����
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
	// ĳ���Ϳ� ���� ������ ���ٸ� ����
	if (pCharacter->CurSector.iX == -1 || pCharacter->CurSector.iY == -1)
	{
		//OUTPUT_DEBUG(L"# REMOVE CHARACTER - INVALID CURRENT SECTOR # SessionID:%d / X:%d / Y:%d / Sector[%d, %d]\n",
		//			pCharacter->dwSessionID, pCharacter->shX, pCharacter->shY, pCharacter->CurSector.iX, pCharacter->CurSector.iY);
		return;
	}

	std::list<st_CHARACTER*> &SectorList = g_Sector[pCharacter->CurSector.iY][pCharacter->CurSector.iX];
	std::list<st_CHARACTER*>::iterator iter = SectorList.begin();

	// ���� ����Ʈ���� �ش� ĳ���� ���� ����
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
	// �ű� �߰��� ������ �� ���� ���� ĳ���͸� ����
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
	// Sector_UpdateCharacter�� �� ��
	// es)
	// if(Sector_UpdateCharacter(pCharacter))
	//		CharacterSectorUpdatePacket(pCharacter);

	////////////////////////////////////////////////////////////////////////////////////
	// 1. RemoveSector�� ĳ���� ���� ��Ŷ ����: 
	//   OldSector���� ����� ������ ĳ���͵� - ��� ĳ���� ���� �޽���
	//
	// 2. �̵��� ĳ���Ϳ��� RemoveSector�� ĳ���� ���� ��Ŷ ���� :
	//   ��� ĳ���� -  OldSector���� �ö��� ������ ĳ���� ���� �޽���
	//
	// 3. AddSector�� ĳ���� ���� ��Ŷ ���� + AddSector�� ������ ĳ���� �̵� ��Ŷ ����: 
	//   CurSector���� �߰��� ������ ĳ���͵� - ��� ĳ���� ���� �޽��� & �̵� �޽���
	//
	// 4. �̵��� ĳ���Ϳ��� AddSector�� �ִ� ĳ���� ���� ��Ŷ ���� : 
	//   ��� ĳ���� - CurSector���� �߰��� ������ ĳ���͵� ���� �޽���
	//
	////////////////////////////////////////////////////////////////////////////////////

	st_SECTOR_AROUND RemoveSector, AddSector;
	st_CHARACTER *pExistCharacter;

	std::list<st_CHARACTER *> *pSectorList;
	std::list<st_CHARACTER *>::iterator ListIter;

	mylib::CSerialBuffer Packet;
	int iCnt;

	GetUpdateSectorAround(pCharacter, &RemoveSector, &AddSector);

	// 1. RemoveSector�� ĳ���� ���� ��Ŷ ����
	mpDeleteCharacter(&Packet, pCharacter->dwSessionID);
	for (iCnt = 0; iCnt < RemoveSector.iCount; iCnt++)
	{
		SendPacket_SectorOne(RemoveSector.Around[iCnt].iX,
			RemoveSector.Around[iCnt].iY,
			&Packet, pCharacter->pSession);
	}

	// 2. �̵��ϰ��ִ� ĳ���Ϳ��� RemoveSector�� ĳ���� ���� ��Ŷ ����
	for (iCnt = 0; iCnt < RemoveSector.iCount; iCnt++)
	{
		pSectorList = &g_Sector[RemoveSector.Around[iCnt].iY][RemoveSector.Around[iCnt].iX];

		for (ListIter = pSectorList->begin(); ListIter != pSectorList->end(); List++iter)
		{
			mpDeleteCharacter(&Packet, (*ListIter)->dwSessionID);

			SendPacket_Unicast(pCharacter->pSession, &Packet);
		}
	}

	// 3. AddSector�� ĳ���� ���� ��Ŷ ����
	mpCreateOtherCharacter(&Packet, pCharacter->dwSessionID, pCharacter->byDirection, pCharacter->shX, pCharacter->shY, pCharacter->byHP);
	for (iCnt = 0; iCnt < AddSector.iCount; ++iCnt)
	{
		SendPacket_SectorOne(AddSector.Around[iCnt].iX, AddSector.Around[iCnt].iY, &Packet, NULL);
	}
	// 3-1. AddSector�� ������ ĳ���� �̵� ��Ŷ ����
	mpMoveStart(&Packet, pCharacter->dwSessionID, pCharacter->byMoveDirection, pCharacter->shX, pCharacter->shY);
	for (iCnt = 0; iCnt < AddSector.iCount; ++iCnt)
	{
		SendPacket_SectorOne(AddSector.Around[iCnt].iX, AddSector.Around[iCnt].iY, &Packet, NULL);
	}

	// 4. �̵��� ĳ���Ϳ��� AddSector�� �ִ� ĳ���� ���� ��Ŷ ����
	for (iCnt = 0; iCnt < AddSector.iCount; ++iCnt)
	{
		pSectorList = &g_Sector[AddSector.Around[iCnt].iY][AddSector.Around[iCnt].iX];
		for (ListIter = pSectorList->begin(); ListIter != pSectorList->end(); List++iter)
		{
			pExistCharacter = *ListIter;
			// �̵��� ĳ������ ������ ���� ����� �ƴ�
			if (pExistCharacter != pCharacter)
			{
				mpCreateOtherCharacter(&Packet, pExistCharacter->dwSessionID, pExistCharacter->byDirection, pExistCharacter->shX, pExistCharacter->shY, pExistCharacter->byHP);
				SendPacket_Unicast(pCharacter->pSession, &Packet);

				// AddSector�� ĳ���Ͱ� �Ȱ� �־��ٸ� �̵� ��Ŷ ����
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