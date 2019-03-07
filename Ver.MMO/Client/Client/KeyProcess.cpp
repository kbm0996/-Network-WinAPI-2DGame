#include "stdafx.h"
#include "KeyProcess.h"

bool KeyProcess(CPlayer* pPlayer)	// if문으로 구성할 경우 키를 많이 눌러야하는 로직을 뒤로, if-else문으로 구성할 경우 위로
{
	if (pPlayer != NULL)
	{
		DWORD dwAction;
		dwAction = dfACTION_STAND;

		if (GetAsyncKeyState(VK_RETURN) & 0x8000)
		{
			dwAction = dfACTION_PUSH;
		}

		if (GetAsyncKeyState(0x44) & 0x8000) // D
		{
			dwAction = dfACTION_ATTACK3;
		}

		if (GetAsyncKeyState(0x53) & 0x8000) // S
		{
			dwAction = dfACTION_ATTACK2;
		}

		if (GetAsyncKeyState(0x41) & 0x8000) // A
		{
			dwAction = dfACTION_ATTACK1;
		}

		if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
		{
			dwAction = dfACTION_MOVE_RR;
		}

		if (GetAsyncKeyState(VK_LEFT) & 0x8000)
		{
			dwAction = dfACTION_MOVE_LL;
		}

		if (GetAsyncKeyState(VK_UP) & 0x8000)
		{
			dwAction = dfACTION_MOVE_UU;
		}

		if (GetAsyncKeyState(VK_DOWN) & 0x8000)
		{
			dwAction = dfACTION_MOVE_DD;
		}

		if (GetAsyncKeyState(VK_DOWN) && GetAsyncKeyState(VK_RIGHT) & 0x8000)
		{
			dwAction = dfACTION_MOVE_RD;
		}

		if (GetAsyncKeyState(VK_DOWN) && GetAsyncKeyState(VK_LEFT) & 0x8000)
		{
			dwAction = dfACTION_MOVE_LD;
		}

		if (GetAsyncKeyState(VK_UP) && GetAsyncKeyState(VK_RIGHT) & 0x8000)
		{
			dwAction = dfACTION_MOVE_RU;
		}

		if (GetAsyncKeyState(VK_UP) && GetAsyncKeyState(VK_LEFT) & 0x8000)
		{
			dwAction = dfACTION_MOVE_LU;
		}

		if (GetAsyncKeyState(VK_UP) && GetAsyncKeyState(VK_LEFT) & 0x8000)
		{
			dwAction = dfACTION_MOVE_LU;
		}

		pPlayer->ActionInput(dwAction);
		return true;
	}
	else
		return false;
}
