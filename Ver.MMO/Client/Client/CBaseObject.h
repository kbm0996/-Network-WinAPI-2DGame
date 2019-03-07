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
	void		 NextFrame();		// �ִϸ��̼� �������� �Ѱ��ִ� ���� Action()���� ȣ��

	void		 SetSprite(int iSpriteStart, int iSpriteMax, int iFrameDelay);		// �ش� �׼��� �������� ������ STAND�� ���ư��� ������ ��Ŷ ��ſ� ����
	int			 GetSprite();

protected:
	BOOL	_bEndFrame;		// �ִϸ��̼� �� ���޽� True
	int		_iSpriteStart;		//  Now�� End�� �����ϸ� m_bEndFrame�� True�� �ٲ�� Start�� ���ư�
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