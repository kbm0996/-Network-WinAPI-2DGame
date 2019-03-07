#include "stdafx.h"
#include "CSpriteDib.h"

CSpriteDib g_cSprite(eSPRITE_MAX, 0x00ffffff);

CSpriteDib::CSpriteDib(int iMaxSprite, DWORD dwColorKey)
{
	_iMaxSprite = iMaxSprite;
	_dwColorKey = dwColorKey;

	_stpSprite = new st_SPRITE[_iMaxSprite];
	memset(_stpSprite, 0, sizeof(st_SPRITE) * _iMaxSprite);
}

CSpriteDib::~CSpriteDib()
{
	for (int iCnt = 0; iCnt > _iMaxSprite; ++iCnt)
	{
		ReleaseSprite(iCnt);
	}
}

BOOL CSpriteDib::LoadDibSprite(int iSpriteIndex, WCHAR* szFileName, int iCenterPointX, int iCenterPointY)
{
	HANDLE		hFile;
	DWORD		dwRead;
	int			iPitch;
	int			iImageSize;
	int			iCnt;

	BITMAPFILEHEADER stFileHeader;
	BITMAPINFOHEADER stInfoHeader;

	hFile = CreateFile(szFileName, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		return FALSE;
	}

	ReleaseSprite(iSpriteIndex);
	ReadFile(hFile, &stFileHeader, sizeof(BITMAPFILEHEADER), &dwRead, NULL);
	if (stFileHeader.bfType == 0x4d42)
	{
		ReadFile(hFile, &stInfoHeader, sizeof(BITMAPINFOHEADER), &dwRead, NULL);
		if (stInfoHeader.biBitCount == 32)
		{
			iPitch = (stInfoHeader.biWidth * 4) + 3 & ~3;

			_stpSprite[iSpriteIndex].iWidth = stInfoHeader.biWidth;
			_stpSprite[iSpriteIndex].iHeight = stInfoHeader.biHeight;
			_stpSprite[iSpriteIndex].iPitch = iPitch;

			iImageSize = iPitch * stInfoHeader.biHeight;
			_stpSprite[iSpriteIndex].bypImage = new BYTE[iImageSize];

			BYTE *bypTempBuffer = new BYTE[iImageSize];
			BYTE *bypSpriteTemp = _stpSprite[iSpriteIndex].bypImage;
			BYTE *bypTurnTemp;

			ReadFile(hFile, bypTempBuffer, iImageSize, &dwRead, NULL);

			bypTurnTemp = bypTempBuffer + iPitch * (stInfoHeader.biHeight - 1);

			for (iCnt = 0; iCnt < stInfoHeader.biHeight; ++iCnt)
			{
				memcpy_s(bypSpriteTemp, iImageSize, bypTurnTemp, iPitch);
				bypSpriteTemp += iPitch;
				bypTurnTemp -= iPitch;
			}
			delete[] bypTempBuffer;

			_stpSprite[iSpriteIndex].iCenterPointX = iCenterPointX;
			_stpSprite[iSpriteIndex].iCenterPointY = iCenterPointY;
			CloseHandle(hFile);
			return TRUE;
		}
	}
	CloseHandle(hFile);
	return FALSE;
}

void CSpriteDib::ReleaseSprite(int iSpriteIndex)
{
	if (_iMaxSprite <= iSpriteIndex)
		return;

	if (_stpSprite[iSpriteIndex].bypImage != NULL)
	{
		delete[] _stpSprite[iSpriteIndex].bypImage;
		memset(&_stpSprite[iSpriteIndex], 0, sizeof(st_SPRITE));
	}
}

void CSpriteDib::DrawSprite(int iSpriteIndex, int iDrawX, int iDrawY, BYTE * bypDest, int iDestWidth, int iDestHeight, int iDestPitch, int iDrawLen)
{
	if (iSpriteIndex >= _iMaxSprite)	return;

	if (_stpSprite[iSpriteIndex].bypImage == NULL)
		return;

	st_SPRITE *stpSprite = &_stpSprite[iSpriteIndex];

	int iSpriteWidth = stpSprite->iWidth;
	int iSpriteHeight = stpSprite->iHeight;
	int iCntX, iCntY;

	iSpriteWidth = iSpriteWidth * iDrawLen / 100;
	DWORD *dwpDest = (DWORD *)bypDest;
	DWORD *dwpSprite = (DWORD *)(stpSprite->bypImage);

	iDrawX = iDrawX - stpSprite->iCenterPointX;
	iDrawY = iDrawY - stpSprite->iCenterPointY;

	if (0 > iDrawY)
	{
		iSpriteHeight = iSpriteHeight - (-iDrawY);
		dwpSprite = (DWORD *)(stpSprite->bypImage + stpSprite->iPitch * (-iDrawY));
		iDrawY = 0;
	}

	if (iDestHeight <= iDrawY + stpSprite->iHeight)
	{
		iSpriteHeight -= ((iDrawY + stpSprite->iHeight) - iDestHeight);
	}

	if (0 > iDrawX)
	{
		iSpriteWidth = iSpriteWidth - (-iDrawX);
		dwpSprite = dwpSprite + (-iDrawX);
		iDrawX = 0;
	}

	if (iDestWidth <= iDrawX + stpSprite->iWidth)
	{
		iSpriteWidth -= ((iDrawX + stpSprite->iWidth) - iDestWidth);
	}

	if (iSpriteWidth <= 0 || iSpriteHeight <= 0)
		return;

	dwpDest = (DWORD *)(((BYTE *)(dwpDest + iDrawX) + (iDrawY * iDestPitch)));

	BYTE *bypDestOrigin = (BYTE *)dwpDest;
	BYTE *bypSpriteOrigin = (BYTE *)dwpSprite;

	for (iCntY = 0; iSpriteHeight > iCntY; iCntY++)
	{
		for (iCntX = 0; iSpriteWidth > iCntX; iCntX++)
		{
			if (_dwColorKey != (*dwpSprite & 0x00ffffff))
			{
				*dwpDest = *dwpSprite;
			}

			++dwpDest;
			++dwpSprite;
		}


		bypDestOrigin = bypDestOrigin + iDestPitch;
		bypSpriteOrigin = bypSpriteOrigin + stpSprite->iPitch;

		dwpDest = (DWORD *)bypDestOrigin;
		dwpSprite = (DWORD *)bypSpriteOrigin;
	}
}

