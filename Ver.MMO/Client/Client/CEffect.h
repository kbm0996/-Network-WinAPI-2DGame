#ifndef __EFFECT_OBJ__
#define __EFFECT_OBJ__

class CEffectObject : public CBaseObject
{
public:
	CEffectObject(int iX, int iY, __int64 Type = eTYPE_EFFECT, __int64 ID = 0)
	{
		_CurX = iX;
		_CurY = iY;
		_iObjectType = Type;
		_iObjectID = ID;
		_bEffectStart = TRUE;
		SetSprite(eEFFECT_SPARK_01, eEFFECT_SPARK_MAX, dfDELAY_EFFECT);
	}

	~CEffectObject() {}

	void Action()
	{
		if (_bEndFrame)
			return;

		if (_bEffectStart)
			NextFrame();
	}

	bool Draw(BYTE *bypDest, int iDestWidth, int iDestHeight, int iDestPitch)
	{
		int iScrollX = g_TileMap.GetScrollX();
		int iScrollY = g_TileMap.GetScrollY();

		g_cSprite.DrawSpriteRed(GetSprite(), _CurX - iScrollX, _CurY - iScrollY, bypDest, iDestWidth, iDestHeight, iDestPitch);
		return true;
	}

private:
	BOOL	_bEffectStart;
	DWORD	_dwAttackID;
};

#endif