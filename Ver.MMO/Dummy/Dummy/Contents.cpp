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
	//  게임 업데이트 처리를 매번 해주면 부하가 많이 일어남.
	// 가장 좋은 것은 클라이언트의 처리 주기와 동일하게 맞추는 것이지만 많은 부하가 발생
	// 적절하게 비슷한 속도로 돌아갈 수 있도록 맞춰줌.
	static DWORD dwFrameCnt = 0;
	static DWORD dwUpdateTick = timeGetTime();
	static DWORD dwSystemTick = timeGetTime();
	DWORD dwCurrentTick = timeGetTime();
	int iAccumFrameTick = dwCurrentTick - dwUpdateTick;
	int iFrame = iAccumFrameTick / 40;

	
	++g_dwLoopCnt;
	
	
	//  클라이언트는 50프레임으로 1초 = 1000ms /25 한프레임은 40ms임.
	// 현재틱 - 과거틱의 차가 40ms보다 작으면 업데이트 처리를 하지않음.
	if (iAccumFrameTick < 40)
		return;

	dwUpdateTick = dwCurrentTick;
	++dwFrameCnt;

	if (dwCurrentTick - dwSystemTick >= 1000)
	{
		Monitoring();


		//// 프레임드롭
		//if (dwFrameCnt < dfFRAME_MAX)
		//{
		//	// 로직을 바로 더 돌려서 따라잡아야 함
		//	Update();
		//}
		//// 프레임오버
		//else if (dwFrameCnt > dfFRAME_MAX)
		//{
		//	return;
		//}

		//PrintSectorInfo();	/// 섹터 상황 출력
		dwSystemTick = dwCurrentTick;
		dwFrameCnt = 0;
	}
	/// ... 적절한 게임 업데이트 처리 타이밍 계산 ..


	/////////////////////////////////////////////////////////////////////////////

	// 게임 업데이트 처리를 매번 해준다면 쓸데없는 부하가 일어남
	// 가장 좋은 것은 클라이언트의 처리 주기와 똑같이 맞추는 것이지만 많은 부하가 예상될 수 있으므로
	// 적절하게 클라이언트와 최대한 비슷한 속도로 달아갈 수 있게끔 해주면 됨
	int iAiProcCnt = 0;
	for (auto iter = g_CharacterMap.begin(); iter != g_CharacterMap.end();)
	{
		st_CHARACTER * pCharacter = iter->second;
		++iter;

		// 행동 주기
		if (timeGetTime() - pCharacter->dwActionTick < rand() % 1000 + 3000)
			continue;

		// Update() 루프 한 번에 움직일 수 있는 AI 수
		/// iter를 저장하여 1~20, 21~40 .. 순서대로 움직이게 하는 것은 위험. 도중에 iter가 사라질 수 있기 때문.
		/// 이런식으로 구현해도 행동 주기가 있기 때문에 모든 Ai가 움직이게 되어있음 
		if (iAiProcCnt > 20)
			break;

		++iAiProcCnt;

		// RTT 체크를 위한 Echo 전송
		/// echo를 많이 보내는 것은 그다지 의미가 없음. 부하만 커짐.
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

	// 이동 방향 설정
	/// 맵의 모서리에 더미들이 몰리는 현상을 방지
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

	// 보는 방향 설정
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
