#include "Contents.h"

DWORD g_dwLoopCnt;
std::map<DWORD, st_CHARACTER *>	g_CharacterMap;

st_CHARACTER * FindCharacter(DWORD dwSessionID)
{
	auto iter = g_CharacterMap.find(dwSessionID);
	if (g_CharacterMap.end() == iter)
		return nullptr;
	return iter->second;
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

	
	++g_dwLoopCnt;
	
	
	//  Ŭ���̾�Ʈ�� 50���������� 1�� = 1000ms /25 ���������� 40ms��.
	// ����ƽ - ����ƽ�� ���� 40ms���� ������ ������Ʈ ó���� ��������.
	if (iAccumFrameTick < 40)
		return;

	dwUpdateTick = dwCurrentTick;
	++dwFrameCnt;

	if (dwCurrentTick - dwSystemTick >= 1000)
	{
		Monitoring();


		//// �����ӵ��
		//if (dwFrameCnt < dfFRAME_MAX)
		//{
		//	// ������ �ٷ� �� ������ ������ƾ� ��
		//	Update();
		//}
		//// �����ӿ���
		//else if (dwFrameCnt > dfFRAME_MAX)
		//{
		//	return;
		//}

		//PrintSectorInfo();	/// ���� ��Ȳ ���
		dwSystemTick = dwCurrentTick;
		dwFrameCnt = 0;
	}
	/// ... ������ ���� ������Ʈ ó�� Ÿ�̹� ��� ..


	/////////////////////////////////////////////////////////////////////////////

	// ���� ������Ʈ ó���� �Ź� ���شٸ� �������� ���ϰ� �Ͼ
	// ���� ���� ���� Ŭ���̾�Ʈ�� ó�� �ֱ�� �Ȱ��� ���ߴ� �������� ���� ���ϰ� ����� �� �����Ƿ�
	// �����ϰ� Ŭ���̾�Ʈ�� �ִ��� ����� �ӵ��� �޾ư� �� �ְԲ� ���ָ� ��
	int iAiProcCnt = 0;
	for (auto iter = g_CharacterMap.begin(); iter != g_CharacterMap.end();)
	{
		st_CHARACTER * pCharacter = iter->second;
		++iter;

		// �ൿ �ֱ�
		if (timeGetTime() - pCharacter->dwActionTick < rand() % 1000 + 3000)
			continue;

		// Update() ���� �� ���� ������ �� �ִ� AI ��
		/// iter�� �����Ͽ� 1~20, 21~40 .. ������� �����̰� �ϴ� ���� ����. ���߿� iter�� ����� �� �ֱ� ����.
		/// �̷������� �����ص� �ൿ �ֱⰡ �ֱ� ������ ��� Ai�� �����̰� �Ǿ����� 
		if (iAiProcCnt > 20)
			break;

		++iAiProcCnt;

		// RTT üũ�� ���� Echo ����
		/// echo�� ���� ������ ���� �״��� �ǹ̰� ����. ���ϸ� Ŀ��.
		if (iter == g_CharacterMap.begin())
			AI_Echo(pCharacter);

		if (pCharacter->dwAction >= dfACTION_MOVE_LL && pCharacter->dwAction <= dfACTION_MOVE_LD)
		{
			AI_Stop(pCharacter);
			continue;
		}

		switch (rand() % 6)
		{
		case 0:
			AI_Stop(pCharacter);
			break;
		case 1:
		case 2:
		case 3:
			AI_Move(pCharacter);
			break;
		case 4:
		case 5:
			AI_Attack(pCharacter);
			break;
		}
	}
}


void AI_Stop(st_CHARACTER * pCharacter)
{
	int idrX, idrY;
	int iDeadFrame = DeadReckoningPos(pCharacter->dwAction, pCharacter->dwActionTick,
		pCharacter->shX, pCharacter->shY, &idrX, &idrY);

	pCharacter->shX = idrX;
	pCharacter->shY = idrY;
	pCharacter->dwActionTick = timeGetTime();
	pCharacter->dwAction = dfACTION_STAND;

	mylib::CSerialBuffer clPacket;
	mpMoveStop(&clPacket, pCharacter->byDirection, pCharacter->shX, pCharacter->shY);
	SendPacket_Unicast(pCharacter->pSession, &clPacket);
}

