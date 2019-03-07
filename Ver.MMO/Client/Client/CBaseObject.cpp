#include "stdafx.h"
#include "CBaseObject.h"

void CBaseObject::SetPosition(int iX, int iY)
{
	_CurX = iX;
	_CurY = iY;
}

void CBaseObject::GetPosition(int * OutX, int * OutY)
{
	OutX = &_CurX;
	OutY = &_CurY;
}

int CBaseObject::GetCurX()
{
	return _CurX;
}

int CBaseObject::GetCurY()
{
	return _CurY;
}

BOOL CBaseObject::IsEndFrame()
{
	return _bEndFrame;
}

void CBaseObject::NextFrame()
{
	if (_iSpriteStart < 0)
		return;

	++_iDelayCount;
	if (_iDelayCount >= _iFrameDelay)
	{
		_iDelayCount = 0;
		++_iSpriteNow;
		if (_iSpriteNow > _iSpriteEnd)
		{
			_iSpriteNow = _iSpriteStart;
			_bEndFrame = TRUE;
		}
	}
}

void CBaseObject::SetSprite(int iSpriteStart, int iSpriteEnd, int iFrameDelay)
{
	_iSpriteStart = iSpriteStart;
	_iSpriteEnd = iSpriteEnd;
	_iFrameDelay = iFrameDelay;

	_iSpriteNow = iSpriteStart;
	_iDelayCount = 0;
	_bEndFrame = FALSE;
}

int CBaseObject::GetSprite()
{
	return _iSpriteNow;
}

void CBaseObject::ActionInput(DWORD dwAction)
{
	_dwActionInput = dwAction;
}

void CBaseObject::SetObjectID(int ObjectID)
{
	_iObjectID = ObjectID;
}

int CBaseObject::GetObjectID()
{
	return _iObjectID;
}

void CBaseObject::SetObjectType(int ObjectType)
{
	_iObjectType = ObjectType;
}

int CBaseObject::GetObjectType()
{
	return _iObjectType;
}