void CSpriteDib::DrawSpriteRed(int iSpriteIndex, int iDrawX, int iDrawY, BYTE * bypDest, int iDestWidth, int iDestHeight, int iDestPitch, int iDrawLen)
{
	if (iSpriteIndex >= _iMaxSprite)	return;

	if (_stpSprite[iSpriteIndex].bypImage == NULL)	return;

	st_SPRITE *stpSprite = &_stpSprite[iSpriteIndex];

	int iSpriteWidth = stpSprite->iWidth;
	int iSpriteHeight = stpSprite->iHeight;
	int iCntX, iCntY;

	iSpriteWidth = iSpriteWidth * iDrawLen / 100;
	DWORD *dwpDest = (DWORD *)bypDest;
	DWORD *dwpSprite = (DWORD *)(stpSprite->bypImage);

	iDrawX = iDrawX - stpSprite->iCenterPointX;
	iDrawY = iDrawY - stpSprite->iCenterPointY;

	if (0 > iDrawY)
	{
		iSpriteHeight = iSpriteHeight - (-iDrawY);
		dwpSprite = (DWORD *)(stpSprite->bypImage + stpSprite->iPitch * (-iDrawY));
		iDrawY = 0;
	}

	if (iDestHeight <= iDrawY + stpSprite->iHeight)
	{
		iSpriteHeight -= ((iDrawY + stpSprite->iHeight) - iDestHeight);
	}

	if (0 > iDrawX)
	{
		iSpriteWidth = iSpriteWidth - (-iDrawX);
		dwpSprite = dwpSprite + (-iDrawX);
		iDrawX = 0;
	}

	if (iDestWidth <= iDrawX + stpSprite->iWidth)
	{
		iSpriteWidth -= ((iDrawX + stpSprite->iWidth) - iDestWidth);
	}

	if (iSpriteWidth <= 0 || iSpriteHeight <= 0)
		return;

	dwpDest = (DWORD *)(((BYTE *)(dwpDest + iDrawX) + (iDrawY * iDestPitch)));

	BYTE *bypDestOrigin = (BYTE *)dwpDest;
	BYTE *bypSpriteOrigin = (BYTE *)dwpSprite;

	for (iCntY = 0; iSpriteHeight > iCntY; ++iCntY)
	{
		for (iCntX = 0; iSpriteWidth > iCntX; ++iCntX)
		{
			if (_dwColorKey != (*dwpSprite & 0x00ffffff))
			{
				*dwpDest = (*dwpSprite & 0x00ff0000);
			}

			++dwpDest;
			++dwpSprite;
		}


		bypDestOrigin = bypDestOrigin + iDestPitch;
		bypSpriteOrigin = bypSpriteOrigin + stpSprite->iPitch;

		dwpDest = (DWORD *)bypDestOrigin;
		dwpSprite = (DWORD *)bypSpriteOrigin;
	}
}

