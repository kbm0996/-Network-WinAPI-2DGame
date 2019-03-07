#ifndef __MAP_H__
#define __MAP_H__

class CTileMap
{
public:
	CTileMap();
	CTileMap(CTileMap &copy);
	CTileMap(int iWidth, int iHeight);
	virtual ~CTileMap();

	void	DrawTile(BYTE *bypDest, int iDestWidth, int iDestHeight, int iDestPitch);
	void	SetScrollPos(int iX, int iY);

	int		GetScrollX();
	int		GetScrollY();

	BYTE**	GetMap();
	int		GetMapHeight();
	int		GetMapWidth();
	BYTE	GetTile(int iX, int iY);

private:
	int		_iScrollX;
	int		_iScrollY;

	BYTE**	dpMap;
	int		_iMapHeightCnt;
	int		_iMapWidthCnt;
};

extern CTileMap g_TileMap;

#endif