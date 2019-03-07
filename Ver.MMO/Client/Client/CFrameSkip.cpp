#include "stdafx.h"
#include "CFrameSkip.h"
#include <timeapi.h>

CFrameSkip::CFrameSkip(int iMaxFPS) :_dwSystemTick(0), _iMaxFPS(iMaxFPS), _iOneFrameTick(0), _iTick(0)
{
}

CFrameSkip::~CFrameSkip()
{
}

BOOL CFrameSkip::FrameSkip(HWND hWnd)
{
	DWORD dwCurTime;
	int iFPS;

	++_iTick;
	dwCurTime = timeGetTime();
	_iOneFrameTick = dwCurTime - _dwSystemTick;
	iFPS = (int)(_iTick * 1000.0 / _iOneFrameTick);


	if (_iOneFrameTick >= 1000)
	{
		WCHAR szFPS[10];
		_itow_s(iFPS, szFPS, 10);
		SetWindowTextW(hWnd, szFPS);

		_iTick = 0;
		_dwSystemTick = dwCurTime;
	}
	else
	{
		if (iFPS < _iMaxFPS)
			return TRUE;

		Sleep(1000 / _iMaxFPS);
	}
	return FALSE;
}
