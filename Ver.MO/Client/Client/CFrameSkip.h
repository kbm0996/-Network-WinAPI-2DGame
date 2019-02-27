#ifndef __FRAMESKIP_H__
#define __FRAMESKIP_H__
class CFrameSkip
{
public:
	CFrameSkip();
	~CFrameSkip();
	BOOL FrameSkip(HWND hWnd);

private:
	int		_iMaxFPS;
	DWORD	_dwSystemTick;

	int		_iOneFrameTick;
	int		_iTick;
};

extern CFrameSkip* g_Frame;

#endif