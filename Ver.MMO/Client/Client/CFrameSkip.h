#ifndef __FRAMESKIP_H__
#define __FRAMESKIP_H__
class CFrameSkip
{
public:
	CFrameSkip(int iMaxFPS = 50);
	~CFrameSkip();
	BOOL FrameSkip(HWND hWnd);

private:
	DWORD	_dwSystemTick;
	INT64	_iMaxFPS;
	INT64	_iOneFrameTick;
	INT64	_iTick;
};

#endif