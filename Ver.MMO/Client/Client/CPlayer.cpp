#include "stdafx.h"
#include "CPlayer.h"

CPlayer::CPlayer(int iX, int iY, BOOL isPlayer, __int64 Type, __int64 ID, __int64 HP, int Dir)
{
	_CurX = iX;
	_CurY = iY;
	_bPlayerCharacter = isPlayer;
	_iObjectType = Type;
	_iObjectID = ID;
	_iHP = HP;

	_bEffect = FALSE;

	_dwActionInput = dfACTION_STAND;
	_iDirCur = Dir;
	_iDirOld = _iDirCur;

	SetActionStand();
}

CPlayer::~CPlayer()
{
}

// 1 Action�� �޾� ����� �����ϴ� �κ�,  2 �ִϸ��̼��� �����ϴ� �κ�, 3 ��ǥ ���� �� �׼��� ���ϴ� �κ����� ��������.
void CPlayer::Action()
{
	// ������ �ѱ��
	NextFrame();

	////////////////////////////////////////////////////////////////////////////////////
	// ���� �ִϸ��̼� ���� �ٸ� �ൿ �Ұ���
	// ������ üũ�Ͽ� Ÿ�� ����Ʈ ����
	////////////////////////////////////////////////////////////////////////////////////
	// ��Ʈ��ũ ������ ���� ���� ����ؼ� ��� �÷��̾�� �ִϸ��̼� ������ ��Ȳ������ �ٸ� �ൿ�� �����ϰ� �ؾ��Ѵ�.
	if (!_bPlayerCharacter && _dwActionInput != _dwActionOld)
	{
		_dwActionOld = _dwActionCur;
		_dwActionCur = _dwActionInput;
		if (_dwActionOld > 7)
			SetActionMove();
	}
	else if (_dwActionOld == dfACTION_ATTACK1 || _dwActionOld == dfACTION_ATTACK2 || _dwActionOld == dfACTION_ATTACK3)
	{
		if (IsEndFrame())
		{
			SetActionStand();
			// ������ ������ �׼��� STAND�� �ٲ㼭 �������� ���ݽ� �������� �����ϵ��� �Ѵ�.
			_dwActionOld = dfACTION_STAND;
			_dwActionInput = dfACTION_STAND;
		}

		////////////////////////////////////////////////////////////////////////////////////
		// ��ȿ Ÿ�ݽ� ����Ʈ ����
		// 
		////////////////////////////////////////////////////////////////////////////////////
		if (_bEffect == TRUE)
		{
			switch (_dwActionOld)
			{
			case dfACTION_ATTACK1:
				if (_iDelayCount == 0)
				{
					if (_iDirCur == dfDIR_LEFT)
					{
						CEffectObject* Effect = new CEffectObject(_CurX - 71, _CurY - 60);
						g_lst.push_back((CBaseObject*)Effect);
					}
					else
					{
						CEffectObject* Effect = new CEffectObject(_CurX + 71, _CurY - 60);
						g_lst.push_back((CBaseObject*)Effect);
					}
					_bEffect = FALSE;
				}
				break;
			case dfACTION_ATTACK2:
				if (_iDelayCount == 1)
				{
					if (_iDirCur == dfDIR_LEFT)
					{
						CEffectObject* Effect = new CEffectObject(_CurX - 71, _CurY - 60);
						g_lst.push_back((CBaseObject*)Effect);
					}
					else
					{
						CEffectObject* Effect = new CEffectObject(_CurX + 71, _CurY - 60);
						g_lst.push_back((CBaseObject*)Effect);
					}
					_bEffect = FALSE;
				}
				break;
			case dfACTION_ATTACK3:
				if (_iDelayCount == 3)
				{
					if (_iDirCur == dfDIR_LEFT)
					{
						CEffectObject* Effect = new CEffectObject(_CurX - 71, _CurY - 60);
						g_lst.push_back((CBaseObject*)Effect);
					}
					else
					{
						CEffectObject* Effect = new CEffectObject(_CurX + 71, _CurY - 60);
						g_lst.push_back((CBaseObject*)Effect);
					}
					_bEffect = FALSE;
				}
				break;
			}
		}
		return;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// 1. Action�� �޾� ������ �����ϴ� �κ�
	//
	////////////////////////////////////////////////////////////////////////////////////
	//if (_CurY != dfRANGE_MOVE_TOP || _CurY != dfRANGE_MOVE_BOTTOM || _CurX != dfRANGE_MOVE_LEFT || _CurX != dfRANGE_MOVE_RIGHT)
	{
		_iDirOld = _iDirCur;
		switch (_dwActionInput)
		{
		case dfACTION_MOVE_LU:
			_iDirCur = dfDIR_LEFT;
			break;
		case dfACTION_MOVE_RU:
			_iDirCur = dfDIR_RIGHT;
			break;
		case dfACTION_MOVE_LD:
			_iDirCur = dfDIR_LEFT;
			break;
		case dfACTION_MOVE_RD:
			_iDirCur = dfDIR_RIGHT;
			break;
		case dfACTION_MOVE_LL:
			_iDirCur = dfDIR_LEFT;
			break;
		case dfACTION_MOVE_RR:
			_iDirCur = dfDIR_RIGHT;
			break;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////
	// 2 �ִϸ��̼� ����
	// @ Ŭ���̾�Ʈ ��Ŷ ����?
	////////////////////////////////////////////////////////////////////////////////////
	_dwActionCur = _dwActionInput;

	st_NETWORK_PACKET_HEADER pHeader;
	mylib::CSerialBuffer clPacket;
	switch (_dwActionCur)
	{
	case dfACTION_ATTACK1:
		SetActionAttack1();

		if (_bPlayerCharacter)
		{
			if (_dwActionOld < 8)	// ������  ��ġ ��ũ�� ���߱� ���� ���� ������ �����̰� �־��ٸ� ���� ��ǥ�� �ѱ�� STOP ��Ŷ ����
			{
				mpMoveStop(&pHeader, &clPacket, _iDirCur, _CurX, _CurY);
				SendPacket(&pHeader, &clPacket);
			}

			if (_dwActionOld != _dwActionCur)
			{
				mpAttack1(&pHeader, &clPacket, _iDirCur, _CurX, _CurY);
				SendPacket(&pHeader, &clPacket);
			}
		}
		break;
	case dfACTION_ATTACK2:
		SetActionAttack2();

		if (_bPlayerCharacter)
		{
			if (_dwActionOld < 8)
			{
				mpMoveStop(&pHeader, &clPacket, _iDirCur, _CurX, _CurY);
				SendPacket(&pHeader, &clPacket);
			}

			if (_dwActionOld != _dwActionCur)
			{
				mpAttack2(&pHeader, &clPacket, _iDirCur, _CurX, _CurY);
				SendPacket(&pHeader, &clPacket);
			}
		}
		break;
	case dfACTION_ATTACK3:
		SetActionAttack3();

		if (_bPlayerCharacter)
		{
			if (_dwActionOld < 8)
			{
				mpMoveStop(&pHeader, &clPacket, _iDirCur, _CurX, _CurY);
				SendPacket(&pHeader, &clPacket);
			}

			if (_dwActionOld != _dwActionCur)
			{
				mpAttack3(&pHeader, &clPacket, _iDirCur, _CurX, _CurY);
				SendPacket(&pHeader, &clPacket);
			}
		}
		break;
	case dfACTION_STAND:
		if (_dwActionOld != dfACTION_STAND)
			SetActionStand();

		if (_bPlayerCharacter)
		{
			if (_dwActionOld != _dwActionCur)
			{
				mpMoveStop(&pHeader, &clPacket, _iDirCur, _CurX, _CurY);
				SendPacket(&pHeader, &clPacket);
			}
		}
		break;

	case dfACTION_MOVE_LU:
		if (_iDirCur != _iDirOld || _dwActionOld == dfACTION_STAND)
			SetActionMove();

		if (_bPlayerCharacter)
		{
			if (_dwActionOld != _dwActionCur)
			{
				mpMoveStart(&pHeader, &clPacket, dfPACKET_MOVE_DIR_LU, _CurX, _CurY);
				SendPacket(&pHeader, &clPacket);
			}
		}
		break;

	case dfACTION_MOVE_RU:
		if (_iDirCur != _iDirOld || _dwActionOld == dfACTION_STAND)
			SetActionMove();

		if (_bPlayerCharacter)
		{
			if (_dwActionOld != _dwActionCur)
			{
				mpMoveStart(&pHeader, &clPacket, dfPACKET_MOVE_DIR_RU, _CurX, _CurY);
				SendPacket(&pHeader, &clPacket);
			}
		}
		break;

	case dfACTION_MOVE_LD:
		if (_iDirCur != _iDirOld || _dwActionOld == dfACTION_STAND)
			SetActionMove();

		if (_bPlayerCharacter)
		{
			if (_dwActionOld != _dwActionCur)
			{
				mpMoveStart(&pHeader, &clPacket, dfPACKET_MOVE_DIR_LD, _CurX, _CurY);
				SendPacket(&pHeader, &clPacket);
			}
		}
		break;

	case dfACTION_MOVE_RD:
		if (_iDirCur != _iDirOld || _dwActionOld == dfACTION_STAND)
			SetActionMove();

		if (_bPlayerCharacter)
		{
			if (_dwActionOld != _dwActionCur)
			{
				mpMoveStart(&pHeader, &clPacket, dfPACKET_MOVE_DIR_RD, _CurX, _CurY);
				SendPacket(&pHeader, &clPacket);
			}
		}
		break;

	case dfACTION_MOVE_UU:
		if (_iDirCur != _iDirOld || _dwActionOld == dfACTION_STAND)
			SetActionMove();

		if (_bPlayerCharacter)
		{
			if (_dwActionOld != _dwActionCur)
			{
				mpMoveStart(&pHeader, &clPacket, dfPACKET_MOVE_DIR_UU, _CurX, _CurY);
				SendPacket(&pHeader, &clPacket);
			}
		}
		break;

	case dfACTION_MOVE_DD:
		if (_iDirCur != _iDirOld || _dwActionOld == dfACTION_STAND)
			SetActionMove();

		if (_bPlayerCharacter)
		{
			if (_dwActionOld != _dwActionCur)
			{
				mpMoveStart(&pHeader, &clPacket, dfPACKET_MOVE_DIR_DD, _CurX, _CurY);
				SendPacket(&pHeader, &clPacket);
			}
		}
		break;

	case dfACTION_MOVE_LL:
		if (_iDirCur != _iDirOld || _dwActionOld == dfACTION_STAND)
			SetActionMove();

		if (_bPlayerCharacter)
		{
			if (_dwActionOld != _dwActionCur)
			{
				mpMoveStart(&pHeader, &clPacket, dfPACKET_MOVE_DIR_LL, _CurX, _CurY);
				SendPacket(&pHeader, &clPacket);
			}
		}
		break;

	case dfACTION_MOVE_RR:
		if (_iDirCur != _iDirOld || _dwActionOld == dfACTION_STAND)
			SetActionMove();

		if (_bPlayerCharacter)
		{
			if (_dwActionOld != _dwActionCur)
			{
				mpMoveStart(&pHeader, &clPacket, dfPACKET_MOVE_DIR_RR, _CurX, _CurY);
				SendPacket(&pHeader, &clPacket);
			}
		}
		break;

	default:
		if (_dwActionOld == dfACTION_STAND || _iDirCur != _iDirOld)
			SetActionMove();
		else
			SetActionStand();

		break;
	}
	_dwActionOld = _dwActionCur;

	////////////////////////////////////////////////////////////////////////////////////
	// 3. ��ǥ �̵�
	//
	////////////////////////////////////////////////////////////////////////////////////
	if (_CurY > dfRANGE_MOVE_TOP || _CurY < dfRANGE_MOVE_BOTTOM || _CurX > dfRANGE_MOVE_LEFT || _CurX < dfRANGE_MOVE_RIGHT)
	{
		switch (_dwActionCur)
		{
		case dfACTION_MOVE_LU:
			if (_CurY != dfRANGE_MOVE_TOP && _CurX != dfRANGE_MOVE_LEFT)
			{
				_CurX -= dfSPEED_PLAYER_X;
				_CurY -= dfSPEED_PLAYER_Y;
			}
			break;

		case dfACTION_MOVE_RU:
			if (_CurY != dfRANGE_MOVE_TOP && _CurX != dfRANGE_MOVE_RIGHT)
			{
				_CurX += dfSPEED_PLAYER_X;
				_CurY -= dfSPEED_PLAYER_Y;
			}
			break;

		case dfACTION_MOVE_LD:
			if (_CurY != dfRANGE_MOVE_BOTTOM && _CurX != dfRANGE_MOVE_LEFT)
			{
				_CurX -= dfSPEED_PLAYER_X;
				_CurY += dfSPEED_PLAYER_Y;
			}
			break;

		case dfACTION_MOVE_RD:
			if (_CurY != dfRANGE_MOVE_BOTTOM && _CurX != dfRANGE_MOVE_RIGHT)
			{
				_CurX += dfSPEED_PLAYER_X;
				_CurY += dfSPEED_PLAYER_Y;
			}
			break;

		case dfACTION_MOVE_UU:
			_CurY -= dfSPEED_PLAYER_Y;
			break;

		case dfACTION_MOVE_DD:
			_CurY += dfSPEED_PLAYER_Y;
			break;

		case dfACTION_MOVE_LL:
			_CurX -= dfSPEED_PLAYER_X;
			break;

		case dfACTION_MOVE_RR:
			_CurX += dfSPEED_PLAYER_X;
			break;
		}
		if (_CurX < dfRANGE_MOVE_LEFT)		_CurX = dfRANGE_MOVE_LEFT;
		if (_CurY < dfRANGE_MOVE_TOP)		_CurY = dfRANGE_MOVE_TOP;
		if (_CurX > dfRANGE_MOVE_RIGHT)	_CurX = dfRANGE_MOVE_RIGHT - 1;
		if (_CurY > dfRANGE_MOVE_BOTTOM)	_CurY = dfRANGE_MOVE_BOTTOM - 1;
	}
}

bool CPlayer::Draw(BYTE *bypDest, int iDestWidth, int iDestHeight, int iDestPitch)
{
	int iScrollX, iScrollY;
	iScrollX = g_TileMap.GetScrollX();
	iScrollY = g_TileMap.GetScrollY();

	g_cSprite.DrawSprite50(eSHADOW, _CurX - iScrollX, _CurY - iScrollY, bypDest, iDestWidth, iDestHeight, iDestPitch);
	if (!_bPlayerCharacter)
		g_cSprite.DrawSpriteRed(GetSprite(), _CurX - iScrollX, _CurY - iScrollY, bypDest, iDestWidth, iDestHeight, iDestPitch);
	else
		g_cSprite.DrawSprite(GetSprite(), _CurX - iScrollX, _CurY - iScrollY, bypDest, iDestWidth, iDestHeight, iDestPitch);
	g_cSprite.DrawSprite(eGUAGE_HP, _CurX - iScrollX - 35, _CurY - iScrollY + 9, bypDest, iDestWidth, iDestHeight, iDestPitch, GetHP());

	return true;
}

void CPlayer::SetHP(int iHP)
{
	_iHP = iHP;
}

__int64 CPlayer::GetHP()
{
	return _iHP;
}

void CPlayer::SetDirection(int iDir)
{
	_iDirCur = iDir;
}

int CPlayer::GetDirection()
{
	return _iDirCur;
}

void CPlayer::SetEffect()
{
	_bEffect = TRUE;
}

void CPlayer::SetActionAttack1()
{
	if (_iDirCur == dfDIR_LEFT)		SetSprite(ePLAYER_ATTACK1_L01, ePLAYER_ATTACK1_L_MAX, dfDELAY_ATTACK1);
	else if (_iDirCur == dfDIR_RIGHT)	SetSprite(ePLAYER_ATTACK1_R01, ePLAYER_ATTACK1_R_MAX, dfDELAY_ATTACK1);
}

void CPlayer::SetActionAttack2()
{
	if (_iDirCur == dfDIR_LEFT)		SetSprite(ePLAYER_ATTACK2_L01, ePLAYER_ATTACK2_L_MAX, dfDELAY_ATTACK2);
	else if (_iDirCur == dfDIR_RIGHT)	SetSprite(ePLAYER_ATTACK2_R01, ePLAYER_ATTACK2_R_MAX, dfDELAY_ATTACK2);
}

void CPlayer::SetActionAttack3()
{
	if (_iDirCur == dfDIR_LEFT)		SetSprite(ePLAYER_ATTACK3_L01, ePLAYER_ATTACK3_L_MAX, dfDELAY_ATTACK3);
	else if (_iDirCur == dfDIR_RIGHT)	SetSprite(ePLAYER_ATTACK3_R01, ePLAYER_ATTACK3_R_MAX, dfDELAY_ATTACK3);
}

void CPlayer::SetActionMove()
{
	if (_dwActionCur == dfACTION_MOVE_DD || _dwActionCur == dfACTION_MOVE_UU)
	{
		if (_iDirCur == dfDIR_LEFT)			SetSprite(ePLAYER_MOVE_L01, ePLAYER_MOVE_L_MAX, dfDELAY_MOVE);
		else if (_iDirCur == dfDIR_RIGHT)		SetSprite(ePLAYER_MOVE_R01, ePLAYER_MOVE_R_MAX, dfDELAY_MOVE);
	}
	else if (_dwActionCur == dfACTION_MOVE_LD || _dwActionCur == dfACTION_MOVE_LU || _dwActionCur == dfACTION_MOVE_LL)
	{
		SetSprite(ePLAYER_MOVE_L01, ePLAYER_MOVE_L_MAX, dfDELAY_MOVE);
	}
	else if (_dwActionCur == dfACTION_MOVE_RD || _dwActionCur == dfACTION_MOVE_RU || _dwActionCur == dfACTION_MOVE_RR)
	{
		SetSprite(ePLAYER_MOVE_R01, ePLAYER_MOVE_R_MAX, dfDELAY_MOVE);
	}
}

void CPlayer::SetActionStand()
{
	if (_iDirCur == dfDIR_LEFT)		SetSprite(ePLAYER_STAND_L01, ePLAYER_STAND_L_MAX, dfDELAY_STAND);
	else if (_iDirCur == dfDIR_RIGHT)	SetSprite(ePLAYER_STAND_R01, ePLAYER_STAND_R_MAX, dfDELAY_STAND);
}
