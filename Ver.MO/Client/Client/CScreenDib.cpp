#include "stdafx.h"
#include "CScreenDib.h"

CScreenDib g_cScreenDib(dfSCREEN_WIDTH, dfSCREEN_HEIGHT, dfSCREEN_BIT);

CScreenDib::CScreenDib(int iWidth, int iHeight, int iColorBit)
{
	memset(&_stDibInfo, 0, sizeof(BITMAPINFO));
	_bypBuffer = nullptr;
	_iWidth = 0;
	_iHeight = 0;
	_iPitch = 0;
	_iBufferSize = 0;
	_iColorBit = 0;
	CreateDibBuffer(iWidth, iHeight, iColorBit);
}

CScreenDib::~CScreenDib()
{
	ReleaseDibBuffer();
}

void CScreenDib::CreateDibBuffer(int iWidth, int iHeight, int iColorBit)
{
	_iWidth = iWidth;
	_iHeight = iHeight;
	_iColorBit = iColorBit;
	_iPitch = ((_iWidth * (_iColorBit / 8)) + 3) & ~3;
	_iBufferSize = _iPitch * _iHeight;

	_stDibInfo.biSize = sizeof(BITMAPINFOHEADER);
	_stDibInfo.biWidth = _iWidth;
	_stDibInfo.biHeight = -_iHeight;
	_stDibInfo.biPlanes = 1;
	_stDibInfo.biBitCount = _iColorBit;
	_stDibInfo.biCompression = 0;
	_stDibInfo.biSizeImage = _stDibInfo.biSizeImage;
	_stDibInfo.biXPelsPerMeter = 0;
	_stDibInfo.biYPelsPerMeter = 0;
	_stDibInfo.biClrUsed = 0;
	_stDibInfo.biClrImportant = 0;

	_bypBuffer = new BYTE[_iBufferSize];
	memset(_bypBuffer, 0xff, _iBufferSize);
}

void CScreenDib::ReleaseDibBuffer()
{
	_iWidth = 0;
	_iHeight = 0;
	_iPitch = 0;
	_iBufferSize = 0;

	memset(&_stDibInfo, 0x00, sizeof(BITMAPINFO));
}

void CScreenDib::DrawBuffer(HWND hWnd, int iX, int iY)
{
	if (_bypBuffer == nullptr) 
		return;

	RECT crt;
	HDC hdc;

	GetClientRect(hWnd, &crt);
	hdc = GetDC(hWnd);

	//StretchDIBits(hdc, iX, iY, _iPitch, _iHeight, 0, 0, _iPitch, _iHeight, _bypBuffer, (LPBITMAPINFO)(&_stDibInfo), DIB_RGB_COLORS, SRCCOPY);
	SetDIBitsToDevice(hdc, iX, iY, _iWidth, _iHeight, 0, 0, 0, _iHeight, _bypBuffer, (LPBITMAPINFO)(&_stDibInfo), DIB_RGB_COLORS);
	ReleaseDC(hWnd, hdc);
}

BYTE * CScreenDib::GetDibBuffer()
{
	return _bypBuffer;
}

int CScreenDib::GetWidth()
{
	return _iWidth;
}

int CScreenDib::GetHeight()
{
	return _iHeight;
}

int CScreenDib::GetPitch(void)
{
	return _iPitch;
}
