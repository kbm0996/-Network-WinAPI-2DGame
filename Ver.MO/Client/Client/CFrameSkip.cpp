#include "stdafx.h"
#include "CFrameSkip.h"
#include <timeapi.h>

CFrameSkip* g_Frame;

CFrameSkip::CFrameSkip() :_dwSystemTick(0), _iMaxFPS(50), _iOneFrameTick(0), _iTick(0)
{
}

CFrameSkip::~CFrameSkip()
{
}

BOOL CFrameSkip::FrameSkip(HWND hWnd)
{
	DWORD dwCurTime;
	DWORD dwElapTime;
	WCHAR szFPS[10];
	int iFPS;

	dwCurTime = timeGetTime();

	_iTick++;
	dwElapTime = dwCurTime - _dwSystemTick;
	iFPS = (int)(_iTick * 1000.0 / dwElapTime);


	if (dwElapTime >= 1000)
	{
		_itow_s(iFPS, szFPS, 10);
		SetWindowTextW(hWnd, szFPS);

		_iTick = 0;
		_dwSystemTick = dwCurTime;
	}
	else
	{
		if (iFPS < _iMaxFPS)
			return TRUE;

		Sleep(0.02f * 1000);
	}
	return FALSE;
}
