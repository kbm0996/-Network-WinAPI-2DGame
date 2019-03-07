#include "stdafx.h"

CTileMap g_TileMap;

CTileMap::CTileMap()
{
	_iScrollX = _iScrollY = 0;
	_iMapHeightCnt = dfMAP_DEFAULT_SIZE / 64;
	_iMapWidthCnt = dfMAP_DEFAULT_SIZE / 64;

	dpMap = (BYTE**)malloc(sizeof(char**)*_iMapHeightCnt);
	memset(dpMap, 0, sizeof(char*)*_iMapHeightCnt);
	for (int i = 0; i < _iMapHeightCnt; ++i)
	{
		dpMap[i] = (BYTE*)malloc(sizeof(char*)*_iMapWidthCnt);
		memset(dpMap[i], 0, sizeof(char*)*_iMapWidthCnt);
	}
}

CTileMap::CTileMap(CTileMap & copy)
{
	_iScrollX = _iScrollY = 0;
	_iMapHeightCnt = copy._iMapHeightCnt;
	_iMapWidthCnt = copy._iMapWidthCnt;

	dpMap = (BYTE**)malloc(sizeof(char**)*copy._iMapHeightCnt);
	memcpy_s(dpMap, sizeof(copy.dpMap), copy.dpMap, sizeof(copy.dpMap));
	memcpy_s(dpMap, sizeof(char*)* copy._iMapHeightCnt, copy.dpMap, sizeof(char*)* copy._iMapHeightCnt);
	for (int i = 0; i < copy._iMapHeightCnt; ++i)
	{
		dpMap[i] = (BYTE*)malloc(sizeof(char*)*copy._iMapWidthCnt);
		memcpy_s(dpMap[i], sizeof(char*)*copy._iMapWidthCnt, copy.dpMap[i], sizeof(char*)*copy._iMapWidthCnt);
	}
}

CTileMap::CTileMap(int iWidth, int iHeight)
{
	_iScrollX = _iScrollY = 0;
	_iMapHeightCnt = iHeight;
	_iMapWidthCnt = iWidth;

	dpMap = (BYTE**)malloc(sizeof(char**)*iHeight);
	memset(dpMap, 0, sizeof(char*)*iHeight);
	for (int i = 0; i < iHeight; ++i)
	{
		dpMap[i] = (BYTE*)malloc(sizeof(char*)*iWidth);
		memset(dpMap[i], 0, sizeof(char*)*iWidth);
	}
}

CTileMap::~CTileMap()
{
	free(dpMap);
}


void CTileMap::DrawTile(BYTE *bypDest, int iDestWidth, int iDestHeight, int iDestPitch)
{
	int iRemainX, iRemainY;
	int iDrawX, iDrawY;

	iRemainX = GetScrollX() % dfTILE_SIZE;
	iRemainY = GetScrollY() % dfTILE_SIZE;
	iDrawX = -iRemainX;
	iDrawY = -iRemainY;

	for (int iCntV = 0; iCntV < dfTILE_DRAWVERT; ++iCntV)
	{
		for (int iCntH = 0; iCntH < dfTILE_DRAWHORZ; ++iCntH)
		{
			g_cSprite.DrawImage(eMAP_TILE, iDrawX + iCntH * dfTILE_SIZE, iDrawY + iCntV * dfTILE_SIZE, bypDest, iDestWidth, iDestHeight, iDestPitch);
		}
	}

}

void CTileMap::SetScrollPos(int iX, int iY)
{
	_iScrollX = iX;
	_iScrollY = iY;
}

int CTileMap::GetScrollX()
{
	if (_iScrollX <= 0)
		_iScrollX = 0;
	if (_iScrollX >= dfMAP_DEFAULT_WIDTH - dfSCREEN_WIDTH)
		_iScrollX = dfMAP_DEFAULT_WIDTH - dfSCREEN_WIDTH;
	return _iScrollX;
}

int CTileMap::GetScrollY()
{
	if (_iScrollY <= 0)
		_iScrollY = 0;
	if (_iScrollY >= dfMAP_DEFAULT_HEIGHT - dfSCREEN_HEIGHT)
		_iScrollY = dfMAP_DEFAULT_HEIGHT - dfSCREEN_HEIGHT;
	return _iScrollY;
}


BYTE ** CTileMap::GetMap()
{
	return dpMap;
}

int CTileMap::GetMapHeight()
{
	return _iMapHeightCnt;
}

int CTileMap::GetMapWidth()
{
	return _iMapWidthCnt;
}

BYTE CTileMap::GetTile(int iX, int iY)
{
	int iTileX = iX / dfTILE_SIZE;
	int iTileY = iY / dfTILE_SIZE;
	return dpMap[iTileX][iTileY];
}