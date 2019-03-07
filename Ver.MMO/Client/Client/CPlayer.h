#ifndef __PLAYER_OBJ__
#define __PLAYER_OBJ__

class CPlayer : public CBaseObject
{
public:
	CPlayer(int iX, int iY, BOOL isPlayer = FALSE, __int64 Type = e_OBJECT_TYPE::eTYPE_PLAYER, __int64 ID = 0, __int64 HP = 100, int Dir = dfDIR_RIGHT);
	~CPlayer();

	void	Action();
	bool	Draw(BYTE *bypDest, int iDestWidth, int iDestHeight, int iDestPitch);

	void	SetHP(int iHP);
	__int64 GetHP();

	void	SetDirection(int iDir);
	int		GetDirection();

	void	SetEffect();
	void	SetActionAttack1();
	void	SetActionAttack2();
	void	SetActionAttack3();
	void	SetActionMove();
	void	SetActionStand();

protected:
	BOOL	_bPlayerCharacter;	// ���� ������ �������� �޾ƿ� ���� ������ �Ǵ�
	__int64	_iHP;
	int		_iDirCur;
	int		_iDirOld;

	BOOL	_bEffect;
	DWORD	_dwActionCur;		// �׼��� �����Ҷ� ��Ŷ ����
	DWORD	_dwActionOld;		// �̸� �Ǵ��ϱ� ���� ����
};

#endif