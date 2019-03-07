#ifndef __BASE_OBJ__
#define __BASE_OBJ__

class CBaseObject
{
public:
	CBaseObject() {}
	virtual ~CBaseObject() {}

	virtual void Action() = 0;
	virtual bool Draw(BYTE *bypDest, int iDestWidth, int iDestHeight, int iDestPitch) = 0;
	void		 ActionInput(DWORD dwAction);

	void		 SetObjectID(int ObjectID);
	int			 GetObjectID();

	void		 SetObjectType(int ObjectType);
	int			 GetObjectType();

	virtual void SetHP(int iHP) {}
	virtual void SetDirection(int iDir) {}
	virtual void SetEffect() {}

	void		 SetPosition(int iX, int iY);
	void		 GetPosition(int *OutX, int *OutY);
	int			 GetCurX();
	int			 GetCurY();

	BOOL		 IsEndFrame();
	void		 NextFrame();		// 애니메이션 프레임을 넘겨주는 역할 Action()에서 호출

	void		 SetSprite(int iSpriteStart, int iSpriteMax, int iFrameDelay);		// 해당 액션이 끝났을때 강제로 STAND로 돌아가게 만들어야 패킷 통신에 적합
	int			 GetSprite();

protected:
	BOOL	_bEndFrame;		// 애니메이션 끝 도달시 True
	int		_iSpriteStart;		//  Now가 End에 도달하면 m_bEndFrame이 True로 바뀌고 Start로 돌아감
	int		_iSpriteNow;
	int		_iSpriteEnd;
	int		_iDelayCount;
	int		_iFrameDelay;

	DWORD	_dwActionInput;

	__int64 _iObjectType;
	__int64 _iObjectID;
	int		_CurX;
	int		_CurY;
	bool	_bFlag;
};

#endif