void CSpriteDib::DrawSprite50(int iSpriteIndex, int iDrawX, int iDrawY, BYTE * bypDest, int iDestWidth, int iDestHeight, int iDestPitch, int iDrawLen)
{
	if (iSpriteIndex >= _iMaxSprite)	return;

	if (_stpSprite[iSpriteIndex].bypImage == NULL)	return;

	st_SPRITE *stpSprite = &_stpSprite[iSpriteIndex];

	int iSpriteWidth = stpSprite->iWidth;
	int iSpriteHeight = stpSprite->iHeight;
	int iCntX, iCntY;

	iSpriteWidth = iSpriteWidth * iDrawLen / 100;
	DWORD *dwpDest = (DWORD *)bypDest;
	DWORD *dwpSprite = (DWORD *)(stpSprite->bypImage);
	DWORD *dwpScreen = (DWORD *)g_cScreenDib.GetDibBuffer();

	iDrawX = iDrawX - stpSprite->iCenterPointX;
	iDrawY = iDrawY - stpSprite->iCenterPointY;

	if (0 > iDrawY)
	{
		iSpriteHeight = iSpriteHeight - (-iDrawY);
		dwpSprite = (DWORD *)(stpSprite->bypImage + stpSprite->iPitch * (-iDrawY));
		iDrawY = 0;
	}

	if (iDestHeight <= iDrawY + stpSprite->iHeight)
	{
		iSpriteHeight -= ((iDrawY + stpSprite->iHeight) - iDestHeight);
	}

	if (0 > iDrawX)
	{
		iSpriteWidth = iSpriteWidth - (-iDrawX);
		dwpSprite = dwpSprite + (-iDrawX);
		iDrawX = 0;
	}

	if (iDestWidth <= iDrawX + stpSprite->iWidth)
	{
		iSpriteWidth -= ((iDrawX + stpSprite->iWidth) - iDestWidth);
	}

	if (iSpriteWidth <= 0 || iSpriteHeight <= 0)
		return;

	dwpDest = (DWORD *)(((BYTE *)(dwpDest + iDrawX) + (iDrawY * iDestPitch)));
	dwpScreen = (DWORD *)(((BYTE *)(dwpScreen + iDrawX) + (iDrawY * iDestPitch)));

	BYTE *bypDestOrigin = (BYTE *)dwpDest;
	BYTE *bypSpriteOrigin = (BYTE *)dwpSprite;
	BYTE *bypScreenOrigin = (BYTE *)dwpScreen;

	for (iCntY = 0; iSpriteHeight > iCntY; ++iCntY)
	{
		for (iCntX = 0; iSpriteWidth > iCntX; ++iCntX)
		{
			if (_dwColorKey != (*dwpSprite & 0x00ffffff))
			{
				//*dwpDest = (*dwpDest >> 1) + *dwpSprite >> 1 & 0x7f7f7f7f;	// 이미지 rgb 추출하지 않고 Alpha 표현
				*dwpDest = ((*dwpSprite & 0x007f7f7f) + (*dwpScreen & 0x007f7f7f));
			}

			++dwpDest;
			++dwpSprite;
			++dwpScreen;
		}

		bypDestOrigin = bypDestOrigin + iDestPitch;
		bypSpriteOrigin = bypSpriteOrigin + stpSprite->iPitch;
		bypScreenOrigin = bypScreenOrigin + g_cScreenDib.GetPitch();

		dwpDest = (DWORD *)bypDestOrigin;
		dwpSprite = (DWORD *)bypSpriteOrigin;
		dwpScreen = (DWORD *)bypScreenOrigin;
	}
}

void CSpriteDib::DrawImage(int iSpriteIndex, int iDrawX, int iDrawY, BYTE * bypDest, int iDestWidth, int iDestHeight, int iDestPitch, int iDrawLen)
{
	if (iSpriteIndex >= _iMaxSprite)	return;

	if (_stpSprite[iSpriteIndex].bypImage == NULL)	return;

	st_SPRITE *stpSprite = &_stpSprite[iSpriteIndex];

	int iSpriteWidth = stpSprite->iWidth;
	int iSpriteHeight = stpSprite->iHeight;
	int iCntY;

	iSpriteWidth = iSpriteWidth * iDrawLen / 100;

	DWORD *dwpDest = (DWORD *)bypDest;
	DWORD *dwpSprite = (DWORD *)(stpSprite->bypImage);

	if (0 > iDrawY)
	{
		iSpriteHeight = iSpriteHeight - (-iDrawY);
		dwpSprite = (DWORD *)(stpSprite->bypImage + stpSprite->iPitch * (-iDrawY));
		iDrawY = 0;
	}

	if (iDestHeight <= iDrawY + stpSprite->iHeight)
	{
		iSpriteHeight -= ((iDrawY + stpSprite->iHeight) - iDestHeight);
	}

	if (0 > iDrawX)
	{
		iSpriteWidth = iSpriteWidth - (-iDrawX);
		dwpSprite = dwpSprite + (-iDrawX);
		iDrawX = 0;
	}

	if (iDestWidth <= iDrawX + stpSprite->iWidth)
	{
		iSpriteWidth -= ((iDrawX + stpSprite->iWidth) - iDestWidth);
	}

	if (iSpriteWidth <= 0 || iSpriteHeight <= 0)
		return;

	dwpDest = (DWORD *)(((BYTE *)(dwpDest + iDrawX) + (iDrawY * iDestPitch)));

	for (iCntY = 0; iSpriteHeight > iCntY; ++iCntY)
	{
		memcpy(dwpDest, dwpSprite, iSpriteWidth * 4);
		dwpDest = (DWORD *)((BYTE *)dwpDest + iDestPitch);
		dwpSprite = (DWORD *)((BYTE *)dwpSprite + stpSprite->iPitch);
	}
}