void AI_Move(st_CHARACTER * pCharacter)
{
	int idrX, idrY;
	int iDeadFrame = DeadReckoningPos(pCharacter->dwAction, pCharacter->dwActionTick,
		pCharacter->shX, pCharacter->shY, &idrX, &idrY);

	// �̵� ���� ����
	/// ���� �𼭸��� ���̵��� ������ ������ ����
	if (dfRANGE_MOVE_LEFT + 100 > pCharacter->shX)
		pCharacter->dwAction = dfPACKET_MOVE_DIR_RR;
	else if (dfRANGE_MOVE_RIGHT - 100 < pCharacter->shX)
		pCharacter->dwAction = dfPACKET_MOVE_DIR_LL;
	else if (dfRANGE_MOVE_TOP + 100 > pCharacter->shY)
		pCharacter->dwAction = dfPACKET_MOVE_DIR_DD;
	else if (dfRANGE_MOVE_BOTTOM - 100 < pCharacter->shY)
		pCharacter->dwAction = dfPACKET_MOVE_DIR_UU;
	else
		pCharacter->dwAction = rand() % 8;

	pCharacter->byMoveDirection = pCharacter->dwAction;

	// ���� ���� ����
	switch (pCharacter->byMoveDirection)
	{
	case dfPACKET_MOVE_DIR_RR:
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RD:
		pCharacter->byDirection = dfPACKET_MOVE_DIR_RR;
		break;
	case dfPACKET_MOVE_DIR_LU:
	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LD:
		pCharacter->byDirection = dfPACKET_MOVE_DIR_LL;
		break;
	}

	pCharacter->shX = idrX;
	pCharacter->shY = idrY;
	pCharacter->dwActionTick = timeGetTime();

	mylib::CSerialBuffer clPacket;
	mpMoveStart(&clPacket, pCharacter->byMoveDirection, pCharacter->shX, pCharacter->shY);
	SendPacket_Unicast(pCharacter->pSession, &clPacket);
}

void AI_Attack(st_CHARACTER * pCharacter)
{
	int idrX, idrY;
	int iDeadFrame = DeadReckoningPos(pCharacter->dwAction, pCharacter->dwActionTick,
		pCharacter->shX, pCharacter->shY, &idrX, &idrY);
	pCharacter->shX = idrX;
	pCharacter->shY = idrY;
	pCharacter->dwActionTick = timeGetTime();

	mylib::CSerialBuffer clPacket;
	switch (rand() % 3)
	{
	case 0:
		pCharacter->dwAction = dfACTION_ATTACK1;
		mpAttack1(&clPacket, pCharacter->byDirection, pCharacter->shX, pCharacter->shY);
		break;
	case 1:
		pCharacter->dwAction = dfACTION_ATTACK2;
		mpAttack2(&clPacket, pCharacter->byDirection, pCharacter->shX, pCharacter->shY);
		break;
	case 2:
		pCharacter->dwAction = dfACTION_ATTACK3;
		mpAttack3(&clPacket, pCharacter->byDirection, pCharacter->shX, pCharacter->shY);
		break;
	}
	SendPacket_Unicast(pCharacter->pSession, &clPacket);
}

void AI_Echo(st_CHARACTER * pCharacter)
{
	mylib::CSerialBuffer clPacket;
	mpEcho(&clPacket, timeGetTime());
	SendPacket_Unicast(pCharacter->pSession, &clPacket);
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

void Monitoring()
{
	if (g_EchoCnt > 0 && g_RTTSum > 0)
		g_RTTAvr = g_RTTSum / g_EchoCnt;

	wprintf(L"Loop/sec : %d\n", g_dwLoopCnt);
	wprintf(L"Packet/sec : %d\n", g_PacketCnt);
	wprintf(L"Recv/sec : %d Bytes\n", g_RecvBytes);
	wprintf(L"Send/sec : %d Bytes\n", g_SendBytes);
	wprintf(L"RTT : Avr %dms / max %dms / EchoCnt %d\n", g_RTTAvr, g_RTTMax, g_EchoCnt);
	wprintf(L"Connect		Try : %lld / Success : %lld / Fail : %lld \n\n", g_ConnectTry, g_ConnectSuccess, g_ConnectFail);

	// TODO: (Log) Cnt Reset
	g_dwLoopCnt = 0;
	g_SendBytes = 0;
	g_RecvBytes = 0;
	g_PacketCnt = 0;

	g_EchoCnt = 0;
	g_RTTMax = 0;
}
