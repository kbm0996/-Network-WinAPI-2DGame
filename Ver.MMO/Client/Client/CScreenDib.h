#ifndef __SCREEN_DIB__
#define __SCREEN_DIB__
class CScreenDib
{
public:
	CScreenDib(int iWidth, int iHeight, int iColorBit);
	virtual ~CScreenDib();

protected:
	void CreateDibBuffer(int iWidth, int iHeight, int iColorBit);
	void ReleaseDibBuffer();

public:
	void DrawBuffer(HWND hWnd, int iX = 0, int iY = 0);

	BYTE* GetDibBuffer();
	int GetWidth();
	int GetHeight();
	int GetPitch(void);

protected:
	BITMAPINFOHEADER	_stDibInfo;
	BYTE*				_bypBuffer;

	int					_iWidth;
	int					_iHeight;
	int					_iPitch;
	int					_iColorBit;
	int					_iBufferSize;
};

extern CScreenDib g_cScreenDib;

#endif